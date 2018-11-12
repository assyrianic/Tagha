#include <stdlib.h>
#include <stdio.h>
#ifdef OS_WINDOWS
	#define HARBOL_LIB
#endif
#include "harbol.h"

/*
struct HarbolVector {
	union HarbolValue *Table;
	size_t	Len, Count;
};
*/

HARBOL_EXPORT struct HarbolVector *HarbolVector_New(void)
{
	struct HarbolVector *v = calloc(1, sizeof *v);
	return v;
}

HARBOL_EXPORT void HarbolVector_Init(struct HarbolVector *const v)
{
	if( !v )
		return;
	
	memset(v, 0, sizeof *v);
}

HARBOL_EXPORT void HarbolVector_Del(struct HarbolVector *const v, fnDestructor *const dtor)
{
	if( !v || !v->Table )
		return;
	
	if( dtor )
		for( size_t i=0 ; i<v->Len ; i++ )
			(*dtor)(&v->Table[i].Ptr);
	
	free(v->Table); memset(v, 0, sizeof *v);
}

HARBOL_EXPORT void HarbolVector_Free(struct HarbolVector **vecref, fnDestructor *const dtor)
{
	if( !*vecref )
		return;
	
	HarbolVector_Del(*vecref, dtor);
	free(*vecref), *vecref=NULL;
}

HARBOL_EXPORT size_t HarbolVector_Len(const struct HarbolVector *const v)
{
	return v ? v->Len : 0;
}

HARBOL_EXPORT size_t HarbolVector_Count(const struct HarbolVector *const v)
{
	return v && v->Table ? v->Count : 0;
}

HARBOL_EXPORT union HarbolValue *HarbolVector_GetIter(const struct HarbolVector *const v)
{
	return v ? v->Table : NULL;
}

HARBOL_EXPORT union HarbolValue *HarbolVector_GetIterEndLen(const struct HarbolVector *const v)
{
	return v ? v->Table+v->Len : NULL;
}

HARBOL_EXPORT union HarbolValue *HarbolVector_GetIterEndCount(const struct HarbolVector *const v)
{
	return v ? v->Table+v->Count : NULL;
}

HARBOL_EXPORT void HarbolVector_Resize(struct HarbolVector *const v)
{
	if( !v )
		return;
	
	// first we get our old size.
	// then resize the actual size.
	const size_t oldsize = v->Len;
	v->Len <<= 1;
	if( !v->Len )
		v->Len = 4;
	
	// allocate new table.
	union HarbolValue *newdata = calloc(v->Len, sizeof *newdata);
	if( !newdata ) {
		v->Len >>= 1;
		if( v->Len == 1 )
			v->Len=0;
		return;
	}
	
	// copy the old table to new then free old table.
	if( v->Table ) {
		memcpy(newdata, v->Table, sizeof *newdata * oldsize);
		free(v->Table), v->Table = NULL;
	}
	v->Table = newdata;
}

HARBOL_EXPORT void HarbolVector_Truncate(struct HarbolVector *const v)
{
	if( !v )
		return;
	
	if( v->Count < v->Len>>1 ) {
		v->Len >>= 1;
		// allocate new table.
		union HarbolValue *newdata = calloc(v->Len, sizeof *newdata);
		if( !newdata )
			return;
		
		// copy the old table to new then free old table.
		if( v->Table ) {
			memcpy(newdata, v->Table, sizeof *newdata * v->Len);
			free(v->Table), v->Table = NULL;
		}
		v->Table = newdata;
	}
}

HARBOL_EXPORT bool HarbolVector_Insert(struct HarbolVector *const v, const union HarbolValue val)
{
	if( !v )
		return false;
	else if( !v->Table || v->Count >= v->Len )
		HarbolVector_Resize(v);
	
	v->Table[v->Count++] = val;
	return true;
}

HARBOL_EXPORT union HarbolValue HarbolVector_Pop(struct HarbolVector *const v)
{
	return ( !v || !v->Table || !v->Count ) ? (union HarbolValue){0} : v->Table[--v->Count];
}

HARBOL_EXPORT union HarbolValue HarbolVector_Get(const struct HarbolVector *const v, const size_t index)
{
	return (!v || !v->Table || index >= v->Count) ? (union HarbolValue){0} : v->Table[index];
}

HARBOL_EXPORT void HarbolVector_Set(struct HarbolVector *const v, const size_t index, const union HarbolValue val)
{
	if( !v || !v->Table || index >= v->Count )
		return;
	
	v->Table[index] = val;
}

HARBOL_EXPORT void HarbolVector_Delete(struct HarbolVector *const v, const size_t index, fnDestructor *const dtor)
{
	if( !v || !v->Table || index >= v->Count )
		return;
	
	if( dtor )
		(*dtor)(&v->Table[index].Ptr);
	
	size_t
		i=index+1,
		j=index
	;
	v->Count--;
	//while( i<v->Count )
	//	v->Table[j++] = v->Table[i++];
	memmove(v->Table+j, v->Table+i, (v->Count-j) * sizeof *v->Table);
	// can't keep auto-truncating, allocating memory every time can be expensive.
	// I'll let the programmers truncate whenever they need to.
	//HarbolVector_Truncate(v);
}

HARBOL_EXPORT void HarbolVector_Add(struct HarbolVector *const restrict vA, const struct HarbolVector *const restrict vB)
{
	if( !vA || !vB || !vB->Table )
		return;
	
	size_t i=0;
	while( i<vB->Count ) {
		if( !vA->Table || vA->Count >= vA->Len )
			HarbolVector_Resize(vA);
		vA->Table[vA->Count++] = vB->Table[i++];
	}
}

HARBOL_EXPORT void HarbolVector_Copy(struct HarbolVector *const restrict vA, const struct HarbolVector *const restrict vB)
{
	if( !vA || !vB || !vB->Table )
		return;
	
	HarbolVector_Del(vA, NULL);
	size_t i=0;
	while( i<vB->Count ) {
		if( !vA->Table || vA->Count >= vA->Len )
			HarbolVector_Resize(vA);
		vA->Table[vA->Count++] = vB->Table[i++];
	}
}

HARBOL_EXPORT void HarbolVector_FromHarbolUniList(struct HarbolVector *const v, const struct HarbolUniList *const list)
{
	if( !v || !list )
		return;
	else if( !v->Table || v->Count+list->Len >= v->Len )
		while( v->Count+list->Len >= v->Len )
			HarbolVector_Resize(v);
	
	for( struct HarbolUniListNode *n=list->Head ; n ; n = n->Next )
		v->Table[v->Count++] = n->Data;
}

HARBOL_EXPORT void HarbolVector_FromHarbolBiList(struct HarbolVector *const v, const struct HarbolBiList *const list)
{
	if( !v || !list )
		return;
	else if( !v->Table || v->Count+list->Len >= v->Len )
		while( v->Count+list->Len >= v->Len )
			HarbolVector_Resize(v);
	
	for( struct HarbolBiListNode *n=list->Head ; n ; n = n->Next )
		v->Table[v->Count++] = n->Data;
}

HARBOL_EXPORT void HarbolVector_FromHarbolHashmap(struct HarbolVector *const restrict v, const struct HarbolHashmap *const map)
{
	if( !v || !map )
		return;
	else if( !v->Table || v->Count+map->Count >= v->Len )
		while( v->Count+map->Count >= v->Len )
			HarbolVector_Resize(v);
	
	for( size_t i=0 ; i<map->Len ; i++ ) {
		struct HarbolVector *restrict vec = map->Table + i;
		for( size_t n=0 ; n<HarbolVector_Count(vec) ; n++ ) {
			struct HarbolKeyValPair *node = vec->Table[n].Ptr;
			v->Table[v->Count++] = node->Data;
		}
	}
}

HARBOL_EXPORT void HarbolVector_FromHarbolTuple(struct HarbolVector *const v, const struct HarbolTuple *const tup)
{
	if( !v || !tup || !tup->Items || !tup->Len )
		return;
	else if( !v->Table || v->Count+tup->Len >= v->Len )
		while( v->Count+tup->Len >= v->Len )
			HarbolVector_Resize(v);
	
	size_t i=0;
	while( i<tup->Len )
		v->Table[v->Count++] = tup->Items[i++];
}

HARBOL_EXPORT void HarbolVector_FromHarbolGraph(struct HarbolVector *const v, const struct HarbolGraph *const graph)
{
	if( !v || !graph )
		return;
	else if( !v->Table || v->Count+graph->Vertices.Count >= v->Len )
		while( v->Count+graph->Vertices.Count >= v->Len )
			HarbolVector_Resize(v);
	
	for( size_t i=0 ; i<graph->Vertices.Count ; i++ ) {
		struct HarbolGraphVertex *vert = graph->Vertices.Table[i].Ptr;
		v->Table[v->Count++] = vert->Data;
	}
}

HARBOL_EXPORT void HarbolVector_FromHarbolLinkMap(struct HarbolVector *const v, const struct HarbolLinkMap *const map)
{
	if( !v || !map )
		return;
	else if( !v->Table || v->Count+map->Count >= v->Len )
		while( v->Count+map->Count >= v->Len )
			HarbolVector_Resize(v);
	
	for( size_t i=0 ; i<map->Order.Count ; i++ ) {
		struct HarbolKeyValPair *n = map->Order.Table[i].Ptr;
		v->Table[v->Count++] = n->Data;
	}
}

HARBOL_EXPORT struct HarbolVector *HarbolVector_NewFromHarbolUniList(const struct HarbolUniList *const list)
{
	if( !list )
		return NULL;
	struct HarbolVector *v = HarbolVector_New();
	HarbolVector_FromHarbolUniList(v, list);
	return v;
}

HARBOL_EXPORT struct HarbolVector *HarbolVector_NewFromHarbolBiList(const struct HarbolBiList *const list)
{
	if( !list )
		return NULL;
	struct HarbolVector *v = HarbolVector_New();
	HarbolVector_FromHarbolBiList(v, list);
	return v;
}

HARBOL_EXPORT struct HarbolVector *HarbolVector_NewFromHarbolHashmap(const struct HarbolHashmap *const map)
{
	if( !map )
		return NULL;
	struct HarbolVector *v = HarbolVector_New();
	HarbolVector_FromHarbolHashmap(v, map);
	return v;
}

HARBOL_EXPORT struct HarbolVector *HarbolVector_NewFromHarbolTuple(const struct HarbolTuple *const tup)
{
	if( !tup )
		return NULL;
	struct HarbolVector *v = HarbolVector_New();
	HarbolVector_FromHarbolTuple(v, tup);
	return v;
}

HARBOL_EXPORT struct HarbolVector *HarbolVector_NewFromHarbolGraph(const struct HarbolGraph *const graph)
{
	if( !graph )
		return NULL;
	struct HarbolVector *v = HarbolVector_New();
	HarbolVector_FromHarbolGraph(v, graph);
	return v;
}

HARBOL_EXPORT struct HarbolVector *HarbolVector_NewFromHarbolLinkMap(const struct HarbolLinkMap *const map)
{
	if( !map )
		return NULL;
	struct HarbolVector *v = HarbolVector_New();
	HarbolVector_FromHarbolLinkMap(v, map);
	return v;
}
