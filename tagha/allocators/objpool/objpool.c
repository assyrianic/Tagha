#include "objpool.h"

#ifdef OS_WINDOWS
#	define HARBOL_LIB
#endif


HARBOL_EXPORT NO_NULL bool harbol_objpool_init(struct HarbolObjPool *const objpool, const size_t objsize, const size_t len) {
	if( len==0 || objsize==0 ) {
		return false;
	}
	
	const size_t aligned_objsize = harbol_align_size(objsize, sizeof(size_t));
	objpool->mem = ( uintptr_t )calloc(len, aligned_objsize);
	if( objpool->mem==NIL ) {
		return false;
	}
	
	objpool->size = objpool->free_blocks = len;
	objpool->objsize = aligned_objsize;
	for( size_t i=0; i<objpool->free_blocks; i++ ) {
		size_t *const restrict index = ( size_t* )(objpool->mem + (i * objpool->objsize));
		*index = i + 1;
	}
	objpool->next = objpool->mem;
	return true;
}

HARBOL_EXPORT struct HarbolObjPool harbol_objpool_make(const size_t objsize, const size_t len, bool *const res)
{
	struct HarbolObjPool objpool = {0};
	*res = harbol_objpool_init(&objpool, objsize, len);
	return objpool;
}

HARBOL_EXPORT NO_NULL bool harbol_objpool_init_from_buffer(struct HarbolObjPool *const restrict objpool, void *const restrict buf, const size_t objsize, const size_t len) {
	/// If the object index isn't large enough to align to a size_t, then we can't use it.
	if( objsize < sizeof(size_t) || (objsize * len) < (harbol_align_size(objsize, sizeof(size_t)) * len) ) {
		return false;
	}
	
	objpool->objsize = harbol_align_size(objsize, sizeof(size_t));
	objpool->size = objpool->free_blocks = len;
	objpool->mem = ( uintptr_t )buf;
	for( size_t i=0; i<objpool->free_blocks; i++ ) {
		size_t *const restrict index = ( size_t* )(objpool->mem + (i * objpool->objsize));
		*index = i + 1;
	}
	objpool->next = objpool->mem;
	return true;
}

HARBOL_EXPORT struct HarbolObjPool harbol_objpool_from_buffer(void *const restrict buf, const size_t objsize, const size_t len, bool *const restrict res)
{
	struct HarbolObjPool objpool = {0};
	*res = harbol_objpool_init_from_buffer(&objpool, buf, objsize, len);
	return objpool;
}

HARBOL_EXPORT void harbol_objpool_clear(struct HarbolObjPool *const objpool)
{
	if( objpool->mem==NIL )
		return;
	
	free(( void* )objpool->mem);
	*objpool = (struct HarbolObjPool){0};
}

HARBOL_EXPORT void *harbol_objpool_alloc(struct HarbolObjPool *const objpool)
{
	if( objpool->free_blocks > 0 ) {
		/// for first allocation, head points to the very first index.
		/// next = &pool[0];
		/// ret = next == ret = &pool[0];
		size_t *const index = ( size_t* )objpool->next;
		objpool->free_blocks--;
		
		/// after allocating, we set head to the address of the index that *next holds.
		/// next = &pool[*next * pool.objsize];
		objpool->next = ( objpool->free_blocks != 0 ) ? objpool->mem + (*index * objpool->objsize) : 0;
		return memset(index, 0, objpool->objsize);
	}
	return NULL;
}

HARBOL_EXPORT void harbol_objpool_free(struct HarbolObjPool *const restrict objpool, void *const restrict ptr)
{
	const uintptr_t p = ( uintptr_t )ptr;
	if( ptr==NULL || p < objpool->mem || p > objpool->mem + (objpool->size * objpool->objsize) ) {
		return;
	}
	/// when we free our pointer, we recycle the pointer space to store the previous index
	/// and then we push it as our new head.
	
	/// *p = index of next in relation to the buffer;
	/// next = p;
	{
		size_t *const restrict index = ptr;
		*index = ( objpool->next != 0 ) ? (objpool->next - objpool->mem) / objpool->objsize : objpool->size;
	}
	objpool->next = p;
	++objpool->free_blocks;
}

HARBOL_EXPORT void harbol_objpool_cleanup(struct HarbolObjPool *const restrict objpool, void **const restrict ptrref)
{
	if( *ptrref==NULL )
		return;
	
	harbol_objpool_free(objpool, *ptrref); *ptrref = NULL;
}