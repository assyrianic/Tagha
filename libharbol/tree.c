#include <stdlib.h>
#include <stdio.h>
#ifdef OS_WINDOWS
	#define HARBOL_LIB
#endif
#include "harbol.h"

HARBOL_EXPORT struct HarbolTree *harbol_tree_new(const union HarbolValue val)
{
	struct HarbolTree *tn = calloc(1, sizeof *tn);
	harbol_tree_init_val(tn, val);
	return tn;
}

HARBOL_EXPORT void harbol_tree_init(struct HarbolTree *const tn)
{
	if( !tn )
		return;
	memset(tn, 0, sizeof *tn);
}

HARBOL_EXPORT void harbol_tree_init_val(struct HarbolTree *const tn, const union HarbolValue val)
{
	if( !tn )
		return;
	memset(tn, 0, sizeof *tn);
	tn->Data = val;
}

HARBOL_EXPORT void harbol_tree_del(struct HarbolTree *const tn, fnDestructor *const dtor)
{
	if( !tn )
		return;
	if( dtor )
		(*dtor)(&tn->Data.Ptr);
	
	struct HarbolVector *vec = &tn->Children;
	for( size_t i=0 ; i<vec->Count ; i++ ) {
		struct HarbolTree *node = vec->Table[i].Ptr;
		harbol_tree_free(&node, dtor);
	}
	harbol_vector_del(vec, NULL);
	memset(tn, 0, sizeof *tn);
}

HARBOL_EXPORT void harbol_tree_free(struct HarbolTree **tnref, fnDestructor *const dtor)
{
	if( !*tnref )
		return;
	harbol_tree_del(*tnref, dtor);
	free(*tnref); *tnref=NULL;
}

HARBOL_EXPORT bool harbol_tree_insert_child_node(struct HarbolTree *const restrict tn, struct HarbolTree *const restrict node)
{
	if( !tn || !node )
		return false;
	
	harbol_vector_insert(&tn->Children, (union HarbolValue){.Ptr=node});
	return true;
}

HARBOL_EXPORT bool harbol_tree_insert_child_val(struct HarbolTree *const restrict tn, const union HarbolValue val)
{
	if( !tn )
		return false;
	
	struct HarbolTree *restrict node = harbol_tree_new(val);
	if( !node )
		return false;
	
	harbol_vector_insert(&tn->Children, (union HarbolValue){.Ptr=node});
	return true;
}

HARBOL_EXPORT bool harbol_tree_delete_child_by_ref(struct HarbolTree *const restrict tn, struct HarbolTree **noderef, fnDestructor *const dtor)
{
	if( !tn || !tn->Children.Table || !*noderef )
		return false;
	
	struct HarbolTree *restrict node = *noderef;
	struct HarbolVector *vec = &tn->Children;
	for( size_t i=0 ; i<vec->Count ; i++ ) {
		if( vec->Table[i].Ptr==node ) {
			struct HarbolTree *child = vec->Table[i].Ptr;
			harbol_tree_free(&child, dtor);
			harbol_vector_delete(vec, i, NULL);
			return true;
		}
	}
	return false;
}

HARBOL_EXPORT bool harbol_tree_delete_child_by_index(struct HarbolTree *const restrict tn, const size_t index, fnDestructor *const dtor)
{
	if( !tn || index >= tn->Children.Count )
		return false;
	
	struct HarbolVector *restrict vec = &tn->Children;
	struct HarbolTree *child = vec->Table[index].Ptr;
	harbol_tree_free(&child, dtor);
	harbol_vector_delete(vec, index, NULL);
	harbol_vector_truncate(vec);
	return true;
}

HARBOL_EXPORT bool harbol_tree_delete_child_by_val(struct HarbolTree *const restrict tn, const union HarbolValue val, fnDestructor *const dtor)
{
	if( !tn || !tn->Children.Table )
		return false;
	
	struct HarbolVector *vec = &tn->Children;
	for( size_t i=0 ; i<vec->Count ; i++ ) {
		struct HarbolTree *node = vec->Table[i].Ptr;
		if( node->Data.Int64==val.Int64 ) {
			harbol_tree_free(&node, dtor);
			harbol_vector_delete(vec, i, NULL);
			return true;
		}
	}
	harbol_vector_truncate(vec);
	return false;
}

HARBOL_EXPORT struct HarbolTree *harbol_tree_get_child_by_index(const struct HarbolTree *const tn, const size_t index)
{
	if( !tn || !tn->Children.Table || index >= tn->Children.Count )
		return NULL;
	return tn->Children.Table[index].Ptr;
}

HARBOL_EXPORT struct HarbolTree *harbol_tree_get_child_by_val(const struct HarbolTree *const restrict tn, const union HarbolValue val)
{
	if( !tn || !tn->Children.Table )
		return NULL;
	
	for( size_t i=0 ; i<tn->Children.Count ; i++ ) {
		struct HarbolTree *restrict node = tn->Children.Table[i].Ptr;
		if( node->Data.Int64==val.Int64 )
			return node;
	}
	return NULL;
}

HARBOL_EXPORT union HarbolValue harbol_tree_get_val(const struct HarbolTree *const tn)
{
	return tn ? tn->Data : (union HarbolValue){0};
}

HARBOL_EXPORT void harbol_tree_set_val(struct HarbolTree *const tn, const union HarbolValue val)
{
	if( !tn )
		return;
	
	tn->Data = val;
}

HARBOL_EXPORT struct HarbolVector *harbol_tree_get_children_vector(struct HarbolTree *const tn)
{
	return tn ? &tn->Children : NULL;
}

HARBOL_EXPORT size_t harbol_tree_get_children_len(const struct HarbolTree *const tn)
{
	return tn ? tn->Children.Len : 0;
}

HARBOL_EXPORT size_t harbol_tree_get_children_count(const struct HarbolTree *const tn)
{
	return tn ? tn->Children.Count : 0;
}
