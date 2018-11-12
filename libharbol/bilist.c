#include <stdlib.h>
#include <stdio.h>

#ifdef OS_WINDOWS
	#define HARBOL_LIB
#endif
#include "harbol.h"

/* Doubly Linked List Node code */
/////////////////////////////////////////
HARBOL_EXPORT struct HarbolBiListNode *HarbolBiListNode_New(void)
{
	return calloc(1, sizeof(struct HarbolBiListNode));
}

HARBOL_EXPORT struct HarbolBiListNode *HarbolBiListNode_NewVal(const union HarbolValue val)
{
	struct HarbolBiListNode *node = calloc(1, sizeof *node);
	if( node )
		node->Data = val;
	return node;
}

HARBOL_EXPORT void HarbolBiListNode_Del(struct HarbolBiListNode *const node, fnDestructor *const dtor)
{
	if( !node )
		return;
	
	// yes I know this syntax is unnecessary but it's for readability.
	if( dtor )
		(*dtor)(&node->Data.Ptr);
	
	node->Prev = NULL;
	HarbolBiListNode_Free(&node->Next, dtor);
}

HARBOL_EXPORT void HarbolBiListNode_Free(struct HarbolBiListNode ** noderef, fnDestructor *const dtor)
{
	if( !*noderef )
		return;
	
	HarbolBiListNode_Del(*noderef, dtor);
	free(*noderef), *noderef=NULL;
}

HARBOL_EXPORT struct HarbolBiListNode *HarbolBiListNode_GetNextNode(const struct HarbolBiListNode *const node)
{
	return node ? node->Next : NULL;
}

HARBOL_EXPORT struct HarbolBiListNode *HarbolBiListNode_GetPrevNode(const struct HarbolBiListNode *const node)
{
	return node ? node->Prev : NULL;
}

HARBOL_EXPORT union HarbolValue HarbolBiListNode_GetValue(const struct HarbolBiListNode *const node)
{
	return node ? node->Data : (union HarbolValue){0};
}
/////////////////////////////////////////



/* Doubly Linked List code */
/////////////////////////////////////////
HARBOL_EXPORT struct HarbolBiList *HarbolBiList_New(void)
{
	struct HarbolBiList *list = calloc(1, sizeof *list);
	return list;
}

HARBOL_EXPORT void HarbolBiList_Del(struct HarbolBiList *const list, fnDestructor *const dtor)
{
	if( !list )
		return;
	
	HarbolBiListNode_Free(&list->Head, dtor);
	memset(list, 0, sizeof *list);
}

HARBOL_EXPORT void HarbolBiList_Free(struct HarbolBiList **listref, fnDestructor *const dtor)
{
	if( !*listref )
		return;
	
	HarbolBiList_Del(*listref, dtor);
	free(*listref), *listref=NULL;
}

HARBOL_EXPORT void HarbolBiList_Init(struct HarbolBiList *const list)
{
	if( !list )
		return;
	
	memset(list, 0, sizeof *list);
}

HARBOL_EXPORT size_t HarbolBiList_Len(const struct HarbolBiList *const list)
{
	return list ? list->Len : 0;
}

HARBOL_EXPORT bool HarbolBiList_InsertNodeAtHead(struct HarbolBiList *const list, struct HarbolBiListNode *const node)
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

HARBOL_EXPORT bool HarbolBiList_InsertNodeAtTail(struct HarbolBiList *const list, struct HarbolBiListNode *const node)
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

HARBOL_EXPORT bool HarbolBiList_InsertNodeAtIndex(struct HarbolBiList *const list, struct HarbolBiListNode *const node, const size_t index)
{
	if( !list || !node )
		return false;
	else if( !list->Head || index==0 )
		return HarbolBiList_InsertNodeAtHead(list, node);
	// if index is out of bounds, append at tail end.
	else if( index >= list->Len )
		return HarbolBiList_InsertNodeAtTail(list, node);
	
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

HARBOL_EXPORT bool HarbolBiList_InsertValueAtHead(struct HarbolBiList *const list, const union HarbolValue val)
{
	return ( !list ) ? false : HarbolBiList_InsertNodeAtHead(list, HarbolBiListNode_NewVal(val));
}

HARBOL_EXPORT bool HarbolBiList_InsertValueAtTail(struct HarbolBiList *const list, const union HarbolValue val)
{
	return ( !list ) ? false : HarbolBiList_InsertNodeAtTail(list, HarbolBiListNode_NewVal(val));
}

HARBOL_EXPORT bool HarbolBiList_InsertValueAtIndex(struct HarbolBiList *const list, const union HarbolValue val, const size_t index)
{
	return ( !list ) ? false : HarbolBiList_InsertNodeAtIndex(list, HarbolBiListNode_NewVal(val), index);
}

HARBOL_EXPORT struct HarbolBiListNode *HarbolBiList_GetNode(const struct HarbolBiList *const list, const size_t index)
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

HARBOL_EXPORT struct HarbolBiListNode *HarbolBiList_GetNodeByValue(const struct HarbolBiList *const list, const union HarbolValue val)
{
	if( !list )
		return NULL;
	for( struct HarbolBiListNode *i=list->Head ; i ; i=i->Next )
		if( !memcmp(&i->Data, &val, sizeof val) )
			return i;
	return NULL;
}

HARBOL_EXPORT union HarbolValue HarbolBiList_GetValue(const struct HarbolBiList *const list, const size_t index)
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

HARBOL_EXPORT void HarbolBiList_SetValue(struct HarbolBiList *const list, const size_t index, const union HarbolValue val)
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

HARBOL_EXPORT bool HarbolBiList_DelNodeByIndex(struct HarbolBiList *const list, const size_t index, fnDestructor *const dtor)
{
	if( !list || !list->Len )
		return false;
	
	struct HarbolBiListNode *node = HarbolBiList_GetNode(list, index);
	node->Prev ? (node->Prev->Next = node->Next) : (list->Head = node->Next);
	node->Next ? (node->Next->Prev = node->Prev) : (list->Tail = node->Prev);
	
	if( dtor )
		(*dtor)(&node->Data.Ptr);
	
	free(node), node=NULL;
	list->Len--;
	return true;
}

HARBOL_EXPORT bool HarbolBiList_DelNodeByRef(struct HarbolBiList *const list, struct HarbolBiListNode **noderef, fnDestructor *const dtor)
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

HARBOL_EXPORT struct HarbolBiListNode *HarbolBiList_GetHead(const struct HarbolBiList *const list)
{
	return list ? list->Head : NULL;
}

HARBOL_EXPORT struct HarbolBiListNode *HarbolBiList_GetTail(const struct HarbolBiList *const list)
{
	return list ? list->Tail : NULL;
}

HARBOL_EXPORT void HarbolBiList_FromHarbolUniList(struct HarbolBiList *const bilist, const struct HarbolUniList *const unilist)
{
	if( !bilist || !unilist )
		return;
	
	for( struct HarbolUniListNode *n=unilist->Head ; n ; n = n->Next )
		HarbolBiList_InsertValueAtTail(bilist, n->Data);
}

HARBOL_EXPORT void HarbolBiList_FromHarbolHashmap(struct HarbolBiList *const bilist, const struct HarbolHashmap *const map)
{
	if( !bilist || !map )
		return;
	
	for( size_t i=0 ; i<map->Len ; i++ ) {
		struct HarbolVector *vec = map->Table + i;
		for( size_t n=0 ; n<HarbolVector_Count(vec) ; n++ ) {
			struct HarbolKeyValPair *node = vec->Table[n].Ptr;
			HarbolBiList_InsertValueAtTail(bilist, node->Data);
		}
	}
}

HARBOL_EXPORT void HarbolBiList_FromHarbolVector(struct HarbolBiList *const bilist, const struct HarbolVector *const v)
{
	if( !bilist || !v )
		return;
	
	for( size_t i=0 ; i<v->Count ; i++ )
		HarbolBiList_InsertValueAtTail(bilist, v->Table[i]);
}

HARBOL_EXPORT void HarbolBiList_FromHarbolTuple(struct HarbolBiList *const bilist, const struct HarbolTuple *const tup)
{
	if( !bilist || !tup || !tup->Items || !tup->Len )
		return;
	
	for( size_t i=0 ; i<tup->Len ; i++ )
		HarbolBiList_InsertValueAtTail(bilist, tup->Items[i]);
}

HARBOL_EXPORT void HarbolBiList_FromHarbolGraph(struct HarbolBiList *const bilist, const struct HarbolGraph *const graph)
{
	if( !bilist || !graph || !graph->Vertices.Table )
		return;
	
	for( size_t i=0 ; i<graph->Vertices.Count ; i++ ) {
		struct HarbolGraphVertex *vert = graph->Vertices.Table[i].Ptr;
		HarbolBiList_InsertValueAtTail(bilist, vert->Data);
	}
}

HARBOL_EXPORT void HarbolBiList_FromHarbolLinkMap(struct HarbolBiList *const bilist, const struct HarbolLinkMap *const map)
{
	if( !bilist || !map )
		return;
	
	for( size_t i=0 ; i<map->Order.Count ; i++ ) {
		struct HarbolKeyValPair *n = map->Order.Table[i].Ptr;
		HarbolBiList_InsertValueAtTail(bilist, n->Data);
	}
}

HARBOL_EXPORT struct HarbolBiList *HarbolBiList_NewFromHarbolUniList(const struct HarbolUniList *const unilist)
{
	if( !unilist )
		return NULL;
	struct HarbolBiList *bilist = HarbolBiList_New();
	HarbolBiList_FromHarbolUniList(bilist, unilist);
	return bilist;
}

HARBOL_EXPORT struct HarbolBiList *HarbolBiList_NewFromHarbolHashmap(const struct HarbolHashmap *const map)
{
	if( !map )
		return NULL;
	struct HarbolBiList *bilist = HarbolBiList_New();
	HarbolBiList_FromHarbolHashmap(bilist, map);
	return bilist;
}

HARBOL_EXPORT struct HarbolBiList *HarbolBiList_NewFromHarbolVector(const struct HarbolVector *const v)
{
	if( !v )
		return NULL;
	struct HarbolBiList *bilist = HarbolBiList_New();
	HarbolBiList_FromHarbolVector(bilist, v);
	return bilist;
}

HARBOL_EXPORT struct HarbolBiList *HarbolBiList_NewFromHarbolTuple(const struct HarbolTuple *const tup)
{
	if( !tup || !tup->Items || !tup->Len )
		return NULL;
	struct HarbolBiList *bilist = HarbolBiList_New();
	HarbolBiList_FromHarbolTuple(bilist, tup);
	return bilist;
}

HARBOL_EXPORT struct HarbolBiList *HarbolBiList_NewFromHarbolGraph(const struct HarbolGraph *const graph)
{
	if( !graph )
		return NULL;
	struct HarbolBiList *bilist = HarbolBiList_New();
	HarbolBiList_FromHarbolGraph(bilist, graph);
	return bilist;
}

HARBOL_EXPORT struct HarbolBiList *HarbolBiList_NewFromHarbolLinkMap(const struct HarbolLinkMap *const map)
{
	if( !map )
		return NULL;
	struct HarbolBiList *bilist = HarbolBiList_New();
	HarbolBiList_FromHarbolLinkMap(bilist, map);
	return bilist;
}
