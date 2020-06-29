#include "cache.h"

#ifdef OS_WINDOWS
#	define HARBOL_LIB
#endif


HARBOL_EXPORT struct HarbolCache harbol_cache_create(const size_t size)
{
	struct HarbolCache cache = {0};
	if( size==0 )
		return cache;
	else {
		uint8_t *const restrict buf = calloc(size, sizeof *buf);
		if( buf==NULL ) {
			return cache;
		} else {
			cache.size = size;
			cache.mem = ( uintptr_t )buf;
			cache.offs = cache.mem + size;
			return cache;
		}
	}
}

HARBOL_EXPORT struct HarbolCache harbol_cache_from_buffer(void *const restrict buf, const size_t size)
{
	struct HarbolCache cache = {0};
	if( size==0 )
		return cache;
	else {
		cache.size = size;
		cache.mem = ( uintptr_t )buf;
		cache.offs = cache.mem + size;
		return cache;
	}
}

HARBOL_EXPORT bool harbol_cache_clear(struct HarbolCache *const restrict cache)
{
	if( cache->mem==0 )
		return false;
	else {
		void *restrict ptr = ( void* )cache->mem;
		free(ptr);
		*cache = (struct HarbolCache){0};
		return true;
	}
}

HARBOL_EXPORT void *harbol_cache_alloc(struct HarbolCache *const restrict cache, const size_t size)
{
	if( cache->mem==0 || size==0 || size > cache->size )
		return NULL;
	else {
		const size_t alloc_size = harbol_align_size(size, sizeof(uintptr_t));
		if( cache->offs - alloc_size < cache->mem )
			return NULL;
		else {
			cache->offs -= alloc_size;
			void *const restrict mem = ( void* )cache->offs;
			return memset(mem, 0, alloc_size);
		}
	}
}

HARBOL_EXPORT size_t harbol_cache_remaining(const struct HarbolCache *const cache)
{
	return cache->offs - cache->mem;
}
