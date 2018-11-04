
#include <stdlib.h>
#include <stdio.h>
#include "dsc.h"


struct LinkMap *LinkMap_New(void)
{
	struct LinkMap *map = calloc(1, sizeof *map);
	return map;
}

void LinkMap_Init(struct LinkMap *const map)
{
	if( !map )
		return;
	
	*map = (struct LinkMap){0};
}
void LinkMap_Del(struct LinkMap *const map, fnDestructor *const dtor)
{
	if( !map || !map->Table )
		return;
	
	for( size_t i=0 ; i<map->Len ; i++ ) {
		struct Vector *vec = map->Table+i;
		for( size_t i=0 ; i<vec->Len ; i++ ) {
			struct KeyValPair *kv = vec->Table[i].Ptr;
			KeyValPair_Free(&kv, dtor);
		}
		Vector_Del(vec, NULL);
	}
	free(map->Table), map->Table=NULL;
	Vector_Del(&map->Order, NULL);
	*map = (struct LinkMap){0};
}

void LinkMap_Free(struct LinkMap **linkmapref, fnDestructor *const dtor)
{
	if( !*linkmapref )
		return;
	
	LinkMap_Del(*linkmapref, dtor);
	free(*linkmapref); *linkmapref=NULL;
}

size_t LinkMap_Count(const struct LinkMap *const map)
{
	return map ? map->Count : 0;
}
size_t LinkMap_Len(const struct LinkMap *const map)
{
	return map ? map->Len : 0;
}
bool LinkMap_Rehash(struct LinkMap *const map)
{
	if( !map || !map->Table )
		return false;
	
	const size_t old_size = map->Len;
	map->Len <<= 1;
	map->Count = 0;
	
	struct Vector
		*curr = NULL,
		*temp = calloc(map->Len, sizeof *temp)
	;
	if( !temp ) {
		puts("**** Memory Allocation Error **** Map_Rehash::temp is NULL\n");
		map->Len = 0;
		return false;
	}
	
	curr = map->Table;
	map->Table = temp;
	
	Vector_Del(&map->Order, NULL);
	for( size_t i=0 ; i<old_size ; i++ ) {
		struct Vector *vec = curr + i;
		for( size_t n=0 ; n<Vector_Count(vec) ; n++ ) {
			struct KeyValPair *node = vec->Table[n].Ptr;
			LinkMap_InsertNode(map, node);
		}
		Vector_Del(vec, NULL);
	}
	free(curr), curr=NULL;
	return true;
}

bool LinkMap_InsertNode(struct LinkMap *const map, struct KeyValPair *node)
{
	if( !map || !node || !node->KeyName.CStr )
		return false;
	
	else if( !map->Len ) {
		map->Len = 8;
		map->Table = calloc(map->Len, sizeof *map->Table);
		if( !map->Table ) {
			puts("**** Memory Allocation Error **** Map_InsertNode::map->Table is NULL\n");
			map->Len = 0;
			return false;
		}
	}
	else if( map->Count >= map->Len )
		LinkMap_Rehash(map);
	else if( LinkMap_HasKey(map, node->KeyName.CStr) )
		return false;
	
	const size_t hash = GenHash(node->KeyName.CStr) % map->Len;
	Vector_Insert(map->Table + hash, (union Value){.Ptr=node});
	Vector_Insert(&map->Order, (union Value){.Ptr=node});
	++map->Count;
	return true;
}
bool LinkMap_Insert(struct LinkMap *const restrict map, const char *restrict strkey, const union Value val)
{
	if( !map || !strkey )
		return false;
	
	struct KeyValPair *node = KeyValPair_NewSP(strkey, val);
	bool b = LinkMap_InsertNode(map, node);
	if( !b )
		KeyValPair_Free(&node, NULL);
	return b;
}

struct KeyValPair *LinkMap_GetNodeByIndex(const struct LinkMap *const map, const size_t index)
{
	if( !map || !map->Table || !map->Order.Table )
		return NULL;
	
	return map->Order.Table[index].Ptr;
}

union Value LinkMap_Get(const struct LinkMap *const restrict map, const char *restrict strkey)
{
	if( !map || !Map_HasKey(&map->Map, strkey) )
		return (union Value){0};
	
	return Map_Get(&map->Map, strkey);
}
void LinkMap_Set(struct LinkMap *const restrict map, const char *restrict strkey, const union Value val)
{
	if( !map || !LinkMap_HasKey(map, strkey) )
		return;
	
	Map_Set(&map->Map, strkey, val);
}

union Value LinkMap_GetByIndex(const struct LinkMap *const map, const size_t index)
{
	if( !map || !map->Table )
		return (union Value){0};
	
	struct KeyValPair *node = LinkMap_GetNodeByIndex(map, index);
	return ( node ) ? node->Data : (union Value){0};
}

void LinkMap_SetByIndex(struct LinkMap *const map, const size_t index, const union Value val)
{
	if( !map || !map->Table )
		return;
	
	struct KeyValPair *node = LinkMap_GetNodeByIndex(map, index);
	if( node )
		node->Data = val;
}

void LinkMap_Delete(struct LinkMap *const restrict map, const char *restrict strkey, fnDestructor *const dtor)
{
	if( !map || !map->Table || !LinkMap_HasKey(map, strkey) )
		return;
	
	Map_Delete(&map->Map, strkey, dtor);
}

void LinkMap_DeleteByIndex(struct LinkMap *const map, const size_t index, fnDestructor *const dtor)
{
	if( !map || !map->Table )
		return;
	
	struct KeyValPair *kv = LinkMap_GetNodeByIndex(map, index);
	if( !kv )
		return;
	
	Map_Delete(&map->Map, kv->KeyName.CStr, dtor);
	Vector_Delete(&map->Order, index, NULL);
}

bool LinkMap_HasKey(const struct LinkMap *const restrict map, const char *restrict strkey)
{
	return !map || !map->Table ? false : Map_HasKey(&map->Map, strkey);
}
struct KeyValPair *LinkMap_GetNodeByKey(const struct LinkMap *const restrict map, const char *restrict strkey)
{
	if( !map || !map->Table )
		return NULL;
	
	return Map_GetKeyValPair(&map->Map, strkey);
}
struct Vector *LinkMap_GetKeyTable(const struct LinkMap *const map)
{
	return map ? map->Table : NULL;
}

size_t LinkMap_GetIndexByName(const struct LinkMap *const restrict map, const char *restrict strkey)
{
	if( !map || !strkey )
		return SIZE_MAX;
	
	for( size_t i=0 ; i<map->Order.Count ; i++ ) {
		struct KeyValPair *kv = map->Order.Table[i].Ptr;
		if( !String_CmpCStr(&kv->KeyName, strkey) )
			return i;
	}
	return SIZE_MAX;
}

size_t LinkMap_GetIndexByNode(const struct LinkMap *const map, struct KeyValPair *const node)
{
	if( !map || !node )
		return SIZE_MAX;
	
	for( size_t i=0 ; i<map->Order.Count ; i++ ) {
		if( (uintptr_t)map->Order.Table[i].Ptr == (uintptr_t)node )
			return i;
	}
	return SIZE_MAX;
}

size_t LinkMap_GetIndexByValue(const struct LinkMap *const map, const union Value val)
{
	if( !map )
		return SIZE_MAX;
	
	for( size_t i=0 ; i<map->Order.Count ; i++ ) {
		struct KeyValPair *n = map->Order.Table[i].Ptr;
		if( n->Data.UInt64 == val.UInt64 )
			return i;
	}
	return SIZE_MAX;
}


void LinkMap_FromMap(struct LinkMap *const linkmap, const struct Hashmap *const map)
{
	if( !linkmap || !map )
		return;
	
	for( size_t i=0 ; i<map->Len ; i++ ) {
		struct Vector *vec = map->Table + i;
		for( size_t n=0 ; n<Vector_Count(vec) ; n++ ) {
			struct KeyValPair *kv = vec->Table[n].Ptr;
			LinkMap_InsertNode(linkmap, KeyValPair_NewSP(kv->KeyName.CStr, kv->Data));
		}
	}
}

void LinkMap_FromUniLinkedList(struct LinkMap *const map, const struct UniLinkedList *const list)
{
	if( !map || !list )
		return;
	
	size_t i=0;
	for( struct UniListNode *n=list->Head ; n ; n = n->Next ) {
		char cstrkey[10] = {0};
		sprintf(cstrkey, "%zu", i);
		LinkMap_Insert(map, cstrkey, n->Data);
		i++;
	}
}
void LinkMap_FromBiLinkedList(struct LinkMap *const map, const struct BiLinkedList *const list)
{
	if( !map || !list )
		return;
	
	size_t i=0;
	for( struct BiListNode *n=list->Head ; n ; n = n->Next ) {
		char cstrkey[10] = {0};
		sprintf(cstrkey, "%zu", i);
		LinkMap_Insert(map, cstrkey, n->Data);
		i++;
	}
}
void LinkMap_FromVector(struct LinkMap *const map, const struct Vector *const v)
{
	if( !map || !v )
		return;
	
	for( size_t i=0 ; i<v->Count ; i++ ) {
		char cstrkey[10] = {0};
		sprintf(cstrkey, "%zu", i);
		LinkMap_Insert(map, cstrkey, v->Table[i]);
	}
}
void LinkMap_FromTuple(struct LinkMap *const map, const struct Tuple *const tup)
{
	if( !map || !tup || !tup->Items || !tup->Len )
		return;
	
	for( size_t i=0 ; i<tup->Len ; i++ ) {
		char cstrkey[10] = {0};
		sprintf(cstrkey, "%zu", i);
		LinkMap_Insert(map, cstrkey, tup->Items[i]);
	}
}
void LinkMap_FromGraph(struct LinkMap *const map, const struct Graph *const graph)
{
	if( !map || !graph )
		return;
	
	for( size_t i=0 ; i<graph->Vertices.Count ; i++ ) {
		char cstrkey[10] = {0};
		sprintf(cstrkey, "%zu", i);
		struct GraphVertex *vert = graph->Vertices.Table[i].Ptr;
		LinkMap_Insert(map, cstrkey, vert->Data);
	}
}

struct LinkMap *LinkMap_NewFromMap(const struct Hashmap *const map)
{
	if( !map )
		return NULL;
	
	struct LinkMap *linkmap = LinkMap_New();
	LinkMap_FromMap(linkmap, map);
	return linkmap;
}
struct LinkMap *LinkMap_NewFromUniLinkedList(const struct UniLinkedList *const list)
{
	if( !list )
		return NULL;
	
	struct LinkMap *map = LinkMap_New();
	LinkMap_FromUniLinkedList(map, list);
	return map;
}
struct LinkMap *LinkMap_NewFromBiLinkedList(const struct BiLinkedList *const list)
{
	if( !list )
		return NULL;
	
	struct LinkMap *map = LinkMap_New();
	LinkMap_FromBiLinkedList(map, list);
	return map;
}
struct LinkMap *LinkMap_NewFromVector(const struct Vector *const vec)
{
	if( !vec )
		return NULL;
	
	struct LinkMap *map = LinkMap_New();
	LinkMap_FromVector(map, vec);
	return map;
}
struct LinkMap *LinkMap_NewFromTuple(const struct Tuple *const tup)
{
	if( !tup )
		return NULL;
	
	struct LinkMap *map = LinkMap_New();
	LinkMap_FromTuple(map, tup);
	return map;
}
struct LinkMap *LinkMap_NewFromGraph(const struct Graph *const graph)
{
	if( !graph )
		return NULL;
	
	struct LinkMap *map = LinkMap_New();
	LinkMap_FromGraph(map, graph);
	return map;
}
