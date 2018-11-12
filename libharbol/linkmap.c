#include <stdlib.h>
#include <stdio.h>
#ifdef OS_WINDOWS
	#define HARBOL_LIB
#endif
#include "harbol.h"


HARBOL_EXPORT struct HarbolLinkMap *HarbolLinkMap_New(void)
{
	struct HarbolLinkMap *map = calloc(1, sizeof *map);
	return map;
}

HARBOL_EXPORT void HarbolLinkMap_Init(struct HarbolLinkMap *const map)
{
	if( !map )
		return;
	memset(map, 0, sizeof *map);
}

HARBOL_EXPORT void HarbolLinkMap_Del(struct HarbolLinkMap *const map, fnDestructor *const dtor)
{
	if( !map || !map->Table )
		return;
	
	for( size_t i=0 ; i<map->Len ; i++ ) {
		struct HarbolVector *vec = map->Table+i;
		for( size_t i=0 ; i<vec->Len ; i++ ) {
			struct HarbolKeyValPair *kv = vec->Table[i].Ptr;
			HarbolKeyValPair_Free(&kv, dtor);
		}
		HarbolVector_Del(vec, NULL);
	}
	free(map->Table), map->Table=NULL;
	HarbolVector_Del(&map->Order, NULL);
	memset(map, 0, sizeof *map);
}

HARBOL_EXPORT void HarbolLinkMap_Free(struct HarbolLinkMap **linkmapref, fnDestructor *const dtor)
{
	if( !*linkmapref )
		return;
	
	HarbolLinkMap_Del(*linkmapref, dtor);
	free(*linkmapref); *linkmapref=NULL;
}

HARBOL_EXPORT size_t HarbolLinkMap_Count(const struct HarbolLinkMap *const map)
{
	return map ? map->Count : 0;
}

HARBOL_EXPORT size_t HarbolLinkMap_Len(const struct HarbolLinkMap *const map)
{
	return map ? map->Len : 0;
}

HARBOL_EXPORT bool HarbolLinkMap_Rehash(struct HarbolLinkMap *const map)
{
	if( !map || !map->Table )
		return false;
	/*
	const size_t old_size = map->Len;
	map->Len <<= 1;
	map->Count = 0;
	
	struct HarbolVector
		*curr = NULL,
		*temp = calloc(map->Len, sizeof *temp)
	;
	if( !temp ) {
		puts("**** Memory Allocation Error **** HarbolMap_Rehash::temp is NULL\n");
		map->Len = 0;
		return false;
	}
	
	curr = map->Table;
	map->Table = temp;
	
	HarbolVector_Del(&map->Order, NULL);
	for( size_t i=0 ; i<old_size ; i++ ) {
		struct HarbolVector *vec = curr + i;
		for( size_t n=0 ; n<HarbolVector_Count(vec) ; n++ ) {
			struct HarbolKeyValPair *node = vec->Table[n].Ptr;
			HarbolLinkMap_InsertNode(map, node);
		}
		HarbolVector_Del(vec, NULL);
	}
	free(curr), curr=NULL;
	
	return true; */
	return HarbolMap_Rehash(&map->Map);
}

HARBOL_EXPORT bool HarbolLinkMap_InsertNode(struct HarbolLinkMap *const map, struct HarbolKeyValPair *node)
{
	if( !map || !node || !node->KeyName.CStr )
		return false;
	
	else if( !map->Len ) {
		map->Len = 8;
		map->Table = calloc(map->Len, sizeof *map->Table);
		if( !map->Table ) {
			puts("**** Memory Allocation Error **** HarbolMap_InsertNode::map->Table is NULL\n");
			map->Len = 0;
			return false;
		}
	}
	else if( map->Count >= map->Len )
		HarbolLinkMap_Rehash(map);
	else if( HarbolLinkMap_HasKey(map, node->KeyName.CStr) )
		return false;
	
	const size_t hash = GenHash(node->KeyName.CStr) % map->Len;
	HarbolVector_Insert(map->Table + hash, (union HarbolValue){.Ptr=node});
	HarbolVector_Insert(&map->Order, (union HarbolValue){.Ptr=node});
	++map->Count;
	return true;
}

HARBOL_EXPORT bool HarbolLinkMap_Insert(struct HarbolLinkMap *const restrict map, const char *restrict strkey, const union HarbolValue val)
{
	if( !map || !strkey )
		return false;
	
	struct HarbolKeyValPair *node = HarbolKeyValPair_NewSP(strkey, val);
	bool b = HarbolLinkMap_InsertNode(map, node);
	if( !b )
		HarbolKeyValPair_Free(&node, NULL);
	return b;
}

HARBOL_EXPORT struct HarbolKeyValPair *HarbolLinkMap_GetNodeByIndex(const struct HarbolLinkMap *const map, const size_t index)
{
	return ( !map || !map->Table || !map->Order.Table ) ? NULL : map->Order.Table[index].Ptr;
}

HARBOL_EXPORT union HarbolValue HarbolLinkMap_Get(const struct HarbolLinkMap *const restrict map, const char *restrict strkey)
{
	return ( !map || !HarbolMap_HasKey(&map->Map, strkey) ) ? (union HarbolValue){0} : HarbolMap_Get(&map->Map, strkey);
}

HARBOL_EXPORT void HarbolLinkMap_Set(struct HarbolLinkMap *const restrict map, const char *restrict strkey, const union HarbolValue val)
{
	if( !map || !HarbolLinkMap_HasKey(map, strkey) )
		return;
	
	HarbolMap_Set(&map->Map, strkey, val);
}

HARBOL_EXPORT union HarbolValue HarbolLinkMap_GetByIndex(const struct HarbolLinkMap *const map, const size_t index)
{
	if( !map || !map->Table )
		return (union HarbolValue){0};
	
	struct HarbolKeyValPair *node = HarbolLinkMap_GetNodeByIndex(map, index);
	return ( node ) ? node->Data : (union HarbolValue){0};
}

HARBOL_EXPORT void HarbolLinkMap_SetByIndex(struct HarbolLinkMap *const map, const size_t index, const union HarbolValue val)
{
	if( !map || !map->Table )
		return;
	
	struct HarbolKeyValPair *node = HarbolLinkMap_GetNodeByIndex(map, index);
	if( node )
		node->Data = val;
}

HARBOL_EXPORT void HarbolLinkMap_Delete(struct HarbolLinkMap *const restrict map, const char *restrict strkey, fnDestructor *const dtor)
{
	if( !map || !map->Table || !HarbolLinkMap_HasKey(map, strkey) )
		return;
	
	const size_t index = HarbolLinkMap_GetIndexByName(map, strkey);
	HarbolMap_Delete(&map->Map, strkey, dtor);
	HarbolVector_Delete(&map->Order, index, NULL);
}

HARBOL_EXPORT void HarbolLinkMap_DeleteByIndex(struct HarbolLinkMap *const map, const size_t index, fnDestructor *const dtor)
{
	if( !map || !map->Table )
		return;
	
	struct HarbolKeyValPair *kv = HarbolLinkMap_GetNodeByIndex(map, index);
	if( !kv )
		return;
	
	HarbolMap_Delete(&map->Map, kv->KeyName.CStr, dtor);
	HarbolVector_Delete(&map->Order, index, NULL);
}

HARBOL_EXPORT bool HarbolLinkMap_HasKey(const struct HarbolLinkMap *const restrict map, const char *restrict strkey)
{
	return !map || !map->Table ? false : HarbolMap_HasKey(&map->Map, strkey);
}

HARBOL_EXPORT struct HarbolKeyValPair *HarbolLinkMap_GetKeyValByKey(const struct HarbolLinkMap *const restrict map, const char *restrict strkey)
{
	if( !map || !map->Table )
		return NULL;
	
	return HarbolMap_GetHarbolKeyValPair(&map->Map, strkey);
}

HARBOL_EXPORT struct HarbolVector *HarbolLinkMap_GetBuckets(const struct HarbolLinkMap *const map)
{
	return map ? map->Table : NULL;
}

HARBOL_EXPORT union HarbolValue *HarbolLinkMap_GetIter(const struct HarbolLinkMap *const map)
{
	return map ? map->Order.Table : NULL;
}

HARBOL_EXPORT union HarbolValue *HarbolLinkMap_GetIterEndLen(const struct HarbolLinkMap *const map)
{
	return map ? map->Order.Table + map->Order.Len : NULL;
}

HARBOL_EXPORT union HarbolValue *HarbolLinkMap_GetIterEndCount(const struct HarbolLinkMap *const map)
{
	return map ? map->Order.Table + map->Order.Count : NULL;
}

HARBOL_EXPORT size_t HarbolLinkMap_GetIndexByName(const struct HarbolLinkMap *const restrict map, const char *restrict strkey)
{
	if( !map || !strkey )
		return SIZE_MAX;
	
	for( size_t i=0 ; i<map->Order.Count ; i++ ) {
		struct HarbolKeyValPair *kv = map->Order.Table[i].Ptr;
		if( !HarbolString_CmpCStr(&kv->KeyName, strkey) )
			return i;
	}
	return SIZE_MAX;
}

HARBOL_EXPORT size_t HarbolLinkMap_GetIndexByNode(const struct HarbolLinkMap *const map, struct HarbolKeyValPair *const node)
{
	if( !map || !node )
		return SIZE_MAX;
	
	for( size_t i=0 ; i<map->Order.Count ; i++ ) {
		if( (uintptr_t)map->Order.Table[i].Ptr == (uintptr_t)node )
			return i;
	}
	return SIZE_MAX;
}

HARBOL_EXPORT size_t HarbolLinkMap_GetIndexByValue(const struct HarbolLinkMap *const map, const union HarbolValue val)
{
	if( !map )
		return SIZE_MAX;
	
	for( size_t i=0 ; i<map->Order.Count ; i++ ) {
		struct HarbolKeyValPair *n = map->Order.Table[i].Ptr;
		if( n->Data.UInt64 == val.UInt64 )
			return i;
	}
	return SIZE_MAX;
}

HARBOL_EXPORT void HarbolLinkMap_FromHarbolHashmap(struct HarbolLinkMap *const linkmap, const struct HarbolHashmap *const map)
{
	if( !linkmap || !map )
		return;
	
	for( size_t i=0 ; i<map->Len ; i++ ) {
		struct HarbolVector *vec = map->Table + i;
		for( size_t n=0 ; n<HarbolVector_Count(vec) ; n++ ) {
			struct HarbolKeyValPair *kv = vec->Table[n].Ptr;
			HarbolLinkMap_InsertNode(linkmap, HarbolKeyValPair_NewSP(kv->KeyName.CStr, kv->Data));
		}
	}
}

HARBOL_EXPORT void HarbolLinkMap_FromHarbolUniList(struct HarbolLinkMap *const map, const struct HarbolUniList *const list)
{
	if( !map || !list )
		return;
	
	size_t i=0;
	for( struct HarbolUniListNode *n=list->Head ; n ; n = n->Next ) {
		char cstrkey[10] = {0};
		sprintf(cstrkey, "%zu", i);
		HarbolLinkMap_Insert(map, cstrkey, n->Data);
		i++;
	}
}

HARBOL_EXPORT void HarbolLinkMap_FromHarbolBiList(struct HarbolLinkMap *const map, const struct HarbolBiList *const list)
{
	if( !map || !list )
		return;
	
	size_t i=0;
	for( struct HarbolBiListNode *n=list->Head ; n ; n = n->Next ) {
		char cstrkey[10] = {0};
		sprintf(cstrkey, "%zu", i);
		HarbolLinkMap_Insert(map, cstrkey, n->Data);
		i++;
	}
}

HARBOL_EXPORT void HarbolLinkMap_FromHarbolVector(struct HarbolLinkMap *const map, const struct HarbolVector *const v)
{
	if( !map || !v )
		return;
	
	for( size_t i=0 ; i<v->Count ; i++ ) {
		char cstrkey[10] = {0};
		sprintf(cstrkey, "%zu", i);
		HarbolLinkMap_Insert(map, cstrkey, v->Table[i]);
	}
}

HARBOL_EXPORT void HarbolLinkMap_FromHarbolTuple(struct HarbolLinkMap *const map, const struct HarbolTuple *const tup)
{
	if( !map || !tup || !tup->Items || !tup->Len )
		return;
	
	for( size_t i=0 ; i<tup->Len ; i++ ) {
		char cstrkey[10] = {0};
		sprintf(cstrkey, "%zu", i);
		HarbolLinkMap_Insert(map, cstrkey, tup->Items[i]);
	}
}

HARBOL_EXPORT void HarbolLinkMap_FromHarbolGraph(struct HarbolLinkMap *const map, const struct HarbolGraph *const graph)
{
	if( !map || !graph )
		return;
	
	for( size_t i=0 ; i<graph->Vertices.Count ; i++ ) {
		char cstrkey[10] = {0};
		sprintf(cstrkey, "%zu", i);
		struct HarbolGraphVertex *vert = graph->Vertices.Table[i].Ptr;
		HarbolLinkMap_Insert(map, cstrkey, vert->Data);
	}
}

HARBOL_EXPORT struct HarbolLinkMap *HarbolLinkMap_NewFromHarbolHashmap(const struct HarbolHashmap *const map)
{
	if( !map )
		return NULL;
	
	struct HarbolLinkMap *linkmap = HarbolLinkMap_New();
	HarbolLinkMap_FromHarbolHashmap(linkmap, map);
	return linkmap;
}

HARBOL_EXPORT struct HarbolLinkMap *HarbolLinkMap_NewFromHarbolUniList(const struct HarbolUniList *const list)
{
	if( !list )
		return NULL;
	
	struct HarbolLinkMap *map = HarbolLinkMap_New();
	HarbolLinkMap_FromHarbolUniList(map, list);
	return map;
}

HARBOL_EXPORT struct HarbolLinkMap *HarbolLinkMap_NewFromHarbolBiList(const struct HarbolBiList *const list)
{
	if( !list )
		return NULL;
	
	struct HarbolLinkMap *map = HarbolLinkMap_New();
	HarbolLinkMap_FromHarbolBiList(map, list);
	return map;
}

HARBOL_EXPORT struct HarbolLinkMap *HarbolLinkMap_NewFromHarbolVector(const struct HarbolVector *const vec)
{
	if( !vec )
		return NULL;
	
	struct HarbolLinkMap *map = HarbolLinkMap_New();
	HarbolLinkMap_FromHarbolVector(map, vec);
	return map;
}

HARBOL_EXPORT struct HarbolLinkMap *HarbolLinkMap_NewFromHarbolTuple(const struct HarbolTuple *const tup)
{
	if( !tup )
		return NULL;
	
	struct HarbolLinkMap *map = HarbolLinkMap_New();
	HarbolLinkMap_FromHarbolTuple(map, tup);
	return map;
}

HARBOL_EXPORT struct HarbolLinkMap *HarbolLinkMap_NewFromHarbolGraph(const struct HarbolGraph *const graph)
{
	if( !graph )
		return NULL;
	
	struct HarbolLinkMap *map = HarbolLinkMap_New();
	HarbolLinkMap_FromHarbolGraph(map, graph);
	return map;
}
