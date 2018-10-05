
#include <stdlib.h>
#include <stdio.h>
#include "dsc.h"

/* Doubly Linked List Node code */
/////////////////////////////////////////
struct BiListNode *BiListNode_New(void)
{
	return calloc(1, sizeof(struct BiListNode));
}

struct BiListNode *BiListNode_NewVal(const union Value val)
{
	struct BiListNode *node = calloc(1, sizeof *node);
	if( node )
		node->Data = val;
	return node;
}

void BiListNode_Del(struct BiListNode *const node, fnDestructor *const dtor)
{
	if( !node )
		return;
	
	// yes I know this syntax is unnecessary but it's for readability.
	if( dtor )
		(*dtor)(&node->Data.Ptr);
	
	node->Prev = NULL;
	BiListNode_Free(&node->Next, dtor);
}

void BiListNode_Free(struct BiListNode ** noderef, fnDestructor *const dtor)
{
	if( !*noderef )
		return;
	
	BiListNode_Del(*noderef, dtor);
	free(*noderef), *noderef=NULL;
}

struct BiListNode *BiListNode_GetNextNode(const struct BiListNode *const node)
{
	return node ? node->Next : NULL;
}

struct BiListNode *BiListNode_GetPrevNode(const struct BiListNode *const node)
{
	return node ? node->Prev : NULL;
}

union Value BiListNode_GetValue(const struct BiListNode *const node)
{
	return node ? node->Data : (union Value){0};
}
/////////////////////////////////////////



/* Doubly Linked List code */
/////////////////////////////////////////
struct BiLinkedList *BiLinkedList_New(void)
{
	struct BiLinkedList *list = calloc(1, sizeof *list);
	return list;
}

void BiLinkedList_Del(struct BiLinkedList *const list, fnDestructor *const dtor)
{
	if( !list )
		return;
	
	BiListNode_Free(&list->Head, dtor);
	*list = (struct BiLinkedList){0};
}

void BiLinkedList_Free(struct BiLinkedList **listref, fnDestructor *const dtor)
{
	if( !*listref )
		return;
	
	BiLinkedList_Del(*listref, dtor);
	free(*listref), *listref=NULL;
}

void BiLinkedList_Init(struct BiLinkedList *const list)
{
	if( !list )
		return;
	
	*list = (struct BiLinkedList){0};
}

size_t BiLinkedList_Len(const struct BiLinkedList *const list)
{
	return list ? list->Len : 0;
}

bool BiLinkedList_InsertNodeAtHead(struct BiLinkedList *const list, struct BiListNode *const node)
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

bool BiLinkedList_InsertNodeAtTail(struct BiLinkedList *const list, struct BiListNode *const node)
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

bool BiLinkedList_InsertNodeAtIndex(struct BiLinkedList *const list, struct BiListNode *const node, const size_t index)
{
	if( !list || !node )
		return false;
	else if( !list->Head || index==0 )
		return BiLinkedList_InsertNodeAtHead(list, node);
	// if index is out of bounds, append at tail end.
	else if( index >= list->Len )
		return BiLinkedList_InsertNodeAtTail(list, node);
	
	const bool prev_dir = ( index >= list->Len/2 );
	struct BiListNode *curr = prev_dir ? list->Tail : list->Head;
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

bool BiLinkedList_InsertValueAtHead(struct BiLinkedList *const list, const union Value val)
{
	if( !list )
		return false;
	
	return BiLinkedList_InsertNodeAtHead(list, BiListNode_NewVal(val));
}

bool BiLinkedList_InsertValueAtTail(struct BiLinkedList *const list, const union Value val)
{
	if( !list )
		return false;
	
	return BiLinkedList_InsertNodeAtTail(list, BiListNode_NewVal(val));
}

bool BiLinkedList_InsertValueAtIndex(struct BiLinkedList *const list, const union Value val, const size_t index)
{
	if( !list )
		return false;
	
	return BiLinkedList_InsertNodeAtIndex(list, BiListNode_NewVal(val), index);
}


struct BiListNode *BiLinkedList_GetNode(const struct BiLinkedList *const list, const size_t index)
{
	if( !list )
		return NULL;
	else if( index==0 )
		return list->Head;
	else if( index >= list->Len )
		return list->Tail;
	
	const bool prev_dir = ( index >= list->Len/2 );
	struct BiListNode *node = prev_dir ? list->Tail : list->Head;
	for( size_t i=prev_dir ? list->Len-1 : 0 ; i<list->Len ; prev_dir ? i-- : i++ ) {
		if( node && i==index )
			return node;
		node = prev_dir ? node->Prev : node->Next;
	}
	return NULL;
}

struct BiListNode *BiLinkedList_GetNodeByValue(const struct BiLinkedList *const list, const union Value val)
{
	if( !list )
		return NULL;
	for( struct BiListNode *i=list->Head ; i ; i=i->Next )
		if( !memcmp(&i->Data, &val, sizeof val) )
			return i;
	return NULL;
}

union Value BiLinkedList_GetValue(const struct BiLinkedList *const list, const size_t index)
{
	if( !list )
		return (union Value){0};
	else if( index==0 && list->Head )
		return list->Head->Data;
	else if( index >= list->Len && list->Tail )
		return list->Tail->Data;
	
	const bool prev_dir = ( index >= list->Len/2 );
	struct BiListNode *node = prev_dir ? list->Tail : list->Head;
	for( size_t i=prev_dir ? list->Len-1 : 0 ; i<list->Len ; prev_dir ? i-- : i++ ) {
		if( node && i==index )
			return node->Data;
		if( (prev_dir && !node->Prev) || (!prev_dir && !node->Next) )
			break;
		node = prev_dir ? node->Prev : node->Next;
	}
	return (union Value){0};
}

void BiLinkedList_SetValue(struct BiLinkedList *const list, const size_t index, const union Value val)
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
	struct BiListNode *node = prev_dir ? list->Tail : list->Head;
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
bool BiLinkedList_DelNodeByIndex(struct BiLinkedList *const list, const size_t index, fnDestructor *const dtor)
{
	if( !list || !list->Len )
		return false;
	
	struct BiListNode *node = BiLinkedList_GetNode(list, index);
	node->Prev ? (node->Prev->Next = node->Next) : (list->Head = node->Next);
	node->Next ? (node->Next->Prev = node->Prev) : (list->Tail = node->Prev);
	
	if( dtor )
		(*dtor)(&node->Data.Ptr);
	
	free(node), node=NULL;
	list->Len--;
	return true;
}

bool BiLinkedList_DelNodeByRef(struct BiLinkedList *const list, struct BiListNode **noderef, fnDestructor *const dtor)
{
	if( !list || !*noderef )
		return false;
	
	struct BiListNode *node = *noderef;
	node->Prev ? (node->Prev->Next = node->Next) : (list->Head = node->Next);
	node->Next ? (node->Next->Prev = node->Prev) : (list->Tail = node->Prev);
	
	if( dtor )
		(*dtor)(&node->Data.Ptr);
	
	free(*noderef), *noderef=NULL;
	node = NULL;
	list->Len--;
	return true;
}


struct BiListNode *BiLinkedList_GetHead(const struct BiLinkedList *const list)
{
	return list ? list->Head : NULL;
}
struct BiListNode *BiLinkedList_GetTail(const struct BiLinkedList *const list)
{
	return list ? list->Tail : NULL;
}

void BiLinkedList_FromUniLinkedList(struct BiLinkedList *const bilist, const struct UniLinkedList *const unilist)
{
	if( !bilist || !unilist )
		return;
	
	for( struct UniListNode *n=unilist->Head ; n ; n = n->Next )
		BiLinkedList_InsertValueAtTail(bilist, n->Data);
}

void BiLinkedList_FromMap(struct BiLinkedList *const bilist, const struct Hashmap *const map)
{
	if( !bilist || !map )
		return;
	
	for( size_t i=0 ; i<map->Len ; i++ ) {
		struct Vector *vec = map->Table + i;
		for( size_t n=0 ; n<Vector_Count(vec) ; n++ ) {
			struct KeyNode *node = vec->Table[n].Ptr;
			BiLinkedList_InsertValueAtTail(bilist, node->Data);
		}
	}
}

void BiLinkedList_FromVector(struct BiLinkedList *const bilist, const struct Vector *const v)
{
	if( !bilist || !v )
		return;
	
	for( size_t i=0 ; i<v->Count ; i++ )
		BiLinkedList_InsertValueAtTail(bilist, v->Table[i]);
}

void BiLinkedList_FromTuple(struct BiLinkedList *const bilist, const struct Tuple *const tup)
{
	if( !bilist || !tup || !tup->Items || !tup->Len )
		return;
	
	for( size_t i=0 ; i<tup->Len ; i++ )
		BiLinkedList_InsertValueAtTail(bilist, tup->Items[i]);
}

void BiLinkedList_FromGraph(struct BiLinkedList *const bilist, const struct Graph *const graph)
{
	if( !bilist || !graph || !graph->Vertices.Table )
		return;
	
	for( size_t i=0 ; i<graph->Vertices.Count ; i++ ) {
		struct GraphVertex *vert = graph->Vertices.Table[i].Ptr;
		BiLinkedList_InsertValueAtTail(bilist, vert->Data);
	}
}

void BiLinkedList_FromLinkMap(struct BiLinkedList *const bilist, const struct LinkMap *const map)
{
	if( !bilist || !map )
		return;
	
	for( size_t i=0 ; i<map->Order.Count ; i++ ) {
		struct KeyNode *n = map->Order.Table[i].Ptr;
		BiLinkedList_InsertValueAtTail(bilist, n->Data);
	}
}


struct BiLinkedList *BiLinkedList_NewFromUniLinkedList(const struct UniLinkedList *const unilist)
{
	if( !unilist )
		return NULL;
	struct BiLinkedList *bilist = BiLinkedList_New();
	BiLinkedList_FromUniLinkedList(bilist, unilist);
	return bilist;
}

struct BiLinkedList *BiLinkedList_NewFromMap(const struct Hashmap *const map)
{
	if( !map )
		return NULL;
	struct BiLinkedList *bilist = BiLinkedList_New();
	BiLinkedList_FromMap(bilist, map);
	return bilist;
}

struct BiLinkedList *BiLinkedList_NewFromVector(const struct Vector *const v)
{
	if( !v )
		return NULL;
	struct BiLinkedList *bilist = BiLinkedList_New();
	BiLinkedList_FromVector(bilist, v);
	return bilist;
}

struct BiLinkedList *BiLinkedList_NewFromTuple(const struct Tuple *const tup)
{
	if( !tup || !tup->Items || !tup->Len )
		return NULL;
	struct BiLinkedList *bilist = BiLinkedList_New();
	BiLinkedList_FromTuple(bilist, tup);
	return bilist;
}

struct BiLinkedList *BiLinkedList_NewFromGraph(const struct Graph *const graph)
{
	if( !graph )
		return NULL;
	struct BiLinkedList *bilist = BiLinkedList_New();
	BiLinkedList_FromGraph(bilist, graph);
	return bilist;
}

struct BiLinkedList *BiLinkedList_NewFromLinkMap(const struct LinkMap *const map)
{
	if( !map )
		return NULL;
	struct BiLinkedList *bilist = BiLinkedList_New();
	BiLinkedList_FromLinkMap(bilist, map);
	return bilist;
}
