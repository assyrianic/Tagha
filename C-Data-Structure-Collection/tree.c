#include <stdlib.h>
#include <stdio.h>
#include "dsc.h"

static void TreeNode_Resize(struct TreeNode *);
static void TreeNode_Truncate(struct TreeNode *);

struct TreeNode *TreeNode_New(const union Value val)
{
	struct TreeNode *tn = calloc(1, sizeof *tn);
	TreeNode_InitVal(tn, val);
	return tn;
}

void TreeNode_Init(struct TreeNode *const restrict tn)
{
	if( !tn )
		return;
	*tn = (struct TreeNode){0};
}
void TreeNode_InitVal(struct TreeNode *const restrict tn, const union Value val)
{
	if( !tn )
		return;
	*tn = (struct TreeNode){0};
	tn->Data = val;
}

void TreeNode_Del(struct TreeNode *const restrict tn, bool(*dtor)())
{
	if( !tn )
		return;
	if( dtor )
		(*dtor)(&tn->Data.Ptr);
	for( size_t i=0 ; i<tn->ChildCount ; i++ )
		TreeNode_Free(tn->Children+i, dtor);
	free(tn->Children);
	tn->Children=NULL;
	*tn = (struct TreeNode){0};
}

void TreeNode_Free(struct TreeNode **restrict tnref, bool(*dtor)())
{
	if( !*tnref )
		return;
	TreeNode_Del(*tnref, dtor);
	free(*tnref);
	*tnref=NULL;
}

bool TreeNode_InsertChildByNode(struct TreeNode *const restrict tn, struct TreeNode *const restrict node)
{
	if( !tn or !node )
		return false;
	else if( !tn->Children or tn->ChildCount >= tn->ChildLen )
		TreeNode_Resize(tn);
	
	tn->Children[tn->ChildCount++] = node;
	return true;
}

bool TreeNode_InsertChildByValue(struct TreeNode *const restrict tn, const union Value val)
{
	if( !tn )
		return false;
	struct TreeNode *child = TreeNode_New(val);
	if( !child )
		return false;
	else if( !tn->Children or tn->ChildCount >= tn->ChildLen )
		TreeNode_Resize(tn);
	
	tn->Children[tn->ChildCount++] = child;
	return true;
}

bool TreeNode_RemoveChildByRef(struct TreeNode *const restrict tn, struct TreeNode **noderef, bool(*dtor)())
{
	if( !tn or !tn->Children or !*noderef )
		return false;
	struct TreeNode *node = *noderef;
	for( size_t index=0 ; index<tn->ChildCount ; index++ ) {
		if( tn->Children[index]==node ) {
			TreeNode_Free(tn->Children+index, dtor);
			size_t
				i=index+1,
				j=index
			;
			while( i<tn->ChildCount )
				tn->Children[j++] = tn->Children[i++];
			tn->ChildCount--;
			return true;
		}
	}
	return false;
}

bool TreeNode_RemoveChildByIndex(struct TreeNode *const restrict tn, const size_t index, bool(*dtor)())
{
	if( !tn or !tn->Children or index >= tn->ChildCount )
		return false;
	
	TreeNode_Free(tn->Children+index, dtor);
	size_t
		i=index+1,
		j=index
	;
	while( i<tn->ChildCount )
		tn->Children[j++] = tn->Children[i++];
	tn->ChildCount--;
	TreeNode_Truncate(tn);
	return true;
}

bool TreeNode_RemoveChildByValue(struct TreeNode *const restrict tn, const union Value val, bool(*dtor)())
{
	if( !tn or !tn->Children )
		return false;
	for( size_t index=0 ; index<tn->ChildCount ; index++ ) {
		if( tn->Children[index]->Data.Int64==val.Int64 ) {
			TreeNode_Free(tn->Children+index, dtor);
			size_t
				i=index+1,
				j=index
			;
			while( i<tn->ChildCount )
				tn->Children[j++] = tn->Children[i++];
			tn->ChildCount--;
			return true;
		}
	}
	TreeNode_Truncate(tn);
	return false;
}

struct TreeNode *TreeNode_GetChildByIndex(const struct TreeNode *const restrict tn, const size_t index)
{
	if( !tn or !tn->Children or index >= tn->ChildCount )
		return NULL;
	return tn->Children[index];
}
struct TreeNode *TreeNode_GetChildByValue(const struct TreeNode *const restrict tn, const union Value val)
{
	if( !tn or !tn->Children )
		return NULL;
	
	for( size_t i=0 ; i<tn->ChildCount ; i++ ) {
		if( tn->Children[i]->Data.Int64==val.Int64 )
			return tn->Children[i];
	}
	return NULL;
}

union Value TreeNode_GetData(const struct TreeNode *const restrict tn)
{
	return tn ? tn->Data : (union Value){0};
}
void TreeNode_SetData(struct TreeNode *const restrict tn, const union Value val)
{
	if( !tn )
		return;
	
	tn->Data = val;
}
struct TreeNode **TreeNode_GetChildren(const struct TreeNode *const restrict tn)
{
	return tn ? tn->Children : NULL;
}
size_t TreeNode_GetChildLen(const struct TreeNode *const restrict tn)
{
	return tn ? tn->ChildLen : 0;
}
size_t TreeNode_GetChildCount(const struct TreeNode *const restrict tn)
{
	return tn ? tn->ChildCount : 0;
}

static void TreeNode_Resize(struct TreeNode *const restrict tn)
{
	if( !tn )
		return;
	
	size_t oldsize = tn->ChildLen;
	tn->ChildLen <<= 1;
	if( !tn->ChildLen )
		tn->ChildLen = 2;
	
	struct TreeNode **newdata = calloc(tn->ChildLen, sizeof *newdata);
	if( !newdata ) {
		tn->ChildLen >>= 1;
		if( tn->ChildLen == 1 )
			tn->ChildLen=0;
		return;
	}
	
	if( tn->Children ) {
		memcpy(newdata, tn->Children, sizeof *newdata * oldsize);
		free(tn->Children);
		tn->Children = NULL;
	}
	tn->Children = newdata;
}

static void TreeNode_Truncate(struct TreeNode *const restrict tn)
{
	if( !tn )
		return;
	
	if( tn->ChildCount < tn->ChildLen>>1 ) {
		tn->ChildLen >>= 1;
		struct TreeNode **newdata = calloc(tn->ChildLen, sizeof *newdata);
		if( !newdata )
			return;
		
		if( tn->Children ) {
			memcpy(newdata, tn->Children, sizeof *newdata * tn->ChildLen);
			free(tn->Children);
			tn->Children = NULL;
		}
		tn->Children = newdata;
	}
}
