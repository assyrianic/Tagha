
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
	uchar i = bytesize-1;
	while( i<bytesize )
		((uchar *)pBuffer)[i--] = script->pInstrStream[--script->ip];
}

static inline u64 _TaghaScript_get_imm8(Script_t *restrict script)
{
	if( !script )
		return 0;
	union conv_union conv;
	conv.c[7] = script->pInstrStream[++script->ip];
	conv.c[6] = script->pInstrStream[++script->ip];
	conv.c[5] = script->pInstrStream[++script->ip];
	conv.c[4] = script->pInstrStream[++script->ip];
	conv.c[3] = script->pInstrStream[++script->ip];
	conv.c[2] = script->pInstrStream[++script->ip];
	conv.c[1] = script->pInstrStream[++script->ip];
	conv.c[0] = script->pInstrStream[++script->ip];
	return conv.ull;
}

static inline uint _TaghaScript_get_imm4(Script_t *restrict script)
{
	if( !script )
		return 0;
	union conv_union conv;
	//	0x0A,0x0B,0x0C,0x0D,
	conv.c[3] = script->pInstrStream[++script->ip];
	conv.c[2] = script->pInstrStream[++script->ip];
	conv.c[1] = script->pInstrStream[++script->ip];
	conv.c[0] = script->pInstrStream[++script->ip];
	return conv.ui;
}

static inline ushort _TaghaScript_get_imm2(Script_t *restrict script)
{
	if( !script )
		return 0;
	union conv_union conv;
	conv.c[1] = script->pInstrStream[++script->ip];
	conv.c[0] = script->pInstrStream[++script->ip];
	return conv.us;
}

static inline void _TaghaScript_push_int64(Script_t *restrict script, const u64 val)
{
	if( !script )
		return;
	if( script->bSafeMode and (script->sp+8) >= script->uiMemsize ) {
		printf("TaghaScript_push_int64 reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+8);
		exit(1);
	}
	union conv_union conv;
	conv.ull = val;
	script->pbMemory[++script->sp] = conv.c[0];
	script->pbMemory[++script->sp] = conv.c[1];
	script->pbMemory[++script->sp] = conv.c[2];
	script->pbMemory[++script->sp] = conv.c[3];
	script->pbMemory[++script->sp] = conv.c[4];
	script->pbMemory[++script->sp] = conv.c[5];
	script->pbMemory[++script->sp] = conv.c[6];
	script->pbMemory[++script->sp] = conv.c[7];
}
static inline u64 _TaghaScript_pop_int64(Script_t *script)
{
	if( !script )
		return 0L;
	if( script->bSafeMode and (script->sp-8) >= script->uiMemsize ) {
		printf("TaghaScript_pop_int64 reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp-8);
		exit(1);
	}
	union conv_union conv;
	conv.c[7] = script->pbMemory[script->sp--];
	conv.c[6] = script->pbMemory[script->sp--];
	conv.c[5] = script->pbMemory[script->sp--];
	conv.c[4] = script->pbMemory[script->sp--];
	conv.c[3] = script->pbMemory[script->sp--];
	conv.c[2] = script->pbMemory[script->sp--];
	conv.c[1] = script->pbMemory[script->sp--];
	conv.c[0] = script->pbMemory[script->sp--];
	return conv.ull;
}

static inline void _TaghaScript_push_float64(Script_t *restrict script, const double val)
{
	if( !script )
		return;
	if( script->bSafeMode and (script->sp+8) >= script->uiMemsize ) {
		printf("TaghaScript_push_float64 reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+8);
		exit(1);
	}
	union conv_union conv;
	conv.dbl = val;
	script->pbMemory[++script->sp] = conv.c[0];
	script->pbMemory[++script->sp] = conv.c[1];
	script->pbMemory[++script->sp] = conv.c[2];
	script->pbMemory[++script->sp] = conv.c[3];
	script->pbMemory[++script->sp] = conv.c[4];
	script->pbMemory[++script->sp] = conv.c[5];
	script->pbMemory[++script->sp] = conv.c[6];
	script->pbMemory[++script->sp] = conv.c[7];
}
static inline double _TaghaScript_pop_float64(Script_t *script)
{
	if( !script )
		return 0;
	if( script->bSafeMode and (script->sp-8) >= script->uiMemsize ) {
		printf("TaghaScript_pop_float64 reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp-8);
		exit(1);
	}
	union conv_union conv;
	conv.c[7] = script->pbMemory[script->sp--];
	conv.c[6] = script->pbMemory[script->sp--];
	conv.c[5] = script->pbMemory[script->sp--];
	conv.c[4] = script->pbMemory[script->sp--];
	conv.c[3] = script->pbMemory[script->sp--];
	conv.c[2] = script->pbMemory[script->sp--];
	conv.c[1] = script->pbMemory[script->sp--];
	conv.c[0] = script->pbMemory[script->sp--];
	return conv.dbl;
}

static inline void _TaghaScript_push_int32(Script_t *restrict script, const uint val)
{
	if( !script )
		return;
	if( script->bSafeMode and (script->sp+4) >= script->uiMemsize ) {
		printf("TaghaScript_push_int32 reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+4);
		exit(1);
	}
	union conv_union conv;
	conv.ui = val;
	script->pbMemory[++script->sp] = conv.c[0];
	script->pbMemory[++script->sp] = conv.c[1];
	script->pbMemory[++script->sp] = conv.c[2];
	script->pbMemory[++script->sp] = conv.c[3];
}
static inline uint _TaghaScript_pop_int32(Script_t *script)
{
	if( !script )
		return 0;
	if( script->bSafeMode and (script->sp-4) >= script->uiMemsize ) {	// we're subtracting, did we integer underflow?
		printf("TaghaScript_pop_int32 reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp-4);
		exit(1);
	}
	union conv_union conv;
	conv.c[3] = script->pbMemory[script->sp--];
	conv.c[2] = script->pbMemory[script->sp--];
	conv.c[1] = script->pbMemory[script->sp--];
	conv.c[0] = script->pbMemory[script->sp--];
	return conv.ui;
}

static inline void _TaghaScript_push_float32(Script_t *restrict script, const float val)
{
	if( !script )
		return;
	if( script->bSafeMode and (script->sp+4) >= script->uiMemsize ) {
		printf("TaghaScript_push_float32 reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+4);
		exit(1);
	}
	union conv_union conv;
	conv.f = val;
	script->pbMemory[++script->sp] = conv.c[0];
	script->pbMemory[++script->sp] = conv.c[1];
	script->pbMemory[++script->sp] = conv.c[2];
	script->pbMemory[++script->sp] = conv.c[3];
}
static inline float _TaghaScript_pop_float32(Script_t *script)
{
	if( !script )
		return 0;
	if( script->bSafeMode and (script->sp-4) >= script->uiMemsize ) {
		printf("TaghaScript_pop_float32 reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp-4);
		exit(1);
	}
	union conv_union conv;
	conv.c[3] = script->pbMemory[script->sp--];
	conv.c[2] = script->pbMemory[script->sp--];
	conv.c[1] = script->pbMemory[script->sp--];
	conv.c[0] = script->pbMemory[script->sp--];
	return conv.f;
}

static inline void _TaghaScript_push_short(Script_t *restrict script, const ushort val)
{
	if( !script )
		return;
	if( script->bSafeMode and (script->sp+2) >= script->uiMemsize ) {
		printf("TaghaScript_push_short reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+2);
		exit(1);
	}
	union conv_union conv;
	conv.us = val;
	script->pbMemory[++script->sp] = conv.c[0];
	script->pbMemory[++script->sp] = conv.c[1];
}
static inline ushort _TaghaScript_pop_short(Script_t *script)
{
	if( !script )
		return 0;
	if( script->bSafeMode and (script->sp-2) >= script->uiMemsize ) {
		printf("TaghaScript_pop_short reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp-2);
		exit(1);
	}
	union conv_union conv;
	conv.c[1] = script->pbMemory[script->sp--];
	conv.c[0] = script->pbMemory[script->sp--];
	return conv.us;
}

static inline void _TaghaScript_push_byte(Script_t *restrict script, const uchar val)
{
	if( !script )
		return;
	if( script->bSafeMode and (script->sp+1) >= script->uiMemsize ) {
		printf("TaghaScript_push_byte reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+1);
		exit(1);
	}
	script->pbMemory[++script->sp] = val;
}
static inline uchar _TaghaScript_pop_byte(Script_t *script)
{
	if( !script )
		return 0;
	if( script->bSafeMode and (script->sp-1) >= script->uiMemsize ) {
		printf("TaghaScript_pop_byte reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp-1);
		exit(1);
	}
	return script->pbMemory[script->sp--];
}

static inline void _TaghaScript_push_nbytes(Script_t *restrict script, void *restrict pItem, const Word_t bytesize)
{
	if( !script )
		return;
	if( script->bSafeMode and (script->sp+bytesize) >= script->uiMemsize ) {
		printf("TaghaScript_push_nbytes reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+bytesize);
		exit(1);
	}
	Word_t i=0;
	for( i=0 ; i<bytesize ; i++ )
		script->pbMemory[++script->sp] = ((uchar *)pItem)[i];
}
static inline void _TaghaScript_pop_nbytes(Script_t *restrict script, void *restrict pBuffer, const Word_t bytesize)
{
	if( !script )
		return;
	if( script->bSafeMode and (script->sp-bytesize) >= script->uiMemsize ) {
		printf("TaghaScript_pop_nbytes reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp-bytesize);
		exit(1);
	}
	Word_t i=0;
	// should stop when the integer underflows
	for( i=bytesize-1 ; i<bytesize ; i-- )
		((uchar *)pBuffer)[i] = script->pbMemory[script->sp--];
}


static inline u64 _TaghaScript_peek_int64(Script_t *script)
{
	if( !script )
		return 0;
	if( script->bSafeMode and (script->sp-7) >= script->uiMemsize ) {
		printf("Tagha_peek_int32 reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp-7);
		exit(1);
	}
	union conv_union conv;
	const Word_t stkptr = script->sp;
	conv.c[7] = script->pbMemory[stkptr-0];
	conv.c[6] = script->pbMemory[stkptr-1];
	conv.c[5] = script->pbMemory[stkptr-2];
	conv.c[4] = script->pbMemory[stkptr-3];
	conv.c[3] = script->pbMemory[stkptr-4];
	conv.c[2] = script->pbMemory[stkptr-5];
	conv.c[1] = script->pbMemory[stkptr-6];
	conv.c[0] = script->pbMemory[stkptr-7];
	return conv.ull;
}
static inline double _TaghaScript_peek_float64(Script_t *script)
{
	if( !script )
		return 0;
	if( script->bSafeMode and (script->sp-7) >= script->uiMemsize ) {
		printf("Tagha_peek_int32 reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp-7);
		exit(1);
	}
	union conv_union conv;
	const Word_t stkptr = script->sp;
	conv.c[7] = script->pbMemory[stkptr-0];
	conv.c[6] = script->pbMemory[stkptr-1];
	conv.c[5] = script->pbMemory[stkptr-2];
	conv.c[4] = script->pbMemory[stkptr-3];
	conv.c[3] = script->pbMemory[stkptr-4];
	conv.c[2] = script->pbMemory[stkptr-5];
	conv.c[1] = script->pbMemory[stkptr-6];
	conv.c[0] = script->pbMemory[stkptr-7];
	return conv.dbl;
}
static inline uint _TaghaScript_peek_int32(Script_t *script)
{
	if( !script )
		return 0;
	if( script->bSafeMode and (script->sp-3) >= script->uiMemsize ) {
		printf("Tagha_peek_int32 reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp-3);
		exit(1);
	}
	union conv_union conv;
	const Word_t stkptr = script->sp;
	conv.c[3] = script->pbMemory[stkptr-0];
	conv.c[2] = script->pbMemory[stkptr-1];
	conv.c[1] = script->pbMemory[stkptr-2];
	conv.c[0] = script->pbMemory[stkptr-3];
	return conv.ui;
}

static inline float _TaghaScript_peek_float32(Script_t *script)
{
	if( !script )
		return 0;
	if( script->bSafeMode and (script->sp-3) >= script->uiMemsize ) {	// we're subtracting, did we integer underflow?
		printf("Tagha_peek_float32 reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp-3);
		exit(1);
	}
	union conv_union conv;
	const Word_t stkptr = script->sp;
	conv.c[3] = script->pbMemory[stkptr-0];
	conv.c[2] = script->pbMemory[stkptr-1];
	conv.c[1] = script->pbMemory[stkptr-2];
	conv.c[0] = script->pbMemory[stkptr-3];
	return conv.f;
}

static inline ushort _TaghaScript_peek_short(Script_t *script)
{
	if( !script )
		return 0;
	if( script->bSafeMode and (script->sp-1) >= script->uiMemsize ) {
		printf("Tagha_peek_short reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp-1);
		exit(1);
	}
	union conv_union conv;
	const Word_t stkptr = script->sp;
	conv.c[1] = script->pbMemory[stkptr];
	conv.c[0] = script->pbMemory[stkptr-1];
	return conv.us;
}

static inline uchar _TaghaScript_peek_byte(Script_t *script)
{
	if( !script )
		return 0;
	if( script->bSafeMode and (script->sp) >= script->uiMemsize ) {
		printf("Tagha_peek_byte reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp);
		exit(1);
	}
	return script->pbMemory[script->sp];
}






//#include <unistd.h>	// sleep() func
void Tagha_exec(TaghaVM_t *vm)
{
	//printf("instruction set size == %" PRIu32 "\n", nop+1);
	if( !vm )
		return;
	else if( !vm->pScript )
		return;
	
	Script_t *script = vm->pScript;
	if( !script->pInstrStream )
		return;
	
	union conv_union	conv, convb;
	uint	b,a;
	u64		qb,qa;
	double	db,da;
	float	fb,fa;
	ushort	sb,sa;
	uchar	cb,ca;
	long double	ldb, lda;
	fnNative_t pfNative;
	
#define X(x) #x ,
	const char *opcode2str[] = { INSTR_SET };
#undef X

#define X(x) &&exec_##x ,
	static const void *dispatch[] = { INSTR_SET };
#undef X
#undef INSTR_SET
	
	
#ifdef _UNISTD_H
	//#define DISPATCH()	sleep(1); INC(); goto *dispatch[ script->pInstrStream[++script->ip] ]
	//#define JUMP()		sleep(1); INC(); goto *dispatch[ script->pInstrStream[script->ip] ]
	#define DISPATCH()	sleep(1); ++script->ip; continue
#else
	//#define DISPATCH()	INC(); goto *dispatch[ script->pInstrStream[++script->ip] ]
	//#define JUMP()		INC(); goto *dispatch[ script->pInstrStream[script->ip] ]
	#define DISPATCH()	++script->ip; continue
#endif
	
	while( 1 ) {
		script->uiMaxInstrs--;
		if( !script->uiMaxInstrs )
			break;
		
		if( script->bSafeMode ) {
			if( script->ip >= script->uiInstrSize ) {
				printf("instruction address out of bounds! instruction == \'%" PRIu32 "\'\n", script->ip);
				goto *dispatch[halt];
			}
			else if( script->pInstrStream[script->ip] > nop) {
				printf("illegal instruction exception! instruction == \'%" PRIu32 "\' @ %" PRIu32 "\n", script->pInstrStream[script->ip], script->ip);
				goto *dispatch[halt];
			}
		}
		//printf( "current instruction == \"%s\" @ ip == %" PRIu32 "\n", opcode2str[script->pInstrStream[script->ip]], script->ip );
		goto *dispatch[ script->pInstrStream[script->ip] ];
		
		exec_nop:;
			DISPATCH();
		
		exec_halt:;
			script = NULL;
			printf("========================= [script done] =========================\n\n");
			return;
		
		exec_pushq:;
			conv.ull = _TaghaScript_get_imm8(script);
			printf("pushq: pushed %" PRIu64 "\n", conv.ull);
			_TaghaScript_push_int64(script, conv.ull);
			DISPATCH();
		
		// opcodes for longs
		exec_pushl:;	// push 4 bytes onto the stack
			conv.ui = _TaghaScript_get_imm4(script);
			printf("pushl: pushed %" PRIu32 "\n", conv.ui);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
		
		exec_pushs:;	// push 2 bytes onto the stack
			conv.us = _TaghaScript_get_imm2(script);
			_TaghaScript_push_short(script, conv.us);
			//script->pbMemory[++script->sp] = conv.c[0];
			//script->pbMemory[++script->sp] = conv.c[1];
			printf("pushs: pushed %" PRIu32 "\n", conv.us);
			DISPATCH();
		
		exec_pushb:;	// push a byte onto the stack
			//script->pbMemory[++script->sp] = script->pInstrStream[++script->ip];
			_TaghaScript_push_byte(script, script->pInstrStream[++script->ip]);
			printf("pushb: pushed %" PRIu32 "\n", script->pbMemory[script->sp]);
			DISPATCH();
		
		exec_pushsp:;	// push sp onto the stack, uses 4 bytes since 'sp' is uint32
			conv.ui = script->sp;
			_TaghaScript_push_int32(script, conv.ui);
			printf("pushsp: pushed sp index: %" PRIu32 "\n", conv.ui);
			DISPATCH();
		
		exec_puship:;
			conv.ui = script->ip;
			_TaghaScript_push_int32(script, conv.ui);
			printf("puship: pushed ip index: %" PRIu32 "\n", conv.ui);
			DISPATCH();
		
		exec_pushbp:;
			_TaghaScript_push_int32(script, script->bp);
			printf("pushbp: pushed bp index: %" PRIu32 "\n", script->bp);
			DISPATCH();
		
		exec_pushspadd:;
			a = script->sp;
			b = _TaghaScript_pop_int32(script);
			_TaghaScript_push_int32(script, a+b);
			printf("pushspadd: added sp with %" PRIu32 ", result: %" PRIu32 "\n", b, a+b);
			DISPATCH();
			
		exec_pushspsub:;
			a = script->sp;
			b = _TaghaScript_pop_int32(script);
			_TaghaScript_push_int32(script, a-b);
			printf("pushspsub: subbed sp with %" PRIu32 ", result: %" PRIu32 "\n", b, a-b);
			DISPATCH();
		
		exec_pushbpadd:;
			a = script->bp;
			b = _TaghaScript_pop_int32(script);
			_TaghaScript_push_int32(script, a+b);
			printf("pushbpadd: added bp with %" PRIu32 ", result: %" PRIu32 "\n", b, a-b);
			DISPATCH();
		
		exec_pushbpsub:;
			a = script->bp;
			b = _TaghaScript_pop_int32(script);
			_TaghaScript_push_int32(script, a-b);
			printf("pushbpsub: subbed bp with %" PRIu32 ", result: %" PRIu32 "\n", b, a-b);
			DISPATCH();
		
		exec_popq:;
			if( script->bSafeMode and (script->sp-8) >= script->uiMemsize ) {
				printf("exec_popq reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp-8);
				goto *dispatch[halt];
			}
			script->sp -= 8;
			printf("popq\n");
			DISPATCH();
			
		exec_popl:;		// pop 4 bytes to eventually be overwritten
			if( script->bSafeMode and (script->sp-4) >= script->uiMemsize ) {
				printf("exec_popl reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp-4);
				goto *dispatch[halt];
			}
			script->sp -= 4;
			printf("popl\n");
			DISPATCH();
		
		exec_pops:;		// pop 2 bytes
			if( script->bSafeMode and (script->sp-2) >= script->uiMemsize ) {
				printf("exec_pops reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp-2);
				goto *dispatch[halt];
			}
			script->sp -= 2;
			printf("pops\n");
			DISPATCH();
		
		exec_popb:;		// pop a byte
			if( script->bSafeMode and (script->sp-1) >= script->uiMemsize ) {
				printf("exec_popb reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp-1);
				goto *dispatch[halt];
			}
			script->sp--;
			printf("popb\n");
			DISPATCH();
			
		exec_popsp:;
			script->sp = _TaghaScript_pop_int32(script);
			printf("popsp: sp is now %" PRIu32 " bytes.\n", script->sp);
			DISPATCH();
			
		exec_popbp:;
			script->bp = _TaghaScript_pop_int32(script);
			printf("popbp: bp is now %" PRIu32 " bytes.\n", script->bp);
			DISPATCH();
			
		exec_popip:;
			script->ip = _TaghaScript_pop_int32(script);
			printf("popip: ip is now at address: %" PRIu32 ".\n", script->ip);
			continue;
		
		exec_loadspq:;
			a = _TaghaScript_pop_int32(script);
			if( script->bSafeMode and (a-7) >= script->uiMemsize ) {
				printf("exec_loadspq reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, a-7);
				goto *dispatch[halt];
			}
			conv.c[7] = script->pbMemory[a-0];
			conv.c[6] = script->pbMemory[a-1];
			conv.c[5] = script->pbMemory[a-2];
			conv.c[4] = script->pbMemory[a-3];
			conv.c[3] = script->pbMemory[a-4];
			conv.c[2] = script->pbMemory[a-5];
			conv.c[1] = script->pbMemory[a-6];
			conv.c[0] = script->pbMemory[a-7];
			_TaghaScript_push_int64(script, conv.ull);
			printf("loaded 8-byte SP address data to T.O.S. - %" PRIu64 " from sp address 0x%x\n", conv.ull, a);
			DISPATCH();
		
		exec_loadspl:;
			a = _TaghaScript_pop_int32(script);
			if( script->bSafeMode and (a-3) >= script->uiMemsize ) {
				printf("exec_loadspl reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, a-3);
				goto *dispatch[halt];
			}
			conv.c[3] = script->pbMemory[a-0];
			conv.c[2] = script->pbMemory[a-1];
			conv.c[1] = script->pbMemory[a-2];
			conv.c[0] = script->pbMemory[a-3];
			_TaghaScript_push_int32(script, conv.ui);
			printf("loaded 4-byte SP address data to T.O.S. - %" PRIu32 " from sp address 0x%x\n", conv.ui, a);
			DISPATCH();
		
		exec_loadsps:;
			a = _TaghaScript_pop_int32(script);
			if( script->bSafeMode and (a-1) >= script->uiMemsize ) {
				printf("exec_loadsps reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, a-1);
				goto *dispatch[halt];
			}
			conv.c[1] = script->pbMemory[a-0];
			conv.c[0] = script->pbMemory[a-1];
			_TaghaScript_push_short(script, conv.us);
			printf("loaded 2-byte SP address data to T.O.S. - %" PRIu32 " from sp address 0x%x\n", conv.us, a);
			DISPATCH();
		
		exec_loadspb:;
			a = _TaghaScript_pop_int32(script);
			if( script->bSafeMode and a >= script->uiMemsize ) {
				printf("exec_loadspb reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, a);
				goto *dispatch[halt];
			}
			conv.c[0] = script->pbMemory[a];
			_TaghaScript_push_byte(script, conv.c[0]);
			printf("loaded byte SP address data to T.O.S. - %" PRIu32 " from sp address 0x%x\n", conv.c[0], a);
			DISPATCH();
		
		exec_storespq:;
			a = _TaghaScript_pop_int32(script);
			if( script->bSafeMode and a-7 >= script->uiMemsize ) {
				printf("exec_storespq reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, a-7);
				goto *dispatch[halt];
			}
			conv.ull = _TaghaScript_pop_int64(script);
			script->pbMemory[a-0] = conv.c[7];
			script->pbMemory[a-1] = conv.c[6];
			script->pbMemory[a-2] = conv.c[5];
			script->pbMemory[a-3] = conv.c[4];
			script->pbMemory[a-4] = conv.c[3];
			script->pbMemory[a-5] = conv.c[2];
			script->pbMemory[a-6] = conv.c[1];
			script->pbMemory[a-7] = conv.c[0];
			printf("stored 8-byte data from T.O.S. - %" PRIu64 " to sp address 0x%x\n", conv.ull, a);
			DISPATCH();
		
		exec_storespl:;		// store TOS into another part of the data stack.
			a = _TaghaScript_pop_int32(script);
			if( script->bSafeMode and a-3 >= script->uiMemsize ) {
				printf("exec_storespl reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, a-3);
				goto *dispatch[halt];
			}
			conv.ui = _TaghaScript_pop_int32(script);
			script->pbMemory[a-0] = conv.c[3];
			script->pbMemory[a-1] = conv.c[2];
			script->pbMemory[a-2] = conv.c[1];
			script->pbMemory[a-3] = conv.c[0];
			printf("stored 4-byte data from T.O.S. - %" PRIu32 " to sp address 0x%x\n", conv.ui, a);
			DISPATCH();
		
		exec_storesps:;
			a = _TaghaScript_pop_int32(script);
			if( script->bSafeMode and a-1 >= script->uiMemsize ) {
				printf("exec_storesps reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, a-1);
				goto *dispatch[halt];
			}
			conv.us = _TaghaScript_pop_short(script);
			script->pbMemory[a-0] = conv.c[1];
			script->pbMemory[a-1] = conv.c[0];
			printf("stored 2-byte data from T.O.S. - %" PRIu32 " to sp address 0x%x\n", conv.us, a);
			DISPATCH();
		
		exec_storespb:;
			a = _TaghaScript_pop_int32(script);
			if( script->bSafeMode and a >= script->uiMemsize ) {
				printf("exec_storespb reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, a);
				goto *dispatch[halt];
			}
			script->pbMemory[a] = _TaghaScript_pop_byte(script);
			printf("stored byte data from T.O.S. - %" PRIu32 " to sp address 0x%x\n", script->pbMemory[a], a);
			DISPATCH();
		
		exec_copyq:;
			if( script->bSafeMode and script->sp-7 >= script->uiMemsize ) {
				printf("exec_copyq reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp-7);
				goto *dispatch[halt];
			}
			conv.c[0] = script->pbMemory[script->sp-7];
			conv.c[1] = script->pbMemory[script->sp-6];
			conv.c[2] = script->pbMemory[script->sp-5];
			conv.c[3] = script->pbMemory[script->sp-4];
			conv.c[4] = script->pbMemory[script->sp-3];
			conv.c[5] = script->pbMemory[script->sp-2];
			conv.c[6] = script->pbMemory[script->sp-1];
			conv.c[7] = script->pbMemory[script->sp-0];
			printf("copied 8-byte data from T.O.S. - %" PRIu64 "\n", conv.ull);
			_TaghaScript_push_int64(script, conv.ull);
			DISPATCH();
		
		exec_copyl:;	// copy 4 bytes of top of stack and put as new top of stack.
			if( script->bSafeMode and script->sp-3 >= script->uiMemsize ) {
				printf("exec_copyl reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp-3);
				goto *dispatch[halt];
			}
			conv.c[0] = script->pbMemory[script->sp-3];
			conv.c[1] = script->pbMemory[script->sp-2];
			conv.c[2] = script->pbMemory[script->sp-1];
			conv.c[3] = script->pbMemory[script->sp];
			printf("copied 4-byte data from T.O.S. - %" PRIu32 "\n", conv.ui);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
		
		exec_copys:;
			if( script->bSafeMode and script->sp-1 >= script->uiMemsize ) {
				printf("exec_copys reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp-1);
				goto *dispatch[halt];
			}
			conv.c[0] = script->pbMemory[script->sp-1];
			conv.c[1] = script->pbMemory[script->sp];
			_TaghaScript_push_short(script, conv.us);
			printf("copied 2-byte data from T.O.S. - %" PRIu32 "\n", conv.us);
			DISPATCH();
		
		exec_copyb:;
			_TaghaScript_push_byte(script, script->pbMemory[script->sp]);
			DISPATCH();
		
		exec_addq:;
			qb = _TaghaScript_pop_int64(script);
			qa = _TaghaScript_pop_int64(script);
			conv.ll = (i64)qa + (i64)qb;
			printf("signed 8 byte addition result: %" PRIi64 " == %" PRIi64 " + %" PRIi64 "\n", conv.ll, qa,qb);
			_TaghaScript_push_int64(script, conv.ull);
			DISPATCH();
			
		exec_uaddq:;
			qb = _TaghaScript_pop_int64(script);
			qa = _TaghaScript_pop_int64(script);
			conv.ull = qa+qb;
			printf("unsigned 8 byte addition result: %" PRIu64 " == %" PRIu64 " + %" PRIu64 "\n", conv.ull, qa,qb);
			_TaghaScript_push_int64(script, conv.ull);
			DISPATCH();
		
		exec_addl:;		// pop 8 bytes, signed addition, and push 4 byte result to top of stack
			b = _TaghaScript_pop_int32(script);
			a = _TaghaScript_pop_int32(script);
			conv.i = (int)a + (int)b;
			printf("signed 4 byte addition result: %" PRIi32 " == %" PRIi32 " + %" PRIi32 "\n", conv.i, a,b);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
		
		exec_uaddl:;	// In C, all integers in an expression are promoted to int32, if number is bigger then uint32 or int64
			b = _TaghaScript_pop_int32(script);
			a = _TaghaScript_pop_int32(script);
			conv.ui = a+b;
			printf("unsigned 4 byte addition result: %" PRIu32 " == %" PRIu32 " + %" PRIu32 "\n", conv.ui, a,b);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
		
		exec_addf:;
			fb = _TaghaScript_pop_float32(script);
			fa = _TaghaScript_pop_float32(script);
			conv.f = fa+fb;
			printf("4-byte float addition result: %f == %f + %f\n", conv.f, fa,fb);
			_TaghaScript_push_float32(script, conv.f);
			DISPATCH();
		
		exec_addf64:;
			db = _TaghaScript_pop_float64(script);
			da = _TaghaScript_pop_float64(script);
			conv.dbl = da+db;
			printf("8-byte float addition result: %f == %f + %f\n", conv.dbl, da,db);
			_TaghaScript_push_float64(script, conv.dbl);
			DISPATCH();
		
		exec_subq:;
			qb = _TaghaScript_pop_int64(script);
			qa = _TaghaScript_pop_int64(script);
			conv.ll = (i64)qa - (i64)qb;
			printf("signed 8 byte subtraction result: %" PRIi64 " == %" PRIi64 " - %" PRIi64 "\n", conv.ll, qa,qb);
			_TaghaScript_push_int64(script, conv.ull);
			DISPATCH();
			
		exec_usubq:;
			qb = _TaghaScript_pop_int64(script);
			qa = _TaghaScript_pop_int64(script);
			conv.ull = qa-qb;
			printf("unsigned 8 byte subtraction result: %" PRIu64 " == %" PRIu64 " - %" PRIu64 "\n", conv.ull, qa,qb);
			_TaghaScript_push_int64(script, conv.ull);
			DISPATCH();
		
		exec_subl:;
			b = _TaghaScript_pop_int32(script);
			a = _TaghaScript_pop_int32(script);
			conv.i = (int)a - (int)b;
			printf("signed 4 byte subtraction result: %" PRIi32 " == %" PRIi32 " - %" PRIi32 "\n", conv.ui, a,b);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
		
		exec_usubl:;
			b = _TaghaScript_pop_int32(script);
			a = _TaghaScript_pop_int32(script);
			conv.ui = a-b;
			printf("unsigned 4 byte subtraction result: %" PRIu32 " == %" PRIu32 " - %" PRIu32 "\n", conv.ui, a,b);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
		
		exec_subf:;
			fb = _TaghaScript_pop_float32(script);
			fa = _TaghaScript_pop_float32(script);
			conv.f = fa-fb;
			printf("4-byte float subtraction result: %f == %f - %f\n", conv.f, fa,fb);
			_TaghaScript_push_float32(script, conv.f);
			DISPATCH();
			
		exec_subf64:;
			db = _TaghaScript_pop_float64(script);
			da = _TaghaScript_pop_float64(script);
			conv.dbl = da-db;
			printf("8-byte float subtraction result: %f == %f - %f\n", conv.dbl, da,db);
			_TaghaScript_push_float64(script, conv.dbl);
			DISPATCH();
		
		exec_mulq:;
			qb = _TaghaScript_pop_int64(script);
			qa = _TaghaScript_pop_int64(script);
			conv.ll = (i64)qa * (i64)qb;
			printf("signed 8 byte mult result: %" PRIi64 " == %" PRIi64 " * %" PRIi64 "\n", conv.ll, qa,qb);
			_TaghaScript_push_int64(script, conv.ull);
			DISPATCH();
			
		exec_umulq:;
			qb = _TaghaScript_pop_int64(script);
			qa = _TaghaScript_pop_int64(script);
			conv.ull = qa*qb;
			printf("unsigned 8 byte mult result: %" PRIu64 " == %" PRIu64 " * %" PRIu64 "\n", conv.ull, qa,qb);
			_TaghaScript_push_int64(script, conv.ull);
			DISPATCH();
		
		exec_mull:;
			b = _TaghaScript_pop_int32(script);
			a = _TaghaScript_pop_int32(script);
			conv.i = (int)a * (int)b;
			printf("signed 4 byte mult result: %" PRIi32 " == %" PRIi32 " * %" PRIi32 "\n", conv.i, a,b);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
		
		exec_umull:;
			b = _TaghaScript_pop_int32(script);
			a = _TaghaScript_pop_int32(script);
			conv.ui = a*b;
			printf("unsigned 4 byte mult result: %" PRIu32 " == %" PRIu32 " * %" PRIu32 "\n", conv.ui, a,b);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
			
		exec_mulf:;
			fb = _TaghaScript_pop_float32(script);
			fa = _TaghaScript_pop_float32(script);
			conv.f = fa*fb;
			printf("4-byte float mult result: %f == %f * %f\n", conv.f, fa,fb);
			_TaghaScript_push_float32(script, conv.f);
			DISPATCH();
			
		exec_mulf64:;
			db = _TaghaScript_pop_float64(script);
			da = _TaghaScript_pop_float64(script);
			conv.dbl = da*db;
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
			printf("unsigned 4 byte modulo result: %" PRIu32 " == %" PRIu32 " %% %" PRIu32 "\n", conv.ui, a,b);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
		
		exec_andq:;
			qb = _TaghaScript_pop_int64(script);
			qa = _TaghaScript_pop_int64(script);
			conv.ull = qa & qb;
			printf("8 byte AND result: %" PRIu64 " == %" PRIu64 " & %" PRIu64 "\n", conv.ull, qa,qb);
			_TaghaScript_push_int64(script, conv.ull);
			DISPATCH();
			
		exec_orq:;
			qb = _TaghaScript_pop_int64(script);
			qa = _TaghaScript_pop_int64(script);
			conv.ull = qa | qb;
			printf("8 byte OR result: %" PRIu64 " == %" PRIu64 " | %" PRIu64 "\n", conv.ull, qa,qb);
			_TaghaScript_push_int64(script, conv.ull);
			DISPATCH();
			
		exec_xorq:;
			qb = _TaghaScript_pop_int64(script);
			qa = _TaghaScript_pop_int64(script);
			conv.ull = qa ^ qb;
			printf("8 byte XOR result: %" PRIu64 " == %" PRIu64 " ^ %" PRIu64 "\n", conv.ull, qa,qb);
			_TaghaScript_push_int64(script, conv.ull);
			DISPATCH();
			
		exec_notq:;
			qa = _TaghaScript_pop_int64(script);
			conv.ull = ~qa;
			printf("8 byte NOT result: %" PRIu64 "\n", conv.ull);
			_TaghaScript_push_int64(script, conv.ull);
			DISPATCH();
			
		exec_shlq:;
			qb = _TaghaScript_pop_int64(script);
			qa = _TaghaScript_pop_int64(script);
			conv.ull = qa << qb;
			printf("8 byte Shift Left result: %" PRIu64 " == %" PRIu64 " << %" PRIu64 "\n", conv.ull, qa,qb);
			_TaghaScript_push_int64(script, conv.ull);
			DISPATCH();
			
		exec_shrq:;
			qb = _TaghaScript_pop_int64(script);
			qa = _TaghaScript_pop_int64(script);
			conv.ull = qa >> qb;
			printf("8 byte Shift Right result: %" PRIu64 " == %" PRIu64 " >> %" PRIu64 "\n", conv.ull, qa,qb);
			_TaghaScript_push_int64(script, conv.ull);
			DISPATCH();
			
		exec_andl:;
			b = _TaghaScript_pop_int32(script);
			a = _TaghaScript_pop_int32(script);
			conv.ui = a & b;
			printf("4 byte AND result: %" PRIu32 " == %" PRIu32 " & %" PRIu32 "\n", conv.ui, a,b);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
		
		exec_orl:;
			b = _TaghaScript_pop_int32(script);
			a = _TaghaScript_pop_int32(script);
			conv.ui = a | b;
			printf("4 byte OR result: %" PRIu32 " == %" PRIu32 " | %" PRIu32 "\n", conv.ui, a,b);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
		
		exec_xorl:;
			b = _TaghaScript_pop_int32(script);
			a = _TaghaScript_pop_int32(script);
			conv.ui = a ^ b;
			printf("4 byte XOR result: %" PRIu32 " == %" PRIu32 " ^ %" PRIu32 "\n", conv.ui, a,b);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
		
		exec_notl:;
			a = _TaghaScript_pop_int32(script);
			conv.ui = ~a;
			printf("4 byte NOT result: %" PRIu32 "\n", conv.ui);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
		
		exec_shll:;
			b = _TaghaScript_pop_int32(script);
			a = _TaghaScript_pop_int32(script);
			conv.ui = a << b;
			printf("4 byte Shift Left result: %" PRIu32 " == %" PRIu32 " << %" PRIu32 "\n", conv.ui, a,b);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
		
		exec_shrl:;
			b = _TaghaScript_pop_int32(script);
			a = _TaghaScript_pop_int32(script);
			conv.ui = a >> b;
			printf("4 byte Shift Right result: %" PRIu32 " == %" PRIu32 " >> %" PRIu32 "\n", conv.ui, a,b);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
		
		exec_incq:;
			qa = _TaghaScript_pop_int64(script);
			conv.ull = ++qa;
			printf("8 byte Increment result: %" PRIu64 "\n", conv.ull);
			_TaghaScript_push_int64(script, conv.ull);
			DISPATCH();
			
		exec_incl:;
			a = _TaghaScript_pop_int32(script);
			conv.ui = ++a;
			printf("4 byte Increment result: %" PRIu32 "\n", conv.ui);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
		
		exec_incf:;
			fa = _TaghaScript_pop_float32(script);
			conv.f = ++fa;
			printf("4-byte float Increment result: %f\n", conv.f);
			_TaghaScript_push_float32(script, conv.f);
			DISPATCH();
			
		exec_incf64:;
			da = _TaghaScript_pop_float64(script);
			conv.dbl = ++da;
			printf("8-byte float Increment result: %f\n", conv.dbl);
			_TaghaScript_push_float64(script, conv.dbl);
			DISPATCH();
		
		exec_decq:;
			qa = _TaghaScript_pop_int64(script);
			conv.ull = --qa;
			printf("8 byte Decrement result: %" PRIu64 "\n", conv.ull);
			_TaghaScript_push_int64(script, conv.ull);
			DISPATCH();
			
		exec_decl:;
			a = _TaghaScript_pop_int32(script);
			conv.ui = --a;
			printf("4 byte Decrement result: %" PRIu32 "\n", conv.ui);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
		
		exec_decf:;
			fa = _TaghaScript_pop_float32(script);
			conv.f = --fa;
			printf("4-byte float Decrement result: %f\n", conv.f);
			_TaghaScript_push_float32(script, conv.f);
			DISPATCH();
		
		exec_decf64:;
			da = _TaghaScript_pop_float64(script);
			conv.dbl = --da;
			printf("8-byte float Decrement result: %f\n", conv.dbl);
			_TaghaScript_push_float64(script, conv.dbl);
			DISPATCH();
		
		exec_negq:;
			qa = _TaghaScript_pop_int64(script);
			conv.ull = -qa;
			printf("8 byte Negate result: %" PRIu64 "\n", conv.ull);
			_TaghaScript_push_int64(script, conv.ull);
			DISPATCH();
		
		exec_negl:;
			a = _TaghaScript_pop_int32(script);
			conv.ui = -a;
			printf("4 byte Negate result: %" PRIu32 "\n", conv.ui);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
			
		exec_negf:;
			fa = _TaghaScript_pop_float32(script);
			conv.f = -fa;
			printf("4-byte float Negate result: %f\n", conv.f);
			_TaghaScript_push_float32(script, conv.f);
			DISPATCH();
			
		exec_negf64:;
			da = _TaghaScript_pop_float64(script);
			conv.dbl = -da;
			printf("8-byte float Negate result: %f\n", conv.dbl);
			_TaghaScript_push_float64(script, conv.dbl);
			DISPATCH();
		
		exec_ltq:;
			qb = _TaghaScript_pop_int64(script);
			qa = _TaghaScript_pop_int64(script);
			conv.ui = (i64)qa < (i64)qb;
			printf("signed 8 byte Less Than result: %" PRIu32 " == %" PRIi64 " < %" PRIi64 "\n", conv.ui, qa,qb);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
		
		exec_ultq:;
			qb = _TaghaScript_pop_int64(script);
			qa = _TaghaScript_pop_int64(script);
			conv.ui = qa < qb;
			printf("unsigned 8 byte Less Than result: %" PRIu32 " == %" PRIu64 " < %" PRIu64 "\n", conv.ui, qa,qb);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
		
		exec_ltl:;
			b = _TaghaScript_pop_int32(script);
			a = _TaghaScript_pop_int32(script);
			conv.ui = (int)a < (int)b;
			printf("4 byte Signed Less Than result: %" PRIu32 " == %" PRIi32 " < %" PRIi32 "\n", conv.ui, a,b);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
		
		exec_ultl:;
			b = _TaghaScript_pop_int32(script);
			a = _TaghaScript_pop_int32(script);
			conv.ui = a < b;
			printf("4 byte Unsigned Less Than result: %" PRIu32 " == %" PRIu32 " < %" PRIu32 "\n", conv.ui, a,b);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
		
		exec_ltf:;
			fb = _TaghaScript_pop_float32(script);
			fa = _TaghaScript_pop_float32(script);
			conv.ui = fa < fb;
			printf("4 byte Less Than Float result: %" PRIu32 " == %f < %f\n", conv.ui, fa,fb);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
		
		exec_ltf64:;
			db = _TaghaScript_pop_float64(script);
			da = _TaghaScript_pop_float64(script);
			conv.ui = da < db;
			printf("8 byte Less Than Float result: %" PRIu32 " == %f < %f\n", conv.ui, da,db);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
		
		exec_gtq:;
			qb = _TaghaScript_pop_int64(script);
			qa = _TaghaScript_pop_int64(script);
			conv.ui = (i64)qa > (i64)qb;
			printf("signed 8 byte Greater Than result: %" PRIu32 " == %" PRIi64 " > %" PRIi64 "\n", conv.ui, qa,qb);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
			
		exec_ugtq:;
			qb = _TaghaScript_pop_int64(script);
			qa = _TaghaScript_pop_int64(script);
			conv.ui = qa > qb;
			printf("unsigned 8 byte Greater Than result: %" PRIu32 " == %" PRIu64 " > %" PRIu64 "\n", conv.ui, qa,qb);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
		
		exec_gtl:;
			b = _TaghaScript_pop_int32(script);
			a = _TaghaScript_pop_int32(script);
			conv.ui = (int)a > (int)b;
			printf("4 byte Signed Greater Than result: %" PRIu32 " == %" PRIi32 " > %" PRIi32 "\n", conv.ui, a,b);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
		
		exec_ugtl:;
			b = _TaghaScript_pop_int32(script);
			a = _TaghaScript_pop_int32(script);
			conv.ui = a > b;
			printf("4 byte Unigned Greater Than result: %" PRIu32 " == %" PRIu32 " > %" PRIu32 "\n", conv.ui, a,b);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
		
		exec_gtf:;
			fb = _TaghaScript_pop_float32(script);
			fa = _TaghaScript_pop_float32(script);
			conv.ui = fa > fb;
			printf("4 byte Greater Than Float result: %" PRIu32 " == %f > %f\n", conv.ui, fa,fb);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
		
		exec_gtf64:;
			db = _TaghaScript_pop_float64(script);
			da = _TaghaScript_pop_float64(script);
			conv.ui = da > db;
			printf("8 byte Greater Than Float result: %" PRIu32 " == %f > %f\n", conv.ui, da,db);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
		
		exec_cmpq:;
			qb = _TaghaScript_pop_int64(script);
			qa = _TaghaScript_pop_int64(script);
			conv.ui = (i64)qa == (i64)qb;
			printf("signed 8 byte Compare result: %" PRIu32 " == %" PRIi64 " == %" PRIi64 "\n", conv.ui, qa,qb);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
		
		exec_ucmpq:;
			qb = _TaghaScript_pop_int64(script);
			qa = _TaghaScript_pop_int64(script);
			conv.ui = qa == qb;
			printf("unsigned 8 byte Compare result: %" PRIu32 " == %" PRIu64 " == %" PRIu64 "\n", conv.ui, qa,qb);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
		
		exec_cmpl:;
			b = _TaghaScript_pop_int32(script);
			a = _TaghaScript_pop_int32(script);
			conv.ui = (int)a == (int)b;
			printf("4 byte Signed Compare result: %" PRIu32 " == %" PRIi32 " == %" PRIi32 "\n", conv.ui, a,b);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
		
		exec_ucmpl:;
			b = _TaghaScript_pop_int32(script);
			a = _TaghaScript_pop_int32(script);
			conv.ui = a == b;
			printf("4 byte Unsigned Compare result: %" PRIu32 " == %" PRIu32 " == %" PRIu32 "\n", conv.ui, a,b);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
		
		exec_compf:;
			fb = _TaghaScript_pop_float32(script);
			fa = _TaghaScript_pop_float32(script);
			conv.ui = fa == fb;
			printf("4 byte Compare Float result: %" PRIu32 " == %f == %f\n", conv.ui, fa,fb);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
		
		exec_cmpf64:;
			db = _TaghaScript_pop_float64(script);
			da = _TaghaScript_pop_float64(script);
			conv.ui = da == db;
			printf("8 byte Compare Float result: %" PRIu32 " == %f == %f\n", conv.ui, da,db);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
		
		exec_leqq:;
			qb = _TaghaScript_pop_int64(script);
			qa = _TaghaScript_pop_int64(script);
			conv.ui = (i64)qa <= (i64)qb;
			printf("8 byte Signed Less Equal result: %" PRIu32 " == %" PRIi64 " <= %" PRIi64 "\n", conv.ui, qa,qb);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
		
		exec_uleqq:;
			qb = _TaghaScript_pop_int64(script);
			qa = _TaghaScript_pop_int64(script);
			conv.ui = qa <= qb;
			printf("8 byte Unsigned Less Equal result: %" PRIu32 " == %" PRIu64 " <= %" PRIu64 "\n", conv.ui, qa,qb);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
		
		exec_leql:;
			b = _TaghaScript_pop_int32(script);
			a = _TaghaScript_pop_int32(script);
			conv.ui = (int)a <= (int)b;
			printf("4 byte Signed Less Equal result: %" PRIu32 " == %" PRIi32 " <= %" PRIi32 "\n", conv.ui, a,b);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
		
		exec_uleql:;
			b = _TaghaScript_pop_int32(script);
			a = _TaghaScript_pop_int32(script);
			conv.ui = a <= b;
			printf("4 byte Unsigned Less Equal result: %" PRIu32 " == %" PRIu32 " <= %" PRIu32 "\n", conv.ui, a,b);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
		
		exec_leqf:;
			fb = _TaghaScript_pop_float32(script);
			fa = _TaghaScript_pop_float32(script);
			conv.ui = fa <= fb;
			printf("4 byte Less Equal Float result: %" PRIu32 " == %f <= %f\n", conv.ui, fa, fb);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
			
		exec_leqf64:;
			db = _TaghaScript_pop_float64(script);
			da = _TaghaScript_pop_float64(script);
			conv.ui = da <= db;
			printf("8 byte Less Equal Float result: %" PRIu32 " == %f <= %f\n", conv.ui, da,db);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
		
		exec_geqq:;
			qb = _TaghaScript_pop_int64(script);
			qa = _TaghaScript_pop_int64(script);
			conv.ui = (i64)qa >= (i64)qb;
			printf("8 byte Signed Greater Equal result: %" PRIu32 " == %" PRIi64 " >= %" PRIi64 "\n", conv.ui, qa,qb);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
			
		exec_ugeqq:;
			qb = _TaghaScript_pop_int64(script);
			qa = _TaghaScript_pop_int64(script);
			conv.ui = qa >= qb;
			printf("8 byte Unsigned Greater Equal result: %" PRIu32 " == %" PRIu64 " >= %" PRIu64 "\n", conv.ui, qa,qb);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
		
		exec_geql:;
			b = _TaghaScript_pop_int32(script);
			a = _TaghaScript_pop_int32(script);
			conv.ui = (int)a >= (int)b;
			printf("4 byte Signed Greater Equal result: %" PRIu32 " == %" PRIi32 " >= %" PRIi32 "\n", conv.ui, a,b);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
		
		exec_ugeql:;
			b = _TaghaScript_pop_int32(script);
			a = _TaghaScript_pop_int32(script);
			conv.ui = a >= b;
			printf("4 byte Unsigned Greater Equal result: %" PRIu32 " == %" PRIu32 " >= %" PRIu32 "\n", conv.ui, a,b);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
		
		exec_geqf:;
			fb = _TaghaScript_pop_float32(script);
			fa = _TaghaScript_pop_float32(script);
			conv.ui = fa >= fb;
			printf("4 byte Greater Equal Float result: %" PRIu32 " == %f >= %f\n", conv.ui, fa, fb);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
		
		exec_geqf64:;
			db = _TaghaScript_pop_float64(script);
			da = _TaghaScript_pop_float64(script);
			conv.ui = da >= db;
			printf("8 byte Greater Equal Float result: %" PRIu32 " == %f >= %f\n", conv.ui, da,db);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
		
		exec_neqq:;
			qb = _TaghaScript_pop_int64(script);
			qa = _TaghaScript_pop_int64(script);
			conv.ui = (i64)qa != (i64)qb;
			printf("8 byte Signed Not Equal result: %" PRIu32 " == %" PRIi64 " != %" PRIi64 "\n", conv.ui, qa,qb);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
			
		exec_uneqq:;
			qb = _TaghaScript_pop_int64(script);
			qa = _TaghaScript_pop_int64(script);
			conv.ui = qa != qb;
			printf("8 byte Unsigned Not Equal result: %" PRIu32 " == %" PRIi64 " != %" PRIi64 "\n", conv.ui, qa,qb);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
			
		exec_neql:;
			b = _TaghaScript_pop_int32(script);
			a = _TaghaScript_pop_int32(script);
			conv.ui = (int)a != (int)b;
			printf("4 byte Signed Not Equal result: %" PRIu32 " == %" PRIi32 " != %" PRIi32 "\n", conv.ui, a,b);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
			
		exec_uneql:;
			b = _TaghaScript_pop_int32(script);
			a = _TaghaScript_pop_int32(script);
			conv.ui = a != b;
			printf("4 byte Unsigned Not Equal result: %" PRIu32 " == %" PRIu32 " != %" PRIu32 "\n", conv.ui, a,b);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
		
		exec_neqf:;
			fb = _TaghaScript_pop_float32(script);
			fa = _TaghaScript_pop_float32(script);
			conv.ui = fa != fb;
			printf("4 byte Not Equal Float result: %" PRIu32 " == %f != %f\n", conv.ui, fa, fb);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
		
		exec_neqf64:;
			db = _TaghaScript_pop_float64(script);
			da = _TaghaScript_pop_float64(script);
			conv.ui = da != db;
			printf("8 byte Not Equal Float result: %" PRIu32 " == %f != %f\n", conv.ui, da,db);
			_TaghaScript_push_int32(script, conv.ui);
			DISPATCH();
		
		exec_jmp:;		// addresses are word sized bytes.
			conv.ui = _TaghaScript_get_imm4(script);
			script->ip = conv.ui;
			printf("jmping to instruction address: %" PRIu32 "\n", script->ip);
			continue;
		
		exec_jzq:;
			if( script->bSafeMode and script->sp-7 >= script->uiMemsize ) {
				printf("exec_jzq reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp-7);
				goto *dispatch[halt];
			}
			a = script->sp;
			conv.c[7] = script->pbMemory[a-0];
			conv.c[6] = script->pbMemory[a-1];
			conv.c[5] = script->pbMemory[a-2];
			conv.c[4] = script->pbMemory[a-3];
			conv.c[3] = script->pbMemory[a-4];
			conv.c[2] = script->pbMemory[a-5];
			conv.c[1] = script->pbMemory[a-6];
			conv.c[0] = script->pbMemory[a-7];
			qa = conv.ull;
			conv.ui = _TaghaScript_get_imm4(script);
			script->ip = (!qa) ? conv.ui : script->ip+1 ;
			printf("jzq'ing to instruction address: %" PRIu32 "\n", script->ip);
			continue;
			
		exec_jnzq:;
			if( script->bSafeMode and script->sp-7 >= script->uiMemsize ) {
				printf("exec_jnzq reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp-7);
				goto *dispatch[halt];
			}
			a = script->sp;
			conv.c[7] = script->pbMemory[a-0];
			conv.c[6] = script->pbMemory[a-1];
			conv.c[5] = script->pbMemory[a-2];
			conv.c[4] = script->pbMemory[a-3];
			conv.c[3] = script->pbMemory[a-4];
			conv.c[2] = script->pbMemory[a-5];
			conv.c[1] = script->pbMemory[a-6];
			conv.c[0] = script->pbMemory[a-7];
			qa = conv.ull;
			conv.ui = _TaghaScript_get_imm4(script);
			script->ip = (qa) ? conv.ui : script->ip+1 ;
			printf("jnzq'ing to instruction address: %" PRIu32 "\n", script->ip);
			continue;
		
		exec_jzl:;		// check if the first 4 bytes on stack are zero, if yes then jump it.
			if( script->bSafeMode and script->sp-3 >= script->uiMemsize ) {
				printf("exec_jzl reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp-3);
				goto *dispatch[halt];
			}
			b = script->sp;
			conv.c[3] = script->pbMemory[b-0];
			conv.c[2] = script->pbMemory[b-1];
			conv.c[1] = script->pbMemory[b-2];
			conv.c[0] = script->pbMemory[b-3];
			a = conv.ui;
			conv.ui = _TaghaScript_get_imm4(script);
			script->ip = (!a) ? conv.ui : script->ip+1 ;
			printf("jzl'ing to instruction address: %" PRIu32 "\n", script->ip);	//opcode2str[script->pInstrStream[script->ip]]
			continue;
		
		exec_jnzl:;
			if( script->bSafeMode and script->sp-3 >= script->uiMemsize ) {
				printf("exec_jnzl reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp-3);
				goto *dispatch[halt];
			}
			b = script->sp;
			conv.c[3] = script->pbMemory[b-0];
			conv.c[2] = script->pbMemory[b-1];
			conv.c[1] = script->pbMemory[b-2];
			conv.c[0] = script->pbMemory[b-3];
			a = conv.ui;
			conv.ui = _TaghaScript_get_imm4(script);
			script->ip = (a) ? conv.ui : script->ip+1 ;
			printf("jnzl'ing to instruction address: %" PRIu32 "\n", script->ip);
			continue;
		
		exec_call:;		// support functions
			conv.ui = _TaghaScript_get_imm4(script);	// get func address
			printf("call: calling address: %" PRIu32 "\n", conv.ui);
			_TaghaScript_push_int32(script, script->ip+1);	// save return address.
			printf("call return addr: %" PRIu32 "\n", _TaghaScript_peek_int32(script));
			_TaghaScript_push_int32(script, script->bp);	// push ebp;
			script->bp = script->sp;	// mov ebp, esp;
			script->ip = conv.ui;	// jump to instruction
			printf("script->bp: %" PRIu32 "\n", script->sp);
			continue;
		
		exec_calls:;	// support local function pointers
			conv.ui = _TaghaScript_pop_int32(script);	// get func address
			printf("calls: calling address: %" PRIu32 "\n", conv.ui);
			_TaghaScript_push_int32(script, script->ip+1);	// save return address.
			printf("call return addr: %" PRIu32 "\n", _TaghaScript_peek_int32(script));
			_TaghaScript_push_int32(script, script->bp);	// push ebp
			script->bp = script->sp;	// mov ebp, esp
			script->ip = conv.ui;	// jump to instruction
			printf("script->bp: %" PRIu32 "\n", script->sp);
			continue;
		
		exec_ret:;
			script->sp = script->bp;	// mov esp, ebp
			printf("sp set to bp, sp == %" PRIu32 "\n", script->sp);
			script->bp = _TaghaScript_pop_int32(script);	// pop ebp
			script->ip = _TaghaScript_pop_int32(script);	// pop return address.
			printf("returning to address: %" PRIu32 "\n", script->ip);
			continue;
		
		exec_retx:; {	// for functions that return something.
			a = _TaghaScript_get_imm4(script);
			uchar bytebuffer[a];
			/* This opcode assumes all the data for return
			 * is on the near top of stack. In theory, you can
			 * use this method to return multiple pieces of data.
			 */
			_TaghaScript_pop_nbytes(script, bytebuffer, a);	// store our needed data to a buffer.
			// do our usual return code.
			script->sp = script->bp;	// mov esp, ebp
			printf("sp set to bp, sp == %" PRIu32 "\n", script->sp);
			script->bp = _TaghaScript_pop_int32(script);	// pop ebp
			script->ip = _TaghaScript_pop_int32(script);	// pop return address.
			_TaghaScript_push_nbytes(script, bytebuffer, a);	// push back return data!
			printf("retxurning to address: %" PRIu32 "\n", script->ip);
			continue;
		}
		
		exec_pushnataddr:;
			if( script->bSafeMode and !script->ppstrNatives ) {
				printf("exec_pushnataddr reported: native table is NULL! Current instruction address: %" PRIu32 "\n", script->ip);
				goto *dispatch[halt];
			}
			// match native name to get an index.
			a = _TaghaScript_get_imm4(script);
			if( script->bSafeMode and a >= script->uiNatives  ) {
				printf("exec_pushnataddr reported: native index \'%" PRIu32 "\' is out of bounds! Current instruction address: %" PRIu32 "\n", a, script->ip);
				goto *dispatch[halt];
			}
			pfNative = (fnNative_t) dict_find(vm->pmapNatives, script->ppstrNatives[a]);
			if( script->bSafeMode and !pfNative ) {
				printf("exec_pushnataddr reported: native \'%s\' not registered! Current instruction address: %" PRIu32 "\n", script->ppstrNatives[a], script->ip);
				goto *dispatch[halt];
			}
			// tried using 'push nbytes' but retrieving caused segfaults.
			_TaghaScript_push_int64(script, (uintptr_t)pfNative);
			DISPATCH();
		
		exec_callnat:; {	// call a native
			if( script->bSafeMode and !script->ppstrNatives ) {
				printf("exec_callnat reported: native table is NULL! Current instruction address: %" PRIu32 "\n", script->ip);
				goto *dispatch[halt];
			}
			a = _TaghaScript_get_imm4(script);
			if( script->bSafeMode and a >= script->uiNatives  ) {
				printf("exec_callnat reported: native index \'%" PRIu32 "\' is out of bounds! Current instruction address: %" PRIu32 "\n", a, script->ip);
				goto *dispatch[halt];
			}
			
			pfNative = (fnNative_t) dict_find(vm->pmapNatives, script->ppstrNatives[a]);
			if( script->bSafeMode and !pfNative ) {
				printf("exec_callnat reported: native \'%s\' not registered! Current instruction address: %" PRIu32 "\n", script->ppstrNatives[a], script->ip);
				goto *dispatch[halt];
			}
			// how many bytes to push to native.
			const Word_t bytes = _TaghaScript_get_imm4(script);
			// how many arguments pushed as native args
			const Word_t argcount = _TaghaScript_get_imm4(script);
			uchar params[bytes];
			_TaghaScript_pop_nbytes(script, params, bytes);
			(*pfNative)(script, argcount, bytes, params);
			DISPATCH();
		}
		/* support calling natives via function pointers */
		exec_callnats:; {	// call native by func ptr allocated on stack
			pfNative = (fnNative_t)(uintptr_t) _TaghaScript_pop_int64(script);
			if( script->bSafeMode and !pfNative ) {
				printf("exec_callnats reported: native \'%s\' not registered! Current instruction address: %" PRIu32 "\n", script->ppstrNatives[a], script->ip);
				goto *dispatch[halt];
			}
			const Word_t bytes = _TaghaScript_get_imm4(script);
			const Word_t argcount = _TaghaScript_get_imm4(script);
			uchar params[bytes];
			_TaghaScript_pop_nbytes(script, params, bytes);
			(*pfNative)(script, argcount, bytes, params);
			DISPATCH();
		}
		exec_reset:;
			TaghaScript_reset(script);
			DISPATCH();
	} /* while( 1 ) */
	script = NULL;
	printf("Tagha_exec :: max instructions reached\n");
}
