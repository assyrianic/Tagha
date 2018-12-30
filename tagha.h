#pragma once

#ifdef __cplusplus
extern "C" {
#define restrict __restrict
#endif

#include <stdbool.h>
#include <stddef.h>
#include <inttypes.h>
#include "libharbol/harbol.h"

#define __TAGHA_FLOAT32_DEFINED // allow tagha to use 32-bit floats
#define __TAGHA_FLOAT64_DEFINED // allow tagha to use 64-bit floats

#if defined(__TAGHA_FLOAT32_DEFINED) || defined(__TAGHA_FLOAT64_DEFINED)
	#ifndef TAGHA_FLOATING_POINT_OPS
		#define TAGHA_FLOATING_POINT_OPS
	#endif
#endif

#ifdef TAGHA_DLL
	#ifndef TAGHA_LIB
		#define TAGHA_EXPORT __declspec(dllimport)
	#else
		#define TAGHA_EXPORT __declspec(dllexport)
	#endif
#else
	#define TAGHA_EXPORT 
#endif


typedef union TaghaVal {
	bool Bool, *PtrBool;
	int8_t Int8, *PtrInt8;
	int16_t Int16, *PtrInt16;
	int32_t Int32, *PtrInt32;
	int64_t Int64, *PtrInt64;
	
	uint8_t UInt8, *PtrUInt8;
	uint16_t UInt16, *PtrUInt16;
	uint32_t UInt32, *PtrUInt32;
	uint64_t UInt64, *PtrUInt64;
	size_t SizeInt, *PtrSizeInt;
	uintptr_t UIntPtr;
	intptr_t IntPtr;
 #ifdef __TAGHA_FLOAT32_DEFINED
	float Float, *PtrFloat;
 #endif
 #ifdef __TAGHA_FLOAT64_DEFINED
	double Double, *PtrDouble;
 #endif
	void *Ptr;
	const char *PtrCStr;
	union TaghaVal *PtrSelf;
} TaghaVal;

typedef union TaghaPtr {
	uint8_t *restrict PtrUInt8;
	uint16_t *restrict PtrUInt16;
	uint32_t *restrict PtrUInt32;
	uint64_t *restrict PtrUInt64;
	
	int8_t *restrict PtrInt8;
	int16_t *restrict PtrInt16;
	int32_t *restrict PtrInt32;
	int64_t *restrict PtrInt64;
	size_t *restrict PtrSizeInt;
 #ifdef __TAGHA_FLOAT32_DEFINED
	float *restrict PtrFloat;
 #endif
 #ifdef __TAGHA_FLOAT64_DEFINED
	double *restrict PtrDouble;
 #endif
	const char *restrict PtrCStr;
	
	union TaghaVal *restrict PtrVal;
	union TaghaPtr *restrict PtrSelf;
	void *restrict Ptr;
} TaghaPtr;


/* Script File/Binary Format Structure
 * ------------------------------ start of header ------------------------------
 * 2 bytes: magic verifier ==> 0xC0DE
 * 4 bytes: stack size, stack size needed for the code
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
 *     else: 8 bytes: native address (0 at first, will be filled in during runtime)
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
 */

struct TaghaModule;
typedef void TaghaNativeFunc(struct TaghaModule *ctxt, union TaghaVal *ret, size_t args, union TaghaVal params[]);

typedef struct TaghaNative {
	const char *Name;
	TaghaNativeFunc *NativeCFunc;
} TaghaNative;


#define TAGHA_REGISTER_FILE \
	Y(regAlaf) Y(regBeth) Y(regGamal) Y(regDalath) \
	Y(regHeh) Y(regWaw) Y(regZain) Y(regHeth) Y(regTeth) Y(regYodh) Y(regKaf) \
	Y(regLamadh) Y(regMeem) Y(regNoon) Y(regSemkath) Y(reg_Eh) \
	Y(regPeh) Y(regSadhe) Y(regQof) Y(regReesh) Y(regSheen) Y(regTaw) \
	/* Syriac alphabet makes great register names! */ \
	Y(regStk) Y(regBase) Y(regInstr)

#define Y(y) y,
typedef enum TaghaRegID { TAGHA_REGISTER_FILE MaxRegisters } TaghaRegID;
#undef Y

typedef enum TaghaErrCode {
	ErrInstrBounds = -1,
	ErrNone=0,
	ErrBadPtr,
	ErrMissingFunc, ErrMissingNative,
	ErrInvalidScript,
	ErrStackSize, ErrStackOver,
} TaghaErrCode;

/* Tagha Item
 * represents either a function or global variable.
 */
#define TAGHA_FLAG_NATIVE	1
#define TAGHA_FLAG_LINKED	2
typedef struct TaghaItem {
	union {
		uint8_t *Data;
		void *RawPtr;
		TaghaNativeFunc *NativeFunc;
	};
	size_t Bytes;
	uint8_t Flags; // 0-bytecode based, 1-native based, 2-resolved
} TaghaItem;


/* Script/Module Structure.
 * Consists of:
 * A virtual machine context
 * An internal stack
 * Dynamic symbol table for functions and global variables
 * An internal memory allocator
 * and flags
 */
typedef struct TaghaModule {
	struct /* TaghaCPU */ { // 208 bytes on 64-bit systems
		union {
			struct {
				#define Y(y) union TaghaVal y;
				TAGHA_REGISTER_FILE
				#undef Y
				#undef TAGHA_REGISTER_FILE
			};
			union TaghaVal Regs[MaxRegisters];
		};
		bool CondFlag : 1;
	}; /* TaghaCPU */
	struct HarbolLinkMap // 48 bytes on 64-bit systems
		FuncMap,
		VarMap
	;
	struct HarbolMemoryPool Heap; // 40 bytes on 64-bit systems
	struct {
		union TaghaVal *Stack;
		size_t StackSize;
	};
	enum TaghaErrCode Error : 8;
	bool SafeMode : 1;
} TaghaModule; // 416 bytes


TAGHA_EXPORT struct TaghaModule *tagha_module_new_from_file(const char filename[]);
TAGHA_EXPORT struct TaghaModule *tagha_module_new_from_buffer(uint8_t buffer[]);
TAGHA_EXPORT bool tagha_module_free(struct TaghaModule **modref);

TAGHA_EXPORT void tagha_module_print_vm_state(const struct TaghaModule *module);
TAGHA_EXPORT const char *tagha_module_get_error(const struct TaghaModule *module);
TAGHA_EXPORT bool tagha_module_register_natives(struct TaghaModule *module, const struct TaghaNative natives[]);
TAGHA_EXPORT bool tagha_module_register_ptr(struct TaghaModule *module, const char varname[], void *ptr);

TAGHA_EXPORT void *tagha_module_get_globalvar_by_name(struct TaghaModule *module, const char varname[]);

TAGHA_EXPORT int32_t tagha_module_call(struct TaghaModule *module, const char funcname[], size_t args, union TaghaVal params[], union TaghaVal *return_val);
TAGHA_EXPORT int32_t tagha_module_run(struct TaghaModule *module, int32_t iargc, char *strargv[]);
TAGHA_EXPORT void tagha_module_throw_error(struct TaghaModule *module, int32_t err);
TAGHA_EXPORT void tagha_module_force_safemode(struct TaghaModule *module);


/*
typedef struct TaghaSystem {
	struct HarbolLinkMap
		ModuleMap,
		NativeMap
	;
} TaghaSystem;

TAGHA_EXPORT struct TaghaSystem *tagha_system_new(void);
TAGHA_EXPORT bool tagha_system_free(struct TaghaSystem **sysref);

TAGHA_EXPORT void tagha_system_init(struct TaghaSystem *sys);
TAGHA_EXPORT void tagha_system_del(struct TaghaSystem *sys);

TAGHA_EXPORT bool tagha_system_add_module_ptr(struct TaghaSystem *sys, const char name[], struct TaghaModule *module);
TAGHA_EXPORT bool tagha_system_add_module_file(struct TaghaSystem *sys, const char filename[]);
TAGHA_EXPORT bool tagha_system_del_module(struct TaghaSystem *sys, const char name[]);
TAGHA_EXPORT bool tagha_system_add_natives(struct TaghaSystem *sys, const struct TaghaNative natives[]);
*/

#ifdef TAGHA_FLOATING_POINT_OPS
	#define TAGHA_INSTR_SET	\
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
	#define TAGHA_INSTR_SET	\
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
//X(syscall) X(syscallr)

#define X(x) x,
typedef enum TaghaInstrSet { TAGHA_INSTR_SET } TaghaInstrSet;
#undef X

#ifdef __cplusplus
}
#endif
