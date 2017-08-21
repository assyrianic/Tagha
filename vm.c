
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <iso646.h>
#include <inttypes.h>
//#include <stdarg.h>
#include <assert.h>
#include "vm.h"

/*	here's the deal ok? make an opcode for each and erry n-bytes!
 * 'o' - int128 = "octo word"
 * 'q' - int64 = "quad word"
 * 'l' - int32 = "long word"
 * 's' - int16
 * 'b' - byte
 * 'f' - float32
 * 'f64' - float64
 * 'f80' - float80
 * 'a' - address
 * 'sp' - takes or uses the current stack pointer address.
 * 'ip' - takes/uses the current instruction pointer address.
*/

// this vm is designed to run C programs. Vastly, if not all C expressions are int32, uint32 if bigger than int
// this is why the arithmetic and bit operations are all int32 sized.
// there's 2 byte and single byte memory storage for the sake of dealing with structs and unions.
// expressions are int or uint then truncated to a variable's byte-width.
#define INSTR_SET	\
	X(halt) \
	X(pushq) X(pushl) X(pushs) X(pushb) X(pushsp) X(puship) X(pushbp) \
	X(pushspadd) X(pushspsub) X(pushbpadd) X(pushbpsub) \
	X(popq) X(popl) X(pops) X(popb) X(popsp) X(popip) X(popbp) \
	X(wrtq) X(wrtl) X(wrts) X(wrtb) \
	X(storeq) X(storel) X(stores) X(storeb) \
	X(storeqa) X(storela) X(storesa) X(storeba) \
	X(storespq) X(storespl) X(storesps) X(storespb) \
	X(loadq) X(loadl) X(loads) X(loadb) \
	X(loadqa) X(loadla) X(loadsa) X(loadba) \
	X(loadspq) X(loadspl) X(loadsps) X(loadspb) \
	X(copyq) X(copyl) X(copys) X(copyb) \
	X(addq) X(uaddq) X(addl) X(uaddl) X(addf) \
	X(subq) X(usubq) X(subl) X(usubl) X(subf) \
	X(mulq) X(umulq) X(mull) X(umull) X(mulf) \
	X(divq) X(udivq) X(divl) X(udivl) X(divf) \
	X(modq) X(umodq) X(modl) X(umodl) \
	X(addf64) X(subf64) X(mulf64) X(divf64) \
	X(andl) X(orl) X(xorl) X(notl) X(shll) X(shrl) \
	X(andq) X(orq) X(xorq) X(notq) X(shlq) X(shrq) \
	X(incq) X(incl) X(incf) X(decq) X(decl) X(decf) X(negq) X(negl) X(negf) \
	X(incf64) X(decf64) X(negf64) \
	X(ltq) X(ltl) X(ultq) X(ultl) X(ltf) \
	X(gtq) X(gtl) X(ugtq) X(ugtl) X(gtf) \
	X(cmpq) X(cmpl) X(ucmpq) X(ucmpl) X(compf) \
	X(leqq) X(uleqq) X(leql) X(uleql) X(leqf) \
	X(geqq) X(ugeqq) X(geql) X(ugeql) X(geqf) \
	X(ltf64) X(gtf64) X(cmpf64) X(leqf64) X(geqf64) \
	X(neqq) X(uneqq) X(neql) X(uneql) X(neqf) X(neqf64) \
	X(jmp) X(jzq) X(jnzq) X(jzl) X(jnzl) \
	X(call) X(calls) X(calla) X(ret) X(retx) X(reset) \
	X(callnat) X(callnats) X(callnata) \
	X(mmxaddl) X(mmxuaddl) X(mmxaddf) X(mmxadds) X(mmxuadds) X(mmxaddb) X(mmxuaddb) \
	X(mmxsubl) X(mmxusubl) X(mmxsubf) X(mmxsubs) X(mmxusubs) X(mmxsubb) X(mmxusubb) \
	X(mmxmull) X(mmxumull) X(mmxmulf) X(mmxmuls) X(mmxumuls) X(mmxmulb) X(mmxumulb) \
	X(mmxdivl) X(mmxudivl) X(mmxdivf) X(mmxdivs) X(mmxudivs) X(mmxdivb) X(mmxudivb) \
	X(mmxmodl) X(mmxumodl) X(mmxmods) X(mmxumods) X(mmxmodb) X(mmxumodb) \
	X(mmxandl) X(mmxands) X(mmxandb) X(mmxorl) X(mmxors) X(mmxorb) \
	X(mmxxorl) X(mmxxors) X(mmxxorb) X(mmxnotl) X(mmxnots) X(mmxnotb) \
	X(mmxshll) X(mmxshls) X(mmxshlb) X(mmxshrl) X(mmxshrs) X(mmxshrb) \
	X(mmxincl) X(mmxincf) X(mmxincs) X(mmxincb) X(mmxdecl) X(mmxdecf) X(mmxdecs) X(mmxdecb) \
	X(mmxnegl) X(mmxnegf) X(mmxnegs) X(mmxnegb) \
	X(nop) \

#define X(x) x,
enum InstrSet { INSTR_SET };
#undef X

static int is_bigendian()
{
	const int i=1;
	return( (*(char *)&i) == 0 );
}

// This is strictly for long doubles
static inline void _tagha_get_immn(TaghaVM_t *restrict vm, void *restrict pBuffer, const Word_t bytesize)
{
	if( !vm or !pBuffer )
		return;
	
	uchar bytes[bytesize];
	uchar i = bytesize-1;
	while( i<bytesize )
		((uchar *)pBuffer)[i--] = vm->pInstrStream[++vm->ip];
}

static inline u64 _tagha_get_imm8(TaghaVM_t *restrict vm)
{
	if( !vm )
		return 0;
	union conv_union conv;
	conv.c[7] = vm->pInstrStream[++vm->ip];
	conv.c[6] = vm->pInstrStream[++vm->ip];
	conv.c[5] = vm->pInstrStream[++vm->ip];
	conv.c[4] = vm->pInstrStream[++vm->ip];
	conv.c[3] = vm->pInstrStream[++vm->ip];
	conv.c[2] = vm->pInstrStream[++vm->ip];
	conv.c[1] = vm->pInstrStream[++vm->ip];
	conv.c[0] = vm->pInstrStream[++vm->ip];
	return conv.ull;
}

static inline uint _tagha_get_imm4(TaghaVM_t *restrict vm)
{
	if( !vm )
		return 0;
	union conv_union conv;
	//	0x0A,0x0B,0x0C,0x0D,
	conv.c[3] = vm->pInstrStream[++vm->ip];
	conv.c[2] = vm->pInstrStream[++vm->ip];
	conv.c[1] = vm->pInstrStream[++vm->ip];
	conv.c[0] = vm->pInstrStream[++vm->ip];
	return conv.ui;
}

static inline ushort _tagha_get_imm2(TaghaVM_t *restrict vm)
{
	if( !vm )
		return 0;
	union conv_union conv;
	conv.c[1] = vm->pInstrStream[++vm->ip];
	conv.c[0] = vm->pInstrStream[++vm->ip];
	return conv.us;
}

static inline void _tagha_push_int64(TaghaVM_t *restrict vm, const u64 val)
{
	if( !vm )
		return;
	if( vm->bSafeMode and (vm->sp+8) >= STK_SIZE ) {
		printf("tagha_push_int64 reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp+8);
		exit(1);
	}
	union conv_union conv;
	conv.ull = val;
	vm->pbStack[++vm->sp] = conv.c[0];
	vm->pbStack[++vm->sp] = conv.c[1];
	vm->pbStack[++vm->sp] = conv.c[2];
	vm->pbStack[++vm->sp] = conv.c[3];
	vm->pbStack[++vm->sp] = conv.c[4];
	vm->pbStack[++vm->sp] = conv.c[5];
	vm->pbStack[++vm->sp] = conv.c[6];
	vm->pbStack[++vm->sp] = conv.c[7];
}
static inline u64 _tagha_pop_int64(TaghaVM_t *vm)
{
	if( !vm )
		return 0L;
	if( vm->bSafeMode and (vm->sp-8) >= STK_SIZE ) {
		printf("tagha_pop_int64 reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-8);
		exit(1);
	}
	union conv_union conv;
	conv.c[7] = vm->pbStack[vm->sp--];
	conv.c[6] = vm->pbStack[vm->sp--];
	conv.c[5] = vm->pbStack[vm->sp--];
	conv.c[4] = vm->pbStack[vm->sp--];
	conv.c[3] = vm->pbStack[vm->sp--];
	conv.c[2] = vm->pbStack[vm->sp--];
	conv.c[1] = vm->pbStack[vm->sp--];
	conv.c[0] = vm->pbStack[vm->sp--];
	return conv.ull;
}

static inline void _tagha_push_float64(TaghaVM_t *restrict vm, const double val)
{
	if( !vm )
		return;
	if( vm->bSafeMode and (vm->sp+8) >= STK_SIZE ) {
		printf("tagha_push_float64 reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp+8);
		exit(1);
	}
	union conv_union conv;
	conv.dbl = val;
	vm->pbStack[++vm->sp] = conv.c[0];
	vm->pbStack[++vm->sp] = conv.c[1];
	vm->pbStack[++vm->sp] = conv.c[2];
	vm->pbStack[++vm->sp] = conv.c[3];
	vm->pbStack[++vm->sp] = conv.c[4];
	vm->pbStack[++vm->sp] = conv.c[5];
	vm->pbStack[++vm->sp] = conv.c[6];
	vm->pbStack[++vm->sp] = conv.c[7];
}
static inline double _tagha_pop_float64(TaghaVM_t *vm)
{
	if( !vm )
		return 0;
	if( vm->bSafeMode and (vm->sp-8) >= STK_SIZE ) {
		printf("tagha_pop_float64 reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-8);
		exit(1);
	}
	union conv_union conv;
	conv.c[7] = vm->pbStack[vm->sp--];
	conv.c[6] = vm->pbStack[vm->sp--];
	conv.c[5] = vm->pbStack[vm->sp--];
	conv.c[4] = vm->pbStack[vm->sp--];
	conv.c[3] = vm->pbStack[vm->sp--];
	conv.c[2] = vm->pbStack[vm->sp--];
	conv.c[1] = vm->pbStack[vm->sp--];
	conv.c[0] = vm->pbStack[vm->sp--];
	return conv.dbl;
}

static inline void _tagha_push_int32(TaghaVM_t *restrict vm, const uint val)
{
	if( !vm )
		return;
	if( vm->bSafeMode and (vm->sp+4) >= STK_SIZE ) {
		printf("tagha_push_int32 reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp+4);
		exit(1);
	}
	union conv_union conv;
	conv.ui = val;
	vm->pbStack[++vm->sp] = conv.c[0];
	vm->pbStack[++vm->sp] = conv.c[1];
	vm->pbStack[++vm->sp] = conv.c[2];
	vm->pbStack[++vm->sp] = conv.c[3];
}
static inline uint _tagha_pop_int32(TaghaVM_t *vm)
{
	if( !vm )
		return 0;
	if( vm->bSafeMode and (vm->sp-4) >= STK_SIZE ) {	// we're subtracting, did we integer underflow?
		printf("tagha_pop_int32 reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-4);
		exit(1);
	}
	union conv_union conv;
	conv.c[3] = vm->pbStack[vm->sp--];
	conv.c[2] = vm->pbStack[vm->sp--];
	conv.c[1] = vm->pbStack[vm->sp--];
	conv.c[0] = vm->pbStack[vm->sp--];
	return conv.ui;
}

static inline void _tagha_push_float32(TaghaVM_t *restrict vm, const float val)
{
	if( !vm )
		return;
	if( vm->bSafeMode and (vm->sp+4) >= STK_SIZE ) {
		printf("tagha_push_float32 reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp+4);
		exit(1);
	}
	union conv_union conv;
	conv.f = val;
	vm->pbStack[++vm->sp] = conv.c[0];
	vm->pbStack[++vm->sp] = conv.c[1];
	vm->pbStack[++vm->sp] = conv.c[2];
	vm->pbStack[++vm->sp] = conv.c[3];
}
static inline float _tagha_pop_float32(TaghaVM_t *vm)
{
	if( !vm )
		return 0;
	if( vm->bSafeMode and (vm->sp-4) >= STK_SIZE ) {
		printf("tagha_pop_float32 reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-4);
		exit(1);
	}
	union conv_union conv;
	conv.c[3] = vm->pbStack[vm->sp--];
	conv.c[2] = vm->pbStack[vm->sp--];
	conv.c[1] = vm->pbStack[vm->sp--];
	conv.c[0] = vm->pbStack[vm->sp--];
	return conv.f;
}

static inline void _tagha_push_short(TaghaVM_t *restrict vm, const ushort val)
{
	if( !vm )
		return;
	if( vm->bSafeMode and (vm->sp+2) >= STK_SIZE ) {
		printf("tagha_push_short reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp+2);
		exit(1);
	}
	union conv_union conv;
	conv.us = val;
	vm->pbStack[++vm->sp] = conv.c[0];
	vm->pbStack[++vm->sp] = conv.c[1];
}
static inline ushort _tagha_pop_short(TaghaVM_t *vm)
{
	if( !vm )
		return 0;
	if( vm->bSafeMode and (vm->sp-2) >= STK_SIZE ) {
		printf("tagha_pop_short reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-2);
		exit(1);
	}
	union conv_union conv;
	conv.c[1] = vm->pbStack[vm->sp--];
	conv.c[0] = vm->pbStack[vm->sp--];
	return conv.us;
}

static inline void _tagha_push_byte(TaghaVM_t *restrict vm, const uchar val)
{
	if( !vm )
		return;
	if( vm->bSafeMode and (vm->sp+1) >= STK_SIZE ) {
		printf("tagha_push_byte reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp+1);
		exit(1);
	}
	vm->pbStack[++vm->sp] = val;
}
static inline uchar _tagha_pop_byte(TaghaVM_t *vm)
{
	if( !vm )
		return 0;
	if( vm->bSafeMode and (vm->sp-1) >= STK_SIZE ) {
		printf("tagha_pop_byte reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-1);
		exit(1);
	}
	return vm->pbStack[vm->sp--];
}

static inline void _tagha_push_nbytes(TaghaVM_t *restrict vm, void *restrict pItem, const Word_t bytesize)
{
	if( !vm )
		return;
	if( vm->bSafeMode and (vm->sp+bytesize) >= STK_SIZE ) {
		printf("tagha_push_nbytes reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp+1);
		exit(1);
	}
	Word_t i=0;
	for( i=0 ; i<bytesize ; i++ )
		vm->pbStack[++vm->sp] = ((uchar *)pItem)[i];
}
static inline void _tagha_pop_nbytes(TaghaVM_t *restrict vm, void *restrict pBuffer, const Word_t bytesize)
{
	if( !vm )
		return;
	if( vm->bSafeMode and (vm->sp-bytesize) >= STK_SIZE ) {
		printf("tagha_pop_nbytes reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp+1);
		exit(1);
	}
	Word_t i=0;
	// should stop when the integer underflows
	for( i=bytesize-1 ; i<bytesize ; i-- )
		((uchar *)pBuffer)[i] = vm->pbStack[vm->sp--];
}


static inline u64 _tagha_read_int64(TaghaVM_t *restrict vm, const Word_t address)
{
	if( !vm )
		return 0;
	if( vm->bSafeMode and address > MEM_SIZE-8 ) {
		printf("tagha_read_int64 reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, address);
		exit(1);
	}
	return( *(u64 *)(vm->pbMemory + address) );
}
static inline void _tagha_write_int64(TaghaVM_t *restrict vm, const u64 val, const Word_t address)
{
	if( !vm )
		return;
	if( vm->bSafeMode and address > MEM_SIZE-8 ) {
		printf("tagha_write_int64 reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, address);
		exit(1);
	}
	*(u64 *)(vm->pbMemory + address) = val;
}

static inline double _tagha_read_float64(TaghaVM_t *restrict vm, const Word_t address)
{
	if( !vm )
		return 0;
	if( vm->bSafeMode and address > MEM_SIZE-8 ) {
		printf("tagha_read_float64 reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, address);
		exit(1);
	}
	return( *(double *)(vm->pbMemory + address) );
}
static inline void _tagha_write_float64(TaghaVM_t *restrict vm, const double val, const Word_t address)
{
	if( !vm )
		return;
	if( vm->bSafeMode and address > MEM_SIZE-8 ) {
		printf("tagha_write_float64 reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, address);
		exit(1);
	}
	*(double *)(vm->pbMemory + address) = val;
}

static inline uint _tagha_read_int32(TaghaVM_t *restrict vm, const Word_t address)
{
	if( !vm )
		return 0;
	if( vm->bSafeMode and address > MEM_SIZE-4 ) {
		printf("tagha_read_int32 reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, address);
		exit(1);
	}
	/*
	union conv_union conv;
	conv.c[0] = vm->pbMemory[address];
	conv.c[1] = vm->pbMemory[address+1];
	conv.c[2] = vm->pbMemory[address+2];
	conv.c[3] = vm->pbMemory[address+3];
	return conv.ui;
	*/
	return( *(uint *)(vm->pbMemory + address) );
}
static inline void _tagha_write_int32(TaghaVM_t *restrict vm, const uint val, const Word_t address)
{
	if( !vm )
		return;
	if( vm->bSafeMode and address > MEM_SIZE-4 ) {
		printf("tagha_write_int32 reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, address);
		exit(1);
	}
	/*
	union conv_union conv;
	conv.ui = val;
	vm->pbMemory[address] = conv.c[0];
	vm->pbMemory[address+1] = conv.c[1];
	vm->pbMemory[address+2] = conv.c[2];
	vm->pbMemory[address+3] = conv.c[3];
	*/
	//printf("wrote %" PRIu32 " to address: %" PRIu32 "\n" );
	*(uint *)(vm->pbMemory + address) = val;
}

static inline void _tagha_write_short(TaghaVM_t *restrict vm, const ushort val, const Word_t address)
{
	if( !vm )
		return;
	if( vm->bSafeMode and address > MEM_SIZE-2 ) {
		printf("tagha_write_short reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, address);
		exit(1);
	}
	/*
	union conv_union conv;
	conv.us = val;
	vm->pbMemory[address] = conv.c[0];
	vm->pbMemory[address+1] = conv.c[1];
	*/
	*(ushort *)(vm->pbMemory + address) = val;
}

static inline ushort _tagha_read_short(TaghaVM_t *restrict vm, const Word_t address)
{
	if( !vm )
		return 0;
	if( vm->bSafeMode and address > MEM_SIZE-2 ) {
		printf("tagha_read_short reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, address);
		exit(1);
	}
	/*
	union conv_union conv;
	conv.c[0] = vm->pbMemory[address];
	conv.c[1] = vm->pbMemory[address+1];
	return conv.us;
	*/
	return( *(ushort *)(vm->pbMemory + address) );
}

static inline uchar _tagha_read_byte(TaghaVM_t *restrict vm, const Word_t address)
{
	if( !vm )
		return 0;
	if( vm->bSafeMode and address >= MEM_SIZE ) {
		printf("tagha_read_byte reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, address);
		exit(1);
	}
	return vm->pbMemory[address];
}
static inline void _tagha_write_byte(TaghaVM_t *restrict vm, const uchar val, const Word_t address)
{
	if( !vm )
		return;
	if( vm->bSafeMode and address >= MEM_SIZE ) {
		printf("tagha_write_byte reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, address);
		exit(1);
	}
	vm->pbMemory[address] = val;
}

static inline float _tagha_read_float32(TaghaVM_t *restrict vm, const Word_t address)
{
	if( !vm )
		return 0;
	if( vm->bSafeMode and address > MEM_SIZE-4 ) {
		printf("tagha_read_float32 reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, address);
		exit(1);
	}
	/*
	union conv_union conv;
	conv.c[0] = vm->pbMemory[address];
	conv.c[1] = vm->pbMemory[address+1];
	conv.c[2] = vm->pbMemory[address+2];
	conv.c[3] = vm->pbMemory[address+3];
	return conv.f;
	*/
	return( *(float *)(vm->pbMemory + address) );
}
static inline void _tagha_write_float32(TaghaVM_t *restrict vm, const float val, const Word_t address)
{
	if( !vm )
		return;
	if( vm->bSafeMode and address > MEM_SIZE-4 ) {
		printf("tagha_write_float32 reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, address);
		exit(1);
	}
	/*
	union conv_union conv;
	conv.f = val;
	vm->pbMemory[address] = conv.c[0];
	vm->pbMemory[address+1] = conv.c[1];
	vm->pbMemory[address+2] = conv.c[2];
	vm->pbMemory[address+3] = conv.c[3];
	*/
	*(float *)(vm->pbMemory + address) = val;
}

static inline void _tagha_read_nbytes(TaghaVM_t *restrict vm, void *restrict pBuffer, const Word_t bytesize, const Word_t address)
{
	if( !vm )
		return;
	
	Word_t	addr = address;
	Word_t	i=0;
	while( i<bytesize ) {
		if( vm->bSafeMode and addr >= MEM_SIZE-i ) {
			printf("tagha_read_array reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, addr);
			exit(1);
		}
		((uchar *)pBuffer)[i++] = vm->pbMemory[addr++];
		//buffer[i++] = vm->pbMemory[addr++];
	}
}
static inline void _tagha_write_nbytes(TaghaVM_t *restrict vm, void *restrict pItem, const Word_t bytesize, const Word_t address)
{
	if( !vm )
		return;
	
	Word_t	addr = address;
	Word_t	i=0;
	while( i<bytesize ) {
		if( vm->bSafeMode and addr >= MEM_SIZE+i ) {
			printf("tagha_write_array reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, addr);
			exit(1);
		}
		//vm->pbMemory[addr++] = val[i++];
		vm->pbMemory[addr++] = ((uchar *)pItem)[i++];
	}
}


static inline u64 _tagha_peek_int64(TaghaVM_t *vm)
{
	if( !vm )
		return 0;
	if( vm->bSafeMode and (vm->sp-7) >= STK_SIZE ) {
		printf("Tagha_peek_int32 reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-7);
		exit(1);
	}
	union conv_union conv;
	conv.c[7] = vm->pbStack[vm->sp];
	conv.c[6] = vm->pbStack[vm->sp-1];
	conv.c[5] = vm->pbStack[vm->sp-2];
	conv.c[4] = vm->pbStack[vm->sp-3];
	conv.c[3] = vm->pbStack[vm->sp-4];
	conv.c[2] = vm->pbStack[vm->sp-5];
	conv.c[1] = vm->pbStack[vm->sp-6];
	conv.c[0] = vm->pbStack[vm->sp-7];
	return conv.ull;
}
static inline double _tagha_peek_float64(TaghaVM_t *vm)
{
	if( !vm )
		return 0;
	if( vm->bSafeMode and (vm->sp-7) >= STK_SIZE ) {
		printf("Tagha_peek_int32 reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-7);
		exit(1);
	}
	union conv_union conv;
	conv.c[7] = vm->pbStack[vm->sp];
	conv.c[6] = vm->pbStack[vm->sp-1];
	conv.c[5] = vm->pbStack[vm->sp-2];
	conv.c[4] = vm->pbStack[vm->sp-3];
	conv.c[3] = vm->pbStack[vm->sp-4];
	conv.c[2] = vm->pbStack[vm->sp-5];
	conv.c[1] = vm->pbStack[vm->sp-6];
	conv.c[0] = vm->pbStack[vm->sp-7];
	return conv.dbl;
}
static inline uint _tagha_peek_int32(TaghaVM_t *vm)
{
	if( !vm )
		return 0;
	if( vm->bSafeMode and (vm->sp-3) >= STK_SIZE ) {
		printf("Tagha_peek_int32 reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-3);
		exit(1);
	}
	union conv_union conv;
	conv.c[3] = vm->pbStack[vm->sp];
	conv.c[2] = vm->pbStack[vm->sp-1];
	conv.c[1] = vm->pbStack[vm->sp-2];
	conv.c[0] = vm->pbStack[vm->sp-3];
	return conv.ui;
}

static inline float _tagha_peek_float32(TaghaVM_t *vm)
{
	if( !vm )
		return 0;
	if( vm->bSafeMode and (vm->sp-3) >= STK_SIZE ) {	// we're subtracting, did we integer underflow?
		printf("Tagha_peek_float32 reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-3);
		exit(1);
	}
	union conv_union conv;
	conv.c[3] = vm->pbStack[vm->sp];
	conv.c[2] = vm->pbStack[vm->sp-1];
	conv.c[1] = vm->pbStack[vm->sp-2];
	conv.c[0] = vm->pbStack[vm->sp-3];
	return conv.f;
}

static inline ushort _tagha_peek_short(TaghaVM_t *vm)
{
	if( !vm )
		return 0;
	if( vm->bSafeMode and (vm->sp-1) >= STK_SIZE ) {
		printf("Tagha_peek_short reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-1);
		exit(1);
	}
	union conv_union conv;
	conv.c[1] = vm->pbStack[vm->sp];
	conv.c[0] = vm->pbStack[vm->sp-1];
	return conv.us;
}

static inline uchar _tagha_peek_byte(TaghaVM_t *vm)
{
	if( !vm )
		return 0;
	if( vm->bSafeMode and (vm->sp) >= STK_SIZE ) {
		printf("Tagha_peek_byte reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp);
		exit(1);
	}
	return vm->pbStack[vm->sp];
}


//#include <unistd.h>	// sleep() func
void tagha_exec(TaghaVM_t *restrict vm)
{
	printf("instruction set size == %u\n", nop);
	if( !vm )
		return;
	else if( !vm->pInstrStream )
		return;
	
	union conv_union conv, convb;
	uint	b,a;
	u64		qb, qa;
	double	db,da;
	float	fb,fa;
	ushort	sb,sa;
	uchar	cb,ca;
	long double ldb, lda;
	
	
#define X(x) #x ,
	const char *opcode2str[] = { INSTR_SET };
#undef X

#define X(x) &&exec_##x ,
	static const void *dispatch[] = { INSTR_SET };
#undef X
#undef INSTR_SET
	
	
#ifdef _UNISTD_H
	//#define DISPATCH()	sleep(1); INC(); goto *dispatch[ vm->pInstrStream[++vm->ip] ]
	//#define JUMP()		sleep(1); INC(); goto *dispatch[ vm->pInstrStream[vm->ip] ]
	#define DISPATCH()	sleep(1); ++vm->ip; continue
#else
	//#define DISPATCH()	INC(); goto *dispatch[ vm->pInstrStream[++vm->ip] ]
	//#define JUMP()		INC(); goto *dispatch[ vm->pInstrStream[vm->ip] ]
	#define DISPATCH()	++vm->ip; continue
#endif
	
	while( 1 ) {
		vm->uiMaxInstrs--;
		if( !vm->uiMaxInstrs )
			break;
		
		if( vm->pInstrStream[vm->ip] > nop) {
			printf("illegal instruction exception! instruction == \'%" PRIu32 "\' @ %" PRIu32 "\n", vm->pInstrStream[vm->ip], vm->ip);
			goto *dispatch[halt];
		}
		//printf( "current instruction == \"%s\" @ ip == %" PRIu32 "\n", opcode2str[vm->pInstrStream[vm->ip]], vm->ip );
		goto *dispatch[ vm->pInstrStream[vm->ip] ];
		
		exec_nop:;
			DISPATCH();
		
		exec_halt:;
			printf("========================= [vm done] =========================\n\n");
			return;
		
		exec_pushq:;
			conv.ull = _tagha_get_imm8(vm);
			printf("pushl: pushed %" PRIu64 "\n", conv.ull);
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
		
		// opcodes for longs
		exec_pushl:;	// push 4 bytes onto the stack
			conv.ui = _tagha_get_imm4(vm);
			printf("pushl: pushed %" PRIu32 "\n", conv.ui);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_pushs:;	// push 2 bytes onto the stack
			conv.us = _tagha_get_imm2(vm);
			_tagha_push_short(vm, conv.us);
			//vm->pbStack[++vm->sp] = conv.c[0];
			//vm->pbStack[++vm->sp] = conv.c[1];
			printf("pushs: pushed %" PRIu32 "\n", conv.us);
			DISPATCH();
		
		exec_pushb:;	// push a byte onto the stack
			//vm->pbStack[++vm->sp] = vm->pInstrStream[++vm->ip];
			_tagha_push_byte(vm, vm->pInstrStream[++vm->ip]);
			printf("pushb: pushed %" PRIu32 "\n", vm->pbStack[vm->sp]);
			DISPATCH();
		
		exec_pushsp:;	// push sp onto the stack, uses 4 bytes since 'sp' is uint32
			conv.ui = vm->sp;
			_tagha_push_int32(vm, conv.ui);
			printf("pushsp: pushed sp index: %" PRIu32 "\n", conv.ui);
			DISPATCH();
		
		exec_puship:;
			conv.ui = vm->ip;
			_tagha_push_int32(vm, conv.ui);
			printf("puship: pushed ip index: %" PRIu32 "\n", conv.ui);
			DISPATCH();
		
		exec_pushbp:;
			_tagha_push_int32(vm, vm->bp);
			printf("pushbp: pushed bp index: %" PRIu32 "\n", vm->bp);
			DISPATCH();
		
		exec_pushspadd:;
			a = vm->sp;
			b = _tagha_pop_int32(vm);
			_tagha_push_int32(vm, a+b);
			printf("pushspadd: added sp with %" PRIu32 ", result: %" PRIu32 "\n", b, a+b);
			DISPATCH();
			
		exec_pushspsub:;
			a = vm->sp;
			b = _tagha_pop_int32(vm);
			_tagha_push_int32(vm, a-b);
			printf("pushspsub: subbed sp with %" PRIu32 ", result: %" PRIu32 "\n", b, a-b);
			DISPATCH();
		
		exec_pushbpadd:;
			a = vm->bp;
			b = _tagha_pop_int32(vm);
			_tagha_push_int32(vm, a-b);
			printf("pushbpadd: added bp with %" PRIu32 ", result: %" PRIu32 "\n", b, a-b);
			DISPATCH();
		
		exec_pushbpsub:;
			a = vm->bp;
			b = _tagha_pop_int32(vm);
			_tagha_push_int32(vm, a-b);
			printf("pushbpsub: subbed bp with %" PRIu32 ", result: %" PRIu32 "\n", b, a-b);
			DISPATCH();
		
		exec_popq:;
			if( vm->bSafeMode and (vm->sp-8) >= STK_SIZE ) {
				printf("exec_popq reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-8);
				goto *dispatch[halt];
			}
			vm->sp -= 8;
			printf("popq\n");
			DISPATCH();
			
		exec_popl:;		// pop 4 bytes to eventually be overwritten
			if( vm->bSafeMode and (vm->sp-4) >= STK_SIZE ) {
				printf("exec_popl reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-4);
				goto *dispatch[halt];
			}
			vm->sp -= 4;
			printf("popl\n");
			DISPATCH();
		
		exec_pops:;		// pop 2 bytes
			if( vm->bSafeMode and (vm->sp-2) >= STK_SIZE ) {
				printf("exec_pops reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-2);
				goto *dispatch[halt];
			}
			vm->sp -= 2;
			printf("pops\n");
			DISPATCH();
		
		exec_popb:;		// pop a byte
			if( vm->bSafeMode and (vm->sp-1) >= STK_SIZE ) {
				printf("exec_popb reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-1);
				goto *dispatch[halt];
			}
			vm->sp--;
			printf("popb\n");
			DISPATCH();
			
		exec_popsp:;
			vm->sp = _tagha_pop_int32(vm);
			printf("popsp: sp is now %" PRIu32 " bytes.\n", vm->sp);
			DISPATCH();
			
		exec_popbp:;
			vm->bp = _tagha_pop_int32(vm);
			printf("popbp: bp is now %" PRIu32 " bytes.\n", vm->bp);
			DISPATCH();
			
		exec_popip:;
			vm->ip = _tagha_pop_int32(vm);
			printf("popip: ip is now at address: %" PRIu32 ".\n", vm->ip);
			continue;
		
		exec_wrtq:;
			a = _tagha_get_imm4(vm);
			if( vm->bSafeMode and a > MEM_SIZE-8 ) {
				printf("exec_wrtq reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, a);
				goto *dispatch[halt];
			}
			// TODO: replace the instr stream with _tagha_get_imm4(vm)
			vm->pbMemory[a+7] = vm->pInstrStream[++vm->ip];
			vm->pbMemory[a+6] = vm->pInstrStream[++vm->ip];
			vm->pbMemory[a+5] = vm->pInstrStream[++vm->ip];
			vm->pbMemory[a+4] = vm->pInstrStream[++vm->ip];
			vm->pbMemory[a+3] = vm->pInstrStream[++vm->ip];
			vm->pbMemory[a+2] = vm->pInstrStream[++vm->ip];
			vm->pbMemory[a+1] = vm->pInstrStream[++vm->ip];
			vm->pbMemory[a+0] = vm->pInstrStream[++vm->ip];
			conv.c[0] = vm->pbMemory[a+0];
			conv.c[1] = vm->pbMemory[a+1];
			conv.c[2] = vm->pbMemory[a+2];
			conv.c[3] = vm->pbMemory[a+3];
			conv.c[4] = vm->pbMemory[a+4];
			conv.c[5] = vm->pbMemory[a+5];
			conv.c[6] = vm->pbMemory[a+6];
			conv.c[7] = vm->pbMemory[a+7];
			printf("wrote long data - %" PRIu64 " @ address 0x%x\n", conv.ull, a);
			DISPATCH();
		
		exec_wrtl:;	// writes an int to memory, First operand is the memory address as 4 byte number, second is the int of data.
			a = _tagha_get_imm4(vm);
			if( vm->bSafeMode and a > MEM_SIZE-4 ) {
				printf("exec_wrtl reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, a);
				goto *dispatch[halt];
			}
			// TODO: replace the instr stream with _tagha_get_imm4(vm)
			vm->pbMemory[a+3] = vm->pInstrStream[++vm->ip];
			vm->pbMemory[a+2] = vm->pInstrStream[++vm->ip];
			vm->pbMemory[a+1] = vm->pInstrStream[++vm->ip];
			vm->pbMemory[a+0] = vm->pInstrStream[++vm->ip];
			conv.c[0] = vm->pbMemory[a+0];
			conv.c[1] = vm->pbMemory[a+1];
			conv.c[2] = vm->pbMemory[a+2];
			conv.c[3] = vm->pbMemory[a+3];
			printf("wrote int data - %" PRIu32 " @ address 0x%x\n", conv.ui, a);
			DISPATCH();
		
		exec_wrts:;	// writes a short to memory. First operand is the memory address as 4 byte number, second is the short of data.
			a = _tagha_get_imm4(vm);
			if( vm->bSafeMode and a > MEM_SIZE-2 ) {
				printf("exec_wrts reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, a);
				goto *dispatch[halt];
			}
			vm->pbMemory[a+1] = vm->pInstrStream[++vm->ip];
			vm->pbMemory[a+0] = vm->pInstrStream[++vm->ip];
			conv.c[0] = vm->pbMemory[a+0];
			conv.c[1] = vm->pbMemory[a+1];
			printf("wrote short data - %" PRIu32 " @ address 0x%x\n", conv.us, a);
			DISPATCH();
		
		exec_wrtb:;	// writes a byte to memory. First operand is the memory address as 32-bit number, second is the byte of data.
			conv.ui = _tagha_get_imm4(vm);
			if( vm->bSafeMode and conv.ui >= MEM_SIZE ) {
				printf("exec_wrtb reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, conv.ui);
				goto *dispatch[halt];
			}
			vm->pbMemory[conv.ui] = vm->pInstrStream[++vm->ip];
			printf("wrote byte data - %" PRIu32 " @ address 0x%x\n", vm->pbMemory[conv.ui], conv.ui);
			DISPATCH();
		
		exec_storeq:;
			a = _tagha_get_imm4(vm);
			if( vm->bSafeMode and a >= MEM_SIZE-8 ) {
				printf("exec_storeq reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, a);
				goto *dispatch[halt];
			}
			else if( vm->bSafeMode and (vm->sp-8) >= STK_SIZE ) {
				printf("exec_storeq reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-8);
				goto *dispatch[halt];
			}
			vm->pbMemory[a+7] = vm->pbStack[vm->sp--];
			vm->pbMemory[a+6] = vm->pbStack[vm->sp--];
			vm->pbMemory[a+5] = vm->pbStack[vm->sp--];
			vm->pbMemory[a+4] = vm->pbStack[vm->sp--];
			vm->pbMemory[a+3] = vm->pbStack[vm->sp--];
			vm->pbMemory[a+2] = vm->pbStack[vm->sp--];
			vm->pbMemory[a+1] = vm->pbStack[vm->sp--];
			vm->pbMemory[a+0] = vm->pbStack[vm->sp--];
			conv.c[0] = vm->pbMemory[a+0];
			conv.c[1] = vm->pbMemory[a+1];
			conv.c[2] = vm->pbMemory[a+2];
			conv.c[3] = vm->pbMemory[a+3];
			conv.c[4] = vm->pbMemory[a+4];
			conv.c[5] = vm->pbMemory[a+5];
			conv.c[6] = vm->pbMemory[a+6];
			conv.c[7] = vm->pbMemory[a+7];
			printf("stored long data - %" PRIu64 " @ address 0x%x\n", conv.ull, a);
			DISPATCH();
		
		exec_storel:;	// pops 4-byte value off stack and into a memory address.
			a = _tagha_get_imm4(vm);
			if( vm->bSafeMode and a >= MEM_SIZE-4 ) {
				printf("exec_storel reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, a);
				goto *dispatch[halt];
			}
			else if( vm->bSafeMode and (vm->sp-4) >= STK_SIZE ) {
				printf("exec_storel reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-4);
				goto *dispatch[halt];
			}
			vm->pbMemory[a+3] = vm->pbStack[vm->sp--];
			vm->pbMemory[a+2] = vm->pbStack[vm->sp--];
			vm->pbMemory[a+1] = vm->pbStack[vm->sp--];
			vm->pbMemory[a] = vm->pbStack[vm->sp--];
			conv.c[0] = vm->pbMemory[a+0];
			conv.c[1] = vm->pbMemory[a+1];
			conv.c[2] = vm->pbMemory[a+2];
			conv.c[3] = vm->pbMemory[a+3];
			printf("stored int data - %" PRIu32 " @ address 0x%x\n", conv.ui, a);
			DISPATCH();
		
		exec_stores:;	// pops 2-byte value off stack and into a memory address.
			a = _tagha_get_imm4(vm);
			if( vm->bSafeMode and a > MEM_SIZE-2 ) {
				printf("exec_stores reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, a);
				goto *dispatch[halt];
			}
			else if( vm->bSafeMode and (vm->sp-2) >= STK_SIZE ) {
				printf("exec_stores reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-2);
				goto *dispatch[halt];
			}
			vm->pbMemory[a+1] = vm->pbStack[vm->sp--];
			vm->pbMemory[a+0] = vm->pbStack[vm->sp--];
			conv.c[0] = vm->pbMemory[a+0];
			conv.c[1] = vm->pbMemory[a+1];
			printf("stored short data - %" PRIu32 " @ address 0x%x\n", conv.us, a);
			DISPATCH();
		
		exec_storeb:;	// pops byte value off stack and into a memory address.
			a = _tagha_get_imm4(vm);
			if( vm->bSafeMode and a >= MEM_SIZE ) {
				printf("exec_storeb reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, a);
				goto *dispatch[halt];
			}
			else if( vm->bSafeMode and (vm->sp-1) >= STK_SIZE ) {
				printf("exec_storeb reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-1);
				goto *dispatch[halt];
			}
			vm->pbMemory[a] = vm->pbStack[vm->sp--];
			printf("stored byte data - %" PRIu32 " @ address 0x%x\n", vm->pbMemory[a], a);
			DISPATCH();
		
		exec_storeqa:;
			a = _tagha_pop_int32(vm);
			if( vm->bSafeMode and a > MEM_SIZE-8 ) {
				printf("exec_storeqa reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, a);
				goto *dispatch[halt];
			}
			else if( vm->bSafeMode and (vm->sp-8) >= STK_SIZE ) {
				printf("exec_storeqa reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-8);
				goto *dispatch[halt];
			}
			vm->pbMemory[a+7] = vm->pbStack[vm->sp--];
			vm->pbMemory[a+6] = vm->pbStack[vm->sp--];
			vm->pbMemory[a+5] = vm->pbStack[vm->sp--];
			vm->pbMemory[a+4] = vm->pbStack[vm->sp--];
			vm->pbMemory[a+3] = vm->pbStack[vm->sp--];
			vm->pbMemory[a+2] = vm->pbStack[vm->sp--];
			vm->pbMemory[a+1] = vm->pbStack[vm->sp--];
			vm->pbMemory[a+0] = vm->pbStack[vm->sp--];
			conv.c[0] = vm->pbMemory[a+0];
			conv.c[1] = vm->pbMemory[a+1];
			conv.c[2] = vm->pbMemory[a+2];
			conv.c[3] = vm->pbMemory[a+3];
			conv.c[4] = vm->pbMemory[a+4];
			conv.c[5] = vm->pbMemory[a+5];
			conv.c[6] = vm->pbMemory[a+6];
			conv.c[7] = vm->pbMemory[a+7];
			printf("stored 4 byte data - %" PRIu32 " to pointer address 0x%x\n", conv.ui, a);
			DISPATCH();
		/*
		 * pushl <value to store>
		 * loadl <ptr address>
		 * storela
		*/
		exec_storela:;
			a = _tagha_pop_int32(vm);
			if( vm->bSafeMode and a > MEM_SIZE-4 ) {
				printf("exec_storela reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, a);
				goto *dispatch[halt];
			}
			else if( vm->bSafeMode and (vm->sp-4) >= STK_SIZE ) {
				printf("exec_storela reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-4);
				goto *dispatch[halt];
			}
			vm->pbMemory[a+3] = vm->pbStack[vm->sp--];
			vm->pbMemory[a+2] = vm->pbStack[vm->sp--];
			vm->pbMemory[a+1] = vm->pbStack[vm->sp--];
			vm->pbMemory[a+0] = vm->pbStack[vm->sp--];
			conv.c[0] = vm->pbMemory[a+0];
			conv.c[1] = vm->pbMemory[a+1];
			conv.c[2] = vm->pbMemory[a+2];
			conv.c[3] = vm->pbMemory[a+3];
			printf("stored 4 byte data - %" PRIu32 " to pointer address 0x%x\n", conv.ui, a);
			DISPATCH();
		
		exec_storesa:;
			a = _tagha_pop_int32(vm);
			if( vm->bSafeMode and a > MEM_SIZE-2 ) {
				printf("exec_storesa reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, a);
				goto *dispatch[halt];
			}
			else if( vm->bSafeMode and (vm->sp-2) >= STK_SIZE ) {
				printf("exec_storesa reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-2);
				goto *dispatch[halt];
			}
			vm->pbMemory[a+1] = vm->pbStack[vm->sp--];
			vm->pbMemory[a+0] = vm->pbStack[vm->sp--];
			conv.c[0] = vm->pbMemory[a+0];
			conv.c[1] = vm->pbMemory[a+1];
			printf("stored 2 byte data - %" PRIu32 " to pointer address 0x%x\n", conv.us, a);
			DISPATCH();
		
		exec_storeba:;
			a = _tagha_pop_int32(vm);
			if( vm->bSafeMode and a >= MEM_SIZE ) {
				printf("exec_storeba reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, a);
				goto *dispatch[halt];
			}
			else if( vm->bSafeMode and (vm->sp-1) >= STK_SIZE ) {
				printf("exec_storeba reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-1);
				goto *dispatch[halt];
			}
			vm->pbMemory[a] = vm->pbStack[vm->sp--];
			printf("stored byte - %" PRIu32 " to pointer address 0x%x\n", vm->pbMemory[a], a);
			DISPATCH();
		
		exec_loadq:;
			a = _tagha_get_imm4(vm);
			if( vm->bSafeMode and a > MEM_SIZE-8 ) {
				printf("exec_loadq reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, a);
				goto *dispatch[halt];
			}
			else if( vm->bSafeMode and (vm->sp+8) >= STK_SIZE ) {
				printf("exec_loadq reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp+8);
				goto *dispatch[halt];
			}
			vm->pbStack[++vm->sp] = vm->pbMemory[a+0];
			vm->pbStack[++vm->sp] = vm->pbMemory[a+1];
			vm->pbStack[++vm->sp] = vm->pbMemory[a+2];
			vm->pbStack[++vm->sp] = vm->pbMemory[a+3];
			vm->pbStack[++vm->sp] = vm->pbMemory[a+4];
			vm->pbStack[++vm->sp] = vm->pbMemory[a+5];
			vm->pbStack[++vm->sp] = vm->pbMemory[a+6];
			vm->pbStack[++vm->sp] = vm->pbMemory[a+7];
			conv.c[7] = vm->pbMemory[a+7];
			conv.c[6] = vm->pbMemory[a+6];
			conv.c[5] = vm->pbMemory[a+5];
			conv.c[4] = vm->pbMemory[a+4];
			conv.c[3] = vm->pbMemory[a+3];
			conv.c[2] = vm->pbMemory[a+2];
			conv.c[1] = vm->pbMemory[a+1];
			conv.c[0] = vm->pbMemory[a+0];
			printf("loaded long data to T.O.S. - %" PRIu64 " from address 0x%x\n", conv.ull, a);
			DISPATCH();
		
		exec_loadl:;
			a = _tagha_get_imm4(vm);
			if( vm->bSafeMode and a > MEM_SIZE-4 ) {
				printf("exec_loadl reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, a);
				goto *dispatch[halt];
			}
			else if( vm->bSafeMode and (vm->sp+4) >= STK_SIZE ) {
				printf("exec_loadl reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp+4);
				goto *dispatch[halt];
			}
			vm->pbStack[++vm->sp] = vm->pbMemory[a+0];
			vm->pbStack[++vm->sp] = vm->pbMemory[a+1];
			vm->pbStack[++vm->sp] = vm->pbMemory[a+2];
			vm->pbStack[++vm->sp] = vm->pbMemory[a+3];
			conv.c[3] = vm->pbMemory[a+3];
			conv.c[2] = vm->pbMemory[a+2];
			conv.c[1] = vm->pbMemory[a+1];
			conv.c[0] = vm->pbMemory[a+0];
			printf("loaded int data to T.O.S. - %" PRIu32 " from address 0x%x\n", conv.ui, a);
			DISPATCH();
		
		exec_loads:;
			a = _tagha_get_imm4(vm);
			if( vm->bSafeMode and a > MEM_SIZE-2 ) {
				printf("exec_loads reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, a);
				goto *dispatch[halt];
			}
			else if( vm->bSafeMode and (vm->sp+2) >= STK_SIZE ) {
				printf("exec_loads reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp+2);
				goto *dispatch[halt];
			}
			vm->pbStack[++vm->sp] = vm->pbMemory[a+0];
			vm->pbStack[++vm->sp] = vm->pbMemory[a+1];
			conv.c[1] = vm->pbMemory[a+1];
			conv.c[0] = vm->pbMemory[a+0];
			printf("loaded short data to T.O.S. - %" PRIu32 " from address 0x%x\n", conv.us, a);
			DISPATCH();
		
		exec_loadb:;
			a = _tagha_get_imm4(vm);
			if( vm->bSafeMode and a >= MEM_SIZE ) {
				printf("exec_loadb reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, a);
				goto *dispatch[halt];
			}
			else if( vm->bSafeMode and (vm->sp+1) >= STK_SIZE ) {
				printf("exec_loadb reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp+1);
				goto *dispatch[halt];
			}
			vm->pbStack[++vm->sp] = vm->pbMemory[a];
			printf("loaded byte data to T.O.S. - %" PRIu32 " from address 0x%x\n", vm->pbStack[vm->sp], a);
			DISPATCH();
		
		exec_loadqa:;
			a = _tagha_pop_int32(vm);
			if( vm->bSafeMode and a > MEM_SIZE-8 ) {
				printf("exec_loadqa reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, a);
				goto *dispatch[halt];
			}
			else if( vm->bSafeMode and (vm->sp+8) >= STK_SIZE ) {
				printf("exec_loadqa reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp+8);
				goto *dispatch[halt];
			}
			vm->pbStack[++vm->sp] = vm->pbMemory[a+0];
			vm->pbStack[++vm->sp] = vm->pbMemory[a+1];
			vm->pbStack[++vm->sp] = vm->pbMemory[a+2];
			vm->pbStack[++vm->sp] = vm->pbMemory[a+3];
			vm->pbStack[++vm->sp] = vm->pbMemory[a+4];
			vm->pbStack[++vm->sp] = vm->pbMemory[a+5];
			vm->pbStack[++vm->sp] = vm->pbMemory[a+6];
			vm->pbStack[++vm->sp] = vm->pbMemory[a+7];
			conv.c[0] = vm->pbMemory[a+7];
			conv.c[1] = vm->pbMemory[a+6];
			conv.c[2] = vm->pbMemory[a+5];
			conv.c[3] = vm->pbMemory[a+4];
			conv.c[4] = vm->pbMemory[a+3];
			conv.c[5] = vm->pbMemory[a+2];
			conv.c[6] = vm->pbMemory[a+1];
			conv.c[7] = vm->pbMemory[a+0];
			printf("loaded 8 byte data to T.O.S. - %" PRIu64 " from pointer address 0x%x\n", conv.ull, a);
			DISPATCH();
		
		exec_loadla:;
			a = _tagha_pop_int32(vm);
			if( vm->bSafeMode and a > MEM_SIZE-4 ) {
				printf("exec_loadla reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, a);
				goto *dispatch[halt];
			}
			else if( vm->bSafeMode and (vm->sp+4) >= STK_SIZE ) {
				printf("exec_loadla reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp+4);
				goto *dispatch[halt];
			}
			vm->pbStack[++vm->sp] = vm->pbMemory[a+0];
			vm->pbStack[++vm->sp] = vm->pbMemory[a+1];
			vm->pbStack[++vm->sp] = vm->pbMemory[a+2];
			vm->pbStack[++vm->sp] = vm->pbMemory[a+3];
			conv.c[0] = vm->pbMemory[a+3];
			conv.c[1] = vm->pbMemory[a+2];
			conv.c[2] = vm->pbMemory[a+1];
			conv.c[3] = vm->pbMemory[a+0];
			printf("loaded 4 byte data to T.O.S. - %" PRIu32 " from pointer address 0x%x\n", conv.ui, a);
			DISPATCH();
		
		exec_loadsa:;
			a = _tagha_pop_int32(vm);
			if( vm->bSafeMode and a > MEM_SIZE-2 ) {
				printf("exec_loadsa reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, a);
				goto *dispatch[halt];
			}
			else if( vm->bSafeMode and (vm->sp+2) >= STK_SIZE ) {
				printf("exec_loadsa reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp+2);
				goto *dispatch[halt];
			}
			vm->pbStack[++vm->sp] = vm->pbMemory[a+0];
			vm->pbStack[++vm->sp] = vm->pbMemory[a+1];
			conv.c[0] = vm->pbMemory[a+0];
			conv.c[1] = vm->pbMemory[a+1];
			printf("loaded 2 byte data to T.O.S. - %" PRIu32 " from pointer address 0x%x\n", conv.us, a);
			DISPATCH();
		
		exec_loadba:;
			a = _tagha_pop_int32(vm);
			if( vm->bSafeMode and a > MEM_SIZE ) {
				printf("exec_loadba reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, a);
				goto *dispatch[halt];
			}
			else if( vm->bSafeMode and (vm->sp+1) >= STK_SIZE ) {
				printf("exec_loadba reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp+1);
				goto *dispatch[halt];
			}
			vm->pbStack[++vm->sp] = vm->pbMemory[a];
			printf("loaded byte data to T.O.S. - %" PRIu32 " from pointer address 0x%x\n", vm->pbStack[vm->sp], a);
			DISPATCH();
		
		exec_loadspq:;
			a = _tagha_pop_int32(vm);
			if( vm->bSafeMode and (a-7) >= STK_SIZE ) {
				printf("exec_loadspq reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, a-7);
				goto *dispatch[halt];
			}
			conv.c[7] = vm->pbStack[a-0];
			conv.c[6] = vm->pbStack[a-1];
			conv.c[5] = vm->pbStack[a-2];
			conv.c[4] = vm->pbStack[a-3];
			conv.c[3] = vm->pbStack[a-4];
			conv.c[2] = vm->pbStack[a-5];
			conv.c[1] = vm->pbStack[a-6];
			conv.c[0] = vm->pbStack[a-7];
			_tagha_push_int64(vm, conv.ull);
			printf("loaded 8-byte SP address data to T.O.S. - %" PRIu64 " from sp address 0x%x\n", conv.ull, a);
			DISPATCH();
		
		exec_loadspl:;
			a = _tagha_pop_int32(vm);
			if( vm->bSafeMode and (a-3) >= STK_SIZE ) {
				printf("exec_loadspl reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, a-3);
				goto *dispatch[halt];
			}
			conv.c[3] = vm->pbStack[a];
			conv.c[2] = vm->pbStack[a-1];
			conv.c[1] = vm->pbStack[a-2];
			conv.c[0] = vm->pbStack[a-3];
			_tagha_push_int32(vm, conv.ui);
			printf("loaded 4-byte SP address data to T.O.S. - %" PRIu32 " from sp address 0x%x\n", conv.ui, a);
			DISPATCH();
		
		exec_loadsps:;
			a = _tagha_pop_int32(vm);
			if( vm->bSafeMode and (a-1) >= STK_SIZE ) {
				printf("exec_loadsps reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, a-1);
				goto *dispatch[halt];
			}
			conv.c[1] = vm->pbStack[a];
			conv.c[0] = vm->pbStack[a-1];
			_tagha_push_short(vm, conv.us);
			printf("loaded 2-byte SP address data to T.O.S. - %" PRIu32 " from sp address 0x%x\n", conv.us, a);
			DISPATCH();
		
		exec_loadspb:;
			a = _tagha_pop_int32(vm);
			if( vm->bSafeMode and a >= STK_SIZE ) {
				printf("exec_loadspb reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, a);
				goto *dispatch[halt];
			}
			conv.c[0] = vm->pbStack[a];
			_tagha_push_byte(vm, conv.c[0]);
			printf("loaded byte SP address data to T.O.S. - %" PRIu32 " from sp address 0x%x\n", conv.c[0], a);
			DISPATCH();
		
		exec_storespq:;
			a = _tagha_pop_int32(vm);
			if( vm->bSafeMode and a-7 >= STK_SIZE ) {
				printf("exec_storespq reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, a-7);
				goto *dispatch[halt];
			}
			conv.ull = _tagha_pop_int64(vm);
			vm->pbStack[a-0] = conv.c[7];
			vm->pbStack[a-1] = conv.c[6];
			vm->pbStack[a-2] = conv.c[5];
			vm->pbStack[a-3] = conv.c[4];
			vm->pbStack[a-4] = conv.c[3];
			vm->pbStack[a-5] = conv.c[2];
			vm->pbStack[a-6] = conv.c[1];
			vm->pbStack[a-7] = conv.c[0];
			printf("stored 8-byte data from T.O.S. - %" PRIu64 " to sp address 0x%x\n", conv.ull, a);
			DISPATCH();
		
		exec_storespl:;		// store TOS into another part of the data stack.
			a = _tagha_pop_int32(vm);
			if( vm->bSafeMode and a-3 >= STK_SIZE ) {
				printf("exec_storespl reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, a-3);
				goto *dispatch[halt];
			}
			conv.ui = _tagha_pop_int32(vm);
			vm->pbStack[a] = conv.c[3];
			vm->pbStack[a-1] = conv.c[2];
			vm->pbStack[a-2] = conv.c[1];
			vm->pbStack[a-3] = conv.c[0];
			printf("stored 4-byte data from T.O.S. - %" PRIu32 " to sp address 0x%x\n", conv.ui, a);
			DISPATCH();
		
		exec_storesps:;
			a = _tagha_pop_int32(vm);
			if( vm->bSafeMode and a-1 >= STK_SIZE ) {
				printf("exec_storesps reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, a-1);
				goto *dispatch[halt];
			}
			conv.us = _tagha_pop_short(vm);
			vm->pbStack[a] = conv.c[1];
			vm->pbStack[a-1] = conv.c[0];
			printf("stored 2-byte data from T.O.S. - %" PRIu32 " to sp address 0x%x\n", conv.us, a);
			DISPATCH();
		
		exec_storespb:;
			a = _tagha_pop_int32(vm);
			if( vm->bSafeMode and a >= STK_SIZE ) {
				printf("exec_storespb reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, a);
				goto *dispatch[halt];
			}
			vm->pbStack[a] = _tagha_pop_byte(vm);
			printf("stored byte data from T.O.S. - %" PRIu32 " to sp address 0x%x\n", vm->pbStack[a], a);
			DISPATCH();
		
		exec_copyq:;
			if( vm->bSafeMode and vm->sp-7 >= STK_SIZE ) {
				printf("exec_copyq reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-7);
				goto *dispatch[halt];
			}
			conv.c[0] = vm->pbStack[vm->sp-7];
			conv.c[1] = vm->pbStack[vm->sp-6];
			conv.c[2] = vm->pbStack[vm->sp-5];
			conv.c[3] = vm->pbStack[vm->sp-4];
			conv.c[4] = vm->pbStack[vm->sp-3];
			conv.c[5] = vm->pbStack[vm->sp-2];
			conv.c[6] = vm->pbStack[vm->sp-1];
			conv.c[7] = vm->pbStack[vm->sp-0];
			printf("copied long data from T.O.S. - %" PRIu64 "\n", conv.ull);
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
		
		exec_copyl:;	// copy 4 bytes of top of stack and put as new top of stack.
			if( vm->bSafeMode and vm->sp-3 >= STK_SIZE ) {
				printf("exec_copyl reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-3);
				goto *dispatch[halt];
			}
			conv.c[0] = vm->pbStack[vm->sp-3];
			conv.c[1] = vm->pbStack[vm->sp-2];
			conv.c[2] = vm->pbStack[vm->sp-1];
			conv.c[3] = vm->pbStack[vm->sp];
			printf("copied int data from T.O.S. - %" PRIu32 "\n", conv.ui);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_copys:;
			if( vm->bSafeMode and vm->sp-1 >= STK_SIZE ) {
				printf("exec_copys reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-1);
				goto *dispatch[halt];
			}
			else if( vm->bSafeMode and vm->sp+2 >= STK_SIZE ) {
				printf("exec_copys reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp+2);
				goto *dispatch[halt];
			}
			conv.c[0] = vm->pbStack[vm->sp-1];
			conv.c[1] = vm->pbStack[vm->sp];
			_tagha_push_short(vm, conv.us);
			//vm->pbStack[++vm->sp] = conv.c[0];
			//vm->pbStack[++vm->sp] = conv.c[1];
			printf("copied short data from T.O.S. - %" PRIu32 "\n", conv.us);
			DISPATCH();
		
		exec_copyb:;
			//conv.c[0] = vm->pbStack[vm->sp];
			_tagha_push_byte(vm, vm->pbStack[vm->sp]);
			//vm->pbStack[++vm->sp] = conv.c[0];
			//printf("copied byte data from T.O.S. - %" PRIu32 "\n", conv.c[0]);
			DISPATCH();
		
		exec_addq:;
			qb = _tagha_pop_int64(vm);
			qa = _tagha_pop_int64(vm);
			conv.ll = (i64)qa + (i64)qb;
			printf("signed 8 byte addition result: %" PRIi64 " == %" PRIi64 " + %" PRIi64 "\n", conv.ll, qa,qb);
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
			
		exec_uaddq:;
			qb = _tagha_pop_int64(vm);
			qa = _tagha_pop_int64(vm);
			conv.ull = qa+qb;
			printf("unsigned 8 byte addition result: %" PRIu64 " == %" PRIu64 " + %" PRIu64 "\n", conv.ull, qa,qb);
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
		
		exec_addl:;		// pop 8 bytes, signed addition, and push 4 byte result to top of stack
			b = _tagha_pop_int32(vm);
			a = _tagha_pop_int32(vm);
			conv.i = (int)a + (int)b;
			printf("signed 4 byte addition result: %" PRIi32 " == %" PRIi32 " + %" PRIi32 "\n", conv.i, a,b);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_uaddl:;	// In C, all integers in an expression are promoted to int32, if number is bigger then uint32 or int64
			b = _tagha_pop_int32(vm);
			a = _tagha_pop_int32(vm);
			conv.ui = a+b;
			printf("unsigned 4 byte addition result: %" PRIu32 " == %" PRIu32 " + %" PRIu32 "\n", conv.ui, a,b);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_addf:;
			fb = _tagha_pop_float32(vm);
			fa = _tagha_pop_float32(vm);
			conv.f = fa+fb;
			printf("4-byte float addition result: %f == %f + %f\n", conv.f, fa,fb);
			_tagha_push_float32(vm, conv.f);
			DISPATCH();
		
		exec_addf64:;
			db = _tagha_pop_float64(vm);
			da = _tagha_pop_float64(vm);
			conv.dbl = da+db;
			printf("8-byte float addition result: %f == %f + %f\n", conv.dbl, da,db);
			_tagha_push_float64(vm, conv.dbl);
			DISPATCH();
		
		exec_subq:;
			qb = _tagha_pop_int64(vm);
			qa = _tagha_pop_int64(vm);
			conv.ll = (i64)qa - (i64)qb;
			printf("signed 8 byte subtraction result: %" PRIi64 " == %" PRIi64 " - %" PRIi64 "\n", conv.ll, qa,qb);
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
			
		exec_usubq:;
			qb = _tagha_pop_int64(vm);
			qa = _tagha_pop_int64(vm);
			conv.ull = qa-qb;
			printf("unsigned 8 byte subtraction result: %" PRIu64 " == %" PRIu64 " - %" PRIu64 "\n", conv.ull, qa,qb);
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
		
		exec_subl:;
			b = _tagha_pop_int32(vm);
			a = _tagha_pop_int32(vm);
			conv.i = (int)a - (int)b;
			printf("signed 4 byte subtraction result: %" PRIi32 " == %" PRIi32 " - %" PRIi32 "\n", conv.ui, a,b);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_usubl:;
			b = _tagha_pop_int32(vm);
			a = _tagha_pop_int32(vm);
			conv.ui = a-b;
			printf("unsigned 4 byte subtraction result: %" PRIu32 " == %" PRIu32 " - %" PRIu32 "\n", conv.ui, a,b);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_subf:;
			fb = _tagha_pop_float32(vm);
			fa = _tagha_pop_float32(vm);
			conv.f = fa-fb;
			printf("4-byte float subtraction result: %f == %f - %f\n", conv.f, fa,fb);
			_tagha_push_float32(vm, conv.f);
			DISPATCH();
			
		exec_subf64:;
			db = _tagha_pop_float64(vm);
			da = _tagha_pop_float64(vm);
			conv.dbl = da-db;
			printf("8-byte float subtraction result: %f == %f - %f\n", conv.dbl, da,db);
			_tagha_push_float64(vm, conv.dbl);
			DISPATCH();
		
		exec_mulq:;
			qb = _tagha_pop_int64(vm);
			qa = _tagha_pop_int64(vm);
			conv.ll = (i64)qa * (i64)qb;
			printf("signed 8 byte mult result: %" PRIi64 " == %" PRIi64 " * %" PRIi64 "\n", conv.ll, qa,qb);
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
			
		exec_umulq:;
			qb = _tagha_pop_int64(vm);
			qa = _tagha_pop_int64(vm);
			conv.ull = qa*qb;
			printf("unsigned 8 byte mult result: %" PRIu64 " == %" PRIu64 " * %" PRIu64 "\n", conv.ull, qa,qb);
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
		
		exec_mull:;
			b = _tagha_pop_int32(vm);
			a = _tagha_pop_int32(vm);
			conv.i = (int)a * (int)b;
			printf("signed 4 byte mult result: %" PRIi32 " == %" PRIi32 " * %" PRIi32 "\n", conv.i, a,b);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_umull:;
			b = _tagha_pop_int32(vm);
			a = _tagha_pop_int32(vm);
			conv.ui = a*b;
			printf("unsigned 4 byte mult result: %" PRIu32 " == %" PRIu32 " * %" PRIu32 "\n", conv.ui, a,b);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
			
		exec_mulf:;
			fb = _tagha_pop_float32(vm);
			fa = _tagha_pop_float32(vm);
			conv.f = fa*fb;
			printf("4-byte float mult result: %f == %f * %f\n", conv.f, fa,fb);
			_tagha_push_float32(vm, conv.f);
			DISPATCH();
			
		exec_mulf64:;
			db = _tagha_pop_float64(vm);
			da = _tagha_pop_float64(vm);
			conv.dbl = da*db;
			printf("8-byte float mult result: %f == %f * %f\n", conv.dbl, da,db);
			_tagha_push_float64(vm, conv.dbl);
			DISPATCH();
			
		exec_divq:;
			qb = _tagha_pop_int64(vm);
			if( !qb ) {
				printf("divq: divide by 0 error.\n");
				goto *dispatch[halt];
			}
			qa = _tagha_pop_int64(vm);
			conv.ll = (i64)qa / (i64)qb;
			printf("signed 8 byte division result: %" PRIi64 " == %" PRIi64 " / %" PRIi64 "\n", conv.ll, qa,qb);
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
			
		exec_udivq:;
			qb = _tagha_pop_int64(vm);
			if( !qb ) {
				printf("udivq: divide by 0 error.\n");
				goto *dispatch[halt];
			}
			qa = _tagha_pop_int64(vm);
			conv.ull = qa/qb;
			printf("unsigned 8 byte division result: %" PRIu64 " == %" PRIu64 " / %" PRIu64 "\n", conv.ull, qa,qb);
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
		
		exec_divl:;
			b = _tagha_pop_int32(vm);
			if( !b ) {
				printf("divl: divide by 0 error.\n");
				goto *dispatch[halt];
			}
			a = _tagha_pop_int32(vm);
			conv.i = (int)a / (int)b;
			printf("signed 4 byte division result: %" PRIi32 " == %" PRIi32 " / %" PRIi32 "\n", conv.i, a,b);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_udivl:;
			b = _tagha_pop_int32(vm);
			if( !b ) {
				printf("udivl: divide by 0 error.\n");
				goto *dispatch[halt];
			}
			a = _tagha_pop_int32(vm);
			conv.ui = a/b;
			printf("unsigned 4 byte division result: %" PRIu32 " == %" PRIu32 " / %" PRIu32 "\n", conv.ui, a,b);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_divf:;
			fb = _tagha_pop_float32(vm);
			if( !fb ) {
				printf("divf: divide by 0.0 error.\n");
				goto *dispatch[halt];
			}
			fa = _tagha_pop_float32(vm);
			conv.f = fa/fb;
			printf("4-byte float division result: %f == %f / %f\n", conv.f, fa,fb);
			_tagha_push_float32(vm, conv.f);
			DISPATCH();
			
		exec_divf64:;
			db = _tagha_pop_float64(vm);
			da = _tagha_pop_float64(vm);
			conv.dbl = da/db;
			printf("8-byte float division result: %f == %f / %f\n", conv.dbl, da,db);
			_tagha_push_float64(vm, conv.dbl);
			DISPATCH();
		
		exec_modq:;
			qb = _tagha_pop_int64(vm);
			if( !qb ) {
				printf("divq: divide by 0 error.\n");
				goto *dispatch[halt];
			}
			qa = _tagha_pop_int64(vm);
			conv.ll = (i64)qa % (i64)qb;
			printf("signed 8 byte modulo result: %" PRIi64 " == %" PRIi64 " %% %" PRIi64 "\n", conv.ll, qa,qb);
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
			
		exec_umodq:;
			qb = _tagha_pop_int64(vm);
			if( !qb ) {
				printf("udivq: divide by 0 error.\n");
				goto *dispatch[halt];
			}
			qa = _tagha_pop_int64(vm);
			conv.ull = qa % qb;
			printf("unsigned 8 byte division result: %" PRIu64 " == %" PRIu64 " %% %" PRIu64 "\n", conv.ull, qa,qb);
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
		
		exec_modl:;
			b = _tagha_pop_int32(vm);
			if( !b ) {
				printf("modl: divide by 0 error.\n");
				goto *dispatch[halt];
			}
			a = _tagha_pop_int32(vm);
			conv.i = (int)a % (int)b;
			printf("signed 4 byte modulo result: %" PRIi32 " == %" PRIi32 " %% %" PRIi32 "\n", conv.i, a,b);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_umodl:;
			b = _tagha_pop_int32(vm);
			if( !b ) {
				printf("umodl: divide by 0 error.\n");
				goto *dispatch[halt];
			}
			a = _tagha_pop_int32(vm);
			conv.ui = a % b;
			printf("unsigned 4 byte modulo result: %" PRIu32 " == %" PRIu32 " %% %" PRIu32 "\n", conv.ui, a,b);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_andq:;
			qb = _tagha_pop_int64(vm);
			qa = _tagha_pop_int64(vm);
			conv.ull = qa & qb;
			printf("8 byte AND result: %" PRIu64 " == %" PRIu64 " & %" PRIu64 "\n", conv.ull, qa,qb);
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
			
		exec_orq:;
			qb = _tagha_pop_int64(vm);
			qa = _tagha_pop_int64(vm);
			conv.ull = qa | qb;
			printf("8 byte OR result: %" PRIu64 " == %" PRIu64 " | %" PRIu64 "\n", conv.ull, qa,qb);
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
			
		exec_xorq:;
			qb = _tagha_pop_int64(vm);
			qa = _tagha_pop_int64(vm);
			conv.ull = qa ^ qb;
			printf("8 byte XOR result: %" PRIu64 " == %" PRIu64 " ^ %" PRIu64 "\n", conv.ull, qa,qb);
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
			
		exec_notq:;
			qa = _tagha_pop_int64(vm);
			conv.ull = ~qa;
			printf("8 byte NOT result: %" PRIu64 "\n", conv.ull);
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
			
		exec_shlq:;
			qb = _tagha_pop_int64(vm);
			qa = _tagha_pop_int64(vm);
			conv.ull = qa << qb;
			printf("8 byte Shift Left result: %" PRIu64 " == %" PRIu64 " << %" PRIu64 "\n", conv.ull, qa,qb);
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
			
		exec_shrq:;
			qb = _tagha_pop_int64(vm);
			qa = _tagha_pop_int64(vm);
			conv.ull = qa >> qb;
			printf("8 byte Shift Right result: %" PRIu64 " == %" PRIu64 " >> %" PRIu64 "\n", conv.ull, qa,qb);
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
			
		exec_andl:;
			b = _tagha_pop_int32(vm);
			a = _tagha_pop_int32(vm);
			conv.ui = a & b;
			printf("4 byte AND result: %" PRIu32 " == %" PRIu32 " & %" PRIu32 "\n", conv.ui, a,b);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_orl:;
			b = _tagha_pop_int32(vm);
			a = _tagha_pop_int32(vm);
			conv.ui = a | b;
			printf("4 byte OR result: %" PRIu32 " == %" PRIu32 " | %" PRIu32 "\n", conv.ui, a,b);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_xorl:;
			b = _tagha_pop_int32(vm);
			a = _tagha_pop_int32(vm);
			conv.ui = a ^ b;
			printf("4 byte XOR result: %" PRIu32 " == %" PRIu32 " ^ %" PRIu32 "\n", conv.ui, a,b);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_notl:;
			a = _tagha_pop_int32(vm);
			conv.ui = ~a;
			printf("4 byte NOT result: %" PRIu32 "\n", conv.ui);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_shll:;
			b = _tagha_pop_int32(vm);
			a = _tagha_pop_int32(vm);
			conv.ui = a << b;
			printf("4 byte Shift Left result: %" PRIu32 " == %" PRIu32 " << %" PRIu32 "\n", conv.ui, a,b);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_shrl:;
			b = _tagha_pop_int32(vm);
			a = _tagha_pop_int32(vm);
			conv.ui = a >> b;
			printf("4 byte Shift Right result: %" PRIu32 " == %" PRIu32 " >> %" PRIu32 "\n", conv.ui, a,b);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_incq:;
			qa = _tagha_pop_int64(vm);
			conv.ull = ++qa;
			printf("8 byte Increment result: %" PRIu64 "\n", conv.ull);
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
			
		exec_incl:;
			a = _tagha_pop_int32(vm);
			conv.ui = ++a;
			printf("4 byte Increment result: %" PRIu32 "\n", conv.ui);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_incf:;
			fa = _tagha_pop_float32(vm);
			conv.f = ++fa;
			printf("4-byte float Increment result: %f\n", conv.f);
			_tagha_push_float32(vm, conv.f);
			DISPATCH();
			
		exec_incf64:;
			da = _tagha_pop_float64(vm);
			conv.dbl = ++da;
			printf("8-byte float Increment result: %f\n", conv.dbl);
			_tagha_push_float64(vm, conv.dbl);
			DISPATCH();
		
		exec_decq:;
			qa = _tagha_pop_int64(vm);
			conv.ull = --qa;
			printf("8 byte Decrement result: %" PRIu64 "\n", conv.ull);
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
			
		exec_decl:;
			a = _tagha_pop_int32(vm);
			conv.ui = --a;
			printf("4 byte Decrement result: %" PRIu32 "\n", conv.ui);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_decf:;
			fa = _tagha_pop_float32(vm);
			conv.f = --fa;
			printf("4-byte float Decrement result: %f\n", conv.f);
			_tagha_push_float32(vm, conv.f);
			DISPATCH();
		
		exec_decf64:;
			da = _tagha_pop_float64(vm);
			conv.dbl = --da;
			printf("8-byte float Decrement result: %f\n", conv.dbl);
			_tagha_push_float64(vm, conv.dbl);
			DISPATCH();
		
		exec_negq:;
			qa = _tagha_pop_int64(vm);
			conv.ull = -qa;
			printf("8 byte Negate result: %" PRIu64 "\n", conv.ull);
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
		
		exec_negl:;
			a = _tagha_pop_int32(vm);
			conv.ui = -a;
			printf("4 byte Negate result: %" PRIu32 "\n", conv.ui);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
			
		exec_negf:;
			fa = _tagha_pop_float32(vm);
			conv.f = -fa;
			printf("4-byte float Negate result: %f\n", conv.f);
			_tagha_push_float32(vm, conv.f);
			DISPATCH();
			
		exec_negf64:;
			da = _tagha_pop_float64(vm);
			conv.dbl = -da;
			printf("8-byte float Negate result: %f\n", conv.dbl);
			_tagha_push_float64(vm, conv.dbl);
			DISPATCH();
		
		exec_ltq:;
			qb = _tagha_pop_int64(vm);
			qa = _tagha_pop_int64(vm);
			conv.ui = (i64)qa < (i64)qb;
			printf("signed 8 byte Less Than result: %" PRIu32 " == %" PRIi64 " < %" PRIi64 "\n", conv.ui, qa,qb);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_ultq:;
			qb = _tagha_pop_int64(vm);
			qa = _tagha_pop_int64(vm);
			conv.ui = qa < qb;
			printf("unsigned 8 byte Less Than result: %" PRIu32 " == %" PRIu64 " < %" PRIu64 "\n", conv.ui, qa,qb);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_ltl:;
			b = _tagha_pop_int32(vm);
			a = _tagha_pop_int32(vm);
			conv.ui = (int)a < (int)b;
			printf("4 byte Signed Less Than result: %" PRIu32 " == %" PRIi32 " < %" PRIi32 "\n", conv.ui, a,b);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_ultl:;
			b = _tagha_pop_int32(vm);
			a = _tagha_pop_int32(vm);
			conv.ui = a < b;
			printf("4 byte Unsigned Less Than result: %" PRIu32 " == %" PRIu32 " < %" PRIu32 "\n", conv.ui, a,b);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_ltf:;
			fb = _tagha_pop_float32(vm);
			fa = _tagha_pop_float32(vm);
			conv.ui = fa < fb;
			printf("4 byte Less Than Float result: %" PRIu32 " == %f < %f\n", conv.ui, fa,fb);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_ltf64:;
			db = _tagha_pop_float64(vm);
			da = _tagha_pop_float64(vm);
			conv.ui = da < db;
			printf("8 byte Less Than Float result: %" PRIu32 " == %f < %f\n", conv.ui, da,db);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_gtq:;
			qb = _tagha_pop_int64(vm);
			qa = _tagha_pop_int64(vm);
			conv.ui = (i64)qa > (i64)qb;
			printf("signed 8 byte Greater Than result: %" PRIu32 " == %" PRIi64 " > %" PRIi64 "\n", conv.ui, qa,qb);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
			
		exec_ugtq:;
			qb = _tagha_pop_int64(vm);
			qa = _tagha_pop_int64(vm);
			conv.ui = qa > qb;
			printf("unsigned 8 byte Greater Than result: %" PRIu32 " == %" PRIu64 " > %" PRIu64 "\n", conv.ui, qa,qb);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_gtl:;
			b = _tagha_pop_int32(vm);
			a = _tagha_pop_int32(vm);
			conv.ui = (int)a > (int)b;
			printf("4 byte Signed Greater Than result: %" PRIu32 " == %" PRIi32 " > %" PRIi32 "\n", conv.ui, a,b);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_ugtl:;
			b = _tagha_pop_int32(vm);
			a = _tagha_pop_int32(vm);
			conv.ui = a > b;
			printf("4 byte Signed Greater Than result: %" PRIu32 " == %" PRIu32 " > %" PRIu32 "\n", conv.ui, a,b);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_gtf:;
			fb = _tagha_pop_float32(vm);
			fa = _tagha_pop_float32(vm);
			conv.ui = fa > fb;
			printf("4 byte Greater Than Float result: %" PRIu32 " == %f > %f\n", conv.ui, fa,fb);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_gtf64:;
			db = _tagha_pop_float64(vm);
			da = _tagha_pop_float64(vm);
			conv.ui = da > db;
			printf("8 byte Greater Than Float result: %" PRIu32 " == %f > %f\n", conv.ui, da,db);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_cmpq:;
			qb = _tagha_pop_int64(vm);
			qa = _tagha_pop_int64(vm);
			conv.ui = (i64)qa == (i64)qb;
			printf("signed 8 byte Compare result: %" PRIu32 " == %" PRIi64 " == %" PRIi64 "\n", conv.ui, qa,qb);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_ucmpq:;
			qb = _tagha_pop_int64(vm);
			qa = _tagha_pop_int64(vm);
			conv.ui = qa == qb;
			printf("unsigned 8 byte Compare result: %" PRIu32 " == %" PRIu64 " == %" PRIu64 "\n", conv.ui, qa,qb);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_cmpl:;
			b = _tagha_pop_int32(vm);
			a = _tagha_pop_int32(vm);
			conv.ui = (int)a == (int)b;
			printf("4 byte Signed Compare result: %" PRIu32 " == %" PRIi32 " == %" PRIi32 "\n", conv.ui, a,b);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_ucmpl:;
			b = _tagha_pop_int32(vm);
			a = _tagha_pop_int32(vm);
			conv.ui = a == b;
			printf("4 byte Unsigned Compare result: %" PRIu32 " == %" PRIu32 " == %" PRIu32 "\n", conv.ui, a,b);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_compf:;
			fb = _tagha_pop_float32(vm);
			fa = _tagha_pop_float32(vm);
			conv.ui = fa == fb;
			printf("4 byte Compare Float result: %" PRIu32 " == %f == %f\n", conv.ui, fa,fb);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_cmpf64:;
			db = _tagha_pop_float64(vm);
			da = _tagha_pop_float64(vm);
			conv.ui = da == db;
			printf("8 byte Compare Float result: %" PRIu32 " == %f == %f\n", conv.ui, da,db);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_leqq:;
			qb = _tagha_pop_int64(vm);
			qa = _tagha_pop_int64(vm);
			conv.ui = (i64)qa <= (i64)qb;
			printf("8 byte Signed Less Equal result: %" PRIu32 " == %" PRIi64 " <= %" PRIi64 "\n", conv.ui, qa,qb);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_uleqq:;
			qb = _tagha_pop_int64(vm);
			qa = _tagha_pop_int64(vm);
			conv.ui = qa <= qb;
			printf("8 byte Unsigned Less Equal result: %" PRIu32 " == %" PRIu64 " <= %" PRIu64 "\n", conv.ui, qa,qb);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_leql:;
			b = _tagha_pop_int32(vm);
			a = _tagha_pop_int32(vm);
			conv.ui = (int)a <= (int)b;
			printf("4 byte Signed Less Equal result: %" PRIu32 " == %" PRIi32 " <= %" PRIi32 "\n", conv.ui, a,b);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_uleql:;
			b = _tagha_pop_int32(vm);
			a = _tagha_pop_int32(vm);
			conv.ui = a <= b;
			printf("4 byte Unsigned Less Equal result: %" PRIu32 " == %" PRIu32 " <= %" PRIu32 "\n", conv.ui, a,b);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_leqf:;
			fb = _tagha_pop_float32(vm);
			fa = _tagha_pop_float32(vm);
			conv.ui = fa <= fb;
			printf("4 byte Less Equal Float result: %" PRIu32 " == %f <= %f\n", conv.ui, fa, fb);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
			
		exec_leqf64:;
			db = _tagha_pop_float64(vm);
			da = _tagha_pop_float64(vm);
			conv.ui = da <= db;
			printf("8 byte Less Equal Float result: %" PRIu32 " == %f <= %f\n", conv.ui, da,db);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_geqq:;
			qb = _tagha_pop_int64(vm);
			qa = _tagha_pop_int64(vm);
			conv.ui = (i64)qa >= (i64)qb;
			printf("8 byte Signed Greater Equal result: %" PRIu32 " == %" PRIi64 " >= %" PRIi64 "\n", conv.ui, qa,qb);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
			
		exec_ugeqq:;
			qb = _tagha_pop_int64(vm);
			qa = _tagha_pop_int64(vm);
			conv.ui = qa >= qb;
			printf("8 byte Unsigned Greater Equal result: %" PRIu32 " == %" PRIu64 " >= %" PRIu64 "\n", conv.ui, qa,qb);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_geql:;
			b = _tagha_pop_int32(vm);
			a = _tagha_pop_int32(vm);
			conv.ui = (int)a >= (int)b;
			printf("4 byte Signed Greater Equal result: %" PRIu32 " == %" PRIi32 " >= %" PRIi32 "\n", conv.ui, a,b);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_ugeql:;
			b = _tagha_pop_int32(vm);
			a = _tagha_pop_int32(vm);
			conv.ui = a >= b;
			printf("4 byte Unsigned Greater Equal result: %" PRIu32 " == %" PRIu32 " >= %" PRIu32 "\n", conv.ui, a,b);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_geqf:;
			fb = _tagha_pop_float32(vm);
			fa = _tagha_pop_float32(vm);
			conv.ui = fa >= fb;
			printf("4 byte Greater Equal Float result: %" PRIu32 " == %f >= %f\n", conv.ui, fa, fb);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_geqf64:;
			db = _tagha_pop_float64(vm);
			da = _tagha_pop_float64(vm);
			conv.ui = da >= db;
			printf("8 byte Greater Equal Float result: %" PRIu32 " == %f >= %f\n", conv.ui, da,db);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_neqq:;
			qb = _tagha_pop_int64(vm);
			qa = _tagha_pop_int64(vm);
			conv.ui = (i64)qa != (i64)qb;
			printf("8 byte Signed Not Equal result: %" PRIu32 " == %" PRIi64 " != %" PRIi64 "\n", conv.ui, qa,qb);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
			
		exec_uneqq:;
			qb = _tagha_pop_int64(vm);
			qa = _tagha_pop_int64(vm);
			conv.ui = qa != qb;
			printf("8 byte Unsigned Not Equal result: %" PRIu32 " == %" PRIi64 " != %" PRIi64 "\n", conv.ui, qa,qb);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
			
		exec_neql:;
			b = _tagha_pop_int32(vm);
			a = _tagha_pop_int32(vm);
			conv.ui = (int)a != (int)b;
			printf("4 byte Signed Not Equal result: %" PRIu32 " == %" PRIi32 " != %" PRIi32 "\n", conv.ui, a,b);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
			
		exec_uneql:;
			b = _tagha_pop_int32(vm);
			a = _tagha_pop_int32(vm);
			conv.ui = a != b;
			printf("4 byte Unsigned Not Equal result: %" PRIu32 " == %" PRIu32 " != %" PRIu32 "\n", conv.ui, a,b);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_neqf:;
			fb = _tagha_pop_float32(vm);
			fa = _tagha_pop_float32(vm);
			conv.ui = fa != fb;
			printf("4 byte Not Equal Float result: %" PRIu32 " == %f != %f\n", conv.ui, fa, fb);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_neqf64:;
			db = _tagha_pop_float64(vm);
			da = _tagha_pop_float64(vm);
			conv.ui = da != db;
			printf("8 byte Not Equal Float result: %" PRIu32 " == %f != %f\n", conv.ui, da,db);
			_tagha_push_int32(vm, conv.ui);
			DISPATCH();
		
		exec_mmxaddl:;	// pops two 64-bit values and adds them as 4 integers.
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			conv.mmx_i[0] += convb.mmx_i[0];
			conv.mmx_i[1] += convb.mmx_i[1];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
		
		exec_mmxuaddl:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			conv.mmx_ui[0] += convb.mmx_ui[0];
			conv.mmx_ui[1] += convb.mmx_ui[1];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
		
		exec_mmxaddf:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			conv.mmx_f[0] += convb.mmx_f[0];
			conv.mmx_f[1] += convb.mmx_f[1];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
		
		exec_mmxadds:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			conv.mmx_s[0] += convb.mmx_s[0]; conv.mmx_s[1] += convb.mmx_s[1];
			conv.mmx_s[2] += convb.mmx_s[2]; conv.mmx_s[3] += convb.mmx_s[3];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
			
		exec_mmxuadds:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			conv.mmx_us[0] += convb.mmx_us[0]; conv.mmx_us[1] += convb.mmx_us[1];
			conv.mmx_us[2] += convb.mmx_us[2]; conv.mmx_us[3] += convb.mmx_us[3];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
			
		exec_mmxaddb:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			conv.mmx_c[0] += convb.mmx_c[0]; conv.mmx_c[1] += convb.mmx_c[1];
			conv.mmx_c[2] += convb.mmx_c[2]; conv.mmx_c[3] += convb.mmx_c[3];
			conv.mmx_c[4] += convb.mmx_c[4]; conv.mmx_c[5] += convb.mmx_c[5];
			conv.mmx_c[6] += convb.mmx_c[6]; conv.mmx_c[7] += convb.mmx_c[7];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
			
		exec_mmxuaddb:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			conv.c[0] += convb.c[0]; conv.c[1] += convb.c[1];
			conv.c[2] += convb.c[2]; conv.c[3] += convb.c[3];
			conv.c[4] += convb.c[4]; conv.c[5] += convb.c[5];
			conv.c[6] += convb.c[6]; conv.c[7] += convb.c[7];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
		
		exec_mmxsubl:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			conv.mmx_i[0] -= convb.mmx_i[0];
			conv.mmx_i[1] -= convb.mmx_i[1];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
		
		exec_mmxusubl:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			conv.mmx_ui[0] -= convb.mmx_ui[0];
			conv.mmx_ui[1] -= convb.mmx_ui[1];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
		
		exec_mmxsubf:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			conv.mmx_f[0] -= convb.mmx_f[0];
			conv.mmx_f[1] -= convb.mmx_f[1];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
		
		exec_mmxsubs:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			conv.mmx_s[0] -= convb.mmx_s[0]; conv.mmx_s[1] -= convb.mmx_s[1];
			conv.mmx_s[2] -= convb.mmx_s[2]; conv.mmx_s[3] -= convb.mmx_s[3];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
		
		exec_mmxusubs:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			conv.mmx_us[0] -= convb.mmx_us[0]; conv.mmx_us[1] -= convb.mmx_us[1];
			conv.mmx_us[2] -= convb.mmx_us[2]; conv.mmx_us[3] -= convb.mmx_us[3];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
			
		exec_mmxsubb:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			conv.mmx_c[0] -= convb.mmx_c[0]; conv.mmx_c[1] -= convb.mmx_c[1];
			conv.mmx_c[2] -= convb.mmx_c[2]; conv.mmx_c[3] -= convb.mmx_c[3];
			conv.mmx_c[4] -= convb.mmx_c[4]; conv.mmx_c[5] -= convb.mmx_c[5];
			conv.mmx_c[6] -= convb.mmx_c[6]; conv.mmx_c[7] -= convb.mmx_c[7];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
			
		exec_mmxusubb:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			conv.c[0] -= convb.c[0]; conv.c[1] -= convb.c[1];
			conv.c[2] -= convb.c[2]; conv.c[3] -= convb.c[3];
			conv.c[4] -= convb.c[4]; conv.c[5] -= convb.c[5];
			conv.c[6] -= convb.c[6]; conv.c[7] -= convb.c[7];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
		
		exec_mmxmull:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			conv.mmx_i[0] *= convb.mmx_i[0];
			conv.mmx_i[1] *= convb.mmx_i[1];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
		
		exec_mmxumull:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			conv.mmx_ui[0] *= convb.mmx_ui[0];
			conv.mmx_ui[1] *= convb.mmx_ui[1];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
		
		exec_mmxmulf:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			conv.mmx_f[0] *= convb.mmx_f[0];
			conv.mmx_f[1] *= convb.mmx_f[1];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
			
		exec_mmxmuls:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			conv.mmx_s[0] *= convb.mmx_s[0]; conv.mmx_s[1] *= convb.mmx_s[1];
			conv.mmx_s[2] *= convb.mmx_s[2]; conv.mmx_s[3] *= convb.mmx_s[3];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
		
		exec_mmxumuls:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			conv.mmx_us[0] *= convb.mmx_us[0]; conv.mmx_us[1] *= convb.mmx_us[1];
			conv.mmx_us[2] *= convb.mmx_us[2]; conv.mmx_us[3] *= convb.mmx_us[3];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
			
		exec_mmxmulb:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			conv.mmx_c[0] *= convb.mmx_c[0]; conv.mmx_c[1] *= convb.mmx_c[1];
			conv.mmx_c[2] *= convb.mmx_c[2]; conv.mmx_c[3] *= convb.mmx_c[3];
			conv.mmx_c[4] *= convb.mmx_c[4]; conv.mmx_c[5] *= convb.mmx_c[5];
			conv.mmx_c[6] *= convb.mmx_c[6]; conv.mmx_c[7] *= convb.mmx_c[7];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
			
		exec_mmxumulb:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			conv.c[0] *= convb.c[0]; conv.c[1] *= convb.c[1];
			conv.c[2] *= convb.c[2]; conv.c[3] *= convb.c[3];
			conv.c[4] *= convb.c[4]; conv.c[5] *= convb.c[5];
			conv.c[6] *= convb.c[6]; conv.c[7] *= convb.c[7];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
		
		exec_mmxdivl:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			if( !convb.mmx_i[0] or !convb.mmx_i[1] ) {
				printf("exec_mmxdivl: divide by 0 error.\n");
				goto *dispatch[halt];
				//_tagha_push_int64(vm, conv.ull);
				//_tagha_push_int64(vm, convb.ull);
				//DISPATCH();
			}
			conv.mmx_i[0] /= convb.mmx_i[0];
			conv.mmx_i[1] /= convb.mmx_i[1];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
		
		exec_mmxudivl:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			if( !convb.mmx_ui[0] or !convb.mmx_ui[1] ) {
				printf("exec_mmxudivl: divide by 0 error.\n");
				goto *dispatch[halt];
			}
			conv.mmx_ui[0] /= convb.mmx_ui[0];
			conv.mmx_ui[1] /= convb.mmx_ui[1];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
		
		exec_mmxdivf:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			if( !convb.mmx_f[0] or !convb.mmx_f[1] ) {
				printf("exec_mmxdivf: divide by 0 error.\n");
				goto *dispatch[halt];
			}
			conv.mmx_f[0] /= convb.mmx_f[0];
			conv.mmx_f[1] /= convb.mmx_f[1];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
			
		exec_mmxdivs:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			if( !convb.mmx_s[0] or !convb.mmx_s[1] or !convb.mmx_s[2] or !convb.mmx_s[3] ) {
				printf("exec_mmxdivs: divide by 0 error.\n");
				goto *dispatch[halt];
			}
			conv.mmx_s[0] /= convb.mmx_s[0]; conv.mmx_s[1] /= convb.mmx_s[1];
			conv.mmx_s[2] /= convb.mmx_s[2]; conv.mmx_s[3] /= convb.mmx_s[3];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
		
		exec_mmxudivs:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			if( !convb.mmx_us[0] or !convb.mmx_us[1] or !convb.mmx_us[2] or !convb.mmx_us[3] ) {
				printf("exec_mmxudivs: divide by 0 error.\n");
				goto *dispatch[halt];
			}
			conv.mmx_us[0] /= convb.mmx_us[0]; conv.mmx_us[1] /= convb.mmx_us[1];
			conv.mmx_us[2] /= convb.mmx_us[2]; conv.mmx_us[3] /= convb.mmx_us[3];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
			
		exec_mmxdivb:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			if( !convb.mmx_c[0]
					or !convb.mmx_c[1]
					or !convb.mmx_c[2]
					or !convb.mmx_c[3]
					or !convb.mmx_c[4]
					or !convb.mmx_c[5]
					or !convb.mmx_c[6]
					or !convb.mmx_c[7] ) {
				printf("exec_mmxudivs: divide by 0 error.\n");
				goto *dispatch[halt];
			}
			conv.mmx_c[0] /= convb.mmx_c[0]; conv.mmx_c[1] /= convb.mmx_c[1];
			conv.mmx_c[2] /= convb.mmx_c[2]; conv.mmx_c[3] /= convb.mmx_c[3];
			conv.mmx_c[4] /= convb.mmx_c[4]; conv.mmx_c[5] /= convb.mmx_c[5];
			conv.mmx_c[6] /= convb.mmx_c[6]; conv.mmx_c[7] /= convb.mmx_c[7];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
			
		exec_mmxudivb:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			if( !convb.c[0]
					or !convb.c[1]
					or !convb.c[2]
					or !convb.c[3]
					or !convb.c[4]
					or !convb.c[5]
					or !convb.c[6]
					or !convb.c[7] ) {
				printf("exec_mmxudivs: divide by 0 error.\n");
				goto *dispatch[halt];
			}
			conv.c[0] /= convb.c[0]; conv.c[1] /= convb.c[1];
			conv.c[2] /= convb.c[2]; conv.c[3] /= convb.c[3];
			conv.c[4] /= convb.c[4]; conv.c[5] /= convb.c[5];
			conv.c[6] /= convb.c[6]; conv.c[7] /= convb.c[7];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
		
		exec_mmxmodl:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			if( !convb.mmx_i[0] or !convb.mmx_i[1] ) {
				printf("exec_mmxmodl: divide by 0 error.\n");
				goto *dispatch[halt];
			}
			conv.mmx_i[0] %= convb.mmx_i[0];
			conv.mmx_i[1] %= convb.mmx_i[1];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
		
		exec_mmxumodl:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			if( !convb.mmx_ui[0] or !convb.mmx_ui[1] ) {
				printf("exec_mmxumodl: divide by 0 error.\n");
				goto *dispatch[halt];
			}
			conv.mmx_ui[0] %= convb.mmx_ui[0];
			conv.mmx_ui[1] %= convb.mmx_ui[1];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
			
		exec_mmxmods:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			if( !convb.mmx_s[0] or !convb.mmx_s[1] or !convb.mmx_s[2] or !convb.mmx_s[3] ) {
				printf("exec_mmxmods: divide by 0 error.\n");
				goto *dispatch[halt];
			}
			conv.mmx_s[0] %= convb.mmx_s[0]; conv.mmx_s[1] %= convb.mmx_s[1];
			conv.mmx_s[2] %= convb.mmx_s[2]; conv.mmx_s[3] %= convb.mmx_s[3];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
		
		exec_mmxumods:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			if( !convb.mmx_us[0] or !convb.mmx_us[1] or !convb.mmx_us[2] or !convb.mmx_us[3] ) {
				printf("exec_mmxumods: divide by 0 error.\n");
				goto *dispatch[halt];
			}
			conv.mmx_us[0] %= convb.mmx_us[0]; conv.mmx_us[1] %= convb.mmx_us[1];
			conv.mmx_us[2] %= convb.mmx_us[2]; conv.mmx_us[3] %= convb.mmx_us[3];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
			
		exec_mmxmodb:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			if( !convb.mmx_c[0]
					or !convb.mmx_c[1]
					or !convb.mmx_c[2]
					or !convb.mmx_c[3]
					or !convb.mmx_c[4]
					or !convb.mmx_c[5]
					or !convb.mmx_c[6]
					or !convb.mmx_c[7] ) {
				printf("exec_mmxmodb: divide by 0 error.\n");
				goto *dispatch[halt];
			}
			conv.mmx_c[0] %= convb.mmx_c[0]; conv.mmx_c[1] %= convb.mmx_c[1];
			conv.mmx_c[2] %= convb.mmx_c[2]; conv.mmx_c[3] %= convb.mmx_c[3];
			conv.mmx_c[4] %= convb.mmx_c[4]; conv.mmx_c[5] %= convb.mmx_c[5];
			conv.mmx_c[6] %= convb.mmx_c[6]; conv.mmx_c[7] %= convb.mmx_c[7];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
			
		exec_mmxumodb:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			if( !convb.c[0]
					or !convb.c[1]
					or !convb.c[2]
					or !convb.c[3]
					or !convb.c[4]
					or !convb.c[5]
					or !convb.c[6]
					or !convb.c[7] ) {
				printf("exec_mmxumodb: divide by 0 error.\n");
				goto *dispatch[halt];
			}
			conv.c[0] %= convb.c[0]; conv.c[1] %= convb.c[1];
			conv.c[2] %= convb.c[2]; conv.c[3] %= convb.c[3];
			conv.c[4] %= convb.c[4]; conv.c[5] %= convb.c[5];
			conv.c[6] %= convb.c[6]; conv.c[7] %= convb.c[7];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
		
		exec_mmxandl:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			conv.mmx_ui[0] &= convb.mmx_ui[0];
			conv.mmx_ui[1] &= convb.mmx_ui[1];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
			
		exec_mmxands:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			conv.mmx_us[0] &= convb.mmx_us[0]; conv.mmx_us[1] &= convb.mmx_us[1];
			conv.mmx_us[2] &= convb.mmx_us[2]; conv.mmx_us[3] &= convb.mmx_us[3];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
			
		exec_mmxandb:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			conv.c[0] &= convb.c[0]; conv.c[1] &= convb.c[1];
			conv.c[2] &= convb.c[2]; conv.c[3] &= convb.c[3];
			conv.c[4] &= convb.c[4]; conv.c[5] &= convb.c[5];
			conv.c[6] &= convb.c[6]; conv.c[7] &= convb.c[7];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
		
		exec_mmxorl:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			conv.mmx_ui[0] |= convb.mmx_ui[0];
			conv.mmx_ui[1] |= convb.mmx_ui[1];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
			
		exec_mmxors:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			conv.mmx_us[0] |= convb.mmx_us[0]; conv.mmx_us[1] |= convb.mmx_us[1];
			conv.mmx_us[2] |= convb.mmx_us[2]; conv.mmx_us[3] |= convb.mmx_us[3];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
			
		exec_mmxorb:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			conv.c[0] |= convb.c[0]; conv.c[1] |= convb.c[1];
			conv.c[2] |= convb.c[2]; conv.c[3] |= convb.c[3];
			conv.c[4] |= convb.c[4]; conv.c[5] |= convb.c[5];
			conv.c[6] |= convb.c[6]; conv.c[7] |= convb.c[7];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
		
		exec_mmxxorl:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			conv.mmx_ui[0] ^= convb.mmx_ui[0];
			conv.mmx_ui[1] ^= convb.mmx_ui[1];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
			
		exec_mmxxors:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			conv.mmx_us[0] ^= convb.mmx_us[0]; conv.mmx_us[1] ^= convb.mmx_us[1];
			conv.mmx_us[2] ^= convb.mmx_us[2]; conv.mmx_us[3] ^= convb.mmx_us[3];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
			
		exec_mmxxorb:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			conv.c[0] ^= convb.c[0]; conv.c[1] ^= convb.c[1];
			conv.c[2] ^= convb.c[2]; conv.c[3] ^= convb.c[3];
			conv.c[4] ^= convb.c[4]; conv.c[5] ^= convb.c[5];
			conv.c[6] ^= convb.c[6]; conv.c[7] ^= convb.c[7];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
		
		exec_mmxnotl:;
			conv.ull = _tagha_pop_int64(vm);
			conv.mmx_ui[0] = ~conv.mmx_ui[0];
			conv.mmx_ui[1] = ~conv.mmx_ui[1];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
			
		exec_mmxnots:;
			conv.ull = _tagha_pop_int64(vm);
			conv.mmx_us[0] = ~conv.mmx_us[0]; conv.mmx_us[1] = ~conv.mmx_us[1];
			conv.mmx_us[2] = ~conv.mmx_us[2]; conv.mmx_us[3] = ~conv.mmx_us[3];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
			
		exec_mmxnotb:;
			conv.ull = _tagha_pop_int64(vm);
			conv.c[0] = ~conv.c[0]; conv.c[1] = ~conv.c[1];
			conv.c[2] = ~conv.c[2]; conv.c[3] = ~conv.c[3];
			conv.c[4] = ~conv.c[4]; conv.c[5] = ~conv.c[5];
			conv.c[6] = ~conv.c[6]; conv.c[7] = ~conv.c[7];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
		
		exec_mmxshll:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			conv.mmx_ui[0] <<= convb.mmx_ui[0];
			conv.mmx_ui[1] <<= convb.mmx_ui[1];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
			
		exec_mmxshls:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			conv.mmx_us[0] <<= convb.mmx_us[0]; conv.mmx_us[1] <<= convb.mmx_us[1];
			conv.mmx_us[2] <<= convb.mmx_us[2]; conv.mmx_us[3] <<= convb.mmx_us[3];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
			
		exec_mmxshlb:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			conv.c[0] <<= convb.c[0]; conv.c[1] <<= convb.c[1];
			conv.c[2] <<= convb.c[2]; conv.c[3] <<= convb.c[3];
			conv.c[4] <<= convb.c[4]; conv.c[5] <<= convb.c[5];
			conv.c[6] <<= convb.c[6]; conv.c[7] <<= convb.c[7];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
		
		exec_mmxshrl:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			conv.mmx_ui[0] >>= convb.mmx_ui[0];
			conv.mmx_ui[1] >>= convb.mmx_ui[1];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
			
		exec_mmxshrs:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			conv.mmx_us[0] >>= convb.mmx_us[0]; conv.mmx_us[1] >>= convb.mmx_us[1];
			conv.mmx_us[2] >>= convb.mmx_us[2]; conv.mmx_us[3] >>= convb.mmx_us[3];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
			
		exec_mmxshrb:;
			convb.ull = _tagha_pop_int64(vm);
			conv.ull = _tagha_pop_int64(vm);
			conv.c[0] >>= convb.c[0]; conv.c[1] >>= convb.c[1];
			conv.c[2] >>= convb.c[2]; conv.c[3] >>= convb.c[3];
			conv.c[4] >>= convb.c[4]; conv.c[5] >>= convb.c[5];
			conv.c[6] >>= convb.c[6]; conv.c[7] >>= convb.c[7];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
		
		exec_mmxincl:;
			conv.ull = _tagha_pop_int64(vm);
			++conv.mmx_ui[0]; ++conv.mmx_ui[1];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
		
		exec_mmxincf:;
			conv.ull = _tagha_pop_int64(vm);
			++conv.mmx_f[0]; ++conv.mmx_f[1];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
		
		exec_mmxincs:;
			conv.ull = _tagha_pop_int64(vm);
			++conv.mmx_us[0]; ++conv.mmx_us[1];
			++conv.mmx_us[2]; ++conv.mmx_us[3];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
			
		exec_mmxincb:;
			conv.ull = _tagha_pop_int64(vm);
			++conv.c[0]; ++conv.c[1];
			++conv.c[2]; ++conv.c[3];
			++conv.c[4]; ++conv.c[5];
			++conv.c[6]; ++conv.c[7];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
		
		exec_mmxdecl:;
			conv.ull = _tagha_pop_int64(vm);
			--conv.mmx_ui[0]; --conv.mmx_ui[1];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
			
		exec_mmxdecf:;
			conv.ull = _tagha_pop_int64(vm);
			--conv.mmx_f[0]; --conv.mmx_f[1];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
		
		exec_mmxdecs:;
			conv.ull = _tagha_pop_int64(vm);
			--conv.mmx_us[0]; --conv.mmx_us[1];
			--conv.mmx_us[2]; --conv.mmx_us[3];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
			
		exec_mmxdecb:;
			conv.ull = _tagha_pop_int64(vm);
			--conv.c[0]; --conv.c[1];
			--conv.c[2]; --conv.c[3];
			--conv.c[4]; --conv.c[5];
			--conv.c[6]; --conv.c[7];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
		
		exec_mmxnegl:;
			conv.ull = _tagha_pop_int64(vm);
			conv.mmx_ui[0] = -conv.mmx_ui[0];
			conv.mmx_ui[1] = -conv.mmx_ui[1];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
			
		exec_mmxnegf:;
			conv.ull = _tagha_pop_int64(vm);
			conv.mmx_f[0] = -conv.mmx_f[0];
			conv.mmx_f[1] = -conv.mmx_f[1];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
		
		exec_mmxnegs:;
			conv.ull = _tagha_pop_int64(vm);
			conv.mmx_us[0] = -conv.mmx_us[0]; conv.mmx_us[1] = -conv.mmx_us[1];
			conv.mmx_us[2] = -conv.mmx_us[2]; conv.mmx_us[3] = -conv.mmx_us[3];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
			
		exec_mmxnegb:;
			conv.ull = _tagha_pop_int64(vm);
			conv.mmx_c[0] = -conv.mmx_c[0]; conv.mmx_c[1] = -conv.mmx_c[1];
			conv.mmx_c[2] = -conv.mmx_c[2]; conv.mmx_c[3] = -conv.mmx_c[3];
			conv.mmx_c[4] = -conv.mmx_c[4]; conv.mmx_c[5] = -conv.mmx_c[5];
			conv.mmx_c[6] = -conv.mmx_c[6]; conv.mmx_c[7] = -conv.mmx_c[7];
			_tagha_push_int64(vm, conv.ull);
			DISPATCH();
		
		
		exec_jmp:;		// addresses are word sized bytes.
			conv.ui = _tagha_get_imm4(vm);
			vm->ip = conv.ui;
			printf("jmping to instruction address: %" PRIu32 "\n", vm->ip);
			continue;
		
		exec_jzq:;
			if( vm->bSafeMode and vm->sp-7 >= STK_SIZE ) {
				printf("exec_jzq reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-7);
				goto *dispatch[halt];
			}
			conv.c[7] = vm->pbStack[vm->sp-0];
			conv.c[6] = vm->pbStack[vm->sp-1];
			conv.c[5] = vm->pbStack[vm->sp-2];
			conv.c[4] = vm->pbStack[vm->sp-3];
			conv.c[3] = vm->pbStack[vm->sp-4];
			conv.c[2] = vm->pbStack[vm->sp-5];
			conv.c[1] = vm->pbStack[vm->sp-6];
			conv.c[0] = vm->pbStack[vm->sp-7];
			qa = conv.ull;
			conv.ui = _tagha_get_imm4(vm);
			vm->ip = (!qa) ? conv.ui : vm->ip+1 ;
			printf("jzq'ing to instruction address: %" PRIu32 "\n", vm->ip);
			continue;
			
		exec_jnzq:;
			if( vm->bSafeMode and vm->sp-7 >= STK_SIZE ) {
				printf("exec_jnzq reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-7);
				goto *dispatch[halt];
			}
			conv.c[7] = vm->pbStack[vm->sp-0];
			conv.c[6] = vm->pbStack[vm->sp-1];
			conv.c[5] = vm->pbStack[vm->sp-2];
			conv.c[4] = vm->pbStack[vm->sp-3];
			conv.c[3] = vm->pbStack[vm->sp-4];
			conv.c[2] = vm->pbStack[vm->sp-5];
			conv.c[1] = vm->pbStack[vm->sp-6];
			conv.c[0] = vm->pbStack[vm->sp-7];
			qa = conv.ull;
			conv.ui = _tagha_get_imm4(vm);
			vm->ip = (qa) ? conv.ui : vm->ip+1 ;
			printf("jnzq'ing to instruction address: %" PRIu32 "\n", vm->ip);
			continue;
		
		exec_jzl:;		// check if the first 4 bytes on stack are zero, if yes then jump it.
			if( vm->bSafeMode and vm->sp-3 >= STK_SIZE ) {
				printf("exec_jzl reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-3);
				goto *dispatch[halt];
			}
			conv.c[3] = vm->pbStack[vm->sp];
			conv.c[2] = vm->pbStack[vm->sp-1];
			conv.c[1] = vm->pbStack[vm->sp-2];
			conv.c[0] = vm->pbStack[vm->sp-3];
			a = conv.ui;
			conv.ui = _tagha_get_imm4(vm);
			vm->ip = (!a) ? conv.ui : vm->ip+1 ;
			printf("jzl'ing to instruction address: %" PRIu32 "\n", vm->ip);	//opcode2str[vm->pInstrStream[vm->ip]]
			continue;
		
		exec_jnzl:;
			if( vm->bSafeMode and vm->sp-3 >= STK_SIZE ) {
				printf("exec_jnzl reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-3);
				goto *dispatch[halt];
			}
			conv.c[3] = vm->pbStack[vm->sp];
			conv.c[2] = vm->pbStack[vm->sp-1];
			conv.c[1] = vm->pbStack[vm->sp-2];
			conv.c[0] = vm->pbStack[vm->sp-3];
			a = conv.ui;
			conv.ui = _tagha_get_imm4(vm);
			vm->ip = (a) ? conv.ui : vm->ip+1 ;
			printf("jnzl'ing to instruction address: %" PRIu32 "\n", vm->ip);
			continue;
		
		exec_call:;		// support functions
			conv.ui = _tagha_get_imm4(vm);	// get func address
			printf("call: calling address: %" PRIu32 "\n", conv.ui);
			_tagha_push_int32(vm, vm->ip+1);	// save return address.
			printf("call return addr: %" PRIu32 "\n", _tagha_peek_int32(vm));
			_tagha_push_int32(vm, vm->bp);	// push ebp;
			vm->bp = vm->sp;	// mov ebp, esp;
			vm->ip = conv.ui;	// jump to instruction
			printf("vm->bp: %" PRIu32 "\n", vm->sp);
			continue;
		
		exec_calls:;	// support local function pointers
			conv.ui = _tagha_pop_int32(vm);	// get func address
			printf("calls: calling address: %" PRIu32 "\n", conv.ui);
			_tagha_push_int32(vm, vm->ip+1);	// save return address.
			printf("call return addr: %" PRIu32 "\n", _tagha_peek_int32(vm));
			_tagha_push_int32(vm, vm->bp);	// push ebp
			vm->bp = vm->sp;	// mov ebp, esp
			vm->ip = conv.ui;	// jump to instruction
			printf("vm->bp: %" PRIu32 "\n", vm->sp);
			continue;
		
		exec_calla:;	// support globally allocated function pointers
			a = _tagha_pop_int32(vm);
			if( vm->bSafeMode and a > MEM_SIZE-4 ) {
				printf("exec_calla reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, a);
				goto *dispatch[halt];
			}
			conv.ui = _tagha_read_int32(vm, a);
			/*
			conv.c[0] = vm->pbMemory[a+0];
			conv.c[1] = vm->pbMemory[a+1];
			conv.c[2] = vm->pbMemory[a+2];
			conv.c[3] = vm->pbMemory[a+3];
			*/
			printf("calla: calling address: %" PRIu32 "\n", conv.ui);
			_tagha_push_int32(vm, vm->ip+1);	// save return address.
			printf("call return addr: %" PRIu32 "\n", _tagha_peek_int32(vm));
			_tagha_push_int32(vm, vm->bp);	// push ebp
			vm->bp = vm->sp;	// mov ebp, esp
			vm->ip = conv.ui;	// jump to instruction
			printf("vm->bp: %" PRIu32 "\n", vm->sp);
			continue;
		
		exec_ret:;
			vm->sp = vm->bp;	// mov esp, ebp
			printf("sp set to bp, sp == %" PRIu32 "\n", vm->sp);
			vm->bp = _tagha_pop_int32(vm);	// pop ebp
			vm->ip = _tagha_pop_int32(vm);	// pop return address.
			printf("returning to address: %" PRIu32 "\n", vm->ip);
			continue;
		
		exec_retx:; {		// for functions that return something.
			a = _tagha_get_imm4(vm);
			uchar bytebuffer[a];
			/* This opcode assumes all the data for return
			 * is on the near top of stack. In theory, you can
			 * use this method to return multiple pieces of data.
			 */
			_tagha_pop_nbytes(vm, bytebuffer, a);	// store our needed data to a buffer.
			// do our usual return code.
			vm->sp = vm->bp;	// mov esp, ebp
			printf("sp set to bp, sp == %" PRIu32 "\n", vm->sp);
			vm->bp = _tagha_pop_int32(vm);	// pop ebp
			vm->ip = _tagha_pop_int32(vm);	// pop return address.
			_tagha_push_nbytes(vm, bytebuffer, a);	// push back return data!
			printf("retxurning to address: %" PRIu32 "\n", vm->ip);
			continue;
		}
		exec_callnat:; {	// call a native
			// pop data and arg count possibly from VM?
			if( !vm->fnpNative ) {
				printf("exec_callnat reported: no registered native! Current instruction address: %" PRIu32 "\n", vm->ip);
				goto *dispatch[halt];
			}
			// how many bytes to push to native.
			const Word_t bytes = _tagha_get_imm4(vm);
			// how many arguments pushed as native args
			uchar argcount = vm->pInstrStream[++vm->ip];
			uchar params[bytes];
			_tagha_pop_nbytes(vm, params, bytes);
			vm->fnpNative(vm, argcount, bytes, params);
			DISPATCH();
		}
		/*
		 * support calling natives via function pointers */
		exec_callnats:;	// call native by local func ptr
			DISPATCH();
		
		exec_callnata:;	// call native by global func ptr
			DISPATCH();
		
		exec_reset:;
			tagha_reset(vm);
			DISPATCH();
	} /* while( 1 ) */
	printf("tagha_exec :: max instructions reached\n");
}
