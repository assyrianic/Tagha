
#include <stdlib.h>
#include <stdio.h>
#include "dsc.h"

struct Tuple *Tuple_New(const size_t arrlen, union Value items[static arrlen])
{
	struct Tuple *tup = calloc(1, sizeof *tup);
	if( tup )
		Tuple_Init(tup, arrlen, items);
	return tup;
}

void Tuple_Free(struct Tuple **tupref)
{
	if( !*tupref )
		return;
	Tuple_Del(*tupref);
	free(*tupref); *tupref=NULL;
}

void Tuple_Init(struct Tuple *const tup, const size_t arrlen, union Value items[static arrlen])
{
	if( !tup )
		return;
	*tup = (struct Tuple){0};
	tup->Items = calloc(arrlen, sizeof *tup->Items);
	if( !tup->Items )
		return;
	memcpy(tup->Items, items, sizeof *tup->Items * arrlen);
	tup->Len = arrlen;
}

void Tuple_Del(struct Tuple *const tup)
{
	if( !tup or !tup->Items )
		return;
	free(tup->Items); tup->Items=NULL;
	*tup = (struct Tuple){0};
}

size_t Tuple_Len(const struct Tuple *const tup)
{
	return tup ? tup->Len : 0 ;
}

union Value *Tuple_GetItems(const struct Tuple *const tup)
{
	return tup ? tup->Items : NULL ;
}

union Value Tuple_GetItem(const struct Tuple *const tup, const size_t index)
{
	if( !tup or !tup->Items or index >= tup->Len )
		return (union Value){0};
	
	return tup->Items[index];
}

void Tuple_FromUniLinkedList(struct Tuple *const tup, const struct UniLinkedList *const list)
{
	if( !tup or !list )
		return;
	
	if( tup->Items )
		Tuple_Del(tup);
	
	union Value list_items[list->Len];
	size_t i=0;
	for( struct UniListNode *n=list->Head ; n ; n=n->Next )
		list_items[i++] = n->Data;
	
	Tuple_Init(tup, i, list_items);
}

void Tuple_FromMap(struct Tuple *const tup, const struct Hashmap *const map)
{
	if( !tup or !map )
		return;
	
	if( tup->Items )
		Tuple_Del(tup);
	
	union Value list_items[map->Count];
	size_t x=0;
	for( size_t i=0 ; i<map->Len ; i++ ) {
		struct Vector *vec = map->Table + i;
		for( size_t n=0 ; n<Vector_Count(vec) ; n++ ) {
			struct KeyValPair *node = vec->Table[n].Ptr;
			list_items[x++] = node->Data;
		}
	}
	Tuple_Init(tup, x, list_items);
}

void Tuple_FromVector(struct Tuple *const tup, const struct Vector *const vec)
{
	if( !tup or !vec )
		return;
	
	if( tup->Items )
		Tuple_Del(tup);
	
	Tuple_Init(tup, vec->Count, vec->Table);
}

void Tuple_FromBiLinkedList(struct Tuple *const tup, const struct BiLinkedList *const list)
{
	if( !tup or !list )
		return;
	
	if( tup->Items )
		Tuple_Del(tup);
	
	union Value list_items[list->Len];
	size_t i=0;
	for( struct BiListNode *n=list->Head ; n ; n=n->Next )
		list_items[i++] = n->Data;
	
	Tuple_Init(tup, i, list_items);
}

void Tuple_FromGraph(struct Tuple *const tup, const struct Graph *const graph)
{
	if( !tup or !graph )
		return;
	
	if( tup->Items )
		Tuple_Del(tup);
	
	union Value list_items[graph->Vertices.Count];
	for( size_t i=0 ; i<graph->Vertices.Count ; i++ ) {
		struct GraphVertex *vert = graph->Vertices.Table[i].Ptr;
		list_items[i] = vert->Data;
	}
	Tuple_Init(tup, graph->Vertices.Count, list_items);
}

void Tuple_FromLinkMap(struct Tuple *const tup, const struct LinkMap *const map)
{
	if( tup->Items )
		Tuple_Del(tup);
	
	union Value list_items[map->Count];
	size_t i=0;
	for( ; i<map->Order.Count ; i++ ) {
		struct KeyValPair *n = map->Order.Table[i].Ptr;
		list_items[i] = n->Data;
	}
	Tuple_Init(tup, i, list_items);
}


struct Tuple *Tuple_NewFromUniLinkedList(const struct UniLinkedList *const list)
{
	if( !list )
		return NULL;
	struct Tuple *const tup = calloc(1, sizeof *tup);
	Tuple_FromUniLinkedList(tup, list);
	return tup;
}
struct Tuple *Tuple_NewFromMap(const struct Hashmap *const map)
{
	if( !map )
		return NULL;
	struct Tuple *const tup = calloc(1, sizeof *tup);
	Tuple_FromMap(tup, map);
	return tup;
}
struct Tuple *Tuple_NewFromVector(const struct Vector *const vec)
{
	if( !vec )
		return NULL;
	struct Tuple *const tup = calloc(1, sizeof *tup);
	Tuple_FromVector(tup, vec);
	return tup;
}
struct Tuple *Tuple_NewFromBiLinkedList(const struct BiLinkedList *const list)
{
	if( !list )
		return NULL;
	struct Tuple *const tup = calloc(1, sizeof *tup);
	Tuple_FromBiLinkedList(tup, list);
	return tup;
}
struct Tuple *Tuple_NewFromGraph(const struct Graph *const graph)
{
	if( !graph )
		return NULL;
	struct Tuple *const tup = calloc(1, sizeof *tup);
	Tuple_FromGraph(tup, graph);
	return tup;
}
struct Tuple *Tuple_NewFromLinkMap(const struct LinkMap *const map)
{
	if( !map )
		return NULL;
	struct Tuple *const tup = calloc(1, sizeof *tup);
	Tuple_FromLinkMap(tup, map);
	return tup;
}
