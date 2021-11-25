#ifndef HARBOL_REGION_INCLUDED
#	define HARBOL_REGION_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "../../harbol_common_defines.h"
#include "../../harbol_common_includes.h"


struct HarbolRegion {
	uintptr_t mem, offs;
	size_t    size;
};

HARBOL_EXPORT struct HarbolRegion harbol_region_make(size_t bytes);
HARBOL_EXPORT NO_NULL struct HarbolRegion harbol_region_make_from_buffer(void *buf, size_t bytes);
HARBOL_EXPORT NO_NULL void harbol_region_clear(struct HarbolRegion *cache);

HARBOL_EXPORT NO_NULL void *harbol_region_alloc(struct HarbolRegion *cache, size_t bytes);
HARBOL_EXPORT NO_NULL size_t harbol_region_remaining(const struct HarbolRegion *cache);
/********************************************************************/


#ifdef __cplusplus
}
#endif

#endif /** HARBOL_REGION_INCLUDED */