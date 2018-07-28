
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "dsc.h"

static inline size_t AlignSize(const size_t size, const size_t align)
{
	return (size + (align-1)) & -align;
}

#ifdef DSC_HEAP_NO_MALLOC
void Heap_Init(struct Heap *const restrict heap)
#else
void Heap_Init(struct Heap *const restrict heap, const size_t size)
#endif
{
	if( !heap )
		return;
	
	*heap = (struct Heap){0};
	
#ifdef DSC_HEAP_NO_MALLOC
	// allocate the heap bottom towards the very end of the heap.
	heap->HeapBottom = heap->HeapMem + (DSC_HEAPSIZE-1);
	heap->HeapSize = DSC_HEAPSIZE;
#else
	// align the heap size to at least the size of an alloc node.
	heap->HeapSize = AlignSize(size, sizeof(struct AllocNode));
	heap->HeapMem = calloc(heap->HeapSize, sizeof *heap->HeapMem);
	//printf("Heap_Init :: heap->HeapSize == %zu\n", heap->HeapSize);
	if( !heap->HeapMem )
		return;
	
	heap->HeapBottom = heap->HeapMem + (heap->HeapSize-1);
#endif
}

void Heap_Del(struct Heap *const restrict heap)
{
	if( !heap )
		return;
	
#ifndef DSC_HEAP_NO_MALLOC
	if( heap->HeapMem )
		free(heap->HeapMem);
#endif
	*heap = (struct Heap){0};
}

void *Heap_Alloc(struct Heap *const restrict heap, const size_t size)
{
	if( !heap or !size )
		return NULL;
	
	struct AllocNode *NewMem = NULL;
	size_t AllocSize = size + sizeof *NewMem;
	void *ReturnMem = NULL;
	
	// if the freelist is valid, let's allocate FROM the freelist then!
	if( heap->FreeList ) {
		struct AllocNode **FreeNode = NULL;
		for(
			// start at the freelist head.
			FreeNode = &heap->FreeList ;
			// make sure it's smaller than the allocation size.
			*FreeNode and (*FreeNode)->Size < AllocSize ;
			// go to the next node if it's smaller than the needed size.
			FreeNode = &(*FreeNode)->NextFree
		);
		
		// if we got here, that means we found a size that's good.
		if( *FreeNode ) {
			struct AllocNode *n = *FreeNode;
			//puts("Heap_Alloc :: *FreeNode is valid.");
			assert(
				(uintptr_t)n >= (uintptr_t)heap->HeapMem
				and ((uintptr_t)n - (uintptr_t)heap->HeapMem) < heap->HeapSize
			);
			
			assert(n->Size < heap->HeapSize and n->Size > 0);
			const size_t Memory_Split = 16;
			if( n->Size < AllocSize + Memory_Split ) {
				//puts("Heap_Alloc :: allocating close sized node");
				/* close in size - reduce fragmentation by not splitting */
				NewMem = *FreeNode;
				assert(
					(uintptr_t)NewMem >= (uintptr_t)heap->HeapMem
					and (uintptr_t)NewMem - (uintptr_t)heap->HeapMem < heap->HeapSize
				);
				*FreeNode = NewMem->NextFree;
			}
			else {
				/* split this big memory chunk */
				//puts("Heap_Alloc :: allocating split up node");
				NewMem = (struct AllocNode *)( (uint8_t *)n + (n->Size - AllocSize) );
				assert(
					(uintptr_t)NewMem >= (uintptr_t)heap->HeapMem
					and (uintptr_t)NewMem - (uintptr_t)heap->HeapMem < heap->HeapSize
				);
				n->Size -= AllocSize;
				NewMem->Size = AllocSize;
			}
		}
	}
	if( !NewMem ) {
		//puts("Heap_Alloc :: allocating from main heap.");
		// couldn't allocate from a freelist
		if( heap->HeapBottom-AllocSize < heap->HeapMem )
			return NULL;
		
		// allocate from available heap.
		// subtract allocation size from the heap.
		heap->HeapBottom -= AllocSize;
		// use the available heap space as the new node.
		NewMem = (struct AllocNode *)heap->HeapBottom;
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
	ReturnMem = (uint8_t *)NewMem + sizeof *NewMem;
	//printf("Heap_Alloc :: AllocSize - sizeof *NewMem == %zu\n", AllocSize - sizeof *NewMem);
	memset(ReturnMem, 0, AllocSize - sizeof *NewMem);
	return ReturnMem;
}

void Heap_Release(struct Heap *const restrict heap, void *ptr)
{
	if( !ptr )
		return;
	
	// behind the actual pointer data is the allocation info.
	struct AllocNode *MemNode = ptr - sizeof *MemNode;
	
	//printf("Heap_Release :: (uintptr_t)ptr: '%zu' >= (uintptr_t)heap->HeapMem: '%zu' == %u\n", (uintptr_t)ptr, (uintptr_t)heap->HeapMem, (uintptr_t)ptr >= (uintptr_t)heap->HeapMem);
	assert( (uintptr_t)ptr >= (uintptr_t)heap->HeapMem );
	
	//printf("Heap_Release :: (uintptr_t)ptr: '%zu' - heap->HeapMem: '%zu' < heap->HeapSize: '%zu' == %u | '%zu'\n", (uintptr_t)ptr, (uintptr_t)heap->HeapMem, heap->HeapSize, ((uintptr_t)ptr - (uintptr_t)heap->HeapMem) <= heap->HeapSize, (uintptr_t)ptr - (uintptr_t)heap->HeapMem);
	
	assert( (uintptr_t)ptr - (uintptr_t)heap->HeapMem <= heap->HeapSize );
	assert( MemNode->Size < heap->HeapSize and MemNode->Size > 0 );
	
	if( (uintptr_t)MemNode == (uintptr_t)heap->HeapBottom ) {
		heap->HeapBottom += MemNode->Size;
	}
	else {
		/* put it in the big memory freelist */
		assert(
			!heap->FreeList
			or ( (uintptr_t)heap->FreeList >= (uintptr_t)heap->HeapMem
				and (uintptr_t)heap->FreeList - (uintptr_t)heap->HeapMem < heap->HeapSize )
		);
		MemNode->NextFree = heap->FreeList;
		heap->FreeList = MemNode;
	}
}

size_t Heap_Remaining(const struct Heap *const restrict heap)
{
	if( !heap or !heap->HeapMem )
		return 0;
	size_t total_remaining = heap->HeapBottom - heap->HeapMem;
	if( heap->FreeList )
		for( struct AllocNode *n=heap->FreeList ; n ; n=n->NextFree )
			total_remaining += n->Size;
	return total_remaining;
}

size_t Heap_Size(const struct Heap *const restrict heap)
{
	return !heap or !heap->HeapMem ? 0 : heap->HeapSize;
}
struct AllocNode *Heap_GetFreeList(const struct Heap *const restrict heap)
{
	return !heap ? NULL : heap->FreeList;
}
