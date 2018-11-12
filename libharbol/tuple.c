#include <stdlib.h>
#include <stdio.h>
#ifdef OS_WINDOWS
	#define HARBOL_LIB
#endif
#include "harbol.h"

HARBOL_EXPORT struct HarbolTuple *HarbolTuple_New(const size_t arrlen, union HarbolValue items[static arrlen])
{
	struct HarbolTuple *tup = calloc(1, sizeof *tup);
	if( tup )
		HarbolTuple_Init(tup, arrlen, items);
	return tup;
}

HARBOL_EXPORT void HarbolTuple_Free(struct HarbolTuple **tupref)
{
	if( !*tupref )
		return;
	HarbolTuple_Del(*tupref);
	free(*tupref); *tupref=NULL;
}

HARBOL_EXPORT void HarbolTuple_Init(struct HarbolTuple *const tup, const size_t arrlen, union HarbolValue items[static arrlen])
{
	if( !tup )
		return;
	memset(tup, 0, sizeof *tup);
	tup->Items = calloc(arrlen, sizeof *tup->Items);
	if( !tup->Items )
		return;
	memcpy(tup->Items, items, sizeof *tup->Items * arrlen);
	tup->Len = arrlen;
}

HARBOL_EXPORT void HarbolTuple_Del(struct HarbolTuple *const tup)
{
	if( !tup || !tup->Items )
		return;
	free(tup->Items); tup->Items=NULL;
	memset(tup, 0, sizeof *tup);
}

HARBOL_EXPORT size_t HarbolTuple_Len(const struct HarbolTuple *const tup)
{
	return tup ? tup->Len : 0 ;
}

HARBOL_EXPORT union HarbolValue *HarbolTuple_GetItems(const struct HarbolTuple *const tup)
{
	return tup ? tup->Items : NULL ;
}

HARBOL_EXPORT union HarbolValue HarbolTuple_GetItem(const struct HarbolTuple *const tup, const size_t index)
{
	return ( !tup || !tup->Items || index >= tup->Len ) ? (union HarbolValue){0} : tup->Items[index];
}

HARBOL_EXPORT void HarbolTuple_FromHarbolUniList(struct HarbolTuple *const tup, const struct HarbolUniList *const list)
{
	if( !tup || !list )
		return;
	
	if( tup->Items )
		HarbolTuple_Del(tup);
	
	union HarbolValue list_items[list->Len];
	size_t i=0;
	for( struct HarbolUniListNode *n=list->Head ; n ; n=n->Next )
		list_items[i++] = n->Data;
	
	HarbolTuple_Init(tup, i, list_items);
}

HARBOL_EXPORT void HarbolTuple_FromHarbolHashmap(struct HarbolTuple *const tup, const struct HarbolHashmap *const map)
{
	if( !tup || !map )
		return;
	
	if( tup->Items )
		HarbolTuple_Del(tup);
	
	union HarbolValue list_items[map->Count];
	size_t x=0;
	for( size_t i=0 ; i<map->Len ; i++ ) {
		struct HarbolVector *vec = map->Table + i;
		for( size_t n=0 ; n<HarbolVector_Count(vec) ; n++ ) {
			struct HarbolKeyValPair *node = vec->Table[n].Ptr;
			list_items[x++] = node->Data;
		}
	}
	HarbolTuple_Init(tup, x, list_items);
}

HARBOL_EXPORT void HarbolTuple_FromHarbolVector(struct HarbolTuple *const tup, const struct HarbolVector *const vec)
{
	if( !tup || !vec )
		return;
	
	if( tup->Items )
		HarbolTuple_Del(tup);
	
	HarbolTuple_Init(tup, vec->Count, vec->Table);
}

HARBOL_EXPORT void HarbolTuple_FromHarbolBiList(struct HarbolTuple *const tup, const struct HarbolBiList *const list)
{
	if( !tup || !list )
		return;
	
	if( tup->Items )
		HarbolTuple_Del(tup);
	
	union HarbolValue list_items[list->Len];
	size_t i=0;
	for( struct HarbolBiListNode *n=list->Head ; n ; n=n->Next )
		list_items[i++] = n->Data;
	
	HarbolTuple_Init(tup, i, list_items);
}

HARBOL_EXPORT void HarbolTuple_FromHarbolGraph(struct HarbolTuple *const tup, const struct HarbolGraph *const graph)
{
	if( !tup || !graph )
		return;
	
	if( tup->Items )
		HarbolTuple_Del(tup);
	
	union HarbolValue list_items[graph->Vertices.Count];
	for( size_t i=0 ; i<graph->Vertices.Count ; i++ ) {
		struct HarbolGraphVertex *vert = graph->Vertices.Table[i].Ptr;
		list_items[i] = vert->Data;
	}
	HarbolTuple_Init(tup, graph->Vertices.Count, list_items);
}

HARBOL_EXPORT void HarbolTuple_FromHarbolLinkMap(struct HarbolTuple *const tup, const struct HarbolLinkMap *const map)
{
	if( tup->Items )
		HarbolTuple_Del(tup);
	
	union HarbolValue list_items[map->Count];
	size_t i=0;
	for( ; i<map->Order.Count ; i++ ) {
		struct HarbolKeyValPair *n = map->Order.Table[i].Ptr;
		list_items[i] = n->Data;
	}
	HarbolTuple_Init(tup, i, list_items);
}

HARBOL_EXPORT struct HarbolTuple *HarbolTuple_NewFromHarbolUniList(const struct HarbolUniList *const list)
{
	if( !list )
		return NULL;
	struct HarbolTuple *const tup = calloc(1, sizeof *tup);
	HarbolTuple_FromHarbolUniList(tup, list);
	return tup;
}

HARBOL_EXPORT struct HarbolTuple *HarbolTuple_NewFromHarbolHashmap(const struct HarbolHashmap *const map)
{
	if( !map )
		return NULL;
	struct HarbolTuple *const tup = calloc(1, sizeof *tup);
	HarbolTuple_FromHarbolHashmap(tup, map);
	return tup;
}

HARBOL_EXPORT struct HarbolTuple *HarbolTuple_NewFromHarbolVector(const struct HarbolVector *const vec)
{
	if( !vec )
		return NULL;
	struct HarbolTuple *const tup = calloc(1, sizeof *tup);
	HarbolTuple_FromHarbolVector(tup, vec);
	return tup;
}

HARBOL_EXPORT struct HarbolTuple *HarbolTuple_NewFromHarbolBiList(const struct HarbolBiList *const list)
{
	if( !list )
		return NULL;
	struct HarbolTuple *const tup = calloc(1, sizeof *tup);
	HarbolTuple_FromHarbolBiList(tup, list);
	return tup;
}

HARBOL_EXPORT struct HarbolTuple *HarbolTuple_NewFromHarbolGraph(const struct HarbolGraph *const graph)
{
	if( !graph )
		return NULL;
	struct HarbolTuple *const tup = calloc(1, sizeof *tup);
	HarbolTuple_FromHarbolGraph(tup, graph);
	return tup;
}

HARBOL_EXPORT struct HarbolTuple *HarbolTuple_NewFromHarbolLinkMap(const struct HarbolLinkMap *const map)
{
	if( !map )
		return NULL;
	struct HarbolTuple *const tup = calloc(1, sizeof *tup);
	HarbolTuple_FromHarbolLinkMap(tup, map);
	return tup;
}
