#include <stdlib.h>
#include <stdio.h>
#ifdef OS_WINDOWS
	#define HARBOL_LIB
#endif
#include "harbol.h"

/* Singly Linked List Node code */
/////////////////////////////////////////
HARBOL_EXPORT struct HarbolUniListNode *HarbolUniListNode_New(void)
{
	return calloc(1, sizeof(struct HarbolUniListNode));
}

HARBOL_EXPORT struct HarbolUniListNode *HarbolUniListNode_NewVal(const union HarbolValue val)
{
	struct HarbolUniListNode *node = HarbolUniListNode_New();
	if( node )
		node->Data = val;
	return node;
}

HARBOL_EXPORT void HarbolUniListNode_Del(struct HarbolUniListNode *const node, fnDestructor *const dtor)
{
	if( !node )
		return;
	
	if( dtor )
		(*dtor)(&node->Data.Ptr);
	
	if( node->Next )
		HarbolUniListNode_Free(&node->Next, dtor);
}

HARBOL_EXPORT void HarbolUniListNode_Free(struct HarbolUniListNode **noderef, fnDestructor *const dtor)
{
	if( !noderef || !*noderef )
		return;
	
	HarbolUniListNode_Del(*noderef, dtor);
	free(*noderef); *noderef=NULL;
}

HARBOL_EXPORT struct HarbolUniListNode *HarbolUniListNode_GetNextNode(const struct HarbolUniListNode *const node)
{
	return node ? node->Next : NULL;
}

HARBOL_EXPORT union HarbolValue HarbolUniListNode_GetValue(const struct HarbolUniListNode *const node)
{
	return node ? node->Data : (union HarbolValue){0};
}
/////////////////////////////////////////


/* Singly Linked List code */
/////////////////////////////////////////
HARBOL_EXPORT struct HarbolUniList *HarbolUniList_New(void)
{
	struct HarbolUniList *list = calloc(1, sizeof *list);
	return list;
}

HARBOL_EXPORT void HarbolUniList_Del(struct HarbolUniList *const list, fnDestructor *const dtor)
{
	if( !list )
		return;
	
	HarbolUniListNode_Free(&list->Head, dtor);
	memset(list, 0, sizeof *list);
}

HARBOL_EXPORT void HarbolUniList_Free(struct HarbolUniList **listref, fnDestructor *const dtor)
{
	if( !*listref )
		return;
	
	HarbolUniList_Del(*listref, dtor);
	free(*listref); *listref=NULL;
}

HARBOL_EXPORT void HarbolUniList_Init(struct HarbolUniList *const list)
{
	if( !list )
		return;
	
	memset(list, 0, sizeof *list);
}

HARBOL_EXPORT size_t HarbolUniList_Len(const struct HarbolUniList *const list)
{
	return list ? list->Len : 0;
}

HARBOL_EXPORT bool HarbolUniList_InsertNodeAtHead(struct HarbolUniList *const list, struct HarbolUniListNode *const node)
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

HARBOL_EXPORT bool HarbolUniList_InsertNodeAtTail(struct HarbolUniList *const list, struct HarbolUniListNode *const node)
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

HARBOL_EXPORT bool HarbolUniList_InsertNodeAtIndex(struct HarbolUniList *const list, struct HarbolUniListNode *const node, const size_t index)
{
	if( !list || !node )
		return false;
	else if( !list->Head || index==0 )
		return HarbolUniList_InsertNodeAtHead(list, node);
	// if index is out of bounds, append at tail end.
	else if( index >= list->Len )
		return HarbolUniList_InsertNodeAtTail(list, node);
	
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

HARBOL_EXPORT bool HarbolUniList_InsertValueAtHead(struct HarbolUniList *const list, const union HarbolValue val)
{
	if( !list )
		return false;
	struct HarbolUniListNode *node = HarbolUniListNode_NewVal(val);
	if( !node )
		return false;
	
	const bool result = HarbolUniList_InsertNodeAtHead(list, node);
	if( !result )
		HarbolUniListNode_Free(&node, NULL);
	return result;
}

HARBOL_EXPORT bool HarbolUniList_InsertValueAtTail(struct HarbolUniList *const list, const union HarbolValue val)
{
	if( !list )
		return false;
	struct HarbolUniListNode *node = HarbolUniListNode_NewVal(val);
	if( !node )
		return false;
	
	const bool result = HarbolUniList_InsertNodeAtTail(list, node);
	if( !result )
		HarbolUniListNode_Free(&node, NULL);
	return result;
}

HARBOL_EXPORT bool HarbolUniList_InsertValueAtIndex(struct HarbolUniList *const list, const union HarbolValue val, const size_t index)
{
	if( !list )
		return false;
	struct HarbolUniListNode *node = HarbolUniListNode_NewVal(val);
	if( !node )
		return false;
	
	const bool result = HarbolUniList_InsertNodeAtIndex(list, node, index);
	if( !result )
		HarbolUniListNode_Free(&node, NULL);
	return result;
}

HARBOL_EXPORT struct HarbolUniListNode *HarbolUniList_GetNode(const struct HarbolUniList *const list, const size_t index)
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

HARBOL_EXPORT struct HarbolUniListNode *HarbolUniList_GetNodeByValue(const struct HarbolUniList *const list, const union HarbolValue val)
{
	if( !list )
		return NULL;
	for( struct HarbolUniListNode *i=list->Head ; i ; i=i->Next )
		if( !memcmp(&i->Data, &val, sizeof val) )
			return i;
	return NULL;
}

HARBOL_EXPORT union HarbolValue HarbolUniList_GetValue(const struct HarbolUniList *const list, const size_t index)
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

HARBOL_EXPORT void HarbolUniList_SetValue(struct HarbolUniList *const list, const size_t index, const union HarbolValue val)
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

HARBOL_EXPORT bool HarbolUniList_DelNodeByIndex(struct HarbolUniList *const list, const size_t index, fnDestructor *const dtor)
{
	if( !list || !list->Len )
		return false;
	
	struct HarbolUniListNode *node = HarbolUniList_GetNode(list, index);
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

HARBOL_EXPORT bool HarbolUniList_DelNodeByRef(struct HarbolUniList *const list, struct HarbolUniListNode **noderef, fnDestructor *const dtor)
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

HARBOL_EXPORT struct HarbolUniListNode *HarbolUniList_GetHead(const struct HarbolUniList *const list)
{
	return list ? list->Head : NULL;
}

HARBOL_EXPORT struct HarbolUniListNode *HarbolUniList_GetTail(const struct HarbolUniList *const list)
{
	return list ? list->Tail : NULL;
}

HARBOL_EXPORT void HarbolUniList_FromHarbolBiList(struct HarbolUniList *const unilist, const struct HarbolBiList *const bilist)
{
	if( !unilist || !bilist )
		return;
	
	for( struct HarbolBiListNode *n=bilist->Head ; n ; n = n->Next )
		HarbolUniList_InsertValueAtTail(unilist, n->Data);
}

HARBOL_EXPORT void HarbolUniList_FromHarbolHashmap(struct HarbolUniList *const unilist, const struct HarbolHashmap *const map)
{
	if( !unilist || !map )
		return;
	
	for( size_t i=0 ; i<map->Len ; i++ ) {
		struct HarbolVector *vec = map->Table + i;
		for( size_t n=0 ; n<HarbolVector_Count(vec) ; n++ ) {
			struct HarbolKeyValPair *node = vec->Table[n].Ptr;
			HarbolUniList_InsertValueAtTail(unilist, node->Data);
		}
	}
}

HARBOL_EXPORT void HarbolUniList_FromHarbolVector(struct HarbolUniList *const unilist, const struct HarbolVector *const v)
{
	if( !unilist || !v || !v->Table )
		return;
	
	for( size_t i=0 ; i<v->Count ; i++ )
		HarbolUniList_InsertValueAtTail(unilist, v->Table[i]);
}

HARBOL_EXPORT void HarbolUniList_FromHarbolTuple(struct HarbolUniList *const unilist, const struct HarbolTuple *const tup)
{
	if( !unilist || !tup || !tup->Items || !tup->Len )
		return;
	
	for( size_t i=0 ; i<tup->Len ; i++ )
		HarbolUniList_InsertValueAtTail(unilist, tup->Items[i]);
}

HARBOL_EXPORT void HarbolUniList_FromHarbolGraph(struct HarbolUniList *const unilist, const struct HarbolGraph *const graph)
{
	if( !unilist || !graph )
		return;
	
	for( size_t i=0 ; i<graph->Vertices.Count ; i++ ) {
		struct HarbolGraphVertex *vert = graph->Vertices.Table[i].Ptr;
		HarbolUniList_InsertValueAtTail(unilist, vert->Data);
	}
}

HARBOL_EXPORT void HarbolUniList_FromHarbolLinkMap(struct HarbolUniList *const unilist, const struct HarbolLinkMap *const map)
{
	if( !unilist || !map )
		return;
	
	for( size_t i=0 ; i<map->Order.Count ; i++ ) {
		struct HarbolKeyValPair *n = map->Order.Table[i].Ptr;
		HarbolUniList_InsertValueAtTail(unilist, n->Data);
	}
}

HARBOL_EXPORT struct HarbolUniList *HarbolUniList_NewFromHarbolBiList(const struct HarbolBiList *const bilist)
{
	if( !bilist )
		return NULL;
	struct HarbolUniList *const unilist = HarbolUniList_New();
	HarbolUniList_FromHarbolBiList(unilist, bilist);
	return unilist;
}

HARBOL_EXPORT struct HarbolUniList *HarbolUniList_NewFromHarbolHashmap(const struct HarbolHashmap *const map)
{
	if( !map )
		return NULL;
	struct HarbolUniList *const unilist = HarbolUniList_New();
	HarbolUniList_FromHarbolHashmap(unilist, map);
	return unilist;
}

HARBOL_EXPORT struct HarbolUniList *HarbolUniList_NewFromHarbolVector(const struct HarbolVector *const v)
{
	if( !v )
		return NULL;
	struct HarbolUniList *const unilist = HarbolUniList_New();
	HarbolUniList_FromHarbolVector(unilist, v);
	return unilist;
}

HARBOL_EXPORT struct HarbolUniList *HarbolUniList_NewFromHarbolTuple(const struct HarbolTuple *const tup)
{
	if( !tup || !tup->Items || !tup->Len )
		return NULL;
	struct HarbolUniList *const unilist = HarbolUniList_New();
	HarbolUniList_FromHarbolTuple(unilist, tup);
	return unilist;
}

HARBOL_EXPORT struct HarbolUniList *HarbolUniList_NewFromHarbolGraph(const struct HarbolGraph *const graph)
{
	if( !graph )
		return NULL;
	struct HarbolUniList *const unilist = HarbolUniList_New();
	HarbolUniList_FromHarbolGraph(unilist, graph);
	return unilist;
}

HARBOL_EXPORT struct HarbolUniList *HarbolUniList_NewFromHarbolLinkMap(const struct HarbolLinkMap *const map)
{
	if( !map )
		return NULL;
	struct HarbolUniList *const unilist = HarbolUniList_New();
	HarbolUniList_FromHarbolLinkMap(unilist, map);
	return unilist;
}
