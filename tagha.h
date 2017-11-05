
#ifndef TAGHA_H_INCLUDED
#define TAGHA_H_INCLUDED


#ifdef __cplusplus
extern "C" {
#endif

#include "ds.h"
#include <inttypes.h>
#include <iso646.h>


//#define typename(x) _Generic((x),        /* Get the name of a type */           \
        _Bool: "_Bool",                    unsigned char: "unsigned char",        \
        char: "char",                      short: "short",                        \
        unsigned short: "unsigned short",  int: "int",                            \
        unsigned int: "unsigned int",      unsigned long long: "unsigned int64",  \
        long long: "long long", \
        float: "float",                    double: "double",                      \
        _Bool *: "_Bool *",                unsigned char *:"uint8_t *",             \
        char *: "char *",                  short *: "short *",                    \
        unsigned short *: "unsigned short *",  int *: "int *",                    \
        unsigned int *: "unsigned int *",      unsigned long long *: "unsigned long2 *",  \
        long long *: "long long *", \
        float *: "float *",                    double *: "double *",              \
        default: "other")

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

#define TAGHA_VERSION_STR		"0.0.15a"


struct TaghaScript;
struct TaghaVM;
struct NativeInfo;
struct FuncTable;
struct DataTable;
union Param;

typedef struct TaghaScript		Script_t, Applet_t, Plugin_t;
typedef struct TaghaVM			TaghaVM_t, TVM_t;
typedef struct DataTable		DataTable_t, GlobalTable_t;
typedef struct FuncTable		FuncTable_t, ProcTable_t;
typedef struct NativeInfo		NativeInfo_t, NativeData_t;
typedef union Param				Param_t, Arg_t, Val_t;

//	API to call C/C++ functions from scripts.
union Param {
	bool Bool;
	int8_t Char;
	int16_t Short;
	int32_t Int32;
	int64_t Int64;
	
	uint8_t UChar;
	uint16_t UShort;
	uint32_t UInt32;
	uint64_t UInt64;
	
	float Float;
	double Double;
	void *Pointer;
	const char *String;
};
typedef		void (*fnNative_t)(struct TaghaScript *, union Param [], union Param **, const uint32_t, struct TaghaVM *);

struct NativeInfo {
	const char	*strName;	// use as string literals
	fnNative_t	pFunc;
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
	uint8_t
		*m_pMemory,	// stack and data stream. Used for stack and data segment
		*m_pText,	// instruction stream.
		*m_pIP,		// instruction ptr.
		*m_pSP,		// stack ptr.
		*m_pBP		// base ptr / stack frame ptr.
	;
	char	**m_pstrNatives;	// natives table as stored strings.
	Map_t
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
};

struct TaghaVM {
	// TODO: Replace script vector with dictionary so we can access scripts by name!
	struct TaghaScript	*m_pScript;
	Map_t	*m_pmapNatives;	// hashmap that stores the native's script name and the C/C++ function pointer bound to it.
};

// taghavm_api.c
void		Tagha_init(struct TaghaVM *vm);
void		Tagha_load_script_by_name(struct TaghaVM *vm, char *filename);
bool		Tagha_register_natives(struct TaghaVM *vm, struct NativeInfo arrNatives[]);
void		Tagha_free(struct TaghaVM *vm);
void		Tagha_call_script_func(struct TaghaVM *vm, const char *strFunc);
Script_t	*Tagha_get_script(const struct TaghaVM *vm);
void		Tagha_set_script(struct TaghaVM *vm, struct TaghaScript *script);
void		gfree(void **ptr);


// tagha_exec.c
void		Tagha_exec(struct TaghaVM *vm, uint8_t *oldbp);


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
void		TaghaScript_push_value(struct TaghaScript *script, const union Param value);
union Param TaghaScript_pop_value(struct TaghaScript *script);

uint32_t	TaghaScript_stacksize(const struct TaghaScript *script);
uint32_t	TaghaScript_instrsize(const struct TaghaScript *script);
uint32_t	TaghaScript_maxinstrs(const struct TaghaScript *script);
uint32_t	TaghaScript_nativecount(const struct TaghaScript *script);
uint32_t	TaghaScript_funcs(const struct TaghaScript *script);
uint32_t	TaghaScript_globals(const struct TaghaScript *script);
bool		TaghaScript_safemode_active(const struct TaghaScript *script);
bool		TaghaScript_debug_active(const struct TaghaScript *script);

void		TaghaScript_PrintErr(struct TaghaScript *script, const char *funcname, const char *err, ...);




#define INSTR_SET	\
	X(halt) \
	X(pushq) X(pushl) X(pushs) X(pushb) X(pushsp) X(puship) X(pushbp) X(pushoffset) \
	X(pushspadd) X(pushspsub) X(pushbpadd) X(pushbpsub) X(pushipadd) X(pushipsub) \
	\
	X(popq) X(popsp) X(popip) X(popbp) \
	\
	X(storespq) X(storespl) X(storesps) X(storespb) \
	X(loadspq) X(loadspl) X(loadsps) X(loadspb) \
	\
	X(copyq) X(copyl) X(copys) X(copyb) \
	\
	X(addq) X(uaddq) X(addl) X(uaddl) X(addf) \
	X(subq) X(usubq) X(subl) X(usubl) X(subf) \
	X(mulq) X(umulq) X(mull) X(umull) X(mulf) \
	X(divq) X(udivq) X(divl) X(udivl) X(divf) \
	X(modq) X(umodq) X(modl) X(umodl) \
	X(addf64) X(subf64) X(mulf64) X(divf64) \
	\
	X(andl) X(orl) X(xorl) X(notl) X(shll) X(shrl) \
	X(andq) X(orq) X(xorq) X(notq) X(shlq) X(shrq) \
	X(incq) X(incl) X(incf) X(decq) X(decl) X(decf) X(negq) X(negl) X(negf) \
	X(incf64) X(decf64) X(negf64) \
	\
	X(ltq) X(ltl) X(ultq) X(ultl) X(ltf) \
	X(gtq) X(gtl) X(ugtq) X(ugtl) X(gtf) \
	X(cmpq) X(cmpl) X(ucmpq) X(ucmpl) X(compf) \
	X(leqq) X(uleqq) X(leql) X(uleql) X(leqf) \
	X(geqq) X(ugeqq) X(geql) X(ugeql) X(geqf) \
	X(ltf64) X(gtf64) X(cmpf64) X(leqf64) X(geqf64) \
	X(neqq) X(uneqq) X(neql) X(uneql) X(neqf) X(neqf64) \
	\
	X(jmp) X(jmps) X(jzq) X(jnzq) X(jzl) X(jnzl) \
	X(call) X(calls) X(ret) X(retq) X(retl) X(rets) X(retb) \
	X(pushnataddr) X(callnat) X(callnats) \
	/*X(float2dbl) X(dbl2float) X(float2int) X(int2float) X(dbl2long) X(long2dbl)*/ \
	X(nop) \

#define X(x) x,
enum InstrSet { INSTR_SET };
#undef X

#ifdef __cplusplus
}
#endif

#endif	// TAGHA_H_INCLUDED

