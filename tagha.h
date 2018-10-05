#pragma once

#ifdef __cplusplus
extern "C" {
#define restrict __restrict
#endif

#include <stdbool.h>
#include <inttypes.h>


#define FLOATING_POINT_OPS 1

union TaghaVal {
	bool Bool, BoolArray[8], *BoolPtr, (*BoolFunc)(), *(*BoolPtrFunc)();
	int8_t Int8, Int8Array[8], *Int8Ptr, (*Int8Func)(), *(*Int8PtrFunc)();
	int16_t Int16, Int16Array[4], *Int16Ptr, (*Int16Func)(), *(*Int16PtrFunc)();
	int32_t Int32, Int32Array[2], *Int32Ptr, (*Int32Func)(), *(*Int32PtrFunc)();
	int64_t Int64, *Int64Ptr, (*Int64Func)(), *(*Int64PtrFunc)();
	
	uint8_t UInt8, UInt8Array[8], *UInt8Ptr, (*UInt8Func)(), *(*UInt8PtrFunc)();
	uint16_t UInt16, UInt16Array[4], *UInt16Ptr, (*UInt16Func)(), *(*UInt16PtrFunc)();
	uint32_t UInt32, UInt32Array[2], *UInt32Ptr, (*UInt32Func)(), *(*UInt32PtrFunc)();
	uint64_t UInt64, *UInt64Ptr, (*UInt64Func)(), *(*UInt64PtrFunc)();
	size_t SizeInt, *SizeIntPtr;
 #ifdef FLOATING_POINT_OPS
	float Float, FloatArray[2], *FloatPtr, (*FloatFunc)(), *(*FloatPtrFunc)();
	double Double, *DoublePtr, (*DoubleFunc)(), *(*DoublePtrFunc)();
 #endif
	void *Ptr, **PtrPtr, (*VoidFunc)(), *(*VoidPtrFunc)();
	union TaghaVal *SelfPtr, (*SelfFunc)(), *(*SelfPtrFunc)();
};

union TaghaPtr {
	uint8_t *restrict UInt8Ptr;
	uint16_t *restrict UInt16Ptr;
	uint32_t *restrict UInt32Ptr;
	uint64_t *restrict UInt64Ptr;
	
	int8_t *restrict Int8Ptr;
	int16_t *restrict Int16Ptr;
	int32_t *restrict Int32Ptr;
	int64_t *restrict Int64Ptr;
 #ifdef FLOATING_POINT_OPS
	float *restrict FloatPtr;
	double *restrict DoublePtr;
 #endif
	const char *restrict CStrPtr;
	
	union TaghaVal *restrict ValPtr;
	union TaghaPtr *restrict SelfPtr;
	void *restrict Ptr;
};


/* For Colored Debugging Printing! */
#define KNRM	"\x1B[0m"	/* Normal */
#define KRED	"\x1B[31m"
#define KGRN	"\x1B[32m"
#define KYEL	"\x1B[33m"
#define KBLU	"\x1B[34m"
#define KMAG	"\x1B[35m"
#define KCYN	"\x1B[36m"
#define KWHT	"\x1B[37m"
#define RESET	"\033[0m"	/* Reset obviously */

#define TAGHA_VERSION_STR		"0.0.1a"


/* Script File/Binary Format Structure (Jun 23, 2018)
 * ------------------------------ start of header ------------------------------
 * 2 bytes: magic verifier ==> 0xC0DE
 * 4 bytes: stack size, stack size needed for the code
 * 4 bytes: func table offset (from this offset)
 * 4 bytes: global var table offset (from this offset)
 * 1 byte: flags
 * ------------------------------ end of header ------------------------------
 * .functions table
 * 4 bytes: amount of funcs
 * n bytes: func table
 *		1 byte: 0 if bytecode func, 1 if it's a native, other flags.
 * 		4 bytes: string size + '\0' of func string
 *		4 bytes: instr len, 8 if native.
 * 		n bytes: func string
 * 		if bytecode func: n bytes - instructions
 *		else: 8 bytes: native address (0 at first, will be filled in during runtime)
 * 
 * .globalvars table
 * 4 bytes: amount of global vars
 * n bytes: global vars table
 * 		1 byte: flags
 * 		4 bytes: string size + '\0' of global var string
 *		4 bytes: byte size, 8 if ptr.
 * 		n bytes: global var string
 * 		if bytecode var: n bytes: data. All 0 if not initialized in script code.
 *		else: 8 bytes: var address (0 at first, filled in during runtime)
 */

#pragma pack(push, 1)
struct TaghaHeader {
	uint16_t Magic;
	uint32_t StackSize;
	uint32_t FuncTblOffs;
	uint32_t VarTblOffs;
	uint8_t Flags;
};
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
typedef union TaghaVal TaghaUserFunc(struct Tagha *vm, uint8_t opcode, size_t args, union TaghaVal params[]);

struct NativeInfo {
	const char *Name;
	TaghaNative *NativeCFunc;
};


#define REGISTER_FILE \
	Y(regAlaf) Y(regBeth) Y(regGamal) Y(regDalath) \
	Y(regHeh) Y(regWaw) Y(regZain) Y(regHeth) Y(regTeth) Y(regYodh) Y(regKaf) \
	Y(regLamadh) Y(regMeem) Y(regNoon) Y(regSemkath) Y(reg_Eh) \
	Y(regPeh) Y(regSadhe) Y(regQof) Y(regReesh) Y(regSheen) Y(regTaw) \
	/* Syriac alphabet makes great register names! */ \
	Y(regStk) Y(regBase) Y(regInstr)

#define Y(y) y,
enum RegID { REGISTER_FILE regsize };
#undef Y

enum TaghaErrCode {
	ErrInstrBounds = -1,
	ErrNone=0,
	ErrBadPtr,
	ErrMissingFunc, ErrMissingNative,
	ErrInvalidScript,
	ErrStackSize
};

// Script structure.
struct Tagha {
	union {
		struct {
			#define Y(y) union TaghaVal y;
			REGISTER_FILE
			#undef Y
			#undef REGISTER_FILE
		};
		union TaghaVal Regs[regsize];
	};
	struct TaghaHeader *Header;
	enum TaghaErrCode Error;
	bool CondFlag : 1; /* conditional flag for conditional jumps! */
};


struct Tagha *Tagha_New(void *);
struct Tagha *Tagha_NewNatives(void *, const struct NativeInfo []);
void Tagha_Free(struct Tagha **);

void Tagha_Init(struct Tagha *, void *);
void Tagha_InitNatives(struct Tagha *, void *, const struct NativeInfo []);
void Tagha_Del(struct Tagha *);

void Tagha_PrintVMState(const struct Tagha *);
const char *Tagha_GetError(const struct Tagha *);

bool Tagha_RegisterNatives(struct Tagha *, const struct NativeInfo []);
void *Tagha_GetGlobalVarByName(struct Tagha *, const char *);

int32_t Tagha_Exec(struct Tagha *)
#if defined(__clang__) || defined(__GNUC__) || defined(__GNUG__)
	__attribute__ ((hot)) // hot attribute for further optimizations.
#endif
;

int32_t Tagha_CallFunc(struct Tagha *, const char *, size_t, union TaghaVal []);
union TaghaVal Tagha_GetReturnValue(const struct Tagha *);
int32_t Tagha_RunScript(struct Tagha *, int32_t, char *[]);


enum AddrMode {
	Immediate	= 1, /* interpret as immediate/constant value */
	Register	= 2, /* interpret as register id */
	RegIndirect	= 4, /* interpret register id's contents as a memory address. */
	UseReg	= 8, /* UseReg. */
	Byte		= 16, /* use data as (u)int8_t& */
	TwoBytes	= 32, /* use data as (u)int16_t& */
	FourBytes	= 64, /* use data as (u)int32_t& */
	EightBytes	= 128, /* use data as (u)int64_t& */
};


#ifdef FLOATING_POINT_OPS
	#define INSTR_SET	\
		X(halt) \
		X(push) X(pop) \
		\
		X(lea) X(mov) \
		\
		X(add) X(sub) X(mul) X(divi) X(mod) \
		\
		X(andb) X(orb) X(xorb) X(notb) X(shl) X(shr) \
		X(inc) X(dec) X(neg) \
		\
		X(ilt) X(igt) X(ult) X(ugt) X(cmp) X(neq) \
		\
		X(jmp) X(jz) X(jnz) \
		X(call) X(syscall) X(ret) \
		\
		X(flt2dbl) X(dbl2flt) X(int2dbl) X(int2flt) \
		X(addf) X(subf) X(mulf) X(divf) \
		X(incf) X(decf) X(negf) \
		X(ltf) X(gtf) X(cmpf) X(neqf) \
		X(nop)
#else
	#define INSTR_SET	\
		X(halt) \
		X(push) X(pop) \
		\
		X(lea) X(mov) \
		\
		X(add) X(sub) X(mul) X(divi) X(mod) \
		\
		X(andb) X(orb) X(xorb) X(notb) X(shl) X(shr) \
		X(inc) X(dec) X(neg) \
		\
		X(ilt) X(igt) X(ult) X(ugt) X(cmp) X(neq) \
		\
		X(jmp) X(jz) X(jnz) \
		X(call) X(syscall) X(ret) \
		\
		X(nop)
#endif

#define X(x) x,
enum InstrSet { INSTR_SET };
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
};

#endif

