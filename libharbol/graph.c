#include <stdlib.h>
#include <stdio.h>
#ifdef OS_WINDOWS
	#define HARBOL_LIB
#endif
#include "harbol.h"

/* HarbolGraph Edge Code */
HARBOL_EXPORT struct HarbolGraphEdge *harbol_edge_new(void)
{
	return calloc(1, sizeof(struct HarbolGraphEdge));
}

HARBOL_EXPORT struct HarbolGraphEdge *harbol_edge_new_val_vert(const union HarbolValue val, struct HarbolGraphVertex *const vert)
{
	struct HarbolGraphEdge *edge = harbol_edge_new();
	if( edge ) {
		edge->Weight = val;
		edge->VertexSocket = vert;
	}
	return edge;
}

HARBOL_EXPORT void harbol_edge_del(struct HarbolGraphEdge *const edge, fnDestructor *const edge_dtor)
{
	if( !edge )
		return;
	
	if( edge_dtor )
		(*edge_dtor)(&edge->Weight.Ptr);
	memset(edge, 0, sizeof *edge);
}

HARBOL_EXPORT void harbol_edge_free(struct HarbolGraphEdge **edgeref, fnDestructor *const edge_dtor)
{
	if( !*edgeref )
		return;
	
	harbol_edge_del(*edgeref, edge_dtor);
	free(*edgeref), *edgeref=NULL;
}

HARBOL_EXPORT union HarbolValue harbol_edge_get_weight(const struct HarbolGraphEdge *const edge)
{
	return edge ? edge->Weight : (union HarbolValue){0};
}

HARBOL_EXPORT void harbol_edge_set_weight(struct HarbolGraphEdge *const edge, const union HarbolValue val)
{
	if( !edge )
		return;
	edge->Weight = val;
}

HARBOL_EXPORT struct HarbolGraphVertex *harbol_edge_get_vertex(const struct HarbolGraphEdge *const edge)
{
	return edge ? edge->VertexSocket : NULL;
}

HARBOL_EXPORT void harbol_edge_set_vertex(struct HarbolGraphEdge *const edge, struct HarbolGraphVertex *const vert)
{
	if( !edge )
		return;
	
	edge->VertexSocket = vert;
}
/**************************************/


/* HarbolGraph Vertex Code */
HARBOL_EXPORT struct HarbolGraphVertex *harbol_vertex_new(const union HarbolValue val)
{
	struct HarbolGraphVertex *vert = calloc(1, sizeof *vert);
	harbol_vertex_init(vert, val);
	return vert;
}

HARBOL_EXPORT void harbol_vertex_init(struct HarbolGraphVertex *const vert, const union HarbolValue val)
{
	if( !vert )
		return;
	
	vert->Data = val;
}

HARBOL_EXPORT void harbol_vertex_del(struct HarbolGraphVertex *const vert, fnDestructor *const edge_dtor, fnDestructor *const vert_dtor)
{
	if( !vert )
		return;
	
	if( vert_dtor )
		(*vert_dtor)(&vert->Data.Ptr);
	
	for( size_t i=0 ; i<vert->Edges.Count ; i++ ) {
		struct HarbolGraphEdge *edge = vert->Edges.Table[i].Ptr;
		harbol_edge_free(&edge, edge_dtor);
	}
	harbol_vector_del(&vert->Edges, NULL);
	memset(vert, 0, sizeof *vert);
}

HARBOL_EXPORT void harbol_vertex_free(struct HarbolGraphVertex **vertref, fnDestructor *const edge_dtor, fnDestructor *const vert_dtor)
{
	if( !vertref || !*vertref )
		return;
	
	harbol_vertex_del(*vertref, edge_dtor, vert_dtor);
	free(*vertref), *vertref=NULL;
}

HARBOL_EXPORT bool harbol_vertex_add_edge(struct HarbolGraphVertex *const vert, struct HarbolGraphEdge *const edge)
{
	return !vert || !edge ? false : harbol_vector_insert(&vert->Edges, (union HarbolValue){.Ptr=edge});
}

HARBOL_EXPORT struct HarbolVector *harbol_vertex_get_edges(struct HarbolGraphVertex *const vert)
{
	return vert ? &vert->Edges : NULL;
}

HARBOL_EXPORT union HarbolValue harbol_vertex_get_val(const struct HarbolGraphVertex *const vert)
{
	return vert ? vert->Data : (union HarbolValue){0};
}

HARBOL_EXPORT void harbol_vertex_set_val(struct HarbolGraphVertex *const vert, const union HarbolValue val)
{
	if( !vert )
		return;
	
	vert->Data = val;
}
/**************************************/


/* HarbolGraph Code Implementation */
HARBOL_EXPORT struct HarbolGraph *harbol_graph_new(void)
{
	struct HarbolGraph *graph = calloc(1, sizeof *graph);
	return graph;
}

HARBOL_EXPORT void harbol_graph_init(struct HarbolGraph *const graph)
{
	if( !graph )
		return;
	memset(graph, 0, sizeof *graph);
}

HARBOL_EXPORT void harbol_graph_del(struct HarbolGraph *const graph, fnDestructor *const edge_dtor, fnDestructor *const vert_dtor)
{
	if( !graph )
		return;
	
	for( size_t i=0 ; i<graph->Count ; i++ ) {
		struct HarbolGraphVertex *vertex = graph->Table[i].Ptr;
		harbol_vertex_free(&vertex, edge_dtor, vert_dtor);
	}
	harbol_vector_del(&graph->Vertices, NULL);
}

HARBOL_EXPORT void harbol_graph_free(struct HarbolGraph **graphref, fnDestructor *const edge_dtor, fnDestructor *const vert_dtor)
{
	if( !*graphref )
		return;
	
	harbol_graph_del(*graphref, edge_dtor, vert_dtor);
	free(*graphref), *graphref=NULL;
}

HARBOL_EXPORT bool harbol_graph_insert_val(struct HarbolGraph *const graph, const union HarbolValue val)
{
	if( !graph )
		return false;
	
	struct HarbolGraphVertex *vert = harbol_vertex_new(val);
	return !vert ? false : harbol_vector_insert(&graph->Vertices, (union HarbolValue){.Ptr=vert});
}

HARBOL_EXPORT bool harbol_graph_delete_val(struct HarbolGraph *const graph, const union HarbolValue val, fnDestructor *const edge_dtor, fnDestructor *const vert_dtor)
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
	harbol_vertex_free(&vert, edge_dtor, vert_dtor);
	harbol_vector_delete(&graph->Vertices, index, NULL);
	return false;
}

HARBOL_EXPORT bool harbol_graph_delete_val_by_index(struct HarbolGraph *const graph, const size_t index, fnDestructor *const edge_dtor, fnDestructor *const vert_dtor)
{
	if( !graph )
		return false;
	
	struct HarbolGraphVertex *vert = harbol_graph_get_vertex_by_index(graph, index);
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
				harbol_edge_free(&edge, edge_dtor);
				harbol_vector_delete(&iter->Edges, n, NULL);
				break;
			}
		}
	}
	harbol_vertex_free(&vert, edge_dtor, vert_dtor);
	harbol_vector_delete(&graph->Vertices, index, NULL);
	return true;
}

HARBOL_EXPORT bool harbol_graph_insert_edge(struct HarbolGraph *const graph, const size_t index1, const size_t index2, const union HarbolValue weight)
{
	if( !graph )
		return false;
	
	struct HarbolGraphVertex *const restrict vert1 = harbol_graph_get_vertex_by_index(graph, index1);
	struct HarbolGraphVertex *const restrict vert2 = harbol_graph_get_vertex_by_index(graph, index2);
	if( !vert1 || !vert2 || vert1==vert2 )
		return false;
	
	struct HarbolGraphEdge *edge = harbol_edge_new_val_vert(weight, vert2);
	return ( !edge ) ? false : harbol_vertex_add_edge(vert1, edge);
}

HARBOL_EXPORT bool harbol_graph_delete_edge(struct HarbolGraph *const graph, const size_t index1, const size_t index2, fnDestructor *const edge_dtor)
{
	if( !graph )
		return false;
	
	struct HarbolGraphVertex *const restrict vert1 = harbol_graph_get_vertex_by_index(graph, index1);
	struct HarbolGraphVertex *const restrict vert2 = harbol_graph_get_vertex_by_index(graph, index2);
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
	
	harbol_edge_free(&edge, edge_dtor);
	harbol_vector_delete(&vert1->Edges, n, NULL);
	return true;
}

HARBOL_EXPORT struct HarbolGraphVertex *harbol_graph_get_vertex_by_index(struct HarbolGraph *const graph, const size_t index)
{
	return !graph || index >= graph->Count ? NULL : graph->Table[index].Ptr;
}

HARBOL_EXPORT union HarbolValue harbol_graph_get_val_by_index(struct HarbolGraph *const graph, const size_t index)
{
	if( !graph )
		return (union HarbolValue){0};
	
	struct HarbolGraphVertex *vertex = harbol_vector_get(&graph->Vertices, index).GraphVertPtr;
	return !vertex ? (union HarbolValue){0} : vertex->Data;
}

HARBOL_EXPORT void harbol_graph_set_val_by_index(struct HarbolGraph *const graph, const size_t index, const union HarbolValue val)
{
	if( !graph || !graph->Table )
		return;
	
	struct HarbolGraphVertex *vertex = graph->Table[index].Ptr;
	if( vertex )
		vertex->Data = val;
}

HARBOL_EXPORT struct HarbolGraphEdge *harbol_graph_get_edge(struct HarbolGraph *const graph, const size_t index1, const size_t index2)
{
	if( !graph )
		return NULL;
	
	const struct HarbolGraphVertex *const restrict vert1 = harbol_graph_get_vertex_by_index(graph, index1);
	const struct HarbolGraphVertex *const restrict vert2 = harbol_graph_get_vertex_by_index(graph, index2);
	if( !vert1 || !vert2 || vert1==vert2 )
		return NULL;
	
	for( size_t i=0 ; i<vert1->Edges.Count ; i++ ) {
		struct HarbolGraphEdge *edge = vert1->Edges.Table[i].Ptr;
		if( edge->VertexSocket==vert2 )
			return edge;
	}
	return NULL;
}

HARBOL_EXPORT bool harbol_graph_is_vertex_adjacent_by_index(struct HarbolGraph *const graph, const size_t index1, const size_t index2)
{
	if( !graph )
		return false;
	
	const struct HarbolGraphVertex *const restrict vert1 = harbol_graph_get_vertex_by_index(graph, index1);
	const struct HarbolGraphVertex *const restrict vert2 = harbol_graph_get_vertex_by_index(graph, index2);
	if( !vert1 || !vert2 || vert1==vert2 )
		return false;
	
	for( size_t i=0 ; i<vert1->Edges.Count ; i++ ) {
		struct HarbolGraphEdge *edge = vert1->Edges.Table[i].Ptr;
		if( edge->VertexSocket==vert2 )
			return true;
	}
	return false;
}

HARBOL_EXPORT struct HarbolVector *harbol_graph_get_vertex_neighbors(struct HarbolGraph *const graph, const size_t index)
{
	return !graph || index >= graph->Count ? NULL : harbol_vertex_get_edges(graph->Table[index].Ptr);
}

HARBOL_EXPORT struct HarbolVector *harbol_graph_get_vertex_vector(struct HarbolGraph *const graph)
{
	return graph ? &graph->Vertices : NULL;
}

HARBOL_EXPORT size_t harbol_graph_get_vertex_count(const struct HarbolGraph *const graph)
{
	return graph ? graph->Count : 0;
}

HARBOL_EXPORT size_t harbol_graph_get_edge_count(const struct HarbolGraph *const graph)
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

HARBOL_EXPORT void harbol_graph_resize(struct HarbolGraph *const graph)
{
	if( !graph )
		return;
	
	harbol_vector_resize(&graph->Vertices);
}

HARBOL_EXPORT void harbol_graph_truncate(struct HarbolGraph *const graph)
{
	if( !graph )
		return;
	
	harbol_vector_truncate(&graph->Vertices);
}

HARBOL_EXPORT void harbol_graph_from_vector(struct HarbolGraph *const graph, const struct HarbolVector *const vec)
{
	if( !graph || !vec )
		return;
	for( size_t i=0 ; i<vec->Count ; i++ )
		harbol_graph_insert_val(graph, vec->Table[i]);
}

HARBOL_EXPORT void harbol_graph_from_hashmap(struct HarbolGraph *const graph, const struct HarbolHashmap *const map)
{
	if( !graph || !map )
		return;
	
	for( size_t i=0 ; i<map->Len ; i++ ) {
		struct HarbolVector *vec = map->Table + i;
		for( size_t n=0 ; n<harbol_vector_get_count(vec) ; n++ ) {
			struct HarbolKeyValPair *kv = vec->Table[n].Ptr;
			harbol_graph_insert_val(graph, kv->Data);
		}
	}
}

HARBOL_EXPORT void harbol_graph_from_unilist(struct HarbolGraph *const graph, const struct HarbolUniList *const list)
{
	if( !graph || !list )
		return;
	
	for( struct HarbolUniListNode *n=list->Head ; n ; n=n->Next )
		harbol_graph_insert_val(graph, n->Data);
}

HARBOL_EXPORT void harbol_graph_from_bilist(struct HarbolGraph *const graph, const struct HarbolBiList *const list)
{
	if( !graph || !list )
		return;
	
	for( struct HarbolBiListNode *n=list->Head ; n ; n=n->Next )
		harbol_graph_insert_val(graph, n->Data);
}

HARBOL_EXPORT void harbol_graph_from_linkmap(struct HarbolGraph *const graph, const struct HarbolLinkMap *const map)
{
	if( !graph || !map )
		return;
	
	for( size_t i=0 ; i<map->Order.Count ; i++ ) {
		struct HarbolKeyValPair *kv = map->Order.Table[i].Ptr;
		harbol_graph_insert_val(graph, kv->Data);
	}
}

HARBOL_EXPORT struct HarbolGraph *harbol_graph_new_from_vector(const struct HarbolVector *const vec)
{
	if( !vec )
		return NULL;
	struct HarbolGraph *graph = harbol_graph_new();
	harbol_graph_from_vector(graph, vec);
	return graph;
}

HARBOL_EXPORT struct HarbolGraph *harbol_graph_new_from_hashmap(const struct HarbolHashmap *const map)
{
	if( !map )
		return NULL;
	struct HarbolGraph *graph = harbol_graph_new();
	harbol_graph_from_hashmap(graph, map);
	return graph;
}

HARBOL_EXPORT struct HarbolGraph *harbol_graph_new_from_unilist(const struct HarbolUniList *const list)
{
	if( !list )
		return NULL;
	struct HarbolGraph *graph = harbol_graph_new();
	harbol_graph_from_unilist(graph, list);
	return graph;
}

HARBOL_EXPORT struct HarbolGraph *harbol_graph_new_from_bilist(const struct HarbolBiList *const list)
{
	if( !list )
		return NULL;
	struct HarbolGraph *graph = harbol_graph_new();
	harbol_graph_from_bilist(graph, list);
	return graph;
}

HARBOL_EXPORT struct HarbolGraph *harbol_graph_new_from_linkmap(const struct HarbolLinkMap *const map)
{
	if( !map )
		return NULL;
	struct HarbolGraph *graph = harbol_graph_new();
	harbol_graph_from_linkmap(graph, map);
	return graph;
}
/**************************************/
