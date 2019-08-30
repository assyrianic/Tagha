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


#define TAGHA_FLOAT32_DEFINED    // allow tagha to use 32-bit floats
#define TAGHA_FLOAT64_DEFINED    // allow tagha to use 64-bit floats

#if defined(TAGHA_FLOAT32_DEFINED) || defined(TAGHA_FLOAT64_DEFINED)
#	ifndef TAGHA_USE_FLOATS
#		define TAGHA_USE_FLOATS
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
	uint64_t uint64, *ptruint64;
	int64_t int64, *ptrint64;
	
	size_t size, *ptrsize; ssize_t ssize, *ptrssize;
	uintptr_t uintptr; intptr_t intptr;
	
	uint32_t uint32, *ptruint32;
	int32_t int32, *ptrint32;
	
	uint16_t uint16, *ptruint16;
	int16_t int16, *ptrint16;
	
	uint8_t uint8, *ptruint8;
	int8_t int8, *ptrint8;
	bool boolean, *ptrbool;
	
#ifdef TAGHA_FLOAT32_DEFINED
	float32_t float32, *ptrfloat32;
#endif
#ifdef TAGHA_FLOAT64_DEFINED
	float64_t float64, *ptrfloat64;
#endif
	
	void *ptrvoid;
	char *string;
	union TaghaVal *ptrself;
} UTaghaVal;

typedef union TaghaPtr {
	uint64_t *restrict ptruint64;
	uint32_t *restrict ptruint32;
	uint16_t *restrict ptruint16;
	uint8_t *restrict ptruint8;
	
	int64_t *restrict ptrint64;
	int32_t *restrict ptrint32;
	int16_t *restrict ptrint16;
	int8_t *restrict ptrint8;
	
	size_t *restrict ptrsize;
	ssize_t *restrict ptrssize;
#ifdef TAGHA_FLOAT32_DEFINED
	float32_t *restrict ptrfloat32;
#endif
#ifdef TAGHA_FLOAT64_DEFINED
	float64_t *restrict ptrfloat64;
#endif
	char *restrict string;
	
	union TaghaVal *restrict ptrval;
	union TaghaPtr *restrict ptrself;
	void *restrict ptrvoid;
} UTaghaPtr;


#define TAGHA_MAGIC_VERIFIER    0xC0DE
/* Script File/Binary Format Structure
 * ------------------------------ start of header ------------------------------
 * 2 bytes: magic verifier ==> 0xC0DE
 * 4 bytes: stack size, stack size needed for the code.
 * 4 bytes: mem region size.
 * 1 byte: flags
 * ------------------------------ end of header ------------------------------
 * .functions table
 * 4 bytes: amount of funcs
 * n bytes: func table
 *     1 byte: 0 if bytecode func, 1 if it's a native, other flags.
 *     4 bytes: string size + '\0' of func string
 *     4 bytes: instr len, 8 if native.
 *     n bytes: func string
 *     if bytecode func: n bytes - instructions
 * 
 * .globalvars table
 * 4 bytes: amount of global vars
 * n bytes: global vars table
 *     1 byte: flags
 *     4 bytes: string size + '\0' of global var string
 *     4 bytes: byte size, 8 if ptr.
 *     n bytes: global var string
 *     if bytecode var: n bytes: data. All 0 if not initialized in script code.
 *     else: 8 bytes: var address (0 at first, filled in during runtime)
 * 
 * .mem region - taken control by the memory pool as both a stack and heap.
 */

struct TaghaModule;
typedef union TaghaVal FnTaghaNative(struct TaghaModule *ctxt, size_t args, const union TaghaVal params[]);

typedef struct TaghaNative {
	const char *name;
	FnTaghaNative *cfunc;
} STaghaNative;


#define TAGHA_REGISTER_FILE \
	Y(alaf) Y(beth) Y(gamal) Y(dalath) \
	Y(heh) Y(waw) Y(zain) Y(heth) Y(teth) Y(yodh) Y(kaf) \
	Y(lamadh) Y(meem) Y(noon) Y(semkath) Y(_eh) \
	Y(peh) Y(sadhe) Y(qof) Y(reesh) Y(sheen) Y(taw) \
	/* Syriac alphabet makes great register names! */ \
	Y(stkptr) Y(baseptr) Y(instr)

#define Y(y) y,
typedef enum TaghaRegID { TAGHA_REGISTER_FILE MaxRegisters } ETaghaRegID;
#undef Y

#ifndef TAGHA_FIRST_PARAM_REG
#	define TAGHA_FIRST_PARAM_REG    semkath
#endif

#ifndef TAGHA_LAST_PARAM_REG
#	define TAGHA_LAST_PARAM_REG     taw
#endif

#ifndef TAGHA_REG_PARAMS_MAX
#	define TAGHA_REG_PARAMS_MAX     (TAGHA_LAST_PARAM_REG - TAGHA_FIRST_PARAM_REG + 1)
#endif


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

/* Tagha Item
 * represents either a function or global variable.
 */
#define TAGHA_FLAG_NATIVE    1 /* if is a native C or JIT compiled function. */
#define TAGHA_FLAG_LINKED    2 /* ptr to native/jit function is linked and verified. */
typedef struct TaghaItem {
	union {
		uint8_t *stream;
		void *datum;
		FnTaghaNative *cfunc;
	} item;
	size_t bytes;
	uint8_t flags; // 0-bytecode based, 1-native based, 2-resolved
} STaghaItem;

#define EMPTY_TAGHA_ITEM    { {NULL}, 0, 0 }


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

#define EMPTY_TAGHA_ITEM_MAP    { NULL,NULL,0,0 }


/* Script/Module Structure.
 * Consists of:
 * A virtual machine context aka CPU.
 * An internal stack + heap controlled by a freelist allocator.
 * Dynamic symbol tables for functions and global variables.
 * An internal stack.
 * pointer to the raw script data.
 * and error code status.
 */
typedef struct TaghaModule {
	struct {
		union {
			union TaghaVal array[MaxRegisters];
			struct {
#				define Y(y) union TaghaVal y;
				TAGHA_REGISTER_FILE
#				undef Y
#				undef TAGHA_REGISTER_FILE
			} struc;
		} regfile;
		bool condflag : 1;
	} cpu;
	struct TaghaItemMap funcs, vars;
	struct HarbolMemPool heap;
	struct { union TaghaVal *start; size_t size; } stack;
	uint8_t *script;
	const uint8_t *start_seg, *end_seg;
	enum TaghaErrCode errcode : 8;
} STaghaModule;

#define EMPTY_TAGHA_MODULE    { { {{{0}}},false }, EMPTY_TAGHA_ITEM_MAP,EMPTY_TAGHA_ITEM_MAP, EMPTY_HARBOL_MEMPOOL, {NULL,0}, NULL,NULL,NULL, 0 }


TAGHA_EXPORT NO_NULL struct TaghaModule *tagha_module_new_from_file(const char filename[]);
TAGHA_EXPORT NO_NULL struct TaghaModule *tagha_module_new_from_buffer(uint8_t buffer[]);
TAGHA_EXPORT NO_NULL bool tagha_module_free(struct TaghaModule **moduleref);

TAGHA_EXPORT NO_NULL struct TaghaModule tagha_module_create_from_file(const char filename[]);
TAGHA_EXPORT NO_NULL struct TaghaModule tagha_module_create_from_buffer(uint8_t buffer[]);
TAGHA_EXPORT NO_NULL bool tagha_module_clear(struct TaghaModule *module);

TAGHA_EXPORT NO_NULL void tagha_module_print_vm_state(const struct TaghaModule *module);
TAGHA_EXPORT NO_NULL NONNULL_RET const char *tagha_module_get_error(const struct TaghaModule *module);
TAGHA_EXPORT NO_NULL bool tagha_module_register_natives(struct TaghaModule *module, const struct TaghaNative natives[]);
TAGHA_EXPORT NO_NULL bool tagha_module_register_ptr(struct TaghaModule *module, const char name[], void *ptr);

TAGHA_EXPORT NO_NULL void *tagha_module_get_var(struct TaghaModule *module, const char name[]);

TAGHA_EXPORT NEVER_NULL(1,2) int32_t tagha_module_call(struct TaghaModule *module, const char name[], size_t args, union TaghaVal params[], union TaghaVal *retval);
TAGHA_EXPORT NEVER_NULL(1) int32_t tagha_module_invoke(struct TaghaModule *module, int64_t func_index, size_t args, union TaghaVal params[], union TaghaVal *retval);
TAGHA_EXPORT NEVER_NULL(1) int32_t tagha_module_run(struct TaghaModule *module, size_t argc, union TaghaVal argv[]);
TAGHA_EXPORT NO_NULL void tagha_module_throw_error(struct TaghaModule *module, int32_t err);
TAGHA_EXPORT NO_NULL void tagha_module_jit_compile(struct TaghaModule *module, FnTaghaNative *jitfunc(const uint8_t*, size_t, void *), void *userdata);


#ifdef TAGHA_USE_FLOATS
#	define TAGHA_INSTR_SET \
		X(halt) \
		X(pushi) X(push) X(pop) \
		\
		X(loadglobal) X(loadaddr) X(loadfunc) \
		X(movi) X(mov) \
		X(ld1) X(ld2) X(ld4) X(ld8) \
		X(st1) X(st2) X(st4) X(st8) \
		\
		X(add) X(sub) X(mul) X(divi) X(mod) \
		\
		X(bit_and) X(bit_or) X(bit_xor) X(bit_not) X(shl) X(shr) \
		X(inc) X(dec) X(neg) \
		\
		X(ilt) X(ile) X(igt) X(ige) \
		X(ult) X(ule) X(ugt) X(uge) \
		X(cmp) X(neq) \
		\
		X(jmp) X(jz) X(jnz) \
		X(call) X(callr) X(ret) \
		X(nop) \
		\
		X(flt2dbl) X(dbl2flt) X(int2dbl) X(int2flt) \
		X(addf) X(subf) X(mulf) X(divf) \
		X(incf) X(decf) X(negf) \
		X(ltf) X(lef) X(gtf) X(gef) X(cmpf) X(neqf)
#else
#	define TAGHA_INSTR_SET \
		X(halt) \
		X(pushi) X(push) X(pop) \
		\
		X(loadglobal) X(loadaddr) X(loadfunc) \
		X(movi) X(mov) \
		X(ld1) X(ld2) X(ld4) X(ld8) \
		X(st1) X(st2) X(st4) X(st8) \
		\
		X(add) X(sub) X(mul) X(divi) X(mod) \
		\
		X(bit_and) X(bit_or) X(bit_xor) X(bit_not) X(shl) X(shr) \
		X(inc) X(dec) X(neg) \
		\
		X(ilt) X(ile) X(igt) X(ige) \
		X(ult) X(ule) X(ugt) X(uge) \
		X(cmp) X(neq) \
		\
		X(jmp) X(jz) X(jnz) \
		X(call) X(callr) X(ret) \
		X(nop)
#endif

#define X(x) x,
typedef enum TaghaInstrSet { TAGHA_INSTR_SET } ETaghaInstrSet;
#undef X

#ifdef __cplusplus
}
#endif
