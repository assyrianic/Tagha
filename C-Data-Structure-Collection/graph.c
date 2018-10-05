
#include <stdlib.h>
#include <stdio.h>
#include "dsc.h"

/* Graph Edge Code */
struct GraphEdge *GraphEdge_New(void)
{
	return calloc(1, sizeof(struct GraphEdge));
}

struct GraphEdge *GraphEdge_NewVP(const union Value val, struct GraphVertex *const vert)
{
	struct GraphEdge *edge = GraphEdge_New();
	if( edge ) {
		edge->Weight = val;
		edge->VertexSocket = vert;
	}
	return edge;
}

void GraphEdge_Del(struct GraphEdge *const edge, fnDestructor *const edge_dtor)
{
	if( !edge )
		return;
	
	if( edge_dtor )
		(*edge_dtor)(&edge->Weight.Ptr);
	
	*edge = (struct GraphEdge){0};
}

void GraphEdge_Free(struct GraphEdge **edgeref, fnDestructor *const edge_dtor)
{
	if( !*edgeref )
		return;
	
	GraphEdge_Del(*edgeref, edge_dtor);
	free(*edgeref), *edgeref=NULL;
}

union Value GraphEdge_GetWeight(const struct GraphEdge *const edge)
{
	return edge ? edge->Weight : (union Value){0};
}

void GraphEdge_SetWeight(struct GraphEdge *const edge, const union Value val)
{
	if( !edge )
		return;
	edge->Weight = val;
}

struct GraphVertex *GraphEdge_GetVertex(const struct GraphEdge *const edge)
{
	return edge ? edge->VertexSocket : NULL;
}

void GraphEdge_SetVertex(struct GraphEdge *const edge, struct GraphVertex *const vert)
{
	if( !edge )
		return;
	
	edge->VertexSocket = vert;
}
/**************************************/


/* Graph Vertex Code */
struct GraphVertex *GraphVertex_New(const union Value val)
{
	struct GraphVertex *vert = calloc(1, sizeof *vert);
	GraphVertex_Init(vert, val);
	return vert;
}

void GraphVertex_Init(struct GraphVertex *const vert, const union Value val)
{
	if( !vert )
		return;
	
	vert->Data = val;
}

void GraphVertex_Del(struct GraphVertex *const vert, fnDestructor *const edge_dtor, fnDestructor *const vert_dtor)
{
	if( !vert )
		return;
	
	if( vert_dtor )
		(*vert_dtor)(&vert->Data.Ptr);
	
	for( size_t i=0 ; i<vert->Edges.Count ; i++ ) {
		struct GraphEdge *edge = vert->Edges.Table[i].Ptr;
		GraphEdge_Free(&edge, edge_dtor);
	}
	Vector_Del(&vert->Edges, NULL);
	*vert = (struct GraphVertex){0};
}

void GraphVertex_Free(struct GraphVertex **vertref, fnDestructor *const edge_dtor, fnDestructor *const vert_dtor)
{
	if( !vertref || !*vertref )
		return;
	
	GraphVertex_Del(*vertref, edge_dtor, vert_dtor);
	free(*vertref), *vertref=NULL;
}

bool GraphVertex_AddEdge(struct GraphVertex *const vert, struct GraphEdge *const edge)
{
	return !vert || !edge ? false : Vector_Insert(&vert->Edges, (union Value){.Ptr=edge});
}

struct Vector GraphVertex_GetEdges(struct GraphVertex *const vert)
{
	return vert ? vert->Edges : (struct Vector){0};
}

union Value GraphVertex_GetData(const struct GraphVertex *const vert)
{
	return vert ? vert->Data : (union Value){0};
}

void GraphVertex_SetData(struct GraphVertex *const vert, const union Value val)
{
	if( !vert )
		return;
	
	vert->Data = val;
}
/**************************************/


/* Graph Code Implementation */
struct Graph *Graph_New(void)
{
	struct Graph *graph = calloc(1, sizeof *graph);
	return graph;
}

void Graph_Init(struct Graph *const graph)
{
	if( !graph )
		return;
	*graph = (struct Graph){0};
}

void Graph_Del(struct Graph *const graph, fnDestructor *const edge_dtor, fnDestructor *const vert_dtor)
{
	if( !graph )
		return;
	
	for( size_t i=0 ; i<graph->Count ; i++ ) {
		struct GraphVertex *vertex = graph->Table[i].Ptr;
		GraphVertex_Free(&vertex, edge_dtor, vert_dtor);
	}
	Vector_Del(&graph->Vertices, NULL);
}

void Graph_Free(struct Graph **graphref, fnDestructor *const edge_dtor, fnDestructor *const vert_dtor)
{
	if( !*graphref )
		return;
	
	Graph_Del(*graphref, edge_dtor, vert_dtor);
	free(*graphref), *graphref=NULL;
}

bool Graph_InsertVertexByValue(struct Graph *const graph, const union Value val)
{
	if( !graph )
		return false;
	
	struct GraphVertex *vert = GraphVertex_New(val);
	return !vert ? false : Vector_Insert(&graph->Vertices, (union Value){.Ptr=vert});
}

bool Graph_RemoveVertexByValue(struct Graph *const graph, const union Value val, fnDestructor *const edge_dtor, fnDestructor *const vert_dtor)
{
	if( !graph )
		return false;
	
	size_t index=0;
	struct GraphVertex *vert = NULL;
	while( !vert && index<graph->Count ) {
		struct GraphVertex *iter = graph->Table[index].Ptr;
		if( iter->Data.UInt64==val.UInt64 ) {
			vert = iter;
			break;
		}
		index++;
	}
	
	if( !vert )
		return true;
	
	// value exists, cut its links with other vertices.
	for( size_t i=0 ; i<graph->Count ; i++ ) {
		struct GraphVertex *iter = graph->Table[i].Ptr;
		if( iter==vert )
			continue;
		
		for( size_t n=0 ; n<iter->Edges.Count ; n++ ) {
			struct GraphEdge *edge = iter->Edges.Table[n].Ptr;
			if( edge->VertexSocket == vert )
				edge->VertexSocket = NULL;
		}
	}
	GraphVertex_Free(&vert, edge_dtor, vert_dtor);
	Vector_Delete(&graph->Vertices, index, NULL);
	return false;
}

bool Graph_RemoveVertexByIndex(struct Graph *const graph, const size_t index, fnDestructor *const edge_dtor, fnDestructor *const vert_dtor)
{
	if( !graph )
		return false;
	
	struct GraphVertex *vert = Graph_GetVertexByIndex(graph, index);
	if( !vert )
		return false;
	
	// cut its links with other vertices.
	for( size_t i=0 ; i<graph->Count ; i++ ) {
		if( i==index )
			continue;
		
		struct GraphVertex *restrict iter = graph->Table[i].Ptr;
		if( !iter )
			continue;
		
		for( size_t n=0 ; n<iter->Count ; n++ ) {
			struct GraphEdge *edge = iter->Table[n].Ptr;
			if( edge->VertexSocket==vert ) {
				edge->VertexSocket = NULL;
				GraphEdge_Free(&edge, edge_dtor);
				Vector_Delete(&iter->Edges, n, NULL);
				break;
			}
		}
	}
	GraphVertex_Free(&vert, edge_dtor, vert_dtor);
	Vector_Delete(&graph->Vertices, index, NULL);
	return true;
}

bool Graph_InsertEdgeBtwnVerts(struct Graph *const graph, const size_t index1, const size_t index2, const union Value weight)
{
	if( !graph )
		return false;
	
	struct GraphVertex *const restrict vert1 = Graph_GetVertexByIndex(graph, index1);
	struct GraphVertex *const restrict vert2 = Graph_GetVertexByIndex(graph, index2);
	if( !vert1 || !vert2 || vert1==vert2 )
		return false;
	
	struct GraphEdge *edge = GraphEdge_NewVP(weight, vert2);
	return ( !edge ) ? false : GraphVertex_AddEdge(vert1, edge);
}


bool Graph_RemoveEdgeBtwnVerts(struct Graph *const graph, const size_t index1, const size_t index2, fnDestructor *const edge_dtor)
{
	if( !graph )
		return false;
	
	struct GraphVertex *const restrict vert1 = Graph_GetVertexByIndex(graph, index1);
	struct GraphVertex *const restrict vert2 = Graph_GetVertexByIndex(graph, index2);
	if( !vert1 || !vert2 || vert1==vert2 )
		return false;
	
	// make sure the vertex #1 actually has a connection to vertex #2
	struct GraphEdge *edge = NULL;
	size_t n = 0;
	for( ; n<vert1->Edges.Count ; n++ ) {
		struct GraphEdge *e = vert1->Edges.Table[n].Ptr;
		if( e->VertexSocket == vert2 ) {
			edge = e;
			break;
		}
	}
	if( !edge )
		return false;
	
	GraphEdge_Free(&edge, edge_dtor);
	Vector_Delete(&vert1->Edges, n, NULL);
	return true;
}

struct GraphVertex *Graph_GetVertexByIndex(struct Graph *const graph, const size_t index)
{
	return !graph || index >= graph->Count ? NULL : graph->Table[index].Ptr;
}

union Value Graph_GetVertexDataByIndex(struct Graph *const graph, const size_t index)
{
	if( !graph || !graph->Table )
		return (union Value){0};
	
	struct GraphVertex *vertex = graph->Table[index].Ptr;
	return !vertex ? (union Value){0} : vertex->Data;
}

void Graph_SetVertexDataByIndex(struct Graph *const graph, const size_t index, const union Value val)
{
	if( !graph || !graph->Table )
		return;
	
	struct GraphVertex *vertex = graph->Table[index].Ptr;
	if( vertex )
		vertex->Data = val;
}

struct GraphEdge *Graph_GetEdgeBtwnVertices(struct Graph *const graph, const size_t index1, const size_t index2)
{
	if( !graph )
		return NULL;
	
	struct GraphVertex *const restrict vert1 = Graph_GetVertexByIndex(graph, index1);
	struct GraphVertex *const restrict vert2 = Graph_GetVertexByIndex(graph, index2);
	if( !vert1 || !vert2 || vert1==vert2 )
		return NULL;
	
	for( size_t i=0 ; i<vert1->Edges.Count ; i++ ) {
		struct GraphEdge *edge = vert1->Edges.Table[i].Ptr;
		if( edge->VertexSocket==vert2 )
			return edge;
	}
	return NULL;
}

bool Graph_IsVertexAdjacent(struct Graph *const graph, const size_t index1, const size_t index2)
{
	if( !graph )
		return false;
	
	struct GraphVertex *const restrict vert1 = Graph_GetVertexByIndex(graph, index1);
	struct GraphVertex *const restrict vert2 = Graph_GetVertexByIndex(graph, index2);
	if( !vert1 || !vert2 || vert1==vert2 )
		return false;
	
	for( size_t i=0 ; i<vert1->Edges.Count ; i++ ) {
		struct GraphEdge *edge = vert1->Edges.Table[i].Ptr;
		if( edge->VertexSocket==vert2 )
			return true;
	}
	return false;
}

struct Vector Graph_GetVertexNeighbors(struct Graph *const graph, const size_t index)
{
	return !graph || index >= graph->Count ? (struct Vector){0} : GraphVertex_GetEdges(graph->Table[index].Ptr);
}

struct Vector Graph_GetVertices(struct Graph *const graph)
{
	return graph ? graph->Vertices : (struct Vector){0};
}

size_t Graph_GetVerticeCount(const struct Graph *const graph)
{
	return graph ? graph->Count : 0;
}

size_t Graph_GetEdgeCount(const struct Graph *const graph)
{
	if( !graph )
		return 0;
	
	size_t totaledges = 0;
	for( size_t i=0 ; i<graph->Count ; i++ ) {
		struct GraphVertex *vert = graph->Table[i].Ptr;
		totaledges += vert->Edges.Count;
	}
	return totaledges;
}

void Graph_Resize(struct Graph *const graph)
{
	if( !graph )
		return;
	
	Vector_Resize(&graph->Vertices);
}

void Graph_Truncate(struct Graph *const graph)
{
	if( !graph )
		return;
	
	Vector_Truncate(&graph->Vertices);
}

void Graph_FromVector(struct Graph *const graph, const struct Vector *const vec)
{
	if( !graph || !vec )
		return;
	for( size_t i=0 ; i<vec->Count ; i++ )
		Graph_InsertVertexByValue(graph, vec->Table[i]);
}

void Graph_FromMap(struct Graph *const graph, const struct Hashmap *const map)
{
	if( !graph || !map )
		return;
	
	for( size_t i=0 ; i<map->Len ; i++ ) {
		struct Vector *vec = map->Table + i;
		for( size_t n=0 ; n<Vector_Count(vec) ; n++ ) {
			struct KeyNode *node = vec->Table[n].Ptr;
			Graph_InsertVertexByValue(graph, node->Data);
		}
	}
}
void Graph_FromUniLinkedList(struct Graph *const graph, const struct UniLinkedList *const list)
{
	if( !graph || !list )
		return;
	
	for( struct UniListNode *n=list->Head ; n ; n=n->Next )
		Graph_InsertVertexByValue(graph, n->Data);
}
void Graph_FromBiLinkedList(struct Graph *const graph, const struct BiLinkedList *const list)
{
	if( !graph || !list )
		return;
	
	for( struct BiListNode *n=list->Head ; n ; n=n->Next )
		Graph_InsertVertexByValue(graph, n->Data);
}
void Graph_FromTuple(struct Graph *const graph, const struct Tuple *const tup)
{
	if( !graph || !tup )
		return;
	for( size_t i=0 ; i<tup->Len ; i++ )
		Graph_InsertVertexByValue(graph, tup->Items[i]);
}
void Graph_FromLinkMap(struct Graph *const graph, const struct LinkMap *const map)
{
	if( !graph || !map )
		return;
	
	for( size_t i=0 ; i<map->Order.Count ; i++ ) {
		struct KeyNode *n = map->Order.Table[i].Ptr;
		Graph_InsertVertexByValue(graph, n->Data);
	}
}


struct Graph *Graph_NewFromVector(const struct Vector *const vec)
{
	if( !vec )
		return NULL;
	struct Graph *graph = Graph_New();
	Graph_FromVector(graph, vec);
	return graph;
}

struct Graph *Graph_NewFromMap(const struct Hashmap *const map)
{
	if( !map )
		return NULL;
	struct Graph *graph = Graph_New();
	Graph_FromMap(graph, map);
	return graph;
}
struct Graph *Graph_NewFromUniLinkedList(const struct UniLinkedList *const list)
{
	if( !list )
		return NULL;
	struct Graph *graph = Graph_New();
	Graph_FromUniLinkedList(graph, list);
	return graph;
}
struct Graph *Graph_NewFromBiLinkedList(const struct BiLinkedList *const list)
{
	if( !list )
		return NULL;
	struct Graph *graph = Graph_New();
	Graph_FromBiLinkedList(graph, list);
	return graph;
}
struct Graph *Graph_NewFromTuple(const struct Tuple *const tup)
{
	if( !tup )
		return NULL;
	struct Graph *graph = Graph_New();
	Graph_FromTuple(graph, tup);
	return graph;
}
struct Graph *Graph_NewFromLinkMap(const struct LinkMap *const map)
{
	if( !map )
		return NULL;
	struct Graph *graph = Graph_New();
	Graph_FromLinkMap(graph, map);
	return graph;
}
/**************************************/
