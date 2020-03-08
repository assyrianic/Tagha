#pragma once

#define TAGHA_VERSION_MAJOR    1
#define TAGHA_VERSION_MINOR    0
#define TAGHA_VERSION_PATCH    0
#define TAGHA_VERSION_PHASE    'beta'
#define TAGHA_STR_HELPER(x)    #x
#define TAGHA_STRINGIFY(x)     TAGHA_STR_HELPER(x)
#define TAGHA_VERSION_STRING   TAGHA_STRINGIFY(TAGHA_VERSION_MAJOR) "." TAGHA_STRINGIFY(TAGHA_VERSION_MINOR) "." TAGHA_STRINGIFY(TAGHA_VERSION_PATCH) " " TAGHA_STRINGIFY(TAGHA_VERSION_PHASE)

#ifdef __cplusplus
extern "C" {
#endif

#include "harbol_common_defines.h"
#include "harbol_common_includes.h"
#include "allocators/mempool.h"


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


typedef union TaghaVal {
	uint64_t       uint64;
	int64_t        int64;
	
	intptr_t       intptr;
	uintptr_t      uintptr;
	
	size_t         size;
	ssize_t        ssize;
	
	uint32_t       uint32,  uint32a[2];
	int32_t        int32,   int32a[2];
	
	uint16_t       uint16,  uint16a[4];
	int16_t        int16,   int16a[4];
	
	uint8_t        uint8,   uint8a[8];
	int8_t         int8,    int8a[8];
	bool           boolean, boola[8];
	
#ifdef TAGHA_FLOAT32_DEFINED
	float32_t      float32, float32a[2];
#endif
#ifdef TAGHA_FLOAT64_DEFINED
	float64_t      float64;
#endif
} UTaghaVal;


typedef union TaghaPtr {
	const uint64_t       *restrict ptruint64;
	const uint32_t       *restrict ptruint32;
	const uint16_t       *restrict ptruint16;
	const uint8_t        *restrict ptruint8;
	
	const int64_t        *restrict ptrint64;
	const int32_t        *restrict ptrint32;
	const int16_t        *restrict ptrint16;
	const int8_t         *restrict ptrint8;
	
	const size_t         *restrict ptrsize;
	const ssize_t        *restrict ptrssize;
#ifdef TAGHA_FLOAT32_DEFINED
	const float32_t      *restrict ptrfloat32;
#endif
#ifdef TAGHA_FLOAT64_DEFINED
	const float64_t      *restrict ptrfloat64;
#endif
	const char           *restrict string;
	
	const union TaghaVal *restrict ptrval;
	const union TaghaPtr *restrict ptrself;
	const void           *restrict ptrvoid;
} UTaghaPtr;


enum {
	TAGHA_MAGIC_VERIFIER = 0x7A6AC0DE
};

typedef struct TaghaHeader {
	uint32_t
		magic,
		stacksize,
		memsize,
		flags
	;
} STaghaHeader;

typedef struct TaghaItemEntry {
	uint32_t
		size,
		flags,
		name_len,
		data_len
	;
} STaghaItemEntry;

/** Script File/Binary Format Structure
 * ------------------------------ start of header ------------------------------
 * 4 bytes: magic verifier ==> TAGHA_MAGIC_VERIFIER
 * 4 bytes: stack size, stack size needed for the code.
 * 4 bytes: mem region size.
 * 4 bytes: flags.
 * ------------------------------ end of header ------------------------------
 * .functions table.
 * 4 bytes: amount of funcs.
 * n bytes: func table.
 *     4 bytes: entry size.
 *     4 bytes: 0 if bytecode func, 1 if it's a native, other flags.
 *     4 bytes: string size + '\0' of func string.
 *     4 bytes: instr len, 8 if native.
 *     n bytes: func string.
 *     if bytecode func: n bytes - instructions.
 * 
 * .globalvars table.
 * 4 bytes: amount of global vars.
 * n bytes: global vars table.
 *     4 bytes: entry size.
 *     4 bytes: flags.
 *     4 bytes: string size + '\0' of global var string.
 *     4 bytes: byte size, 8 if ptr.
 *     n bytes: global var string.
 *     n bytes: data. All 0 if not initialized in script code.
 * 
 * .mem region - taken control by the memory pool as both a stack and heap.
 */


#define TAGHA_REG_FILE                               \
	Y(alaf)  Y(beth)   Y(gamal) Y(dalath) Y(heh)     \
	Y(waw)   Y(zain)   Y(heth)  Y(teth)   Y(yodh)    \
	Y(kaf)   Y(lamadh) Y(meem)  Y(noon)   Y(semkath) \
	Y(_eh)   Y(peh)    Y(sadhe) Y(qof)    Y(reesh)   \
	Y(sheen) Y(taw)    Y(veth)  Y(ghamal) Y(dhalath) \
	Y(khaf)  Y(feh)    Y(thaw)  Y(zeth)   Y(dadeh)   \
	Y(sp)    Y(bp)
	/** Syriac alphabet makes great register names! */

#define Y(y) y,
typedef enum TaghaRegID { TAGHA_REG_FILE MaxRegisters } ETaghaRegID;
#undef Y

#ifndef TAGHA_FIRST_PARAM_REG
#	define TAGHA_FIRST_PARAM_REG    semkath
#endif

#ifndef TAGHA_LAST_PARAM_REG
#	define TAGHA_LAST_PARAM_REG     dadeh
#endif

#ifndef TAGHA_REG_PARAMS_MAX
#	define TAGHA_REG_PARAMS_MAX     (TAGHA_LAST_PARAM_REG - TAGHA_FIRST_PARAM_REG + 1)
#endif


struct TaghaModule;
typedef union TaghaVal TaghaCFunc(struct TaghaModule *ctxt, size_t args, const union TaghaVal params[]);

typedef struct TaghaNative {
	const char *name;
	TaghaCFunc *cfunc;
} STaghaNative;


typedef enum TaghaErrCode {
	tagha_err_instr_oob = -1,
	tagha_err_none=0,
	tagha_err_bad_ptr,
	tagha_err_no_func,
	tagha_err_no_cfunc,
	tagha_err_bad_script,
	tagha_err_stk_size,
	tagha_err_stk_overflow,
} ETaghaErrCode;

/** Tagha Item
 *  represents either a function or global variable.
 */
enum {
	TAGHA_FLAG_NATIVE = 1,    /** if is a native C or JIT compiled function. */
	TAGHA_FLAG_LINKED         /** ptr to native/jit function is linked and verified. */
};

typedef struct TaghaItem {
	void     *item;
	size_t   bytes;
	uint32_t flags; /// 0-bytecode based, 1-native based, 2-resolved
} STaghaItem;

typedef struct TaghaItemMap {
	struct {
		struct TaghaKeyVal {
			const char *key;
			size_t keylen;
			struct TaghaItem *val;
		} *table;
		size_t len;
	} *buckets;
	struct TaghaItem *array;
	size_t arrlen, hashlen;
} STaghaItemMap;


/** Script/Module Structure.
 * Consists of:
 * A virtual machine context aka CPU.
 * An internal stack + heap controlled by a freelist allocator.
 * Dynamic symbol tables for functions and global variables.
 * An internal stack.
 * pointer to the raw script data.
 * and error code status.
 */
typedef struct TaghaModule {
	struct HarbolMemPool heap;
	struct TaghaItemMap funcs, vars;
	struct { union TaghaVal *start; size_t size; } stack;
	union TaghaVal regs[MaxRegisters];
	uint8_t *script, *ip;
	uintptr_t start_seg, end_seg;
	enum TaghaErrCode errcode;
	uint32_t condflag, flags;
} STaghaModule;


TAGHA_EXPORT NO_NULL struct TaghaModule *tagha_module_new_from_file(const char filename[]);
TAGHA_EXPORT NO_NULL struct TaghaModule *tagha_module_new_from_buffer(uint8_t buffer[]);
TAGHA_EXPORT NO_NULL bool tagha_module_free(struct TaghaModule **moduleref);

TAGHA_EXPORT NO_NULL struct TaghaModule tagha_module_create_from_file(const char filename[]);
TAGHA_EXPORT NO_NULL struct TaghaModule tagha_module_create_from_buffer(uint8_t buffer[]);
TAGHA_EXPORT NO_NULL bool tagha_module_clear(struct TaghaModule *module);

TAGHA_EXPORT NO_NULL void tagha_module_print_vm_state(const struct TaghaModule *module, bool hex);
TAGHA_EXPORT NO_NULL NONNULL_RET const char *tagha_module_get_error(const struct TaghaModule *module);
TAGHA_EXPORT NO_NULL bool tagha_module_register_natives(struct TaghaModule *module, const struct TaghaNative natives[]);
TAGHA_EXPORT NO_NULL bool tagha_module_register_ptr(struct TaghaModule *module, const char name[], void *ptr);

TAGHA_EXPORT NO_NULL void *tagha_module_get_var(struct TaghaModule *module, const char name[]);
TAGHA_EXPORT NO_NULL uint32_t tagha_module_get_flags(const struct TaghaModule *module);

TAGHA_EXPORT NEVER_NULL(1,2) int32_t tagha_module_call(struct TaghaModule *module, const char name[], size_t args, const union TaghaVal params[], union TaghaVal *retval);
TAGHA_EXPORT NEVER_NULL(1) int32_t tagha_module_invoke(struct TaghaModule *module, int64_t func_index, size_t args, const union TaghaVal params[], union TaghaVal *retval);
TAGHA_EXPORT NEVER_NULL(1) int32_t tagha_module_run(struct TaghaModule *module, size_t argc, const union TaghaVal argv[]);
TAGHA_EXPORT NO_NULL void tagha_module_throw_error(struct TaghaModule *module, int32_t err);
TAGHA_EXPORT NO_NULL void tagha_module_jit_compile(struct TaghaModule *module, TaghaCFunc *jitfunc(const uint8_t*, size_t, void *), void *userdata);


#define TAGHA_INSTR_SET \
	X(halt) \
	X(push) X(pop) \
	\
	X(ldvar) X(ldaddr) X(ldfunc) \
	X(movi) X(mov) \
	X(ld1) X(ld2) X(ld4) X(ld8) \
	X(st1) X(st2) X(st4) X(st8) \
	\
	X(add) X(sub) X(mul) X(divi) X(mod) \
	X(bit_and) X(bit_or) X(bit_xor) X(bit_not) X(shl) X(shr) X(neg) \
	\
	X(ilt) X(ile) X(igt) X(ige) \
	X(ult) X(ule) X(ugt) X(uge) X(cmp) \
	\
	X(jmp) X(jz) X(jnz) \
	X(call) X(callr) X(ret) \
	X(nop) \
	\
	X(f32tof64) X(f64tof32) X(itof64) X(itof32) X(f64toi) X(f32toi) \
	X(addf) X(subf) X(mulf) X(divf) X(negf) \
	X(ltf) X(lef) X(gtf) X(gef)

#define X(x) x,
typedef enum TaghaInstrSet { TAGHA_INSTR_SET } ETaghaInstrSet;
#undef X

#ifdef __cplusplus
}
#endif
