#include <stdlib.h>
#include <stdio.h>

#ifdef OS_WINDOWS
	#define HARBOL_LIB
#endif
#include "harbol.h"

/* Doubly Linked List Node code */
/////////////////////////////////////////
HARBOL_EXPORT struct HarbolBiListNode *harbol_bilist_node_new(void)
{
	return calloc(1, sizeof(struct HarbolBiListNode));
}

HARBOL_EXPORT struct HarbolBiListNode *harbol_bilist_node_new_val(const union HarbolValue val)
{
	struct HarbolBiListNode *node = calloc(1, sizeof *node);
	if( node )
		node->Data = val;
	return node;
}

HARBOL_EXPORT void harbol_bilist_node_del(struct HarbolBiListNode *const node, fnDestructor *const dtor)
{
	if( !node )
		return;
	
	// yes I know this syntax is unnecessary but it's for readability.
	if( dtor )
		(*dtor)(&node->Data.Ptr);
	
	node->Prev = NULL;
	harbol_bilist_node_free(&node->Next, dtor);
}

HARBOL_EXPORT void harbol_bilist_node_free(struct HarbolBiListNode ** noderef, fnDestructor *const dtor)
{
	if( !*noderef )
		return;
	
	harbol_bilist_node_del(*noderef, dtor);
	free(*noderef), *noderef=NULL;
}

HARBOL_EXPORT struct HarbolBiListNode *harbol_bilist_node_get_next_node(const struct HarbolBiListNode *const node)
{
	return node ? node->Next : NULL;
}

HARBOL_EXPORT struct HarbolBiListNode *harbol_bilist_node_get_prev_node(const struct HarbolBiListNode *const node)
{
	return node ? node->Prev : NULL;
}

HARBOL_EXPORT union HarbolValue harbol_bilist_node_get_val(const struct HarbolBiListNode *const node)
{
	return node ? node->Data : (union HarbolValue){0};
}
/////////////////////////////////////////



/* Doubly Linked List code */
/////////////////////////////////////////
HARBOL_EXPORT struct HarbolBiList *harbol_bilist_new(void)
{
	struct HarbolBiList *list = calloc(1, sizeof *list);
	return list;
}

HARBOL_EXPORT void harbol_bilist_del(struct HarbolBiList *const list, fnDestructor *const dtor)
{
	if( !list )
		return;
	
	harbol_bilist_node_free(&list->Head, dtor);
	memset(list, 0, sizeof *list);
}

HARBOL_EXPORT void harbol_bilist_free(struct HarbolBiList **listref, fnDestructor *const dtor)
{
	if( !*listref )
		return;
	
	harbol_bilist_del(*listref, dtor);
	free(*listref), *listref=NULL;
}

HARBOL_EXPORT void harbol_bilist_init(struct HarbolBiList *const list)
{
	if( !list )
		return;
	
	memset(list, 0, sizeof *list);
}

HARBOL_EXPORT size_t harbol_bilist_get_len(const struct HarbolBiList *const list)
{
	return list ? list->Len : 0;
}

HARBOL_EXPORT bool harbol_bilist_insert_node_at_head(struct HarbolBiList *const list, struct HarbolBiListNode *const node)
{
	if( !list || !node )
		return false;
	
	if( !list->Head ) {
		list->Head = list->Tail = node;
		node->Next = node->Prev = NULL;
	}
	else {
		// x(node)-> x(CurrHead)
		node->Next = list->Head;
		// NULL <-(node)-> x(CurrHead)
		node->Prev = NULL;
		// NULL <-(node)-> <-(CurrHead)
		list->Head->Prev = node;
		// NULL <-(CurrHead)-> <-(OldHead)
		list->Head = node;
	}
	list->Len++;
	return true;
}

HARBOL_EXPORT bool harbol_bilist_insert_node_at_tail(struct HarbolBiList *const list, struct HarbolBiListNode *const node)
{
	if( !list || !node )
		return false;
	
	if( list->Len ) {
		// <-(CurrTail)x <-(node)x
		node->Prev = list->Tail;
		// <-(CurrTail)x <-(node)-> NULL
		node->Next = NULL;
		// <-(CurrTail)-> <-(node)-> NULL
		list->Tail->Next = node;
		// <-(OldTail)-> <-(CurrTail)-> NULL
		list->Tail = node;
	}
	else {
		list->Head = list->Tail = node;
		node->Next = node->Prev = NULL;
	}
	list->Len++;
	return true;
}

HARBOL_EXPORT bool harbol_bilist_insert_node_at_index(struct HarbolBiList *const list, struct HarbolBiListNode *const node, const size_t index)
{
	if( !list || !node )
		return false;
	else if( !list->Head || index==0 )
		return harbol_bilist_insert_node_at_head(list, node);
	// if index is out of bounds, append at tail end.
	else if( index >= list->Len )
		return harbol_bilist_insert_node_at_tail(list, node);
	
	const bool prev_dir = ( index >= list->Len/2 );
	struct HarbolBiListNode *curr = prev_dir ? list->Tail : list->Head;
	size_t i=prev_dir ? list->Len-1 : 0;
	while( (prev_dir ? curr->Prev != NULL : curr->Next != NULL) && i != index ) {
		curr = prev_dir ? curr->Prev : curr->Next;
		prev_dir ? i-- : i++;
	}
	if( i ) {
		// P-> <-(curr)
		// P-> x(node)x
		curr->Prev->Next = node;
		// P-> <-(node)x
		node->Prev = curr->Prev;
		// P-> <-(node)-> x(curr)
		node->Next = curr;
		// P-> <-(node)-> <-(curr)
		curr->Prev = node;
		list->Len++;
		return true;
	}
	return false;
}

HARBOL_EXPORT bool harbol_bilist_insert_at_head(struct HarbolBiList *const list, const union HarbolValue val)
{
	return ( !list ) ? false : harbol_bilist_insert_node_at_head(list, harbol_bilist_node_new_val(val));
}

HARBOL_EXPORT bool harbol_bilist_insert_at_tail(struct HarbolBiList *const list, const union HarbolValue val)
{
	return ( !list ) ? false : harbol_bilist_insert_node_at_tail(list, harbol_bilist_node_new_val(val));
}

HARBOL_EXPORT bool harbol_bilist_insert_at_index(struct HarbolBiList *const list, const union HarbolValue val, const size_t index)
{
	return ( !list ) ? false : harbol_bilist_insert_node_at_index(list, harbol_bilist_node_new_val(val), index);
}

HARBOL_EXPORT struct HarbolBiListNode *harbol_bilist_get_node_by_index(const struct HarbolBiList *const list, const size_t index)
{
	if( !list )
		return NULL;
	else if( index==0 )
		return list->Head;
	else if( index >= list->Len )
		return list->Tail;
	
	const bool prev_dir = ( index >= list->Len/2 );
	struct HarbolBiListNode *node = prev_dir ? list->Tail : list->Head;
	for( size_t i=prev_dir ? list->Len-1 : 0 ; i<list->Len ; prev_dir ? i-- : i++ ) {
		if( node && i==index )
			return node;
		node = prev_dir ? node->Prev : node->Next;
	}
	return NULL;
}

HARBOL_EXPORT struct HarbolBiListNode *harbol_bilist_get_node_by_val(const struct HarbolBiList *const list, const union HarbolValue val)
{
	if( !list )
		return NULL;
	for( struct HarbolBiListNode *i=list->Head ; i ; i=i->Next )
		if( !memcmp(&i->Data, &val, sizeof val) )
			return i;
	return NULL;
}

HARBOL_EXPORT union HarbolValue harbol_bilist_get_val(const struct HarbolBiList *const list, const size_t index)
{
	if( !list )
		return (union HarbolValue){0};
	else if( index==0 && list->Head )
		return list->Head->Data;
	else if( index >= list->Len && list->Tail )
		return list->Tail->Data;
	
	const bool prev_dir = ( index >= list->Len/2 );
	struct HarbolBiListNode *node = prev_dir ? list->Tail : list->Head;
	for( size_t i=prev_dir ? list->Len-1 : 0 ; i<list->Len ; prev_dir ? i-- : i++ ) {
		if( node && i==index )
			return node->Data;
		if( (prev_dir && !node->Prev) || (!prev_dir && !node->Next) )
			break;
		node = prev_dir ? node->Prev : node->Next;
	}
	return (union HarbolValue){0};
}

HARBOL_EXPORT void harbol_bilist_set_val(struct HarbolBiList *const list, const size_t index, const union HarbolValue val)
{
	if( !list )
		return;
	else if( index==0 && list->Head ) {
		list->Head->Data = val;
		return;
	}
	else if( index >= list->Len && list->Tail ) {
		list->Tail->Data = val;
		return;
	}
	
	const bool prev_dir = ( index >= list->Len/2 );
	struct HarbolBiListNode *node = prev_dir ? list->Tail : list->Head;
	for( size_t i=prev_dir ? list->Len-1 : 0 ; i<list->Len ; prev_dir ? i-- : i++ ) {
		if( node && i==index ) {
			node->Data = val;
			break;
		}
		if( (prev_dir && !node->Prev) || (!prev_dir && !node->Next) )
			break;
		node = prev_dir ? node->Prev : node->Next;
	}
}

HARBOL_EXPORT bool harbol_bilist_del_node_by_index(struct HarbolBiList *const list, const size_t index, fnDestructor *const dtor)
{
	if( !list || !list->Len )
		return false;
	
	struct HarbolBiListNode *node = harbol_bilist_get_node_by_index(list, index);
	node->Prev ? (node->Prev->Next = node->Next) : (list->Head = node->Next);
	node->Next ? (node->Next->Prev = node->Prev) : (list->Tail = node->Prev);
	
	if( dtor )
		(*dtor)(&node->Data.Ptr);
	
	free(node), node=NULL;
	list->Len--;
	return true;
}

HARBOL_EXPORT bool harbol_bilist_del_node_by_ref(struct HarbolBiList *const list, struct HarbolBiListNode **noderef, fnDestructor *const dtor)
{
	if( !list || !*noderef )
		return false;
	
	struct HarbolBiListNode *node = *noderef;
	node->Prev ? (node->Prev->Next = node->Next) : (list->Head = node->Next);
	node->Next ? (node->Next->Prev = node->Prev) : (list->Tail = node->Prev);
	
	if( dtor )
		(*dtor)(&node->Data.Ptr);
	
	free(*noderef), *noderef=NULL;
	node = NULL;
	list->Len--;
	return true;
}

HARBOL_EXPORT struct HarbolBiListNode *harbol_bilist_get_head_node(const struct HarbolBiList *const list)
{
	return list ? list->Head : NULL;
}

HARBOL_EXPORT struct HarbolBiListNode *harbol_bilist_get_tail_node(const struct HarbolBiList *const list)
{
	return list ? list->Tail : NULL;
}

HARBOL_EXPORT void harbol_bilist_from_unilist(struct HarbolBiList *const bilist, const struct HarbolUniList *const unilist)
{
	if( !bilist || !unilist )
		return;
	
	for( struct HarbolUniListNode *n=unilist->Head ; n ; n = n->Next )
		harbol_bilist_insert_at_tail(bilist, n->Data);
}

HARBOL_EXPORT void harbol_bilist_from_hashmap(struct HarbolBiList *const bilist, const struct HarbolHashmap *const map)
{
	if( !bilist || !map )
		return;
	
	for( size_t i=0 ; i<map->Len ; i++ ) {
		struct HarbolVector *vec = map->Table + i;
		for( size_t n=0 ; n<harbol_vector_get_count(vec) ; n++ ) {
			struct HarbolKeyValPair *node = vec->Table[n].Ptr;
			harbol_bilist_insert_at_tail(bilist, node->Data);
		}
	}
}

HARBOL_EXPORT void harbol_bilist_from_vector(struct HarbolBiList *const bilist, const struct HarbolVector *const v)
{
	if( !bilist || !v )
		return;
	
	for( size_t i=0 ; i<v->Count ; i++ )
		harbol_bilist_insert_at_tail(bilist, v->Table[i]);
}

HARBOL_EXPORT void harbol_bilist_from_graph(struct HarbolBiList *const bilist, const struct HarbolGraph *const graph)
{
	if( !bilist || !graph || !graph->Vertices.Table )
		return;
	
	for( size_t i=0 ; i<graph->Vertices.Count ; i++ ) {
		struct HarbolGraphVertex *vert = graph->Vertices.Table[i].Ptr;
		harbol_bilist_insert_at_tail(bilist, vert->Data);
	}
}

HARBOL_EXPORT void harbol_bilist_from_linkmap(struct HarbolBiList *const bilist, const struct HarbolLinkMap *const map)
{
	if( !bilist || !map )
		return;
	
	for( size_t i=0 ; i<map->Order.Count ; i++ ) {
		struct HarbolKeyValPair *n = map->Order.Table[i].Ptr;
		harbol_bilist_insert_at_tail(bilist, n->Data);
	}
}

HARBOL_EXPORT struct HarbolBiList *harbol_bilist_new_from_unilist(const struct HarbolUniList *const unilist)
{
	if( !unilist )
		return NULL;
	struct HarbolBiList *bilist = harbol_bilist_new();
	harbol_bilist_from_unilist(bilist, unilist);
	return bilist;
}

HARBOL_EXPORT struct HarbolBiList *harbol_bilist_new_from_hashmap(const struct HarbolHashmap *const map)
{
	if( !map )
		return NULL;
	struct HarbolBiList *bilist = harbol_bilist_new();
	harbol_bilist_from_hashmap(bilist, map);
	return bilist;
}

HARBOL_EXPORT struct HarbolBiList *harbol_bilist_new_from_vector(const struct HarbolVector *const v)
{
	if( !v )
		return NULL;
	struct HarbolBiList *bilist = harbol_bilist_new();
	harbol_bilist_from_vector(bilist, v);
	return bilist;
}

HARBOL_EXPORT struct HarbolBiList *harbol_bilist_new_from_graph(const struct HarbolGraph *const graph)
{
	if( !graph )
		return NULL;
	struct HarbolBiList *bilist = harbol_bilist_new();
	harbol_bilist_from_graph(bilist, graph);
	return bilist;
}

HARBOL_EXPORT struct HarbolBiList *harbol_bilist_new_from_linkmap(const struct HarbolLinkMap *const map)
{
	if( !map )
		return NULL;
	struct HarbolBiList *bilist = harbol_bilist_new();
	harbol_bilist_from_linkmap(bilist, map);
	return bilist;
}
