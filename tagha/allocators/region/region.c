#include "region.h"

#ifdef OS_WINDOWS
#	define HARBOL_LIB
#endif


HARBOL_EXPORT struct HarbolRegion harbol_region_make(const size_t size)
{
	struct HarbolRegion region = {0};
	if( size==0 )
		return region;
	
	region.mem = ( uintptr_t )calloc(size, sizeof(uint8_t));
	if( region.mem==NIL ) {
		return region;
	}
	region.size = size;
	region.offs = region.mem + size;
	return region;
}

HARBOL_EXPORT struct HarbolRegion harbol_region_make_from_buffer(void *const restrict buf, const size_t size)
{
	struct HarbolRegion region = {0};
	if( size==0 )
		return region;
	
	region.size = size;
	region.mem = ( uintptr_t )buf;
	region.offs = region.mem + size;
	return region;
}

HARBOL_EXPORT void harbol_region_clear(struct HarbolRegion *const region)
{
	if( region->mem==NIL )
		return;
	
	free(( void* )region->mem);
	*region = (struct HarbolRegion){0};
}

HARBOL_EXPORT void *harbol_region_alloc(struct HarbolRegion *const region, const size_t size)
{
	if( region->mem==0 || size==0 || size > region->size )
		return NULL;
	
	const size_t alloc_size = harbol_align_size(size, sizeof(uintptr_t));
	if( region->offs - alloc_size < region->mem )
		return NULL;
	
	region->offs -= alloc_size;
	return memset(( void* )region->offs, 0, alloc_size);
}

HARBOL_EXPORT size_t harbol_region_remaining(const struct HarbolRegion *const region)
{
	return region->offs - region->mem;
}