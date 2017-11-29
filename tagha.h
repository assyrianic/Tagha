#ifndef TAGHA_H_INCLUDED
	#define TAGHA_H_INCLUDED


#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <stdbool.h>
#include <iso646.h>
#include "ds.h"

/* For Colored Debugging Printing! */
#define KNRM	"\x1B[0m"	// Normal
#define KRED	"\x1B[31m"
#define KGRN	"\x1B[32m"
#define KYEL	"\x1B[33m"
#define KBLU	"\x1B[34m"
#define KMAG	"\x1B[35m"
#define KCYN	"\x1B[36m"
#define KWHT	"\x1B[37m"
#define RESET	"\033[0m"	// Reset obviously


struct TaghaScript;
struct TaghaVM;
struct NativeInfo;
struct FuncTable;
struct DataTable;
union CValue;

typedef struct TaghaScript		Script_t, Applet_t, Plugin_t;
typedef struct TaghaVM			TaghaVM_t, TVM_t;
typedef struct DataTable		DataTable_t, GlobalTable_t;
typedef struct FuncTable		FuncTable_t, ProcTable_t;
typedef struct NativeInfo		NativeInfo_t, NativeData_t;
typedef union CValue			Param_t, Arg_t, Val_t;



union CValue {
	bool Bool, *BoolPtr;
	int8_t Char, *CharPtr;
	int16_t Short, *ShortPtr;
	int32_t Int32, *Int32Ptr;
	int64_t Int64, *Int64Ptr;
	
	uint8_t UChar, *UCharPtr;
	uint16_t UShort, *UShortPtr;
	uint32_t UInt32, *UInt32Ptr;
	uint64_t UInt64, *UInt64Ptr;
	
	float Float, *FloatPtr;
	double Double, *DoublePtr;
	
	void *Ptr, **PtrPtr;
	const char *String, **StringPtr;
	union CValue *SelfPtr;
};

//	API for scripts to call C/C++ host functions.
typedef		void (*fnNative_t)(struct TaghaScript *, union CValue [], union CValue *, const uint32_t, struct TaghaVM *);

struct NativeInfo {
	const char	*strName;	// use as string literals
	fnNative_t	pFunc;
};


/* addressing modes
 *	immediate - simple constant value.
 *	register - register holds the exact data.
 *	register indirect - register holds memory address and dereferenced. Can be used as displacement as well.
 *	direct - a simple memory address. Useful for static data like global vars.
*/
enum AddrMode {
	Immediate	= 1,
	Register	= 2,
	RegIndirect	= 4,
	Direct		= 8,
	Byte		= 16,
	TwoBytes	= 32,
	FourBytes	= 64,
	EightBytes	= 128,
};

// Register ID list
enum RegID {
	ras=0,		// general purpose, accumulator - all return results go here.
	rbs,rcs,	// general purpose
	rds,res,	// general purpose
	rfs,rgs,rhs,// general purpose, floating point regs, rfs is used as float accumulator
	ris,rjs,rks,// general purpose
	
	// do not modify after this. Add more registers, if u need, above.
	rsp,rbp,	// stack ptrs, do not touch
	rip,		// instr ptr, do not touch as well.
	regsize		// for lazily updating id list
};


/* Script File Structure.
 * magic verifier
 * Stack Vector
 * Native Table Vector
 * Func Table Dictionary
 * Data Table Dictionary
 */

struct FuncTable {
	uint32_t	m_uiParams;
	uint32_t	m_uiEntry;	// TODO: make this into uint64?
};

struct DataTable {
	uint32_t	m_uiBytes;
	uint32_t	m_uiOffset;	// TODO: make this into uint64?
};

struct TaghaScript {
	char m_strName[64];	// script's name
	uint8_t
		*m_pMemory,	// stack and data stream. Used for stack and data segment
		*m_pText	// instruction stream.
	;
	union CValue m_Regs[regsize];
	char	**m_pstrNatives;	// natives table as stored strings.
	struct hashmap
		*m_pmapFuncs,	// stores the functions compiled to script.
		*m_pmapGlobals	// stores global vars like string literals or variables.
	;
	uint32_t
		m_uiMemsize,	// size of m_pMemory
		m_uiInstrSize,	// size of m_pText
		m_uiMaxInstrs,	// max amount of instrs a script can execute.
		m_uiNatives,	// amount of natives the script uses.
		m_uiFuncs,	// how many functions the script has.
		m_uiGlobals	// how many globals variables the script has.
	;
	bool	m_bSafeMode : 1;	// does the script want bounds checking?
	bool	m_bDebugMode : 1;	// print debug info.
	uint8_t	m_ucZeroFlag : 1;	// conditional zero flag.
};


struct TaghaVM {
	// TODO: Replace script vector with dictionary so we can access scripts by name!
	struct TaghaScript	*m_pScript;
	struct hashmap		*m_pmapNatives;	// hashmap that stores the native's script name and the C/C++ function pointer bound to it.
};


// taghavm_api.c
void		Tagha_init(struct TaghaVM *vm);
void		Tagha_load_script_by_name(struct TaghaVM *vm, char *filename);
bool		Tagha_register_natives(struct TaghaVM *vm, struct NativeInfo arrNatives[]);
void		Tagha_free(struct TaghaVM *vm);
int32_t		Tagha_call_script_func(struct TaghaVM *vm, const char *strFunc);
struct TaghaScript	*Tagha_get_script(const struct TaghaVM *vm);
void		Tagha_set_script(struct TaghaVM *vm, struct TaghaScript *script);
void		gfree(void **ptr);


// tagha_exec.c
int32_t		Tagha_exec(struct TaghaVM *vm, uint8_t *oldbp);


// tagha_libc.c
void		Tagha_load_libc_natives(struct TaghaVM *vm);
void		Tagha_load_self_natives(struct TaghaVM *vm);


// taghascript_api.c
Script_t	*TaghaScript_from_file(const char *filename);
void		TaghaScript_debug_print_ptrs(const struct TaghaScript *script);
void		TaghaScript_debug_print_memory(const struct TaghaScript *script);
void		TaghaScript_debug_print_instrs(const struct TaghaScript *script);
void		TaghaScript_reset(struct TaghaScript *script);
void		TaghaScript_free(struct TaghaScript *script);

void		*TaghaScript_get_global_by_name(struct TaghaScript *script, const char *strGlobalName);
bool		TaghaScript_bind_global_ptr(struct TaghaScript *script, const char *strGlobalName, void *pVar);
void		TaghaScript_push_value(struct TaghaScript *script, const union CValue value);
union CValue	TaghaScript_pop_value(struct TaghaScript *script);

uint32_t	TaghaScript_memsize(const struct TaghaScript *script);
uint32_t	TaghaScript_instrsize(const struct TaghaScript *script);
uint32_t	TaghaScript_maxinstrs(const struct TaghaScript *script);
uint32_t	TaghaScript_nativecount(const struct TaghaScript *script);
uint32_t	TaghaScript_funcs(const struct TaghaScript *script);
uint32_t	TaghaScript_globals(const struct TaghaScript *script);
bool		TaghaScript_safemode_active(const struct TaghaScript *script);
bool		TaghaScript_debug_active(const struct TaghaScript *script);

void		TaghaScript_PrintErr(struct TaghaScript *script, const char *funcname, const char *err, ...);

/*
*	r = register is first operand
*	m = memory address is first operand
*/
#define INSTR_SET \
	X(halt) \
	/* single operand opcodes */ \
	/* stack ops */ \
	X(push) X(pop) \
	/* unary arithmetic and bitwise ops */ \
	X(neg) X(inc) X(dec) X(bnot) \
	/* conversion ops */ \
	X(long2int) X(long2short) X(long2byte) \
	X(int2long) X(short2long) X(byte2long) \
	/* jump ops */ \
	X(jmp) X(jz) X(jnz) \
	\
	/* subroutine ops */ \
	X(call) X(ret) X(callnat) \
	\
	/* two operand opcodes */ \
	X(movr) X(movm) X(lea) \
	/* signed and unsigned integer arithmetic ops */ \
	X(addr) X(addm) X(uaddr) X(uaddm) \
	X(subr) X(subm) X(usubr) X(usubm) \
	X(mulr) X(mulm) X(umulr) X(umulm) \
	X(divr) X(divm) X(udivr) X(udivm) \
	X(modr) X(modm) X(umodr) X(umodm) \
	/* bitwise ops */ \
	X(shrr) X(shrm) X(shlr) X(shlm) \
	X(andr) X(andm) X(orr) X(orm) X(xorr) X(xorm) \
	/* comparison ops */ \
	X(ltr) X(ltm) X(ultr) X(ultm) \
	X(gtr) X(gtm) X(ugtr) X(ugtm) \
	X(cmpr) X(cmpm) X(ucmpr) X(ucmpm) \
	X(neqr) X(neqm) X(uneqr) X(uneqm) \
	X(reset) \
	\
	/* floating point opcodes */ \
	X(int2float) X(int2dbl) X(float2dbl) X(dbl2float) \
	X(faddr) X(faddm) X(fsubr) X(fsubm) X(fmulr) X(fmulm) X(fdivr) X(fdivm) \
	X(fneg) X(fltr) X(fltm) X(fgtr) X(fgtm) X(fcmpr) X(fcmpm) X(fneqr) X(fneqm) \
	/* misc opcodes */ \
	X(nop) \

#define X(x)	x,
enum InstrSet{ INSTR_SET };
#undef X

#ifdef __cplusplus
}
#endif

#endif	// TAGHA_H_INCLUDED
