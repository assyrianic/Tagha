#include <stdlib.h>
#include <stdio.h>
#include "dsc.h"

struct TreeNode *TreeNode_New(const union Value val)
{
	struct TreeNode *tn = calloc(1, sizeof *tn);
	TreeNode_InitVal(tn, val);
	return tn;
}

void TreeNode_Init(struct TreeNode *const tn)
{
	if( !tn )
		return;
	*tn = (struct TreeNode){0};
}
void TreeNode_InitVal(struct TreeNode *const tn, const union Value val)
{
	if( !tn )
		return;
	*tn = (struct TreeNode){0};
	tn->Data = val;
}

void TreeNode_Del(struct TreeNode *const tn, fnDestructor *const dtor)
{
	if( !tn )
		return;
	if( dtor )
		(*dtor)(&tn->Data.Ptr);
	
	struct Vector *vec = &tn->Children;
	for( size_t i=0 ; i<vec->Count ; i++ ) {
		struct TreeNode *node = vec->Table[i].Ptr;
		TreeNode_Free(&node, dtor);
	}
	Vector_Del(vec, NULL);
	*tn = (struct TreeNode){0};
}

void TreeNode_Free(struct TreeNode **tnref, fnDestructor *const dtor)
{
	if( !*tnref )
		return;
	TreeNode_Del(*tnref, dtor);
	free(*tnref); *tnref=NULL;
}

bool TreeNode_InsertChildByNode(struct TreeNode *const restrict tn, struct TreeNode *const restrict node)
{
	if( !tn || !node )
		return false;
	
	Vector_Insert(&tn->Children, (union Value){.Ptr=node});
	return true;
}

bool TreeNode_InsertChildByValue(struct TreeNode *const restrict tn, const union Value val)
{
	if( !tn )
		return false;
	
	struct TreeNode *restrict node = TreeNode_New(val);
	if( !node )
		return false;
	
	Vector_Insert(&tn->Children, (union Value){.Ptr=node});
	return true;
}

bool TreeNode_RemoveChildByRef(struct TreeNode *const restrict tn, struct TreeNode **noderef, fnDestructor *const dtor)
{
	if( !tn || !tn->Children.Table || !*noderef )
		return false;
	
	struct TreeNode *restrict node = *noderef;
	struct Vector *vec = &tn->Children;
	for( size_t i=0 ; i<vec->Count ; i++ ) {
		if( vec->Table[i].Ptr==node ) {
			struct TreeNode *child = vec->Table[i].Ptr;
			TreeNode_Free(&child, dtor);
			Vector_Delete(vec, i, NULL);
			return true;
		}
	}
	return false;
}

bool TreeNode_RemoveChildByIndex(struct TreeNode *const restrict tn, const size_t index, fnDestructor *const dtor)
{
	if( !tn || index >= tn->Children.Count )
		return false;
	
	struct Vector *restrict vec = &tn->Children;
	struct TreeNode *child = vec->Table[index].Ptr;
	TreeNode_Free(&child, dtor);
	Vector_Delete(vec, index, NULL);
	Vector_Truncate(vec);
	return true;
}

bool TreeNode_RemoveChildByValue(struct TreeNode *const restrict tn, const union Value val, fnDestructor *const dtor)
{
	if( !tn || !tn->Children.Table )
		return false;
	
	struct Vector *vec = &tn->Children;
	for( size_t i=0 ; i<vec->Count ; i++ ) {
		struct TreeNode *node = vec->Table[i].Ptr;
		if( node->Data.Int64==val.Int64 ) {
			TreeNode_Free(&node, dtor);
			Vector_Delete(vec, i, NULL);
			return true;
		}
	}
	Vector_Truncate(vec);
	return false;
}

struct TreeNode *TreeNode_GetChildByIndex(const struct TreeNode *const tn, const size_t index)
{
	if( !tn || !tn->Children.Table || index >= tn->Children.Count )
		return NULL;
	return tn->Children.Table[index].Ptr;
}
struct TreeNode *TreeNode_GetChildByValue(const struct TreeNode *const restrict tn, const union Value val)
{
	if( !tn || !tn->Children.Table )
		return NULL;
	
	for( size_t i=0 ; i<tn->Children.Count ; i++ ) {
		struct TreeNode *restrict node = tn->Children.Table[i].Ptr;
		if( node->Data.Int64==val.Int64 )
			return node;
	}
	return NULL;
}

union Value TreeNode_GetData(const struct TreeNode *const tn)
{
	return tn ? tn->Data : (union Value){0};
}
void TreeNode_SetData(struct TreeNode *const tn, const union Value val)
{
	if( !tn )
		return;
	
	tn->Data = val;
}
struct Vector TreeNode_GetChildren(const struct TreeNode *const tn)
{
	return tn ? tn->Children : (struct Vector){NULL,0,0};
}
size_t TreeNode_GetChildLen(const struct TreeNode *const tn)
{
	return tn ? tn->Children.Len : 0;
}
size_t TreeNode_GetChildCount(const struct TreeNode *const tn)
{
	return tn ? tn->Children.Count : 0;
}
