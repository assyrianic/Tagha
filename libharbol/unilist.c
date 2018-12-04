#include <stdlib.h>
#include <stdio.h>
#ifdef OS_WINDOWS
	#define HARBOL_LIB
#endif
#include "harbol.h"

/* Singly Linked List Node code */
/////////////////////////////////////////
HARBOL_EXPORT struct HarbolUniListNode *harbol_unilistnode_new(void)
{
	return calloc(1, sizeof(struct HarbolUniListNode));
}

HARBOL_EXPORT struct HarbolUniListNode *harbol_unilistnode_new_val(const union HarbolValue val)
{
	struct HarbolUniListNode *node = harbol_unilistnode_new();
	if( node )
		node->Data = val;
	return node;
}

HARBOL_EXPORT void harbol_unilistnode_del(struct HarbolUniListNode *const node, fnDestructor *const dtor)
{
	if( !node )
		return;
	
	if( dtor )
		(*dtor)(&node->Data.Ptr);
	
	if( node->Next )
		harbol_unilistnode_free(&node->Next, dtor);
}

HARBOL_EXPORT void harbol_unilistnode_free(struct HarbolUniListNode **noderef, fnDestructor *const dtor)
{
	if( !noderef || !*noderef )
		return;
	
	harbol_unilistnode_del(*noderef, dtor);
	free(*noderef); *noderef=NULL;
}

HARBOL_EXPORT struct HarbolUniListNode *harbol_unilistnode_get_next_node(const struct HarbolUniListNode *const node)
{
	return node ? node->Next : NULL;
}

HARBOL_EXPORT union HarbolValue harbol_unilistnode_get_val(const struct HarbolUniListNode *const node)
{
	return node ? node->Data : (union HarbolValue){0};
}
/////////////////////////////////////////


/* Singly Linked List code */
/////////////////////////////////////////
HARBOL_EXPORT struct HarbolUniList *harbol_unilist_new(void)
{
	struct HarbolUniList *list = calloc(1, sizeof *list);
	return list;
}

HARBOL_EXPORT void harbol_unilist_del(struct HarbolUniList *const list, fnDestructor *const dtor)
{
	if( !list )
		return;
	
	harbol_unilistnode_free(&list->Head, dtor);
	memset(list, 0, sizeof *list);
}

HARBOL_EXPORT void harbol_unilist_free(struct HarbolUniList **listref, fnDestructor *const dtor)
{
	if( !*listref )
		return;
	
	harbol_unilist_del(*listref, dtor);
	free(*listref); *listref=NULL;
}

HARBOL_EXPORT void harbol_unilist_init(struct HarbolUniList *const list)
{
	if( !list )
		return;
	
	memset(list, 0, sizeof *list);
}

HARBOL_EXPORT size_t harbol_unilistnode_get_len(const struct HarbolUniList *const list)
{
	return list ? list->Len : 0;
}

HARBOL_EXPORT bool harbol_unilist_insert_node_at_head(struct HarbolUniList *const list, struct HarbolUniListNode *const node)
{
	if( !list || !node )
		return false;
	
	node->Next = list->Head;
	list->Head = node;
	if( !list->Tail )
		list->Tail = node;
	list->Len++;
	return true;
}

HARBOL_EXPORT bool harbol_unilist_insert_node_at_tail(struct HarbolUniList *const list, struct HarbolUniListNode *const node)
{
	if( !list || !node )
		return false;
	
	if( list->Head ) {
		node->Next = NULL;
		list->Tail->Next = node;
		list->Tail = node;
	}
	else list->Head = list->Tail = node;
	list->Len++;
	return true;
}

HARBOL_EXPORT bool harbol_unilist_insert_node_at_index(struct HarbolUniList *const list, struct HarbolUniListNode *const node, const size_t index)
{
	if( !list || !node )
		return false;
	else if( !list->Head || index==0 )
		return harbol_unilist_insert_node_at_head(list, node);
	// if index is out of bounds, append at tail end.
	else if( index >= list->Len )
		return harbol_unilist_insert_node_at_tail(list, node);
	
	struct HarbolUniListNode
		*curr=list->Head,
		*prev=NULL
	;
	size_t i=0;
	while( curr->Next != NULL && i != index ) {
		prev = curr;
		curr = curr->Next;
		i++;
	}
	if( i ) {
		if( prev == list->Tail )
			list->Tail->Next = node;
		else prev->Next = node;
		node->Next = curr;
		list->Len++;
		return true;
	}
	return false;
}

HARBOL_EXPORT bool harbol_unilist_insert_at_head(struct HarbolUniList *const list, const union HarbolValue val)
{
	if( !list )
		return false;
	struct HarbolUniListNode *node = harbol_unilistnode_new_val(val);
	if( !node )
		return false;
	
	const bool result = harbol_unilist_insert_node_at_head(list, node);
	if( !result )
		harbol_unilistnode_free(&node, NULL);
	return result;
}

HARBOL_EXPORT bool harbol_unilist_insert_at_tail(struct HarbolUniList *const list, const union HarbolValue val)
{
	if( !list )
		return false;
	struct HarbolUniListNode *node = harbol_unilistnode_new_val(val);
	if( !node )
		return false;
	
	const bool result = harbol_unilist_insert_node_at_tail(list, node);
	if( !result )
		harbol_unilistnode_free(&node, NULL);
	return result;
}

HARBOL_EXPORT bool harbol_unilist_insert_at_index(struct HarbolUniList *const list, const union HarbolValue val, const size_t index)
{
	if( !list )
		return false;
	struct HarbolUniListNode *node = harbol_unilistnode_new_val(val);
	if( !node )
		return false;
	
	const bool result = harbol_unilist_insert_node_at_index(list, node, index);
	if( !result )
		harbol_unilistnode_free(&node, NULL);
	return result;
}

HARBOL_EXPORT struct HarbolUniListNode *harbol_unilist_get_node_by_index(const struct HarbolUniList *const list, const size_t index)
{
	if( !list )
		return NULL;
	else if( index==0 )
		return list->Head;
	else if( index >= list->Len )
		return list->Tail;
	
	struct HarbolUniListNode *node = list->Head;
	for( size_t i=0 ; i<list->Len ; i++ ) {
		if( node && i==index )
			return node;
		node = node->Next;
	}
	return NULL;
}

HARBOL_EXPORT struct HarbolUniListNode *harbol_unilist_get_node_by_val(const struct HarbolUniList *const list, const union HarbolValue val)
{
	if( !list )
		return NULL;
	for( struct HarbolUniListNode *i=list->Head ; i ; i=i->Next )
		if( !memcmp(&i->Data, &val, sizeof val) )
			return i;
	return NULL;
}

HARBOL_EXPORT union HarbolValue harbol_unilist_get_val(const struct HarbolUniList *const list, const size_t index)
{
	if( !list )
		return (union HarbolValue){0};
	else if( index==0 && list->Head )
		return list->Head->Data;
	else if( index >= list->Len && list->Tail )
		return list->Tail->Data;
	
	struct HarbolUniListNode *node = list->Head;
	for( size_t i=0 ; i<list->Len ; i++ ) {
		if( node && i==index )
			return node->Data;
		if( !node->Next )
			break;
		node = node->Next;
	}
	return (union HarbolValue){0};
}

HARBOL_EXPORT void harbol_unilist_set_val(struct HarbolUniList *const list, const size_t index, const union HarbolValue val)
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
	
	struct HarbolUniListNode *node = list->Head;
	for( size_t i=0 ; i<list->Len ; i++ ) {
		if( node && i==index ) {
			node->Data = val;
			break;
		}
		if( !node->Next )
			break;
		node = node->Next;
	}
}

HARBOL_EXPORT bool harbol_unilist_del_node_by_index(struct HarbolUniList *const list, const size_t index, fnDestructor *const dtor)
{
	if( !list || !list->Len )
		return false;
	
	struct HarbolUniListNode *node = harbol_unilist_get_node_by_index(list, index);
	if( !node )
		return false;
	
	if( node==list->Head )
		list->Head = node->Next;
	else {
		struct HarbolUniListNode *travnode = list->Head;
		for( size_t i=0 ; i<list->Len ; i++ ) {
			if( travnode->Next == node ) {
				if( list->Tail == node ) {
					travnode->Next = NULL;
					list->Tail = travnode;
				}
				else travnode->Next = node->Next;
				break;
			}
			travnode = travnode->Next;
		}
	}
	
	if( dtor )
		(*dtor)(&node->Data.Ptr);
	free(node); node=NULL;
	
	list->Len--;
	if( !list->Len && list->Tail )
		list->Tail = NULL;
	return true;
}

HARBOL_EXPORT bool harbol_unilist_del_node_by_ref(struct HarbolUniList *const list, struct HarbolUniListNode **noderef, fnDestructor *const dtor)
{
	if( !list || !*noderef )
		return false;
	
	struct HarbolUniListNode *node = *noderef;
	if( node==list->Head )
		list->Head = node->Next;
	else {
		struct HarbolUniListNode *travnode = list->Head;
		for( size_t i=0 ; i<list->Len ; i++ ) {
			if( travnode->Next == node ) {
				if( list->Tail == node ) {
					travnode->Next = NULL;
					list->Tail = travnode;
				}
				else travnode->Next = node->Next;
				break;
			}
			travnode = travnode->Next;
		}
	}
	
	if( dtor )
		(*dtor)(&node->Data.Ptr);
	free(*noderef); *noderef=NULL;
	list->Len--;
	return true;
}

HARBOL_EXPORT struct HarbolUniListNode *harbol_unilist_get_head_node(const struct HarbolUniList *const list)
{
	return list ? list->Head : NULL;
}

HARBOL_EXPORT struct HarbolUniListNode *harbol_unilist_get_tail_node(const struct HarbolUniList *const list)
{
	return list ? list->Tail : NULL;
}

HARBOL_EXPORT void harbol_unilist_from_bilist(struct HarbolUniList *const unilist, const struct HarbolBiList *const bilist)
{
	if( !unilist || !bilist )
		return;
	
	for( struct HarbolBiListNode *n=bilist->Head ; n ; n = n->Next )
		harbol_unilist_insert_at_tail(unilist, n->Data);
}

HARBOL_EXPORT void harbol_unilist_from_hashmap(struct HarbolUniList *const unilist, const struct HarbolHashmap *const map)
{
	if( !unilist || !map )
		return;
	
	for( size_t i=0 ; i<map->Len ; i++ ) {
		struct HarbolVector *vec = map->Table + i;
		for( size_t n=0 ; n<harbol_vector_get_count(vec) ; n++ ) {
			struct HarbolKeyValPair *node = vec->Table[n].Ptr;
			harbol_unilist_insert_at_tail(unilist, node->Data);
		}
	}
}

HARBOL_EXPORT void harbol_unilist_from_vector(struct HarbolUniList *const unilist, const struct HarbolVector *const v)
{
	if( !unilist || !v || !v->Table )
		return;
	
	for( size_t i=0 ; i<v->Count ; i++ )
		harbol_unilist_insert_at_tail(unilist, v->Table[i]);
}

HARBOL_EXPORT void harbol_unilist_from_graph(struct HarbolUniList *const unilist, const struct HarbolGraph *const graph)
{
	if( !unilist || !graph )
		return;
	
	for( size_t i=0 ; i<graph->Vertices.Count ; i++ ) {
		struct HarbolGraphVertex *vert = graph->Vertices.Table[i].Ptr;
		harbol_unilist_insert_at_tail(unilist, vert->Data);
	}
}

HARBOL_EXPORT void harbol_unilist_from_linkmap(struct HarbolUniList *const unilist, const struct HarbolLinkMap *const map)
{
	if( !unilist || !map )
		return;
	
	for( size_t i=0 ; i<map->Order.Count ; i++ ) {
		struct HarbolKeyValPair *n = map->Order.Table[i].Ptr;
		harbol_unilist_insert_at_tail(unilist, n->Data);
	}
}

HARBOL_EXPORT struct HarbolUniList *harbol_unilist_new_from_bilist(const struct HarbolBiList *const bilist)
{
	if( !bilist )
		return NULL;
	struct HarbolUniList *const unilist = harbol_unilist_new();
	harbol_unilist_from_bilist(unilist, bilist);
	return unilist;
}

HARBOL_EXPORT struct HarbolUniList *harbol_unilist_new_from_hashmap(const struct HarbolHashmap *const map)
{
	if( !map )
		return NULL;
	struct HarbolUniList *const unilist = harbol_unilist_new();
	harbol_unilist_from_hashmap(unilist, map);
	return unilist;
}

HARBOL_EXPORT struct HarbolUniList *harbol_unilist_new_from_vector(const struct HarbolVector *const v)
{
	if( !v )
		return NULL;
	struct HarbolUniList *const unilist = harbol_unilist_new();
	harbol_unilist_from_vector(unilist, v);
	return unilist;
}

HARBOL_EXPORT struct HarbolUniList *harbol_unilist_new_from_graph(const struct HarbolGraph *const graph)
{
	if( !graph )
		return NULL;
	struct HarbolUniList *const unilist = harbol_unilist_new();
	harbol_unilist_from_graph(unilist, graph);
	return unilist;
}

HARBOL_EXPORT struct HarbolUniList *harbol_unilist_new_from_linkmap(const struct HarbolLinkMap *const map)
{
	if( !map )
		return NULL;
	struct HarbolUniList *const unilist = harbol_unilist_new();
	harbol_unilist_from_linkmap(unilist, map);
	return unilist;
}
