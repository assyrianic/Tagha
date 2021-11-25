#include "bistack.h"

#ifdef OS_WINDOWS
#	define HARBOL_LIB
#endif


HARBOL_EXPORT bool harbol_bistack_init(struct HarbolBiStack *const bistack, const size_t len) {
	if( len==0 )
		return false;
	
	bistack->mem = ( uintptr_t )calloc(len, sizeof(uint8_t));
	if( bistack->mem==NIL )
		return false;
	
	bistack->size = len;
	bistack->front = bistack->mem;
	bistack->back = bistack->mem + len;
	return true;
}

HARBOL_EXPORT struct HarbolBiStack harbol_bistack_make(const size_t len, bool *const res)
{
	struct HarbolBiStack bistack = {0};
	*res = harbol_bistack_init(&bistack, len);
	return bistack;
}

HARBOL_EXPORT struct HarbolBiStack harbol_bistack_make_from_buffer(void *const restrict buf, const size_t len)
{
	struct HarbolBiStack bistack = {0};
	bistack.size = len;
	bistack.mem = bistack.front = ( uintptr_t )buf;
	bistack.back = bistack.mem + len;
	return bistack;
}

HARBOL_EXPORT void harbol_bistack_clear(struct HarbolBiStack *const bistack)
{
	if( bistack->mem==NIL )
		return;
	
	free(( void* )bistack->mem);
	bistack->mem = bistack->front = bistack->back = NIL;
	bistack->size = 0;
}

HARBOL_EXPORT void *harbol_bistack_alloc_front(struct HarbolBiStack *const bistack, const size_t size)
{
	if( bistack->mem==NIL )
		return NULL;
	
	const size_t aligned_size = harbol_align_size(size, sizeof(uintptr_t));
	/// front end arena is too high!
	if( bistack->front + aligned_size >= bistack->back )
		return NULL;
	
	const uintptr_t p = bistack->front;
	bistack->front += aligned_size;
	return memset(( void* )p, 0, aligned_size);
}

HARBOL_EXPORT void *harbol_bistack_alloc_back(struct HarbolBiStack *const restrict bistack, const size_t size)
{
	if( bistack->mem==NIL )
		return NULL;
	
	const size_t aligned_size = harbol_align_size(size, sizeof(uintptr_t));
	/// back end arena is too low
	if( bistack->back - aligned_size <= bistack->front )
		return NULL;
	
	bistack->back -= aligned_size;
	return memset(( void* )bistack->back, 0, aligned_size);
}

HARBOL_EXPORT void harbol_bistack_reset_front(struct HarbolBiStack *const bistack)
{
	if( bistack->mem==NIL )
		return;
	
	bistack->front = bistack->mem;
}

HARBOL_EXPORT void harbol_bistack_reset_back(struct HarbolBiStack *const bistack)
{
	if( bistack->mem==NIL )
		return;
	
	bistack->back = bistack->mem + bistack->size;
}

HARBOL_EXPORT void harbol_bistack_reset_all(struct HarbolBiStack *const bistack)
{
	if( bistack->mem==NIL )
		return;
	
	bistack->front = bistack->mem;
	bistack->back = bistack->mem + bistack->size;
}

HARBOL_EXPORT intptr_t harbol_bistack_get_margins(const struct HarbolBiStack bistack)
{
	return ( intptr_t )bistack.back - ( intptr_t )bistack.front;
}


HARBOL_EXPORT bool harbol_bistack_resize(struct HarbolBiStack *const restrict bistack, const size_t new_size) {
	uint8_t *const new_buf = harbol_recalloc(( void* )bistack->mem, new_size, sizeof *new_buf, bistack->size);
	if( new_buf==NULL )
		return false;
	
	bistack->mem  = bistack->front = ( uintptr_t )new_buf;
	bistack->size = new_size;
	bistack->back = bistack->mem + new_size;
	return true;
}