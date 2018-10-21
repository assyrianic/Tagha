
#include <stdlib.h>
#include <stdio.h>
#include "dsc.h"


struct KeyNode *KeyNode_New(void)
{
	return calloc(1, sizeof(struct KeyNode));
}

struct KeyNode *KeyNode_NewSP(const char *restrict cstr, const union Value val)
{
	struct KeyNode *restrict n = KeyNode_New();
	if( n ) {
		String_InitStr(&n->KeyName, cstr);
		n->Data = val;
	}
	return n;
}

void KeyNode_Del(struct KeyNode *const n, fnDestructor *const dtor)
{
	if( !n )
		return;
	
	String_Del(&n->KeyName);
	if( dtor )
		(*dtor)(&n->Data.Ptr);
}

void KeyNode_Free(struct KeyNode **noderef, fnDestructor *const dtor)
{
	if( !noderef || !*noderef )
		return;
	
	KeyNode_Del(*noderef, dtor);
	free(*noderef), *noderef=NULL;
}


// size_t general hash function.
size_t GenHash(const char *restrict cstr)
{
	if( !cstr )
		return SIZE_MAX;
	
	size_t h = 0;
	while( *cstr )
		h = 37 * h + *cstr++;
	return h;
}

struct Hashmap *Map_New(void)
{
	struct Hashmap *map = calloc(1, sizeof *map);
	return map;
}

void Map_Init(struct Hashmap *const map)
{
	if( !map )
		return;
	
	*map = (struct Hashmap){0};
}

void Map_Del(struct Hashmap *const map, fnDestructor *const dtor)
{
	if( !map || !map->Table )
		return;
	
	for( size_t i=0 ; i<map->Len ; i++ ) {
		struct Vector *vec = map->Table+i;
		for( size_t i=0 ; i<vec->Len ; i++ ) {
			struct KeyNode *kv = vec->Table[i].Ptr;
			KeyNode_Free(&kv, dtor);
		}
		Vector_Del(vec, NULL);
	}
	free(map->Table), map->Table=NULL;
	*map = (struct Hashmap){0};
}

void Map_Free(struct Hashmap **mapref, fnDestructor *const dtor)
{
	if( !*mapref )
		return;
	
	Map_Del(*mapref, dtor);
	free(*mapref), *mapref=NULL;
}

size_t Map_Count(const struct Hashmap *const map)
{
	return map ? map->Count : 0;
}

size_t Map_Len(const struct Hashmap *const map)
{
	return map ? map->Len : 0;
}

bool Map_Rehash(struct Hashmap *const map)
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
	
	for( size_t i=0 ; i<old_size ; i++ ) {
		struct Vector *restrict vec = curr + i;
		for( size_t n=0 ; n<Vector_Count(vec) ; n++ ) {
			struct KeyNode *node = vec->Table[n].Ptr;
			Map_InsertNode(map, node);
		}
		Vector_Del(vec, NULL);
	}
	free(curr), curr=NULL;
	return true;
}

bool Map_InsertNode(struct Hashmap *const map, struct KeyNode *node)
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
		Map_Rehash(map);
	else if( Map_HasKey(map, node->KeyName.CStr) ) {
		puts("Map_InsertNode::map already has entry!\n");
		return false;
	}
	
	const size_t hash = GenHash(node->KeyName.CStr) % map->Len;
	Vector_Insert(map->Table + hash, (union Value){.Ptr=node});
	++map->Count;
	return true;
}


bool Map_Insert(struct Hashmap *const restrict map, const char *restrict strkey, const union Value val)
{
	if( !map || !strkey )
		return false;
	
	struct KeyNode *node = KeyNode_NewSP(strkey, val);
	bool b = Map_InsertNode(map, node);
	if( !b )
		KeyNode_Free(&node, NULL);
	return b;
}

union Value Map_Get(const struct Hashmap *const restrict map, const char *restrict strkey)
{
	if( !map || !map->Table || !Map_HasKey(map, strkey) )
		return (union Value){0};
	
	const size_t hash = GenHash(strkey) % map->Len;
	struct Vector *restrict vec = map->Table+hash;
	for( size_t i=0 ; i<Vector_Count(vec) ; i++ ) {
		const struct KeyNode *restrict kv = vec->Table[i].Ptr;
		if( !String_CmpCStr(&kv->KeyName, strkey) )
			return kv->Data;
	}
	return (union Value){0};
}

void Map_Set(struct Hashmap *const restrict map, const char *restrict strkey, const union Value val)
{
	if( !map || !Map_HasKey(map, strkey) )
		return;
	
	const size_t hash = GenHash(strkey) % map->Len;
	struct Vector *restrict vec = map->Table+hash;
	for( size_t i=0 ; i<Vector_Count(vec) ; i++ ) {
		struct KeyNode *restrict kv = vec->Table[i].Ptr;
		if( !String_CmpCStr(&kv->KeyName, strkey) )
			kv->Data = val;
	}
}

void Map_Delete(struct Hashmap *const restrict map, const char *restrict strkey, fnDestructor *const dtor)
{
	if( !map || !map->Table || !Map_HasKey(map, strkey) )
		return;
	
	const size_t hash = GenHash(strkey) % map->Len;
	struct Vector *restrict vec = map->Table+hash;
	for( size_t i=0 ; i<vec->Count ; i++ ) {
		struct KeyNode *kv = vec->Table[i].Ptr;
		if( !String_CmpCStr(&kv->KeyName, strkey) ) {
			KeyNode_Del(kv, dtor);
			Vector_Delete(vec, i, NULL);
			map->Count--;
			break;
		}
	}
}

bool Map_HasKey(const struct Hashmap *const restrict map, const char *restrict strkey)
{
	if( !map || !map->Table )
		return false;
	
	const size_t hash = GenHash(strkey) % map->Len;
	struct Vector *restrict vec = map->Table+hash;
	for( size_t i=0 ; i<Vector_Count(vec) ; i++ ) {
		const struct KeyNode *restrict kv = vec->Table[i].Ptr;
		if( !String_CmpCStr(&kv->KeyName, strkey) )
			return true;
	}
	return false;
}

struct KeyNode *Map_GetKeyNode(const struct Hashmap *const restrict map, const char *restrict strkey)
{
	if( !map || !strkey || !map->Table )
		return NULL;
	
	const size_t hash = GenHash(strkey) % map->Len;
	struct Vector *restrict vec = map->Table+hash;
	for( size_t i=0 ; i<Vector_Count(vec) ; i++ ) {
		struct KeyNode *restrict kv = vec->Table[i].Ptr;
		if( !String_CmpCStr(&kv->KeyName, strkey) )
			return kv;
	}
	return NULL;
}

struct Vector *Map_GetKeyTable(const struct Hashmap *const map)
{
	return map ? map->Table : NULL;
}

void Map_FromUniLinkedList(struct Hashmap *const map, const struct UniLinkedList *const list)
{
	if( !map || !list )
		return;
	
	size_t i=0;
	for( struct UniListNode *n=list->Head ; n ; n = n->Next ) {
		char cstrkey[10] = {0};
		sprintf(cstrkey, "%zu", i);
		Map_Insert(map, cstrkey, n->Data);
		i++;
	}
}

void Map_FromBiLinkedList(struct Hashmap *const map, const struct BiLinkedList *const list)
{
	if( !map || !list )
		return;
	
	size_t i=0;
	for( struct BiListNode *n=list->Head ; n ; n = n->Next ) {
		char cstrkey[10] = {0};
		sprintf(cstrkey, "%zu", i);
		Map_Insert(map, cstrkey, n->Data);
		i++;
	}
}

void Map_FromVector(struct Hashmap *const map, const struct Vector *const v)
{
	if( !map || !v || !v->Table )
		return;
	
	for( size_t i=0 ; i<v->Count ; i++ ) {
		char cstrkey[10] = {0};
		sprintf(cstrkey, "%zu", i);
		Map_Insert(map, cstrkey, v->Table[i]);
	}
}

void Map_FromTuple(struct Hashmap *const map, const struct Tuple *const tup)
{
	if( !map || !tup || !tup->Items || !tup->Len )
		return;
	
	for( size_t i=0 ; i<tup->Len ; i++ ) {
		char cstrkey[10] = {0};
		sprintf(cstrkey, "%zu", i);
		Map_Insert(map, cstrkey, tup->Items[i]);
	}
}

void Map_FromGraph(struct Hashmap *const map, const struct Graph *const graph)
{
	if( !map || !graph )
		return;
	
	for( size_t i=0 ; i<graph->Vertices.Count ; i++ ) {
		char cstrkey[10] = {0};
		sprintf(cstrkey, "%zu", i);
		struct GraphVertex *vert = graph->Vertices.Table[i].Ptr;
		Map_Insert(map, cstrkey, vert->Data);
	}
}

void Map_FromLinkMap(struct Hashmap *const map, const struct LinkMap *const linkmap)
{
	if( !map || !linkmap )
		return;
	
	for( size_t i=0 ; i<linkmap->Order.Count ; i++ ) {
		struct KeyNode *n = linkmap->Order.Table[i].Ptr;
		Map_InsertNode(map, KeyNode_NewSP(n->KeyName.CStr, n->Data));
	}
}


struct Hashmap *Map_NewFromUniLinkedList(const struct UniLinkedList *const list)
{
	if( !list )
		return NULL;
	
	struct Hashmap *map = Map_New();
	Map_FromUniLinkedList(map, list);
	return map;
}

struct Hashmap *Map_NewFromBiLinkedList(const struct BiLinkedList *const list)
{
	if( !list )
		return NULL;
	
	struct Hashmap *map = Map_New();
	Map_FromBiLinkedList(map, list);
	return map;
}

struct Hashmap *Map_NewFromVector(const struct Vector *const v)
{
	if( !v )
		return NULL;
	
	struct Hashmap *map = Map_New();
	Map_FromVector(map, v);
	return map;
}

struct Hashmap *Map_NewFromTuple(const struct Tuple *const tup)
{
	if( !tup || !tup->Items || !tup->Len )
		return NULL;
	
	struct Hashmap *map = Map_New();
	Map_FromTuple(map, tup);
	return map;
}

struct Hashmap *Map_NewFromGraph(const struct Graph *const graph)
{
	if( !graph )
		return NULL;
	
	struct Hashmap *map = Map_New();
	Map_FromGraph(map, graph);
	return map;
}

struct Hashmap *Map_NewFromLinkMap(const struct LinkMap *const linkmap)
{
	if( !linkmap )
		return NULL;
	
	struct Hashmap *map = Map_New();
	Map_FromLinkMap(map, linkmap);
	return map;
}
