#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#if _WIN32 || _WIN64
	#define OS_WINDOWS 1
#elif unix || __unix || __unix__ || __linux__ || linux || __linux || __FreeBSD__
	#define OS_LINUX_UNIX 1
#elif __ANDROID__
	#define OS_ANDROID 1
#else
	#define OS_MAC 1
#endif

#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <iso646.h>

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

union Value {
	bool Bool, *BoolPtr, (*BoolFunc)(), *(*BoolPtrFunc)();
	int8_t Char, *CharPtr, (*CharFunc)(), *(*CharPtrFunc)();
	int16_t Short, *ShortPtr, (*ShortFunc)(), *(*ShortPtrFunc)();
	int32_t Int32, *Int32Ptr, (*Int32Func)(), *(*Int32PtrFunc)();
	int64_t Int64, *Int64Ptr, (*Int64Func)(), *(*Int64PtrFunc)();
	
	uint8_t UChar, *UCharPtr, (*UCharFunc)(), *(*UCharPtrFunc)();
	uint16_t UShort, *UShortPtr, (*UShortFunc)(), *(*UShortPtrFunc)();
	uint32_t UInt32, *UInt32Ptr, (*UInt32Func)(), *(*UInt32PtrFunc)();
	uint64_t UInt64, *UInt64Ptr, (*UInt64Func)(), *(*UInt64PtrFunc)();
	
	float Float, *FloatPtr, (*FloatFunc)(), *(*FloatPtrFunc)();
	double Double, *DoublePtr, (*DoubleFunc)(), *(*DoublePtrFunc)();
	
	void *Ptr, **PtrPtr, (*VoidFunc)(), *(*VoidPtrFunc)();
	union Value *SelfPtr, (*SelfFunc)(), *(*SelfPtrFunc)();
	struct Variant *VariantPtr, (*VariantFunc)(), *(*VariantPtrFunc)();
	struct String *StrObjPtr, (*StrObjFunc)(), *(*StrObjPtrFunc)();
	struct Vector *VecPtr, (*VecFunc)(), *(*VecPtrFunc)();
	struct Hashmap *MapPtr, (*MapFunc)(), *(*MapPtrFunc)();
	struct UniLinkedList *UniListPtr, (*UniListFunc)(), *(*UniListPtrFunc)();
	struct BiLinkedList *BiListPtr, (*BiListFunc)(), *(*BiListPtrFunc)();
	struct ByteBuffer *BufferPtr, (*BufferFunc)(), *(*BufferPtrFunc)();
	struct Tuple *TuplePtr, (*TupleFunc)(), *(*TuplePtrFunc)();
	struct Graph *GraphPtr, (*GraphFunc)(), *(*GraphPtrFunc)();
	struct TreeNode *TreePtr, (*TreeFunc)(), *(*TreePtrFunc)();
	struct LinkMap *LinkMapPtr, (*LinkMapFunc)(), *(*LinkMapPtrFunc)();
};

typedef enum ValType {
	TypeInvalid=0,
	// integer types
	TypeBool, TypeBoolPtr, TypeBoolFunc, TypeBoolPtrFunc,
	TypeChar, TypeCharPtr, TypeCharFunc, TypeCharPtrFunc, 
	TypeShort, TypeShortPtr, TypeShortFunc, TypeShortPtrFunc, 
	TypeInt32, TypeInt32Ptr, TypeInt32Func, TypeInt32PtrFunc, 
	TypeInt64, TypeInt64Ptr, TypeInt64Func, TypeInt64PtrFunc, 
	TypeUChar, TypeUCharPtr, TypeUCharFunc, TypeUCharPtrFunc, 
	TypeUShort, TypeUShortPtr, TypeUShortFunc, TypeUShortPtrFunc, 
	TypeUInt32, TypeUInt32Ptr, TypeUInt32Func, TypeUInt32PtrFunc, 
	TypeUInt64, TypeUInt64Ptr, TypeUInt64Func, TypeUInt64PtrFunc, 
	
	// floating point types
	TypeFloat, TypeFloatPtr, TypeFloatFunc, TypeFloatPtrFunc,
	TypeDouble, TypeDoublePtr, TypeDoubleFunc, TypeDoublePtrFunc,
	// misc.
	TypePtr, TypePtrPtr, TypeVoidFunc, TypePtrFunc,
	TypeSelfPtr, TypeSelfFunc, TypeSelfPtrFunc,
	TypeVariantPtr, TypeVariantFunc, TypeVariantPtrFunc,
	// data structure oriented.
	TypeStrObjPtr, TypeStrObjFunc, TypeStrObjPtrFunc,
	TypeVecPtr, TypeVecFunc, TypeVecPtrFunc,
	TypeMapPtr, TypeMapFunc, TypeMapPtrFunc,
	TypeUniListPtr, TypeUniListFunc, TypeUniListPtrFunc,
	TypeBiListPtr, TypeBiListFunc, TypeBiListPtrFunc,
	TypeBufferPtr, TypeBufferFunc, TypeBufferPtrFunc,
	TypeTuplePtr, TypeTupleFunc, TypeTuplePtrFunc,
	TypeGraphPtr, TypeGraphFunc, TypeGraphPtrFunc,
	TypeTreePtr, TypeTreeFunc, TypeTreePtrFunc,
	TypeLinkMapPtr, TypeLinkMapFunc, TypeLinkMapPtrFunc,
} ValType;

// discriminated union type
struct Variant {
	union Value Val;
	enum ValType TypeTag;
};


/************* C++ Style Automated String (stringobj.c) *************/
struct String {
	char *CStr;
	size_t Len;
};

struct String *String_New(void);
struct String *String_NewStr(const char *);
void String_Del(struct String *);
void String_Free(struct String **);
void String_Init(struct String *);
void String_InitStr(struct String *, const char *);
void String_AddChar(struct String *, char);
void String_Add(struct String *, const struct String *);
void String_AddStr(struct String *, const char *);
char *String_GetStr(const struct String *);
size_t String_Len(const struct String *);
void String_Copy(struct String *, const struct String *);
void String_CopyStr(struct String *, const char *);
int32_t String_StrCmpCStr(const struct String *, const char *);
int32_t String_StrCmpStr(const struct String *, const struct String *);
int32_t String_StrnCmpCStr(const struct String *, const char *, size_t);
int32_t String_StrnCmpStr(const struct String *, const struct String *, size_t);
bool String_IsEmpty(const struct String *);
/***************/


/************* Vector / Dynamic Array (vector.c) *************/
struct Vector {
	union Value *Table;
	size_t Len, Count;
	bool (*Destructor)(/* Type **Obj */);
};

struct Vector *Vector_New(bool (*)());
void Vector_Init(struct Vector *, bool (*)());
void Vector_Del(struct Vector *);
void Vector_Free(struct Vector **);

size_t Vector_Len(const struct Vector *);
size_t Vector_Count(const struct Vector *);
union Value *Vector_GetTable(const struct Vector *);
void Vector_Resize(struct Vector *);
void Vector_Truncate(struct Vector *);

bool Vector_Insert(struct Vector *, union Value);
union Value Vector_Get(const struct Vector *, size_t);
void Vector_Set(struct Vector *, size_t, union Value);
void Vector_SetItemDestructor(struct Vector *, bool (*)());

void Vector_Delete(struct Vector *, size_t);
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
struct KeyNode {
	struct String KeyName;
	union Value Data;
	struct KeyNode *Next;
};

struct KeyNode *KeyNode_New(void);
struct KeyNode *KeyNode_NewSP(const char *, union Value);

void KeyNode_Del(struct KeyNode *, bool(*)());
bool KeyNode_Free(struct KeyNode **, bool(*)());


struct Hashmap {
	struct KeyNode **Table;
	size_t Len, Count;
	bool (*Destructor)(/* Type **Obj */);
};

struct Hashmap *Map_New(bool (*)());
void Map_Init(struct Hashmap *, bool (*)());
void Map_Del(struct Hashmap *);
void Map_Free(struct Hashmap **);
size_t Map_Count(const struct Hashmap *);
size_t Map_Len(const struct Hashmap *);
bool Map_Rehash(struct Hashmap *);

bool Map_InsertNode(struct Hashmap *, struct KeyNode *);
bool Map_Insert(struct Hashmap *, const char *, union Value);

union Value Map_Get(const struct Hashmap *, const char *);
void Map_Set(struct Hashmap *, const char *, union Value);
void Map_SetItemDestructor(struct Hashmap *, bool (*)());

void Map_Delete(struct Hashmap *, const char *);
bool Map_HasKey(const struct Hashmap *, const char *);
struct KeyNode *Map_GetKeyNode(const struct Hashmap *, const char *);
struct KeyNode **Map_GetKeyTable(const struct Hashmap *);

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
void UniListNode_Del(struct UniListNode *, bool(*)());
void UniListNode_Free(struct UniListNode **, bool(*)());
struct UniListNode *UniListNode_GetNextNode(const struct UniListNode *);
union Value UniListNode_GetValue(const struct UniListNode *);


struct UniLinkedList {
	struct UniListNode *Head, *Tail;
	size_t Len;
	bool (*Destructor)(/* Type **Obj */);
};

struct UniLinkedList *UniLinkedList_New(bool (*)());
void UniLinkedList_Del(struct UniLinkedList *);
void UniLinkedList_Free(struct UniLinkedList **);
void UniLinkedList_Init(struct UniLinkedList *, bool (*)());

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
bool UniLinkedList_DelNodeByIndex(struct UniLinkedList *, size_t);
bool UniLinkedList_DelNodeByRef(struct UniLinkedList *, struct UniListNode **);
void UniLinkedList_SetDestructor(struct UniLinkedList *, bool (*)());

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
void BiListNode_Del(struct BiListNode *, bool(*)());
void BiListNode_Free(struct BiListNode **, bool(*)());
struct BiListNode *BiListNode_GetNextNode(const struct BiListNode *);
struct BiListNode *BiListNode_GetPrevNode(const struct BiListNode *);
union Value BiListNode_GetValue(const struct BiListNode *);


struct BiLinkedList {
	struct BiListNode *Head, *Tail;
	size_t Len;
	bool (*Destructor)(/* Type **Obj */);
};

struct BiLinkedList *BiLinkedList_New(bool (*)());
void BiLinkedList_Del(struct BiLinkedList *);
void BiLinkedList_Free(struct BiLinkedList **);
void BiLinkedList_Init(struct BiLinkedList *, bool (*)());

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
bool BiLinkedList_DelNodeByIndex(struct BiLinkedList *, size_t);
bool BiLinkedList_DelNodeByRef(struct BiLinkedList *, struct BiListNode **);
void BiLinkedList_SetDestructor(struct BiLinkedList *, bool (*)());

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
void ByteBuffer_Delete(struct ByteBuffer *, size_t);
void ByteBuffer_Resize(struct ByteBuffer *);
void ByteBuffer_DumpToFile(/* struct ByteBuffer *, FILE * */);
size_t ByteBuffer_ReadFromFile(/* struct ByteBuffer *, FILE * */);
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

/************* Heap Memory Pool (heap.c) *************/
// uncomment 'DSC_NO_MALLOC' if you can't or don't want to use 'malloc/calloc'.
//#define DSC_NO_MALLOC
#ifdef DSC_NO_MALLOC
	#define DSC_HEAPSIZE	(1<<10)
#endif

struct AllocNode {
	size_t Size;
	struct AllocNode *NextFree;
};

struct Heap {
	uint8_t
#ifdef DSC_NO_MALLOC
		HeapMem[DSC_HEAPSIZE],
#else
		*HeapMem,
#endif
		*HeapBottom
	;
	struct AllocNode *FreeList;
	size_t HeapSize;
};

#ifdef DSC_NO_MALLOC
void Heap_Init(struct Heap *);
#else
void Heap_Init(struct Heap *, size_t);
#endif

void Heap_Del(struct Heap *);
void *Heap_Alloc(struct Heap *, size_t);
void Heap_Release(struct Heap *, void *);
size_t Heap_Remaining(const struct Heap *);
size_t Heap_Size(const struct Heap *);
struct AllocNode *Heap_GetFreeList(const struct Heap *);
/***************/


/************* Graph (Adjacency List) (graph.c) *************/
struct GraphVertex;
struct GraphEdge {
	union Value Weight;
	struct GraphEdge *NextEdge;
	struct GraphVertex *VertexSocket;
};

struct GraphEdge *GraphEdge_New(void);
struct GraphEdge *GraphEdge_NewVP(union Value, struct GraphVertex *);
void GraphEdge_Del(struct GraphEdge *, bool (*)());
void GraphEdge_Free(struct GraphEdge **, bool (*)());

union Value GraphEdge_GetWeight(const struct GraphEdge *);
void GraphEdge_SetWeight(struct GraphEdge *, union Value);
struct GraphVertex *GraphEdge_GetVertex(const struct GraphEdge *);
void GraphEdge_SetVertex(struct GraphEdge *, struct GraphVertex *);


struct GraphVertex {
	struct GraphEdge *EdgeHead, *EdgeTail;
	size_t EdgeLen;
	union Value Data;
};

void GraphVertex_Del(struct GraphVertex *, bool (*)(), bool (*)());
struct GraphEdge *GraphVertex_GetEdges(struct GraphVertex *);
union Value GraphVertex_GetData(const struct GraphVertex *);
void GraphVertex_SetData(struct GraphVertex *, union Value);


struct Graph {
	struct GraphVertex *Vertices;
	size_t VertexLen, VertexCount;
	bool
		(*VertexDestructor)(/* Type **Obj */),
		(*EdgeDestructor)(/* Type **Obj */)
	;
};

struct Graph *Graph_New(bool (*)(), bool (*)());
void Graph_Init(struct Graph *, bool (*)(), bool (*)());
void Graph_Del(struct Graph *);
void Graph_Free(struct Graph **);

bool Graph_InsertVertexByValue(struct Graph *, union Value);
bool Graph_RemoveVertexByValue(struct Graph *, union Value);
bool Graph_RemoveVertexByIndex(struct Graph *, size_t);

bool Graph_InsertEdgeBtwnVerts(struct Graph *, size_t, size_t, union Value);
bool Graph_RemoveEdgeBtwnVerts(struct Graph *, size_t, size_t);

struct GraphVertex *Graph_GetVertexByIndex(struct Graph *, size_t);
union Value Graph_GetVertexDataByIndex(struct Graph *, size_t);
void Graph_SetVertexDataByIndex(struct Graph *, size_t, union Value);
struct GraphEdge *Graph_GetEdgeBtwnVertices(struct Graph *, size_t, size_t);

bool Graph_IsVertexAdjacent(struct Graph *, size_t, size_t);
struct GraphEdge *Graph_GetVertexNeighbors(struct Graph *, size_t);

struct GraphVertex *Graph_GetVertexArray(struct Graph *);
size_t Graph_GetVerticeCount(const struct Graph *);
size_t Graph_GetEdgeCount(const struct Graph *);
void Graph_SetItemDestructors(struct Graph *, bool (*)(), bool (*)());
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
	struct TreeNode **Children;
	size_t ChildLen, ChildCount;
	union Value Data;
};

struct TreeNode *TreeNode_New(union Value);
void TreeNode_Init(struct TreeNode *);
void TreeNode_InitVal(struct TreeNode *, union Value);
void TreeNode_Del(struct TreeNode *, bool(*)());
void TreeNode_Free(struct TreeNode **, bool(*)());

bool TreeNode_InsertChildByNode(struct TreeNode *, struct TreeNode *);
bool TreeNode_InsertChildByValue(struct TreeNode *, union Value);

bool TreeNode_RemoveChildByRef(struct TreeNode *, struct TreeNode **, bool(*)());
bool TreeNode_RemoveChildByIndex(struct TreeNode *, size_t, bool(*)());
bool TreeNode_RemoveChildByValue(struct TreeNode *, union Value, bool(*)());

struct TreeNode *TreeNode_GetChildByIndex(const struct TreeNode *, size_t);
struct TreeNode *TreeNode_GetChildByValue(const struct TreeNode *, union Value);
union Value TreeNode_GetData(const struct TreeNode *);
void TreeNode_SetData(struct TreeNode *, union Value);
struct TreeNode **TreeNode_GetChildren(const struct TreeNode *);
size_t TreeNode_GetChildLen(const struct TreeNode *);
size_t TreeNode_GetChildCount(const struct TreeNode *);
/***************/


/************* C Plugin Architecture (pluginarch.c) *************/
struct PluginData {
	// hashmap for functions or global vars.
	struct Hashmap Symbols;
	struct String
		Name,	// plugin name
		Version,	// plugin version
		Author,	// plugin author's name (and possible contact info)
		RunTimeName, // plugin runtime/file name.
		Descr	// plugin description of operations.
	;
#if _WIN32 || _WIN64
	HMODULE ModulePtr;
#else
	void *ModulePtr;
#endif
};

struct PluginData *Plugin_New(void);
void Plugin_Del(struct PluginData *);
void Plugin_Free(struct PluginData **);
const char *Plugin_GetName(const struct PluginData *);
const char *Plugin_GetVersion(const struct PluginData *);
const char *Plugin_GetAuthor(const struct PluginData *);
const char *Plugin_GetRuntimeName(const struct PluginData *);
const char *Plugin_GetDescription(const struct PluginData *);

void Plugin_SetName(struct PluginData *, const char *);
void Plugin_SetVersion(struct PluginData *, const char *);
void Plugin_SetAuthor(struct PluginData *, const char *);
void Plugin_SetDescription(struct PluginData *, const char *);

void *Plugin_GetModulePtr(const struct PluginData *);
void *Plugin_GetExportedSymbol(const struct PluginData *, const char *);


struct PluginManager {
	struct Hashmap ModuleMap;
	struct Vector ModuleVec; // purpose of vector is for knowing insertion order.
	struct String PluginDir;
};

struct PluginManager *PluginManager_New(const char *);
void PluginManager_Init(struct PluginManager *, const char *);
void PluginManager_Del(struct PluginManager *);
void PluginManager_Free(struct PluginManager **);

bool PluginManager_LoadModule(struct PluginManager *, const char *, size_t, void *[]);
bool PluginManager_ReloadModule(struct PluginManager *, const char *, size_t, void *[]);
bool PluginManager_ReloadAllModules(struct PluginManager *, size_t, void *[]);
bool PluginManager_UnloadModule(struct PluginManager *, const char *, size_t, void *[]);
bool PluginManager_UnloadAllModules(struct PluginManager *, size_t, void *[]);

/***************/


/************* Ordered Hashmap (linkmap.c) *************/

struct LinkNode {
	struct String KeyName;
	union Value Data;
	struct LinkNode *Next, *After, *Before;
};

struct LinkNode *LinkNode_New(void);
struct LinkNode *LinkNode_NewSP(const char *, union Value);

void LinkNode_Del(struct LinkNode *, bool(*)());
bool LinkNode_Free(struct LinkNode **, bool(*)());


struct LinkMap {
	struct LinkNode **Table, *Head, *Tail;
	size_t Len, Count;
	bool (*Destructor)(/* Type **Obj */);
};

struct LinkMap *LinkMap_New(bool (*)());
void LinkMap_Init(struct LinkMap *, bool (*)());
void LinkMap_Del(struct LinkMap *);
void LinkMap_Free(struct LinkMap **);
size_t LinkMap_Count(const struct LinkMap *);
size_t LinkMap_Len(const struct LinkMap *);
bool LinkMap_Rehash(struct LinkMap *);

bool LinkMap_InsertNode(struct LinkMap *, struct LinkNode *);
bool LinkMap_Insert(struct LinkMap *, const char *, union Value);

struct LinkNode *LinkMap_GetNodeByIndex(const struct LinkMap *, size_t);
union Value LinkMap_Get(const struct LinkMap *, const char *);
void LinkMap_Set(struct LinkMap *, const char *, union Value);
union Value LinkMap_GetByIndex(const struct LinkMap *, size_t);
void LinkMap_SetByIndex(struct LinkMap *, size_t, union Value);
void LinkMap_SetItemDestructor(struct LinkMap *, bool (*)());

void LinkMap_Delete(struct LinkMap *, const char *);
void LinkMap_DeleteByIndex(struct LinkMap *, size_t);
bool LinkMap_HasKey(const struct LinkMap *, const char *);
struct LinkNode *LinkMap_GetNodeByKey(const struct LinkMap *, const char *);
struct LinkNode **LinkMap_GetKeyTable(const struct LinkMap *);

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
