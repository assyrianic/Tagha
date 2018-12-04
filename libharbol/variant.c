#include <stdlib.h>
#include <stdio.h>
#ifdef OS_WINDOWS
	#define HARBOL_LIB
#endif
#include "harbol.h"


HARBOL_EXPORT struct HarbolVariant *harbol_variant_new(const union HarbolValue val, const int32_t tagtype)
{
	struct HarbolVariant *variant = calloc(1, sizeof *variant);
	harbol_variant_init(variant, val, tagtype);
	return variant;
}

HARBOL_EXPORT void harbol_variant_free(struct HarbolVariant **varref, fnDestructor *const dtor)
{
	if( !varref || !*varref )
		return;
	harbol_variant_del(*varref, dtor);
	free(*varref), *varref=NULL;
}

HARBOL_EXPORT void harbol_variant_init(struct HarbolVariant *const variant, const union HarbolValue val, const int32_t tagtype)
{
	if( !variant )
		return;
	
	memset(variant, 0, sizeof *variant);
	variant->Val = val;
	variant->TypeTag = tagtype;
}

HARBOL_EXPORT void harbol_variant_del(struct HarbolVariant *const variant, fnDestructor *const dtor)
{
	if( !variant )
		return;
	else if( dtor )
		(*dtor)(&variant->Val.Ptr);
	memset(variant, 0, sizeof *variant);
}

HARBOL_EXPORT union HarbolValue harbol_linkmap_get_val(const struct HarbolVariant *const variant)
{
	return variant ? variant->Val : (union HarbolValue){0};
}

HARBOL_EXPORT void harbol_linkmap_set_val(struct HarbolVariant *const variant, const union HarbolValue val)
{
	if( !variant )
		return;
	
	variant->Val = val;
}

HARBOL_EXPORT int32_t harbol_linkmap_get_type(const struct HarbolVariant *const variant)
{
	return variant ? variant->TypeTag : 0;
}

HARBOL_EXPORT void harbol_linkmap_set_type(struct HarbolVariant *const variant, const int32_t tagtype)
{
	if( !variant )
		return;
	
	variant->TypeTag = tagtype;
}
