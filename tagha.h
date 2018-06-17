#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "dsc.h"

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

#define TAGHA_VERSION_STR		"0.0.1a"


enum DataClass {
	Bytecode,
	Native
};

/* Script File Structure (May 19, 2018)
 * ------------------------------ start of header ------------------------------
 * 2 bytes: magic verifier ==> 0xC0DE if main exists, else verifier is 0x0D11 "dll"
 * 4 bytes: stack size (will be aligned by 8 bytes)
 * 1 byte: flags
 * ------------------------------ end of header ------------------------------
 * .functions table
 * 4 bytes: amount of functions
 * n bytes: functions table
 *		1 byte: 0 if bytecode function, 1+ if it's a native function.
 * 		4 bytes: string size + '\0' of func string
 *		4 bytes: instruction count, 8 if native function.
 * 		n bytes: function string
 * 		if bytecode function:
 *			n bytes: instructions
 *		else: 8 bytes: native address (0 at first, will be filled in during runtime)
 * 
 * .globalvars table
 * 4 bytes: amount of global vars
 * n bytes: global vars table
 *		1 byte: 0 if bytecode var, 1+ if it's a native var.
 * 		4 bytes: string size + '\0' of global var string
 *		4 bytes: byte size, 8 if native var.
 * 		n bytes: global var string
 * 		if bytecode var:
 *			n bytes: data. All 0 if not initialized in script code.
 *		else: 8 bytes: var address (0 at first, filled in during runtime)
 */

enum ScriptFlags {
	FlagSafeMode	=0x01,
	FlagDebug		=0x02,
	FlagAllocMem	=0x04, // allows malloc calls (for systems with healthy amounts of memory.)
};


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
 * The first 8 parameters are passed registers Peh to Thaw, remaining params are pushed to the stack.
 * If 8 params, Thaw will hold the 8th param, Peh the first.
 * 
 * Return values must be 8 or less bytes in size in register 'Alaf'.
 * For Natives, structs must always be passed by pointer, even if they're smaller than 8 bytes.
 *
 * Since 'Alaf' is the return value register and Peh to Thaw are for functions, they need preservation, all other registers are volatile.
 * 
 */
struct Tagha;
struct NativeInfo {
	const char *Name;
	void (*NativeCFunc)(struct Tagha *, union Value *, size_t, union Value []);
};


enum RegID {
	regAlaf, regBeth, regVeth, regGamal, regGhamal,
	regDalath, regDhalath, regHeh, regWaw, regZain,
	regHeth, regTeth, regYodh, regKaf, regKhaf, regLamad,
	regMeem, regNoon, regSemkath, reg_Eh,
	regPeh, regFeh, regSade, regQof, regReesh,
	regSheen, regTaw, regThaw,
	// Syriac alphabet makes great register names!
	regStk, regBase, regInstr,
	regsize
};

struct Tagha {
	union Value Regs[regsize];
	struct Hashmap Natives;
	union Value
		ScriptHdr,
		FuncTable,
		GVarTable
	;
	bool CondFlag : 1; // conditional flag for conditional jumps!
};

struct Tagha *Tagha_New(void *);
void Tagha_Init(struct Tagha *, void *);
void Tagha_Del(struct Tagha *);
void Tagha_Free(struct Tagha **);

void TaghaDebug_PrintRegisters(const struct Tagha *);
bool Tagha_RegisterNatives(struct Tagha *, struct NativeInfo []);
void *Tagha_GetGlobalVarByName(struct Tagha *, const char *);
int32_t Tagha_CallFunc(struct Tagha *, const char *, size_t, union Value []);
union Value Tagha_GetReturnValue(const struct Tagha *);
int32_t Tagha_RunScript(struct Tagha *, int32_t, char *[]);
int32_t Tagha_Exec(struct Tagha *);


enum AddrMode {
	Immediate	= 1, /* interpret as immediate/constant value */
	Register	= 2, /* interpret as register id */
	RegIndirect	= 4, /* interpret register id's contents as a memory address. */
	Unsign = 8, /* interpret as unsigned data. */
	Byte		= 16, /* use data as (u)int8_t * */
	TwoBytes	= 32, /* use data as (u)int16_t * */
	FourBytes	= 64, /* use data as (u)int32_t * */
	EightBytes	= 128, /* use data as (u)int64_t * */
};


#define FLOATING_POINT_OPS 1

#ifdef FLOATING_POINT_OPS
	#define INSTR_SET	\
		X(halt) \
		X(push) X(pop) \
		\
		X(lea) X(mov) X(movgbl) \
		\
		X(add) X(sub) X(mul) X(divi) X(mod) \
		\
		X(andb) X(orb) X(xorb) X(notb) X(shl) X(shr) \
		X(inc) X(dec) X(neg) \
		\
		X(lt) X(gt) X(cmp) X(neq) \
		\
		X(jmp) X(jz) X(jnz) \
		X(call) X(syscall) X(ret) \
		\
		X(flt2dbl) X(dbl2flt) \
		X(addf) X(subf) X(mulf) X(divf) \
		X(incf) X(decf) X(negf) \
		X(ltf) X(gtf) X(cmpf) X(neqf) \
		X(nop)
#else
#define INSTR_SET	\
		X(halt) \
		X(push) X(pop) \
		\
		X(lea) X(mov) X(movgbl) \
		\
		X(add) X(sub) X(mul) X(divi) X(mod) \
		\
		X(andb) X(orb) X(xorb) X(notb) X(shl) X(shr) \
		X(inc) X(dec) X(neg) \
		\
		X(lt) X(gt) X(cmp) X(neq) \
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
#endif
