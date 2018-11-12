#include <stdlib.h>
#include <stdio.h>
#ifdef OS_WINDOWS
	#define HARBOL_LIB
#endif
#include "harbol.h"

/* HarbolGraph Edge Code */
HARBOL_EXPORT struct HarbolGraphEdge *HarbolGraphEdge_New(void)
{
	return calloc(1, sizeof(struct HarbolGraphEdge));
}

HARBOL_EXPORT struct HarbolGraphEdge *HarbolGraphEdge_NewVP(const union HarbolValue val, struct HarbolGraphVertex *const vert)
{
	struct HarbolGraphEdge *edge = HarbolGraphEdge_New();
	if( edge ) {
		edge->Weight = val;
		edge->VertexSocket = vert;
	}
	return edge;
}

HARBOL_EXPORT void HarbolGraphEdge_Del(struct HarbolGraphEdge *const edge, fnDestructor *const edge_dtor)
{
	if( !edge )
		return;
	
	if( edge_dtor )
		(*edge_dtor)(&edge->Weight.Ptr);
	memset(edge, 0, sizeof *edge);
}

HARBOL_EXPORT void HarbolGraphEdge_Free(struct HarbolGraphEdge **edgeref, fnDestructor *const edge_dtor)
{
	if( !*edgeref )
		return;
	
	HarbolGraphEdge_Del(*edgeref, edge_dtor);
	free(*edgeref), *edgeref=NULL;
}

HARBOL_EXPORT union HarbolValue HarbolGraphEdge_GetWeight(const struct HarbolGraphEdge *const edge)
{
	return edge ? edge->Weight : (union HarbolValue){0};
}

HARBOL_EXPORT void HarbolGraphEdge_SetWeight(struct HarbolGraphEdge *const edge, const union HarbolValue val)
{
	if( !edge )
		return;
	edge->Weight = val;
}

HARBOL_EXPORT struct HarbolGraphVertex *HarbolGraphEdge_GetVertex(const struct HarbolGraphEdge *const edge)
{
	return edge ? edge->VertexSocket : NULL;
}

HARBOL_EXPORT void HarbolGraphEdge_SetVertex(struct HarbolGraphEdge *const edge, struct HarbolGraphVertex *const vert)
{
	if( !edge )
		return;
	
	edge->VertexSocket = vert;
}
/**************************************/


/* HarbolGraph Vertex Code */
HARBOL_EXPORT struct HarbolGraphVertex *HarbolGraphVertex_New(const union HarbolValue val)
{
	struct HarbolGraphVertex *vert = calloc(1, sizeof *vert);
	HarbolGraphVertex_Init(vert, val);
	return vert;
}

HARBOL_EXPORT void HarbolGraphVertex_Init(struct HarbolGraphVertex *const vert, const union HarbolValue val)
{
	if( !vert )
		return;
	
	vert->Data = val;
}

HARBOL_EXPORT void HarbolGraphVertex_Del(struct HarbolGraphVertex *const vert, fnDestructor *const edge_dtor, fnDestructor *const vert_dtor)
{
	if( !vert )
		return;
	
	if( vert_dtor )
		(*vert_dtor)(&vert->Data.Ptr);
	
	for( size_t i=0 ; i<vert->Edges.Count ; i++ ) {
		struct HarbolGraphEdge *edge = vert->Edges.Table[i].Ptr;
		HarbolGraphEdge_Free(&edge, edge_dtor);
	}
	HarbolVector_Del(&vert->Edges, NULL);
	memset(vert, 0, sizeof *vert);
}

HARBOL_EXPORT void HarbolGraphVertex_Free(struct HarbolGraphVertex **vertref, fnDestructor *const edge_dtor, fnDestructor *const vert_dtor)
{
	if( !vertref || !*vertref )
		return;
	
	HarbolGraphVertex_Del(*vertref, edge_dtor, vert_dtor);
	free(*vertref), *vertref=NULL;
}

HARBOL_EXPORT bool HarbolGraphVertex_AddEdge(struct HarbolGraphVertex *const vert, struct HarbolGraphEdge *const edge)
{
	return !vert || !edge ? false : HarbolVector_Insert(&vert->Edges, (union HarbolValue){.Ptr=edge});
}

HARBOL_EXPORT struct HarbolVector *HarbolGraphVertex_GetEdges(struct HarbolGraphVertex *const vert)
{
	return vert ? &vert->Edges : NULL;
}

HARBOL_EXPORT union HarbolValue HarbolGraphVertex_GetData(const struct HarbolGraphVertex *const vert)
{
	return vert ? vert->Data : (union HarbolValue){0};
}

HARBOL_EXPORT void HarbolGraphVertex_SetData(struct HarbolGraphVertex *const vert, const union HarbolValue val)
{
	if( !vert )
		return;
	
	vert->Data = val;
}
/**************************************/


/* HarbolGraph Code Implementation */
HARBOL_EXPORT struct HarbolGraph *HarbolGraph_New(void)
{
	struct HarbolGraph *graph = calloc(1, sizeof *graph);
	return graph;
}

HARBOL_EXPORT void HarbolGraph_Init(struct HarbolGraph *const graph)
{
	if( !graph )
		return;
	memset(graph, 0, sizeof *graph);
}

HARBOL_EXPORT void HarbolGraph_Del(struct HarbolGraph *const graph, fnDestructor *const edge_dtor, fnDestructor *const vert_dtor)
{
	if( !graph )
		return;
	
	for( size_t i=0 ; i<graph->Count ; i++ ) {
		struct HarbolGraphVertex *vertex = graph->Table[i].Ptr;
		HarbolGraphVertex_Free(&vertex, edge_dtor, vert_dtor);
	}
	HarbolVector_Del(&graph->Vertices, NULL);
}

HARBOL_EXPORT void HarbolGraph_Free(struct HarbolGraph **graphref, fnDestructor *const edge_dtor, fnDestructor *const vert_dtor)
{
	if( !*graphref )
		return;
	
	HarbolGraph_Del(*graphref, edge_dtor, vert_dtor);
	free(*graphref), *graphref=NULL;
}

HARBOL_EXPORT bool HarbolGraph_InsertVertexByValue(struct HarbolGraph *const graph, const union HarbolValue val)
{
	if( !graph )
		return false;
	
	struct HarbolGraphVertex *vert = HarbolGraphVertex_New(val);
	return !vert ? false : HarbolVector_Insert(&graph->Vertices, (union HarbolValue){.Ptr=vert});
}

HARBOL_EXPORT bool HarbolGraph_RemoveVertexByValue(struct HarbolGraph *const graph, const union HarbolValue val, fnDestructor *const edge_dtor, fnDestructor *const vert_dtor)
{
	if( !graph )
		return false;
	
	size_t index=0;
	struct HarbolGraphVertex *vert = NULL;
	while( !vert && index<graph->Count ) {
		struct HarbolGraphVertex *iter = graph->Table[index].Ptr;
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
		struct HarbolGraphVertex *iter = graph->Table[i].Ptr;
		if( iter==vert )
			continue;
		
		for( size_t n=0 ; n<iter->Edges.Count ; n++ ) {
			struct HarbolGraphEdge *edge = iter->Edges.Table[n].Ptr;
			if( edge->VertexSocket == vert )
				edge->VertexSocket = NULL;
		}
	}
	HarbolGraphVertex_Free(&vert, edge_dtor, vert_dtor);
	HarbolVector_Delete(&graph->Vertices, index, NULL);
	return false;
}

HARBOL_EXPORT bool HarbolGraph_RemoveVertexByIndex(struct HarbolGraph *const graph, const size_t index, fnDestructor *const edge_dtor, fnDestructor *const vert_dtor)
{
	if( !graph )
		return false;
	
	struct HarbolGraphVertex *vert = HarbolGraph_GetVertexByIndex(graph, index);
	if( !vert )
		return false;
	
	// cut its links with other vertices.
	for( size_t i=0 ; i<graph->Count ; i++ ) {
		if( i==index )
			continue;
		
		struct HarbolGraphVertex *restrict iter = graph->Table[i].Ptr;
		if( !iter )
			continue;
		
		for( size_t n=0 ; n<iter->Count ; n++ ) {
			struct HarbolGraphEdge *edge = iter->Table[n].Ptr;
			if( edge->VertexSocket==vert ) {
				edge->VertexSocket = NULL;
				HarbolGraphEdge_Free(&edge, edge_dtor);
				HarbolVector_Delete(&iter->Edges, n, NULL);
				break;
			}
		}
	}
	HarbolGraphVertex_Free(&vert, edge_dtor, vert_dtor);
	HarbolVector_Delete(&graph->Vertices, index, NULL);
	return true;
}

HARBOL_EXPORT bool HarbolGraph_InsertEdgeBtwnVerts(struct HarbolGraph *const graph, const size_t index1, const size_t index2, const union HarbolValue weight)
{
	if( !graph )
		return false;
	
	struct HarbolGraphVertex *const restrict vert1 = HarbolGraph_GetVertexByIndex(graph, index1);
	struct HarbolGraphVertex *const restrict vert2 = HarbolGraph_GetVertexByIndex(graph, index2);
	if( !vert1 || !vert2 || vert1==vert2 )
		return false;
	
	struct HarbolGraphEdge *edge = HarbolGraphEdge_NewVP(weight, vert2);
	return ( !edge ) ? false : HarbolGraphVertex_AddEdge(vert1, edge);
}

HARBOL_EXPORT bool HarbolGraph_RemoveEdgeBtwnVerts(struct HarbolGraph *const graph, const size_t index1, const size_t index2, fnDestructor *const edge_dtor)
{
	if( !graph )
		return false;
	
	struct HarbolGraphVertex *const restrict vert1 = HarbolGraph_GetVertexByIndex(graph, index1);
	struct HarbolGraphVertex *const restrict vert2 = HarbolGraph_GetVertexByIndex(graph, index2);
	if( !vert1 || !vert2 || vert1==vert2 )
		return false;
	
	// make sure the vertex #1 actually has a connection to vertex #2
	struct HarbolGraphEdge *edge = NULL;
	size_t n = 0;
	for( ; n<vert1->Edges.Count ; n++ ) {
		struct HarbolGraphEdge *e = vert1->Edges.Table[n].Ptr;
		if( e->VertexSocket == vert2 ) {
			edge = e;
			break;
		}
	}
	if( !edge )
		return false;
	
	HarbolGraphEdge_Free(&edge, edge_dtor);
	HarbolVector_Delete(&vert1->Edges, n, NULL);
	return true;
}

HARBOL_EXPORT struct HarbolGraphVertex *HarbolGraph_GetVertexByIndex(struct HarbolGraph *const graph, const size_t index)
{
	return !graph || index >= graph->Count ? NULL : graph->Table[index].Ptr;
}

HARBOL_EXPORT union HarbolValue HarbolGraph_GetVertexDataByIndex(struct HarbolGraph *const graph, const size_t index)
{
	if( !graph || !graph->Table )
		return (union HarbolValue){0};
	
	struct HarbolGraphVertex *vertex = graph->Table[index].Ptr;
	return !vertex ? (union HarbolValue){0} : vertex->Data;
}

HARBOL_EXPORT void HarbolGraph_SetVertexDataByIndex(struct HarbolGraph *const graph, const size_t index, const union HarbolValue val)
{
	if( !graph || !graph->Table )
		return;
	
	struct HarbolGraphVertex *vertex = graph->Table[index].Ptr;
	if( vertex )
		vertex->Data = val;
}

HARBOL_EXPORT struct HarbolGraphEdge *HarbolGraph_GetEdgeBtwnVertices(struct HarbolGraph *const graph, const size_t index1, const size_t index2)
{
	if( !graph )
		return NULL;
	
	struct HarbolGraphVertex *const restrict vert1 = HarbolGraph_GetVertexByIndex(graph, index1);
	struct HarbolGraphVertex *const restrict vert2 = HarbolGraph_GetVertexByIndex(graph, index2);
	if( !vert1 || !vert2 || vert1==vert2 )
		return NULL;
	
	for( size_t i=0 ; i<vert1->Edges.Count ; i++ ) {
		struct HarbolGraphEdge *edge = vert1->Edges.Table[i].Ptr;
		if( edge->VertexSocket==vert2 )
			return edge;
	}
	return NULL;
}

HARBOL_EXPORT bool HarbolGraph_IsVertexAdjacent(struct HarbolGraph *const graph, const size_t index1, const size_t index2)
{
	if( !graph )
		return false;
	
	struct HarbolGraphVertex *const restrict vert1 = HarbolGraph_GetVertexByIndex(graph, index1);
	struct HarbolGraphVertex *const restrict vert2 = HarbolGraph_GetVertexByIndex(graph, index2);
	if( !vert1 || !vert2 || vert1==vert2 )
		return false;
	
	for( size_t i=0 ; i<vert1->Edges.Count ; i++ ) {
		struct HarbolGraphEdge *edge = vert1->Edges.Table[i].Ptr;
		if( edge->VertexSocket==vert2 )
			return true;
	}
	return false;
}

HARBOL_EXPORT struct HarbolVector *HarbolGraph_GetVertexNeighbors(struct HarbolGraph *const graph, const size_t index)
{
	return !graph || index >= graph->Count ? NULL : HarbolGraphVertex_GetEdges(graph->Table[index].Ptr);
}

HARBOL_EXPORT struct HarbolVector *HarbolGraph_GetVertices(struct HarbolGraph *const graph)
{
	return graph ? &graph->Vertices : NULL;
}

HARBOL_EXPORT size_t HarbolGraph_GetVerticeCount(const struct HarbolGraph *const graph)
{
	return graph ? graph->Count : 0;
}

HARBOL_EXPORT size_t HarbolGraph_GetEdgeCount(const struct HarbolGraph *const graph)
{
	if( !graph )
		return 0;
	
	size_t totaledges = 0;
	for( size_t i=0 ; i<graph->Count ; i++ ) {
		struct HarbolGraphVertex *vert = graph->Table[i].Ptr;
		totaledges += vert->Edges.Count;
	}
	return totaledges;
}

HARBOL_EXPORT void HarbolGraph_Resize(struct HarbolGraph *const graph)
{
	if( !graph )
		return;
	
	HarbolVector_Resize(&graph->Vertices);
}

HARBOL_EXPORT void HarbolGraph_Truncate(struct HarbolGraph *const graph)
{
	if( !graph )
		return;
	
	HarbolVector_Truncate(&graph->Vertices);
}

HARBOL_EXPORT void HarbolGraph_FromHarbolVector(struct HarbolGraph *const graph, const struct HarbolVector *const vec)
{
	if( !graph || !vec )
		return;
	for( size_t i=0 ; i<vec->Count ; i++ )
		HarbolGraph_InsertVertexByValue(graph, vec->Table[i]);
}

HARBOL_EXPORT void HarbolGraph_FromHarbolHashmap(struct HarbolGraph *const graph, const struct HarbolHashmap *const map)
{
	if( !graph || !map )
		return;
	
	for( size_t i=0 ; i<map->Len ; i++ ) {
		struct HarbolVector *vec = map->Table + i;
		for( size_t n=0 ; n<HarbolVector_Count(vec) ; n++ ) {
			struct HarbolKeyValPair *node = vec->Table[n].Ptr;
			HarbolGraph_InsertVertexByValue(graph, node->Data);
		}
	}
}

HARBOL_EXPORT void HarbolGraph_FromHarbolUniList(struct HarbolGraph *const graph, const struct HarbolUniList *const list)
{
	if( !graph || !list )
		return;
	
	for( struct HarbolUniListNode *n=list->Head ; n ; n=n->Next )
		HarbolGraph_InsertVertexByValue(graph, n->Data);
}

HARBOL_EXPORT void HarbolGraph_FromHarbolBiList(struct HarbolGraph *const graph, const struct HarbolBiList *const list)
{
	if( !graph || !list )
		return;
	
	for( struct HarbolBiListNode *n=list->Head ; n ; n=n->Next )
		HarbolGraph_InsertVertexByValue(graph, n->Data);
}

HARBOL_EXPORT void HarbolGraph_FromHarbolTuple(struct HarbolGraph *const graph, const struct HarbolTuple *const tup)
{
	if( !graph || !tup )
		return;
	for( size_t i=0 ; i<tup->Len ; i++ )
		HarbolGraph_InsertVertexByValue(graph, tup->Items[i]);
}

HARBOL_EXPORT void HarbolGraph_FromHarbolLinkMap(struct HarbolGraph *const graph, const struct HarbolLinkMap *const map)
{
	if( !graph || !map )
		return;
	
	for( size_t i=0 ; i<map->Order.Count ; i++ ) {
		struct HarbolKeyValPair *n = map->Order.Table[i].Ptr;
		HarbolGraph_InsertVertexByValue(graph, n->Data);
	}
}

HARBOL_EXPORT struct HarbolGraph *HarbolGraph_NewFromHarbolVector(const struct HarbolVector *const vec)
{
	if( !vec )
		return NULL;
	struct HarbolGraph *graph = HarbolGraph_New();
	HarbolGraph_FromHarbolVector(graph, vec);
	return graph;
}

HARBOL_EXPORT struct HarbolGraph *HarbolGraph_NewFromHarbolHashmap(const struct HarbolHashmap *const map)
{
	if( !map )
		return NULL;
	struct HarbolGraph *graph = HarbolGraph_New();
	HarbolGraph_FromHarbolHashmap(graph, map);
	return graph;
}

HARBOL_EXPORT struct HarbolGraph *HarbolGraph_NewFromHarbolUniList(const struct HarbolUniList *const list)
{
	if( !list )
		return NULL;
	struct HarbolGraph *graph = HarbolGraph_New();
	HarbolGraph_FromHarbolUniList(graph, list);
	return graph;
}

HARBOL_EXPORT struct HarbolGraph *HarbolGraph_NewFromHarbolBiList(const struct HarbolBiList *const list)
{
	if( !list )
		return NULL;
	struct HarbolGraph *graph = HarbolGraph_New();
	HarbolGraph_FromHarbolBiList(graph, list);
	return graph;
}

HARBOL_EXPORT struct HarbolGraph *HarbolGraph_NewFromHarbolTuple(const struct HarbolTuple *const tup)
{
	if( !tup )
		return NULL;
	struct HarbolGraph *graph = HarbolGraph_New();
	HarbolGraph_FromHarbolTuple(graph, tup);
	return graph;
}

HARBOL_EXPORT struct HarbolGraph *HarbolGraph_NewFromHarbolLinkMap(const struct HarbolLinkMap *const map)
{
	if( !map )
		return NULL;
	struct HarbolGraph *graph = HarbolGraph_New();
	HarbolGraph_FromHarbolLinkMap(graph, map);
	return graph;
}
/**************************************/
