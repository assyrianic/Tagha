#pragma once

#ifdef __cplusplus
extern "C" {
#define restrict __restrict
#endif

#include <stdbool.h>
#include <stddef.h>
#include <inttypes.h>

#define __TAGHA_FLOAT32_DEFINED // allow tagha to use 32-bit floats
#define __TAGHA_FLOAT64_DEFINED // allow tagha to use 64-bit floats

#if defined(__TAGHA_FLOAT32_DEFINED) || defined(__TAGHA_FLOAT64_DEFINED)
#	ifndef FLOATING_POINT_OPS
#		define FLOATING_POINT_OPS
#	endif
#endif

#if defined(_WIN32) || defined(_WIN64)
#	ifndef OS_WINDOWS
#		define OS_WINDOWS 1
#	endif
#endif

#ifdef TAGHA_DLL
#	ifndef TAGHA_LIB
#		define TAGHA_EXPORT __declspec(dllimport)
#	else
#		define TAGHA_EXPORT __declspec(dllexport)
#	endif
#else
#	define TAGHA_EXPORT 
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



/* Script File/Binary Format Structure (Jun 23, 2018)
 * ------------------------------ start of header ------------------------------
 * 2 bytes: magic verifier ==> 0xC0DE
 * 4 bytes: stack size, stack size needed for the code
 * 4 bytes: func table offset (from header)
 * 4 bytes: global var table offset (from header)
 * 4 bytes: stack base offset (from header)
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
 * 
 * n bytes : stack base
 */

#pragma pack(push, 1)
typedef struct TaghaHeader {
	uint16_t Magic;
	uint32_t StackSize;
	uint32_t FuncTblOffs;
	uint32_t VarTblOffs;
	uint32_t StackOffs;
	uint8_t Flags;
} TaghaHeader;
#pragma pack(pop)


/*
 Things to consider concerning Native functions:
	- Parameters can be of any size in bytes, not just 8 bytes.
	- Return values must also be of any size in bytes.
		-- usually optimized into a hidden pointer argument.
	- the native function must be able to take an n-amount of Parameters.
	Possible Solutions:
		Pass ALL values by reference or...
		Set a standard that all arguments and return values given fit within 8 bytes or less.
*/

/* Tagha's Calling Convention:
 * Params are passed from right to left. (last argument is pushed first, first arg is last.)
 * The first 8 parameters are passed registers Semkath to Taw, remaining params are pushed to the stack.
 * If 8 params, Taw will hold the 8th param, Semkath the first.
 * 
 * Return values must be 8 or less bytes in size in register 'Alaf'.
 * For Natives, structs must always be passed by pointer.
 *
 * Since 'Alaf' is the return value register and Semkath to Taw are for functions, they need preservation, all other registers are volatile.
 */
struct Tagha;
typedef void TaghaNative(struct Tagha *vm, union TaghaVal *ret, size_t args, union TaghaVal params[]);
typedef union TaghaVal TaghaSysCall(struct Tagha *vm, int8_t callid, size_t args, union TaghaVal params[]);

typedef struct NativeInfo {
	const char *Name;
	TaghaNative *NativeCFunc;
} NativeInfo;


#define REGISTER_FILE \
	Y(regAlaf) Y(regBeth) Y(regGamal) Y(regDalath) \
	Y(regHeh) Y(regWaw) Y(regZain) Y(regHeth) Y(regTeth) Y(regYodh) Y(regKaf) \
	Y(regLamadh) Y(regMeem) Y(regNoon) Y(regSemkath) Y(reg_Eh) \
	Y(regPeh) Y(regSadhe) Y(regQof) Y(regReesh) Y(regSheen) Y(regTaw) \
	/* Syriac alphabet makes great register names! */ \
	Y(regStk) Y(regBase) Y(regInstr)

#define Y(y) y,
typedef enum RegID { REGISTER_FILE regsize } RegID;
#undef Y

typedef enum TaghaErrCode {
	ErrInstrBounds = -1,
	ErrNone=0,
	ErrBadPtr,
	ErrMissingFunc, ErrMissingNative,
	ErrInvalidScript,
	ErrStackSize, ErrStackOver,
} TaghaErrCode;


// Script structure.
typedef struct Tagha {
	union {
		struct {
			#define Y(y) union TaghaVal y;
			REGISTER_FILE
			#undef Y
			#undef REGISTER_FILE
		};
		union TaghaVal Regs[regsize];
	};
	uint8_t *Header, *DataBase, *Footer;
	enum TaghaErrCode Error;
	bool
		SafeMode : 1,
		CondFlag : 1 /* conditional flag for conditional jumps! */
	;
} Tagha;


TAGHA_EXPORT struct Tagha *Tagha_New(void *);
TAGHA_EXPORT struct Tagha *Tagha_NewNatives(void *, const struct NativeInfo []);
TAGHA_EXPORT void Tagha_Free(struct Tagha **);

TAGHA_EXPORT void Tagha_Init(struct Tagha *, void *);
TAGHA_EXPORT void Tagha_InitNatives(struct Tagha *, void *, const struct NativeInfo []);
TAGHA_EXPORT void Tagha_Del(struct Tagha *);

TAGHA_EXPORT void Tagha_PrintVMState(const struct Tagha *);
TAGHA_EXPORT const char *Tagha_GetError(const struct Tagha *);

TAGHA_EXPORT bool Tagha_RegisterNatives(struct Tagha *, const struct NativeInfo []);
TAGHA_EXPORT void *Tagha_GetGlobalVarByName(struct Tagha *, const char *);
TAGHA_EXPORT void *Tagha_GetRawScriptPtr(const struct Tagha *);

TAGHA_EXPORT int32_t Tagha_Exec(struct Tagha *)
#if defined(__clang__) || defined(__GNUC__) || defined(__GNUG__)
	__attribute__ ((hot)) // hot attribute for further optimizations.
#endif
;

TAGHA_EXPORT int32_t Tagha_CallFunc(struct Tagha *, const char *, size_t, union TaghaVal []);
TAGHA_EXPORT union TaghaVal Tagha_GetReturnValue(const struct Tagha *);
TAGHA_EXPORT int32_t Tagha_RunScript(struct Tagha *, int32_t, char *[]);
TAGHA_EXPORT void Tagha_ThrowError(struct Tagha *, int32_t);


#ifdef FLOATING_POINT_OPS
	#define INSTR_SET	\
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
		X(call) X(callr) X(syscall) X(syscallr) X(ret) \
		X(nop) \
		\
		X(flt2dbl) X(dbl2flt) X(int2dbl) X(int2flt) \
		X(addf) X(subf) X(mulf) X(divf) \
		X(incf) X(decf) X(negf) \
		X(ltf) X(lef) X(gtf) X(gef) X(cmpf) X(neqf)
#else
	#define INSTR_SET	\
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
		X(call) X(callr) X(syscall) X(syscallr) X(ret) \
		X(nop)
#endif

#define X(x) x,
typedef enum InstrSet { INSTR_SET } InstrSet;
#undef X

#ifdef __cplusplus
}

class CTagha;
typedef void TaghaNative_cpp(class CTagha *, union TaghaVal *, size_t, union TaghaVal []);

struct CNativeInfo {
	const char *Name;
	TaghaNative_cpp *NativeFunc;
};

class CTagha : public Tagha {
 public:
	CTagha(void *);
	CTagha(void *, const struct CNativeInfo []);
	~CTagha();
	
	bool RegisterNatives(const struct CNativeInfo []);
	void *GetGlobalVarByName(const char *);
	int32_t CallFunc(const char *, size_t, union TaghaVal []);
	union TaghaVal GetReturnValue();
	int32_t RunScript(int32_t, char *[]);
	const char *GetError();
	void PrintVMState();
	void *GetRawScriptPtr();
	void ThrowError(int32_t);
};
#endif
