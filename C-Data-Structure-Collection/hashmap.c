
#include <stdlib.h>
#include <stdio.h>
#include "dsc.h"


struct KeyNode *KeyNode_New()
{
	return calloc(1, sizeof(struct KeyNode));
}

struct KeyNode *KeyNode_NewSP(const char *restrict cstr, const union Value val)
{
	struct KeyNode *n = KeyNode_New();
	if( n ) {
		String_InitStr(&n->KeyName, cstr);
		n->Data = val;
	}
	return n;
}

void KeyNode_Del(struct KeyNode *const restrict n, bool (*dtor)())
{
	if( !n )
		return;
	
	String_Del(&n->KeyName);
	if( dtor )
		(*dtor)(&n->Data.Ptr);
	if( n->Next )
		KeyNode_Free(&n->Next, dtor);
}

bool KeyNode_Free(struct KeyNode **restrict noderef, bool (*dtor)())
{
	if( !*noderef )
		return false;
	
	KeyNode_Del(*noderef, dtor);
	free(*noderef);
	*noderef=NULL;
	return true;
}


// size_t general hash function.
static size_t GenHash(const char *cstr)
{
	size_t h = 0;
	if( !cstr )
		return h;
	
	for( const char *restrict us = cstr ; *us ; us++ )
		h = 37 * h + *us;
	return h;
}

struct Hashmap *Map_New(bool (*dtor)())
{
	struct Hashmap *map = calloc(1, sizeof *map);
	Map_SetItemDestructor(map, dtor);
	return map;
}

void Map_Init(struct Hashmap *const restrict map, bool (*dtor)())
{
	if( !map )
		return;
	
	*map = (struct Hashmap){0};
	Map_SetItemDestructor(map, dtor);
}

void Map_Del(struct Hashmap *const restrict map)
{
	if( !map or !map->Table )
		return;
	
	for( size_t i=0 ; i<map->Len ; i++ )
		KeyNode_Free(map->Table+i, map->Destructor);
	
	if( map->Table )
		free(map->Table);
	Map_Init(map, map->Destructor);
}

void Map_Free(struct Hashmap **restrict mapref)
{
	if( !*mapref )
		return;
	
	Map_Del(*mapref);
	free(*mapref);
	*mapref=NULL;
}

inline size_t Map_Count(const struct Hashmap *const restrict map)
{
	return map ? map->Count : 0;
}

inline size_t Map_Len(const struct Hashmap *const restrict map)
{
	return map ? map->Len : 0;
}

bool Map_Rehash(struct Hashmap *const restrict map)
{
	if( !map or !map->Table )
		return false;
	
	size_t old_size = map->Len;
	map->Len <<= 1;
	map->Count = 0;
	
	struct KeyNode **curr, **temp;
	temp = calloc(map->Len, sizeof *temp);
	if( !temp ) {
		puts("**** Memory Allocation Error **** Map_Rehash::temp is NULL\n");
		map->Len = 0;
		return false;
	}
	
	curr = map->Table;
	map->Table = temp;
	
	for( size_t i=0 ; i<old_size ; i++ ) {
		if( !curr[i] )
			continue;
		for( struct KeyNode *kv=curr[i], *next=NULL ; kv ; kv=next ) {
			next = kv->Next;
			Map_InsertNode(map, kv);
		}
	}
	free(curr);
	curr=NULL;
	return true;
}

bool Map_InsertNode(struct Hashmap *const restrict map, struct KeyNode *restrict node)
{
	if( !map or !node )
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
	node->Next = map->Table[hash];
	map->Table[hash] = node;
	++map->Count;
	return true;
}


bool Map_Insert(struct Hashmap *const restrict map, const char *restrict strkey, const union Value val)
{
	if( !map or !strkey )
		return false;
	
	struct KeyNode *node = KeyNode_NewSP(strkey, val);
	bool b = Map_InsertNode(map, node);
	if( !b )
		KeyNode_Free(&node, map->Destructor);
	return b;
}

union Value Map_Get(const struct Hashmap *const restrict map, const char *restrict strkey)
{
	if( !map or !map->Table or !Map_HasKey(map, strkey) )
		return (union Value){0};
	
	const size_t hash = GenHash(strkey) % map->Len;
	for( struct KeyNode *restrict kv=map->Table[hash] ; kv ; kv=kv->Next )
		if( !String_CmpCStr(&kv->KeyName, strkey) )
			return kv->Data;
	
	return (union Value){0};
}

void Map_Set(struct Hashmap *const restrict map, const char *restrict strkey, const union Value val)
{
	if( !map or !Map_HasKey(map, strkey) )
		return;
	
	const size_t hash = GenHash(strkey) % map->Len;
	for( struct KeyNode *restrict kv=map->Table[hash] ; kv ; kv=kv->Next )
		if( !String_CmpCStr(&kv->KeyName, strkey) )
			kv->Data = val;
}

void Map_SetItemDestructor(struct Hashmap *const restrict map, bool (*destructor)())
{
	if( !map )
		return;
	
	map->Destructor = destructor;
}

void Map_Delete(struct Hashmap *const restrict map, const char *restrict strkey)
{
	if( !map or !map->Table or !Map_HasKey(map, strkey) )
		return;
	
	const size_t hash = GenHash(strkey) % map->Len;
	for( struct KeyNode *kv=map->Table[hash], *next=NULL ; kv ; kv=next ) {
		next = kv->Next;
		
		if( !String_CmpCStr(&kv->KeyName, strkey) ) {
			map->Table[hash] = kv->Next;
			kv->Next = NULL;
			
			if( map->Destructor )
				(*map->Destructor)(&kv->Data.Ptr);
			KeyNode_Free(&kv, map->Destructor);
			map->Count--;
		}
	}
}

bool Map_HasKey(const struct Hashmap *const restrict map, const char *restrict strkey)
{
	if( !map or !map->Table )
		return false;
	
	const size_t hash = GenHash(strkey) % map->Len;
	for( struct KeyNode *restrict n = map->Table[hash] ; n ; n=n->Next )
		if( !String_CmpCStr(&n->KeyName, strkey) )
			return true;
	
	return false;
}

struct KeyNode *Map_GetKeyNode(const struct Hashmap *const restrict map, const char *restrict strkey)
{
	if( !map or !strkey or !map->Table )
		return NULL;
	
	const size_t hash = GenHash(strkey) % map->Len;
	for( struct KeyNode *restrict n = map->Table[hash] ; n ; n=n->Next )
		if( !String_CmpCStr(&n->KeyName, strkey) )
			return n;
	
	return NULL;
}

struct KeyNode **Map_GetKeyTable(const struct Hashmap *const restrict map)
{
	return map ? map->Table : NULL;
}

void Map_FromUniLinkedList(struct Hashmap *const restrict map, const struct UniLinkedList *const restrict list)
{
	if( !map or !list )
		return;
	
	size_t i=0;
	for( struct UniListNode *n=list->Head ; n ; n = n->Next ) {
		char cstrkey[10] = {0};
		sprintf(cstrkey, "%zu", i);
		Map_Insert(map, cstrkey, n->Data);
		i++;
	}
}

void Map_FromBiLinkedList(struct Hashmap *const restrict map, const struct BiLinkedList *const restrict list)
{
	if( !map or !list )
		return;
	
	size_t i=0;
	for( struct BiListNode *n=list->Head ; n ; n = n->Next ) {
		char cstrkey[10] = {0};
		sprintf(cstrkey, "%zu", i);
		Map_Insert(map, cstrkey, n->Data);
		i++;
	}
}

void Map_FromVector(struct Hashmap *const restrict map, const struct Vector *const restrict v)
{
	if( !map or !v )
		return;
	
	for( size_t i=0 ; i<v->Count ; i++ ) {
		char cstrkey[10] = {0};
		sprintf(cstrkey, "%zu", i);
		Map_Insert(map, cstrkey, v->Table[i]);
	}
}

void Map_FromTuple(struct Hashmap *const restrict map, const struct Tuple *const restrict tup)
{
	if( !map or !tup or !tup->Items or !tup->Len )
		return;
	
	for( size_t i=0 ; i<tup->Len ; i++ ) {
		char cstrkey[10] = {0};
		sprintf(cstrkey, "%zu", i);
		Map_Insert(map, cstrkey, tup->Items[i]);
	}
}

void Map_FromGraph(struct Hashmap *const restrict map, const struct Graph *const restrict graph)
{
	if( !map or !graph )
		return;
	
	for( size_t i=0 ; i<graph->VertexCount ; i++ ) {
		char cstrkey[10] = {0};
		sprintf(cstrkey, "%zu", i);
		Map_Insert(map, cstrkey, graph->Vertices[i].Data);
	}
}

void Map_FromLinkMap(struct Hashmap *const restrict map, const struct LinkMap *const restrict linkmap)
{
	if( !map or !linkmap )
		return;
	
	for( struct LinkNode *l=linkmap->Head ; l ; l=l->After )
		Map_InsertNode(map, KeyNode_NewSP(l->KeyName.CStr, l->Data));
}


struct Hashmap *Map_NewFromUniLinkedList(const struct UniLinkedList *const restrict list)
{
	if( !list )
		return NULL;
	
	struct Hashmap *map = Map_New(list->Destructor);
	Map_FromUniLinkedList(map, list);
	return map;
}

struct Hashmap *Map_NewFromBiLinkedList(const struct BiLinkedList *const restrict list)
{
	if( !list )
		return NULL;
	
	struct Hashmap *map = Map_New(list->Destructor);
	Map_FromBiLinkedList(map, list);
	return map;
}

struct Hashmap *Map_NewFromVector(const struct Vector *const restrict v)
{
	if( !v )
		return NULL;
	
	struct Hashmap *map = Map_New(v->Destructor);
	Map_FromVector(map, v);
	return map;
}

struct Hashmap *Map_NewFromTuple(const struct Tuple *const restrict tup)
{
	if( !tup or !tup->Items or !tup->Len )
		return NULL;
	
	struct Hashmap *map = Map_New(NULL);
	Map_FromTuple(map, tup);
	return map;
}

struct Hashmap *Map_NewFromGraph(const struct Graph *const restrict graph)
{
	if( !graph )
		return NULL;
	
	struct Hashmap *map = Map_New(graph->VertexDestructor);
	Map_FromGraph(map, graph);
	return map;
}

struct Hashmap *Map_NewFromLinkMap(const struct LinkMap *const restrict linkmap)
{
	if( !linkmap )
		return NULL;
	
	struct Hashmap *map = Map_New(linkmap->Destructor);
	Map_FromLinkMap(map, linkmap);
	return map;
}
