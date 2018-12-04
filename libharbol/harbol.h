#pragma once

#ifdef __cplusplus
extern "C" {
	#ifndef restrict
		#define restrict __restrict
	#endif
#endif

// Windows
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
	#ifndef OS_WINDOWS
		#define OS_WINDOWS 1
	#endif
// Linux/UNIX & FreeBSD
#elif defined(unix) || defined(__unix) || defined(__unix__) || defined(__linux__) || defined(linux) || defined(__linux) || defined(__freeBSD__)
	#ifndef OS_LINUX_UNIX
		#define OS_LINUX_UNIX 1
	#endif
// Android
#elif defined(__ANDROID__)
	#ifndef OS_ANDROID
		#define OS_ANDROID 1
	#endif
// Solaris/SunOS
#elif defined(sun) || defined(__sun)
	#if defined(__SVR4) || defined(__svr4__)
		#ifndef OS_SOLARIS
			#define OS_SOLARIS 1
		#endif
	#else
		#ifndef OS_SUNOS
			#define OS_SUNOS 1
		#endif
	#endif
// Macintosh/MacOS
#elif defined(macintosh) || defined(Macintosh) || defined(__APPLE__)
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
#include <stdalign.h>
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
struct HarbolGraphEdge;
struct HarbolGraphVertex;
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
	struct HarbolGraphEdge *GraphEdgePtr;
	struct HarbolGraphVertex *GraphVertPtr;
	struct HarbolGraph *GraphPtr;
	struct HarbolTree *TreePtr;
	struct HarbolLinkMap *LinkMapPtr;
} HarbolValue;


/************* C++ Style Automated HarbolString (stringobj.c) *************/
typedef struct HarbolString {
	char *CStr;
	size_t Len;
} HarbolString;

HARBOL_EXPORT struct HarbolString *harbol_string_new(void);
HARBOL_EXPORT struct HarbolString *harbol_string_new_cstr(const char []);
HARBOL_EXPORT void harbol_string_del(struct HarbolString *);
HARBOL_EXPORT bool harbol_string_free(struct HarbolString **);
HARBOL_EXPORT void harbol_string_init(struct HarbolString *);
HARBOL_EXPORT void harbol_string_init_cstr(struct HarbolString *, const char []);
HARBOL_EXPORT void harbol_string_add_char(struct HarbolString *, char);
HARBOL_EXPORT void harbol_string_add_str(struct HarbolString *, const struct HarbolString *);
HARBOL_EXPORT void harbol_string_add_cstr(struct HarbolString *, const char []);
HARBOL_EXPORT char *harbol_string_get_cstr(const struct HarbolString *);
HARBOL_EXPORT size_t harbol_string_get_len(const struct HarbolString *);
HARBOL_EXPORT void harbol_string_copy_str(struct HarbolString *, const struct HarbolString *);
HARBOL_EXPORT void harbol_string_copy_cstr(struct HarbolString *, const char []);
HARBOL_EXPORT int32_t harbol_string_format(struct HarbolString *, const char [], ...);
HARBOL_EXPORT int32_t harbol_string_cmpcstr(const struct HarbolString *, const char []);
HARBOL_EXPORT int32_t harbol_string_cmpstr(const struct HarbolString *, const struct HarbolString *);
HARBOL_EXPORT int32_t harbol_string_ncmpcstr(const struct HarbolString *, const char [], size_t);
HARBOL_EXPORT int32_t harbol_string_ncmpstr(const struct HarbolString *, const struct HarbolString *, size_t);
HARBOL_EXPORT bool harbol_string_is_empty(const struct HarbolString *);
HARBOL_EXPORT bool harbol_string_reserve(struct HarbolString *, size_t);
HARBOL_EXPORT char *harbol_string_fgets(struct HarbolString *, FILE *);
HARBOL_EXPORT void harbol_string_clear(struct HarbolString *);
/***************/


/************* HarbolVector / Dynamic Array (vector.c) *************/
typedef struct HarbolVector {
	union HarbolValue *Table;
	size_t Len, Count;
} HarbolVector;

HARBOL_EXPORT struct HarbolVector *harbol_vector_new(void);
HARBOL_EXPORT void harbol_vector_init(struct HarbolVector *);
HARBOL_EXPORT void harbol_vector_del(struct HarbolVector *, fnDestructor *);
HARBOL_EXPORT void harbol_vector_free(struct HarbolVector **, fnDestructor *);

HARBOL_EXPORT size_t harbol_vector_get_len(const struct HarbolVector *);
HARBOL_EXPORT size_t harbol_vector_get_count(const struct HarbolVector *);
HARBOL_EXPORT union HarbolValue *harbol_vector_get_iter(const struct HarbolVector *);
HARBOL_EXPORT union HarbolValue *harbol_vector_get_iter_end_len(const struct HarbolVector *);
HARBOL_EXPORT union HarbolValue *harbol_vector_get_iter_end_count(const struct HarbolVector *);
HARBOL_EXPORT void harbol_vector_resize(struct HarbolVector *);
HARBOL_EXPORT void harbol_vector_truncate(struct HarbolVector *);

HARBOL_EXPORT bool harbol_vector_insert(struct HarbolVector *, union HarbolValue);
HARBOL_EXPORT union HarbolValue harbol_vector_pop(struct HarbolVector *);
HARBOL_EXPORT union HarbolValue harbol_vector_get(const struct HarbolVector *, size_t);
HARBOL_EXPORT void harbol_vector_set(struct HarbolVector *, size_t, union HarbolValue);

HARBOL_EXPORT void harbol_vector_delete(struct HarbolVector *, size_t, fnDestructor *);
HARBOL_EXPORT void harbol_vector_add(struct HarbolVector *, const struct HarbolVector *);
HARBOL_EXPORT void harbol_vector_copy(struct HarbolVector *, const struct HarbolVector *);

HARBOL_EXPORT void harbol_vector_from_unilist(struct HarbolVector *, const struct HarbolUniList *);
HARBOL_EXPORT void harbol_vector_from_bilist(struct HarbolVector *, const struct HarbolBiList *);
HARBOL_EXPORT void harbol_vector_from_hashmap(struct HarbolVector *, const struct HarbolHashmap *);
HARBOL_EXPORT void harbol_vector_from_graph(struct HarbolVector *, const struct HarbolGraph *);
HARBOL_EXPORT void harbol_vector_from_linkmap(struct HarbolVector *, const struct HarbolLinkMap *);

HARBOL_EXPORT struct HarbolVector *harbol_vector_new_from_unilist(const struct HarbolUniList *);
HARBOL_EXPORT struct HarbolVector *harbol_vector_new_from_bilist(const struct HarbolBiList *);
HARBOL_EXPORT struct HarbolVector *harbol_vector_new_from_hashmap(const struct HarbolHashmap *);
HARBOL_EXPORT struct HarbolVector *harbol_vector_new_from_graph(const struct HarbolGraph *);
HARBOL_EXPORT struct HarbolVector *harbol_vector_new_from_linkmap(const struct HarbolLinkMap *);
/***************/


/************* HarbolHashmap (hashmap.c) *************/
typedef struct HarbolKeyValPair {
	struct HarbolString KeyName;
	union HarbolValue Data;
} HarbolKeyValPair;

HARBOL_EXPORT struct HarbolKeyValPair *harbol_kvpair_new(void);
HARBOL_EXPORT struct HarbolKeyValPair *harbol_kvpair_new_strval(const char [], union HarbolValue);

HARBOL_EXPORT void harbol_kvpair_del(struct HarbolKeyValPair *, fnDestructor *);
HARBOL_EXPORT void harbol_kvpair_free(struct HarbolKeyValPair **, fnDestructor *);


typedef struct HarbolHashmap {
	struct HarbolVector *Table; // a vector of vectors!
	size_t Len, Count;
} HarbolHashmap;

HARBOL_EXPORT size_t GenHash(const char []);
HARBOL_EXPORT struct HarbolHashmap *harbol_hashmap_new(void);
HARBOL_EXPORT void harbol_hashmap_init(struct HarbolHashmap *);
HARBOL_EXPORT void harbol_hashmap_del(struct HarbolHashmap *, fnDestructor *);
HARBOL_EXPORT void harbol_hashmap_free(struct HarbolHashmap **, fnDestructor *);
HARBOL_EXPORT size_t harbol_hashmap_get_count(const struct HarbolHashmap *);
HARBOL_EXPORT size_t harbol_hashmap_get_len(const struct HarbolHashmap *);
HARBOL_EXPORT bool harbol_hashmap_rehash(struct HarbolHashmap *);

HARBOL_EXPORT bool harbol_hashmap_insert_node(struct HarbolHashmap *, struct HarbolKeyValPair *);
HARBOL_EXPORT bool harbol_hashmap_insert(struct HarbolHashmap *, const char [], union HarbolValue);

HARBOL_EXPORT union HarbolValue harbol_hashmap_get(const struct HarbolHashmap *, const char []);
HARBOL_EXPORT void harbol_hashmap_set(struct HarbolHashmap *, const char [], union HarbolValue);

HARBOL_EXPORT void harbol_hashmap_delete(struct HarbolHashmap *, const char [], fnDestructor *);
HARBOL_EXPORT bool harbol_hashmap_has_key(const struct HarbolHashmap *, const char []);
HARBOL_EXPORT struct HarbolKeyValPair *harbol_hashmap_get_node(const struct HarbolHashmap *, const char []);
struct HarbolVector *harbol_hashmap_get_buckets(const struct HarbolHashmap *);

HARBOL_EXPORT void harbol_hashmap_from_unilist(struct HarbolHashmap *, const struct HarbolUniList *);
HARBOL_EXPORT void harbol_hashmap_from_bilist(struct HarbolHashmap *, const struct HarbolBiList *);
HARBOL_EXPORT void harbol_hashmap_from_vector(struct HarbolHashmap *, const struct HarbolVector *);
HARBOL_EXPORT void harbol_hashmap_from_graph(struct HarbolHashmap *, const struct HarbolGraph *);
HARBOL_EXPORT void harbol_hashmap_from_linkmap(struct HarbolHashmap *, const struct HarbolLinkMap *);

HARBOL_EXPORT struct HarbolHashmap *harbol_hashmap_new_from_unilist(const struct HarbolUniList *);
HARBOL_EXPORT struct HarbolHashmap *harbol_hashmap_new_from_bilist(const struct HarbolBiList *);
HARBOL_EXPORT struct HarbolHashmap *harbol_hashmap_new_from_vector(const struct HarbolVector *);
HARBOL_EXPORT struct HarbolHashmap *harbol_hashmap_new_from_graph(const struct HarbolGraph *);
HARBOL_EXPORT struct HarbolHashmap *harbol_hashmap_new_from_linkmap(const struct HarbolLinkMap *);
/***************/


/************* Singly Linked List (unilist.c) *************/
typedef struct HarbolUniListNode {
	union HarbolValue Data;
	struct HarbolUniListNode *Next;
} HarbolUniListNode;

HARBOL_EXPORT struct HarbolUniListNode *harbol_unilistnode_new(void);
HARBOL_EXPORT struct HarbolUniListNode *harbol_unilistnode_new_val(union HarbolValue);
HARBOL_EXPORT void harbol_unilistnode_del(struct HarbolUniListNode *, fnDestructor *);
HARBOL_EXPORT void harbol_unilistnode_free(struct HarbolUniListNode **, fnDestructor *);
HARBOL_EXPORT struct HarbolUniListNode *harbol_unilistnode_get_next_node(const struct HarbolUniListNode *);
HARBOL_EXPORT union HarbolValue harbol_unilistnode_get_val(const struct HarbolUniListNode *);


typedef struct HarbolUniList {
	struct HarbolUniListNode *Head, *Tail;
	size_t Len;
} HarbolUniList;

HARBOL_EXPORT struct HarbolUniList *harbol_unilist_new(void);
HARBOL_EXPORT void harbol_unilist_del(struct HarbolUniList *, fnDestructor *);
HARBOL_EXPORT void harbol_unilist_free(struct HarbolUniList **, fnDestructor *);
HARBOL_EXPORT void harbol_unilist_init(struct HarbolUniList *);

HARBOL_EXPORT size_t harbol_unilistnode_get_len(const struct HarbolUniList *);
HARBOL_EXPORT bool harbol_unilist_insert_node_at_head(struct HarbolUniList *, struct HarbolUniListNode *);
HARBOL_EXPORT bool harbol_unilist_insert_node_at_tail(struct HarbolUniList *, struct HarbolUniListNode *);
HARBOL_EXPORT bool harbol_unilist_insert_node_at_index(struct HarbolUniList *, struct HarbolUniListNode *, size_t);
HARBOL_EXPORT bool harbol_unilist_insert_at_head(struct HarbolUniList *, union HarbolValue);
HARBOL_EXPORT bool harbol_unilist_insert_at_tail(struct HarbolUniList *, union HarbolValue);
HARBOL_EXPORT bool harbol_unilist_insert_at_index(struct HarbolUniList *, union HarbolValue, size_t);

HARBOL_EXPORT struct HarbolUniListNode *harbol_unilist_get_node_by_index(const struct HarbolUniList *, size_t);
HARBOL_EXPORT struct HarbolUniListNode *harbol_unilist_get_node_by_val(const struct HarbolUniList *, union HarbolValue);
HARBOL_EXPORT union HarbolValue harbol_unilist_get_val(const struct HarbolUniList *, size_t);
HARBOL_EXPORT void harbol_unilist_set_val(struct HarbolUniList *, size_t, union HarbolValue);
HARBOL_EXPORT bool harbol_unilist_del_node_by_index(struct HarbolUniList *, size_t, fnDestructor *);
HARBOL_EXPORT bool harbol_unilist_del_node_by_ref(struct HarbolUniList *, struct HarbolUniListNode **, fnDestructor *);

HARBOL_EXPORT struct HarbolUniListNode *harbol_unilist_get_head_node(const struct HarbolUniList *);
HARBOL_EXPORT struct HarbolUniListNode *harbol_unilist_get_tail_node(const struct HarbolUniList *);

HARBOL_EXPORT void harbol_unilist_from_bilist(struct HarbolUniList *, const struct HarbolBiList *);
HARBOL_EXPORT void harbol_unilist_from_hashmap(struct HarbolUniList *, const struct HarbolHashmap *);
HARBOL_EXPORT void harbol_unilist_from_vector(struct HarbolUniList *, const struct HarbolVector *);
HARBOL_EXPORT void harbol_unilist_from_graph(struct HarbolUniList *, const struct HarbolGraph *);
HARBOL_EXPORT void harbol_unilist_from_linkmap(struct HarbolUniList *, const struct HarbolLinkMap *);

HARBOL_EXPORT struct HarbolUniList *harbol_unilist_new_from_bilist(const struct HarbolBiList *);
HARBOL_EXPORT struct HarbolUniList *harbol_unilist_new_from_hashmap(const struct HarbolHashmap *);
HARBOL_EXPORT struct HarbolUniList *harbol_unilist_new_from_vector(const struct HarbolVector *);
HARBOL_EXPORT struct HarbolUniList *harbol_unilist_new_from_graph(const struct HarbolGraph *);
HARBOL_EXPORT struct HarbolUniList *harbol_unilist_new_from_linkmap(const struct HarbolLinkMap *);
/***************/


/************* Doubly Linked List (bilist.c) *************/
typedef struct HarbolBiListNode {
	union HarbolValue Data;
	struct HarbolBiListNode *Next, *Prev;
} HarbolBiListNode;

HARBOL_EXPORT struct HarbolBiListNode *harbol_bilist_node_new(void);
HARBOL_EXPORT struct HarbolBiListNode *harbol_bilist_node_new_val(union HarbolValue);
HARBOL_EXPORT void harbol_bilist_node_del(struct HarbolBiListNode *, fnDestructor *);
HARBOL_EXPORT void harbol_bilist_node_free(struct HarbolBiListNode **, fnDestructor *);
HARBOL_EXPORT struct HarbolBiListNode *harbol_bilist_node_get_next_node(const struct HarbolBiListNode *);
HARBOL_EXPORT struct HarbolBiListNode *harbol_bilist_node_get_prev_node(const struct HarbolBiListNode *);
HARBOL_EXPORT union HarbolValue harbol_bilist_node_get_val(const struct HarbolBiListNode *);


typedef struct HarbolBiList {
	struct HarbolBiListNode *Head, *Tail;
	size_t Len;
} HarbolBiList;

HARBOL_EXPORT struct HarbolBiList *harbol_bilist_new(void);
HARBOL_EXPORT void harbol_bilist_del(struct HarbolBiList *, fnDestructor *);
HARBOL_EXPORT void harbol_bilist_free(struct HarbolBiList **,fnDestructor *);
HARBOL_EXPORT void harbol_bilist_init(struct HarbolBiList *);

HARBOL_EXPORT size_t harbol_bilist_get_len(const struct HarbolBiList *);
HARBOL_EXPORT bool harbol_bilist_insert_node_at_head(struct HarbolBiList *, struct HarbolBiListNode *);
HARBOL_EXPORT bool harbol_bilist_insert_node_at_tail(struct HarbolBiList *, struct HarbolBiListNode *);
HARBOL_EXPORT bool harbol_bilist_insert_node_at_index(struct HarbolBiList *, struct HarbolBiListNode *, size_t);
HARBOL_EXPORT bool harbol_bilist_insert_at_head(struct HarbolBiList *, union HarbolValue);
HARBOL_EXPORT bool harbol_bilist_insert_at_tail(struct HarbolBiList *, union HarbolValue);
HARBOL_EXPORT bool harbol_bilist_insert_at_index(struct HarbolBiList *, union HarbolValue, size_t);

HARBOL_EXPORT struct HarbolBiListNode *harbol_bilist_get_node_by_index(const struct HarbolBiList *, size_t);
HARBOL_EXPORT struct HarbolBiListNode *harbol_bilist_get_node_by_val(const struct HarbolBiList *, union HarbolValue);
HARBOL_EXPORT union HarbolValue harbol_bilist_get_val(const struct HarbolBiList *, size_t);
HARBOL_EXPORT void harbol_bilist_set_val(struct HarbolBiList *, size_t, union HarbolValue);
HARBOL_EXPORT bool harbol_bilist_del_node_by_index(struct HarbolBiList *, size_t, fnDestructor *);
HARBOL_EXPORT bool harbol_bilist_del_node_by_ref(struct HarbolBiList *, struct HarbolBiListNode **, fnDestructor *);

HARBOL_EXPORT struct HarbolBiListNode *harbol_bilist_get_head_node(const struct HarbolBiList *);
HARBOL_EXPORT struct HarbolBiListNode *harbol_bilist_get_tail_node(const struct HarbolBiList *);

HARBOL_EXPORT void harbol_bilist_from_unilist(struct HarbolBiList *, const struct HarbolUniList *);
HARBOL_EXPORT void harbol_bilist_from_hashmap(struct HarbolBiList *, const struct HarbolHashmap *);
HARBOL_EXPORT void harbol_bilist_from_vector(struct HarbolBiList *, const struct HarbolVector *);
HARBOL_EXPORT void harbol_bilist_from_graph(struct HarbolBiList *, const struct HarbolGraph *);
HARBOL_EXPORT void harbol_bilist_from_linkmap(struct HarbolBiList *, const struct HarbolLinkMap *);

HARBOL_EXPORT struct HarbolBiList *harbol_bilist_new_from_unilist(const struct HarbolUniList *);
HARBOL_EXPORT struct HarbolBiList *harbol_bilist_new_from_hashmap(const struct HarbolHashmap *);
HARBOL_EXPORT struct HarbolBiList *harbol_bilist_new_from_vector(const struct HarbolVector *);
HARBOL_EXPORT struct HarbolBiList *harbol_bilist_new_from_graph(const struct HarbolGraph *);
HARBOL_EXPORT struct HarbolBiList *harbol_bilist_new_from_linkmap(const struct HarbolLinkMap *);
/***************/


/************* Byte Buffer (bytebuffer.c) *************/
typedef struct HarbolByteBuffer {
	uint8_t *Buffer;
	size_t Len, Count;
} HarbolByteBuffer;

HARBOL_EXPORT struct HarbolByteBuffer *harbol_bytebuffer_new(void);
HARBOL_EXPORT void harbol_bytebuffer_init(struct HarbolByteBuffer *);
HARBOL_EXPORT void harbol_bytebuffer_del(struct HarbolByteBuffer *);
HARBOL_EXPORT void harbol_bytebuffer_free(struct HarbolByteBuffer **);
HARBOL_EXPORT size_t harbol_bytebuffer_get_len(const struct HarbolByteBuffer *);
HARBOL_EXPORT size_t harbol_bytebuffer_get_count(const struct HarbolByteBuffer *);
HARBOL_EXPORT uint8_t *harbol_bytebuffer_get_raw_buffer(const struct HarbolByteBuffer *);
HARBOL_EXPORT void harbol_bytebuffer_insert_byte(struct HarbolByteBuffer *, uint8_t);
HARBOL_EXPORT void harbol_bytebuffer_insert_integer(struct HarbolByteBuffer *, uint64_t, size_t);
HARBOL_EXPORT void harbol_bytebuffer_insert_float32(struct HarbolByteBuffer *, float);
HARBOL_EXPORT void harbol_bytebuffer_insert_float64(struct HarbolByteBuffer *, double);
HARBOL_EXPORT void harbol_bytebuffer_insert_cstr(struct HarbolByteBuffer *, const char [], size_t);
HARBOL_EXPORT void harbol_bytebuffer_insert_obj(struct HarbolByteBuffer *, const void *, size_t);
HARBOL_EXPORT void harbol_bytebuffer_insert_zeros(struct HarbolByteBuffer *, size_t);
HARBOL_EXPORT void harbol_bytebuffer_delete_byte(struct HarbolByteBuffer *, size_t);
HARBOL_EXPORT void harbol_bytebuffer_resize(struct HarbolByteBuffer *);
HARBOL_EXPORT void harbol_bytebuffer_to_file(const struct HarbolByteBuffer *, FILE *);
HARBOL_EXPORT size_t harbol_bytebuffer_read_from_file(struct HarbolByteBuffer *, FILE *);
HARBOL_EXPORT void harbol_bytebuffer_append(struct HarbolByteBuffer *, struct HarbolByteBuffer *);
/***************/


/************* Memory-aligned, Packed Data Structure (tuple.c) *************/
/* Tuples act like constant structs but use indexes instead of named fields. */
typedef struct HarbolTuple {
	struct HarbolVector Fields; // contains the offsets of each member
	uint8_t *Datum;
	size_t Len;
	bool Packed : 1;
} HarbolTuple;

HARBOL_EXPORT struct HarbolTuple *harbol_tuple_new(size_t, const size_t [], bool);
HARBOL_EXPORT bool harbol_tuple_free(struct HarbolTuple **);

HARBOL_EXPORT void harbol_tuple_init(struct HarbolTuple *, size_t, const size_t [], bool);
HARBOL_EXPORT void harbol_tuple_del(struct HarbolTuple *);

HARBOL_EXPORT size_t harbol_tuple_get_len(const struct HarbolTuple *);
HARBOL_EXPORT void *harbol_tuple_get_field(const struct HarbolTuple *, size_t);
HARBOL_EXPORT void *harbol_tuple_set_field(const struct HarbolTuple *, size_t, void *);

HARBOL_EXPORT size_t harbol_tuple_get_field_size(const struct HarbolTuple *, size_t);
HARBOL_EXPORT bool harbol_tuple_is_packed(const struct HarbolTuple *);
HARBOL_EXPORT bool harbol_tuple_to_struct(const struct HarbolTuple *, void *);
/***************/


/************* Memory Pool (mempool.c) *************/
// uncomment 'POOL_NO_MALLOC' if you can't or don't want to use 'malloc/calloc'.
// library will need recompiling though.

//#define POOL_NO_MALLOC

#ifdef POOL_NO_MALLOC
	#ifndef POOL_HEAPSIZE
		#define POOL_HEAPSIZE    0xFFFF //(65535)
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
		HeapMem[POOL_HEAPSIZE+1],
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
HARBOL_EXPORT void harbol_mempool_init(struct HarbolMemoryPool *);
#else
HARBOL_EXPORT void harbol_mempool_init(struct HarbolMemoryPool *, size_t);
#endif

HARBOL_EXPORT void harbol_mempool_del(struct HarbolMemoryPool *);
HARBOL_EXPORT void *harbol_mempool_alloc(struct HarbolMemoryPool *, size_t);
HARBOL_EXPORT void *harbol_mempool_realloc(struct HarbolMemoryPool *, void *, size_t);
HARBOL_EXPORT void harbol_mempool_dealloc(struct HarbolMemoryPool *, void *);
HARBOL_EXPORT void harbol_mempool_destroy(struct HarbolMemoryPool *, void *);
HARBOL_EXPORT size_t harbol_mempool_get_remaining(const struct HarbolMemoryPool *);
HARBOL_EXPORT size_t harbol_mempool_get_heap_size(const struct HarbolMemoryPool *);
HARBOL_EXPORT struct HarbolAllocNode *harbol_mempool_get_freelist(const struct HarbolMemoryPool *);
HARBOL_EXPORT bool harbol_mempool_defrag(struct HarbolMemoryPool *);
/***************/


/************* HarbolGraph (Adjacency List) (graph.c) *************/
struct HarbolGraphVertex;
typedef struct HarbolGraphEdge {
	union HarbolValue Weight;
	struct HarbolGraphVertex *VertexSocket;
} HarbolGraphEdge;

HARBOL_EXPORT struct HarbolGraphEdge *harbol_edge_new(void);
HARBOL_EXPORT struct HarbolGraphEdge *harbol_edge_new_val_vert(union HarbolValue, struct HarbolGraphVertex *);
HARBOL_EXPORT void harbol_edge_del(struct HarbolGraphEdge *, fnDestructor *);
HARBOL_EXPORT void harbol_edge_free(struct HarbolGraphEdge **, fnDestructor *);

HARBOL_EXPORT union HarbolValue harbol_edge_get_weight(const struct HarbolGraphEdge *);
HARBOL_EXPORT void harbol_edge_set_weight(struct HarbolGraphEdge *, union HarbolValue);
HARBOL_EXPORT struct HarbolGraphVertex *harbol_edge_get_vertex(const struct HarbolGraphEdge *);
HARBOL_EXPORT void harbol_edge_set_vertex(struct HarbolGraphEdge *, struct HarbolGraphVertex *);


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

HARBOL_EXPORT struct HarbolGraphVertex *harbol_vertex_new(union HarbolValue);
HARBOL_EXPORT void harbol_vertex_init(struct HarbolGraphVertex *, union HarbolValue);
HARBOL_EXPORT void harbol_vertex_del(struct HarbolGraphVertex *, fnDestructor *, fnDestructor *);
HARBOL_EXPORT void harbol_vertex_free(struct HarbolGraphVertex **, fnDestructor *, fnDestructor *);
HARBOL_EXPORT bool harbol_vertex_add_edge(struct HarbolGraphVertex *, struct HarbolGraphEdge *);
HARBOL_EXPORT struct HarbolVector *harbol_vertex_get_edges(struct HarbolGraphVertex *);
HARBOL_EXPORT union HarbolValue harbol_vertex_get_val(const struct HarbolGraphVertex *);
HARBOL_EXPORT void harbol_vertex_set_val(struct HarbolGraphVertex *, union HarbolValue);


typedef struct HarbolGraph {
	union {
		struct {
			union HarbolValue *Table;
			size_t Len, Count;
		};
		struct HarbolVector Vertices;
	};
} HarbolGraph;

HARBOL_EXPORT struct HarbolGraph *harbol_graph_new(void);
HARBOL_EXPORT void harbol_graph_init(struct HarbolGraph *);
HARBOL_EXPORT void harbol_graph_del(struct HarbolGraph *, fnDestructor *, fnDestructor *);
HARBOL_EXPORT void harbol_graph_free(struct HarbolGraph **, fnDestructor *, fnDestructor *);

HARBOL_EXPORT bool harbol_graph_insert_val(struct HarbolGraph *, union HarbolValue);
HARBOL_EXPORT bool harbol_graph_delete_val(struct HarbolGraph *, union HarbolValue, fnDestructor *, fnDestructor *);
HARBOL_EXPORT bool harbol_graph_delete_val_by_index(struct HarbolGraph *, size_t, fnDestructor *, fnDestructor *);

HARBOL_EXPORT bool harbol_graph_insert_edge(struct HarbolGraph *, size_t, size_t, union HarbolValue);
HARBOL_EXPORT bool harbol_graph_delete_edge(struct HarbolGraph *, size_t, size_t, fnDestructor *);

HARBOL_EXPORT struct HarbolGraphVertex *harbol_graph_get_vertex_by_index(struct HarbolGraph *, size_t);
HARBOL_EXPORT union HarbolValue harbol_graph_get_val_by_index(struct HarbolGraph *, size_t);
HARBOL_EXPORT void harbol_graph_set_val_by_index(struct HarbolGraph *, size_t, union HarbolValue);
HARBOL_EXPORT struct HarbolGraphEdge *harbol_graph_get_edge(struct HarbolGraph *, size_t, size_t);

HARBOL_EXPORT bool harbol_graph_is_vertex_adjacent_by_index(struct HarbolGraph *, size_t, size_t);
HARBOL_EXPORT struct HarbolVector *harbol_graph_get_vertex_neighbors(struct HarbolGraph *, size_t);

HARBOL_EXPORT struct HarbolVector *harbol_graph_get_vertex_vector(struct HarbolGraph *);
HARBOL_EXPORT size_t harbol_graph_get_vertex_count(const struct HarbolGraph *);
HARBOL_EXPORT size_t harbol_graph_get_edge_count(const struct HarbolGraph *);
HARBOL_EXPORT void harbol_graph_resize(struct HarbolGraph *);
HARBOL_EXPORT void harbol_graph_truncate(struct HarbolGraph *);

HARBOL_EXPORT void harbol_graph_from_vector(struct HarbolGraph *, const struct HarbolVector *);
HARBOL_EXPORT void harbol_graph_from_hashmap(struct HarbolGraph *, const struct HarbolHashmap *);
HARBOL_EXPORT void harbol_graph_from_unilist(struct HarbolGraph *, const struct HarbolUniList *);
HARBOL_EXPORT void harbol_graph_from_bilist(struct HarbolGraph *, const struct HarbolBiList *);
HARBOL_EXPORT void harbol_graph_from_linkmap(struct HarbolGraph *, const struct HarbolLinkMap *);

HARBOL_EXPORT struct HarbolGraph *harbol_graph_new_from_vector(const struct HarbolVector *);
HARBOL_EXPORT struct HarbolGraph *harbol_graph_new_from_hashmap(const struct HarbolHashmap *);
HARBOL_EXPORT struct HarbolGraph *harbol_graph_new_from_unilist(const struct HarbolUniList *);
HARBOL_EXPORT struct HarbolGraph *harbol_graph_new_from_bilist(const struct HarbolBiList *);
HARBOL_EXPORT struct HarbolGraph *harbol_graph_new_from_linkmap(const struct HarbolLinkMap *);
/***************/


/************* General Tree (tree.c) *************/
typedef struct HarbolTree {
	struct HarbolVector Children;
	union HarbolValue Data;
} HarbolTree;

HARBOL_EXPORT struct HarbolTree *harbol_tree_new(union HarbolValue);
HARBOL_EXPORT void harbol_tree_init(struct HarbolTree *);
HARBOL_EXPORT void harbol_tree_init_val(struct HarbolTree *, union HarbolValue);
HARBOL_EXPORT void harbol_tree_del(struct HarbolTree *, fnDestructor *);
HARBOL_EXPORT void harbol_tree_free(struct HarbolTree **, fnDestructor *);

HARBOL_EXPORT bool harbol_tree_insert_child_node(struct HarbolTree *, struct HarbolTree *);
HARBOL_EXPORT bool harbol_tree_insert_child_val(struct HarbolTree *, union HarbolValue);

HARBOL_EXPORT bool harbol_tree_delete_child_by_ref(struct HarbolTree *, struct HarbolTree **, fnDestructor *);
HARBOL_EXPORT bool harbol_tree_delete_child_by_index(struct HarbolTree *, size_t, fnDestructor *);
HARBOL_EXPORT bool harbol_tree_delete_child_by_val(struct HarbolTree *, union HarbolValue, fnDestructor *);

HARBOL_EXPORT struct HarbolTree *harbol_tree_get_child_by_index(const struct HarbolTree *, size_t);
HARBOL_EXPORT struct HarbolTree *harbol_tree_get_child_by_val(const struct HarbolTree *, union HarbolValue);
HARBOL_EXPORT union HarbolValue harbol_tree_get_val(const struct HarbolTree *);
HARBOL_EXPORT void harbol_tree_set_val(struct HarbolTree *, union HarbolValue);
HARBOL_EXPORT struct HarbolVector *harbol_tree_get_children_vector(struct HarbolTree *);
HARBOL_EXPORT size_t harbol_tree_get_children_len(const struct HarbolTree *);
HARBOL_EXPORT size_t harbol_tree_get_children_count(const struct HarbolTree *);
/***************/


/************* Ordered Hash Map (preserves insertion order) (linkmap.c) *************/
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

HARBOL_EXPORT struct HarbolLinkMap *harbol_linkmap_new(void);
HARBOL_EXPORT void harbol_linkmap_init(struct HarbolLinkMap *);
HARBOL_EXPORT void harbol_linkmap_del(struct HarbolLinkMap *, fnDestructor *);
HARBOL_EXPORT void harbol_linkmap_free(struct HarbolLinkMap **, fnDestructor *);
HARBOL_EXPORT size_t harbol_linkmap_get_count(const struct HarbolLinkMap *);
HARBOL_EXPORT size_t harbol_linkmap_get_len(const struct HarbolLinkMap *);
HARBOL_EXPORT bool harbol_linkmap_rehash(struct HarbolLinkMap *);

HARBOL_EXPORT bool harbol_linkmap_insert(struct HarbolLinkMap *, const char [], union HarbolValue);
HARBOL_EXPORT bool harbol_linkmap_insert_node(struct HarbolLinkMap *, struct HarbolKeyValPair *);

HARBOL_EXPORT struct HarbolKeyValPair *harbol_linkmap_get_node_by_index(const struct HarbolLinkMap *, size_t);
HARBOL_EXPORT union HarbolValue harbol_linkmap_get(const struct HarbolLinkMap *, const char []);
HARBOL_EXPORT void harbol_linkmap_set(struct HarbolLinkMap *, const char [], union HarbolValue);
HARBOL_EXPORT union HarbolValue harbol_linkmap_get_by_index(const struct HarbolLinkMap *, size_t);
HARBOL_EXPORT void harbol_linkmap_set_by_index(struct HarbolLinkMap *, size_t, union HarbolValue);

HARBOL_EXPORT void harbol_linkmap_delete(struct HarbolLinkMap *, const char [], fnDestructor *);
HARBOL_EXPORT void harbol_linkmap_delete_by_index(struct HarbolLinkMap *, size_t, fnDestructor *);
HARBOL_EXPORT bool harbol_linkmap_has_key(const struct HarbolLinkMap *, const char []);
HARBOL_EXPORT struct HarbolKeyValPair *harbol_linkmap_get_node_by_key(const struct HarbolLinkMap *, const char []);
HARBOL_EXPORT struct HarbolVector *harbol_linkmap_get_buckets(const struct HarbolLinkMap *);

HARBOL_EXPORT union HarbolValue *harbol_linkmap_get_iter(const struct HarbolLinkMap *);
HARBOL_EXPORT union HarbolValue *harbol_linkmap_get_iter_end_len(const struct HarbolLinkMap *);
HARBOL_EXPORT union HarbolValue *harbol_linkmap_get_iter_end_count(const struct HarbolLinkMap *);

HARBOL_EXPORT size_t harbol_linkmap_get_index_by_name(const struct HarbolLinkMap *, const char []);
HARBOL_EXPORT size_t harbol_linkmap_get_index_by_node(const struct HarbolLinkMap *, struct HarbolKeyValPair *);
HARBOL_EXPORT size_t harbol_linkmap_get_index_by_val(const struct HarbolLinkMap *, union HarbolValue);

HARBOL_EXPORT void harbol_linkmap_from_hashmap(struct HarbolLinkMap *, const struct HarbolHashmap *);
HARBOL_EXPORT void harbol_linkmap_from_unilist(struct HarbolLinkMap *, const struct HarbolUniList *);
HARBOL_EXPORT void harbol_linkmap_from_bilist(struct HarbolLinkMap *, const struct HarbolBiList *);
HARBOL_EXPORT void harbol_linkmap_from_vector(struct HarbolLinkMap *, const struct HarbolVector *);
HARBOL_EXPORT void harbol_linkmap_from_graph(struct HarbolLinkMap *, const struct HarbolGraph *);

HARBOL_EXPORT struct HarbolLinkMap *harbol_linkmap_new_from_hashmap(const struct HarbolHashmap *);
HARBOL_EXPORT struct HarbolLinkMap *harbol_linkmap_new_from_unilist(const struct HarbolUniList *);
HARBOL_EXPORT struct HarbolLinkMap *harbol_linkmap_new_from_bilist(const struct HarbolBiList *);
HARBOL_EXPORT struct HarbolLinkMap *harbol_linkmap_new_from_vector(const struct HarbolVector *);
HARBOL_EXPORT struct HarbolLinkMap *harbol_linkmap_new_from_graph(const struct HarbolGraph *);
/***************/


/************* Tagged Union Type (variant.c) *************/
// discriminated union type
typedef struct HarbolVariant {
	union HarbolValue Val;
	int32_t TypeTag;
} HarbolVariant;

HARBOL_EXPORT struct HarbolVariant *harbol_variant_new(union HarbolValue, int32_t);
HARBOL_EXPORT void harbol_variant_free(struct HarbolVariant **, fnDestructor *);
HARBOL_EXPORT void harbol_variant_init(struct HarbolVariant *, union HarbolValue, int32_t);
HARBOL_EXPORT void harbol_variant_del(struct HarbolVariant *, fnDestructor *);

HARBOL_EXPORT union HarbolValue harbol_linkmap_get_val(const struct HarbolVariant *);
HARBOL_EXPORT void harbol_linkmap_set_val(struct HarbolVariant *, union HarbolValue);

HARBOL_EXPORT int32_t harbol_linkmap_get_type(const struct HarbolVariant *);
HARBOL_EXPORT void harbol_linkmap_set_type(struct HarbolVariant *, int32_t);
/***************/


/************* Minimal JSON-like Configuration File Parser (cfg.c) *************/
typedef enum HarbolCfgType {
	HarbolTypeNull=0,
	HarbolTypeLinkMap,
	HarbolTypeString,
	HarbolTypeFloat,
	HarbolTypeInt,
	HarbolTypeBool,
	HarbolTypeColor,
	HarbolTypeVec4D,
} HarbolCfgType;

typedef union HarbolColor {
	uint32_t UIntColor;
	struct{ uint8_t R,G,B,A; };
	uint8_t RGBA[4];
} HarbolColor;

typedef union HarbolVec4D {
	struct{ float X,Y,Z,W; };
	float XYZW[4];
} HarbolVec4D;

HARBOL_EXPORT struct HarbolLinkMap *harbol_cfg_from_file(const char []);
HARBOL_EXPORT struct HarbolLinkMap *harbol_cfg_parse_cstr(const char []);
HARBOL_EXPORT bool harbol_cfg_free(struct HarbolLinkMap **);
HARBOL_EXPORT bool harbol_cfg_to_str(const struct HarbolLinkMap *, struct HarbolString *);

HARBOL_EXPORT struct HarbolLinkMap *harbol_cfg_get_section_by_key(struct HarbolLinkMap *, const char []);
HARBOL_EXPORT struct HarbolString *harbol_cfg_get_str_by_key(struct HarbolLinkMap *, const char []);
HARBOL_EXPORT double harbol_cfg_get_float_by_key(struct HarbolLinkMap *, const char []);
HARBOL_EXPORT int64_t harbol_cfg_get_int_by_key(struct HarbolLinkMap *, const char []);
HARBOL_EXPORT bool harbol_cfg_get_bool_by_key(struct HarbolLinkMap *, const char [], bool *);
HARBOL_EXPORT bool harbol_cfg_get_color_by_key(struct HarbolLinkMap *, const char [], union HarbolColor *);
HARBOL_EXPORT bool harbol_cfg_get_vec4D_by_key(struct HarbolLinkMap *, const char [], union HarbolVec4D *);
/***************/


#ifdef __cplusplus
}
#endif
