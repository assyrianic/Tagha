
#include <stdlib.h>
#include <stdio.h>
#include "dsc.h"

/*
struct Vector {
	union Value *Table;
	size_t	Len, Count;
};
*/

struct Vector *Vector_New(void)
{
	struct Vector *v = calloc(1, sizeof *v);
	return v;
}

void Vector_Init(struct Vector *const v)
{
	if( !v )
		return;
	
	*v = (struct Vector){0};
}

void Vector_Del(struct Vector *const v, fnDestructor *const dtor)
{
	if( !v || !v->Table )
		return;
	
	if( dtor )
		for( size_t i=0 ; i<v->Len ; i++ )
			(*dtor)(&v->Table[i].Ptr);
	
	free(v->Table);
	Vector_Init(v);
}

void Vector_Free(struct Vector **vecref, fnDestructor *const dtor)
{
	if( !*vecref )
		return;
	
	Vector_Del(*vecref, dtor);
	free(*vecref), *vecref=NULL;
}

inline size_t Vector_Len(const struct Vector *const v)
{
	return v ? v->Len : 0;
}

inline size_t Vector_Count(const struct Vector *const v)
{
	return v && v->Table ? v->Count : 0;
}
inline union Value *Vector_GetTable(const struct Vector *const v)
{
	return v ? v->Table : NULL;
}

void Vector_Resize(struct Vector *const v)
{
	if( !v )
		return;
	
	// first we get our old size.
	// then resize the actual size.
	const size_t oldsize = v->Len;
	v->Len <<= 1;
	if( !v->Len )
		v->Len = 4;
	
	// allocate new table.
	union Value *newdata = calloc(v->Len, sizeof *newdata);
	if( !newdata ) {
		v->Len >>= 1;
		if( v->Len == 1 )
			v->Len=0;
		return;
	}
	
	// copy the old table to new then free old table.
	if( v->Table ) {
		memcpy(newdata, v->Table, sizeof *newdata * oldsize);
		free(v->Table), v->Table = NULL;
	}
	v->Table = newdata;
}

void Vector_Truncate(struct Vector *const v)
{
	if( !v )
		return;
	
	if( v->Count < v->Len>>1 ) {
		v->Len >>= 1;
		// allocate new table.
		union Value *newdata = calloc(v->Len, sizeof *newdata);
		if( !newdata )
			return;
		
		// copy the old table to new then free old table.
		if( v->Table ) {
			memcpy(newdata, v->Table, sizeof *newdata * v->Len);
			free(v->Table), v->Table = NULL;
		}
		v->Table = newdata;
	}
}


bool Vector_Insert(struct Vector *const v, const union Value val)
{
	if( !v )
		return false;
	else if( !v->Table || v->Count >= v->Len )
		Vector_Resize(v);
	
	v->Table[v->Count++] = val;
	return true;
}

union Value Vector_Pop(struct Vector *const v)
{
	return ( !v || !v->Table || !v->Count ) ? (union Value){0} : v->Table[--v->Count];
}

union Value Vector_Get(const struct Vector *const v, const size_t index)
{
	return (!v || !v->Table || index >= v->Count) ? (union Value){0} : v->Table[index];
}

void Vector_Set(struct Vector *const v, const size_t index, const union Value val)
{
	if( !v || !v->Table || index >= v->Count )
		return;
	
	v->Table[index] = val;
}

void Vector_Delete(struct Vector *const v, const size_t index, fnDestructor *const dtor)
{
	if( !v || !v->Table || index >= v->Count )
		return;
	
	if( dtor )
		(*dtor)(&v->Table[index].Ptr);
	
	size_t
		i=index+1,
		j=index
	;
	//while( i<v->Count )
	//	v->Table[j++] = v->Table[i++];
	memmove(v->Table+j, v->Table+i, (v->Len-j) * sizeof *v->Table);
	v->Count--;
	// can't keep auto-truncating, allocating memory every time can be expensive.
	// I'll let the programmers truncate whenever they need to.
	//Vector_Truncate(v);
}

void Vector_Add(struct Vector *const restrict vA, const struct Vector *const restrict vB)
{
	if( !vA || !vB || !vB->Table )
		return;
	
	size_t i=0;
	while( i<vB->Count ) {
		if( !vA->Table || vA->Count >= vA->Len )
			Vector_Resize(vA);
		vA->Table[vA->Count++] = vB->Table[i++];
	}
}

void Vector_Copy(struct Vector *const restrict vA, const struct Vector *const restrict vB)
{
	if( !vA || !vB || !vB->Table )
		return;
	
	Vector_Del(vA, NULL);
	size_t i=0;
	while( i<vB->Count ) {
		if( !vA->Table || vA->Count >= vA->Len )
			Vector_Resize(vA);
		vA->Table[vA->Count++] = vB->Table[i++];
	}
}

void Vector_FromUniLinkedList(struct Vector *const v, const struct UniLinkedList *const list)
{
	if( !v || !list )
		return;
	else if( !v->Table || v->Count+list->Len >= v->Len )
		while( v->Count+list->Len >= v->Len )
			Vector_Resize(v);
	
	for( struct UniListNode *n=list->Head ; n ; n = n->Next )
		v->Table[v->Count++] = n->Data;
}

void Vector_FromBiLinkedList(struct Vector *const v, const struct BiLinkedList *const list)
{
	if( !v || !list )
		return;
	else if( !v->Table || v->Count+list->Len >= v->Len )
		while( v->Count+list->Len >= v->Len )
			Vector_Resize(v);
	
	for( struct BiListNode *n=list->Head ; n ; n = n->Next )
		v->Table[v->Count++] = n->Data;
}

void Vector_FromMap(struct Vector *const restrict v, const struct Hashmap *const map)
{
	if( !v || !map )
		return;
	else if( !v->Table || v->Count+map->Count >= v->Len )
		while( v->Count+map->Count >= v->Len )
			Vector_Resize(v);
	
	for( size_t i=0 ; i<map->Len ; i++ ) {
		struct Vector *restrict vec = map->Table + i;
		for( size_t n=0 ; n<Vector_Count(vec) ; n++ ) {
			struct KeyNode *node = vec->Table[n].Ptr;
			v->Table[v->Count++] = node->Data;
		}
	}
}

void Vector_FromTuple(struct Vector *const v, const struct Tuple *const tup)
{
	if( !v || !tup || !tup->Items || !tup->Len )
		return;
	else if( !v->Table || v->Count+tup->Len >= v->Len )
		while( v->Count+tup->Len >= v->Len )
			Vector_Resize(v);
	
	size_t i=0;
	while( i<tup->Len )
		v->Table[v->Count++] = tup->Items[i++];
}

void Vector_FromGraph(struct Vector *const v, const struct Graph *const graph)
{
	if( !v || !graph )
		return;
	else if( !v->Table || v->Count+graph->Vertices.Count >= v->Len )
		while( v->Count+graph->Vertices.Count >= v->Len )
			Vector_Resize(v);
	
	for( size_t i=0 ; i<graph->Vertices.Count ; i++ ) {
		struct GraphVertex *vert = graph->Vertices.Table[i].Ptr;
		v->Table[v->Count++] = vert->Data;
	}
}

void Vector_FromLinkMap(struct Vector *const v, const struct LinkMap *const map)
{
	if( !v || !map )
		return;
	else if( !v->Table || v->Count+map->Count >= v->Len )
		while( v->Count+map->Count >= v->Len )
			Vector_Resize(v);
	
	for( size_t i=0 ; i<map->Order.Count ; i++ ) {
		struct KeyNode *n = map->Order.Table[i].Ptr;
		v->Table[v->Count++] = n->Data;
	}
}

struct Vector *Vector_NewFromUniLinkedList(const struct UniLinkedList *const list)
{
	if( !list )
		return NULL;
	struct Vector *v = Vector_New();
	Vector_FromUniLinkedList(v, list);
	return v;
}

struct Vector *Vector_NewFromBiLinkedList(const struct BiLinkedList *const list)
{
	if( !list )
		return NULL;
	struct Vector *v = Vector_New();
	Vector_FromBiLinkedList(v, list);
	return v;
}

struct Vector *Vector_NewFromMap(const struct Hashmap *const map)
{
	if( !map )
		return NULL;
	struct Vector *v = Vector_New();
	Vector_FromMap(v, map);
	return v;
}

struct Vector *Vector_NewFromTuple(const struct Tuple *const tup)
{
	if( !tup )
		return NULL;
	struct Vector *v = Vector_New();
	Vector_FromTuple(v, tup);
	return v;
}

struct Vector *Vector_NewFromGraph(const struct Graph *const graph)
{
	if( !graph )
		return NULL;
	struct Vector *v = Vector_New();
	Vector_FromGraph(v, graph);
	return v;
}

struct Vector *Vector_NewFromLinkMap(const struct LinkMap *const map)
{
	if( !map )
		return NULL;
	struct Vector *v = Vector_New();
	Vector_FromLinkMap(v, map);
	return v;
}
