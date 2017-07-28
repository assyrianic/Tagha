
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>
#include "vm.h"


/*	here's the deal ok? make an opcode for each and erry n-bytes!
 * 'q' = int64
 * 'l' - int32
 * 's' - int16
 * 'b' - byte
 * 'f32' - float32
 * 'f64' - float64
 * 'a' - address
 * 'sp' - takes/uses the current stack pointer address.
 * 'ip' - takes/uses the current instruction pointer address.
*/

// this vm is designed to run C programs. Vastly, if not all C expressions are int32, uint32 if bigger than int
// this is why the arithmetic and bit operations are all int32 sized.
// there's 2 byte and single byte memory storage for the sake of dealing with structs and unions.
// expressions are int or uint then truncated to a variable's byte-width.
// comparison operators always return int32.
#define INSTR_SET		\
	X(halt)				\
	X(pushq) X(pushl) X(pushs) X(pushb) X(pushsp) X(puship) \
	X(popq) X(popl) X(pops) X(popb) X(popsp) X(popip) \
	X(wrtq) X(wrtl) X(wrts) X(wrtb) \
	X(storeq) X(storel) X(stores) X(storeb) \
	X(storeqa) X(storela) X(storesa) X(storeba) \
	X(loadq) X(loadl) X(loads) X(loadb) \
	X(loadqa) X(loadla) X(loadsa) X(loadba) \
	X(loadspq) X(loadspl) X(loadsps) X(loadspb) \
	X(storespq) X(storespl) X(storesps) X(storespb) \
	X(copyq) X(copyl) X(copys) X(copyb) \
	X(addq) X(uaddq) X(addl) X(uaddl) X(addf32) X(addf64) \
	X(subq) X(usubq) X(subl) X(usubl) X(subf32) X(subf64) \
	X(mulq) X(umulq) X(mull) X(umull) X(mulf32) X(mulf64) \
	X(divq) X(udivq) X(divl) X(udivl) X(divf32) X(divf64) \
	X(modq) X(umodq) X(modl) X(umodl) \
	X(andq) X(orq) X(xorq) X(andl) X(orl) X(xorl) \
	X(notq) X(shlq) X(shrq) X(notl) X(shll) X(shrl) \
	X(incq) X(incl) X(incf32) X(incf64) \
	X(decq) X(decl) X(decf32) X(decf64) \
	X(negq) X(negl) X(negf32) X(negf64) \
	X(ltq) X(ultq) X(ltl) X(ultl) X(ltf32) X(ltf64) \
	X(gtq) X(ugtq) X(gtl) X(ugtl) X(gtf32) X(gtf64) \
	X(cmpq) X(ucmpq) X(cmpl) X(ucmpl) X(compf32) X(compf64) \
	X(leqq) X(uleqq) X(leql) X(uleql) X(leqf32) X(leqf64) \
	X(geql) X(ugeql) X(geql) X(ugeql) X(geqf32) X(geqf64) \
	X(jmp) X(jzq) X(jnzq) \
	X(nop)				\

#define X(x) x,
enum InstrSet { INSTR_SET };
#undef X

void crown_init(CrownVM_t *vm)
{
	if( !vm )
		return;
	
	vm->pScript = NULL;
}


// TODO: change this to load a script by file if testing goes well.
void crown_load_script(CrownVM_t *restrict vm, uchar *restrict program)
{
	if( !vm )
		return;
	
	vm->pScript = malloc( sizeof(Script_t) );
	if( vm->pScript ) {
		Script_t *code = vm->pScript;
		uchar *verify = program;
		// verify that this is executable code.
		if( *(ushort *)verify == 0xC0DE ) {
			verify += 2;
			
			code->uiMemSize = *(ullong *)verify; verify += 8;
			printf("crown_load_script :: Memory size: %u\n", code->uiMemSize);
			if( code->uiMemSize )
				code->pMemory = calloc(code->uiMemSize, sizeof(uchar));
			else code->pMemory = NULL;
			
			code->uiStkSize = *(ullong *)verify; verify += 8;
			printf("crown_load_script :: Stack size: %u\n", code->uiStkSize+1);
			if( code->uiStkSize )
				code->pStack = calloc(code->uiStkSize+1, sizeof(uchar));
			else code->pStack = NULL;
			
			code->ipstart = *(ullong *)verify; verify += 8;
			printf("crown_load_script :: ip starts at %u\n", code->ipstart);
			
			code->uiInstrCount = *(ullong *)verify;
			
			if( code->uiInstrCount ) {
				// TODO: have pInstrStream as calloc'd array and memcpy the program.
				code->pInstrStream = calloc(HEADER_BYTES+code->uiInstrCount, sizeof(uchar)); //program;
				printf("crown_load_script :: allocated instruction count: %u\n", code->uiInstrCount);
				if( !code->pInstrStream ) {
					printf("crown_load_script :: failed to load script :: instruction array is NULL!\n");
					crown_free_script(vm);
					exit(1);
				}
				//verify -= 14;
				//printf("crown_load_script :: reversed ptr success?: %u\n", *(ushort *)verify == 0xC0DE);
				uint i=0;
				while( i<(HEADER_BYTES + code->uiInstrCount) )
					code->pInstrStream[i++] = *program++;
			}
			else {
				printf("crown_load_script :: failed to load script :: script header has no instruction count!\n");
				crown_free_script(vm);
				exit(1);
			}
			script_reset(code);
			code->ip = code->ipstart;
		}
		else {
			printf("crown_load_script :: failed to load script :: unknown file format\n");
		}
	}
	else vm->pScript=NULL;
}

void crown_free_script(CrownVM_t *restrict vm)
{
	if( !vm )
		return;
	else if( !vm->pScript )
		return;
	
	if( vm->pScript->pMemory )
		free(vm->pScript->pMemory);
	vm->pScript->pMemory = NULL;
	
	if( vm->pScript->pStack )
		free(vm->pScript->pStack);
	vm->pScript->pStack = NULL;
	
	free(vm->pScript);
	vm->pScript=NULL;
}


void script_reset(Script_t *pScript)
{
	if( !pScript )
		return;
	uchar *p=NULL;
	uint i;
	p = pScript->pMemory;
	if( p )
		for( i=0 ; i<pScript->uiMemSize ; i++ )
			*p++ = 0; //pScript->pMemory[i] = 0;

	p = pScript->pStack;
	if( p )
		for( i=0 ; i<pScript->uiStkSize ; i++ )
			*p++ = 0; //pScript->pStack[i] = 0;

	pScript->ip = pScript->sp = pScript->bp = 0;
}


ullong script_pop_quad(Script_t *pScript)
{
	if( !pScript )
		return 0;
	else if( !pScript->pStack )
		return 0;
#ifdef SAFEMODE
	if( (pScript->sp-8) >= pCurrScript->uiStkSize ) {	// we're subtracting, did we integer underflow?
		printf("script_pop_quad reported: stack underflow! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\n", pScript->ip, pScript->sp-8);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.c[7] = pScript->pStack[pScript->sp--];
	conv.c[6] = pScript->pStack[pScript->sp--];
	conv.c[5] = pScript->pStack[pScript->sp--];
	conv.c[4] = pScript->pStack[pScript->sp--];
	conv.c[3] = pScript->pStack[pScript->sp--];
	conv.c[2] = pScript->pStack[pScript->sp--];
	conv.c[1] = pScript->pStack[pScript->sp--];
	conv.c[0] = pScript->pStack[pScript->sp--];
	return conv.ull;
}
uint script_pop_long(Script_t *pScript)
{
	if( !pScript )
		return 0;
	else if( !pScript->pStack )
		return 0;
#ifdef SAFEMODE
	if( (pScript->sp-4) >= pCurrScript->uiStkSize ) {	// we're subtracting, did we integer underflow?
		printf("script_pop_long reported: stack underflow! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\n", pScript->ip, pScript->sp-4);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.c[3] = pScript->pStack[pScript->sp--];
	conv.c[2] = pScript->pStack[pScript->sp--];
	conv.c[1] = pScript->pStack[pScript->sp--];
	conv.c[0] = pScript->pStack[pScript->sp--];
	return conv.ui;
}
ushort script_pop_short(Script_t *pScript)
{
	if( !pScript )
		return 0;
	else if( !pScript->pStack )
		return 0;
#ifdef SAFEMODE
	if( (pScript->sp-2) >= pCurrScript->uiStkSize ) {	// we're subtracting, did we integer underflow?
		printf("script_pop_short reported: stack underflow! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\n", pScript->ip, pScript->sp-2);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.c[1] = pScript->pStack[pScript->sp--];
	conv.c[0] = pScript->pStack[pScript->sp--];
	return conv.us;
}
uchar script_pop_byte(Script_t *pScript)
{
	if( !pScript )
		return 0;
	else if( !pScript->pStack )
		return 0;
#ifdef SAFEMODE
	if( (pScript->sp-1) >= pCurrScript->uiStkSize ) {	// we're subtracting, did we integer underflow?
		printf("script_pop_byte reported: stack underflow! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\n", pScript->ip, pScript->sp-1);
		exit(1);
	}
#endif
	return pScript->pStack[pScript->sp--];
}

float script_pop_float32(Script_t *pScript)
{
	if( !pScript )
		return 0;
	else if( !pScript->pStack )
		return 0;
#ifdef SAFEMODE
	if( (pScript->sp-4) >= pCurrScript->uiStkSize ) {	// we're subtracting, did we integer underflow?
		printf("script_pop_float32 reported: stack underflow! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\n", pScript->ip, pScript->sp-4);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.c[3] = pScript->pStack[pScript->sp--];
	conv.c[2] = pScript->pStack[pScript->sp--];
	conv.c[1] = pScript->pStack[pScript->sp--];
	conv.c[0] = pScript->pStack[pScript->sp--];
	return conv.f;
}

double script_pop_float64(Script_t *pScript)
{
	if( !pScript )
		return 0;
	else if( !pScript->pStack )
		return 0;
#ifdef SAFEMODE
	if( (pScript->sp-8) >= pCurrScript->uiStkSize ) {
		printf("script_pop_float64 reported: stack underflow! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\n", pScript->ip, pScript->sp-8);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.c[7] = pScript->pStack[pScript->sp--];
	conv.c[6] = pScript->pStack[pScript->sp--];
	conv.c[5] = pScript->pStack[pScript->sp--];
	conv.c[4] = pScript->pStack[pScript->sp--];
	conv.c[3] = pScript->pStack[pScript->sp--];
	conv.c[2] = pScript->pStack[pScript->sp--];
	conv.c[1] = pScript->pStack[pScript->sp--];
	conv.c[0] = pScript->pStack[pScript->sp--];
	return conv.dbl;
}

void script_push_quad(Script_t *restrict pScript, const ullong val)
{
	if( !pScript )
		return;
	else if( !pScript->pStack )
		return;
#ifdef SAFEMODE
	if( (pScript->sp+8) >= pCurrScript->uiStkSize ) {
		printf("script_pop_quad reported: stack overflow! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\n", pScript->ip, pScript->sp+8);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.ull = val;
	pScript->pStack[++pScript->sp] = conv.c[0];
	pScript->pStack[++pScript->sp] = conv.c[1];
	pScript->pStack[++pScript->sp] = conv.c[2];
	pScript->pStack[++pScript->sp] = conv.c[3];
	pScript->pStack[++pScript->sp] = conv.c[4];
	pScript->pStack[++pScript->sp] = conv.c[5];
	pScript->pStack[++pScript->sp] = conv.c[6];
	pScript->pStack[++pScript->sp] = conv.c[7];
}

void script_push_long(Script_t *restrict pScript, const uint val)
{
	if( !pScript )
		return;
	else if( !pScript->pStack )
		return;
#ifdef SAFEMODE
	if( (pScript->sp+4) >= pCurrScript->uiStkSize ) {
		printf("script_pop_long reported: stack overflow! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\n", pScript->ip, pScript->sp+4);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.ui = val;
	pScript->pStack[++pScript->sp] = conv.c[0];
	pScript->pStack[++pScript->sp] = conv.c[1];
	pScript->pStack[++pScript->sp] = conv.c[2];
	pScript->pStack[++pScript->sp] = conv.c[3];
}

void script_push_short(Script_t *restrict pScript, const ushort val)
{
	if( !pScript )
		return;
	else if( !pScript->pStack )
		return;
#ifdef SAFEMODE
	if( (pScript->sp+2) >= pCurrScript->uiStkSize ) {
		printf("script_pop_short reported: stack overflow! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\n", pScript->ip, pScript->sp+2);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.us = val;
	pScript->pStack[++pScript->sp] = conv.c[0];
	pScript->pStack[++pScript->sp] = conv.c[1];
}

void script_push_byte(Script_t *restrict pScript, const uchar val)
{
	if( !pScript )
		return;
	else if( !pScript->pStack )
		return;
#ifdef SAFEMODE
	if( (pScript->sp+1) >= pCurrScript->uiStkSize ) {
		printf("script_pop_byte reported: stack overflow! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\n", pScript->ip, pScript->sp+1);
		exit(1);
	}
#endif
	pScript->pStack[++pScript->sp] = val;
}

void script_push_float32(Script_t *restrict pScript, const float val)
{
	if( !pScript )
		return;
	else if( !pScript->pStack )
		return;
#ifdef SAFEMODE
	if( (pScript->sp+4) >= pCurrScript->uiStkSize ) {
		printf("script_pop_float32 reported: stack overflow! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\n", pScript->ip, pScript->sp+4);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.f = val;
	pScript->pStack[++pScript->sp] = conv.c[0];
	pScript->pStack[++pScript->sp] = conv.c[1];
	pScript->pStack[++pScript->sp] = conv.c[2];
	pScript->pStack[++pScript->sp] = conv.c[3];
}

void script_push_float64(Script_t *restrict pScript, const double val)
{
	if( !pScript )
		return;
	else if( !pScript->pStack )
		return;
#ifdef SAFEMODE
	if( (pScript->sp+8) >= pCurrScript->uiStkSize ) {
		printf("script_pop_float64 reported: stack overflow! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\n", pScript->ip, pScript->sp+8);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.dbl = val;
	pScript->pStack[++pScript->sp] = conv.c[0];
	pScript->pStack[++pScript->sp] = conv.c[1];
	pScript->pStack[++pScript->sp] = conv.c[2];
	pScript->pStack[++pScript->sp] = conv.c[3];
	pScript->pStack[++pScript->sp] = conv.c[4];
	pScript->pStack[++pScript->sp] = conv.c[5];
	pScript->pStack[++pScript->sp] = conv.c[6];
	pScript->pStack[++pScript->sp] = conv.c[7];
}


void script_write_quad(Script_t *restrict pScript, const ullong val, const ullong address){
	if( !pScript )
		return;
	else if( !pScript->pMemory )
		return;
#ifdef SAFEMODE
	if( address > pCurrScript->uiMemSize-8 ) {
		printf("script_write_quad reported: Invalid Memory Access! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\nInvalid Memory Address: %" PRIu64 "\n", pScript->ip, pScript->sp, address);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.ull = val;
	pScript->pMemory[address] = conv.c[7];
	pScript->pMemory[address+1] = conv.c[6];
	pScript->pMemory[address+2] = conv.c[5];
	pScript->pMemory[address+3] = conv.c[4];
	pScript->pMemory[address+4] = conv.c[3];
	pScript->pMemory[address+5] = conv.c[2];
	pScript->pMemory[address+6] = conv.c[1];
	pScript->pMemory[address+7] = conv.c[0];
}

void script_write_long(Script_t *restrict pScript, const uint val, const ullong address)
{
	if( !pScript )
		return;
	else if( !pScript->pMemory )
		return;
#ifdef SAFEMODE
	if( address > pCurrScript->uiMemSize-4 ) {
		printf("script_write_long reported: Invalid Memory Access! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\nInvalid Memory Address: %" PRIu64 "\n", pScript->ip, pScript->sp, address);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.ui = val;
	pScript->pMemory[address] = conv.c[3];
	pScript->pMemory[address+1] = conv.c[2];
	pScript->pMemory[address+2] = conv.c[1];
	pScript->pMemory[address+3] = conv.c[0];
}

void script_write_short(Script_t *restrict pScript, const ushort val, const ullong address)
{
	if( !pScript )
		return;
	else if( !pScript->pMemory )
		return;
#ifdef SAFEMODE
	if( address > pCurrScript->uiMemSize-2 ) {
		printf("script_write_short reported: Invalid Memory Access! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\nInvalid Memory Address: %" PRIu64 "\n", pScript->ip, pScript->sp, address);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.us = val;
	pScript->pMemory[address] = conv.c[1];
	pScript->pMemory[address+1] = conv.c[0];
}

void script_write_byte(Script_t *restrict pScript, const uchar val, const ullong address)
{
	if( !pScript )
		return;
	else if( !pScript->pMemory )
		return;
#ifdef SAFEMODE
	if( address >= pCurrScript->uiMemSize ) {
		printf("script_write_byte reported: Invalid Memory Access! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\nInvalid Memory Address: %" PRIu64 "\n", pScript->ip, pScript->sp, address);
		exit(1);
	}
#endif
	pScript->pMemory[address] = val;
}

void script_write_float32(Script_t *restrict pScript, const float val, const ullong address)
{
	if( !pScript )
		return;
	else if( !pScript->pMemory )
		return;
#ifdef SAFEMODE
	if( address > pCurrScript->uiMemSize-4 ) {
		printf("script_write_float32 reported: Invalid Memory Access! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\nInvalid Memory Address: %" PRIu64 "\n", pScript->ip, pScript->sp, address);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.f = val;
	pScript->pMemory[address] = conv.c[3];
	pScript->pMemory[address+1] = conv.c[2];
	pScript->pMemory[address+2] = conv.c[1];
	pScript->pMemory[address+3] = conv.c[0];
}

void script_write_float64(Script_t *restrict pScript, const double val, const ullong address)
{
	if( !pScript )
		return;
	else if( !pScript->pMemory )
		return;
#ifdef SAFEMODE
	if( address > pCurrScript->uiMemSize-8 ) {
		printf("script_write_float64 reported: Invalid Memory Access! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\nInvalid Memory Address: %" PRIu64 "\n", pScript->ip, pScript->sp, address);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.dbl = val;
	pScript->pMemory[address] = conv.c[7];
	pScript->pMemory[address+1] = conv.c[6];
	pScript->pMemory[address+2] = conv.c[5];
	pScript->pMemory[address+3] = conv.c[4];
	pScript->pMemory[address+4] = conv.c[3];
	pScript->pMemory[address+5] = conv.c[2];
	pScript->pMemory[address+6] = conv.c[1];
	pScript->pMemory[address+7] = conv.c[0];
}


void script_write_bytearray(Script_t *restrict pScript, uchar *restrict val, const uint size, const ullong address)
{
	if( !pScript || !val )
		return;
	else if( !pScript->pMemory )
		return;
	ullong addr = address+(size-1);
	uint i = 0;
	while( i<size ) {
#ifdef SAFEMODE
		if( addr >= pCurrScript->uiMemSize-i ) {
			printf("script_write_array reported: Invalid Memory Access! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\nInvalid Memory Address: %" PRIu64 "\n", pScript->ip, pScript->sp, addr);
			exit(1);
		}
#endif
		pScript->pMemory[addr--] = val[i++];
	}
}

ullong script_read_quad(Script_t *restrict pScript, const ullong address)
{
	if( !pScript )
		return 0;
	else if( !pScript->pMemory )
		return 0;
#ifdef SAFEMODE
	if( address > pCurrScript->uiMemSize-8 ) {
		printf("script_read_quad reported: Invalid Memory Access! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\nInvalid Memory Address: %" PRIu64 "\n", pScript->ip, pScript->sp, address);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.c[7] = pScript->pMemory[address];
	conv.c[6] = pScript->pMemory[address+1];
	conv.c[5] = pScript->pMemory[address+2];
	conv.c[4] = pScript->pMemory[address+3];
	conv.c[3] = pScript->pMemory[address+4];
	conv.c[2] = pScript->pMemory[address+5];
	conv.c[1] = pScript->pMemory[address+6];
	conv.c[0] = pScript->pMemory[address+7];
	return conv.ull;
}

uint script_read_long(Script_t *restrict pScript, const ullong address)
{
	if( !pScript )
		return 0;
	else if( !pScript->pMemory )
		return 0;
#ifdef SAFEMODE
	if( address > pCurrScript->uiMemSize-4 ) {
		printf("script_read_long reported: Invalid Memory Access! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\nInvalid Memory Address: %" PRIu64 "\n", pScript->ip, pScript->sp, address);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.c[3] = pScript->pMemory[address];
	conv.c[2] = pScript->pMemory[address+1];
	conv.c[1] = pScript->pMemory[address+2];
	conv.c[0] = pScript->pMemory[address+3];
	return conv.ui;
}

ushort script_read_short(Script_t *restrict pScript, const ullong address)
{
	if( !pScript )
		return 0;
	else if( !pScript->pMemory )
		return 0;
#ifdef SAFEMODE
	if( address > pCurrScript->uiMemSize-2 ) {
		printf("script_read_short reported: Invalid Memory Access! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\nInvalid Memory Address: %" PRIu64 "\n", pScript->ip, pScript->sp, address);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.c[1] = pScript->pMemory[address];
	conv.c[0] = pScript->pMemory[address+1];
	return conv.us;
}

uchar script_read_byte(Script_t *restrict pScript, const ullong address)
{
	if( !pScript )
		return 0;
	else if( !pScript->pMemory )
		return 0;
#ifdef SAFEMODE
	if( address >= pCurrScript->uiMemSize ) {
		printf("script_read_byte reported: Invalid Memory Access! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\nInvalid Memory Address: %" PRIu64 "\n", pScript->ip, pScript->sp, address);
		exit(1);
	}
#endif
	return pScript->pMemory[address];
}

float script_read_float32(Script_t *restrict pScript, const ullong address)
{
	if( !pScript )
		return 0;
	else if( !pScript->pMemory )
		return 0;
	
#ifdef SAFEMODE
	if( address > pCurrScript->uiMemSize-4 ) {
		printf("script_read_float32 reported: Invalid Memory Access! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\nInvalid Memory Address: %" PRIu64 "\n", pScript->ip, pScript->sp, address);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.c[0] = pScript->pMemory[address];
	conv.c[1] = pScript->pMemory[address+1];
	conv.c[2] = pScript->pMemory[address+2];
	conv.c[3] = pScript->pMemory[address+3];
	return conv.f;
}

double script_read_float64(Script_t *restrict pScript, const ullong address)
{
	if( !pScript )
		return 0;
	else if( !pScript->pMemory )
		return 0;
	
#ifdef SAFEMODE
	if( address > pCurrScript->uiMemSize-8 ) {
		printf("script_read_float64 reported: Invalid Memory Access! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\nInvalid Memory Address: %" PRIu64 "\n", pScript->ip, pScript->sp, address);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.c[7] = pScript->pMemory[address];
	conv.c[6] = pScript->pMemory[address+1];
	conv.c[5] = pScript->pMemory[address+2];
	conv.c[4] = pScript->pMemory[address+3];
	conv.c[3] = pScript->pMemory[address+4];
	conv.c[2] = pScript->pMemory[address+5];
	conv.c[1] = pScript->pMemory[address+6];
	conv.c[0] = pScript->pMemory[address+7];
	return conv.dbl;
}

void script_read_bytearray(Script_t *restrict pScript, uchar *restrict buffer, const uint size, const ullong address)
{
	if( !pScript || !buffer )
		return;
	else if( !pScript->pMemory )
		return;
	
	ullong addr = address+(size-1);
	uint i = 0;
	while( i<size ) {
#ifdef SAFEMODE
		if( addr >= pCurrScript->uiMemSize-i ) {
			printf("script_read_array reported: Invalid Memory Access! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\nInvalid Memory Address: %" PRIu64 "\n", pScript->ip, pScript->sp, addr);
			exit(1);
		}
#endif
		buffer[i++] = pScript->pMemory[addr--];
	}
}


void scripts_debug_print_memory(const Script_t *pScript)
{
	if( !pScript )
		return;
	else if( !pScript->pMemory )
		return;
	
	printf("DEBUG ...---===---... Printing Memory...\n");
	ullong i;
	for( i=0 ; i<pScript->uiMemSize ; i++ )
		printf("Memory Index: %" PRIu64 " | data: %" PRIu8 "\n", i, pScript->pMemory[i]);
	printf("\n");
}
void scripts_debug_print_stack(const Script_t *pScript)
{
	if( !pScript )
		return;
	else if( !pScript->pStack )
		return;
	
	printf("DEBUG ...---===---... Printing Stack...\n");
	ullong i;
	for( i=1 ; i<pScript->uiStkSize ; i++ )
		printf("Stack Index: %" PRIu64 " | data: %" PRIu8 "\n", i, pScript->pStack[i]);
	printf("\n");
}

void scripts_debug_print_ptrs(const Script_t *pScript)
{
	if( !pScript )
		return;
	
	printf("DEBUG ...---===---... Printing Pointers...\n");
	printf("Instruction Pointer: %" PRIu64 "\
			\nStack Pointer: %" PRIu64 "\
			\nStack Frame Pointer: %" PRIu64 "\n", pScript->ip, pScript->sp, pScript->bp);
	printf("\n");
}



static inline ullong script_get_imm8(Script_t *restrict pScript)
{
	if( !pScript )
		return 0;
	union conv_union conv;
	conv.c[0] = pScript->pInstrStream[++pScript->ip];
	conv.c[1] = pScript->pInstrStream[++pScript->ip];
	conv.c[2] = pScript->pInstrStream[++pScript->ip];
	conv.c[3] = pScript->pInstrStream[++pScript->ip];
	conv.c[4] = pScript->pInstrStream[++pScript->ip];
	conv.c[5] = pScript->pInstrStream[++pScript->ip];
	conv.c[6] = pScript->pInstrStream[++pScript->ip];
	conv.c[7] = pScript->pInstrStream[++pScript->ip];
	return conv.ull;
}
static inline uint script_get_imm4(Script_t *restrict pScript)
{
	if( !pScript )
		return 0;
	union conv_union conv;
	conv.c[0] = pScript->pInstrStream[++pScript->ip];
	conv.c[1] = pScript->pInstrStream[++pScript->ip];
	conv.c[2] = pScript->pInstrStream[++pScript->ip];
	conv.c[3] = pScript->pInstrStream[++pScript->ip];
	return conv.ui;
}

static inline ushort script_get_imm2(Script_t *restrict pScript)
{
	if( !pScript )
		return 0;
	union conv_union conv;
	conv.c[0] = pScript->pInstrStream[++pScript->ip];
	conv.c[1] = pScript->pInstrStream[++pScript->ip];
	return conv.us;
}


static inline ullong _script_pop_quad(Script_t *restrict pScript)
{
	if( !pScript )
		return 0;
#ifdef SAFEMODE
	if( (pScript->sp-8) >= pCurrScript->uiStkSize ) {	// we're subtracting, did we integer underflow?
		printf("_script_pop_quad reported: stack underflow! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\n", pScript->ip, pScript->sp-8);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.c[7] = pScript->pStack[pScript->sp--];
	conv.c[6] = pScript->pStack[pScript->sp--];
	conv.c[5] = pScript->pStack[pScript->sp--];
	conv.c[4] = pScript->pStack[pScript->sp--];
	conv.c[3] = pScript->pStack[pScript->sp--];
	conv.c[2] = pScript->pStack[pScript->sp--];
	conv.c[1] = pScript->pStack[pScript->sp--];
	conv.c[0] = pScript->pStack[pScript->sp--];
	return conv.ull;
}

static inline uint _script_pop_long(Script_t *restrict pScript)
{
	if( !pScript )
		return 0;
#ifdef SAFEMODE
	if( (pScript->sp-4) >= pCurrScript->uiStkSize ) {	// we're subtracting, did we integer underflow?
		printf("_script_pop_long reported: stack underflow! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\n", pScript->ip, pScript->sp-4);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.c[3] = pScript->pStack[pScript->sp--];
	conv.c[2] = pScript->pStack[pScript->sp--];
	conv.c[1] = pScript->pStack[pScript->sp--];
	conv.c[0] = pScript->pStack[pScript->sp--];
	return conv.ui;
}

static inline float _script_pop_float32(Script_t *restrict pScript)
{
	if( !pScript )
		return 0;
#ifdef SAFEMODE
	if( (pScript->sp-4) >= pCurrScript->uiStkSize ) {	// we're subtracting, did we integer underflow?
		printf("_script_pop_float32 reported: stack underflow! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\n", pScript->ip, pScript->sp-4);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.c[3] = pScript->pStack[pScript->sp--];
	conv.c[2] = pScript->pStack[pScript->sp--];
	conv.c[1] = pScript->pStack[pScript->sp--];
	conv.c[0] = pScript->pStack[pScript->sp--];
	return conv.f;
}
static inline double _script_pop_float64(Script_t *restrict pScript)
{
	if( !pScript )
		return 0;
#ifdef SAFEMODE
	if( (pScript->sp-8) >= pCurrScript->uiStkSize ) {	// we're subtracting, did we integer underflow?
		printf("_script_pop_float64 reported: stack underflow! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\n", pScript->ip, pScript->sp-8);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.c[7] = pScript->pStack[pScript->sp--];
	conv.c[6] = pScript->pStack[pScript->sp--];
	conv.c[5] = pScript->pStack[pScript->sp--];
	conv.c[4] = pScript->pStack[pScript->sp--];
	conv.c[3] = pScript->pStack[pScript->sp--];
	conv.c[2] = pScript->pStack[pScript->sp--];
	conv.c[1] = pScript->pStack[pScript->sp--];
	conv.c[0] = pScript->pStack[pScript->sp--];
	return conv.dbl;
}

static inline ushort _script_pop_short(Script_t *restrict pScript)
{
	if( !pScript )
		return 0;
#ifdef SAFEMODE
	if( (pScript->sp-2) >= pCurrScript->uiStkSize ) {	// we're subtracting, did we integer underflow?
		printf("_script_pop_short reported: stack underflow! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\n", pScript->ip, pScript->sp-2);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.c[1] = pScript->pStack[pScript->sp--];
	conv.c[0] = pScript->pStack[pScript->sp--];
	return conv.us;
}

static inline uchar _script_pop_byte(Script_t *restrict pScript)
{
	if( !pScript )
		return 0;
#ifdef SAFEMODE
	if( (pScript->sp-1) >= pCurrScript->uiStkSize ) {	// we're subtracting, did we integer underflow?
		printf("script_pop_long reported: stack underflow! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\n", pScript->ip, pScript->sp-1);
		exit(1);
	}
#endif
	return pScript->pStack[pScript->sp--];
}


static inline void _script_push_quad(Script_t *restrict pScript, const ullong val)
{
	if( !pScript )
		return;
#ifdef SAFEMODE
	if( (pScript->sp+8) >= pCurrScript->uiStkSize ) {
		printf("script_pop_long reported: stack overflow! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\n", pScript->ip, pScript->sp+8);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.ull = val;
	pScript->pStack[++pScript->sp] = conv.c[0];
	pScript->pStack[++pScript->sp] = conv.c[1];
	pScript->pStack[++pScript->sp] = conv.c[2];
	pScript->pStack[++pScript->sp] = conv.c[3];
	pScript->pStack[++pScript->sp] = conv.c[4];
	pScript->pStack[++pScript->sp] = conv.c[5];
	pScript->pStack[++pScript->sp] = conv.c[6];
	pScript->pStack[++pScript->sp] = conv.c[7];
}

static inline void _script_push_long(Script_t *restrict pScript, const uint val)
{
	if( !pScript )
		return;
#ifdef SAFEMODE
	if( (pScript->sp+4) >= pCurrScript->uiStkSize ) {
		printf("script_pop_long reported: stack overflow! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\n", pScript->ip, pScript->sp+4);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.ui = val;
	pScript->pStack[++pScript->sp] = conv.c[0];
	pScript->pStack[++pScript->sp] = conv.c[1];
	pScript->pStack[++pScript->sp] = conv.c[2];
	pScript->pStack[++pScript->sp] = conv.c[3];
}
static inline void _script_push_float32(Script_t *restrict pScript, const float val)
{
	if( !pScript )
		return;
#ifdef SAFEMODE
	if( (pScript->sp+4) >= pCurrScript->uiStkSize ) {
		printf("script_pop_float32 reported: stack overflow! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\n", pScript->ip, pScript->sp+4);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.f = val;
	pScript->pStack[++pScript->sp] = conv.c[0];
	pScript->pStack[++pScript->sp] = conv.c[1];
	pScript->pStack[++pScript->sp] = conv.c[2];
	pScript->pStack[++pScript->sp] = conv.c[3];
}
static inline void _script_push_float64(Script_t *restrict pScript, const double val)
{
	if( !pScript )
		return;
#ifdef SAFEMODE
	if( (pScript->sp+8) >= pCurrScript->uiStkSize ) {
		printf("script_pop_long reported: stack overflow! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\n", pScript->ip, pScript->sp+8);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.dbl = val;
	pScript->pStack[++pScript->sp] = conv.c[0];
	pScript->pStack[++pScript->sp] = conv.c[1];
	pScript->pStack[++pScript->sp] = conv.c[2];
	pScript->pStack[++pScript->sp] = conv.c[3];
	pScript->pStack[++pScript->sp] = conv.c[4];
	pScript->pStack[++pScript->sp] = conv.c[5];
	pScript->pStack[++pScript->sp] = conv.c[6];
	pScript->pStack[++pScript->sp] = conv.c[7];
}

static inline void _script_push_short(Script_t *restrict pScript, const ushort val)
{
	if( !pScript )
		return;
#ifdef SAFEMODE
	if( (pScript->sp+2) >= pCurrScript->uiStkSize ) {
		printf("script_pop_short reported: stack overflow! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\n", pScript->ip, pScript->sp+2);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.us = val;
	pScript->pStack[++pScript->sp] = conv.c[0];
	pScript->pStack[++pScript->sp] = conv.c[1];
}

static inline void _script_push_byte(Script_t *restrict pScript, const uchar val)
{
	if( !pScript )
		return;
#ifdef SAFEMODE
	if( (pScript->sp+1) >= pCurrScript->uiStkSize ) {
		printf("script_pop_byte reported: stack overflow! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\n", pScript->ip, pScript->sp+1);
		exit(1);
	}
#endif
	pScript->pStack[++pScript->sp] = val;
}


//#include <unistd.h>	// sleep() func
void crown_exec(CrownVM_t *restrict vm)
{
	if( !vm )
		return;
	
	Script_t *pCurrScript = vm->pScript;
	if( !pCurrScript )
		return;
	else if( !pCurrScript->pInstrStream )
		return;
	
	union conv_union conv;
	ullong qb,qa;
	uint lb,la;
	float f32b,f32a;
	ushort sb,sa;
	uchar cb,ca;
	double f64b, f64a;
	
#define X(x) #x ,
	const char *opcode2str[] = { INSTR_SET };
#undef X
	
#define X(x) &&exec_##x ,
	static const void *dispatch[] = { INSTR_SET };
#undef X
#undef INSTR_SET
	
	if( pCurrScript->pInstrStream[pCurrScript->ip] > nop ) {
		printf("illegal instruction exception! instruction == \'%" PRIu8 "\' @ %" PRIu64 "\n", pCurrScript->pInstrStream[pCurrScript->ip], pCurrScript->ip);
		goto *dispatch[halt];
		return;
	}
	//printf( "current instruction == \"%s\" @ ip == %" PRIu64 "\n", opcode2str[pCurrScript->pInstrStream[pCurrScript->ip]], pCurrScript->ip );
#ifdef _UNISTD_H
	#define DISPATCH()	sleep(1); goto *dispatch[ pCurrScript->pInstrStream[++pCurrScript->ip] ]
#else
	#define DISPATCH()	goto *dispatch[ pCurrScript->pInstrStream[++pCurrScript->ip] ]
#endif
	goto *dispatch[ pCurrScript->pInstrStream[pCurrScript->ip] ];
	
	
exec_nop:;
	printf("no op.\n");
	DISPATCH();
	
exec_halt:;
	printf("===================== vm done\n\n");
	return;
	
exec_pushq:;	// push 8 bytes onto the stack
	conv.ull = script_get_imm8(pCurrScript);
	_script_push_quad(pCurrScript, conv.ull);
	printf("pushq: pushed %" PRIu64 "\n", conv.ull);
	DISPATCH();
	
exec_pushl:;	// push 4 bytes onto the stack
	conv.ui = script_get_imm4(pCurrScript);
	_script_push_long(pCurrScript, conv.ui);
	printf("pushl: pushed %" PRIu32 "\n", conv.ui);
	DISPATCH();
	
exec_pushs:;	// push 8 bytes onto the stack
	conv.us = script_get_imm2(pCurrScript);
	_script_push_short(pCurrScript, conv.us);
	printf("pushs: pushed %" PRIu16 "\n", conv.us);
	DISPATCH();
	
exec_pushb:;	// push 8 bytes onto the stack
	conv.c[0] = pScript->pInstrStream[++pScript->ip];
	_script_push_byte(pCurrScript, conv.c[0]);
	printf("pushb: pushed %" PRIu8 "\n", conv.c[0]);
	DISPATCH();
	
exec_pushsp:;	// sp is 64-bits wide.
	conv.ull = vm->sp;
	_script_push_quad(pCurrScript, conv.ull);
	printf("pushsp: pushed sp index: %" PRIu64 "\n", conv.ull);
	DISPATCH();
	
exec_puship:;
	conv.ull = vm->ip;
	_script_push_quad(pCurrScript, conv.ull);
	printf("puship: pushed ip index: %" PRIu64 "\n", conv.ull);
	DISPATCH();
	
exec_popq:;
#ifdef SAFEMODE
	if( pCurrScript->sp-8 >= pCurrScript->uiStkSize ) {
		printf("exec_popl reported: stack underflow! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\n", pCurrScript->ip, pCurrScript->sp-8);
		goto *dispatch[halt];
	}
#endif
	pCurrScript->sp -= 8;
	printf("popq\n");
	DISPATCH();
	
exec_popl:;		// pop 4 bytes to eventually be overwritten
#ifdef SAFEMODE
	if( pCurrScript->sp-4 >= pCurrScript->uiStkSize ) {
		printf("exec_popl reported: stack underflow! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\n", pCurrScript->ip, pCurrScript->sp-4);
		goto *dispatch[halt];
	}
#endif
	pCurrScript->sp -= 4;
	printf("popl\n");
	DISPATCH();

exec_pops:;
exec_popb:;
exec_popsp:;
exec_popip:;
exec_wrtq:;
exec_wrtl:;
exec_wrts:;
exec_wrtb:;
exec_storeq:;
exec_storel:;
exec_stores:;
exec_storeb:;
exec_storeqa:;
exec_storela:;
exec_storesa:;
exec_storeba:;
exec_loadq:;
exec_loadl:;
exec_loads:;
exec_loadb:;
exec_loadqa:;
exec_loadla:;
exec_loadsa:;
exec_loadba:;
exec_loadspq:;
exec_loadspl:;
exec_loadsps:;
exec_loadspb:;
exec_storespq:;
exec_storespl:;
exec_storesps:;
exec_storespb:;
exec_copyq:;
exec_copyl:;
exec_copys:;
exec_copyb:;
exec_addq:;
exec_uaddq:;
exec_addl:;
exec_uaddl:;
exec_addf32:;
exec_addf64:;
exec_subq:;
exec_usubq:;
exec_subl:;
exec_usubl:;
exec_subf32:;
exec_subf64:;
exec_mulq:;
exec_umulq:;
exec_mull:;
exec_umull:;
exec_mulf32:;
exec_mulf64:;
exec_divq:;
exec_udivq:;
exec_divl:;
exec_udivl:;
exec_divf32:;
exec_divf64:;
exec_modq:;
exec_umodq:;
exec_modl:;
exec_umodl:;
exec_andq:;
exec_orq:;
exec_xorq:;
exec_andl:;
exec_orl:;
exec_xorl:;
exec_notq:;
exec_shlq:;
exec_shrq:;
exec_notl:;
exec_shll:;
exec_shrl:;
exec_incq:;
exec_incl:;
exec_incf32:;
exec_incf64:;
exec_decq:;
exec_decl:;
exec_decf32:;
exec_decf64:;
exec_negq:;
exec_negl:;
exec_negf32:;
exec_negf64:;
exec_ltq:;
exec_ultq:;
exec_ltl:;
exec_ultl:;
exec_ltf32:;
exec_ltf64:;
exec_gtq:;
exec_ugtq:;
exec_gtl:;
exec_ugtl:;
exec_gtf32:;
exec_gtf64:;
exec_cmpq:;
exec_ucmpq:;
exec_cmpl:;
exec_ucmpl:;
exec_compf32:;
exec_compf64:;
exec_leqq:;
exec_uleqq:;
exec_leql:;
exec_uleql:;
exec_leqf32:;
exec_leqf64:;
exec_geql:;
exec_ugeql:;
exec_geql:;
exec_ugeql:;
exec_geqf32:;
exec_geqf64:;
exec_jmp:;
exec_jzq:;
exec_jnzq:;
	DISPATCH();
}


int main(void)
{
	bytecode test1={
		0xDE, 0xC0,	// magic header
		0,0,0,0,0,0,0,0,	// set memory size.
		9,0,0,0,0,0,0,0,	// set stack size.
		18,0,0,0,0,0,0,0,	// set ip entry point.
		11,0,0,0,0,0,0,0,	// set instruction count.
		nop,
		pushq, 255,0,0,0,0,0,0,0,
		halt
	};
	
	CrownVM_t *vm = &(CrownVM_t){0}; //malloc(sizeof(CrownVM_t));
	printf("sizeof(Script_t) == \"%u\"\n", sizeof(Script_t));
	crown_init(vm);
	crown_load_script(vm, test1); crown_exec(vm); //crown_free(cvm);
	scripts_debug_print_memory(vm->pScript);
	scripts_debug_print_stack(vm->pScript);
	
	//	this is for testing to see how much memory \
		the program is using while consuming scripts.
	uint d=1;
	while( scanf("%u", &d)>0 && d != 0 );
	crown_free_script(vm);
	//script_write_bytearray(vm->pScript, (uchar *)"Hello World", 11, 4);
	
	//uchar buffer[12] = {0};
	//script_read_bytearray(vm->pScript, buffer, 11, 0x04);
	//printf("buffer == \'%s\'\n", buffer);
	return 0;
}





