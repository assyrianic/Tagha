#include <stdlib.h>
#include <stdio.h>
#ifdef OS_WINDOWS
	#define HARBOL_LIB
#endif
#include "harbol.h"


static size_t AlignSize(const size_t size, const size_t align)
{
	return (size + (align-1)) & -align;
}

#ifdef POOL_NO_MALLOC
HARBOL_EXPORT void harbol_mempool_init(struct HarbolMemoryPool *const mempool)
#else
HARBOL_EXPORT void harbol_mempool_init(struct HarbolMemoryPool *const mempool, const size_t size)
#endif
{
	if( !mempool )
		return;
	
	memset(mempool, 0, sizeof *mempool);
	
#ifdef POOL_NO_MALLOC
	// allocate the mempool bottom towards the very end of the mempool.
	mempool->HeapBottom = mempool->HeapMem + POOL_HEAPSIZE;
	mempool->HeapSize = POOL_HEAPSIZE;
#else
	// align the mempool size to at least the size of an alloc node.
	mempool->HeapSize = AlignSize(size, sizeof(struct HarbolAllocNode));
	mempool->HeapMem = calloc(mempool->HeapSize+1, sizeof *mempool->HeapMem);
	//printf("harbol_mempool_init :: mempool->HeapSize == %zu\n", mempool->HeapSize);
	if( !mempool->HeapMem )
		return;
	
	mempool->HeapBottom = mempool->HeapMem + mempool->HeapSize;
#endif
}

HARBOL_EXPORT void harbol_mempool_del(struct HarbolMemoryPool *const mempool)
{
	if( !mempool )
		return;
	
#ifndef POOL_NO_MALLOC
	if( mempool->HeapMem )
		free(mempool->HeapMem);
#endif
	memset(mempool, 0, sizeof *mempool);
}

HARBOL_EXPORT void *harbol_mempool_alloc(struct HarbolMemoryPool *const restrict mempool, const size_t size)
{
	if( !mempool || !size )
		return NULL;
	
	struct HarbolAllocNode *NewMem = NULL;
	const size_t AllocSize = size + sizeof *NewMem;
	
	// if the freelist is valid, let's allocate FROM the freelist then!
	if( mempool->FreeList ) {
		struct HarbolAllocNode **FreeNode = NULL;
		for(
			// start at the freelist head.
			FreeNode = &mempool->FreeList ;
			// make sure it's smaller than the allocation size.
			*FreeNode && (*FreeNode)->Size < AllocSize ;
			// go to the next node if it's smaller than the needed size.
			FreeNode = &(*FreeNode)->NextFree
		);
		
		// if we got here, that means we found a size that's good.
		if( *FreeNode ) {
			struct HarbolAllocNode *const n = *FreeNode;
			//puts("harbol_mempool_alloc :: *FreeNode is valid.");
			if( (uintptr_t)n < (uintptr_t)mempool->HeapMem || ((uintptr_t)n - (uintptr_t)mempool->HeapMem) >= mempool->HeapSize || n->Size >= mempool->HeapSize || !n->Size )
				return NULL;
			
			const size_t Memory_Split = sizeof(intptr_t);
			if( n->Size < AllocSize + Memory_Split ) {
				//puts("harbol_mempool_alloc :: allocating close sized node");
				/* close in size - reduce fragmentation by not splitting */
				NewMem = *FreeNode;
				*FreeNode = NewMem->NextFree;
				NewMem->NextFree = NULL;
				mempool->FreeNodes--;
			}
			else {
				/* split this big memory chunk */
				//puts("harbol_mempool_alloc :: allocating split up node");
				NewMem = (struct HarbolAllocNode *)( (uint8_t *)n + (n->Size - AllocSize) );
				//if( (uintptr_t)NewMem < (uintptr_t)mempool->HeapMem || ((uintptr_t)NewMem - (uintptr_t)mempool->HeapMem) >= mempool->HeapSize )
				//	return NULL;
				
				n->Size -= AllocSize;
				NewMem->Size = AllocSize;
				NewMem->NextFree = NULL;
			}
		}
	}
	if( !NewMem ) {
		//puts("harbol_mempool_alloc :: allocating from main mempool.");
		// couldn't allocate from a freelist
		if( mempool->HeapBottom-AllocSize < mempool->HeapMem )
			return NULL;
		
		// allocate from available mempool.
		// subtract allocation size from the mempool.
		mempool->HeapBottom -= AllocSize;
		// use the available mempool space as the new node.
		NewMem = (struct HarbolAllocNode *)mempool->HeapBottom;
		NewMem->Size = AllocSize;
		NewMem->NextFree = NULL;
	}
	// here's the structure of the allocation block.
	// --------------
	// | mem size   | lowest addr of block
	// | next node  |
	// --------------
	// | allocated  |
	// |   memory   |
	// |   space    | highest addr of block
	// --------------
	void *restrict ReturnMem = (uint8_t *)NewMem + sizeof *NewMem;
	memset(ReturnMem, 0, NewMem->Size - sizeof *NewMem);
	//printf("harbol_mempool_alloc :: AllocSize - sizeof *NewMem == %zu\n", AllocSize - sizeof *NewMem);
	return ReturnMem;
}

HARBOL_EXPORT void *harbol_mempool_realloc(struct HarbolMemoryPool *const restrict mempool, void *ptr, const size_t newsize)
{
	if( !ptr ) // NULL ptr should make this work like regular Allocation.
		return harbol_mempool_alloc(mempool, newsize);
	else if( (uintptr_t)ptr <= (uintptr_t)mempool->HeapMem )
		return NULL;
	
	struct HarbolAllocNode *ptr_node = (ptr - sizeof *ptr_node);
	const size_t node_size = sizeof *ptr_node;
	
	void *resized_block = harbol_mempool_alloc(mempool, newsize);
	if( !resized_block )
		return NULL;
	
	struct HarbolAllocNode *resized_node = (resized_block - sizeof *resized_node);
	memmove(resized_block, ptr, ptr_node->Size > resized_node->Size ? (resized_node->Size - node_size) : (ptr_node->Size - node_size));
	harbol_mempool_dealloc(mempool, ptr);
	return resized_block;
}

HARBOL_EXPORT void harbol_mempool_dealloc(struct HarbolMemoryPool *const restrict mempool, void *ptr)
{
	if( !mempool || !ptr || (uintptr_t)ptr <= (uintptr_t)mempool->HeapMem )
		return;
	
	// behind the actual pointer data is the allocation info.
	struct HarbolAllocNode *restrict MemNode = (ptr - sizeof *MemNode);
	if( !MemNode->Size || (MemNode->Size > mempool->HeapSize) || (((uintptr_t)ptr - (uintptr_t)mempool->HeapMem) > mempool->HeapSize) )
		return;
	
	//memset(ptr, 0, MemNode->Size - sizeof *MemNode);
	if( (uintptr_t)MemNode == (uintptr_t)mempool->HeapBottom ) {
		mempool->HeapBottom += MemNode->Size;
	}
	else if( !mempool->FreeList || ( (uintptr_t)mempool->FreeList >= (uintptr_t)mempool->HeapMem && (uintptr_t)mempool->FreeList - (uintptr_t)mempool->HeapMem < mempool->HeapSize ) ) {
		/* put it in the big memory freelist */
		// first check if we already have the pointer in the freelist.
		for( struct HarbolAllocNode *n=mempool->FreeList ; n ; n=n->NextFree )
			// if memnode's data is part of the freelist, return as we've already freed it.
			if( (uintptr_t)n == (uintptr_t)MemNode )
				return;
		
		MemNode->NextFree = mempool->FreeList;
		mempool->FreeList = MemNode;
		mempool->FreeNodes++;
		if( mempool->FreeNodes>10 )
			harbol_mempool_defrag(mempool);
	}
}

HARBOL_EXPORT void harbol_mempool_destroy(struct HarbolMemoryPool *const restrict mempool, void *ptr)
{
	if( !mempool || !ptr )
		return;
	
	void **const restrict ptrref = ptr;
	harbol_mempool_dealloc(mempool, *ptrref), *ptrref=NULL;
}

HARBOL_EXPORT size_t harbol_mempool_get_remaining(const struct HarbolMemoryPool *const mempool)
{
	if( !mempool || !mempool->HeapMem )
		return 0;
	size_t total_remaining = (uintptr_t)mempool->HeapBottom - (uintptr_t)mempool->HeapMem;
	if( mempool->FreeList )
		for( struct HarbolAllocNode *n=mempool->FreeList ; n ; n=n->NextFree )
			total_remaining += n->Size;
	return total_remaining;
}

HARBOL_EXPORT size_t harbol_mempool_get_heap_size(const struct HarbolMemoryPool *const mempool)
{
	return !mempool || !mempool->HeapMem ? 0 : mempool->HeapSize;
}

HARBOL_EXPORT struct HarbolAllocNode *harbol_mempool_get_freelist(const struct HarbolMemoryPool *const mempool)
{
	return !mempool ? NULL : mempool->FreeList;
}

HARBOL_EXPORT bool harbol_mempool_defrag(struct HarbolMemoryPool *const mempool)
{
	if( !mempool || !mempool->FreeList )
		return false;
	///*
	// if the memory pool has been entirely released, fully defrag it.
	if( mempool->HeapSize == harbol_mempool_get_remaining(mempool) ) {
		mempool->FreeList = NULL;
		memset(mempool->HeapMem, 0, mempool->HeapSize);
		mempool->HeapBottom = mempool->HeapMem + mempool->HeapSize;
		mempool->FreeNodes = 0;
		return true;
	}
	//*/
	// Check if the nodes are close to the mempool's bottom "stack".
	// if it is, pop the Allocation Block && increase the mempool bottom.
	struct HarbolAllocNode
		**iter = &mempool->FreeList,
		*restrict prev = NULL,
		*restrict next = NULL
	;
	while( *iter ) {
		struct HarbolAllocNode *curr = *iter;
		next = curr->NextFree;
		
		// Patch: last node's sizing gets zeroed somehow but the node is never removed.
		if( curr && !next && !curr->Size ) {
			prev->NextFree = *iter = NULL;
			mempool->FreeNodes--;
			break;
		}
		
		if( (uintptr_t)curr == (uintptr_t)mempool->HeapBottom ) {
			*iter = next;
			mempool->HeapBottom += curr->Size;
			mempool->FreeNodes--;
			continue;
		}
		else if( (uintptr_t)curr + curr->Size==(uintptr_t)next ) {
			curr->Size += next->Size;
			curr->NextFree = next->NextFree;
			mempool->FreeNodes--;
			continue;
		}
		
		/*
			prev - X > [size:16 | next:A | memory block]
			next - B > [size:12 | next:XYZ | memory block]
			curr - A > [size:8 | next:B | memory block]
			
			issue: we need to know the node previous to A...
			Add B's size with A.
			set the prev node's Next ptr to point to B.
			set the curr node to B
			
			result:
			prev - X > [size:16 | next:B | memory block]
			curr - B > [size:20 | next:XYZ | memory block]
		*/
		else if( prev && next && (uintptr_t)curr - next->Size==(uintptr_t)next ) {
			next->Size += curr->Size;
			prev->NextFree = next;
			*iter = next;
			mempool->FreeNodes--;
			continue;
		}
		prev = curr;
		iter = &curr->NextFree;
	}
	/*
	for( struct HarbolAllocNode *n=mempool->FreeList ; n ; n = n->NextFree )
		printf(!n->Size ? "n == %zu | next == %zu | size of 0.\n" : "n == %zu | next == %zu\n", (uintptr_t)n, (uintptr_t)n->NextFree);
	printf("HeapBottom == %zu\n", (uintptr_t)mempool->HeapBottom);
	*/
	return true;
}
