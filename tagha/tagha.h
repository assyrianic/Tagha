#ifndef TAGHA_INCLUDED
#	define TAGHA_INCLUDED

#define TAGHA_VERSION_MAJOR    1
#define TAGHA_VERSION_MINOR    2
#define TAGHA_VERSION_PATCH    0
#define TAGHA_VERSION_PHASE    "beta"
#define TAGHA_STR_HELPER(x)    #x
#define TAGHA_STRINGIFY(x)     TAGHA_STR_HELPER(x)
#define TAGHA_VERSION_STRING   TAGHA_STRINGIFY(TAGHA_VERSION_MAJOR) "." TAGHA_STRINGIFY(TAGHA_VERSION_MINOR) "." TAGHA_STRINGIFY(TAGHA_VERSION_PATCH) " " TAGHA_VERSION_PHASE

#ifdef __cplusplus
extern "C" {
#endif

#include "harbol_common_defines.h"
#include "harbol_common_includes.h"
#include "allocators/mempool/mempool.h"


#define TAGHA_FLOAT32_DEFINED    /// allow tagha to use 32-bit floats
#define TAGHA_FLOAT64_DEFINED    /// allow tagha to use 64-bit floats

#if defined(TAGHA_FLOAT32_DEFINED) || defined(TAGHA_FLOAT64_DEFINED)
#	ifndef TAGHA_FLOATS_ENABLED
#		define TAGHA_FLOATS_ENABLED
#	endif
#endif

#ifdef TAGHA_DLL
#	ifndef TAGHA_LIB
#		define TAGHA_EXPORT    __declspec(dllimport)
#	else
#		define TAGHA_EXPORT    __declspec(dllexport)
#	endif
#else
#	define TAGHA_EXPORT 
#endif


union TaghaVal {
	uint64_t       uint64;
	int64_t        int64;
#ifdef TAGHA_FLOAT64_DEFINED
	float64_t      float64;
#endif
	
	/// for general pointer data, use (u)intptr. 
	intptr_t       intptr;
	uintptr_t      uintptr;
	
	size_t         size;
	ssize_t        ssize;
	
	uint32_t       uint32,  uint32a[sizeof(uint64_t) / sizeof(uint32_t)];
	int32_t        int32,   int32a[sizeof(uint64_t) / sizeof(int32_t)];
#ifdef TAGHA_FLOAT32_DEFINED
	float32_t      float32, float32a[sizeof(uint64_t) / sizeof(float32_t)];
#endif
	
	uint16_t       uint16,  uint16a[sizeof(uint64_t) / sizeof(uint16_t)];
	int16_t        int16,   int16a[sizeof(uint64_t) / sizeof(int16_t)];
	
	uint8_t        uint8,   uint8a[sizeof(uint64_t) / sizeof(uint8_t)];
	int8_t         int8,    int8a[sizeof(uint64_t) / sizeof(int8_t)];
	bool           b00l,    boola[sizeof(uint64_t) / sizeof(bool)];
};


union TaghaPtr {
	const uint64_t       *uint64;
	const uint32_t       *uint32;
	const uint16_t       *uint16;
	const uint8_t        *uint8;
	
	const int64_t        *int64;
	const int32_t        *int32;
	const int16_t        *int16;
	const int8_t         *int8;
	
#ifdef TAGHA_FLOAT32_DEFINED
	const float32_t      *float32;
#endif
#ifdef TAGHA_FLOAT64_DEFINED
	const float64_t      *float64;
#endif
	const char           *cstr;
	const union TaghaVal *val;
};


#define TAGHA_INSTR_SET \
	X(halt) X(nop) \
	\
	/** data movement ops. */ \
	X(alloc) X(redux) X(movi) X(mov) \
	\
	/** memory ops. */ \
	X(lra) X(lea) X(ldvar) X(ldfn) \
	X(ld1) X(ld2) X(ld4) X(ld8) X(ldu1) X(ldu2) X(ldu4) \
	X(st1) X(st2) X(st4) X(st8) \
	\
	/** arithmetic ops. */ \
	X(add)  X(sub)  X(mul)  X(idiv) X(mod) X(neg) \
	X(fadd) X(fsub) X(fmul) X(fdiv) X(fneg) \
	\
	/** bit-wise ops. */ \
	X(bit_and) X(bit_or) X(bit_xor) X(shl) X(shr) X(shar) X(bit_not) \
	\
	/** comparison ops. */ \
	X(ilt) X(ile) X(ult) X(ule) X(cmp) X(flt) X(fle) X(setc) \
	\
	/** float<=>int conversion ops. */ \
	X(f32tof64) X(f64tof32) X(itof64) X(itof32) X(f64toi) X(f32toi) \
	\
	/** control flow ops. */ \
	X(jmp) X(jz) X(jnz) \
	\
	/** function ops. */ \
	X(pushlr) X(poplr) X(call) X(callr) X(ret) \
	\
	/** vector extension. */ \
	X(setvlen) X(setelen) \
	X(vmov) \
	X(vadd)  X(vsub)  X(vmul)  X(vdiv)  X(vmod) X(vneg) \
	X(vfadd) X(vfsub) X(vfmul) X(vfdiv) X(vfneg) \
	X(vand)  X(vor)   X(vxor)  X(vshl)  X(vshr) X(vshar) X(vnot) \
	X(vcmp)  X(vilt)  X(vile)  X(vult)  X(vule) X(vflt)  X(vfle)

#define X(x) x,
enum TaghaInstrSet { TAGHA_INSTR_SET MaxOps };
#undef X


enum {
	TAGHA_MAGIC_VERIFIER = 0x7A6AC0DE    /// "tagha code"
};
struct TaghaModuleHeader {
	uint32_t
		magic,
		opstacksize,
		callstacksize,
		stacksize,
		heapsize,
		memsize,
		funcs_offset,
		func_count,
		vars_offset,
		var_count,
		mem_offset,
		flags
	;
};

struct TaghaItemEntry {
	uint32_t
		size,
		flags,
		name_len,
		data_len
	;
};

/** Tagha Script File/Binary Format Structure
 * ------------------------------ start of header ------------------------------
 * 4 bytes: magic verifier ==> TAGHA_MAGIC_VERIFIER
 * 4 bytes: operand stack size.
 * 4 bytes: call stack size.
 * 4 bytes: total stack size.
 * 4 bytes: heap size.
 * 4 bytes: total mem size.
 * 4 bytes: func table offset (from base).
 * 4 bytes: amount of funcs.
 * 4 bytes: var table offset (from base).
 * 4 bytes: amount of vars.
 * 4 bytes: mem region offset (from base).
 * 4 bytes: flags.
 * ------------------------------ end of header --------------------------------
 * .funcs table.
 * n bytes: func table.
 *     4 bytes: entry size.
 *     4 bytes: flags: if bytecode func, a native, or extern.
 *     4 bytes: string size + '\0' of func string.
 *     4 bytes: instr len, 8 if native.
 *     n bytes: func string.
 *     if bytecode func:
 *         n bytes - instructions.
 *     else if native func:
 *         8 bytes - function pointer to native.
 *     else if extern func:
 *         8 bytes - pointer to owning module.
 * 
 * .vars table.
 * n bytes: global vars table. == low segment
 *     4 bytes: entry size.
 *     4 bytes: flags.
 *     4 bytes: string size + '\0' of global var string.
 *     4 bytes: byte size, 8 if ptr.
 *     n bytes: global var string.
 *     n bytes: data. All 0 if not initialized in script code.
 * 
 * .mem region - taken control by the memory pool as both a stack and heap.
 *     | portion marshalled by the bifurcated stack:
 *     |    <- operand stack start
 *     |    ...
 *     |    <- operand stack end => highest memory safety segment.
 *     |    <- call stack end
 *     |    ...
 *     |    <- call stack start, unaccessable from bytecode.
 *     | remaining memory region portion marshalled by pool allocator as the heap.
 */


struct TaghaModule;
typedef union TaghaVal TaghaCFunc(struct TaghaModule *ctxt, const union TaghaVal params[]);

struct TaghaNative {
	const char *name;
	TaghaCFunc *cfunc;
};


enum {
	TAGHA_FLAG_NATIVE = 1,  /// if is a native C or JIT compiled function.
	TAGHA_FLAG_EXTERN = 2,  /// function is from different module.
	TAGHA_FLAG_LINKED = 4,  /// ptr has been linked.
};
struct TaghaItem {
	uintptr_t
		item, /// data, as uint8_t*
		owner /// Add an owner so we can do dynamic linking & loading.
	;
	size_t    bytes;
	uint32_t  flags;
};
typedef const struct TaghaItem *TaghaFunc;


enum { TAGHA_SYM_BUCKETS = 32 };
struct TaghaSymTable {
	const char **keys;              /// array of string names of each item.
	struct TaghaItem *table;        /// table of items.
	size_t
		len,                        /// table's len.
		buckets[TAGHA_SYM_BUCKETS], /// hash index bucket for each index. SIZE_MAX if invalid.
		*hashes,                    /// hash value for each item index.
		*chain                      /// index chain to resolve collisions. SIZE_MAX if invalid.
	;
};


enum TaghaErrCode {
	TaghaErrNone,      /// a-okay!
	TaghaErrBadNative, /// missing native function. (native wasn't linked.)
	TaghaErrBadExtern, /// missing extern function. (extern wasn't linked.)
	TaghaErrOpStackOF, /// op stack overflow!
	TaghaErrOpcodeOOB, /// opcode out of bounds!
	TaghaErrBadPtr,    /// nil/invalid pointer.
	TaghaErrBadFunc,   /// nil function.
};


/// Script/Module Structure.
struct TaghaModule {
	struct HarbolMemPool heap;   /// holds ALL memory in a script.
	const struct TaghaSymTable *funcs, *vars;
	uintptr_t
		script,     /// ptr to base address of script (uint8_t*)
		ip,         /// instruction ptr (uint8_t*)
		low_seg,    /// lower  memory segment (uint8_t*)
		high_seg,   /// higher memory segment (uint8_t*)
		opstack,    /// ptr to base of operand stack (union TaghaVal*)
		callstack,  /// ptr to base of call stack (uintptr_t*)
		osp,        /// operand stack ptr (union TaghaVal*)
		ofp,        /// operand frame ptr (union TaghaVal*)
		csp,        /// call stack ptr (uintptr_t*)
		lr          /// link register.
	;
	size_t        opstack_size, callstack_size;
	uint_fast16_t vec_len, elem_len;
	uint32_t      flags;
	int           err, cond;
};

/// Module Constructors.
TAGHA_EXPORT NO_NULL struct TaghaModule *tagha_module_new_from_file(const char filename[]);
TAGHA_EXPORT NO_NULL struct TaghaModule *tagha_module_new_from_buffer(uint8_t buffer[]);

/// Module Destructors.
TAGHA_EXPORT bool tagha_module_clear(struct TaghaModule *module);
TAGHA_EXPORT bool tagha_module_free(struct TaghaModule **modref);

/// Calling/Execution API.
TAGHA_EXPORT NEVER_NULL(1,2) bool tagha_module_call(struct TaghaModule *module, const char name[], size_t args, const union TaghaVal params[], union TaghaVal *retval);

TAGHA_EXPORT NEVER_NULL(1,2) bool tagha_module_invoke(struct TaghaModule *module, TaghaFunc func, size_t args, const union TaghaVal params[], union TaghaVal *retval);

TAGHA_EXPORT NEVER_NULL(1) int tagha_module_run(struct TaghaModule *module, size_t argc, const union TaghaVal argv[]);

/// Runtime Data API.
TAGHA_EXPORT NO_NULL void *tagha_module_get_var(const struct TaghaModule *module, const char name[]);
TAGHA_EXPORT NO_NULL TaghaFunc tagha_module_get_func(const struct TaghaModule *module, const char name[]);
TAGHA_EXPORT NO_NULL uint32_t tagha_module_get_flags(const struct TaghaModule *module);

TAGHA_EXPORT NO_NULL uintptr_t tagha_module_heap_alloc(struct TaghaModule *module, size_t size);
TAGHA_EXPORT NO_NULL bool tagha_module_heap_free(struct TaghaModule *module, uintptr_t ptr);

/// Error API.
TAGHA_EXPORT NO_NULL NONNULL_RET const char *tagha_module_get_err(const struct TaghaModule *module);
TAGHA_EXPORT NO_NULL void tagha_module_throw_err(struct TaghaModule *module, enum TaghaErrCode err);

/// Inter-Module/Process Linking API.
TAGHA_EXPORT NO_NULL void tagha_module_link_natives(struct TaghaModule *module, const struct TaghaNative natives[]);
TAGHA_EXPORT NO_NULL bool tagha_module_link_ptr(struct TaghaModule *module, const char name[], uintptr_t ptr);
TAGHA_EXPORT NO_NULL void tagha_module_link_module(struct TaghaModule *module, const struct TaghaModule *lib);

/** I like Golang.
	type TaghaSys struct {
		modules map[string]*TaghaModule // map[string]TaghaFunc
		natives map[string]TaghaCFunc
	}
 */

#ifdef __cplusplus
} /// extern "C"
#endif

#endif /** TAGHA_INCLUDED */