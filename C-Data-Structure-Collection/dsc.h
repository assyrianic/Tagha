#pragma once

#ifdef __cplusplus
extern "C" {
	#ifndef restrict
		#define restrict __restrict
	#endif
#endif

#if defined(_WIN32) || defined(_WIN64)
	#ifndef OS_WINDOWS
		#define OS_WINDOWS 1
	#endif
#elif defined(unix) || defined(__unix) || defined(__unix__) || defined(__linux__) || defined(linux) || defined(__linux) || defined(__FreeBSD__)
	#ifndef OS_LINUX_UNIX
		#define OS_LINUX_UNIX 1
	#endif
#elif defined(__ANDROID__)
	#ifndef OS_ANDROID
		#define OS_ANDROID 1
	#endif
#else
	#ifndef OS_MAC
		#define OS_MAC 1
	#endif
#endif

#if defined(__clang__)
	#ifndef COMPILER_CLANG
		#define COMPILER_CLANG
	#endif
#elif defined(__GNUC__) || defined(__GNUG__)
	#ifndef COMPILER_GCC
		#define COMPILER_GCC
	#endif
#elif defined(_MSC_VER)
	#ifndef COMPILER_MSVC
		#define COMPILER_MSVC
	#endif
#endif


#ifndef DSC_EXPORT
	#ifdef OS_WINDOWS
		#define DSC_EXPORT __declspec(dllexport)
	#else
		#define DSC_EXPORT extern
	#endif
#endif

#ifndef DSC_IMPORT
	#ifdef OS_WINDOWS
		#define DSC_IMPORT __declspec(dllimport)
	#else
		#define DSC_IMPORT
	#endif
#endif

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <iso646.h>


#ifndef RAII_DTOR
	#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
		#define RAII_DTOR(func) __attribute__ ((cleanup((func))))
	#else
		#define RAII_DTOR(func)
	#endif
#endif
	
#ifndef NEVER_NULL
	#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
		#define NEVER_NULL(...) __attribute__( (nonnull(__VA_ARGS__)) )
	#else
		#define NEVER_NULL(...)
	#endif
#endif
	
#ifndef NO_NULL
	#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
		#define NO_NULL __attribute__((nonnull))
	#else
		#define NO_NULL
	#endif
#endif
	
#ifndef NONNULL_RET
	#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
		#define NONNULL_RET __attribute__((returns_nonnull))
	#else
		#define NONNULL_RET
	#endif
#endif

typedef bool fnDestructor(void *); // use pointer to a pointer.

struct Variant;
struct String;
struct Vector;
struct Hashmap;
struct UniLinkedList;
struct BiLinkedList;
struct ByteBuffer;
struct Tuple;
struct Graph;
struct TreeNode;
struct LinkMap;
struct MemPool;

union Value {
	bool Bool, BoolArray[8], *BoolPtr;
	int8_t Int8, Int8Array[8], *Int8Ptr;
	int16_t Int16, Int16Array[4], *Int16Ptr;
	int32_t Int32, Int32Array[2], *Int32Ptr;
	int64_t Int64, *Int64Ptr;
	
	uint8_t UInt8, UInt8Array[8], *UInt8Ptr;
	uint16_t UInt16, UInt16Array[4], *UInt16Ptr;
	uint32_t UInt32, UInt32Array[2], *UInt32Ptr;
	uint64_t UInt64, *UInt64Ptr;
	size_t IntSize, *IntSizePtr;
	
	float Float, FloatArray[2], *FloatPtr;
	double Double, *DoublePtr;
	
	void *Ptr;
	union Value *SelfPtr;
	struct Variant *VarPtr;
	struct String *StrObjPtr;
	struct Vector *VecPtr;
	struct Hashmap *MapPtr;
	struct UniLinkedList *UniListPtr;
	struct BiLinkedList *BiListPtr;
	struct ByteBuffer *ByteBufferPtr;
	struct Tuple *TuplePtr;
	struct Graph *GraphPtr;
	struct TreeNode *TreeNodePtr;
	struct LinkMap *LinkMapPtr;
};

// discriminated union type
struct Variant {
	union Value Val;
	int32_t TypeTag;
};


/************* C++ Style Automated String (stringobj.c) *************/
struct String {
	char *CStr;
	size_t Len;
};

struct String *String_New(void);
struct String *String_NewStr(const char *);
void String_Del(struct String *);
bool String_Free(struct String **);
void String_Init(struct String *);
void String_InitStr(struct String *, const char *);
void String_AddChar(struct String *, char);
void String_Add(struct String *, const struct String *);
void String_AddStr(struct String *, const char *);
char *String_GetStr(const struct String *);
size_t String_Len(const struct String *);
void String_Copy(struct String *, const struct String *);
void String_CopyStr(struct String *, const char *);
int32_t String_Format(struct String *, const char *, ...);
int32_t String_CmpCStr(const struct String *, const char *);
int32_t String_CmpStr(const struct String *, const struct String *);
int32_t String_NCmpCStr(const struct String *, const char *, size_t);
int32_t String_NCmpStr(const struct String *, const struct String *, size_t);
bool String_IsEmpty(const struct String *);
bool String_Reserve(struct String *, size_t);
char *String_fgets(struct String *, size_t, FILE *);
/***************/


/************* Vector / Dynamic Array (vector.c) *************/
struct Vector {
	union Value *Table;
	size_t Len, Count;
};

struct Vector *Vector_New(void);
void Vector_Init(struct Vector *);
void Vector_Del(struct Vector *, fnDestructor *);
void Vector_Free(struct Vector **, fnDestructor *);

size_t Vector_Len(const struct Vector *);
size_t Vector_Count(const struct Vector *);
union Value *Vector_GetTable(const struct Vector *);
void Vector_Resize(struct Vector *);
void Vector_Truncate(struct Vector *);

bool Vector_Insert(struct Vector *, union Value);
union Value Vector_Pop(struct Vector *);
union Value Vector_Get(const struct Vector *, size_t);
void Vector_Set(struct Vector *, size_t, union Value);

void Vector_Delete(struct Vector *, size_t, fnDestructor *);
void Vector_Add(struct Vector *, const struct Vector *);
void Vector_Copy(struct Vector *, const struct Vector *);

void Vector_FromUniLinkedList(struct Vector *, const struct UniLinkedList *);
void Vector_FromBiLinkedList(struct Vector *, const struct BiLinkedList *);
void Vector_FromMap(struct Vector *, const struct Hashmap *);
void Vector_FromTuple(struct Vector *, const struct Tuple *);
void Vector_FromGraph(struct Vector *, const struct Graph *);
void Vector_FromLinkMap(struct Vector *, const struct LinkMap *);

struct Vector *Vector_NewFromUniLinkedList(const struct UniLinkedList *);
struct Vector *Vector_NewFromBiLinkedList(const struct BiLinkedList *);
struct Vector *Vector_NewFromMap(const struct Hashmap *);
struct Vector *Vector_NewFromTuple(const struct Tuple *);
struct Vector *Vector_NewFromGraph(const struct Graph *);
struct Vector *Vector_NewFromLinkMap(const struct LinkMap *);
/***************/

/************* Hashmap (hashmap.c) *************/
struct KeyValPair {
	struct String KeyName;
	union Value Data;
};

struct KeyValPair *KeyValPair_New(void);
struct KeyValPair *KeyValPair_NewSP(const char *, union Value);

void KeyValPair_Del(struct KeyValPair *, fnDestructor *);
void KeyValPair_Free(struct KeyValPair **, fnDestructor *);


struct Hashmap {
	struct Vector *Table; // a vector of vectors!
	size_t Len, Count;
};

size_t GenHash(const char *);
struct Hashmap *Map_New(void);
void Map_Init(struct Hashmap *);
void Map_Del(struct Hashmap *, fnDestructor *);
void Map_Free(struct Hashmap **, fnDestructor *);
size_t Map_Count(const struct Hashmap *);
size_t Map_Len(const struct Hashmap *);
bool Map_Rehash(struct Hashmap *);

bool Map_InsertNode(struct Hashmap *, struct KeyValPair *);
bool Map_Insert(struct Hashmap *, const char *, union Value);

union Value Map_Get(const struct Hashmap *, const char *);
void Map_Set(struct Hashmap *, const char *, union Value);

void Map_Delete(struct Hashmap *, const char *, fnDestructor *);
bool Map_HasKey(const struct Hashmap *, const char *);
struct KeyValPair *Map_GetKeyValPair(const struct Hashmap *, const char *);
struct Vector *Map_GetKeyTable(const struct Hashmap *);

void Map_FromUniLinkedList(struct Hashmap *, const struct UniLinkedList *);
void Map_FromBiLinkedList(struct Hashmap *, const struct BiLinkedList *);
void Map_FromVector(struct Hashmap *, const struct Vector *);
void Map_FromTuple(struct Hashmap *, const struct Tuple *);
void Map_FromGraph(struct Hashmap *, const struct Graph *);
void Map_FromLinkMap(struct Hashmap *, const struct LinkMap *);

struct Hashmap *Map_NewFromUniLinkedList(const struct UniLinkedList *);
struct Hashmap *Map_NewFromBiLinkedList(const struct BiLinkedList *);
struct Hashmap *Map_NewFromVector(const struct Vector *);
struct Hashmap *Map_NewFromTuple(const struct Tuple *);
struct Hashmap *Map_NewFromGraph(const struct Graph *);
struct Hashmap *Map_NewFromLinkMap(const struct LinkMap *);
/***************/


/************* Singly Linked List (unilist.c) *************/
struct UniListNode {
	union Value Data;
	struct UniListNode *Next;
};

struct UniListNode *UniListNode_New(void);
struct UniListNode *UniListNode_NewVal(union Value);
void UniListNode_Del(struct UniListNode *, fnDestructor *);
void UniListNode_Free(struct UniListNode **, fnDestructor *);
struct UniListNode *UniListNode_GetNextNode(const struct UniListNode *);
union Value UniListNode_GetValue(const struct UniListNode *);


struct UniLinkedList {
	struct UniListNode *Head, *Tail;
	size_t Len;
};

struct UniLinkedList *UniLinkedList_New(void);
void UniLinkedList_Del(struct UniLinkedList *, fnDestructor *);
void UniLinkedList_Free(struct UniLinkedList **, fnDestructor *);
void UniLinkedList_Init(struct UniLinkedList *);

size_t UniLinkedList_Len(const struct UniLinkedList *);
bool UniLinkedList_InsertNodeAtHead(struct UniLinkedList *, struct UniListNode *);
bool UniLinkedList_InsertNodeAtTail(struct UniLinkedList *, struct UniListNode *);
bool UniLinkedList_InsertNodeAtIndex(struct UniLinkedList *, struct UniListNode *, size_t);
bool UniLinkedList_InsertValueAtHead(struct UniLinkedList *, union Value);
bool UniLinkedList_InsertValueAtTail(struct UniLinkedList *, union Value);
bool UniLinkedList_InsertValueAtIndex(struct UniLinkedList *, union Value, size_t);

struct UniListNode *UniLinkedList_GetNode(const struct UniLinkedList *, size_t);
struct UniListNode *UniLinkedList_GetNodeByValue(const struct UniLinkedList *, union Value);
union Value UniLinkedList_GetValue(const struct UniLinkedList *, size_t);
void UniLinkedList_SetValue(struct UniLinkedList *, size_t, union Value);
bool UniLinkedList_DelNodeByIndex(struct UniLinkedList *, size_t, fnDestructor *);
bool UniLinkedList_DelNodeByRef(struct UniLinkedList *, struct UniListNode **, fnDestructor *);

struct UniListNode *UniLinkedList_GetHead(const struct UniLinkedList *);
struct UniListNode *UniLinkedList_GetTail(const struct UniLinkedList *);

void UniLinkedList_FromBiLinkedList(struct UniLinkedList *, const struct BiLinkedList *);
void UniLinkedList_FromMap(struct UniLinkedList *, const struct Hashmap *);
void UniLinkedList_FromVector(struct UniLinkedList *, const struct Vector *);
void UniLinkedList_FromTuple(struct UniLinkedList *, const struct Tuple *);
void UniLinkedList_FromGraph(struct UniLinkedList *, const struct Graph *);
void UniLinkedList_FromLinkMap(struct UniLinkedList *, const struct LinkMap *);

struct UniLinkedList *UniLinkedList_NewFromBiLinkedList(const struct BiLinkedList *);
struct UniLinkedList *UniLinkedList_NewFromMap(const struct Hashmap *);
struct UniLinkedList *UniLinkedList_NewFromVector(const struct Vector *);
struct UniLinkedList *UniLinkedList_NewFromTuple(const struct Tuple *);
struct UniLinkedList *UniLinkedList_NewFromGraph(const struct Graph *);
struct UniLinkedList *UniLinkedList_NewFromLinkMap(const struct LinkMap *);
/***************/


/************* Doubly Linked List (bilist.c) *************/
struct BiListNode {
	union Value Data;
	struct BiListNode *Next, *Prev;
};

struct BiListNode *BiListNode_New(void);
struct BiListNode *BiListNode_NewVal(union Value);
void BiListNode_Del(struct BiListNode *, fnDestructor *);
void BiListNode_Free(struct BiListNode **, fnDestructor *);
struct BiListNode *BiListNode_GetNextNode(const struct BiListNode *);
struct BiListNode *BiListNode_GetPrevNode(const struct BiListNode *);
union Value BiListNode_GetValue(const struct BiListNode *);


struct BiLinkedList {
	struct BiListNode *Head, *Tail;
	size_t Len;
};

struct BiLinkedList *BiLinkedList_New(void);
void BiLinkedList_Del(struct BiLinkedList *, fnDestructor *);
void BiLinkedList_Free(struct BiLinkedList **,fnDestructor *);
void BiLinkedList_Init(struct BiLinkedList *);

size_t BiLinkedList_Len(const struct BiLinkedList *);
bool BiLinkedList_InsertNodeAtHead(struct BiLinkedList *, struct BiListNode *);
bool BiLinkedList_InsertNodeAtTail(struct BiLinkedList *, struct BiListNode *);
bool BiLinkedList_InsertNodeAtIndex(struct BiLinkedList *, struct BiListNode *, size_t);
bool BiLinkedList_InsertValueAtHead(struct BiLinkedList *, union Value);
bool BiLinkedList_InsertValueAtTail(struct BiLinkedList *, union Value);
bool BiLinkedList_InsertValueAtIndex(struct BiLinkedList *, union Value, size_t);

struct BiListNode *BiLinkedList_GetNode(const struct BiLinkedList *, size_t);
struct BiListNode *BiLinkedList_GetNodeByValue(const struct BiLinkedList *, union Value);
union Value BiLinkedList_GetValue(const struct BiLinkedList *, size_t);
void BiLinkedList_SetValue(struct BiLinkedList *, size_t, union Value);
bool BiLinkedList_DelNodeByIndex(struct BiLinkedList *, size_t, fnDestructor *);
bool BiLinkedList_DelNodeByRef(struct BiLinkedList *, struct BiListNode **, fnDestructor *);

struct BiListNode *BiLinkedList_GetHead(const struct BiLinkedList *);
struct BiListNode *BiLinkedList_GetTail(const struct BiLinkedList *);

void BiLinkedList_FromUniLinkedList(struct BiLinkedList *, const struct UniLinkedList *);
void BiLinkedList_FromMap(struct BiLinkedList *, const struct Hashmap *);
void BiLinkedList_FromVector(struct BiLinkedList *, const struct Vector *);
void BiLinkedList_FromTuple(struct BiLinkedList *, const struct Tuple *);
void BiLinkedList_FromGraph(struct BiLinkedList *, const struct Graph *);
void BiLinkedList_FromLinkMap(struct BiLinkedList *, const struct LinkMap *);

struct BiLinkedList *BiLinkedList_NewFromUniLinkedList(const struct UniLinkedList *);
struct BiLinkedList *BiLinkedList_NewFromMap(const struct Hashmap *);
struct BiLinkedList *BiLinkedList_NewFromVector(const struct Vector *);
struct BiLinkedList *BiLinkedList_NewFromTuple(const struct Tuple *);
struct BiLinkedList *BiLinkedList_NewFromGraph(const struct Graph *);
struct BiLinkedList *BiLinkedList_NewFromLinkMap(const struct LinkMap *);
/***************/


/************* Byte Buffer (bytebuffer.c) *************/
struct ByteBuffer {
	uint8_t *Buffer;
	size_t Len, Count;
};

struct ByteBuffer *ByteBuffer_New(void);
void ByteBuffer_Init(struct ByteBuffer *);
void ByteBuffer_Del(struct ByteBuffer *);
void ByteBuffer_Free(struct ByteBuffer **);
size_t ByteBuffer_Len(const struct ByteBuffer *);
size_t ByteBuffer_Count(const struct ByteBuffer *);
uint8_t *ByteBuffer_GetBuffer(const struct ByteBuffer *);
void ByteBuffer_InsertByte(struct ByteBuffer *, uint8_t);
void ByteBuffer_InsertInt(struct ByteBuffer *, uint64_t, size_t);
void ByteBuffer_InsertFloat(struct ByteBuffer *, float);
void ByteBuffer_InsertDouble(struct ByteBuffer *, double);
void ByteBuffer_InsertString(struct ByteBuffer *, const char *, size_t);
void ByteBuffer_InsertObject(struct ByteBuffer *, const void *, size_t);
void ByteBuffer_InsertZeroes(struct ByteBuffer *, size_t);
void ByteBuffer_Delete(struct ByteBuffer *, size_t);
void ByteBuffer_Resize(struct ByteBuffer *);
void ByteBuffer_DumpToFile(const struct ByteBuffer *, FILE *);
size_t ByteBuffer_ReadFromFile(struct ByteBuffer *, FILE *);
void ByteBuffer_Append(struct ByteBuffer *, struct ByteBuffer *);
/***************/


/************* Tuple (tuple.c) *************/
struct Tuple {
	union Value *Items;
	size_t Len;
};

struct Tuple *Tuple_New(size_t, union Value []);
void Tuple_Free(struct Tuple **);

void Tuple_Init(struct Tuple *, size_t, union Value []);
void Tuple_Del(struct Tuple *);
size_t Tuple_Len(const struct Tuple *);
union Value *Tuple_GetItems(const struct Tuple *);
union Value Tuple_GetItem(const struct Tuple *, size_t);

void Tuple_FromUniLinkedList(struct Tuple *, const struct UniLinkedList *);
void Tuple_FromMap(struct Tuple *, const struct Hashmap *);
void Tuple_FromVector(struct Tuple *, const struct Vector *);
void Tuple_FromBiLinkedList(struct Tuple *, const struct BiLinkedList *);
void Tuple_FromGraph(struct Tuple *, const struct Graph *);
void Tuple_FromLinkMap(struct Tuple *, const struct LinkMap *);

struct Tuple *Tuple_NewFromUniLinkedList(const struct UniLinkedList *);
struct Tuple *Tuple_NewFromMap(const struct Hashmap *);
struct Tuple *Tuple_NewFromVector(const struct Vector *);
struct Tuple *Tuple_NewFromBiLinkedList(const struct BiLinkedList *);
struct Tuple *Tuple_NewFromGraph(const struct Graph *);
struct Tuple *Tuple_NewFromLinkMap(const struct LinkMap *);
/***************/

/************* Memory Pool (mempool.c) *************/
// uncomment 'POOL_NO_MALLOC' if you can't or don't want to use 'malloc/calloc'.
// library will need recompiling though.

//#define POOL_NO_MALLOC

#ifdef POOL_NO_MALLOC
	#ifndef POOL_HEAPSIZE
		#define POOL_HEAPSIZE	(65536)
	#endif
#endif

struct AllocNode {
	size_t Size;
	struct AllocNode *NextFree;
};

struct MemPool {
	uint8_t
#ifdef POOL_NO_MALLOC
	#ifndef POOL_HEAPSIZE
		#error please define 'POOL_HEAPSIZE' with a valid size.
	#else
		HeapMem[POOL_HEAPSIZE],
	#endif
#else
		*HeapMem,
#endif
		*HeapBottom
	;
	size_t HeapSize, FreeNodes;
	struct AllocNode *FreeList;
};

#ifdef POOL_NO_MALLOC
void MemPool_Init(struct MemPool *);
#else
void MemPool_Init(struct MemPool *, size_t);
#endif

void MemPool_Del(struct MemPool *);
void *MemPool_Alloc(struct MemPool *, size_t);
void *MemPool_Realloc(struct MemPool *, void *, size_t);
void MemPool_Dealloc(struct MemPool *, void *);
void MemPool_Destroy(struct MemPool *, void *);
size_t MemPool_Remaining(const struct MemPool *);
size_t MemPool_Size(const struct MemPool *);
struct AllocNode *MemPool_GetFreeList(const struct MemPool *);
bool MemPool_Defrag(struct MemPool *);

/***************/


/************* Graph (Adjacency List) (graph.c) *************/

struct GraphVertex;
struct GraphEdge {
	union Value Weight;
	struct GraphVertex *VertexSocket;
};

struct GraphEdge *GraphEdge_New(void);
struct GraphEdge *GraphEdge_NewVP(union Value, struct GraphVertex *);
void GraphEdge_Del(struct GraphEdge *, fnDestructor *);
void GraphEdge_Free(struct GraphEdge **, fnDestructor *);

union Value GraphEdge_GetWeight(const struct GraphEdge *);
void GraphEdge_SetWeight(struct GraphEdge *, union Value);
struct GraphVertex *GraphEdge_GetVertex(const struct GraphEdge *);
void GraphEdge_SetVertex(struct GraphEdge *, struct GraphVertex *);


struct GraphVertex {
	union {
		struct {
			union Value *Table;
			size_t Len, Count;
		};
		struct Vector Edges;
	};
	union Value Data;
};

struct GraphVertex *GraphVertex_New(union Value);
void GraphVertex_Init(struct GraphVertex *, union Value);
void GraphVertex_Del(struct GraphVertex *, fnDestructor *, fnDestructor *);
void GraphVertex_Free(struct GraphVertex **, fnDestructor *, fnDestructor *);
bool GraphVertex_AddEdge(struct GraphVertex *, struct GraphEdge *);
struct Vector GraphVertex_GetEdges(struct GraphVertex *);
union Value GraphVertex_GetData(const struct GraphVertex *);
void GraphVertex_SetData(struct GraphVertex *, union Value);


struct Graph {
	union {
		struct {
			union Value *Table;
			size_t Len, Count;
		};
		struct Vector Vertices;
	};
};

struct Graph *Graph_New(void);
void Graph_Init(struct Graph *);
void Graph_Del(struct Graph *, fnDestructor *, fnDestructor *);
void Graph_Free(struct Graph **, fnDestructor *, fnDestructor *);

bool Graph_InsertVertexByValue(struct Graph *, union Value);
bool Graph_RemoveVertexByValue(struct Graph *, union Value, fnDestructor *, fnDestructor *);
bool Graph_RemoveVertexByIndex(struct Graph *, size_t, fnDestructor *, fnDestructor *);

bool Graph_InsertEdgeBtwnVerts(struct Graph *, size_t, size_t, union Value);
bool Graph_RemoveEdgeBtwnVerts(struct Graph *, size_t, size_t, fnDestructor *);

struct GraphVertex *Graph_GetVertexByIndex(struct Graph *, size_t);
union Value Graph_GetVertexDataByIndex(struct Graph *, size_t);
void Graph_SetVertexDataByIndex(struct Graph *, size_t, union Value);
struct GraphEdge *Graph_GetEdgeBtwnVertices(struct Graph *, size_t, size_t);

bool Graph_IsVertexAdjacent(struct Graph *, size_t, size_t);
struct Vector Graph_GetVertexNeighbors(struct Graph *, size_t);

struct Vector Graph_GetVertices(struct Graph *);
size_t Graph_GetVerticeCount(const struct Graph *);
size_t Graph_GetEdgeCount(const struct Graph *);
void Graph_Resize(struct Graph *);
void Graph_Truncate(struct Graph *);

void Graph_FromVector(struct Graph *, const struct Vector *);
void Graph_FromMap(struct Graph *, const struct Hashmap *);
void Graph_FromUniLinkedList(struct Graph *, const struct UniLinkedList *);
void Graph_FromBiLinkedList(struct Graph *, const struct BiLinkedList *);
void Graph_FromTuple(struct Graph *, const struct Tuple *);
void Graph_FromLinkMap(struct Graph *, const struct LinkMap *);

struct Graph *Graph_NewFromVector(const struct Vector *);
struct Graph *Graph_NewFromMap(const struct Hashmap *);
struct Graph *Graph_NewFromUniLinkedList(const struct UniLinkedList *);
struct Graph *Graph_NewFromBiLinkedList(const struct BiLinkedList *);
struct Graph *Graph_NewFromTuple(const struct Tuple *);
struct Graph *Graph_NewFromLinkMap(const struct LinkMap *);
/***************/


/************* General Tree (tree.c) *************/
struct TreeNode {
	struct Vector Children;
	union Value Data;
};

struct TreeNode *TreeNode_New(union Value);
void TreeNode_Init(struct TreeNode *);
void TreeNode_InitVal(struct TreeNode *, union Value);
void TreeNode_Del(struct TreeNode *, fnDestructor *);
void TreeNode_Free(struct TreeNode **, fnDestructor *);

bool TreeNode_InsertChildByNode(struct TreeNode *, struct TreeNode *);
bool TreeNode_InsertChildByValue(struct TreeNode *, union Value);

bool TreeNode_RemoveChildByRef(struct TreeNode *, struct TreeNode **, fnDestructor *);
bool TreeNode_RemoveChildByIndex(struct TreeNode *, size_t, fnDestructor *);
bool TreeNode_RemoveChildByValue(struct TreeNode *, union Value, fnDestructor *);

struct TreeNode *TreeNode_GetChildByIndex(const struct TreeNode *, size_t);
struct TreeNode *TreeNode_GetChildByValue(const struct TreeNode *, union Value);
union Value TreeNode_GetData(const struct TreeNode *);
void TreeNode_SetData(struct TreeNode *, union Value);
struct Vector TreeNode_GetChildren(const struct TreeNode *);
size_t TreeNode_GetChildLen(const struct TreeNode *);
size_t TreeNode_GetChildCount(const struct TreeNode *);
/***************/


/************* Ordered Hashmap (linkmap.c) *************/

struct LinkMap {
	union {
		struct {
			struct Vector *Table;
			size_t Len, Count;
		};
		struct Hashmap Map;
	};
	struct Vector Order;
};

struct LinkMap *LinkMap_New(void);
void LinkMap_Init(struct LinkMap *);
void LinkMap_Del(struct LinkMap *, fnDestructor *);
void LinkMap_Free(struct LinkMap **, fnDestructor *);
size_t LinkMap_Count(const struct LinkMap *);
size_t LinkMap_Len(const struct LinkMap *);
bool LinkMap_Rehash(struct LinkMap *);

bool LinkMap_Insert(struct LinkMap *, const char *, union Value);
bool LinkMap_InsertNode(struct LinkMap *, struct KeyValPair *);

struct KeyValPair *LinkMap_GetNodeByIndex(const struct LinkMap *, size_t);
union Value LinkMap_Get(const struct LinkMap *, const char *);
void LinkMap_Set(struct LinkMap *, const char *, union Value);
union Value LinkMap_GetByIndex(const struct LinkMap *, size_t);
void LinkMap_SetByIndex(struct LinkMap *, size_t, union Value);

void LinkMap_Delete(struct LinkMap *, const char *, fnDestructor *);
void LinkMap_DeleteByIndex(struct LinkMap *, size_t, fnDestructor *);
bool LinkMap_HasKey(const struct LinkMap *, const char *);
struct KeyValPair *LinkMap_GetNodeByKey(const struct LinkMap *, const char *);
struct Vector *LinkMap_GetKeyTable(const struct LinkMap *);
size_t LinkMap_GetIndexByName(const struct LinkMap *, const char *);
size_t LinkMap_GetIndexByNode(const struct LinkMap *, struct KeyValPair *);
size_t LinkMap_GetIndexByValue(const struct LinkMap *, union Value);

void LinkMap_FromMap(struct LinkMap *, const struct Hashmap *);
void LinkMap_FromUniLinkedList(struct LinkMap *, const struct UniLinkedList *);
void LinkMap_FromBiLinkedList(struct LinkMap *, const struct BiLinkedList *);
void LinkMap_FromVector(struct LinkMap *, const struct Vector *);
void LinkMap_FromTuple(struct LinkMap *, const struct Tuple *);
void LinkMap_FromGraph(struct LinkMap *, const struct Graph *);

struct LinkMap *LinkMap_NewFromMap(const struct Hashmap *);
struct LinkMap *LinkMap_NewFromUniLinkedList(const struct UniLinkedList *);
struct LinkMap *LinkMap_NewFromBiLinkedList(const struct BiLinkedList *);
struct LinkMap *LinkMap_NewFromVector(const struct Vector *);
struct LinkMap *LinkMap_NewFromTuple(const struct Tuple *);
struct LinkMap *LinkMap_NewFromGraph(const struct Graph *);

/***************/

#ifdef __cplusplus
}
#endif
