#ifndef CROWN_HPP_INCLUDED
	#define CROWN_HPP_INCLUDED

#include <inttypes.h>
#include <stdbool.h>
#include <iso646.h>

enum RegID;
struct VM;
union Val;

typedef struct VM		VM_t;
typedef union Val		Val_t;

union Val {
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
	union Val *SelfVal;
	
	int8_t *CharPtr;
	int16_t *ShortPtr;
	int32_t *Int32Ptr;
	int64_t *Int64Ptr;
	
	uint8_t *UCharPtr;
	uint16_t *UShortPtr;
	uint32_t *UInt32Ptr;
	uint64_t *UInt64Ptr;
	
	float *FloatPtr;
	double *DoublePtr;
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
	Half		= 32,
	Long		= 64,
	Quad		= 128,
};

// Register ID list
// (almost) all registers are General Purpose.
// "r" = register, "s" = storage
enum RegID {
	ras=0,		// general purpose, accumulator - all comparison operation results go here.
	rbs,rcs,	// general purpose
	rds,res,	// general purpose
	rfs,rgs,rhs,// general purpose, floating point regs, rfs is used as float accumulator
	ris,rjs,rks,// general purpose
	rsp,rbp,	// stack ptrs, do not touch
	rip,		// instr ptr, do not touch as well.
	regsize		// for lazily updating id list
};

struct VM {
	union Val
		reg[regsize]
	;
	uint8_t zero_flag : 1;
};

const char *vm_regid_to_str(const enum RegID id);

/* halt
 * mov
 * push, pop
 * lea
 * add, sub, mul, div, mod
 * inc, dec, neg
 * shr, shl, and, or, xor, not
 * lt, gt, cmp, leq, geq, neq
 * jmp, jnz, jz
 * call, ret
 * nop
*/


/*
*	r = register
*	m = memory address
*	i = immediate
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
	X(movi) X(movr) X(movm) X(lea) \
	/* signed and unsigned integer arithmetic ops */ \
	X(addi) X(addr) X(addm) X(uaddi) X(uaddr) X(uaddm) \
	X(subi) X(subr) X(subm) X(usubi) X(usubr) X(usubm) \
	X(muli) X(mulr) X(mulm) X(umuli) X(umulr) X(umulm) \
	X(divi) X(divr) X(divm) X(udivi) X(udivr) X(udivm) \
	X(modi) X(modr) X(modm) X(umodi) X(umodr) X(umodm) \
	/* bitwise ops */ \
	X(shri) X(shrr) X(shrm) X(shli) X(shlr) X(shlm) \
	X(andi) X(andr) X(andm) X(ori) X(orr) X(orm) X(xori) X(xorr) X(xorm) \
	/* comparison ops */ \
	X(lti) X(ltr) X(ltm) X(ulti) X(ultr) X(ultm) \
	X(gti) X(gtr) X(gtm) X(ugti) X(ugtr) X(ugtm) \
	X(cmpi) X(cmpr) X(cmpm) X(ucmpi) X(ucmpr) X(ucmpm) \
	X(neqi) X(neqr) X(neqm) X(uneqi) X(uneqr) X(uneqm) \
	X(reset) \
	\
	/* floating point opcodes */ \
	X(nop) \

#define X(x)	x,
enum InstrSet{ INSTR_SET };
#undef X

#endif	// CROWN_HPP_INCLUDED
