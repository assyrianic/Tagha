#include <stdlib.h>
#include <stdio.h>
#ifdef OS_WINDOWS
	#define HARBOL_LIB
#endif
#include "harbol.h"


HARBOL_EXPORT struct HarbolKeyValPair *harbol_kvpair_new(void)
{
	return calloc(1, sizeof(struct HarbolKeyValPair));
}

HARBOL_EXPORT struct HarbolKeyValPair *harbol_kvpair_new_strval(const char cstr[restrict], const union HarbolValue val)
{
	struct HarbolKeyValPair *restrict n = harbol_kvpair_new();
	if( n ) {
		harbol_string_init_cstr(&n->KeyName, cstr);
		n->Data = val;
	}
	return n;
}

HARBOL_EXPORT void harbol_kvpair_del(struct HarbolKeyValPair *const n, fnDestructor *const dtor)
{
	if( !n )
		return;
	
	harbol_string_del(&n->KeyName);
	if( dtor )
		(*dtor)(&n->Data.Ptr);
}

HARBOL_EXPORT void harbol_kvpair_free(struct HarbolKeyValPair **noderef, fnDestructor *const dtor)
{
	if( !noderef || !*noderef )
		return;
	
	harbol_kvpair_del(*noderef, dtor);
	free(*noderef), *noderef=NULL;
}

// size_t general hash function.
HARBOL_EXPORT size_t GenHash(const char cstr[restrict])
{
	if( !cstr )
		return SIZE_MAX;
	
	size_t h = 0;
	while( *cstr )
		h = 37 * h + *cstr++;
	return h;
}

uint32_t Int32Hash(uint32_t a)
{
	a = (a+0x7ed55d16) + (a<<12);
	a = (a^0xc761c23c) ^ (a>>19);
	a = (a+0x165667b1) + (a<<5);
	a = (a+0xd3a2646c) ^ (a<<9);
	a = (a+0xfd7046c5) + (a<<3);
	a = (a^0xb55a4f09) ^ (a>>16);
	return a;
}

uint64_t Int64Hash(uint64_t a)
{
	a = (~a) + (a << 21);
	a = a ^ (a >> 24);
	a = (a + (a << 3)) + (a << 8);
	a = a ^ (a >> 14);
	a = (a + (a << 2)) + (a << 4);
	a = a ^ (a >> 28);
	a = a + (a << 31);
	return a;
}

size_t GenIntHash(size_t a)
{
	return sizeof(size_t)==4 ? Int32Hash(a) : sizeof(size_t)==8 ? Int64Hash(a) : 0;
}

size_t PtrHash(const void *const p)
{
	size_t y = (size_t)p;
	return (y >> 4u) | (y << (8u * sizeof(void *) - 4u));
}

size_t FloatHash(const float fval)
{
	union {
		float f;
		size_t s;
	} conv = {fval};
	return GenIntHash(conv.s);
}

uint64_t DoubleHash(const double dbl)
{
	union {
		double d;
		uint64_t l;
	} conv = {dbl};
	return Int64Hash(conv.l);
}


HARBOL_EXPORT struct HarbolHashmap *harbol_hashmap_new(void)
{
	struct HarbolHashmap *map = calloc(1, sizeof *map);
	return map;
}

HARBOL_EXPORT void harbol_hashmap_init(struct HarbolHashmap *const map)
{
	if( !map )
		return;
	
	memset(map, 0, sizeof *map);
}

HARBOL_EXPORT void harbol_hashmap_del(struct HarbolHashmap *const map, fnDestructor *const dtor)
{
	if( !map || !map->Table )
		return;
	
	for( size_t i=0 ; i<map->Len ; i++ ) {
		const union HarbolValue *const end = harbol_vector_get_iter_end_len(map->Table+i);
		for( union HarbolValue *iter=harbol_vector_get_iter(map->Table+i) ; iter && iter != end ; iter++ ) {
			struct HarbolKeyValPair *kv = iter->Ptr;
			harbol_kvpair_free(&kv, dtor);
		}
		harbol_vector_del(map->Table+i, NULL);
	}
	free(map->Table), map->Table=NULL;
	memset(map, 0, sizeof *map);
}

HARBOL_EXPORT void harbol_hashmap_free(struct HarbolHashmap **mapref, fnDestructor *const dtor)
{
	if( !*mapref )
		return;
	
	harbol_hashmap_del(*mapref, dtor);
	free(*mapref), *mapref=NULL;
}

HARBOL_EXPORT size_t harbol_hashmap_get_count(const struct HarbolHashmap *const map)
{
	return map ? map->Count : 0;
}

HARBOL_EXPORT size_t harbol_hashmap_get_len(const struct HarbolHashmap *const map)
{
	return map ? map->Len : 0;
}

HARBOL_EXPORT bool harbol_hashmap_rehash(struct HarbolHashmap *const map)
{
	if( !map || !map->Table )
		return false;
	
	const size_t old_size = map->Len;
	map->Len <<= 1;
	map->Count = 0;
	
	struct HarbolVector
		*curr = NULL,
		*temp = calloc(map->Len, sizeof *temp)
	;
	if( !temp ) {
		//puts("**** Memory Allocation Error **** harbol_hashmap_rehash::temp is NULL\n");
		map->Len = 0;
		return false;
	}
	
	curr = map->Table;
	map->Table = temp;
	
	for( size_t i=0 ; i<old_size ; i++ ) {
		const union HarbolValue *const end = harbol_vector_get_iter_end_count(curr+i);
		for( union HarbolValue *iter=harbol_vector_get_iter(curr+i) ; iter && iter != end ; iter++ ) {
			struct HarbolKeyValPair *node = iter->Ptr;
			harbol_hashmap_insert_node(map, node);
		}
		harbol_vector_del(curr+i, NULL);
	}
	free(curr), curr=NULL;
	return true;
}

HARBOL_EXPORT bool harbol_hashmap_insert_node(struct HarbolHashmap *const map, struct HarbolKeyValPair *node)
{
	if( !map || !node || !node->KeyName.CStr )
		return false;
	
	else if( !map->Len ) {
		map->Len = 8;
		map->Table = calloc(map->Len, sizeof *map->Table);
		if( !map->Table ) {
			//puts("**** Memory Allocation Error **** harbol_hashmap_insert_node::map->Table is NULL\n");
			map->Len = 0;
			return false;
		}
	}
	else if( map->Count >= map->Len )
		harbol_hashmap_rehash(map);
	else if( harbol_hashmap_has_key(map, node->KeyName.CStr) ) {
		//puts("harbol_hashmap_insert_node::map already has entry!\n");
		return false;
	}
	
	const size_t hash = GenHash(node->KeyName.CStr) % map->Len;
	harbol_vector_insert(map->Table + hash, (union HarbolValue){.Ptr=node});
	++map->Count;
	return true;
}

HARBOL_EXPORT bool harbol_hashmap_insert(struct HarbolHashmap *const restrict map, const char strkey[restrict], const union HarbolValue val)
{
	if( !map || !strkey )
		return false;
	
	struct HarbolKeyValPair *node = harbol_kvpair_new_strval(strkey, val);
	bool b = harbol_hashmap_insert_node(map, node);
	if( !b )
		harbol_kvpair_free(&node, NULL);
	return b;
}

HARBOL_EXPORT union HarbolValue harbol_hashmap_get(const struct HarbolHashmap *const restrict map, const char strkey[restrict])
{
	if( !map || !map->Table || !harbol_hashmap_has_key(map, strkey) )
		return (union HarbolValue){0};
	
	const size_t hash = GenHash(strkey) % map->Len;
	const union HarbolValue *const end = harbol_vector_get_iter_end_count(map->Table+hash);
	for( union HarbolValue *iter=harbol_vector_get_iter(map->Table+hash) ; iter && iter != end ; iter++ ) {
		const struct HarbolKeyValPair *const restrict kv = iter->Ptr;
		if( !harbol_string_cmpcstr(&kv->KeyName, strkey) )
			return kv->Data;
	}
	return (union HarbolValue){0};
}

HARBOL_EXPORT void harbol_hashmap_set(struct HarbolHashmap *const restrict map, const char strkey[restrict], const union HarbolValue val)
{
	if( !map || !harbol_hashmap_has_key(map, strkey) )
		return;
	
	const size_t hash = GenHash(strkey) % map->Len;
	const union HarbolValue *const end = harbol_vector_get_iter_end_count(map->Table+hash);
	for( union HarbolValue *iter=harbol_vector_get_iter(map->Table+hash) ; iter && iter != end ; iter++ ) {
		struct HarbolKeyValPair *const restrict kv = iter->Ptr;
		if( !harbol_string_cmpcstr(&kv->KeyName, strkey) )
			kv->Data = val;
	}
}

HARBOL_EXPORT void harbol_hashmap_delete(struct HarbolHashmap *const restrict map, const char strkey[restrict], fnDestructor *const dtor)
{
	if( !map || !map->Table || !harbol_hashmap_has_key(map, strkey) )
		return;
	
	const size_t hash = GenHash(strkey) % map->Len;
	struct HarbolVector *restrict vec = map->Table+hash;
	for( size_t i=0 ; i<vec->Count ; i++ ) {
		struct HarbolKeyValPair *kv = vec->Table[i].Ptr;
		if( !harbol_string_cmpcstr(&kv->KeyName, strkey) ) {
			harbol_kvpair_del(kv, dtor);
			harbol_vector_delete(vec, i, NULL);
			map->Count--;
			break;
		}
	}
}

HARBOL_EXPORT bool harbol_hashmap_has_key(const struct HarbolHashmap *const restrict map, const char strkey[restrict])
{
	if( !map || !map->Table )
		return false;
	
	const size_t hash = GenHash(strkey) % map->Len;
	const union HarbolValue *const end = harbol_vector_get_iter_end_count(map->Table+hash);
	for( union HarbolValue *iter=harbol_vector_get_iter(map->Table+hash) ; iter && iter != end ; iter++ ) {
		const struct HarbolKeyValPair *const restrict kv = iter->Ptr;
		if( !harbol_string_cmpcstr(&kv->KeyName, strkey) )
			return true;
	}
	return false;
}

HARBOL_EXPORT struct HarbolKeyValPair *harbol_hashmap_get_node(const struct HarbolHashmap *const restrict map, const char strkey[restrict])
{
	if( !map || !strkey || !map->Table )
		return NULL;
	
	const size_t hash = GenHash(strkey) % map->Len;
	const union HarbolValue *const end = harbol_vector_get_iter_end_count(map->Table+hash);
	for( union HarbolValue *iter=harbol_vector_get_iter(map->Table+hash) ; iter && iter != end ; iter++ ) {
		struct HarbolKeyValPair *restrict kv = iter->Ptr;
		if( !harbol_string_cmpcstr(&kv->KeyName, strkey) )
			return kv;
	}
	return NULL;
}

HARBOL_EXPORT struct HarbolVector *harbol_hashmap_get_buckets(const struct HarbolHashmap *const map)
{
	return map ? map->Table : NULL;
}

HARBOL_EXPORT void harbol_hashmap_from_unilist(struct HarbolHashmap *const map, const struct HarbolUniList *const list)
{
	if( !map || !list )
		return;
	
	size_t i=0;
	for( struct HarbolUniListNode *n=list->Head ; n ; n = n->Next ) {
		char cstrkey[21] = {0};
		sprintf(cstrkey, "%zu", i);
		harbol_hashmap_insert(map, cstrkey, n->Data);
		i++;
	}
}

HARBOL_EXPORT void harbol_hashmap_from_bilist(struct HarbolHashmap *const map, const struct HarbolBiList *const list)
{
	if( !map || !list )
		return;
	
	size_t i=0;
	for( struct HarbolBiListNode *n=list->Head ; n ; n = n->Next ) {
		char cstrkey[21] = {0};
		sprintf(cstrkey, "%zu", i);
		harbol_hashmap_insert(map, cstrkey, n->Data);
		i++;
	}
}

HARBOL_EXPORT void harbol_hashmap_from_vector(struct HarbolHashmap *const map, const struct HarbolVector *const v)
{
	if( !map || !v || !v->Table )
		return;
	
	for( size_t i=0 ; i<v->Count ; i++ ) {
		char cstrkey[21] = {0};
		sprintf(cstrkey, "%zu", i);
		harbol_hashmap_insert(map, cstrkey, v->Table[i]);
	}
}

HARBOL_EXPORT void harbol_hashmap_from_graph(struct HarbolHashmap *const map, const struct HarbolGraph *const graph)
{
	if( !map || !graph )
		return;
	
	for( size_t i=0 ; i<graph->Vertices.Count ; i++ ) {
		char cstrkey[21] = {0};
		sprintf(cstrkey, "%zu", i);
		struct HarbolGraphVertex *vert = graph->Vertices.Table[i].Ptr;
		harbol_hashmap_insert(map, cstrkey, vert->Data);
	}
}

HARBOL_EXPORT void harbol_hashmap_from_linkmap(struct HarbolHashmap *const map, const struct HarbolLinkMap *const linkmap)
{
	if( !map || !linkmap )
		return;
	
	for( size_t i=0 ; i<linkmap->Order.Count ; i++ ) {
		struct HarbolKeyValPair *n = linkmap->Order.Table[i].Ptr;
		harbol_hashmap_insert_node(map, harbol_kvpair_new_strval(n->KeyName.CStr, n->Data));
	}
}

HARBOL_EXPORT struct HarbolHashmap *harbol_hashmap_new_from_unilist(const struct HarbolUniList *const list)
{
	if( !list )
		return NULL;
	
	struct HarbolHashmap *map = harbol_hashmap_new();
	harbol_hashmap_from_unilist(map, list);
	return map;
}

HARBOL_EXPORT struct HarbolHashmap *harbol_hashmap_new_from_bilist(const struct HarbolBiList *const list)
{
	if( !list )
		return NULL;
	
	struct HarbolHashmap *map = harbol_hashmap_new();
	harbol_hashmap_from_bilist(map, list);
	return map;
}

HARBOL_EXPORT struct HarbolHashmap *harbol_hashmap_new_from_vector(const struct HarbolVector *const v)
{
	if( !v )
		return NULL;
	
	struct HarbolHashmap *map = harbol_hashmap_new();
	harbol_hashmap_from_vector(map, v);
	return map;
}

HARBOL_EXPORT struct HarbolHashmap *harbol_hashmap_new_from_graph(const struct HarbolGraph *const graph)
{
	if( !graph )
		return NULL;
	
	struct HarbolHashmap *map = harbol_hashmap_new();
	harbol_hashmap_from_graph(map, graph);
	return map;
}

HARBOL_EXPORT struct HarbolHashmap *harbol_hashmap_new_from_linkmap(const struct HarbolLinkMap *const linkmap)
{
	if( !linkmap )
		return NULL;
	
	struct HarbolHashmap *map = harbol_hashmap_new();
	harbol_hashmap_from_linkmap(map, linkmap);
	return map;
}
