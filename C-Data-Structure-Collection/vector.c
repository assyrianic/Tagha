
#include <stdlib.h>
#include <stdio.h>
#include "dsc.h"

/*
struct Vector {
	union Value *Table;
	size_t	Len, Count;
};
*/

struct Vector *Vector_New(bool (*dtor)())
{
	struct Vector *v = calloc(1, sizeof(struct Vector));
	Vector_SetItemDestructor(v, dtor);
	return v;
}

void Vector_Init(struct Vector *const restrict v, bool (*dtor)())
{
	if( !v )
		return;
	
	*v = (struct Vector){0};
	Vector_SetItemDestructor(v, dtor);
}

void Vector_Del(struct Vector *const restrict v)
{
	if( !v or !v->Table )
		return;
	
	for( size_t i=0 ; i<v->Len ; i++ ) {
		if( v->Destructor )
			(*v->Destructor)(&v->Table[i].Ptr);
	}
	free(v->Table);
	Vector_Init(v, v->Destructor);
}

void Vector_Free(struct Vector **restrict vecref)
{
	if( !*vecref )
		return;
	
	Vector_Del(*vecref);
	free(*vecref);
	*vecref=NULL;
}

inline size_t Vector_Len(const struct Vector *const restrict v)
{
	return v and v->Table ? v->Len : 0;
}

inline size_t Vector_Count(const struct Vector *const restrict v)
{
	return v and v->Table ? v->Count : 0;
}
union Value *Vector_GetTable(const struct Vector *const restrict v)
{
	return v ? v->Table : NULL;
}

void Vector_Resize(struct Vector *const restrict v)
{
	if( !v )
		return;
	
	// first we get our old size.
	// then resize the actual size.
	size_t oldsize = v->Len;
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
		free(v->Table);
		v->Table = NULL;
	}
	v->Table = newdata;
}

void Vector_Truncate(struct Vector *const restrict v)
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
			free(v->Table);
			v->Table = NULL;
		}
		v->Table = newdata;
	}
}


bool Vector_Insert(struct Vector *const restrict v, const union Value val)
{
	if( !v )
		return false;
	else if( !v->Table or v->Count >= v->Len )
		Vector_Resize(v);
	
	v->Table[v->Count++] = val;
	return true;
}

union Value Vector_Get(const struct Vector *const restrict v, const size_t index)
{
	if( !v or !v->Table or index >= v->Count )
		return (union Value){0};
	
	return v->Table[index];
}

void Vector_Set(struct Vector *const restrict v, const size_t index, const union Value val)
{
	if( !v or !v->Table or index >= v->Count )
		return;
	
	v->Table[index] = val;
}

void Vector_SetItemDestructor(struct Vector *const restrict v, bool (*dtor)())
{
	if( !v )
		return;
	v->Destructor = dtor;
}

void Vector_Delete(struct Vector *const restrict v, const size_t index)
{
	if( !v or !v->Table or index >= v->Count )
		return;
	
	if( v->Destructor )
		(*v->Destructor)(&v->Table[index].Ptr);
	
	size_t
		i=index+1,
		j=index
	;
	while( i<v->Count )
		v->Table[j++] = v->Table[i++];
	v->Count--;
	// can't keep auto-truncating, allocating memory every time can be expensive.
	// I'll let the programmers truncate whenever they need to.
	//Vector_Truncate(v);
}

void Vector_Add(struct Vector *const restrict vA, const struct Vector *const restrict vB)
{
	if( !vA or !vB or !vB->Table )
		return;
	
	size_t i=0;
	while( i<vB->Count ) {
		if( !vA->Table or vA->Count >= vA->Len )
			Vector_Resize(vA);
		vA->Table[vA->Count++] = vB->Table[i++];
	}
}

void Vector_Copy(struct Vector *const restrict vA, const struct Vector *const restrict vB)
{
	if( !vA or !vB or !vB->Table )
		return;
	
	Vector_Del(vA);
	size_t i=0;
	while( i<vB->Count ) {
		if( !vA->Table or vA->Count >= vA->Len )
			Vector_Resize(vA);
		vA->Table[vA->Count++] = vB->Table[i++];
	}
}

void Vector_FromUniLinkedList(struct Vector *const restrict v, const struct UniLinkedList *const restrict list)
{
	if( !v or !list )
		return;
	else if( !v->Table or v->Count+list->Len >= v->Len )
		while( v->Count+list->Len >= v->Len )
			Vector_Resize(v);
	
	for( struct UniListNode *n=list->Head ; n ; n = n->Next )
		v->Table[v->Count++] = n->Data;
}

void Vector_FromBiLinkedList(struct Vector *const restrict v, const struct BiLinkedList *const restrict list)
{
	if( !v or !list )
		return;
	else if( !v->Table or v->Count+list->Len >= v->Len )
		while( v->Count+list->Len >= v->Len )
			Vector_Resize(v);
	
	for( struct BiListNode *n=list->Head ; n ; n = n->Next )
		v->Table[v->Count++] = n->Data;
}

void Vector_FromMap(struct Vector *const restrict v, const struct Hashmap *const restrict map)
{
	if( !v or !map )
		return;
	else if( !v->Table or v->Count+map->Count >= v->Len )
		while( v->Count+map->Count >= v->Len )
			Vector_Resize(v);
	
	for( size_t i=0 ; i<map->Len ; i++ )
		for( struct KeyNode *n = map->Table[i] ; n ; n=n->Next )
			v->Table[v->Count++] = n->Data;
}

void Vector_FromTuple(struct Vector *const restrict v, const struct Tuple *const restrict tup)
{
	if( !v or !tup or !tup->Items or !tup->Len )
		return;
	else if( !v->Table or v->Count+tup->Len >= v->Len )
		while( v->Count+tup->Len >= v->Len )
			Vector_Resize(v);
	
	size_t i=0;
	while( i<tup->Len )
		v->Table[v->Count++] = tup->Items[i++];
}

void Vector_FromGraph(struct Vector *const restrict v, const struct Graph *const restrict graph)
{
	if( !v or !graph )
		return;
	else if( !v->Table or v->Count+graph->VertexCount >= v->Len )
		while( v->Count+graph->VertexCount >= v->Len )
			Vector_Resize(v);
	
	size_t i=0;
	while( i<graph->VertexCount )
		v->Table[v->Count++] = graph->Vertices[i++].Data;
}

void Vector_FromLinkMap(struct Vector *const restrict v, const struct LinkMap *const restrict map)
{
	if( !v or !map )
		return;
	else if( !v->Table or v->Count+map->Count >= v->Len )
		while( v->Count+map->Count >= v->Len )
			Vector_Resize(v);
	
	for( struct LinkNode *n = map->Head ; n ; n=n->After )
		v->Table[v->Count++] = n->Data;
}

struct Vector *Vector_NewFromUniLinkedList(const struct UniLinkedList *const restrict list)
{
	if( !list )
		return NULL;
	struct Vector *v = Vector_New(list->Destructor);
	Vector_FromUniLinkedList(v, list);
	return v;
}

struct Vector *Vector_NewFromBiLinkedList(const struct BiLinkedList *const restrict list)
{
	if( !list )
		return NULL;
	struct Vector *v = Vector_New(list->Destructor);
	Vector_FromBiLinkedList(v, list);
	return v;
}

struct Vector *Vector_NewFromMap(const struct Hashmap *const restrict map)
{
	if( !map )
		return NULL;
	struct Vector *v = Vector_New(map->Destructor);
	Vector_FromMap(v, map);
	return v;
}

struct Vector *Vector_NewFromTuple(const struct Tuple *const restrict tup)
{
	if( !tup )
		return NULL;
	struct Vector *v = Vector_New(NULL);
	Vector_FromTuple(v, tup);
	return v;
}

struct Vector *Vector_NewFromGraph(const struct Graph *const restrict graph)
{
	if( !graph )
		return NULL;
	struct Vector *v = Vector_New(graph->VertexDestructor);
	Vector_FromGraph(v, graph);
	return v;
}

struct Vector *Vector_NewFromLinkMap(const struct LinkMap *const restrict map)
{
	if( !map )
		return NULL;
	struct Vector *v = Vector_New(map->Destructor);
	Vector_FromLinkMap(v, map);
	return v;
}
