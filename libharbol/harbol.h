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

/*
#ifndef HARBOL_EXPORT
	#ifdef OS_WINDOWS
		#define HARBOL_EXPORT __declspec(dllexport)
	#else
		#define HARBOL_EXPORT
	#endif
#endif

#ifndef HARBOL_IMPORT
	#ifdef OS_WINDOWS
		#define HARBOL_IMPORT __declspec(dllimport)
	#else
		#define HARBOL_IMPORT
	#endif
#endif
*/

#ifdef HARBOL_DLL
	#ifndef HARBOL_LIB 
		#define HARBOL_EXPORT __declspec(dllimport)
	#else
		#define HARBOL_EXPORT __declspec(dllexport)
	#endif
#else
	#define HARBOL_EXPORT 
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

struct HarbolVariant;
struct HarbolString;
struct HarbolVector;
struct HarbolKeyValPair;
struct HarbolHashmap;
struct HarbolUniList;
struct HarbolBiList;
struct HarbolByteBuffer;
struct HarbolTuple;
struct HarbolGraph;
struct HarbolTree;
struct HarbolLinkMap;
struct HarbolMemoryPool;

typedef union HarbolValue {
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
	
	intptr_t IntNative;
	uintptr_t UIntNative;
	
	float Float, FloatArray[2], *FloatPtr;
	double Double, *DoublePtr;
	
	void *Ptr;
	union HarbolValue *SelfPtr;
	struct HarbolVariant *VarPtr;
	struct HarbolString *StrObjPtr;
	struct HarbolVector *VecPtr;
	struct HarbolKeyValPair *KvPairPtr;
	struct HarbolHashmap *MapPtr;
	struct HarbolUniList *UniListPtr;
	struct HarbolBiList *BiListPtr;
	struct HarbolByteBuffer *ByteBufferPtr;
	struct HarbolTuple *TuplePtr;
	struct HarbolGraph *GraphPtr;
	struct HarbolTree *TreePtr;
	struct HarbolLinkMap *LinkMapPtr;
} HarbolValue;

typedef union HarbolValue fnTypeSelector(void *); // use pointer to a data structure or something.


/************* C++ Style Automated HarbolString (stringobj.c) *************/
typedef struct HarbolString {
	char *CStr;
	size_t Len;
} HarbolString;

HARBOL_EXPORT struct HarbolString *HarbolString_New(void);
HARBOL_EXPORT struct HarbolString *HarbolString_NewStr(const char *);
HARBOL_EXPORT void HarbolString_Del(struct HarbolString *);
HARBOL_EXPORT bool HarbolString_Free(struct HarbolString **);
HARBOL_EXPORT void HarbolString_Init(struct HarbolString *);
HARBOL_EXPORT void HarbolString_InitStr(struct HarbolString *, const char *);
HARBOL_EXPORT void HarbolString_AddChar(struct HarbolString *, char);
HARBOL_EXPORT void HarbolString_Add(struct HarbolString *, const struct HarbolString *);
HARBOL_EXPORT void HarbolString_AddStr(struct HarbolString *, const char *);
HARBOL_EXPORT char *HarbolString_GetStr(const struct HarbolString *);
HARBOL_EXPORT size_t HarbolString_Len(const struct HarbolString *);
HARBOL_EXPORT void HarbolString_Copy(struct HarbolString *, const struct HarbolString *);
HARBOL_EXPORT void HarbolString_CopyStr(struct HarbolString *, const char *);
HARBOL_EXPORT int32_t HarbolString_Format(struct HarbolString *, const char *, ...);
HARBOL_EXPORT int32_t HarbolString_CmpCStr(const struct HarbolString *, const char *);
HARBOL_EXPORT int32_t HarbolString_CmpStr(const struct HarbolString *, const struct HarbolString *);
HARBOL_EXPORT int32_t HarbolString_NCmpCStr(const struct HarbolString *, const char *, size_t);
HARBOL_EXPORT int32_t HarbolString_NCmpStr(const struct HarbolString *, const struct HarbolString *, size_t);
HARBOL_EXPORT bool HarbolString_IsEmpty(const struct HarbolString *);
HARBOL_EXPORT bool HarbolString_Reserve(struct HarbolString *, size_t);
HARBOL_EXPORT char *HarbolString_fgets(struct HarbolString *, FILE *);
HARBOL_EXPORT void HarbolString_Empty(struct HarbolString *);
/***************/


/************* HarbolVector / Dynamic Array (vector.c) *************/
typedef struct HarbolVector {
	union HarbolValue *Table;
	size_t Len, Count;
} HarbolVector;

HARBOL_EXPORT struct HarbolVector *HarbolVector_New(void);
HARBOL_EXPORT void HarbolVector_Init(struct HarbolVector *);
HARBOL_EXPORT void HarbolVector_Del(struct HarbolVector *, fnDestructor *);
HARBOL_EXPORT void HarbolVector_Free(struct HarbolVector **, fnDestructor *);

HARBOL_EXPORT size_t HarbolVector_Len(const struct HarbolVector *);
HARBOL_EXPORT size_t HarbolVector_Count(const struct HarbolVector *);
HARBOL_EXPORT union HarbolValue *HarbolVector_GetIter(const struct HarbolVector *);
HARBOL_EXPORT union HarbolValue *HarbolVector_GetIterEndLen(const struct HarbolVector *);
HARBOL_EXPORT union HarbolValue *HarbolVector_GetIterEndCount(const struct HarbolVector *);
HARBOL_EXPORT void HarbolVector_Resize(struct HarbolVector *);
HARBOL_EXPORT void HarbolVector_Truncate(struct HarbolVector *);

HARBOL_EXPORT bool HarbolVector_Insert(struct HarbolVector *, union HarbolValue);
HARBOL_EXPORT union HarbolValue HarbolVector_Pop(struct HarbolVector *);
HARBOL_EXPORT union HarbolValue HarbolVector_Get(const struct HarbolVector *, size_t);
HARBOL_EXPORT void HarbolVector_Set(struct HarbolVector *, size_t, union HarbolValue);

HARBOL_EXPORT void HarbolVector_Delete(struct HarbolVector *, size_t, fnDestructor *);
HARBOL_EXPORT void HarbolVector_Add(struct HarbolVector *, const struct HarbolVector *);
HARBOL_EXPORT void HarbolVector_Copy(struct HarbolVector *, const struct HarbolVector *);

HARBOL_EXPORT void HarbolVector_FromHarbolUniList(struct HarbolVector *, const struct HarbolUniList *);
HARBOL_EXPORT void HarbolVector_FromHarbolBiList(struct HarbolVector *, const struct HarbolBiList *);
HARBOL_EXPORT void HarbolVector_FromHarbolHashmap(struct HarbolVector *, const struct HarbolHashmap *);
HARBOL_EXPORT void HarbolVector_FromHarbolTuple(struct HarbolVector *, const struct HarbolTuple *);
HARBOL_EXPORT void HarbolVector_FromHarbolGraph(struct HarbolVector *, const struct HarbolGraph *);
HARBOL_EXPORT void HarbolVector_FromHarbolLinkMap(struct HarbolVector *, const struct HarbolLinkMap *);

HARBOL_EXPORT struct HarbolVector *HarbolVector_NewFromHarbolUniList(const struct HarbolUniList *);
HARBOL_EXPORT struct HarbolVector *HarbolVector_NewFromHarbolBiList(const struct HarbolBiList *);
HARBOL_EXPORT struct HarbolVector *HarbolVector_NewFromHarbolHashmap(const struct HarbolHashmap *);
HARBOL_EXPORT struct HarbolVector *HarbolVector_NewFromHarbolTuple(const struct HarbolTuple *);
HARBOL_EXPORT struct HarbolVector *HarbolVector_NewFromHarbolGraph(const struct HarbolGraph *);
HARBOL_EXPORT struct HarbolVector *HarbolVector_NewFromHarbolLinkMap(const struct HarbolLinkMap *);
/***************/

/************* HarbolHashmap (hashmap.c) *************/
typedef struct HarbolKeyValPair {
	struct HarbolString KeyName;
	union HarbolValue Data;
} HarbolKeyValPair;

HARBOL_EXPORT struct HarbolKeyValPair *HarbolKeyValPair_New(void);
HARBOL_EXPORT struct HarbolKeyValPair *HarbolKeyValPair_NewSP(const char *, union HarbolValue);

HARBOL_EXPORT void HarbolKeyValPair_Del(struct HarbolKeyValPair *, fnDestructor *);
HARBOL_EXPORT void HarbolKeyValPair_Free(struct HarbolKeyValPair **, fnDestructor *);


typedef struct HarbolHashmap {
	struct HarbolVector *Table; // a vector of vectors!
	size_t Len, Count;
} HarbolHashmap;

HARBOL_EXPORT size_t GenHash(const char *);
HARBOL_EXPORT struct HarbolHashmap *HarbolMap_New(void);
HARBOL_EXPORT void HarbolMap_Init(struct HarbolHashmap *);
HARBOL_EXPORT void HarbolMap_Del(struct HarbolHashmap *, fnDestructor *);
HARBOL_EXPORT void HarbolMap_Free(struct HarbolHashmap **, fnDestructor *);
HARBOL_EXPORT size_t HarbolMap_Count(const struct HarbolHashmap *);
HARBOL_EXPORT size_t HarbolMap_Len(const struct HarbolHashmap *);
HARBOL_EXPORT bool HarbolMap_Rehash(struct HarbolHashmap *);

HARBOL_EXPORT bool HarbolMap_InsertNode(struct HarbolHashmap *, struct HarbolKeyValPair *);
HARBOL_EXPORT bool HarbolMap_Insert(struct HarbolHashmap *, const char *, union HarbolValue);

HARBOL_EXPORT union HarbolValue HarbolMap_Get(const struct HarbolHashmap *, const char *);
HARBOL_EXPORT void HarbolMap_Set(struct HarbolHashmap *, const char *, union HarbolValue);

HARBOL_EXPORT void HarbolMap_Delete(struct HarbolHashmap *, const char *, fnDestructor *);
HARBOL_EXPORT bool HarbolMap_HasKey(const struct HarbolHashmap *, const char *);
HARBOL_EXPORT struct HarbolKeyValPair *HarbolMap_GetHarbolKeyValPair(const struct HarbolHashmap *, const char *);
struct HarbolVector *HarbolMap_GetBuckets(const struct HarbolHashmap *);

HARBOL_EXPORT void HarbolMap_FromHarbolUniList(struct HarbolHashmap *, const struct HarbolUniList *);
HARBOL_EXPORT void HarbolMap_FromHarbolBiList(struct HarbolHashmap *, const struct HarbolBiList *);
HARBOL_EXPORT void HarbolMap_FromHarbolVector(struct HarbolHashmap *, const struct HarbolVector *);
HARBOL_EXPORT void HarbolMap_FromHarbolTuple(struct HarbolHashmap *, const struct HarbolTuple *);
HARBOL_EXPORT void HarbolMap_FromHarbolGraph(struct HarbolHashmap *, const struct HarbolGraph *);
HARBOL_EXPORT void HarbolMap_FromHarbolLinkMap(struct HarbolHashmap *, const struct HarbolLinkMap *);

HARBOL_EXPORT struct HarbolHashmap *HarbolMap_NewFromHarbolUniList(const struct HarbolUniList *);
HARBOL_EXPORT struct HarbolHashmap *HarbolMap_NewFromHarbolBiList(const struct HarbolBiList *);
HARBOL_EXPORT struct HarbolHashmap *HarbolMap_NewFromHarbolVector(const struct HarbolVector *);
HARBOL_EXPORT struct HarbolHashmap *HarbolMap_NewFromHarbolTuple(const struct HarbolTuple *);
HARBOL_EXPORT struct HarbolHashmap *HarbolMap_NewFromHarbolGraph(const struct HarbolGraph *);
HARBOL_EXPORT struct HarbolHashmap *HarbolMap_NewFromHarbolLinkMap(const struct HarbolLinkMap *);
/***************/


/************* Singly Linked List (unilist.c) *************/
typedef struct HarbolUniListNode {
	union HarbolValue Data;
	struct HarbolUniListNode *Next;
} HarbolUniListNode;

HARBOL_EXPORT struct HarbolUniListNode *HarbolUniListNode_New(void);
HARBOL_EXPORT struct HarbolUniListNode *HarbolUniListNode_NewVal(union HarbolValue);
HARBOL_EXPORT void HarbolUniListNode_Del(struct HarbolUniListNode *, fnDestructor *);
HARBOL_EXPORT void HarbolUniListNode_Free(struct HarbolUniListNode **, fnDestructor *);
HARBOL_EXPORT struct HarbolUniListNode *HarbolUniListNode_GetNextNode(const struct HarbolUniListNode *);
HARBOL_EXPORT union HarbolValue HarbolUniListNode_GetValue(const struct HarbolUniListNode *);


typedef struct HarbolUniList {
	struct HarbolUniListNode *Head, *Tail;
	size_t Len;
} HarbolUniList;

HARBOL_EXPORT struct HarbolUniList *HarbolUniList_New(void);
HARBOL_EXPORT void HarbolUniList_Del(struct HarbolUniList *, fnDestructor *);
HARBOL_EXPORT void HarbolUniList_Free(struct HarbolUniList **, fnDestructor *);
HARBOL_EXPORT void HarbolUniList_Init(struct HarbolUniList *);

HARBOL_EXPORT size_t HarbolUniList_Len(const struct HarbolUniList *);
HARBOL_EXPORT bool HarbolUniList_InsertNodeAtHead(struct HarbolUniList *, struct HarbolUniListNode *);
HARBOL_EXPORT bool HarbolUniList_InsertNodeAtTail(struct HarbolUniList *, struct HarbolUniListNode *);
HARBOL_EXPORT bool HarbolUniList_InsertNodeAtIndex(struct HarbolUniList *, struct HarbolUniListNode *, size_t);
HARBOL_EXPORT bool HarbolUniList_InsertValueAtHead(struct HarbolUniList *, union HarbolValue);
HARBOL_EXPORT bool HarbolUniList_InsertValueAtTail(struct HarbolUniList *, union HarbolValue);
HARBOL_EXPORT bool HarbolUniList_InsertValueAtIndex(struct HarbolUniList *, union HarbolValue, size_t);

HARBOL_EXPORT struct HarbolUniListNode *HarbolUniList_GetNode(const struct HarbolUniList *, size_t);
HARBOL_EXPORT struct HarbolUniListNode *HarbolUniList_GetNodeByValue(const struct HarbolUniList *, union HarbolValue);
HARBOL_EXPORT union HarbolValue HarbolUniList_GetValue(const struct HarbolUniList *, size_t);
HARBOL_EXPORT void HarbolUniList_SetValue(struct HarbolUniList *, size_t, union HarbolValue);
HARBOL_EXPORT bool HarbolUniList_DelNodeByIndex(struct HarbolUniList *, size_t, fnDestructor *);
HARBOL_EXPORT bool HarbolUniList_DelNodeByRef(struct HarbolUniList *, struct HarbolUniListNode **, fnDestructor *);

HARBOL_EXPORT struct HarbolUniListNode *HarbolUniList_GetHead(const struct HarbolUniList *);
HARBOL_EXPORT struct HarbolUniListNode *HarbolUniList_GetTail(const struct HarbolUniList *);

HARBOL_EXPORT void HarbolUniList_FromHarbolBiList(struct HarbolUniList *, const struct HarbolBiList *);
HARBOL_EXPORT void HarbolUniList_FromHarbolHashmap(struct HarbolUniList *, const struct HarbolHashmap *);
HARBOL_EXPORT void HarbolUniList_FromHarbolVector(struct HarbolUniList *, const struct HarbolVector *);
HARBOL_EXPORT void HarbolUniList_FromHarbolTuple(struct HarbolUniList *, const struct HarbolTuple *);
HARBOL_EXPORT void HarbolUniList_FromHarbolGraph(struct HarbolUniList *, const struct HarbolGraph *);
HARBOL_EXPORT void HarbolUniList_FromHarbolLinkMap(struct HarbolUniList *, const struct HarbolLinkMap *);

HARBOL_EXPORT struct HarbolUniList *HarbolUniList_NewFromHarbolBiList(const struct HarbolBiList *);
HARBOL_EXPORT struct HarbolUniList *HarbolUniList_NewFromHarbolHashmap(const struct HarbolHashmap *);
HARBOL_EXPORT struct HarbolUniList *HarbolUniList_NewFromHarbolVector(const struct HarbolVector *);
HARBOL_EXPORT struct HarbolUniList *HarbolUniList_NewFromHarbolTuple(const struct HarbolTuple *);
HARBOL_EXPORT struct HarbolUniList *HarbolUniList_NewFromHarbolGraph(const struct HarbolGraph *);
HARBOL_EXPORT struct HarbolUniList *HarbolUniList_NewFromHarbolLinkMap(const struct HarbolLinkMap *);
/***************/


/************* Doubly Linked List (bilist.c) *************/
typedef struct HarbolBiListNode {
	union HarbolValue Data;
	struct HarbolBiListNode *Next, *Prev;
} HarbolBiListNode;

HARBOL_EXPORT struct HarbolBiListNode *HarbolBiListNode_New(void);
HARBOL_EXPORT struct HarbolBiListNode *HarbolBiListNode_NewVal(union HarbolValue);
HARBOL_EXPORT void HarbolBiListNode_Del(struct HarbolBiListNode *, fnDestructor *);
HARBOL_EXPORT void HarbolBiListNode_Free(struct HarbolBiListNode **, fnDestructor *);
HARBOL_EXPORT struct HarbolBiListNode *HarbolBiListNode_GetNextNode(const struct HarbolBiListNode *);
HARBOL_EXPORT struct HarbolBiListNode *HarbolBiListNode_GetPrevNode(const struct HarbolBiListNode *);
HARBOL_EXPORT union HarbolValue HarbolBiListNode_GetValue(const struct HarbolBiListNode *);


typedef struct HarbolBiList {
	struct HarbolBiListNode *Head, *Tail;
	size_t Len;
} HarbolBiList;

HARBOL_EXPORT struct HarbolBiList *HarbolBiList_New(void);
HARBOL_EXPORT void HarbolBiList_Del(struct HarbolBiList *, fnDestructor *);
HARBOL_EXPORT void HarbolBiList_Free(struct HarbolBiList **,fnDestructor *);
HARBOL_EXPORT void HarbolBiList_Init(struct HarbolBiList *);

HARBOL_EXPORT size_t HarbolBiList_Len(const struct HarbolBiList *);
HARBOL_EXPORT bool HarbolBiList_InsertNodeAtHead(struct HarbolBiList *, struct HarbolBiListNode *);
HARBOL_EXPORT bool HarbolBiList_InsertNodeAtTail(struct HarbolBiList *, struct HarbolBiListNode *);
HARBOL_EXPORT bool HarbolBiList_InsertNodeAtIndex(struct HarbolBiList *, struct HarbolBiListNode *, size_t);
HARBOL_EXPORT bool HarbolBiList_InsertValueAtHead(struct HarbolBiList *, union HarbolValue);
HARBOL_EXPORT bool HarbolBiList_InsertValueAtTail(struct HarbolBiList *, union HarbolValue);
HARBOL_EXPORT bool HarbolBiList_InsertValueAtIndex(struct HarbolBiList *, union HarbolValue, size_t);

HARBOL_EXPORT struct HarbolBiListNode *HarbolBiList_GetNode(const struct HarbolBiList *, size_t);
HARBOL_EXPORT struct HarbolBiListNode *HarbolBiList_GetNodeByValue(const struct HarbolBiList *, union HarbolValue);
HARBOL_EXPORT union HarbolValue HarbolBiList_GetValue(const struct HarbolBiList *, size_t);
HARBOL_EXPORT void HarbolBiList_SetValue(struct HarbolBiList *, size_t, union HarbolValue);
HARBOL_EXPORT bool HarbolBiList_DelNodeByIndex(struct HarbolBiList *, size_t, fnDestructor *);
HARBOL_EXPORT bool HarbolBiList_DelNodeByRef(struct HarbolBiList *, struct HarbolBiListNode **, fnDestructor *);

HARBOL_EXPORT struct HarbolBiListNode *HarbolBiList_GetHead(const struct HarbolBiList *);
HARBOL_EXPORT struct HarbolBiListNode *HarbolBiList_GetTail(const struct HarbolBiList *);

HARBOL_EXPORT void HarbolBiList_FromHarbolUniList(struct HarbolBiList *, const struct HarbolUniList *);
HARBOL_EXPORT void HarbolBiList_FromHarbolHashmap(struct HarbolBiList *, const struct HarbolHashmap *);
HARBOL_EXPORT void HarbolBiList_FromHarbolVector(struct HarbolBiList *, const struct HarbolVector *);
HARBOL_EXPORT void HarbolBiList_FromHarbolTuple(struct HarbolBiList *, const struct HarbolTuple *);
HARBOL_EXPORT void HarbolBiList_FromHarbolGraph(struct HarbolBiList *, const struct HarbolGraph *);
HARBOL_EXPORT void HarbolBiList_FromHarbolLinkMap(struct HarbolBiList *, const struct HarbolLinkMap *);

HARBOL_EXPORT struct HarbolBiList *HarbolBiList_NewFromHarbolUniList(const struct HarbolUniList *);
HARBOL_EXPORT struct HarbolBiList *HarbolBiList_NewFromHarbolHashmap(const struct HarbolHashmap *);
HARBOL_EXPORT struct HarbolBiList *HarbolBiList_NewFromHarbolVector(const struct HarbolVector *);
HARBOL_EXPORT struct HarbolBiList *HarbolBiList_NewFromHarbolTuple(const struct HarbolTuple *);
HARBOL_EXPORT struct HarbolBiList *HarbolBiList_NewFromHarbolGraph(const struct HarbolGraph *);
HARBOL_EXPORT struct HarbolBiList *HarbolBiList_NewFromHarbolLinkMap(const struct HarbolLinkMap *);
/***************/


/************* Byte Buffer (bytebuffer.c) *************/
typedef struct HarbolByteBuffer {
	uint8_t *Buffer;
	size_t Len, Count;
} HarbolByteBuffer;

HARBOL_EXPORT struct HarbolByteBuffer *HarbolByteBuffer_New(void);
HARBOL_EXPORT void HarbolByteBuffer_Init(struct HarbolByteBuffer *);
HARBOL_EXPORT void HarbolByteBuffer_Del(struct HarbolByteBuffer *);
HARBOL_EXPORT void HarbolByteBuffer_Free(struct HarbolByteBuffer **);
HARBOL_EXPORT size_t HarbolByteBuffer_Len(const struct HarbolByteBuffer *);
HARBOL_EXPORT size_t HarbolByteBuffer_Count(const struct HarbolByteBuffer *);
HARBOL_EXPORT uint8_t *HarbolByteBuffer_GetBuffer(const struct HarbolByteBuffer *);
HARBOL_EXPORT void HarbolByteBuffer_InsertByte(struct HarbolByteBuffer *, uint8_t);
HARBOL_EXPORT void HarbolByteBuffer_InsertInt(struct HarbolByteBuffer *, uint64_t, size_t);
HARBOL_EXPORT void HarbolByteBuffer_InsertFloat(struct HarbolByteBuffer *, float);
HARBOL_EXPORT void HarbolByteBuffer_InsertDouble(struct HarbolByteBuffer *, double);
HARBOL_EXPORT void HarbolByteBuffer_InsertString(struct HarbolByteBuffer *, const char *, size_t);
HARBOL_EXPORT void HarbolByteBuffer_InsertObject(struct HarbolByteBuffer *, const void *, size_t);
HARBOL_EXPORT void HarbolByteBuffer_InsertZeroes(struct HarbolByteBuffer *, size_t);
HARBOL_EXPORT void HarbolByteBuffer_Delete(struct HarbolByteBuffer *, size_t);
HARBOL_EXPORT void HarbolByteBuffer_Resize(struct HarbolByteBuffer *);
HARBOL_EXPORT void HarbolByteBuffer_DumpToFile(const struct HarbolByteBuffer *, FILE *);
HARBOL_EXPORT size_t HarbolByteBuffer_ReadFromFile(struct HarbolByteBuffer *, FILE *);
HARBOL_EXPORT void HarbolByteBuffer_Append(struct HarbolByteBuffer *, struct HarbolByteBuffer *);
/***************/


/************* HarbolTuple (tuple.c) *************/
typedef struct HarbolTuple {
	union HarbolValue *Items;
	size_t Len;
} HarbolTuple;

HARBOL_EXPORT struct HarbolTuple *HarbolTuple_New(size_t, union HarbolValue []);
HARBOL_EXPORT void HarbolTuple_Free(struct HarbolTuple **);

HARBOL_EXPORT void HarbolTuple_Init(struct HarbolTuple *, size_t, union HarbolValue []);
HARBOL_EXPORT void HarbolTuple_Del(struct HarbolTuple *);
HARBOL_EXPORT size_t HarbolTuple_Len(const struct HarbolTuple *);
HARBOL_EXPORT union HarbolValue *HarbolTuple_GetItems(const struct HarbolTuple *);
HARBOL_EXPORT union HarbolValue HarbolTuple_GetItem(const struct HarbolTuple *, size_t);

HARBOL_EXPORT void HarbolTuple_FromHarbolUniList(struct HarbolTuple *, const struct HarbolUniList *);
HARBOL_EXPORT void HarbolTuple_FromHarbolHashmap(struct HarbolTuple *, const struct HarbolHashmap *);
HARBOL_EXPORT void HarbolTuple_FromHarbolVector(struct HarbolTuple *, const struct HarbolVector *);
HARBOL_EXPORT void HarbolTuple_FromHarbolBiList(struct HarbolTuple *, const struct HarbolBiList *);
HARBOL_EXPORT void HarbolTuple_FromHarbolGraph(struct HarbolTuple *, const struct HarbolGraph *);
HARBOL_EXPORT void HarbolTuple_FromHarbolLinkMap(struct HarbolTuple *, const struct HarbolLinkMap *);

HARBOL_EXPORT struct HarbolTuple *HarbolTuple_NewFromHarbolUniList(const struct HarbolUniList *);
HARBOL_EXPORT struct HarbolTuple *HarbolTuple_NewFromHarbolHashmap(const struct HarbolHashmap *);
HARBOL_EXPORT struct HarbolTuple *HarbolTuple_NewFromHarbolVector(const struct HarbolVector *);
HARBOL_EXPORT struct HarbolTuple *HarbolTuple_NewFromHarbolBiList(const struct HarbolBiList *);
HARBOL_EXPORT struct HarbolTuple *HarbolTuple_NewFromHarbolGraph(const struct HarbolGraph *);
HARBOL_EXPORT struct HarbolTuple *HarbolTuple_NewFromHarbolLinkMap(const struct HarbolLinkMap *);
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

typedef struct HarbolAllocNode {
	size_t Size;
	struct HarbolAllocNode *NextFree;
} HarbolAllocNode;

typedef struct HarbolMemoryPool {
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
	struct HarbolAllocNode *FreeList;
} HarbolMemoryPool;

#ifdef POOL_NO_MALLOC
HARBOL_EXPORT void HarbolMemoryPool_Init(struct HarbolMemoryPool *);
#else
HARBOL_EXPORT void HarbolMemoryPool_Init(struct HarbolMemoryPool *, size_t);
#endif

HARBOL_EXPORT void HarbolMemoryPool_Del(struct HarbolMemoryPool *);
HARBOL_EXPORT void *HarbolMemoryPool_Alloc(struct HarbolMemoryPool *, size_t);
HARBOL_EXPORT void *HarbolMemoryPool_Realloc(struct HarbolMemoryPool *, void *, size_t);
HARBOL_EXPORT void HarbolMemoryPool_Dealloc(struct HarbolMemoryPool *, void *);
HARBOL_EXPORT void HarbolMemoryPool_Destroy(struct HarbolMemoryPool *, void *);
HARBOL_EXPORT size_t HarbolMemoryPool_Remaining(const struct HarbolMemoryPool *);
HARBOL_EXPORT size_t HarbolMemoryPool_Size(const struct HarbolMemoryPool *);
HARBOL_EXPORT struct HarbolAllocNode *HarbolMemoryPool_GetFreeList(const struct HarbolMemoryPool *);
HARBOL_EXPORT bool HarbolMemoryPool_Defrag(struct HarbolMemoryPool *);
/***************/


/************* HarbolGraph (Adjacency List) (graph.c) *************/
struct HarbolGraphVertex;
typedef struct HarbolGraphEdge {
	union HarbolValue Weight;
	struct HarbolGraphVertex *VertexSocket;
} HarbolGraphEdge;

HARBOL_EXPORT struct HarbolGraphEdge *HarbolGraphEdge_New(void);
HARBOL_EXPORT struct HarbolGraphEdge *HarbolGraphEdge_NewVP(union HarbolValue, struct HarbolGraphVertex *);
HARBOL_EXPORT void HarbolGraphEdge_Del(struct HarbolGraphEdge *, fnDestructor *);
HARBOL_EXPORT void HarbolGraphEdge_Free(struct HarbolGraphEdge **, fnDestructor *);

HARBOL_EXPORT union HarbolValue HarbolGraphEdge_GetWeight(const struct HarbolGraphEdge *);
HARBOL_EXPORT void HarbolGraphEdge_SetWeight(struct HarbolGraphEdge *, union HarbolValue);
HARBOL_EXPORT struct HarbolGraphVertex *HarbolGraphEdge_GetVertex(const struct HarbolGraphEdge *);
HARBOL_EXPORT void HarbolGraphEdge_SetVertex(struct HarbolGraphEdge *, struct HarbolGraphVertex *);


typedef struct HarbolGraphVertex {
	union {
		struct {
			union HarbolValue *Table;
			size_t Len, Count;
		};
		struct HarbolVector Edges;
	};
	union HarbolValue Data;
} HarbolGraphVertex;

HARBOL_EXPORT struct HarbolGraphVertex *HarbolGraphVertex_New(union HarbolValue);
HARBOL_EXPORT void HarbolGraphVertex_Init(struct HarbolGraphVertex *, union HarbolValue);
HARBOL_EXPORT void HarbolGraphVertex_Del(struct HarbolGraphVertex *, fnDestructor *, fnDestructor *);
HARBOL_EXPORT void HarbolGraphVertex_Free(struct HarbolGraphVertex **, fnDestructor *, fnDestructor *);
HARBOL_EXPORT bool HarbolGraphVertex_AddEdge(struct HarbolGraphVertex *, struct HarbolGraphEdge *);
HARBOL_EXPORT struct HarbolVector *HarbolGraphVertex_GetEdges(struct HarbolGraphVertex *);
HARBOL_EXPORT union HarbolValue HarbolGraphVertex_GetData(const struct HarbolGraphVertex *);
HARBOL_EXPORT void HarbolGraphVertex_SetData(struct HarbolGraphVertex *, union HarbolValue);


typedef struct HarbolGraph {
	union {
		struct {
			union HarbolValue *Table;
			size_t Len, Count;
		};
		struct HarbolVector Vertices;
	};
} HarbolGraph;

HARBOL_EXPORT struct HarbolGraph *HarbolGraph_New(void);
HARBOL_EXPORT void HarbolGraph_Init(struct HarbolGraph *);
HARBOL_EXPORT void HarbolGraph_Del(struct HarbolGraph *, fnDestructor *, fnDestructor *);
HARBOL_EXPORT void HarbolGraph_Free(struct HarbolGraph **, fnDestructor *, fnDestructor *);

HARBOL_EXPORT bool HarbolGraph_InsertVertexByValue(struct HarbolGraph *, union HarbolValue);
HARBOL_EXPORT bool HarbolGraph_RemoveVertexByValue(struct HarbolGraph *, union HarbolValue, fnDestructor *, fnDestructor *);
HARBOL_EXPORT bool HarbolGraph_RemoveVertexByIndex(struct HarbolGraph *, size_t, fnDestructor *, fnDestructor *);

HARBOL_EXPORT bool HarbolGraph_InsertEdgeBtwnVerts(struct HarbolGraph *, size_t, size_t, union HarbolValue);
HARBOL_EXPORT bool HarbolGraph_RemoveEdgeBtwnVerts(struct HarbolGraph *, size_t, size_t, fnDestructor *);

HARBOL_EXPORT struct HarbolGraphVertex *HarbolGraph_GetVertexByIndex(struct HarbolGraph *, size_t);
HARBOL_EXPORT union HarbolValue HarbolGraph_GetVertexDataByIndex(struct HarbolGraph *, size_t);
HARBOL_EXPORT void HarbolGraph_SetVertexDataByIndex(struct HarbolGraph *, size_t, union HarbolValue);
HARBOL_EXPORT struct HarbolGraphEdge *HarbolGraph_GetEdgeBtwnVertices(struct HarbolGraph *, size_t, size_t);

HARBOL_EXPORT bool HarbolGraph_IsVertexAdjacent(struct HarbolGraph *, size_t, size_t);
HARBOL_EXPORT struct HarbolVector *HarbolGraph_GetVertexNeighbors(struct HarbolGraph *, size_t);

HARBOL_EXPORT struct HarbolVector *HarbolGraph_GetVertices(struct HarbolGraph *);
HARBOL_EXPORT size_t HarbolGraph_GetVerticeCount(const struct HarbolGraph *);
HARBOL_EXPORT size_t HarbolGraph_GetEdgeCount(const struct HarbolGraph *);
HARBOL_EXPORT void HarbolGraph_Resize(struct HarbolGraph *);
HARBOL_EXPORT void HarbolGraph_Truncate(struct HarbolGraph *);

HARBOL_EXPORT void HarbolGraph_FromHarbolVector(struct HarbolGraph *, const struct HarbolVector *);
HARBOL_EXPORT void HarbolGraph_FromHarbolHashmap(struct HarbolGraph *, const struct HarbolHashmap *);
HARBOL_EXPORT void HarbolGraph_FromHarbolUniList(struct HarbolGraph *, const struct HarbolUniList *);
HARBOL_EXPORT void HarbolGraph_FromHarbolBiList(struct HarbolGraph *, const struct HarbolBiList *);
HARBOL_EXPORT void HarbolGraph_FromHarbolTuple(struct HarbolGraph *, const struct HarbolTuple *);
HARBOL_EXPORT void HarbolGraph_FromHarbolLinkMap(struct HarbolGraph *, const struct HarbolLinkMap *);

HARBOL_EXPORT struct HarbolGraph *HarbolGraph_NewFromHarbolVector(const struct HarbolVector *);
HARBOL_EXPORT struct HarbolGraph *HarbolGraph_NewFromHarbolHashmap(const struct HarbolHashmap *);
HARBOL_EXPORT struct HarbolGraph *HarbolGraph_NewFromHarbolUniList(const struct HarbolUniList *);
HARBOL_EXPORT struct HarbolGraph *HarbolGraph_NewFromHarbolBiList(const struct HarbolBiList *);
HARBOL_EXPORT struct HarbolGraph *HarbolGraph_NewFromHarbolTuple(const struct HarbolTuple *);
HARBOL_EXPORT struct HarbolGraph *HarbolGraph_NewFromHarbolLinkMap(const struct HarbolLinkMap *);
/***************/


/************* General Tree (tree.c) *************/
typedef struct HarbolTree {
	struct HarbolVector Children;
	union HarbolValue Data;
} HarbolTree;

HARBOL_EXPORT struct HarbolTree *HarbolTree_New(union HarbolValue);
HARBOL_EXPORT void HarbolTree_Init(struct HarbolTree *);
HARBOL_EXPORT void HarbolTree_InitVal(struct HarbolTree *, union HarbolValue);
HARBOL_EXPORT void HarbolTree_Del(struct HarbolTree *, fnDestructor *);
HARBOL_EXPORT void HarbolTree_Free(struct HarbolTree **, fnDestructor *);

HARBOL_EXPORT bool HarbolTree_InsertChildByNode(struct HarbolTree *, struct HarbolTree *);
HARBOL_EXPORT bool HarbolTree_InsertChildByValue(struct HarbolTree *, union HarbolValue);

HARBOL_EXPORT bool HarbolTree_RemoveChildByRef(struct HarbolTree *, struct HarbolTree **, fnDestructor *);
HARBOL_EXPORT bool HarbolTree_RemoveChildByIndex(struct HarbolTree *, size_t, fnDestructor *);
HARBOL_EXPORT bool HarbolTree_RemoveChildByValue(struct HarbolTree *, union HarbolValue, fnDestructor *);

HARBOL_EXPORT struct HarbolTree *HarbolTree_GetChildByIndex(const struct HarbolTree *, size_t);
HARBOL_EXPORT struct HarbolTree *HarbolTree_GetChildByValue(const struct HarbolTree *, union HarbolValue);
HARBOL_EXPORT union HarbolValue HarbolTree_GetData(const struct HarbolTree *);
HARBOL_EXPORT void HarbolTree_SetData(struct HarbolTree *, union HarbolValue);
HARBOL_EXPORT struct HarbolVector HarbolTree_GetChildren(const struct HarbolTree *);
HARBOL_EXPORT size_t HarbolTree_GetChildLen(const struct HarbolTree *);
HARBOL_EXPORT size_t HarbolTree_GetChildCount(const struct HarbolTree *);
/***************/


/************* Ordered HarbolHashmap (linkmap.c) *************/
typedef struct HarbolLinkMap {
	union {
		struct {
			struct HarbolVector *Table;
			size_t Len, Count;
		};
		struct HarbolHashmap Map;
	};
	struct HarbolVector Order;
} HarbolLinkMap;

HARBOL_EXPORT struct HarbolLinkMap *HarbolLinkMap_New(void);
HARBOL_EXPORT void HarbolLinkMap_Init(struct HarbolLinkMap *);
HARBOL_EXPORT void HarbolLinkMap_Del(struct HarbolLinkMap *, fnDestructor *);
HARBOL_EXPORT void HarbolLinkMap_Free(struct HarbolLinkMap **, fnDestructor *);
HARBOL_EXPORT size_t HarbolLinkMap_Count(const struct HarbolLinkMap *);
HARBOL_EXPORT size_t HarbolLinkMap_Len(const struct HarbolLinkMap *);
HARBOL_EXPORT bool HarbolLinkMap_Rehash(struct HarbolLinkMap *);

HARBOL_EXPORT bool HarbolLinkMap_Insert(struct HarbolLinkMap *, const char *, union HarbolValue);
HARBOL_EXPORT bool HarbolLinkMap_InsertNode(struct HarbolLinkMap *, struct HarbolKeyValPair *);

HARBOL_EXPORT struct HarbolKeyValPair *HarbolLinkMap_GetNodeByIndex(const struct HarbolLinkMap *, size_t);
HARBOL_EXPORT union HarbolValue HarbolLinkMap_Get(const struct HarbolLinkMap *, const char *);
HARBOL_EXPORT void HarbolLinkMap_Set(struct HarbolLinkMap *, const char *, union HarbolValue);
HARBOL_EXPORT union HarbolValue HarbolLinkMap_GetByIndex(const struct HarbolLinkMap *, size_t);
HARBOL_EXPORT void HarbolLinkMap_SetByIndex(struct HarbolLinkMap *, size_t, union HarbolValue);

HARBOL_EXPORT void HarbolLinkMap_Delete(struct HarbolLinkMap *, const char *, fnDestructor *);
HARBOL_EXPORT void HarbolLinkMap_DeleteByIndex(struct HarbolLinkMap *, size_t, fnDestructor *);
HARBOL_EXPORT bool HarbolLinkMap_HasKey(const struct HarbolLinkMap *, const char *);
HARBOL_EXPORT struct HarbolKeyValPair *HarbolLinkMap_GetKeyValByKey(const struct HarbolLinkMap *, const char *);
HARBOL_EXPORT struct HarbolVector *HarbolLinkMap_GetBuckets(const struct HarbolLinkMap *);

HARBOL_EXPORT union HarbolValue *HarbolLinkMap_GetIter(const struct HarbolLinkMap *);
HARBOL_EXPORT union HarbolValue *HarbolLinkMap_GetIterEndLen(const struct HarbolLinkMap *);
HARBOL_EXPORT union HarbolValue *HarbolLinkMap_GetIterEndCount(const struct HarbolLinkMap *);

HARBOL_EXPORT size_t HarbolLinkMap_GetIndexByName(const struct HarbolLinkMap *, const char *);
HARBOL_EXPORT size_t HarbolLinkMap_GetIndexByNode(const struct HarbolLinkMap *, struct HarbolKeyValPair *);
HARBOL_EXPORT size_t HarbolLinkMap_GetIndexByValue(const struct HarbolLinkMap *, union HarbolValue);

HARBOL_EXPORT void HarbolLinkMap_FromHarbolHashmap(struct HarbolLinkMap *, const struct HarbolHashmap *);
HARBOL_EXPORT void HarbolLinkMap_FromHarbolUniList(struct HarbolLinkMap *, const struct HarbolUniList *);
HARBOL_EXPORT void HarbolLinkMap_FromHarbolBiList(struct HarbolLinkMap *, const struct HarbolBiList *);
HARBOL_EXPORT void HarbolLinkMap_FromHarbolVector(struct HarbolLinkMap *, const struct HarbolVector *);
HARBOL_EXPORT void HarbolLinkMap_FromHarbolTuple(struct HarbolLinkMap *, const struct HarbolTuple *);
HARBOL_EXPORT void HarbolLinkMap_FromHarbolGraph(struct HarbolLinkMap *, const struct HarbolGraph *);

HARBOL_EXPORT struct HarbolLinkMap *HarbolLinkMap_NewFromHarbolHashmap(const struct HarbolHashmap *);
HARBOL_EXPORT struct HarbolLinkMap *HarbolLinkMap_NewFromHarbolUniList(const struct HarbolUniList *);
HARBOL_EXPORT struct HarbolLinkMap *HarbolLinkMap_NewFromHarbolBiList(const struct HarbolBiList *);
HARBOL_EXPORT struct HarbolLinkMap *HarbolLinkMap_NewFromHarbolVector(const struct HarbolVector *);
HARBOL_EXPORT struct HarbolLinkMap *HarbolLinkMap_NewFromHarbolTuple(const struct HarbolTuple *);
HARBOL_EXPORT struct HarbolLinkMap *HarbolLinkMap_NewFromHarbolGraph(const struct HarbolGraph *);
/***************/


/************* Tagged Union Type (variant.c) *************/
// discriminated union type
typedef struct HarbolVariant {
	union HarbolValue Val;
	int32_t TypeTag;
} HarbolVariant;

HARBOL_EXPORT struct HarbolVariant *HarbolVariant_New(union HarbolValue, int32_t);
HARBOL_EXPORT void HarbolVariant_Free(struct HarbolVariant **, fnDestructor *);
HARBOL_EXPORT void HarbolVariant_Init(struct HarbolVariant *, union HarbolValue, int32_t);
HARBOL_EXPORT void HarbolVariant_Del(struct HarbolVariant *, fnDestructor *);

HARBOL_EXPORT union HarbolValue HarbolVariant_GetVal(const struct HarbolVariant *);
HARBOL_EXPORT void HarbolVariant_SetVal(struct HarbolVariant *, union HarbolValue);

HARBOL_EXPORT int32_t HarbolVariant_GetType(const struct HarbolVariant *);
HARBOL_EXPORT void HarbolVariant_SetType(struct HarbolVariant *, int32_t);
/***************/

#ifdef __cplusplus
}
#endif
