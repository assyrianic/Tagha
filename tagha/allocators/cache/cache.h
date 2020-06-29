#ifndef HARBOL_CACHE_INCLUDED
#	define HARBOL_CACHE_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "../../harbol_common_defines.h"
#include "../../harbol_common_includes.h"


struct HarbolCache {
	uintptr_t mem, offs;
	size_t size;
};

HARBOL_EXPORT struct HarbolCache harbol_cache_create(size_t bytes);
HARBOL_EXPORT NO_NULL struct HarbolCache harbol_cache_from_buffer(void *buf, size_t bytes);
HARBOL_EXPORT NO_NULL bool harbol_cache_clear(struct HarbolCache *cache);

HARBOL_EXPORT NO_NULL void *harbol_cache_alloc(struct HarbolCache *cache, size_t bytes);
HARBOL_EXPORT NO_NULL size_t harbol_cache_remaining(const struct HarbolCache *cache);
/********************************************************************/


#ifdef __cplusplus
}
#endif

#endif /* HARBOL_CACHE_INCLUDED */
