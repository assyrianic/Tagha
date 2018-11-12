#include <stdlib.h>
#include <stdio.h>
#ifdef OS_WINDOWS
	#define HARBOL_LIB
#endif
#include "harbol.h"


HARBOL_EXPORT struct HarbolVariant *HarbolVariant_New(const union HarbolValue val, const int32_t tagtype)
{
	struct HarbolVariant *variant = calloc(1, sizeof *variant);
	HarbolVariant_Init(variant, val, tagtype);
	return variant;
}

HARBOL_EXPORT void HarbolVariant_Free(struct HarbolVariant **varref, fnDestructor *const dtor)
{
	if( !varref || !*varref )
		return;
	HarbolVariant_Del(*varref, dtor);
	free(*varref), *varref=NULL;
}

HARBOL_EXPORT void HarbolVariant_Init(struct HarbolVariant *const variant, const union HarbolValue val, const int32_t tagtype)
{
	if( !variant )
		return;
	
	memset(variant, 0, sizeof *variant);
	variant->Val = val;
	variant->TypeTag = tagtype;
}

HARBOL_EXPORT void HarbolVariant_Del(struct HarbolVariant *const variant, fnDestructor *const dtor)
{
	if( !variant )
		return;
	else if( dtor )
		(*dtor)(&variant->Val.Ptr);
	memset(variant, 0, sizeof *variant);
}

HARBOL_EXPORT union HarbolValue HarbolVariant_GetVal(const struct HarbolVariant *const variant)
{
	return variant ? variant->Val : (union HarbolValue){0};
}

HARBOL_EXPORT void HarbolVariant_SetVal(struct HarbolVariant *const variant, const union HarbolValue val)
{
	if( !variant )
		return;
	
	variant->Val = val;
}

HARBOL_EXPORT int32_t HarbolVariant_GetType(const struct HarbolVariant *const variant)
{
	return variant ? variant->TypeTag : 0;
}

HARBOL_EXPORT void HarbolVariant_SetType(struct HarbolVariant *const variant, const int32_t tagtype)
{
	if( !variant )
		return;
	
	variant->TypeTag = tagtype;
}
