
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <iso646.h>
#include <inttypes.h>
//#include <stdarg.h>
#include <assert.h>
#include "tagha.h"

#define INSTR_SET	\
	X(halt) \
	X(pushq) X(pushl) X(pushs) X(pushb) X(pushsp) X(puship) X(pushbp) \
	X(pushspadd) X(pushspsub) X(pushbpadd) X(pushbpsub) \
	X(popq) X(popl) X(pops) X(popb) X(popsp) X(popip) X(popbp) \
	X(storespq) X(storespl) X(storesps) X(storespb) \
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
	X(call) X(calls) X(ret) X(retx) X(reset) \
	X(pushnataddr) X(callnat) X(callnats) \
	X(nop) \

#define X(x) x,
enum InstrSet { INSTR_SET };
#undef X

// This is strictly for long doubles
static inline void _TaghaScript_get_immn(Script_t *restrict script, void *restrict pBuffer, const Word_t bytesize)
{
	if( !script or !pBuffer )
		return;
	
	uchar bytes[bytesize];
	Word_t i = bytesize-1;
	while( i<bytesize )
		((uchar *)pBuffer)[i--] = script->pInstrStream[++script->ip];
}

/*
 * Private, inlined implementation of getting values from
 * the instruction stream to help with speeding up code.
 */ 
static inline u64 _TaghaScript_get_imm8(Script_t *restrict script)
{
	if( !script )
		return 0L;
	if( script->bSafeMode and (script->ip+8) >= script->uiInstrSize ) {
		printf("_TaghaScript_get_imm8 reported: instr overflow! Current instruction address: %" PRIu32 "\n", script->ip);
		return 0L;
	}
	union conv_union conv;
	conv.c[0] = script->pInstrStream[++script->ip];
	conv.c[1] = script->pInstrStream[++script->ip];
	conv.c[2] = script->pInstrStream[++script->ip];
	conv.c[3] = script->pInstrStream[++script->ip];
	conv.c[4] = script->pInstrStream[++script->ip];
	conv.c[5] = script->pInstrStream[++script->ip];
	conv.c[6] = script->pInstrStream[++script->ip];
	conv.c[7] = script->pInstrStream[++script->ip];
	return conv.ull;
}

static inline uint _TaghaScript_get_imm4(Script_t *restrict script)
{
	if( !script )
		return 0;
	if( script->bSafeMode and (script->ip+4) >= script->uiInstrSize ) {
		printf("_TaghaScript_get_imm4 reported: instr overflow! Current instruction address: %" PRIu32 "\n", script->ip);
		return 0;
	}
	union conv_union conv;
	//	0x0A,0x0B,0x0C,0x0D,
	conv.c[0] = script->pInstrStream[++script->ip];
	conv.c[1] = script->pInstrStream[++script->ip];
	conv.c[2] = script->pInstrStream[++script->ip];
	conv.c[3] = script->pInstrStream[++script->ip];
	return conv.ui;
}

static inline ushort _TaghaScript_get_imm2(Script_t *restrict script)
{
	if( !script )
		return 0;
	if( script->bSafeMode and (script->ip+2) >= script->uiInstrSize ) {
		printf("_TaghaScript_get_imm2 reported: instr overflow! Current instruction address: %" PRIu32 "\n", script->ip);
		return 0;
	}
	union conv_union conv;
	conv.c[0] = script->pInstrStream[++script->ip];
	conv.c[1] = script->pInstrStream[++script->ip];
	return conv.us;
}


/*
 * private, inlined versions of the Tagha API for optimization
 */

static inline void _TaghaScript_push_int64(Script_t *restrict script, const u64 val)
{
	if( !script )
		return;
	uint size = sizeof(u64);
	if( script->bSafeMode and (script->sp-size) >= script->uiMemsize ) {
		printf("_TaghaScript_push_int64 reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp);
		return;
	}
	script->sp -= size;
	*(u64 *)(script->pbMemory + script->sp) = val;
}
static inline u64 _TaghaScript_pop_int64(Script_t *script)
{
	if( !script )
		return 0L;
	uint size = sizeof(u64);
	if( script->bSafeMode and (script->sp+size) >= script->uiMemsize ) {
		printf("_TaghaScript_pop_int64 reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp);
		return 0L;
	}
	u64 val = *(u64 *)(script->pbMemory + script->sp);
	script->sp += 8;
	return val;
}

static inline void _TaghaScript_push_float64(Script_t *restrict script, const double val)
{
	if( !script )
		return;
	uint size = sizeof(double);
	if( script->bSafeMode and (script->sp-size) >= script->uiMemsize ) {
		printf("_TaghaScript_push_float64 reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp);
		return;
	}
	script->sp -= size;
	*(double *)(script->pbMemory + script->sp) = val;
}
static inline double _TaghaScript_pop_float64(Script_t *script)
{
	if( !script )
		return 0;
	uint size = sizeof(double);
	if( script->bSafeMode and (script->sp+size) >= script->uiMemsize ) {
		printf("_TaghaScript_pop_float64 reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp);
		return 0;
	}
	double val = *(double *)(script->pbMemory + script->sp);
	script->sp += size;
	return val;
}

static inline void _TaghaScript_push_int32(Script_t *restrict script, const uint val)
{
	if( !script )
		return;
	uint size = sizeof(uint);
	if( script->bSafeMode and (script->sp-size) >= script->uiMemsize ) {
		printf("_TaghaScript_push_int32 reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp);
		return;
	}
	script->sp -= size;
	*(uint *)(script->pbMemory + script->sp) = val;
}
static inline uint _TaghaScript_pop_int32(Script_t *script)
{
	if( !script )
		return 0;
	uint size = sizeof(uint);
	if( script->bSafeMode and (script->sp+size) >= script->uiMemsize ) {	// we're subtracting, did we integer underflow?
		printf("_TaghaScript_pop_int32 reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp);
		return 0;
	}
	uint val = *(uint *)(script->pbMemory + script->sp);
	script->sp += size;
	return val;
}

static inline void _TaghaScript_push_float32(Script_t *restrict script, const float val)
{
	if( !script )
		return;
	uint size = sizeof(float);
	if( script->bSafeMode and (script->sp-size) >= script->uiMemsize ) {
		printf("_TaghaScript_push_float32 reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp);
		return;
	}
	script->sp -= size;
	*(float *)(script->pbMemory + script->sp) = val;
}
static inline float _TaghaScript_pop_float32(Script_t *script)
{
	if( !script )
		return 0;
	uint size = sizeof(float);
	if( script->bSafeMode and (script->sp+size) >= script->uiMemsize ) {
		printf("_TaghaScript_pop_float32 reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp);
		return 0;
	}
	float val = *(float *)(script->pbMemory + script->sp);
	script->sp += size;
	return val;
}

static inline void _TaghaScript_push_short(Script_t *restrict script, const ushort val)
{
	if( !script )
		return;
	uint size = sizeof(ushort);
	if( script->bSafeMode and (script->sp-size) >= script->uiMemsize ) {
		printf("_TaghaScript_push_short reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp);
		return;
	}
	script->sp -= size;
	*(ushort *)(script->pbMemory + script->sp) = val;
}
static inline ushort _TaghaScript_pop_short(Script_t *script)
{
	if( !script )
		return 0;
	uint size = sizeof(ushort);
	if( script->bSafeMode and (script->sp+size) >= script->uiMemsize ) {
		printf("_TaghaScript_pop_short reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp);
		return 0;
	}
	ushort val = *(ushort *)(script->pbMemory + script->sp);
	script->sp += size;
	return val;
}

static inline void _TaghaScript_push_byte(Script_t *restrict script, const uchar val)
{
	if( !script )
		return;
	uint size = sizeof(uchar);
	if( script->bSafeMode and (script->sp-size) >= script->uiMemsize ) {
		printf("_TaghaScript_push_byte reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp);
		return;
	}
	script->sp -= size;
	*(script->pbMemory + script->sp) = val;
}
static inline uchar _TaghaScript_pop_byte(Script_t *script)
{
	if( !script )
		return 0;
	uint size = sizeof(uchar);
	if( script->bSafeMode and (script->sp+size) >= script->uiMemsize ) {
		printf("_TaghaScript_pop_byte reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp);
		return 0;
	}
	uchar val = *(script->pbMemory + script->sp);
	script->sp += size;
	return val;
}

static inline void _TaghaScript_push_nbytes(Script_t *restrict script, void *restrict pItem, const Word_t bytesize)
{
	if( !script )
		return;
	if( script->bSafeMode and (script->sp-bytesize) >= script->uiMemsize ) {
		printf("_TaghaScript_push_nbytes reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp);
		return;
	}
	Word_t i=0;
	//for( i=0 ; i<bytesize ; i++ )
	for( i=bytesize-1 ; i<bytesize ; i-- )
		script->pbMemory[--script->sp] = ((uchar *)pItem)[i];
}
static inline void _TaghaScript_pop_nbytes(Script_t *restrict script, void *restrict pBuffer, const Word_t bytesize)
{
	if( !script )
		return;
	if( script->bSafeMode and (script->sp+bytesize) >= script->uiMemsize ) {
		printf("_TaghaScript_pop_nbytes reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp);
		return;
	}
	Word_t i=0;
	// should stop when the integer underflows
	//for( i=bytesize-1 ; i<bytesize ; i-- )
	for( i=0 ; i<bytesize ; i++ )
		((uchar *)pBuffer)[i] = script->pbMemory[script->sp++];
}


static inline u64 _TaghaScript_peek_int64(Script_t *script)
{
	if( !script )
		return 0;
	if( script->bSafeMode and (script->sp+7) >= script->uiMemsize ) {
		printf("Tagha_peek_int32 reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+7);
		return 0;
	}
	return *(u64 *)( script->pbMemory + script->sp );
}
static inline double _TaghaScript_peek_float64(Script_t *script)
{
	if( !script )
		return 0;
	if( script->bSafeMode and (script->sp+7) >= script->uiMemsize ) {
		printf("Tagha_peek_int32 reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+7);
		return 0;
	}
	return *(double *)( script->pbMemory + script->sp );
}
static inline uint _TaghaScript_peek_int32(Script_t *script)
{
	if( !script )
		return 0;
	if( script->bSafeMode and (script->sp+3) >= script->uiMemsize ) {
		printf("Tagha_peek_int32 reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+3);
		return 0;
	}
	return *(uint *)( script->pbMemory + script->sp );
}

static inline float _TaghaScript_peek_float32(Script_t *script)
{
	if( !script )
		return 0;
	if( script->bSafeMode and (script->sp+3) >= script->uiMemsize ) {	// we're subtracting, did we integer underflow?
		printf("Tagha_peek_float32 reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+3);
		return 0;
	}
	return *(float *)( script->pbMemory + script->sp );
}

static inline ushort _TaghaScript_peek_short(Script_t *script)
{
	if( !script )
		return 0;
	if( script->bSafeMode and (script->sp+1) >= script->uiMemsize ) {
		printf("Tagha_peek_short reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+1);
		return 0;
	}
	return *(ushort *)( script->pbMemory + script->sp );
}

static inline uchar _TaghaScript_peek_byte(Script_t *script)
{
	if( !script )
		return 0;
	if( script->bSafeMode and (script->sp) >= script->uiMemsize ) {
		printf("Tagha_peek_byte reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp);
		return 0;
	}
	return script->pbMemory[script->sp];
}



//#include <unistd.h>	// sleep() func
void Tagha_exec(TaghaVM_t *vm)
{
	//printf("instruction set size == %" PRIu32 "\n", nop);
	if( !vm )
		return;
	else if( !vm->pvecScripts )
		return;
	
	uint nScripts = vector_count(vm->pvecScripts);
	Script_t *script = NULL;
	
	// our value temporaries
	union conv_union conv, convb;
	uint	b,a;
	u64		qb,qa;
	double	db,da;
	float	fb,fa;
	ushort	sb,sa;
	uchar	cb,ca;
	long double	ldb, lda;
	fnNative_t pfNative;
	bool safemode;
	bool debugmode;
	
#define X(x) #x ,
	// for debugging purposes.
	const char *opcode2str[] = { INSTR_SET };
#undef X
	
#define X(x) &&exec_##x ,
	// our instruction dispatch table.
	static const void *dispatch[] = { INSTR_SET };
#undef X
#undef INSTR_SET
	
	
#ifdef _UNISTD_H
	#define DISPATCH()	sleep(1); ++script->ip; continue
#else
	#define DISPATCH()	++script->ip; continue
#endif
	
	uint x;
	for( x=0 ; x<nScripts ; x++ ) {
		script=vector_get(vm->pvecScripts, x);
		if( !script )
			continue;
		else if( !script->pInstrStream )
			continue;
		
		while( 1 ) {
			script->uiMaxInstrs--;
			if( !script->uiMaxInstrs )
				break;
			
			safemode = script->bSafeMode;
			debugmode = script->bDebugMode;
			if( safemode ) {
				if( script->ip >= script->uiInstrSize ) {
					printf("instruction address out of bounds! instruction == \'%" PRIu32 "\'\n", script->ip);
					goto *dispatch[halt];
				}
				else if( script->pInstrStream[script->ip] > nop ) {
					printf("illegal instruction exception! instruction == \'%" PRIu32 "\' @ %" PRIu32 "\n", script->pInstrStream[script->ip], script->ip);
					goto *dispatch[halt];
				}
			}
			//printf( "current instruction == \"%s\" @ ip == %" PRIu32 "\n", opcode2str[script->pInstrStream[script->ip]], script->ip );
			goto *dispatch[ script->pInstrStream[script->ip] ];
			
			exec_nop:;
				DISPATCH();
			
			exec_halt:;
				printf("========================= [script done] =========================\n\n");
				if( debugmode )
					TaghaScript_debug_print_memory(script);
				break;
			
			exec_pushq:;
				conv.ull = _TaghaScript_get_imm8(script);
				if( debugmode )
					printf("pushq: pushed %" PRIu64 "\n", conv.ull);
				_TaghaScript_push_int64(script, conv.ull);
				DISPATCH();
			
			exec_pushl:;	// push 4 bytes onto the stack
				conv.ui = _TaghaScript_get_imm4(script);
				if( debugmode )
					printf("pushl: pushed %" PRIu32 "\n", conv.ui);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_pushs:;	// push 2 bytes onto the stack
				conv.us = _TaghaScript_get_imm2(script);
				_TaghaScript_push_short(script, conv.us);
				if( debugmode )
					printf("pushs: pushed %" PRIu32 "\n", conv.us);
				DISPATCH();
			
			exec_pushb:;	// push a byte onto the stack
				_TaghaScript_push_byte(script, script->pInstrStream[++script->ip]);
				if( debugmode )
					printf("pushb: pushed %" PRIu32 "\n", script->pbMemory[script->sp]);
				DISPATCH();
			
			exec_pushsp:;	// push sp onto the stack, uses 4 bytes since 'sp' is uint32
				conv.ui = script->sp;
				_TaghaScript_push_int32(script, conv.ui);
				if( debugmode )
					printf("pushsp: pushed sp index: %" PRIu32 "\n", conv.ui);
				DISPATCH();
			
			exec_puship:;
				conv.ui = script->ip;
				_TaghaScript_push_int32(script, conv.ui);
				if( debugmode )
					printf("puship: pushed ip index: %" PRIu32 "\n", conv.ui);
				DISPATCH();
			
			exec_pushbp:;
				_TaghaScript_push_int32(script, script->bp);
				if( debugmode )
					printf("pushbp: pushed bp index: %" PRIu32 "\n", script->bp);
				DISPATCH();
			
			exec_pushspadd:;
				a = script->sp;
				b = _TaghaScript_pop_int32(script);
				_TaghaScript_push_int32(script, a+b);
				if( debugmode )
					printf("pushspadd: added sp with %" PRIu32 ", result: %" PRIu32 "\n", b, a+b);
				DISPATCH();
				
			exec_pushspsub:;
				a = script->sp;
				b = _TaghaScript_pop_int32(script);
				_TaghaScript_push_int32(script, a-b);
				if( debugmode )
					printf("pushspsub: subbed sp with %" PRIu32 ", result: %" PRIu32 "\n", b, a-b);
				DISPATCH();
			
			exec_pushbpadd:;
				a = script->bp;
				b = _TaghaScript_pop_int32(script);
				_TaghaScript_push_int32(script, a+b);
				if( debugmode )
					printf("pushbpadd: added bp with %" PRIu32 ", result: %" PRIu32 "\n", b, a+b);
				DISPATCH();
			
			exec_pushbpsub:;
				a = script->bp;
				b = _TaghaScript_pop_int32(script);
				_TaghaScript_push_int32(script, a-b);
				if( debugmode )
					printf("pushbpsub: subbed bp with %" PRIu32 ", result: %" PRIu32 "\n", b, a-b);
				DISPATCH();
			
			exec_popq:;
				if( safemode and (script->sp+8) >= script->uiMemsize ) {
					printf("exec_popq reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+8);
					goto *dispatch[halt];
				}
				script->sp += 8;
				if( debugmode )
					printf("popq\n");
				DISPATCH();
			
			exec_popl:;		// pop 4 bytes to eventually be overwritten
				if( safemode and (script->sp+4) >= script->uiMemsize ) {
					printf("exec_popl reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+4);
					goto *dispatch[halt];
				}
				script->sp += 4;
				if( debugmode )
					printf("popl\n");
				DISPATCH();
			
			exec_pops:;		// pop 2 bytes
				if( safemode and (script->sp+2) >= script->uiMemsize ) {
					printf("exec_pops reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+2);
					goto *dispatch[halt];
				}
				script->sp += 2;
				if( debugmode )
					printf("pops\n");
				DISPATCH();
			
			exec_popb:;		// pop a byte
				if( safemode and (script->sp+1) >= script->uiMemsize ) {
					printf("exec_popb reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+1);
					goto *dispatch[halt];
				}
				script->sp++;
				if( debugmode )
					printf("popb\n");
				DISPATCH();
				
			exec_popsp:;
				script->sp = _TaghaScript_pop_int32(script);
				if( safemode )
					printf("popsp: sp is now %" PRIu32 " bytes.\n", script->sp);
				DISPATCH();
				
			exec_popbp:;
				script->bp = _TaghaScript_pop_int32(script);
				if( debugmode )
					printf("popbp: bp is now %" PRIu32 " bytes.\n", script->bp);
				DISPATCH();
				
			exec_popip:;
				script->ip = _TaghaScript_pop_int32(script);
				if( debugmode )
					printf("popip: ip is now at address: %" PRIu32 ".\n", script->ip);
				continue;
			
			exec_loadspq:;
				a = _TaghaScript_pop_int32(script);
				if( safemode and (a+7) >= script->uiMemsize ) {
					printf("exec_loadspq reported: Invalid memory access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, a+7);
					goto *dispatch[halt];
				}
				conv.ull = *(u64 *)(script->pbMemory + a);
				_TaghaScript_push_int64(script, conv.ull);
				if( debugmode )
					printf("loaded 8-byte SP address data to T.O.S. - %" PRIu64 " from sp address [%"PRIu32"]\n", conv.ull, a);
				DISPATCH();
			
			exec_loadspl:;
				a = _TaghaScript_pop_int32(script);
				if( safemode and (a+3) >= script->uiMemsize ) {
					printf("exec_loadspl reported: Invalid memory access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, a+3);
					goto *dispatch[halt];
				}
				conv.ui = *(uint *)(script->pbMemory + a);
				_TaghaScript_push_int32(script, conv.ui);
				if( debugmode )
					printf("loaded 4-byte SP address data to T.O.S. - %" PRIu32 " from sp address [%"PRIu32"]\n", conv.ui, a);
				DISPATCH();
			
			exec_loadsps:;
				a = _TaghaScript_pop_int32(script);
				if( safemode and (a+1) >= script->uiMemsize ) {
					printf("exec_loadsps reported: Invalid memory access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, a+1);
					goto *dispatch[halt];
				}
				conv.us = *(ushort *)(script->pbMemory + a);
				_TaghaScript_push_short(script, conv.us);
				if( debugmode )
					printf("loaded 2-byte SP address data to T.O.S. - %" PRIu32 " from sp address [%"PRIu32"]\n", conv.us, a);
				DISPATCH();
			
			exec_loadspb:;
				a = _TaghaScript_pop_int32(script);
				if( safemode and a >= script->uiMemsize ) {
					printf("exec_loadspb reported: Invalid memory access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, a);
					goto *dispatch[halt];
				}
				conv.c[0] = script->pbMemory[a];
				_TaghaScript_push_byte(script, conv.c[0]);
				if( debugmode )
					printf("loaded byte SP address data to T.O.S. - %" PRIu32 " from sp address [%"PRIu32"]\n", conv.c[0], a);
				DISPATCH();
			
			exec_storespq:;
				a = _TaghaScript_pop_int32(script);
				if( safemode and a+7 >= script->uiMemsize ) {
					printf("exec_storespq reported: Invalid memory access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, a+7);
					goto *dispatch[halt];
				}
				conv.ull = _TaghaScript_pop_int64(script);
				*(u64 *)(script->pbMemory + a) = conv.ull;
				if( debugmode )
					printf("stored 8-byte data from T.O.S. - %" PRIu64 " to sp address [%"PRIu32"]\n", conv.ull, a);
				DISPATCH();
			
			exec_storespl:;		// store TOS into another part of the data stack.
				a = _TaghaScript_pop_int32(script);
				if( safemode and a+3 >= script->uiMemsize ) {
					printf("exec_storespl reported: Invalid memory access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, a+3);
					goto *dispatch[halt];
				}
				conv.ui = _TaghaScript_pop_int32(script);
				*(uint *)(script->pbMemory + a) = conv.ui;
				if( debugmode )
					printf("stored 4-byte data from T.O.S. - %" PRIu32 " to sp address [%"PRIu32"]\n", conv.ui, a);
				DISPATCH();
			
			exec_storesps:;
				a = _TaghaScript_pop_int32(script);
				if( safemode and a+1 >= script->uiMemsize ) {
					printf("exec_storesps reported: Invalid memory access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, a+1);
					goto *dispatch[halt];
				}
				conv.us = _TaghaScript_pop_short(script);
				*(ushort *)(script->pbMemory + a) = conv.ui;
				if( debugmode )
					printf("stored 2-byte data from T.O.S. - %" PRIu32 " to sp address [%"PRIu32"]\n", conv.us, a);
				DISPATCH();
			
			exec_storespb:;
				a = _TaghaScript_pop_int32(script);
				if( safemode and a >= script->uiMemsize ) {
					printf("exec_storespb reported: Invalid memory access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, a);
					goto *dispatch[halt];
				}
				script->pbMemory[a] = _TaghaScript_pop_byte(script);
				if( debugmode )
					printf("stored byte data from T.O.S. - %" PRIu32 " to sp address [%"PRIu32"]\n", script->pbMemory[a], a);
				DISPATCH();
			
			exec_copyq:;
				if( safemode and script->sp+7 >= script->uiMemsize ) {
					printf("exec_copyq reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+7);
					goto *dispatch[halt];
				}
				conv.ull = *(u64 *)(script->pbMemory + script->sp);
				if( debugmode )
					printf("copied 8-byte data from T.O.S. - %" PRIu64 "\n", conv.ull);
				_TaghaScript_push_int64(script, conv.ull);
				DISPATCH();
			
			exec_copyl:;	// copy 4 bytes of top of stack and put as new top of stack.
				if( safemode and script->sp+3 >= script->uiMemsize ) {
					printf("exec_copyl reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+3);
					goto *dispatch[halt];
				}
				conv.ui = *(uint *)(script->pbMemory + script->sp);
				if( debugmode )
					printf("copied 4-byte data from T.O.S. - %" PRIu32 "\n", conv.ui);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_copys:;
				if( safemode and script->sp+1 >= script->uiMemsize ) {
					printf("exec_copys reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+1);
					goto *dispatch[halt];
				}
				conv.us = *(ushort *)(script->pbMemory + script->sp);
				_TaghaScript_push_short(script, conv.us);
				if( debugmode )
					printf("copied 2-byte data from T.O.S. - %" PRIu32 "\n", conv.us);
				DISPATCH();
			
			exec_copyb:;
				_TaghaScript_push_byte(script, script->pbMemory[script->sp]);
				DISPATCH();
			
			exec_addq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				conv.ll = (i64)qa + (i64)qb;
				if( debugmode )
					printf("signed 8 byte addition result: %" PRIi64 " == %" PRIi64 " + %" PRIi64 "\n", conv.ll, qa,qb);
				_TaghaScript_push_int64(script, conv.ull);
				DISPATCH();
				
			exec_uaddq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				conv.ull = qa+qb;
				if( debugmode )
					printf("unsigned 8 byte addition result: %" PRIu64 " == %" PRIu64 " + %" PRIu64 "\n", conv.ull, qa,qb);
				_TaghaScript_push_int64(script, conv.ull);
				DISPATCH();
			
			exec_addl:;		// pop 8 bytes, signed addition, and push 4 byte result to top of stack
				b = _TaghaScript_pop_int32(script);
				a = _TaghaScript_pop_int32(script);
				conv.i = (int)a + (int)b;
				if( debugmode )
					printf("signed 4 byte addition result: %" PRIi32 " == %" PRIi32 " + %" PRIi32 "\n", conv.i, a,b);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_uaddl:;	// In C, all integers in an expression are promoted to int32, if number is bigger then uint32 or int64
				b = _TaghaScript_pop_int32(script);
				a = _TaghaScript_pop_int32(script);
				conv.ui = a+b;
				if( debugmode )
					printf("unsigned 4 byte addition result: %" PRIu32 " == %" PRIu32 " + %" PRIu32 "\n", conv.ui, a,b);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_addf:;
				fb = _TaghaScript_pop_float32(script);
				fa = _TaghaScript_pop_float32(script);
				conv.f = fa+fb;
				if( debugmode )
					printf("4-byte float addition result: %f == %f + %f\n", conv.f, fa,fb);
				_TaghaScript_push_float32(script, conv.f);
				DISPATCH();
			
			exec_addf64:;
				db = _TaghaScript_pop_float64(script);
				da = _TaghaScript_pop_float64(script);
				conv.dbl = da+db;
				if( debugmode )
					printf("8-byte float addition result: %f == %f + %f\n", conv.dbl, da,db);
				_TaghaScript_push_float64(script, conv.dbl);
				DISPATCH();
			
			exec_subq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				conv.ll = (i64)qa - (i64)qb;
				if( debugmode )
					printf("signed 8 byte subtraction result: %" PRIi64 " == %" PRIi64 " - %" PRIi64 "\n", conv.ll, qa,qb);
				_TaghaScript_push_int64(script, conv.ull);
				DISPATCH();
			
			exec_usubq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				conv.ull = qa-qb;
				if( debugmode )
					printf("unsigned 8 byte subtraction result: %" PRIu64 " == %" PRIu64 " - %" PRIu64 "\n", conv.ull, qa,qb);
				_TaghaScript_push_int64(script, conv.ull);
				DISPATCH();
			
			exec_subl:;
				b = _TaghaScript_pop_int32(script);
				a = _TaghaScript_pop_int32(script);
				conv.i = (int)a - (int)b;
				if( debugmode )
					printf("signed 4 byte subtraction result: %" PRIi32 " == %" PRIi32 " - %" PRIi32 "\n", conv.ui, a,b);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_usubl:;
				b = _TaghaScript_pop_int32(script);
				a = _TaghaScript_pop_int32(script);
				conv.ui = a-b;
				if( debugmode )
					printf("unsigned 4 byte subtraction result: %" PRIu32 " == %" PRIu32 " - %" PRIu32 "\n", conv.ui, a,b);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_subf:;
				fb = _TaghaScript_pop_float32(script);
				fa = _TaghaScript_pop_float32(script);
				conv.f = fa-fb;
				if( debugmode )
					printf("4-byte float subtraction result: %f == %f - %f\n", conv.f, fa,fb);
				_TaghaScript_push_float32(script, conv.f);
				DISPATCH();
			
			exec_subf64:;
				db = _TaghaScript_pop_float64(script);
				da = _TaghaScript_pop_float64(script);
				conv.dbl = da-db;
				if( debugmode )
					printf("8-byte float subtraction result: %f == %f - %f\n", conv.dbl, da,db);
				_TaghaScript_push_float64(script, conv.dbl);
				DISPATCH();
			
			exec_mulq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				conv.ll = (i64)qa * (i64)qb;
				if( debugmode )
					printf("signed 8 byte mult result: %" PRIi64 " == %" PRIi64 " * %" PRIi64 "\n", conv.ll, qa,qb);
				_TaghaScript_push_int64(script, conv.ull);
				DISPATCH();
			
			exec_umulq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				conv.ull = qa*qb;
				if( debugmode )
					printf("unsigned 8 byte mult result: %" PRIu64 " == %" PRIu64 " * %" PRIu64 "\n", conv.ull, qa,qb);
				_TaghaScript_push_int64(script, conv.ull);
				DISPATCH();
			
			exec_mull:;
				b = _TaghaScript_pop_int32(script);
				a = _TaghaScript_pop_int32(script);
				conv.i = (int)a * (int)b;
				if( debugmode )
					printf("signed 4 byte mult result: %" PRIi32 " == %" PRIi32 " * %" PRIi32 "\n", conv.i, a,b);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_umull:;
				b = _TaghaScript_pop_int32(script);
				a = _TaghaScript_pop_int32(script);
				conv.ui = a*b;
				if( debugmode )
					printf("unsigned 4 byte mult result: %" PRIu32 " == %" PRIu32 " * %" PRIu32 "\n", conv.ui, a,b);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_mulf:;
				fb = _TaghaScript_pop_float32(script);
				fa = _TaghaScript_pop_float32(script);
				conv.f = fa*fb;
				if( debugmode )
					printf("4-byte float mult result: %f == %f * %f\n", conv.f, fa,fb);
				_TaghaScript_push_float32(script, conv.f);
				DISPATCH();
			
			exec_mulf64:;
				db = _TaghaScript_pop_float64(script);
				da = _TaghaScript_pop_float64(script);
				conv.dbl = da*db;
				if( debugmode )
					printf("8-byte float mult result: %f == %f * %f\n", conv.dbl, da,db);
				_TaghaScript_push_float64(script, conv.dbl);
				DISPATCH();
			
			exec_divq:;
				qb = _TaghaScript_pop_int64(script);
				if( !qb ) {
					printf("divq: divide by 0 error.\n");
					qb = 1;
				}
				qa = _TaghaScript_pop_int64(script);
				conv.ll = (i64)qa / (i64)qb;
				if( debugmode )
					printf("signed 8 byte division result: %" PRIi64 " == %" PRIi64 " / %" PRIi64 "\n", conv.ll, qa,qb);
				_TaghaScript_push_int64(script, conv.ull);
				DISPATCH();
			
			exec_udivq:;
				qb = _TaghaScript_pop_int64(script);
				if( !qb ) {
					printf("udivq: divide by 0 error.\n");
					qb = 1;
				}
				qa = _TaghaScript_pop_int64(script);
				conv.ull = qa/qb;
				if( debugmode )
					printf("unsigned 8 byte division result: %" PRIu64 " == %" PRIu64 " / %" PRIu64 "\n", conv.ull, qa,qb);
				_TaghaScript_push_int64(script, conv.ull);
				DISPATCH();
			
			exec_divl:;
				b = _TaghaScript_pop_int32(script);
				if( !b ) {
					printf("divl: divide by 0 error.\n");
					b=1;
				}
				a = _TaghaScript_pop_int32(script);
				conv.i = (int)a / (int)b;
				if( debugmode )
					printf("signed 4 byte division result: %" PRIi32 " == %" PRIi32 " / %" PRIi32 "\n", conv.i, a,b);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_udivl:;
				b = _TaghaScript_pop_int32(script);
				if( !b ) {
					printf("udivl: divide by 0 error.\n");
					b=1;
				}
				a = _TaghaScript_pop_int32(script);
				conv.ui = a/b;
				if( debugmode )
					printf("unsigned 4 byte division result: %" PRIu32 " == %" PRIu32 " / %" PRIu32 "\n", conv.ui, a,b);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_divf:;
				fb = _TaghaScript_pop_float32(script);
				if( !fb ) {
					printf("divf: divide by 0.0 error.\n");
					fb = 1.f;
				}
				fa = _TaghaScript_pop_float32(script);
				conv.f = fa/fb;
				if( debugmode )
					printf("4-byte float division result: %f == %f / %f\n", conv.f, fa,fb);
				_TaghaScript_push_float32(script, conv.f);
				DISPATCH();
			
			exec_divf64:;
				db = _TaghaScript_pop_float64(script);
				if( !db ) {
					printf("divf64: divide by 0.0 error.\n");
					db=1.0;
				}
				da = _TaghaScript_pop_float64(script);
				conv.dbl = da/db;
				if( debugmode )
					printf("8-byte float division result: %f == %f / %f\n", conv.dbl, da,db);
				_TaghaScript_push_float64(script, conv.dbl);
				DISPATCH();
			
			exec_modq:;
				qb = _TaghaScript_pop_int64(script);
				if( !qb ) {
					printf("modq: divide by 0 error.\n");
					qb = 1;
				}
				qa = _TaghaScript_pop_int64(script);
				conv.ll = (i64)qa % (i64)qb;
				if( debugmode )
					printf("signed 8 byte modulo result: %" PRIi64 " == %" PRIi64 " %% %" PRIi64 "\n", conv.ll, qa,qb);
				_TaghaScript_push_int64(script, conv.ull);
				DISPATCH();
			
			exec_umodq:;
				qb = _TaghaScript_pop_int64(script);
				if( !qb ) {
					printf("umodq: divide by 0 error.\n");
					qb = 1;
				}
				qa = _TaghaScript_pop_int64(script);
				conv.ull = qa % qb;
				if( debugmode )
					printf("unsigned 8 byte modulo result: %" PRIu64 " == %" PRIu64 " %% %" PRIu64 "\n", conv.ull, qa,qb);
				_TaghaScript_push_int64(script, conv.ull);
				DISPATCH();
			
			exec_modl:;
				b = _TaghaScript_pop_int32(script);
				if( !b ) {
					printf("modl: divide by 0 error.\n");
					b=1;
				}
				a = _TaghaScript_pop_int32(script);
				conv.i = (int)a % (int)b;
				if( debugmode )
					printf("signed 4 byte modulo result: %" PRIi32 " == %" PRIi32 " %% %" PRIi32 "\n", conv.i, a,b);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_umodl:;
				b = _TaghaScript_pop_int32(script);
				if( !b ) {
					printf("umodl: divide by 0 error.\n");
					b=1;
				}
				a = _TaghaScript_pop_int32(script);
				conv.ui = a % b;
				if( debugmode )
					printf("unsigned 4 byte modulo result: %" PRIu32 " == %" PRIu32 " %% %" PRIu32 "\n", conv.ui, a,b);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_andq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				conv.ull = qa & qb;
				if( safemode )
					printf("8 byte AND result: %" PRIu64 " == %" PRIu64 " & %" PRIu64 "\n", conv.ull, qa,qb);
				_TaghaScript_push_int64(script, conv.ull);
				DISPATCH();
				
			exec_orq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				conv.ull = qa | qb;
				if( debugmode )
					printf("8 byte OR result: %" PRIu64 " == %" PRIu64 " | %" PRIu64 "\n", conv.ull, qa,qb);
				_TaghaScript_push_int64(script, conv.ull);
				DISPATCH();
				
			exec_xorq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				conv.ull = qa ^ qb;
				if( debugmode )
					printf("8 byte XOR result: %" PRIu64 " == %" PRIu64 " ^ %" PRIu64 "\n", conv.ull, qa,qb);
				_TaghaScript_push_int64(script, conv.ull);
				DISPATCH();
				
			exec_notq:;
				qa = _TaghaScript_pop_int64(script);
				conv.ull = ~qa;
				if( debugmode )
					printf("8 byte NOT result: %" PRIu64 "\n", conv.ull);
				_TaghaScript_push_int64(script, conv.ull);
				DISPATCH();
				
			exec_shlq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				conv.ull = qa << qb;
				if( debugmode )
					printf("8 byte Shift Left result: %" PRIu64 " == %" PRIu64 " << %" PRIu64 "\n", conv.ull, qa,qb);
				_TaghaScript_push_int64(script, conv.ull);
				DISPATCH();
				
			exec_shrq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				conv.ull = qa >> qb;
				if( debugmode )
					printf("8 byte Shift Right result: %" PRIu64 " == %" PRIu64 " >> %" PRIu64 "\n", conv.ull, qa,qb);
				_TaghaScript_push_int64(script, conv.ull);
				DISPATCH();
				
			exec_andl:;
				b = _TaghaScript_pop_int32(script);
				a = _TaghaScript_pop_int32(script);
				conv.ui = a & b;
				if( debugmode )
					printf("4 byte AND result: %" PRIu32 " == %" PRIu32 " & %" PRIu32 "\n", conv.ui, a,b);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_orl:;
				b = _TaghaScript_pop_int32(script);
				a = _TaghaScript_pop_int32(script);
				conv.ui = a | b;
				if( debugmode )
					printf("4 byte OR result: %" PRIu32 " == %" PRIu32 " | %" PRIu32 "\n", conv.ui, a,b);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_xorl:;
				b = _TaghaScript_pop_int32(script);
				a = _TaghaScript_pop_int32(script);
				conv.ui = a ^ b;
				if( debugmode )
					printf("4 byte XOR result: %" PRIu32 " == %" PRIu32 " ^ %" PRIu32 "\n", conv.ui, a,b);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_notl:;
				a = _TaghaScript_pop_int32(script);
				conv.ui = ~a;
				if( debugmode )
					printf("4 byte NOT result: %" PRIu32 "\n", conv.ui);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_shll:;
				b = _TaghaScript_pop_int32(script);
				a = _TaghaScript_pop_int32(script);
				conv.ui = a << b;
				if( debugmode )
					printf("4 byte Shift Left result: %" PRIu32 " == %" PRIu32 " << %" PRIu32 "\n", conv.ui, a,b);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_shrl:;
				b = _TaghaScript_pop_int32(script);
				a = _TaghaScript_pop_int32(script);
				conv.ui = a >> b;
				if( debugmode )
					printf("4 byte Shift Right result: %" PRIu32 " == %" PRIu32 " >> %" PRIu32 "\n", conv.ui, a,b);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_incq:;
				qa = _TaghaScript_pop_int64(script);
				++qa;
				if( debugmode )
					printf("8 byte Increment result: %" PRIu64 "\n", qa);
				_TaghaScript_push_int64(script, qa);
				DISPATCH();
				
			exec_incl:;
				a = _TaghaScript_pop_int32(script);
				++a;
				if( debugmode )
					printf("4 byte Increment result: %" PRIu32 "\n", a);
				_TaghaScript_push_int32(script, a);
				DISPATCH();
			
			exec_incf:;
				fa = _TaghaScript_pop_float32(script);
				++fa;
				if( debugmode )
					printf("4-byte float Increment result: %f\n", fa);
				_TaghaScript_push_float32(script, fa);
				DISPATCH();
				
			exec_incf64:;
				da = _TaghaScript_pop_float64(script);
				++da;
				if( debugmode )
					printf("8-byte float Increment result: %f\n", da);
				_TaghaScript_push_float64(script, da);
				DISPATCH();
			
			exec_decq:;
				qa = _TaghaScript_pop_int64(script);
				--qa;
				if( debugmode )
					printf("8 byte Decrement result: %" PRIu64 "\n", qa);
				_TaghaScript_push_int64(script, qa);
				DISPATCH();
				
			exec_decl:;
				a = _TaghaScript_pop_int32(script);
				--a;
				if( debugmode )
					printf("4 byte Decrement result: %" PRIu32 "\n", a);
				_TaghaScript_push_int32(script, a);
				DISPATCH();
			
			exec_decf:;
				fa = _TaghaScript_pop_float32(script);
				--fa;
				if( debugmode )
					printf("4-byte float Decrement result: %f\n", fa);
				_TaghaScript_push_float32(script, fa);
				DISPATCH();
			
			exec_decf64:;
				da = _TaghaScript_pop_float64(script);
				--da;
				if( debugmode )
					printf("8-byte float Decrement result: %f\n", da);
				_TaghaScript_push_float64(script, da);
				DISPATCH();
			
			exec_negq:;
				qa = _TaghaScript_pop_int64(script);
				conv.ull = -qa;
				if( debugmode )
					printf("8 byte Negate result: %" PRIu64 "\n", conv.ull);
				_TaghaScript_push_int64(script, conv.ull);
				DISPATCH();
			
			exec_negl:;
				a = _TaghaScript_pop_int32(script);
				conv.ui = -a;
				if( debugmode )
					printf("4 byte Negate result: %" PRIu32 "\n", conv.ui);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
				
			exec_negf:;
				fa = _TaghaScript_pop_float32(script);
				conv.f = -fa;
				if( debugmode )
					printf("4-byte float Negate result: %f\n", conv.f);
				_TaghaScript_push_float32(script, conv.f);
				DISPATCH();
				
			exec_negf64:;
				da = _TaghaScript_pop_float64(script);
				conv.dbl = -da;
				if( debugmode )
					printf("8-byte float Negate result: %f\n", conv.dbl);
				_TaghaScript_push_float64(script, conv.dbl);
				DISPATCH();
			
			exec_ltq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				conv.ui = (i64)qa < (i64)qb;
				if( debugmode )
					printf("signed 8 byte Less Than result: %" PRIu32 " == %" PRIi64 " < %" PRIi64 "\n", conv.ui, qa,qb);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_ultq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				conv.ui = qa < qb;
				if( debugmode )
					printf("unsigned 8 byte Less Than result: %" PRIu32 " == %" PRIu64 " < %" PRIu64 "\n", conv.ui, qa,qb);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_ltl:;
				b = _TaghaScript_pop_int32(script);
				a = _TaghaScript_pop_int32(script);
				conv.ui = (int)a < (int)b;
				if( debugmode )
					printf("4 byte Signed Less Than result: %" PRIu32 " == %" PRIi32 " < %" PRIi32 "\n", conv.ui, a,b);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_ultl:;
				b = _TaghaScript_pop_int32(script);
				a = _TaghaScript_pop_int32(script);
				conv.ui = a < b;
				if( debugmode )
					printf("4 byte Unsigned Less Than result: %" PRIu32 " == %" PRIu32 " < %" PRIu32 "\n", conv.ui, a,b);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_ltf:;
				fb = _TaghaScript_pop_float32(script);
				fa = _TaghaScript_pop_float32(script);
				conv.ui = fa < fb;
				if( debugmode )
					printf("4 byte Less Than Float result: %" PRIu32 " == %f < %f\n", conv.ui, fa,fb);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_ltf64:;
				db = _TaghaScript_pop_float64(script);
				da = _TaghaScript_pop_float64(script);
				conv.ui = da < db;
				if( debugmode )
					printf("8 byte Less Than Float result: %" PRIu32 " == %f < %f\n", conv.ui, da,db);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_gtq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				conv.ui = (i64)qa > (i64)qb;
				if( debugmode )
					printf("signed 8 byte Greater Than result: %" PRIu32 " == %" PRIi64 " > %" PRIi64 "\n", conv.ui, qa,qb);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
				
			exec_ugtq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				conv.ui = qa > qb;
				if( debugmode )
					printf("unsigned 8 byte Greater Than result: %" PRIu32 " == %" PRIu64 " > %" PRIu64 "\n", conv.ui, qa,qb);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_gtl:;
				b = _TaghaScript_pop_int32(script);
				a = _TaghaScript_pop_int32(script);
				conv.ui = (int)a > (int)b;
				if( debugmode )
					printf("4 byte Signed Greater Than result: %" PRIu32 " == %" PRIi32 " > %" PRIi32 "\n", conv.ui, a,b);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_ugtl:;
				b = _TaghaScript_pop_int32(script);
				a = _TaghaScript_pop_int32(script);
				conv.ui = a > b;
				if( debugmode )
					printf("4 byte Unigned Greater Than result: %" PRIu32 " == %" PRIu32 " > %" PRIu32 "\n", conv.ui, a,b);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_gtf:;
				fb = _TaghaScript_pop_float32(script);
				fa = _TaghaScript_pop_float32(script);
				conv.ui = fa > fb;
				if( debugmode )
					printf("4 byte Greater Than Float result: %" PRIu32 " == %f > %f\n", conv.ui, fa,fb);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_gtf64:;
				db = _TaghaScript_pop_float64(script);
				da = _TaghaScript_pop_float64(script);
				conv.ui = da > db;
				if( debugmode )
					printf("8 byte Greater Than Float result: %" PRIu32 " == %f > %f\n", conv.ui, da,db);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_cmpq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				conv.ui = (i64)qa == (i64)qb;
				if( debugmode )
					printf("signed 8 byte Compare result: %" PRIu32 " == %" PRIi64 " == %" PRIi64 "\n", conv.ui, qa,qb);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_ucmpq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				conv.ui = qa == qb;
				if( debugmode )
					printf("unsigned 8 byte Compare result: %" PRIu32 " == %" PRIu64 " == %" PRIu64 "\n", conv.ui, qa,qb);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_cmpl:;
				b = _TaghaScript_pop_int32(script);
				a = _TaghaScript_pop_int32(script);
				conv.ui = (int)a == (int)b;
				if( debugmode )
					printf("4 byte Signed Compare result: %" PRIu32 " == %" PRIi32 " == %" PRIi32 "\n", conv.ui, a,b);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_ucmpl:;
				b = _TaghaScript_pop_int32(script);
				a = _TaghaScript_pop_int32(script);
				conv.ui = a == b;
				if( debugmode )
					printf("4 byte Unsigned Compare result: %" PRIu32 " == %" PRIu32 " == %" PRIu32 "\n", conv.ui, a,b);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_compf:;
				fb = _TaghaScript_pop_float32(script);
				fa = _TaghaScript_pop_float32(script);
				conv.ui = fa == fb;
				if( debugmode )
					printf("4 byte Compare Float result: %" PRIu32 " == %f == %f\n", conv.ui, fa,fb);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_cmpf64:;
				db = _TaghaScript_pop_float64(script);
				da = _TaghaScript_pop_float64(script);
				conv.ui = da == db;
				if( debugmode )
					printf("8 byte Compare Float result: %" PRIu32 " == %f == %f\n", conv.ui, da,db);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_leqq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				conv.ui = (i64)qa <= (i64)qb;
				if( debugmode )
					printf("8 byte Signed Less Equal result: %" PRIu32 " == %" PRIi64 " <= %" PRIi64 "\n", conv.ui, qa,qb);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_uleqq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				conv.ui = qa <= qb;
				if( debugmode )
					printf("8 byte Unsigned Less Equal result: %" PRIu32 " == %" PRIu64 " <= %" PRIu64 "\n", conv.ui, qa,qb);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_leql:;
				b = _TaghaScript_pop_int32(script);
				a = _TaghaScript_pop_int32(script);
				conv.ui = (int)a <= (int)b;
				if( debugmode )
					printf("4 byte Signed Less Equal result: %" PRIu32 " == %" PRIi32 " <= %" PRIi32 "\n", conv.ui, a,b);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_uleql:;
				b = _TaghaScript_pop_int32(script);
				a = _TaghaScript_pop_int32(script);
				conv.ui = a <= b;
				if( debugmode )
					printf("4 byte Unsigned Less Equal result: %" PRIu32 " == %" PRIu32 " <= %" PRIu32 "\n", conv.ui, a,b);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_leqf:;
				fb = _TaghaScript_pop_float32(script);
				fa = _TaghaScript_pop_float32(script);
				conv.ui = fa <= fb;
				if( debugmode )
					printf("4 byte Less Equal Float result: %" PRIu32 " == %f <= %f\n", conv.ui, fa, fb);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
				
			exec_leqf64:;
				db = _TaghaScript_pop_float64(script);
				da = _TaghaScript_pop_float64(script);
				conv.ui = da <= db;
				if( debugmode )
					printf("8 byte Less Equal Float result: %" PRIu32 " == %f <= %f\n", conv.ui, da,db);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_geqq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				conv.ui = (i64)qa >= (i64)qb;
				if( debugmode )
					printf("8 byte Signed Greater Equal result: %" PRIu32 " == %" PRIi64 " >= %" PRIi64 "\n", conv.ui, qa,qb);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
				
			exec_ugeqq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				conv.ui = qa >= qb;
				if( debugmode )
					printf("8 byte Unsigned Greater Equal result: %" PRIu32 " == %" PRIu64 " >= %" PRIu64 "\n", conv.ui, qa,qb);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_geql:;
				b = _TaghaScript_pop_int32(script);
				a = _TaghaScript_pop_int32(script);
				conv.ui = (int)a >= (int)b;
				if( debugmode )
					printf("4 byte Signed Greater Equal result: %" PRIu32 " == %" PRIi32 " >= %" PRIi32 "\n", conv.ui, a,b);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_ugeql:;
				b = _TaghaScript_pop_int32(script);
				a = _TaghaScript_pop_int32(script);
				conv.ui = a >= b;
				if( debugmode )
					printf("4 byte Unsigned Greater Equal result: %" PRIu32 " == %" PRIu32 " >= %" PRIu32 "\n", conv.ui, a,b);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_geqf:;
				fb = _TaghaScript_pop_float32(script);
				fa = _TaghaScript_pop_float32(script);
				conv.ui = fa >= fb;
				if( debugmode )
					printf("4 byte Greater Equal Float result: %" PRIu32 " == %f >= %f\n", conv.ui, fa, fb);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_geqf64:;
				db = _TaghaScript_pop_float64(script);
				da = _TaghaScript_pop_float64(script);
				conv.ui = da >= db;
				if( debugmode )
					printf("8 byte Greater Equal Float result: %" PRIu32 " == %f >= %f\n", conv.ui, da,db);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_neqq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				conv.ui = (i64)qa != (i64)qb;
				if( debugmode )
					printf("8 byte Signed Not Equal result: %" PRIu32 " == %" PRIi64 " != %" PRIi64 "\n", conv.ui, qa,qb);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
				
			exec_uneqq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				conv.ui = qa != qb;
				if( debugmode )
					printf("8 byte Unsigned Not Equal result: %" PRIu32 " == %" PRIi64 " != %" PRIi64 "\n", conv.ui, qa,qb);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
				
			exec_neql:;
				b = _TaghaScript_pop_int32(script);
				a = _TaghaScript_pop_int32(script);
				conv.ui = (int)a != (int)b;
				if( debugmode )
					printf("4 byte Signed Not Equal result: %" PRIu32 " == %" PRIi32 " != %" PRIi32 "\n", conv.ui, a,b);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
				
			exec_uneql:;
				b = _TaghaScript_pop_int32(script);
				a = _TaghaScript_pop_int32(script);
				conv.ui = a != b;
				if( debugmode )
					printf("4 byte Unsigned Not Equal result: %" PRIu32 " == %" PRIu32 " != %" PRIu32 "\n", conv.ui, a,b);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_neqf:;
				fb = _TaghaScript_pop_float32(script);
				fa = _TaghaScript_pop_float32(script);
				conv.ui = fa != fb;
				if( debugmode )
					printf("4 byte Not Equal Float result: %" PRIu32 " == %f != %f\n", conv.ui, fa, fb);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_neqf64:;
				db = _TaghaScript_pop_float64(script);
				da = _TaghaScript_pop_float64(script);
				conv.ui = da != db;
				if( debugmode )
					printf("8 byte Not Equal Float result: %" PRIu32 " == %f != %f\n", conv.ui, da,db);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_jmp:;		// addresses are word sized bytes.
				script->ip = _TaghaScript_get_imm4(script);
				if( debugmode )
					printf("jmping to instruction address: %" PRIu32 "\n", script->ip);
				continue;
			
			exec_jzq:;
				if( safemode and script->sp+7 >= script->uiMemsize ) {
					printf("exec_jzq reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+7);
					goto *dispatch[halt];
				}
				qa = *(u64 *)(script->pbMemory + script->sp);
				conv.ui = _TaghaScript_get_imm4(script);
				script->ip = (!qa) ? conv.ui : script->ip+1;
				
				if( debugmode )
					printf("jzq'ing to instruction address: %" PRIu32 "\n", script->ip);
				continue;
			
			exec_jnzq:;
				if( safemode and script->sp+7 >= script->uiMemsize ) {
					printf("exec_jnzq reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+7);
					goto *dispatch[halt];
				}
				qa = *(u64 *)(script->pbMemory + script->sp);
				conv.ui = _TaghaScript_get_imm4(script);
				script->ip = (qa) ? conv.ui : script->ip+1;
				
				if( debugmode )
					printf("jnzq'ing to instruction address: %" PRIu32 "\n", script->ip);
				continue;
			
			exec_jzl:;		// check if the first 4 bytes on stack are zero, if yes then jump it.
				if( safemode and script->sp+3 >= script->uiMemsize ) {
					printf("exec_jzl reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+3);
					goto *dispatch[halt];
				}
				a = *(uint *)(script->pbMemory + script->sp);
				conv.ui = _TaghaScript_get_imm4(script);
				script->ip = (!a) ? conv.ui : script->ip+1;
				
				if( debugmode )
					printf("jzl'ing to instruction address: %" PRIu32 "\n", script->ip);	//opcode2str[script->pInstrStream[script->ip]]
				continue;
			
			exec_jnzl:;
				if( safemode and script->sp+3 >= script->uiMemsize ) {
					printf("exec_jnzl reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+3);
					goto *dispatch[halt];
				}
				a = *(uint *)(script->pbMemory + script->sp);
				conv.ui = _TaghaScript_get_imm4(script);
				script->ip = (a) ? conv.ui : script->ip+1;
				
				if( debugmode )
					printf("jnzl'ing to instruction address: %" PRIu32 "\n", script->ip);
				continue;
			
			exec_call:;		// support functions
				conv.ui = _TaghaScript_get_imm4(script);	// get func address
				if( debugmode )
					printf("call :: calling address: %" PRIu32 "\n", conv.ui);
					
				_TaghaScript_push_int32(script, script->ip+1);	// save return address.
				if( debugmode )
					printf("call :: return addr: %" PRIu32 "\n", script->ip+1);
					
				script->ip = conv.ui;	// jump to instruction
					
				_TaghaScript_push_int32(script, script->bp);	// push ebp;
				if( debugmode )
					printf("call :: pushing bp: %" PRIu32 "\n", script->bp);
				script->bp = script->sp;	// mov ebp, esp;
				
				if( debugmode )
					printf("call :: bp set to sp: %" PRIu32 "\n", script->bp);
				continue;
			
			exec_calls:;	// support local function pointers
				conv.ui = _TaghaScript_pop_int32(script);	// get func address
				if( debugmode )
					printf("calls: calling address: %" PRIu32 "\n", conv.ui);
					
				_TaghaScript_push_int32(script, script->ip+1);	// save return address.
				
				if( debugmode )
					printf("call return addr: %" PRIu32 "\n", _TaghaScript_peek_int32(script));
					
				_TaghaScript_push_int32(script, script->bp);	// push ebp
				script->bp = script->sp;	// mov ebp, esp
				script->ip = conv.ui;	// jump to instruction
				
				if( debugmode )
					printf("script->bp: %" PRIu32 "\n", script->sp);
				continue;
			
			exec_ret:;
				script->sp = script->bp;	// mov esp, ebp
				if( debugmode )
					printf("ret ;: sp set to bp, sp == %" PRIu32 "\n", script->sp);
					
				script->bp = _TaghaScript_pop_int32(script);	// pop ebp
				if( debugmode )
					printf("ret ;: popped to bp, bp == %" PRIu32 "\n", script->bp);
					
				script->ip = _TaghaScript_pop_int32(script);	// pop return address.
				if( debugmode )
					printf("returning to address: %" PRIu32 "\n", script->ip);
				continue;
			
			exec_retx:; {	// for functions that return something.
				a = _TaghaScript_get_imm4(script);
				uchar bytebuffer[a];
				if( debugmode )
					printf("retx :: popping %" PRIu32 " to return\n", a);
				/* This opcode assumes all the data for return
				 * is on the near top of stack. In theory, you can
				 * use this method to return multiple pieces of data.
				 */
				_TaghaScript_pop_nbytes(script, bytebuffer, a);	// store our needed data to a buffer.
				if( debugmode )
					printf("retx :: popped %" PRIu32 " bytes\n", a);
				// do our usual return code.
				script->sp = script->bp;	// mov esp, ebp
				
				if( debugmode )
					printf("retx :: sp set to bp, sp == %" PRIu32 "\n", script->sp);
					
				script->bp = _TaghaScript_pop_int32(script);	// pop ebp
				if( debugmode )
					printf("retx :: popping bp, bp == %" PRIu32 "\n", script->bp);
					
				script->ip = _TaghaScript_pop_int32(script);	// pop return address.
				if( debugmode )
					printf("retx :: popping ip, bp == %" PRIu32 "\n", script->ip);
					
				_TaghaScript_push_nbytes(script, bytebuffer, a);	// push back return data!
				if( debugmode )
					printf("retx :: pushed back %" PRIu32 " bytes\n", a);
				continue;
			}
			
			exec_pushnataddr:;
				if( safemode and !script->ppstrNatives ) {
					printf("exec_pushnataddr reported: native table is NULL! Current instruction address: %" PRIu32 "\n", script->ip);
					goto *dispatch[halt];
				}
				// match native name to get an index.
				a = _TaghaScript_get_imm4(script);
				if( safemode and a >= script->uiNatives  ) {
					printf("exec_pushnataddr reported: native index \'%" PRIu32 "\' is out of bounds! Current instruction address: %" PRIu32 "\n", a, script->ip);
					goto *dispatch[halt];
				}
				pfNative = (fnNative_t) dict_find(vm->pmapNatives, script->ppstrNatives[a]);
				if( safemode and !pfNative ) {
					printf("exec_pushnataddr reported: native \'%s\' not registered! Current instruction address: %" PRIu32 "\n", script->ppstrNatives[a], script->ip);
					goto *dispatch[halt];
				}
				_TaghaScript_push_int64(script, (uintptr_t)pfNative);
				if( debugmode )
					printf("pushnataddr: pushed native func addr: %" PRIu64 "\n", (uintptr_t)pfNative);
				DISPATCH();
			
			exec_callnat:; {	// call a native
				if( safemode and !script->ppstrNatives ) {
					printf("exec_callnat reported: native table is NULL! Current instruction address: %" PRIu32 "\n", script->ip);
					goto *dispatch[halt];
				}
				a = _TaghaScript_get_imm4(script);
				if( safemode and a >= script->uiNatives  ) {
					printf("exec_callnat reported: native index \'%" PRIu32 "\' is out of bounds! Current instruction address: %" PRIu32 "\n", a, script->ip);
					goto *dispatch[halt];
				}
				
				pfNative = (fnNative_t) dict_find(vm->pmapNatives, script->ppstrNatives[a]);
				if( safemode and !pfNative ) {
					printf("exec_callnat reported: native \'%s\' not registered! Current instruction address: %" PRIu32 "\n", script->ppstrNatives[a], script->ip);
					goto *dispatch[halt];
				}
				// how many bytes to push to native.
				const Word_t bytes = _TaghaScript_get_imm4(script);
				// how many arguments pushed as native args
				const Word_t argcount = _TaghaScript_get_imm4(script);
				if( debugmode ) {
					printf("callnat: Calling func addr: %"PRIu64" ", (uintptr_t)pfNative);
					printf("with %"PRIu32" amount of bytes pushed ", bytes);
					printf("and %"PRIu32" args / parameters.\n", argcount);
				}
				//uchar params[bytes];
				//_TaghaScript_pop_nbytes(script, params, bytes);
				(*pfNative)(script, argcount, bytes/*, params*/);
				DISPATCH();
			}
			/* support calling natives via function pointers */
			exec_callnats:; {	// call native by func ptr allocated on stack
				pfNative = (fnNative_t)(uintptr_t) _TaghaScript_pop_int64(script);
				if( safemode and !pfNative ) {
					printf("exec_callnats reported: native \'%s\' not registered! Current instruction address: %" PRIu32 "\n", script->ppstrNatives[a], script->ip);
					goto *dispatch[halt];
				}
				const Word_t bytes = _TaghaScript_get_imm4(script);
				const Word_t argcount = _TaghaScript_get_imm4(script);
				if( debugmode ) {
					printf("callnat: Calling func addr: %"PRIu64" ", (uintptr_t)pfNative);
					printf("with %"PRIu32" amount of bytes pushed ", bytes);
					printf("and %"PRIu32" args / parameters.\n", argcount);
				}
				//uchar params[bytes];
				//_TaghaScript_pop_nbytes(script, params, bytes);
				(*pfNative)(script, argcount, bytes/*, params*/);
				DISPATCH();
			}
			exec_reset:;
				TaghaScript_reset(script);
				if( debugmode )
					printf("reset: Zero'd out Memory and stack pointers\n");
				DISPATCH();
		} /* while( 1 ) */
	} /* for( ... ) */
	script = NULL;
}
