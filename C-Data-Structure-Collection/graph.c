
#include <stdlib.h>
#include <stdio.h>
#include "dsc.h"

/* Graph Edge Code */
struct GraphEdge *GraphEdge_New(void)
{
	return calloc(1, sizeof(struct GraphEdge));
}

struct GraphEdge *GraphEdge_NewVP(const union Value val, struct GraphVertex *const restrict vert)
{
	struct GraphEdge *edge = GraphEdge_New();
	if( edge ) {
		edge->Weight = val;
		edge->VertexSocket = vert;
	}
	return edge;
}
void GraphEdge_Del(struct GraphEdge *const restrict edge, bool (*edge_dtor)())
{
	if( !edge )
		return;
	
	if( edge_dtor )
		(*edge_dtor)(&edge->Weight.Ptr);
	if( edge->NextEdge )
		GraphEdge_Free(&edge->NextEdge, edge_dtor);
	*edge = (struct GraphEdge){0};
}

void GraphEdge_Free(struct GraphEdge **restrict edgeref, bool (*edge_dtor)())
{
	if( !*edgeref )
		return;
	
	GraphEdge_Del(*edgeref, edge_dtor);
	free(*edgeref);
	*edgeref=NULL;
}

union Value GraphEdge_GetWeight(const struct GraphEdge *const restrict edge)
{
	return edge ? edge->Weight : (union Value){0};
}

void GraphEdge_SetWeight(struct GraphEdge *const restrict edge, const union Value val)
{
	if( !edge )
		return;
	edge->Weight = val;
}

struct GraphVertex *GraphEdge_GetVertex(const struct GraphEdge *const restrict edge)
{
	return edge ? edge->VertexSocket : NULL;
}

void GraphEdge_SetVertex(struct GraphEdge *const restrict edge, struct GraphVertex *const vert)
{
	if( !edge )
		return;
	
	edge->VertexSocket = vert;
}
/**************************************/


/* Graph Vertex Code */
void GraphVertex_Del(struct GraphVertex *const restrict vert, bool (*edge_dtor)(), bool (*vert_dtor)())
{
	if( !vert )
		return;
	
	if( vert_dtor )
		(*vert_dtor)(&vert->Data.Ptr);
	
	GraphEdge_Free(&vert->EdgeHead, edge_dtor);
	*vert = (struct GraphVertex){0};
}

struct GraphEdge *GraphVertex_GetEdges(struct GraphVertex *const restrict vert)
{
	return vert ? vert->EdgeHead : NULL;
}

union Value GraphVertex_GetData(const struct GraphVertex *const restrict vert)
{
	return vert ? vert->Data : (union Value){0};
}

void GraphVertex_SetData(struct GraphVertex *const restrict vert, const union Value val)
{
	if( !vert )
		return;
	
	vert->Data = val;
}
/**************************************/


/* Graph Code Implementation */
struct Graph *Graph_New(bool (*edgedtor)(), bool (*vertdtor)())
{
	struct Graph *graph = calloc(1, sizeof *graph);
	Graph_SetItemDestructors(graph, edgedtor, vertdtor);
	return graph;
}

void Graph_Init(struct Graph *const restrict graph, bool (*edgedtor)(), bool (*vertdtor)())
{
	if( !graph )
		return;
	*graph = (struct Graph){0};
	Graph_SetItemDestructors(graph, edgedtor, vertdtor);
}

void Graph_Del(struct Graph *const restrict graph)
{
	if( !graph )
		return;
	
	for( size_t i=0 ; i<graph->VertexCount ; i++ )
		GraphVertex_Del(graph->Vertices+i, graph->EdgeDestructor, graph->VertexDestructor);
	
	free(graph->Vertices);
	Graph_Init(graph, graph->EdgeDestructor, graph->VertexDestructor);
}

void Graph_Free(struct Graph **restrict graphref)
{
	if( !*graphref )
		return;
	
	Graph_Del(*graphref);
	free(*graphref);
	*graphref=NULL;
}

bool Graph_InsertVertexByValue(struct Graph *const restrict graph, const union Value val)
{
	if( !graph )
		return false;
	else if( !graph->Vertices or graph->VertexCount >= graph->VertexLen )
		Graph_Resize(graph);
	
	graph->Vertices[graph->VertexCount++].Data = val;
	return true;
}

bool Graph_RemoveVertexByValue(struct Graph *const restrict graph, const union Value val)
{
	if( !graph )
		return false;
	
	struct GraphVertex *vert = NULL;
	size_t index=0;
	for( index=0 ; index<graph->VertexCount ; index++ ) {
		if( !vert and graph->Vertices[index].Data.UInt64==val.UInt64 )
			vert = graph->Vertices + index;
		// value exists, cut its links with other VertexCount.
		for( struct GraphEdge *edge=graph->Vertices[index].EdgeHead ; edge ; edge=edge->NextEdge ) {
			if( edge->VertexSocket == vert )
				// sever the link!
				edge->VertexSocket = NULL;
		}
	}
	if( vert ) {
		GraphVertex_Del(vert, graph->EdgeDestructor, graph->VertexDestructor);
		const size_t
			i=index+1,
			j=index
		;
		memmove(graph->Vertices+j, graph->Vertices+i, graph->VertexCount * sizeof *graph->Vertices);
		graph->VertexCount--;
		return true;
	}
	return false;
}

bool Graph_RemoveVertexByIndex(struct Graph *const restrict graph, const size_t index)
{
	if( !graph )
		return false;
	struct GraphVertex *vert = Graph_GetVertexByIndex(graph, index);
	if( !vert )
		return false;
	
	for( struct GraphEdge *edge=vert->EdgeHead ; edge ; edge=edge->NextEdge )
		edge->VertexSocket = NULL;
	
	for( size_t i=0 ; i<graph->VertexCount ; i++ ) {
		if( graph->Vertices+i == vert )
			continue;
		
		// cut links with other VertexCount.
		for( struct GraphEdge *edge=graph->Vertices[i].EdgeHead ; edge ; edge=edge->NextEdge ) {
			if( edge->VertexSocket == vert )
				// sever the link!
				edge->VertexSocket = NULL;
		}
	}
	
	GraphVertex_Del(vert, graph->EdgeDestructor, graph->VertexDestructor);
	size_t
		i=index+1,
		j=index
	;
	while( i<graph->VertexCount )
		graph->Vertices[j++] = graph->Vertices[i++];
	graph->VertexCount--;
	return true;
}

bool Graph_InsertEdgeBtwnVerts(struct Graph *const restrict graph, const size_t index, const size_t otherindex, const union Value weight)
{
	if( !graph )
		return false;
	
	struct GraphVertex *vert1 = Graph_GetVertexByIndex(graph, index);
	struct GraphVertex *vert2 = Graph_GetVertexByIndex(graph, otherindex);
	if( !vert1 or !vert2 )
		return false;
	
	struct GraphEdge *edge = GraphEdge_NewVP(weight, vert2);
	if( !edge )
		return false;
	
	if( vert1->EdgeLen ) {
		edge->NextEdge = NULL;
		vert1->EdgeTail->NextEdge = edge;
		vert1->EdgeTail = edge;
	}
	else vert1->EdgeHead = vert1->EdgeTail = edge;
	vert1->EdgeLen++;
	return true;
}

bool Graph_RemoveEdgeBtwnVerts(struct Graph *const restrict graph, const size_t index, const size_t otherindex)
{
	if( !graph )
		return false;
	
	struct GraphVertex *vert1 = Graph_GetVertexByIndex(graph, index);
	struct GraphVertex *vert2 = Graph_GetVertexByIndex(graph, otherindex);
	if( !vert1 or !vert2 )
		return false;
	
	// make sure the vertex #1 actually has a connection to vertex #2
	struct GraphEdge *n = NULL;
	for( n=vert1->EdgeHead ; n ; n=n->NextEdge ) {
		if( n->VertexSocket == vert2 )
			break;
	}
	// we found the connection between the two! let's break it!
	if( !n )
		return false;
	
	if( n==vert1->EdgeHead )
		vert1->EdgeHead = n->NextEdge;
	else {
		struct GraphEdge *travnode = vert1->EdgeHead;
		for( size_t i=0 ; i<vert1->EdgeLen ; i++ ) {
			if( travnode->NextEdge == n ) {
				if( vert1->EdgeTail == n ) {
					travnode->NextEdge = NULL;
					vert1->EdgeTail = travnode;
				}
				else travnode->NextEdge = n->NextEdge;
				break;
			}
			travnode = travnode->NextEdge;
		}
	}
	GraphEdge_Free(&n, graph->EdgeDestructor);
	vert1->EdgeLen--;
	return true;
}

struct GraphVertex *Graph_GetVertexByIndex(struct Graph *const restrict graph, const size_t index)
{
	if( !graph or !graph->Vertices or index >= graph->VertexCount )
		return NULL;
	
	return graph->Vertices + index;
}

union Value Graph_GetVertexDataByIndex(struct Graph *const restrict graph, const size_t index)
{
	if( !graph or !graph->Vertices or index>=graph->VertexCount )
		return (union Value){0};
	return graph->Vertices[index].Data;
}

void Graph_SetVertexDataByIndex(struct Graph *const restrict graph, const size_t index, const union Value val)
{
	if( !graph or !graph->Vertices or index>=graph->VertexCount )
		return;
	graph->Vertices[index].Data = val;
}

struct GraphEdge *Graph_GetEdgeBtwnVertices(struct Graph *const restrict graph, const size_t index, const size_t otherindex)
{
	if( !graph )
		return NULL;
	
	struct GraphVertex *vert1 = Graph_GetVertexByIndex(graph, index);
	struct GraphVertex *vert2 = Graph_GetVertexByIndex(graph, otherindex);
	if( !vert1 or !vert2 )
		return NULL;
	
	for( struct GraphEdge *edge=vert1->EdgeHead ; edge ; edge=edge->NextEdge )
		if( edge->VertexSocket==vert2 )
			return edge;
	
	return NULL;
}

bool Graph_IsVertexAdjacent(struct Graph *const restrict graph, const size_t index, const size_t otherindex)
{
	if( !graph )
		return false;
	
	struct GraphVertex *vert1 = Graph_GetVertexByIndex(graph, index);
	struct GraphVertex *vert2 = Graph_GetVertexByIndex(graph, otherindex);
	if( !vert1 or !vert2 )
		return false;
	
	for( struct GraphEdge *edge=vert1->EdgeHead ; edge ; edge=edge->NextEdge )
		if( edge->VertexSocket==vert2 )
			return true;
	return false;
}

struct GraphEdge *Graph_GetVertexNeighbors(struct Graph *const restrict graph, const size_t index)
{
	if( !graph or index >= graph->VertexCount )
		return NULL;
	
	return graph->Vertices[index].EdgeHead;
}

struct GraphVertex *Graph_GetVertexArray(struct Graph *const restrict graph)
{
	return graph ? graph->Vertices : NULL;
}

size_t Graph_GetVerticeCount(const struct Graph *const restrict graph)
{
	return graph ? graph->VertexCount : 0;
}

size_t Graph_GetEdgeCount(const struct Graph *const restrict graph)
{
	if( !graph )
		return 0;
	
	size_t totaledges = 0;
	for( size_t i=0 ; i<graph->VertexCount ; i++ )
		totaledges += graph->Vertices[i].EdgeLen;
	return totaledges;
}

void Graph_SetItemDestructors(struct Graph *const restrict graph, bool (*edgedtor)(), bool (*vertdtor)())
{
	if( !graph )
		return;
	
	graph->EdgeDestructor = edgedtor;
	graph->VertexDestructor = vertdtor;
}

void Graph_Resize(struct Graph *const restrict graph)
{
	if( !graph )
		return;
	
	size_t oldsize = graph->VertexLen;
	graph->VertexLen <<= 1;
	if( !graph->VertexLen )
		graph->VertexLen = 2;
	
	struct GraphVertex *newdata = calloc(graph->VertexLen, sizeof *newdata);
	if( !newdata ) {
		graph->VertexLen >>= 1;
		if( graph->VertexLen == 1 )
			graph->VertexLen=0;
		return;
	}
	
	if( graph->Vertices ) {
		memcpy(newdata, graph->Vertices, sizeof *newdata * oldsize);
		free(graph->Vertices);
		graph->Vertices = NULL;
	}
	graph->Vertices = newdata;
}

void Graph_Truncate(struct Graph *const restrict graph)
{
	if( !graph )
		return;
	
	if( graph->VertexCount < graph->VertexLen>>1 ) {
		graph->VertexLen >>= 1;
		struct GraphVertex *newdata = calloc(graph->VertexLen, sizeof *newdata);
		if( !newdata )
			return;
		
		if( graph->Vertices ) {
			memcpy(newdata, graph->Vertices, sizeof *newdata * graph->VertexLen);
			free(graph->Vertices);
			graph->Vertices = NULL;
		}
		graph->Vertices = newdata;
	}
}

void Graph_FromVector(struct Graph *const restrict graph, const struct Vector *const restrict vec)
{
	if( !graph or !vec )
		return;
	for( size_t i=0 ; i<vec->Count ; i++ )
		Graph_InsertVertexByValue(graph, vec->Table[i]);
}

void Graph_FromMap(struct Graph *const restrict graph, const struct Hashmap *const restrict map)
{
	if( !graph or !map )
		return;
	
	for( size_t i=0 ; i<map->Len ; i++ )
		for( struct KeyNode *n = map->Table[i] ; n ; n=n->Next )
			Graph_InsertVertexByValue(graph, n->Data);
}
void Graph_FromUniLinkedList(struct Graph *const restrict graph, const struct UniLinkedList *const restrict list)
{
	if( !graph or !list )
		return;
	
	for( struct UniListNode *n=list->Head ; n ; n=n->Next )
		Graph_InsertVertexByValue(graph, n->Data);
}
void Graph_FromBiLinkedList(struct Graph *const restrict graph, const struct BiLinkedList *const restrict list)
{
	if( !graph or !list )
		return;
	
	for( struct BiListNode *n=list->Head ; n ; n=n->Next )
		Graph_InsertVertexByValue(graph, n->Data);
}
void Graph_FromTuple(struct Graph *const restrict graph, const struct Tuple *const restrict tup)
{
	if( !graph or !tup )
		return;
	for( size_t i=0 ; i<tup->Len ; i++ )
		Graph_InsertVertexByValue(graph, tup->Items[i]);
}
void Graph_FromLinkMap(struct Graph *const restrict graph, const struct LinkMap *const restrict map)
{
	if( !graph or !map )
		return;
	
	for( struct LinkNode *n=map->Head ; n ; n=n->After )
		Graph_InsertVertexByValue(graph, n->Data);
}


struct Graph *Graph_NewFromVector(const struct Vector *const restrict vec)
{
	if( !vec )
		return NULL;
	struct Graph *graph = Graph_New(NULL, vec->Destructor);
	Graph_FromVector(graph, vec);
	return graph;
}

struct Graph *Graph_NewFromMap(const struct Hashmap *const restrict map)
{
	if( !map )
		return NULL;
	struct Graph *graph = Graph_New(NULL, map->Destructor);
	Graph_FromMap(graph, map);
	return graph;
}
struct Graph *Graph_NewFromUniLinkedList(const struct UniLinkedList *const restrict list)
{
	if( !list )
		return NULL;
	struct Graph *graph = Graph_New(NULL, list->Destructor);
	Graph_FromUniLinkedList(graph, list);
	return graph;
}
struct Graph *Graph_NewFromBiLinkedList(const struct BiLinkedList *const restrict list)
{
	if( !list )
		return NULL;
	struct Graph *graph = Graph_New(NULL, list->Destructor);
	Graph_FromBiLinkedList(graph, list);
	return graph;
}
struct Graph *Graph_NewFromTuple(const struct Tuple *const restrict tup)
{
	if( !tup )
		return NULL;
	struct Graph *graph = Graph_New(NULL, NULL);
	Graph_FromTuple(graph, tup);
	return graph;
}
struct Graph *Graph_NewFromLinkMap(const struct LinkMap *const restrict map)
{
	if( !map )
		return NULL;
	struct Graph *graph = Graph_New(NULL, map->Destructor);
	Graph_FromLinkMap(graph, map);
	return graph;
}
/**************************************/
