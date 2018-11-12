#include <stdlib.h>
#include <stdio.h>
#ifdef OS_WINDOWS
	#define HARBOL_LIB
#endif
#include "harbol.h"

HARBOL_EXPORT struct HarbolTree *HarbolTree_New(const union HarbolValue val)
{
	struct HarbolTree *tn = calloc(1, sizeof *tn);
	HarbolTree_InitVal(tn, val);
	return tn;
}

HARBOL_EXPORT void HarbolTree_Init(struct HarbolTree *const tn)
{
	if( !tn )
		return;
	memset(tn, 0, sizeof *tn);
}

HARBOL_EXPORT void HarbolTree_InitVal(struct HarbolTree *const tn, const union HarbolValue val)
{
	if( !tn )
		return;
	memset(tn, 0, sizeof *tn);
	tn->Data = val;
}

HARBOL_EXPORT void HarbolTree_Del(struct HarbolTree *const tn, fnDestructor *const dtor)
{
	if( !tn )
		return;
	if( dtor )
		(*dtor)(&tn->Data.Ptr);
	
	struct HarbolVector *vec = &tn->Children;
	for( size_t i=0 ; i<vec->Count ; i++ ) {
		struct HarbolTree *node = vec->Table[i].Ptr;
		HarbolTree_Free(&node, dtor);
	}
	HarbolVector_Del(vec, NULL);
	memset(tn, 0, sizeof *tn);
}

HARBOL_EXPORT void HarbolTree_Free(struct HarbolTree **tnref, fnDestructor *const dtor)
{
	if( !*tnref )
		return;
	HarbolTree_Del(*tnref, dtor);
	free(*tnref); *tnref=NULL;
}

HARBOL_EXPORT bool HarbolTree_InsertChildByNode(struct HarbolTree *const restrict tn, struct HarbolTree *const restrict node)
{
	if( !tn || !node )
		return false;
	
	HarbolVector_Insert(&tn->Children, (union HarbolValue){.Ptr=node});
	return true;
}

HARBOL_EXPORT bool HarbolTree_InsertChildByValue(struct HarbolTree *const restrict tn, const union HarbolValue val)
{
	if( !tn )
		return false;
	
	struct HarbolTree *restrict node = HarbolTree_New(val);
	if( !node )
		return false;
	
	HarbolVector_Insert(&tn->Children, (union HarbolValue){.Ptr=node});
	return true;
}

HARBOL_EXPORT bool HarbolTree_RemoveChildByRef(struct HarbolTree *const restrict tn, struct HarbolTree **noderef, fnDestructor *const dtor)
{
	if( !tn || !tn->Children.Table || !*noderef )
		return false;
	
	struct HarbolTree *restrict node = *noderef;
	struct HarbolVector *vec = &tn->Children;
	for( size_t i=0 ; i<vec->Count ; i++ ) {
		if( vec->Table[i].Ptr==node ) {
			struct HarbolTree *child = vec->Table[i].Ptr;
			HarbolTree_Free(&child, dtor);
			HarbolVector_Delete(vec, i, NULL);
			return true;
		}
	}
	return false;
}

HARBOL_EXPORT bool HarbolTree_RemoveChildByIndex(struct HarbolTree *const restrict tn, const size_t index, fnDestructor *const dtor)
{
	if( !tn || index >= tn->Children.Count )
		return false;
	
	struct HarbolVector *restrict vec = &tn->Children;
	struct HarbolTree *child = vec->Table[index].Ptr;
	HarbolTree_Free(&child, dtor);
	HarbolVector_Delete(vec, index, NULL);
	HarbolVector_Truncate(vec);
	return true;
}

HARBOL_EXPORT bool HarbolTree_RemoveChildByValue(struct HarbolTree *const restrict tn, const union HarbolValue val, fnDestructor *const dtor)
{
	if( !tn || !tn->Children.Table )
		return false;
	
	struct HarbolVector *vec = &tn->Children;
	for( size_t i=0 ; i<vec->Count ; i++ ) {
		struct HarbolTree *node = vec->Table[i].Ptr;
		if( node->Data.Int64==val.Int64 ) {
			HarbolTree_Free(&node, dtor);
			HarbolVector_Delete(vec, i, NULL);
			return true;
		}
	}
	HarbolVector_Truncate(vec);
	return false;
}

HARBOL_EXPORT struct HarbolTree *HarbolTree_GetChildByIndex(const struct HarbolTree *const tn, const size_t index)
{
	if( !tn || !tn->Children.Table || index >= tn->Children.Count )
		return NULL;
	return tn->Children.Table[index].Ptr;
}

HARBOL_EXPORT struct HarbolTree *HarbolTree_GetChildByValue(const struct HarbolTree *const restrict tn, const union HarbolValue val)
{
	if( !tn || !tn->Children.Table )
		return NULL;
	
	for( size_t i=0 ; i<tn->Children.Count ; i++ ) {
		struct HarbolTree *restrict node = tn->Children.Table[i].Ptr;
		if( node->Data.Int64==val.Int64 )
			return node;
	}
	return NULL;
}

HARBOL_EXPORT union HarbolValue HarbolTree_GetData(const struct HarbolTree *const tn)
{
	return tn ? tn->Data : (union HarbolValue){0};
}

HARBOL_EXPORT void HarbolTree_SetData(struct HarbolTree *const tn, const union HarbolValue val)
{
	if( !tn )
		return;
	
	tn->Data = val;
}

HARBOL_EXPORT struct HarbolVector HarbolTree_GetChildren(const struct HarbolTree *const tn)
{
	return tn ? tn->Children : (struct HarbolVector){NULL,0,0};
}

HARBOL_EXPORT size_t HarbolTree_GetChildLen(const struct HarbolTree *const tn)
{
	return tn ? tn->Children.Len : 0;
}

HARBOL_EXPORT size_t HarbolTree_GetChildCount(const struct HarbolTree *const tn)
{
	return tn ? tn->Children.Count : 0;
}
