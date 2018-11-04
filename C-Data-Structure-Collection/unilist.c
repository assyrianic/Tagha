
#include <stdlib.h>
#include <stdio.h>
#include "dsc.h"

/* Singly Linked List Node code */
/////////////////////////////////////////
struct UniListNode *UniListNode_New(void)
{
	return calloc(1, sizeof(struct UniListNode));
}

struct UniListNode *UniListNode_NewVal(const union Value val)
{
	struct UniListNode *node = UniListNode_New();
	if( node )
		node->Data = val;
	return node;
}

void UniListNode_Del(struct UniListNode *const node, fnDestructor *const dtor)
{
	if( !node )
		return;
	
	if( dtor )
		(*dtor)(&node->Data.Ptr);
	
	if( node->Next )
		UniListNode_Free(&node->Next, dtor);
}

void UniListNode_Free(struct UniListNode **noderef, fnDestructor *const dtor)
{
	if( !noderef || !*noderef )
		return;
	
	UniListNode_Del(*noderef, dtor);
	free(*noderef); *noderef=NULL;
}

struct UniListNode *UniListNode_GetNextNode(const struct UniListNode *const node)
{
	return node ? node->Next : NULL;
}
union Value UniListNode_GetValue(const struct UniListNode *const node)
{
	return node ? node->Data : (union Value){0};
}
/////////////////////////////////////////


/* Singly Linked List code */
/////////////////////////////////////////
struct UniLinkedList *UniLinkedList_New(void)
{
	struct UniLinkedList *list = calloc(1, sizeof *list);
	return list;
}

void UniLinkedList_Del(struct UniLinkedList *const list, fnDestructor *const dtor)
{
	if( !list )
		return;
	
	UniListNode_Free(&list->Head, dtor);
	*list = (struct UniLinkedList){0};
}

void UniLinkedList_Free(struct UniLinkedList **listref, fnDestructor *const dtor)
{
	if( !*listref )
		return;
	
	UniLinkedList_Del(*listref, dtor);
	free(*listref); *listref=NULL;
}

void UniLinkedList_Init(struct UniLinkedList *const list)
{
	if( !list )
		return;
	
	*list = (struct UniLinkedList){0};
}

size_t UniLinkedList_Len(const struct UniLinkedList *const list)
{
	return list ? list->Len : 0;
}

bool UniLinkedList_InsertNodeAtHead(struct UniLinkedList *const list, struct UniListNode *const node)
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

bool UniLinkedList_InsertNodeAtTail(struct UniLinkedList *const list, struct UniListNode *const node)
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

bool UniLinkedList_InsertNodeAtIndex(struct UniLinkedList *const list, struct UniListNode *const node, const size_t index)
{
	if( !list || !node )
		return false;
	else if( !list->Head || index==0 )
		return UniLinkedList_InsertNodeAtHead(list, node);
	// if index is out of bounds, append at tail end.
	else if( index >= list->Len )
		return UniLinkedList_InsertNodeAtTail(list, node);
	
	struct UniListNode
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

bool UniLinkedList_InsertValueAtHead(struct UniLinkedList *const list, const union Value val)
{
	if( !list )
		return false;
	struct UniListNode *node = UniListNode_NewVal(val);
	if( !node )
		return false;
	
	const bool result = UniLinkedList_InsertNodeAtHead(list, node);
	if( !result )
		UniListNode_Free(&node, NULL);
	return result;
}

bool UniLinkedList_InsertValueAtTail(struct UniLinkedList *const list, const union Value val)
{
	if( !list )
		return false;
	struct UniListNode *node = UniListNode_NewVal(val);
	if( !node )
		return false;
	
	const bool result = UniLinkedList_InsertNodeAtTail(list, node);
	if( !result )
		UniListNode_Free(&node, NULL);
	return result;
}

bool UniLinkedList_InsertValueAtIndex(struct UniLinkedList *const list, const union Value val, const size_t index)
{
	if( !list )
		return false;
	struct UniListNode *node = UniListNode_NewVal(val);
	if( !node )
		return false;
	
	const bool result = UniLinkedList_InsertNodeAtIndex(list, node, index);
	if( !result )
		UniListNode_Free(&node, NULL);
	return result;
}

struct UniListNode *UniLinkedList_GetNode(const struct UniLinkedList *const list, const size_t index)
{
	if( !list )
		return NULL;
	else if( index==0 )
		return list->Head;
	else if( index >= list->Len )
		return list->Tail;
	
	struct UniListNode *node = list->Head;
	for( size_t i=0 ; i<list->Len ; i++ ) {
		if( node && i==index )
			return node;
		node = node->Next;
	}
	return NULL;
}

struct UniListNode *UniLinkedList_GetNodeByValue(const struct UniLinkedList *const list, const union Value val)
{
	if( !list )
		return NULL;
	for( struct UniListNode *i=list->Head ; i ; i=i->Next )
		if( !memcmp(&i->Data, &val, sizeof val) )
			return i;
	return NULL;
}

union Value UniLinkedList_GetValue(const struct UniLinkedList *const list, const size_t index)
{
	if( !list )
		return (union Value){0};
	else if( index==0 && list->Head )
		return list->Head->Data;
	else if( index >= list->Len && list->Tail )
		return list->Tail->Data;
	
	struct UniListNode *node = list->Head;
	for( size_t i=0 ; i<list->Len ; i++ ) {
		if( node && i==index )
			return node->Data;
		if( !node->Next )
			break;
		node = node->Next;
	}
	return (union Value){0};
}

void UniLinkedList_SetValue(struct UniLinkedList *const list, const size_t index, const union Value val)
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
	
	struct UniListNode *node = list->Head;
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

bool UniLinkedList_DelNodeByIndex(struct UniLinkedList *const list, const size_t index, fnDestructor *const dtor)
{
	if( !list || !list->Len )
		return false;
	
	struct UniListNode *node = UniLinkedList_GetNode(list, index);
	if( !node )
		return false;
	
	if( node==list->Head )
		list->Head = node->Next;
	else {
		struct UniListNode *travnode = list->Head;
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

bool UniLinkedList_DelNodeByRef(struct UniLinkedList *const list, struct UniListNode **noderef, fnDestructor *const dtor)
{
	if( !list || !*noderef )
		return false;
	
	struct UniListNode *node = *noderef;
	if( node==list->Head )
		list->Head = node->Next;
	else {
		struct UniListNode *travnode = list->Head;
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

struct UniListNode *UniLinkedList_GetHead(const struct UniLinkedList *const list)
{
	return list ? list->Head : NULL;
}
struct UniListNode *UniLinkedList_GetTail(const struct UniLinkedList *const list)
{
	return list ? list->Tail : NULL;
}

void UniLinkedList_FromBiLinkedList(struct UniLinkedList *const unilist, const struct BiLinkedList *const bilist)
{
	if( !unilist || !bilist )
		return;
	
	for( struct BiListNode *n=bilist->Head ; n ; n = n->Next )
		UniLinkedList_InsertValueAtTail(unilist, n->Data);
}

void UniLinkedList_FromMap(struct UniLinkedList *const unilist, const struct Hashmap *const map)
{
	if( !unilist || !map )
		return;
	
	for( size_t i=0 ; i<map->Len ; i++ ) {
		struct Vector *vec = map->Table + i;
		for( size_t n=0 ; n<Vector_Count(vec) ; n++ ) {
			struct KeyValPair *node = vec->Table[n].Ptr;
			UniLinkedList_InsertValueAtTail(unilist, node->Data);
		}
	}
}

void UniLinkedList_FromVector(struct UniLinkedList *const unilist, const struct Vector *const v)
{
	if( !unilist || !v || !v->Table )
		return;
	
	for( size_t i=0 ; i<v->Count ; i++ )
		UniLinkedList_InsertValueAtTail(unilist, v->Table[i]);
}

void UniLinkedList_FromTuple(struct UniLinkedList *const unilist, const struct Tuple *const tup)
{
	if( !unilist || !tup || !tup->Items || !tup->Len )
		return;
	
	for( size_t i=0 ; i<tup->Len ; i++ )
		UniLinkedList_InsertValueAtTail(unilist, tup->Items[i]);
}

void UniLinkedList_FromGraph(struct UniLinkedList *const unilist, const struct Graph *const graph)
{
	if( !unilist || !graph )
		return;
	
	for( size_t i=0 ; i<graph->Vertices.Count ; i++ ) {
		struct GraphVertex *vert = graph->Vertices.Table[i].Ptr;
		UniLinkedList_InsertValueAtTail(unilist, vert->Data);
	}
}

void UniLinkedList_FromLinkMap(struct UniLinkedList *const unilist, const struct LinkMap *const map)
{
	if( !unilist || !map )
		return;
	
	for( size_t i=0 ; i<map->Order.Count ; i++ ) {
		struct KeyValPair *n = map->Order.Table[i].Ptr;
		UniLinkedList_InsertValueAtTail(unilist, n->Data);
	}
}


struct UniLinkedList *UniLinkedList_NewFromBiLinkedList(const struct BiLinkedList *const bilist)
{
	if( !bilist )
		return NULL;
	struct UniLinkedList *const unilist = UniLinkedList_New();
	UniLinkedList_FromBiLinkedList(unilist, bilist);
	return unilist;
}

struct UniLinkedList *UniLinkedList_NewFromMap(const struct Hashmap *const map)
{
	if( !map )
		return NULL;
	struct UniLinkedList *const unilist = UniLinkedList_New();
	UniLinkedList_FromMap(unilist, map);
	return unilist;
}

struct UniLinkedList *UniLinkedList_NewFromVector(const struct Vector *const v)
{
	if( !v )
		return NULL;
	struct UniLinkedList *const unilist = UniLinkedList_New();
	UniLinkedList_FromVector(unilist, v);
	return unilist;
}

struct UniLinkedList *UniLinkedList_NewFromTuple(const struct Tuple *const tup)
{
	if( !tup || !tup->Items || !tup->Len )
		return NULL;
	struct UniLinkedList *const unilist = UniLinkedList_New();
	UniLinkedList_FromTuple(unilist, tup);
	return unilist;
}

struct UniLinkedList *UniLinkedList_NewFromGraph(const struct Graph *const graph)
{
	if( !graph )
		return NULL;
	struct UniLinkedList *const unilist = UniLinkedList_New();
	UniLinkedList_FromGraph(unilist, graph);
	return unilist;
}

struct UniLinkedList *UniLinkedList_NewFromLinkMap(const struct LinkMap *const map)
{
	if( !map )
		return NULL;
	struct UniLinkedList *const unilist = UniLinkedList_New();
	UniLinkedList_FromLinkMap(unilist, map);
	return unilist;
}
