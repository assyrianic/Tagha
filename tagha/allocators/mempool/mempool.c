#include "mempool.h"

#ifdef OS_WINDOWS
#	define HARBOL_LIB
#endif


HARBOL_EXPORT struct HarbolMemNode *harbol_memnode_split(struct HarbolMemNode *const node, const size_t bytes)
{
	uintptr_t n = ( uintptr_t )node;
	struct HarbolMemNode *r = ( struct HarbolMemNode* )(n + (node->size - bytes));
	node->size -= bytes;
	r->size = bytes;
	return r;
}


HARBOL_EXPORT void harbol_memnode_replace(struct HarbolMemNode *const old, struct HarbolMemNode *const replace)
{
	/// replace this node.
	replace->prev = old->prev;
	replace->next = old->next;
	if( replace->prev != NULL )
		replace->prev->next = replace;
	if( replace->next != NULL )
		replace->next->prev = replace;
}


/// makes a current node into the next node.
HARBOL_EXPORT void harbol_freelist_insert_before(struct HarbolFreeList *const list, struct HarbolMemNode *const curr, struct HarbolMemNode *const insert)
{
	insert->next = curr;
	if (curr->prev==NULL) list->head = insert;
	else {
		insert->prev = curr->prev;
		curr->prev->next = insert;
	}
    curr->prev = insert;
}


HARBOL_EXPORT void harbol_freelist_insert(struct HarbolMemPool *const mempool, struct HarbolFreeList *const list, struct HarbolMemNode *const node, const bool is_bucket)
{
	if( list->head==NULL ) {
		list->head = node;
		list->len++;
	} else {
		for( struct HarbolMemNode *iter = list->head; iter != NULL; iter = iter->next ) {
			if( ( uintptr_t )iter == mempool->stack.offs ) {
				mempool->stack.offs += iter->size;
				harbol_freelist_remove(list, iter);
				iter = list->head;
			}
			const uintptr_t inode = ( uintptr_t )node;
			const uintptr_t iiter = ( uintptr_t )iter;
			const uintptr_t iter_end = iiter + iter->size;
			const uintptr_t node_end = inode + node->size;
			
			if( iter==node )
				return;
			else if( iter < node ) {
				if( iter_end > inode ) // node was coalesced prior.
					return;
				else if( iter_end==inode ) { // if we can coalesce, do so.
					iter->size += node->size;
					if( is_bucket )
						harbol_freelist_insert(mempool, &mempool->large, harbol_freelist_remove(list, iter), false);
					return;
				}
			} else if( iter > node ) {
				// Address sort, lowest to highest aka ascending order.
				if( iter==list->head ) {
					if( iter_end==inode ) {
						iter->size += node->size;
						if( is_bucket )
							harbol_freelist_insert(mempool, &mempool->large, harbol_freelist_remove(list, iter), false);
					} else if( node_end==iiter ) {
						node->size += list->head->size;
						node->next = list->head->next;
						node->prev = NULL;
						list->head = node;
						if( is_bucket )
							harbol_freelist_insert(mempool, &mempool->large, harbol_freelist_remove(list, list->head), false);
					} else {
						node->next = iter;
						node->prev = NULL;
						iter->prev = node;
						list->head = node;
						list->len++;
					}
					return;
				} else if( iter_end==inode ) {
					iter->size += node->size;
					if( is_bucket )
						harbol_freelist_insert(mempool, &mempool->large, harbol_freelist_remove(list, iter), false);
					return;
				} else {
					harbol_freelist_insert_before(list, iter, node);
					list->len++;
					return;
				}
			}
		}
	}
}

HARBOL_EXPORT struct HarbolMemNode *harbol_freelist_remove(struct HarbolFreeList *const list, struct HarbolMemNode *const node)
{
	if( node->prev != NULL )
		node->prev->next = node->next;
	else {
		list->head = node->next;
		if( list->head != NULL )
			list->head->prev = NULL;
		else list->tail = NULL;
	}
	
	if( node->next != NULL )
		node->next->prev = node->prev;
	else {
		list->tail = node->prev;
		if( list->tail != NULL )
			list->tail->next = NULL;
		else list->head = NULL;
	}
	list->len--;
	return node;
}

HARBOL_EXPORT struct HarbolMemNode *harbol_freelist_find(struct HarbolFreeList *const list, const size_t bytes)
{
	for( struct HarbolMemNode *node = list->head; node != NULL; node = node->next ) {
		if( node->size < bytes )
			continue;
		// close in size - reduce fragmentation by not splitting.
		else if( node->size <= bytes + MEM_SPLIT_THRESHOLD )
			return harbol_freelist_remove(list, node);
		else return harbol_memnode_split(node, bytes);
	}
	return NULL;
}


HARBOL_EXPORT struct HarbolMemPool harbol_mempool_create(const size_t size)
{
	struct HarbolMemPool mempool = { 0 };
	if( size==0 )
		return mempool;
	else {
		mempool.stack = harbol_cache_create(size);
		return mempool;
	}
}

HARBOL_EXPORT struct HarbolMemPool harbol_mempool_from_buffer(void *const restrict buf, const size_t size)
{
	struct HarbolMemPool mempool = { 0 };
	if( size==0 || size<=sizeof(struct HarbolMemNode) )
		return mempool;
	else {
		mempool.stack = harbol_cache_from_buffer(buf, size);
		return mempool;
	}
}

HARBOL_EXPORT bool harbol_mempool_clear(struct HarbolMemPool *const mempool)
{
	bool result = harbol_cache_clear(&mempool->stack);
	*mempool = (struct HarbolMemPool){ 0 };
	return result;
}

static NO_NULL struct HarbolMemNode *_get_freenode(struct HarbolMemPool *const mempool, const size_t bytes)
{
	// check if we have a good sized node from the buckets.
	const uindex_t slot = (bytes >> HARBOL_BUCKET_BITS) - 1;
	struct HarbolFreeList *const l = (slot < HARBOL_BUCKET_SIZE) ? &mempool->buckets[slot] : &mempool->large;
	return harbol_freelist_find(l, bytes);
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
		// |------------|
		// |   alloc'd  |
		// |   memory   |
		// |   space    | highest addr of block
		// --------------
		const size_t alloc_bytes = harbol_align_size(size + sizeof(struct HarbolMemNode), sizeof(intptr_t));
		struct HarbolMemNode *new_mem = _get_freenode(mempool, alloc_bytes);
		
		if( new_mem==NULL ) {
			new_mem = harbol_cache_alloc(&mempool->stack, alloc_bytes);
			if( new_mem==NULL )
				return NULL;
			else new_mem->size = alloc_bytes;
		}
		
		new_mem->next = new_mem->prev = NULL;
		uint8_t *const final_mem = ( uint8_t* )new_mem + sizeof *new_mem;
		return memset(final_mem, 0, new_mem->size - sizeof *new_mem);
	}
}

HARBOL_EXPORT void *harbol_mempool_realloc(struct HarbolMemPool *const restrict mempool, void *const ptr, const size_t size)
{
	if( size > mempool->stack.size )
		return NULL;
	// NULL ptr should make this work like regular alloc.
	else if( ptr==NULL )
		return harbol_mempool_alloc(mempool, size);
	else if( ( uintptr_t )ptr - sizeof(struct HarbolMemNode) < mempool->stack.mem )
		return NULL;
	else {
		struct HarbolMemNode *node = ( struct HarbolMemNode* )(( uint8_t* )ptr - sizeof *node);
		uint8_t *resized_block = harbol_mempool_alloc(mempool, size);
		if( resized_block==NULL )
			return NULL;
		else {
			struct HarbolMemNode *resized = ( struct HarbolMemNode* )(resized_block - sizeof *resized);
			memmove(resized_block, ptr, ((node->size > resized->size)? (resized->size) : (node->size)) - sizeof *node);
			harbol_mempool_free(mempool, ptr);
			return resized_block;
		}
	}
}

HARBOL_EXPORT bool harbol_mempool_free(struct HarbolMemPool *const restrict mempool, void *const ptr)
{
	if( ptr==NULL || ( uintptr_t )ptr - sizeof(struct HarbolMemNode) < mempool->stack.mem )
		return false;
	else {
		// behind the actual pointer data is the allocation info.
		struct HarbolMemNode *mem_node = (struct HarbolMemNode *)((uint8_t *)ptr - sizeof *mem_node);
		const uindex_t slot = (mem_node->size >> HARBOL_BUCKET_BITS) - 1;
		
		// make sure the pointer data is valid.
		if( (uintptr_t)mem_node < mempool->stack.offs
				|| ((uintptr_t)mem_node - mempool->stack.mem) > mempool->stack.size
				|| mem_node->size==0
				|| mem_node->size > mempool->stack.size )
			return false;
		
		// if the mem_node is right at the stack base ptr, then add it to the stack.
		else if( (uintptr_t)mem_node==mempool->stack.offs )
			mempool->stack.offs += mem_node->size;
		else {
			// try to place it into bucket or large freelist.
			struct HarbolFreeList *const list = ( slot < HARBOL_BUCKET_SIZE ) ? &mempool->buckets[slot] : &mempool->large;
			harbol_freelist_insert(mempool, list, mem_node, ( slot < HARBOL_BUCKET_SIZE ));
		}
		return true;
	}
}

HARBOL_EXPORT bool harbol_mempool_cleanup(struct HarbolMemPool *const restrict mempool, void **const restrict ptrref)
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
	size_t total_remaining = mempool->stack.offs - mempool->stack.mem;
	
	for( struct HarbolMemNode *n = mempool->large.head; n != NULL; n = n->next )
		total_remaining += n->size;
	
	for( uindex_t i=0; i<HARBOL_BUCKET_SIZE; i++ )
		for( struct HarbolMemNode *n=mempool->buckets[i].head; n != NULL; n = n->next )
			total_remaining += n->size;
	
	return total_remaining;
}
