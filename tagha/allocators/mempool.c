#include "mempool.h"

#ifdef OS_WINDOWS
#	define HARBOL_LIB
#endif

HARBOL_EXPORT struct HarbolMemPool harbol_mempool_create(const size_t size)
{
	struct HarbolMemPool mempool = EMPTY_HARBOL_MEMPOOL;
	if( size==0 )
		return mempool;
	else {
		mempool.stack.size = size;
		mempool.stack.mem = malloc(mempool.stack.size * sizeof *mempool.stack.mem);
		if( mempool.stack.mem==NULL ) {
			mempool.stack.size = 0;
			return mempool;
		} else {
			mempool.stack.base = mempool.stack.mem + mempool.stack.size;
			return mempool;
		}
	}
}

HARBOL_EXPORT struct HarbolMemPool harbol_mempool_from_buffer(void *const buf, const size_t size)
{
	struct HarbolMemPool mempool = EMPTY_HARBOL_MEMPOOL;
	if( size==0 || size<=sizeof(struct HarbolMemNode) )
		return mempool;
	else {
		mempool.stack.size = size;
		mempool.stack.mem = buf;
		mempool.stack.base = mempool.stack.mem + mempool.stack.size;
		return mempool;
	}
}

HARBOL_EXPORT bool harbol_mempool_clear(struct HarbolMemPool *const mempool)
{
	if( mempool->stack.mem==NULL )
		return false;
	else {
		free(mempool->stack.mem);
		*mempool = (struct HarbolMemPool)EMPTY_HARBOL_MEMPOOL;
		return true;
	}
}

static NO_NULL struct HarbolMemNode *__iterate_freenodes(struct HarbolMemPool *const mempool, const size_t bytes)
{
	const uindex_t b = (bytes >> HARBOL_BUCKET_BITS) - 1;
	// check if we have a good sized node from the buckets.
	if( b < HARBOL_BUCKET_SIZE && mempool->buckets[b] != NULL && mempool->buckets[b]->size >= bytes ) {
		struct HarbolMemNode *new_mem = mempool->buckets[b];
		mempool->buckets[b] = mempool->buckets[b]->next;
		if( mempool->buckets[b] != NULL )
			mempool->buckets[b]->prev = NULL;
		return new_mem;
	} else if( mempool->freelist.len>0 ) {
		const size_t mem_split_threshold = 24;
		// if the freelist is valid, let's allocate FROM the freelist then!
		for( struct HarbolMemNode *inode = mempool->freelist.head; inode != NULL; inode = inode->next ) {
			if( inode->size < bytes )
				continue;
			else if( inode->size <= bytes + mem_split_threshold ) {
				// close in size - reduce fragmentation by not splitting.
				struct HarbolMemNode *new_mem = inode;
				inode->prev != NULL ? (inode->prev->next = inode->next) : (mempool->freelist.head = inode->next);
				inode->next != NULL ? (inode->next->prev = inode->prev) : (mempool->freelist.tail = inode->prev);
				if( mempool->freelist.head != NULL )
					mempool->freelist.head->prev = NULL;
				else mempool->freelist.tail = NULL;
				
				if( mempool->freelist.tail != NULL )
					mempool->freelist.tail->next = NULL;
				
				mempool->freelist.len--;
				return new_mem;
			} else {
				// split the memory chunk.
				struct HarbolMemNode *new_mem = (struct HarbolMemNode *)( (uint8_t *)inode + (inode->size - bytes) );
				inode->size -= bytes;
				new_mem->size = bytes;
				return new_mem;
			}
		}
		return NULL;
	}
	else return NULL;
}

HARBOL_EXPORT void *harbol_mempool_alloc(struct HarbolMemPool *const mempool, const size_t size)
{
	if( size==0 || size > mempool->stack.size )
		return NULL;
	else {
		// visual of the allocation block.
		// --------------
		// |  mem size  | lowest addr of block
		// |  next node | 12 bytes - 32 bit
		// |  prev node | 24 bytes - 64 bit
		// --------------
		// |   alloc'd  |
		// |   memory   |
		// |   space    | highest addr of block
		// --------------
		const size_t alloc_bytes = harbol_align_size(size + sizeof(struct HarbolMemNode), sizeof(intptr_t));
		struct HarbolMemNode *new_mem = __iterate_freenodes(mempool, alloc_bytes);
		if( new_mem==NULL ) {
			// not enough memory to support the size!
			if( mempool->stack.base - alloc_bytes < mempool->stack.mem )
				return NULL;
			else {
				// couldn't allocate from a freelist, allocate from available mempool.
				// subtract allocation size from the mempool.
				mempool->stack.base -= alloc_bytes;
				
				// use the available mempool space as the new node.
				new_mem = (struct HarbolMemNode *)mempool->stack.base;
				new_mem->size = alloc_bytes;
			}
		}
		new_mem->next = new_mem->prev = NULL;
		uint8_t *const final_mem = (uint8_t *)new_mem + sizeof *new_mem;
		memset(final_mem, 0, new_mem->size - sizeof *new_mem);
		return final_mem;
	}
}

HARBOL_EXPORT void *harbol_mempool_realloc(struct HarbolMemPool *const restrict mempool, void *const ptr, const size_t size)
{
	if( size > mempool->stack.size )
		return NULL;
	// NULL ptr should make this work like regular alloc.
	else if( ptr==NULL )
		return harbol_mempool_alloc(mempool, size);
	else if( (uintptr_t)ptr - sizeof(struct HarbolMemNode) < (uintptr_t)mempool->stack.mem )
		return NULL;
	else {
		struct HarbolMemNode *node = (struct HarbolMemNode *)((uint8_t *)ptr - sizeof *node);
		uint8_t *resized_block = harbol_mempool_alloc(mempool, size);
		if( resized_block==NULL )
			return NULL;
		else {
			struct HarbolMemNode *resized = (struct HarbolMemNode *)(resized_block - sizeof *resized);
			memmove(resized_block, ptr, ((node->size > resized->size)? (resized->size) : (node->size)) - sizeof *node);
			harbol_mempool_free(mempool, ptr);
			return resized_block;
		}
	}
}

HARBOL_EXPORT bool harbol_mempool_free(struct HarbolMemPool *const restrict mempool, void *const ptr)
{
	if( ptr==NULL || (uintptr_t)ptr - sizeof(struct HarbolMemNode) < (uintptr_t)mempool->stack.mem )
		return false;
	else {
		// behind the actual pointer data is the allocation info.
		struct HarbolMemNode *mem_node = (struct HarbolMemNode *)((uint8_t *)ptr - sizeof *mem_node);
		const uindex_t b = (mem_node->size >> HARBOL_BUCKET_BITS) - 1;
		
		// make sure the pointer data is valid.
		if( (uintptr_t)mem_node < (uintptr_t)mempool->stack.base || ((uintptr_t)mem_node - (uintptr_t)mempool->stack.mem) > mempool->stack.size || mem_node->size==0 || mem_node->size > mempool->stack.size )
			return false;
		// if the mem_node is right at the stack base ptr, then add it to the stack.
		else if( (uintptr_t)mem_node==(uintptr_t)mempool->stack.base ) {
			mempool->stack.base += mem_node->size;
		}
		// try to place it into bucket.
		else if( b < HARBOL_BUCKET_SIZE ) {
			if( mempool->buckets[b]==NULL ) {
				mempool->buckets[b] = mem_node;
			} else {
				for( struct HarbolMemNode *n = mempool->buckets[b]; n != NULL; n = n->next )
					if( n==mem_node )
						return false;
				mempool->buckets[b]->prev = mem_node;
				mem_node->next = mempool->buckets[b];
				mempool->buckets[b] = mem_node;
			}
		}
		// otherwise, we add it to the free list.
		// We also check if the freelist already has the pointer so we can prevent double frees.
		else if( mempool->freelist.len==0 || ((uintptr_t)mempool->freelist.head >= (uintptr_t)mempool->stack.mem && (uintptr_t)mempool->freelist.head - (uintptr_t)mempool->stack.mem < mempool->stack.size) ) {
			for( struct HarbolMemNode *n = mempool->freelist.head; n != NULL; n = n->next )
				if( n==mem_node )
					return false;
			
			// this code insertion sorts where largest size is first.
			if( mempool->freelist.head==NULL ) {
				mempool->freelist.head = mempool->freelist.tail = mem_node;
				mempool->freelist.len++;
			} else if( mempool->freelist.head->size > mem_node->size ) {
				mem_node->next = mempool->freelist.head;
				mempool->freelist.head->prev = mem_node;
				mempool->freelist.head = mem_node;
				mempool->freelist.len++;
			} else {
				mem_node->prev = mempool->freelist.tail;
				mempool->freelist.tail->next = mem_node;
				mempool->freelist.tail = mem_node;
				mempool->freelist.len++;
			}
			
			if( mempool->freelist.auto_defrag && mempool->freelist.max_nodes != 0 && mempool->freelist.len > mempool->freelist.max_nodes )
				harbol_mempool_defrag(mempool);
		}
		return true;
	}
}

HARBOL_EXPORT bool harbol_mempool_cleanup(struct HarbolMemPool *const restrict mempool, void **ptrref)
{
	if( *ptrref==NULL )
		return false;
	else {
		const bool free_result = harbol_mempool_free(mempool, *ptrref);
		*ptrref = NULL;
		return free_result;
	}
}

HARBOL_EXPORT size_t harbol_mempool_mem_remaining(const struct HarbolMemPool *mempool)
{
	size_t total_remaining = (uintptr_t)mempool->stack.base - (uintptr_t)mempool->stack.mem;
	for( struct HarbolMemNode *n = mempool->freelist.head; n != NULL; n = n->next )
		total_remaining += n->size;
	for( uindex_t i=0; i<HARBOL_BUCKET_SIZE; i++ )
		for( struct HarbolMemNode *n=mempool->buckets[i]; n != NULL; n = n->next )
			total_remaining += n->size;
	return total_remaining;
}


HARBOL_EXPORT bool harbol_mempool_defrag(struct HarbolMemPool *const mempool)
{
	// if the memory pool has been entirely released, fully defrag it.
	if( mempool->stack.size == harbol_mempool_mem_remaining(mempool) ) {
		mempool->freelist.head = mempool->freelist.tail = NULL;
		mempool->freelist.len = 0;
		for( uindex_t i=0; i<HARBOL_BUCKET_SIZE; i++ )
			mempool->buckets[i] = NULL;
		mempool->stack.base = mempool->stack.mem + mempool->stack.size;
		return true;
	} else {
		for( uindex_t i=0; i<HARBOL_BUCKET_SIZE; i++ ) {
			while( mempool->buckets[i] != NULL ) {
				if( (uintptr_t)mempool->buckets[i] == (uintptr_t)mempool->stack.base ) {
					mempool->stack.base += mempool->buckets[i]->size;
					mempool->buckets[i]->size = 0;
					mempool->buckets[i] = mempool->buckets[i]->next;
					if( mempool->buckets[i] != NULL )
						mempool->buckets[i]->prev = NULL;
					i = 0;
				}
				else break;
			}
		}
		
		const size_t predefrag_len = mempool->freelist.len;
		struct HarbolMemNode **node = &mempool->freelist.head;
		while( *node != NULL ) {
			if( (uintptr_t)*node == (uintptr_t)mempool->stack.base ) {
				// if node is right at the stack, merge it back into the stack.
				mempool->stack.base += (*node)->size;
				(*node)->size = 0;
				(*node)->prev != NULL ? ((*node)->prev->next = (*node)->next) : (mempool->freelist.head = (*node)->next);
				(*node)->next != NULL ? ((*node)->next->prev = (*node)->prev) : (mempool->freelist.tail = (*node)->prev);
				if( mempool->freelist.head != NULL )
					mempool->freelist.head->prev = NULL;
				else mempool->freelist.tail = NULL;
				
				if( mempool->freelist.tail != NULL )
					mempool->freelist.tail->next = NULL;
				mempool->freelist.len--;
				node = &mempool->freelist.head;
			} else if( (uintptr_t)*node + (*node)->size == (uintptr_t)(*node)->next ) {
				// next node is at a higher address.
				(*node)->size += (*node)->next->size;
				(*node)->next->size = 0;
				
				// <-[P Curr N]-> <-[P next N]-> <-[P NextNext N]->
				// 
				//           |--------------------|
				// <-[P Curr N]-> <-[P next N]-> [P NextNext N]->
				if( (*node)->next->next != NULL )
					(*node)->next->next->prev = *node;
				
				// <-[P Curr N]-> <-[P NextNext N]->
				(*node)->next = (*node)->next->next;
				
				mempool->freelist.len--;
				node = &mempool->freelist.head;
			} else if( (uintptr_t)*node + (*node)->size == (uintptr_t)(*node)->prev && (*node)->prev->prev != NULL ) {
				// prev node is at a higher address.
				(*node)->size += (*node)->prev->size;
				(*node)->prev->size = 0;
				
				// <-[P PrevPrev N]-> <-[P prev N]-> <-[P Curr N]->
				// 
				//               |--------------------|
				// <-[P PrevPrev N] <-[P prev N]-> <-[P Curr N]->
				(*node)->prev->prev->next = *node;
				
				// <-[P PrevPrev N]-> <-[P Curr N]->
				(*node)->prev = (*node)->prev->prev;
				
				mempool->freelist.len--;
				node = &mempool->freelist.head;
			} else if( (*node)->prev != NULL && (*node)->next != NULL && (uintptr_t)*node - (*node)->next->size == (uintptr_t)(*node)->next ) {
				// next node is at a lower address.
				// <-[P prev N]-> <-[P Curr N]-> <-[P next N]->
				(*node)->next->size += (*node)->size;
				(*node)->size = 0;
				
				//           |--------------------|
				// <-[P prev N]-> <-[P Curr N]-> [P next N]->
				(*node)->next->prev = (*node)->prev;
				
				// <-[P prev N]-> <-[P next N]->
				(*node)->prev->next = (*node)->next;
				
				// <-[P prev N]-> <-[P Curr N]->
				*node = (*node)->next;
				
				mempool->freelist.len--;
				node = &mempool->freelist.head;
			} else if( (*node)->prev != NULL && (*node)->next != NULL && (uintptr_t)*node - (*node)->prev->size == (uintptr_t)(*node)->prev ) {
				// prev node is at a lower address.
				// <-[P prev N]-> <-[P Curr N]-> <-[P next N]->
				(*node)->prev->size += (*node)->size;
				(*node)->size = 0;
				
				//           |--------------------|
				// <-[P prev N]-> <-[P Curr N]-> [P next N]->
				(*node)->next->prev = (*node)->prev;
				
				// <-[P prev N]-> <-[P next N]->
				(*node)->prev->next = (*node)->next;
				
				// <-[P Curr N]-> <-[P next N]->
				*node = (*node)->prev;
				
				mempool->freelist.len--;
				node = &mempool->freelist.head;
			}
			else node = &(*node)->next;
		}
		return predefrag_len > mempool->freelist.len;
	}
}

HARBOL_EXPORT NO_NULL void harbol_mempool_set_max_nodes(struct HarbolMemPool *const mempool, const size_t nodes)
{
	mempool->freelist.max_nodes = nodes;
}

HARBOL_EXPORT void harbol_mempool_toggle_auto_defrag(struct HarbolMemPool *const mempool)
{
	mempool->freelist.auto_defrag ^= true;
}
