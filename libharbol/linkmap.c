#include <stdlib.h>
#include <stdio.h>
#ifdef OS_WINDOWS
	#define HARBOL_LIB
#endif
#include "harbol.h"


HARBOL_EXPORT struct HarbolLinkMap *harbol_linkmap_new(void)
{
	struct HarbolLinkMap *map = calloc(1, sizeof *map);
	return map;
}

HARBOL_EXPORT void harbol_linkmap_init(struct HarbolLinkMap *const map)
{
	if( !map )
		return;
	memset(map, 0, sizeof *map);
}

HARBOL_EXPORT void harbol_linkmap_del(struct HarbolLinkMap *const map, fnDestructor *const dtor)
{
	if( !map || !map->Table )
		return;
	
	for( size_t i=0 ; i<map->Len ; i++ ) {
		struct HarbolVector *vec = map->Table+i;
		for( size_t i=0 ; i<vec->Len ; i++ ) {
			struct HarbolKeyValPair *kv = vec->Table[i].Ptr;
			harbol_kvpair_free(&kv, dtor);
		}
		harbol_vector_del(vec, NULL);
	}
	free(map->Table), map->Table=NULL;
	harbol_vector_del(&map->Order, NULL);
	memset(map, 0, sizeof *map);
}

HARBOL_EXPORT void harbol_linkmap_free(struct HarbolLinkMap **linkmapref, fnDestructor *const dtor)
{
	if( !*linkmapref )
		return;
	
	harbol_linkmap_del(*linkmapref, dtor);
	free(*linkmapref); *linkmapref=NULL;
}

HARBOL_EXPORT size_t harbol_linkmap_get_count(const struct HarbolLinkMap *const map)
{
	return map ? map->Count : 0;
}

HARBOL_EXPORT size_t harbol_linkmap_get_len(const struct HarbolLinkMap *const map)
{
	return map ? map->Len : 0;
}

HARBOL_EXPORT bool harbol_linkmap_rehash(struct HarbolLinkMap *const map)
{
	return ( !map || !map->Table ) ? false : harbol_hashmap_rehash(&map->Map);
}

HARBOL_EXPORT bool harbol_linkmap_insert_node(struct HarbolLinkMap *const map, struct HarbolKeyValPair *node)
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
		harbol_linkmap_rehash(map);
	else if( harbol_linkmap_has_key(map, node->KeyName.CStr) )
		return false;
	
	const size_t hash = GenHash(node->KeyName.CStr) % map->Len;
	harbol_vector_insert(map->Table + hash, (union HarbolValue){.Ptr=node});
	harbol_vector_insert(&map->Order, (union HarbolValue){.Ptr=node});
	++map->Count;
	return true;
}

HARBOL_EXPORT bool harbol_linkmap_insert(struct HarbolLinkMap *const restrict map, const char strkey[restrict], const union HarbolValue val)
{
	if( !map || !strkey )
		return false;
	
	struct HarbolKeyValPair *node = harbol_kvpair_new_strval(strkey, val);
	bool b = harbol_linkmap_insert_node(map, node);
	if( !b )
		harbol_kvpair_free(&node, NULL);
	return b;
}

HARBOL_EXPORT struct HarbolKeyValPair *harbol_linkmap_get_node_by_index(const struct HarbolLinkMap *const map, const size_t index)
{
	return ( !map || !map->Table || !map->Order.Table ) ? NULL : map->Order.Table[index].Ptr;
}

HARBOL_EXPORT union HarbolValue harbol_linkmap_get(const struct HarbolLinkMap *const restrict map, const char strkey[restrict])
{
	return ( !map || !harbol_hashmap_has_key(&map->Map, strkey) ) ? (union HarbolValue){0} : harbol_hashmap_get(&map->Map, strkey);
}

HARBOL_EXPORT void harbol_linkmap_set(struct HarbolLinkMap *const restrict map, const char strkey[restrict], const union HarbolValue val)
{
	if( !map || !harbol_linkmap_has_key(map, strkey) )
		return;
	
	harbol_hashmap_set(&map->Map, strkey, val);
}

HARBOL_EXPORT union HarbolValue harbol_linkmap_get_by_index(const struct HarbolLinkMap *const map, const size_t index)
{
	if( !map || !map->Table )
		return (union HarbolValue){0};
	
	struct HarbolKeyValPair *node = harbol_linkmap_get_node_by_index(map, index);
	return ( node ) ? node->Data : (union HarbolValue){0};
}

HARBOL_EXPORT void harbol_linkmap_set_by_index(struct HarbolLinkMap *const map, const size_t index, const union HarbolValue val)
{
	if( !map || !map->Table )
		return;
	
	struct HarbolKeyValPair *node = harbol_linkmap_get_node_by_index(map, index);
	if( node )
		node->Data = val;
}

HARBOL_EXPORT void harbol_linkmap_delete(struct HarbolLinkMap *const restrict map, const char strkey[restrict], fnDestructor *const dtor)
{
	if( !map || !map->Table || !harbol_linkmap_has_key(map, strkey) )
		return;
	
	const size_t index = harbol_linkmap_get_index_by_name(map, strkey);
	harbol_hashmap_delete(&map->Map, strkey, dtor);
	harbol_vector_delete(&map->Order, index, NULL);
}

HARBOL_EXPORT void harbol_linkmap_delete_by_index(struct HarbolLinkMap *const map, const size_t index, fnDestructor *const dtor)
{
	if( !map || !map->Table )
		return;
	
	struct HarbolKeyValPair *kv = harbol_linkmap_get_node_by_index(map, index);
	if( !kv )
		return;
	
	harbol_hashmap_delete(&map->Map, kv->KeyName.CStr, dtor);
	harbol_vector_delete(&map->Order, index, NULL);
}

HARBOL_EXPORT bool harbol_linkmap_has_key(const struct HarbolLinkMap *const restrict map, const char strkey[restrict])
{
	return !map || !map->Table ? false : harbol_hashmap_has_key(&map->Map, strkey);
}

HARBOL_EXPORT struct HarbolKeyValPair *harbol_linkmap_get_node_by_key(const struct HarbolLinkMap *const restrict map, const char strkey[restrict])
{
	if( !map || !map->Table )
		return NULL;
	
	return harbol_hashmap_get_node(&map->Map, strkey);
}

HARBOL_EXPORT struct HarbolVector *harbol_linkmap_get_buckets(const struct HarbolLinkMap *const map)
{
	return map ? map->Table : NULL;
}

HARBOL_EXPORT union HarbolValue *harbol_linkmap_get_iter(const struct HarbolLinkMap *const map)
{
	return map ? map->Order.Table : NULL;
}

HARBOL_EXPORT union HarbolValue *harbol_linkmap_get_iter_end_len(const struct HarbolLinkMap *const map)
{
	return map ? map->Order.Table + map->Order.Len : NULL;
}

HARBOL_EXPORT union HarbolValue *harbol_linkmap_get_iter_end_count(const struct HarbolLinkMap *const map)
{
	return map ? map->Order.Table + map->Order.Count : NULL;
}

HARBOL_EXPORT size_t harbol_linkmap_get_index_by_name(const struct HarbolLinkMap *const restrict map, const char strkey[restrict])
{
	if( !map || !strkey )
		return SIZE_MAX;
	
	for( size_t i=0 ; i<map->Order.Count ; i++ ) {
		struct HarbolKeyValPair *kv = map->Order.Table[i].Ptr;
		if( !harbol_string_cmpcstr(&kv->KeyName, strkey) )
			return i;
	}
	return SIZE_MAX;
}

HARBOL_EXPORT size_t harbol_linkmap_get_index_by_node(const struct HarbolLinkMap *const map, struct HarbolKeyValPair *const node)
{
	if( !map || !node )
		return SIZE_MAX;
	
	for( size_t i=0 ; i<map->Order.Count ; i++ ) {
		if( (uintptr_t)map->Order.Table[i].Ptr == (uintptr_t)node )
			return i;
	}
	return SIZE_MAX;
}

HARBOL_EXPORT size_t harbol_linkmap_get_index_by_val(const struct HarbolLinkMap *const map, const union HarbolValue val)
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

HARBOL_EXPORT void harbol_linkmap_from_hashmap(struct HarbolLinkMap *const linkmap, const struct HarbolHashmap *const map)
{
	if( !linkmap || !map )
		return;
	
	for( size_t i=0 ; i<map->Len ; i++ ) {
		struct HarbolVector *vec = map->Table + i;
		for( size_t n=0 ; n<harbol_vector_get_count(vec) ; n++ ) {
			struct HarbolKeyValPair *kv = vec->Table[n].Ptr;
			harbol_linkmap_insert_node(linkmap, harbol_kvpair_new_strval(kv->KeyName.CStr, kv->Data));
		}
	}
}

HARBOL_EXPORT void harbol_linkmap_from_unilist(struct HarbolLinkMap *const map, const struct HarbolUniList *const list)
{
	if( !map || !list )
		return;
	
	size_t i=0;
	for( struct HarbolUniListNode *n=list->Head ; n ; n = n->Next ) {
		char cstrkey[21] = {0};
		sprintf(cstrkey, "%zu", i);
		harbol_linkmap_insert(map, cstrkey, n->Data);
		i++;
	}
}

HARBOL_EXPORT void harbol_linkmap_from_bilist(struct HarbolLinkMap *const map, const struct HarbolBiList *const list)
{
	if( !map || !list )
		return;
	
	size_t i=0;
	for( struct HarbolBiListNode *n=list->Head ; n ; n = n->Next ) {
		char cstrkey[21] = {0};
		sprintf(cstrkey, "%zu", i);
		harbol_linkmap_insert(map, cstrkey, n->Data);
		i++;
	}
}

HARBOL_EXPORT void harbol_linkmap_from_vector(struct HarbolLinkMap *const map, const struct HarbolVector *const v)
{
	if( !map || !v )
		return;
	
	for( size_t i=0 ; i<v->Count ; i++ ) {
		char cstrkey[21] = {0};
		sprintf(cstrkey, "%zu", i);
		harbol_linkmap_insert(map, cstrkey, v->Table[i]);
	}
}

HARBOL_EXPORT void harbol_linkmap_from_graph(struct HarbolLinkMap *const map, const struct HarbolGraph *const graph)
{
	if( !map || !graph )
		return;
	
	for( size_t i=0 ; i<graph->Vertices.Count ; i++ ) {
		char cstrkey[10] = {0};
		sprintf(cstrkey, "%zu", i);
		struct HarbolGraphVertex *vert = graph->Vertices.Table[i].Ptr;
		harbol_linkmap_insert(map, cstrkey, vert->Data);
	}
}

HARBOL_EXPORT struct HarbolLinkMap *harbol_linkmap_new_from_hashmap(const struct HarbolHashmap *const map)
{
	if( !map )
		return NULL;
	
	struct HarbolLinkMap *linkmap = harbol_linkmap_new();
	harbol_linkmap_from_hashmap(linkmap, map);
	return linkmap;
}

HARBOL_EXPORT struct HarbolLinkMap *harbol_linkmap_new_from_unilist(const struct HarbolUniList *const list)
{
	if( !list )
		return NULL;
	
	struct HarbolLinkMap *map = harbol_linkmap_new();
	harbol_linkmap_from_unilist(map, list);
	return map;
}

HARBOL_EXPORT struct HarbolLinkMap *harbol_linkmap_new_from_bilist(const struct HarbolBiList *const list)
{
	if( !list )
		return NULL;
	
	struct HarbolLinkMap *map = harbol_linkmap_new();
	harbol_linkmap_from_bilist(map, list);
	return map;
}

HARBOL_EXPORT struct HarbolLinkMap *harbol_linkmap_new_from_vector(const struct HarbolVector *const vec)
{
	if( !vec )
		return NULL;
	
	struct HarbolLinkMap *map = harbol_linkmap_new();
	harbol_linkmap_from_vector(map, vec);
	return map;
}

HARBOL_EXPORT struct HarbolLinkMap *harbol_linkmap_new_from_graph(const struct HarbolGraph *const graph)
{
	if( !graph )
		return NULL;
	
	struct HarbolLinkMap *map = harbol_linkmap_new();
	harbol_linkmap_from_graph(map, graph);
	return map;
}
