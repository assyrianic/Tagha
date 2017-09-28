
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include "tagha.h"

#define INSTR_SET	\
	X(halt) \
	X(pushq) X(pushl) X(pushs) X(pushb) X(pushsp) X(puship) X(pushbp) \
	X(pushspadd) X(pushspsub) X(pushbpadd) X(pushbpsub) X(pushipadd) X(pushipsub) \
	\
	X(popq) X(popl) X(pops) X(popb) X(popsp) X(popip) X(popbp) \
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
	X(jmp) X(jzq) X(jnzq) X(jzl) X(jnzl) \
	X(call) X(calls) X(ret) X(retx) X(reset) \
	X(pushnataddr) X(callnat) X(callnats) \
	X(nop) \

#define X(x) x,
enum InstrSet { INSTR_SET };
#undef X

// This is strictly for long doubles
static inline void _TaghaScript_get_immn(struct TaghaScript *restrict script, void *restrict pBuffer, const uint32_t bytesize)
{
	if( !script or !pBuffer )
		return;
	
	uint32_t i = 0;
	while( i<bytesize )
		((uint8_t *)pBuffer)[i++] = script->pInstrStream[++script->ip];
}

/*
 * Private, inlined implementation of getting values from
 * the instruction stream to help with speeding up code.
 */

static inline uint64_t _TaghaScript_get_imm8(struct TaghaScript *restrict script)
{
	if( !script )
		return 0L;
	if( script->bSafeMode and (script->ip+8) >= script->uiInstrSize ) {
		printf("_TaghaScript_get_imm8 reported: instr overflow! Current instruction address: %" PRIWord "\n", script->ip);
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

static inline uint32_t _TaghaScript_get_imm4(struct TaghaScript *restrict script)
{
	if( !script )
		return 0;
	if( script->bSafeMode and (script->ip+4) >= script->uiInstrSize ) {
		printf("_TaghaScript_get_imm4 reported: instr overflow! Current instruction address: %" PRIWord "\n", script->ip);
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

static inline uint16_t _TaghaScript_get_imm2(struct TaghaScript *restrict script)
{
	if( !script )
		return 0;
	if( script->bSafeMode and (script->ip+2) >= script->uiInstrSize ) {
		printf("_TaghaScript_get_imm2 reported: instr overflow! Current instruction address: %" PRIWord "\n", script->ip);
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

static inline void _TaghaScript_push_int64(struct TaghaScript *restrict script, const uint64_t val)
{
	if( !script )
		return;
	uint32_t size = sizeof(uint64_t);
	if( script->bSafeMode and (script->sp-size) >= script->uiMemsize ) {
		printf("_TaghaScript_push_int64 reported: stack overflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return;
	}
	script->sp -= size;
	*(uint64_t *)(script->pbMemory + script->sp) = val;
}
static inline uint64_t _TaghaScript_pop_int64(struct TaghaScript *script)
{
	if( !script )
		return 0L;
	uint32_t size = sizeof(uint64_t);
	if( script->bSafeMode and (script->sp+size) >= script->uiMemsize ) {
		printf("_TaghaScript_pop_int64 reported: stack underflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return 0L;
	}
	uint64_t val = *(uint64_t *)(script->pbMemory + script->sp);
	script->sp += size;
	return val;
}

static inline void _TaghaScript_push_float64(struct TaghaScript *restrict script, const double val)
{
	if( !script )
		return;
	uint32_t size = sizeof(double);
	if( script->bSafeMode and (script->sp-size) >= script->uiMemsize ) {
		printf("_TaghaScript_push_float64 reported: stack overflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return;
	}
	script->sp -= size;
	*(double *)(script->pbMemory + script->sp) = val;
}
static inline double _TaghaScript_pop_float64(struct TaghaScript *script)
{
	if( !script )
		return 0;
	uint32_t size = sizeof(double);
	if( script->bSafeMode and (script->sp+size) >= script->uiMemsize ) {
		printf("_TaghaScript_pop_float64 reported: stack underflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return 0;
	}
	double val = *(double *)(script->pbMemory + script->sp);
	script->sp += size;
	return val;
}

static inline void _TaghaScript_push_int32(struct TaghaScript *restrict script, const uint32_t val)
{
	if( !script )
		return;
	uint32_t size = sizeof(uint32_t);
	if( script->bSafeMode and (script->sp-size) >= script->uiMemsize ) {
		printf("_TaghaScript_push_int32 reported: stack overflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return;
	}
	script->sp -= size;
	*(uint32_t *)(script->pbMemory + script->sp) = val;
}
static inline uint32_t _TaghaScript_pop_int32(struct TaghaScript *script)
{
	if( !script )
		return 0;
	uint32_t size = sizeof(uint32_t);
	if( script->bSafeMode and (script->sp+size) >= script->uiMemsize ) {	// we're subtracting, did we integer underflow?
		printf("_TaghaScript_pop_int32 reported: stack underflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return 0;
	}
	uint32_t val = *(uint32_t *)(script->pbMemory + script->sp);
	script->sp += size;
	return val;
}

static inline void _TaghaScript_push_float32(struct TaghaScript *restrict script, const float val)
{
	if( !script )
		return;
	uint32_t size = sizeof(float);
	if( script->bSafeMode and (script->sp-size) >= script->uiMemsize ) {
		printf("_TaghaScript_push_float32 reported: stack overflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return;
	}
	script->sp -= size;
	*(float *)(script->pbMemory + script->sp) = val;
}
static inline float _TaghaScript_pop_float32(struct TaghaScript *script)
{
	if( !script )
		return 0;
	uint32_t size = sizeof(float);
	if( script->bSafeMode and (script->sp+size) >= script->uiMemsize ) {
		printf("_TaghaScript_pop_float32 reported: stack underflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return 0;
	}
	float val = *(float *)(script->pbMemory + script->sp);
	script->sp += size;
	return val;
}

static inline void _TaghaScript_push_short(struct TaghaScript *restrict script, const uint16_t val)
{
	if( !script )
		return;
	uint32_t size = sizeof(uint16_t);
	if( script->bSafeMode and (script->sp-size) >= script->uiMemsize ) {
		printf("_TaghaScript_push_short reported: stack overflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return;
	}
	script->sp -= size;
	*(uint16_t *)(script->pbMemory + script->sp) = val;
}
static inline uint16_t _TaghaScript_pop_short(struct TaghaScript *script)
{
	if( !script )
		return 0;
	uint32_t size = sizeof(uint16_t);
	if( script->bSafeMode and (script->sp+size) >= script->uiMemsize ) {
		printf("_TaghaScript_pop_short reported: stack underflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return 0;
	}
	uint16_t val = *(uint16_t *)(script->pbMemory + script->sp);
	script->sp += size;
	return val;
}

static inline void _TaghaScript_push_byte(struct TaghaScript *restrict script, const uint8_t val)
{
	if( !script )
		return;
	uint32_t size = sizeof(uint8_t);
	if( script->bSafeMode and (script->sp-size) >= script->uiMemsize ) {
		printf("_TaghaScript_push_byte reported: stack overflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return;
	}
	script->sp -= size;
	*(script->pbMemory + script->sp) = val;
}
static inline uint8_t _TaghaScript_pop_byte(struct TaghaScript *script)
{
	if( !script )
		return 0;
	uint32_t size = sizeof(uint8_t);
	if( script->bSafeMode and (script->sp+size) >= script->uiMemsize ) {
		printf("_TaghaScript_pop_byte reported: stack underflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return 0;
	}
	uint8_t val = *(script->pbMemory + script->sp);
	script->sp += size;
	return val;
}

static inline void _TaghaScript_push_nbytes(struct TaghaScript *restrict script, void *restrict pItem, const uint32_t bytesize)
{
	if( !script )
		return;
	if( script->bSafeMode and (script->sp-bytesize) >= script->uiMemsize ) {
		printf("_TaghaScript_push_nbytes reported: stack overflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return;
	}
	script->sp -= bytesize;
	memcpy((script->pbMemory + script->sp), pItem, bytesize);
	/*
	uint64_t i=0;
	for( i=bytesize-1 ; i<bytesize ; i-- )
		script->pbMemory[--script->sp] = ((uint8_t *)pItem)[i];
	*/
}
static inline void _TaghaScript_pop_nbytes(struct TaghaScript *restrict script, void *restrict pBuffer, const uint32_t bytesize)
{
	if( !script )
		return;
	if( script->bSafeMode and (script->sp+bytesize) >= script->uiMemsize ) {
		printf("_TaghaScript_pop_nbytes reported: stack underflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return;
	}
	memcpy(pBuffer, (script->pbMemory + script->sp), bytesize);
	script->sp += bytesize;
	/*
	uint64_t i=0;
	for( i=0 ; i<bytesize ; i++ )
		((uint8_t *)pBuffer)[i] = script->pbMemory[script->sp++];
	*/
}


static inline uint64_t _TaghaScript_peek_int64(struct TaghaScript *script)
{
	if( !script )
		return 0;
	if( script->bSafeMode and (script->sp+7) >= script->uiMemsize ) {
		printf("Tagha_peek_int32 reported: stack underflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return 0;
	}
	return *(uint64_t *)( script->pbMemory + script->sp );
}
static inline double _TaghaScript_peek_float64(struct TaghaScript *script)
{
	if( !script )
		return 0;
	if( script->bSafeMode and (script->sp+7) >= script->uiMemsize ) {
		printf("Tagha_peek_int32 reported: stack underflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return 0;
	}
	return *(double *)( script->pbMemory + script->sp );
}
static inline uint32_t _TaghaScript_peek_int32(struct TaghaScript *script)
{
	if( !script )
		return 0;
	if( script->bSafeMode and (script->sp+3) >= script->uiMemsize ) {
		printf("Tagha_peek_int32 reported: stack underflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return 0;
	}
	return *(uint32_t *)( script->pbMemory + script->sp );
}

static inline float _TaghaScript_peek_float32(struct TaghaScript *script)
{
	if( !script )
		return 0;
	if( script->bSafeMode and (script->sp+3) >= script->uiMemsize ) {
		printf("Tagha_peek_float32 reported: stack underflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return 0;
	}
	return *(float *)( script->pbMemory + script->sp );
}

static inline uint16_t _TaghaScript_peek_short(struct TaghaScript *script)
{
	if( !script )
		return 0;
	if( script->bSafeMode and (script->sp+1) >= script->uiMemsize ) {
		printf("Tagha_peek_short reported: stack underflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return 0;
	}
	return *(uint16_t *)( script->pbMemory + script->sp );
}

static inline uint8_t _TaghaScript_peek_byte(struct TaghaScript *script)
{
	if( !script )
		return 0;
	if( script->bSafeMode and (script->sp) >= script->uiMemsize ) {
		printf("Tagha_peek_byte reported: stack underflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return 0;
	}
	return script->pbMemory[script->sp];
}



//#include <unistd.h>	// sleep() func
void Tagha_exec(struct TaghaVM *vm)
{
	//printf("instruction set size == %" PRIu32 "\n", nop);
	if( !vm )
		return;
	else if( !vm->pvecScripts )
		return;
	
	uint32_t nScripts = vector_count(vm->pvecScripts);
	struct TaghaScript *script = NULL;
	
	// our value temporaries
	union conv_union conv;
	uint32_t	b,a;
	uint64_t	addr,qb,qa;
	double		db,da;
	float		fb,fa;
	uint16_t	sb,sa;
	fnNative_t	pfNative = NULL;
	bool		safemode;
	bool		debugmode;
	
#define X(x) #x ,
	// for debugging purposes.
	const char *opcode2str[] = { INSTR_SET };
#undef X
	
#define X(x) &&exec_##x ,
	// our instruction dispatch table.
	static const void *dispatch[] = { INSTR_SET };
#undef X
#undef INSTR_SET
	
	
#define DISPATCH()	++script->ip; continue
	
	for( uint32_t x=0 ; x<nScripts ; x++ ) {
		script = vector_get(vm->pvecScripts, x);
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
					printf("instruction address out of bounds! instruction == \'%" PRIWord "\'\n", script->ip);
					goto *dispatch[halt];
				}
				else if( script->pInstrStream[script->ip] > nop ) {
					printf("illegal instruction exception! instruction == \'%" PRIu32 "\' @ %" PRIWord "\n", script->pInstrStream[script->ip], script->ip);
					goto *dispatch[halt];
				}
			}
#ifdef _UNISTD_H
			sleep(1);
#endif
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
				qa = _TaghaScript_get_imm8(script);
				if( debugmode )
					printf("pushq: pushed %" PRIu64 "\n", qa);
				_TaghaScript_push_int64(script, qa);
				DISPATCH();
			
			exec_pushl:;	// push 4 bytes onto the stack
				a = _TaghaScript_get_imm4(script);
				if( debugmode )
					printf("pushl: pushed %" PRIu32 "\n", a);
				_TaghaScript_push_int32(script, a);
				DISPATCH();
			
			exec_pushs:;	// push 2 bytes onto the stack
				sa = _TaghaScript_get_imm2(script);
				_TaghaScript_push_short(script, sa);
				if( debugmode )
					printf("pushs: pushed %" PRIu32 "\n", sa);
				DISPATCH();
			
			exec_pushb:;	// push a byte onto the stack
				_TaghaScript_push_byte(script, script->pInstrStream[++script->ip]);
				if( debugmode )
					printf("pushb: pushed %" PRIu32 "\n", script->pbMemory[script->sp]);
				DISPATCH();
			
			exec_pushsp:;	// push sp onto the stack, uses 4 bytes since 'sp' is uint32_t32
				qa = script->sp;
				_TaghaScript_push_int64(script, qa);
				if( debugmode )
					printf("pushsp: pushed sp index: %" PRIWord "\n", qa);
				DISPATCH();
			
			exec_puship:;
				qa = script->ip;
				_TaghaScript_push_int64(script, qa);
				if( debugmode )
					printf("puship: pushed ip index: %" PRIWord "\n", qa);
				DISPATCH();
			
			exec_pushbp:;
				_TaghaScript_push_int64(script, script->bp);
				if( debugmode )
					printf("pushbp: pushed bp index: %" PRIWord "\n", script->bp);
				DISPATCH();
			
			exec_pushspadd:;
				qa = script->sp;
				qb = _TaghaScript_pop_int64(script);
				_TaghaScript_push_int64(script, qa+qb);
				if( debugmode )
					printf("pushspadd: added sp with %" PRIWord ", result: %" PRIWord "\n", qb, qa+qb);
				DISPATCH();
			
			exec_pushspsub:;
				qa = script->sp;
				qb = _TaghaScript_pop_int64(script);
				_TaghaScript_push_int64(script, qa-qb);
				if( debugmode )
					printf("pushspsub: subbed sp with %" PRIWord ", result: %" PRIWord "\n", qb, qa-qb);
				DISPATCH();
			
			exec_pushbpadd:;
				qa = script->bp;
				qb = _TaghaScript_pop_int64(script);
				_TaghaScript_push_int64(script, qa+qb);
				if( debugmode )
					printf("pushbpadd: added bp with %" PRIWord ", result: %" PRIWord "\n", qb, qa+qb);
				DISPATCH();
			
			exec_pushbpsub:;
				qa = script->bp;
				qb = _TaghaScript_pop_int64(script);
				_TaghaScript_push_int64(script, qa-qb);
				if( debugmode )
					printf("pushbpsub: subbed bp with %" PRIWord ", result: %" PRIWord "\n", qb, qa-qb);
				DISPATCH();
			
			exec_pushipadd:;
				qa = script->ip;
				qb = _TaghaScript_pop_int64(script);
				_TaghaScript_push_int64(script, qa+qb);
				if( debugmode )
					printf("pushipadd: added ip with %" PRIWord ", result: %" PRIWord "\n", qb, qa+qb);
				DISPATCH();
			
			exec_pushipsub:;
				qa = script->ip;
				qb = _TaghaScript_pop_int64(script);
				_TaghaScript_push_int64(script, qa-qb);
				if( debugmode )
					printf("pushipsub: subbed ip with %" PRIWord ", result: %" PRIWord "\n", qb, qa-qb);
				DISPATCH();
			
			exec_popq:;
				if( safemode and (script->sp+8) >= script->uiMemsize ) {
					printf("exec_popq reported: stack underflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
					goto *dispatch[halt];
				}
				script->sp += 8;
				if( debugmode )
					printf("popq\n");
				DISPATCH();
			
			exec_popl:;		// pop 4 bytes to eventually be overwritten
				if( safemode and (script->sp+4) >= script->uiMemsize ) {
					printf("exec_popl reported: stack underflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
					goto *dispatch[halt];
				}
				script->sp += 4;
				if( debugmode )
					printf("popl\n");
				DISPATCH();
			
			exec_pops:;		// pop 2 bytes
				if( safemode and (script->sp+2) >= script->uiMemsize ) {
					printf("exec_pops reported: stack underflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
					goto *dispatch[halt];
				}
				script->sp += 2;
				if( debugmode )
					printf("pops\n");
				DISPATCH();
			
			exec_popb:;		// pop a byte
				if( safemode and (script->sp+1) >= script->uiMemsize ) {
					printf("exec_popb reported: stack underflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
					goto *dispatch[halt];
				}
				script->sp++;
				if( debugmode )
					printf("popb\n");
				DISPATCH();
			
			exec_popsp:;
				qa = _TaghaScript_pop_int64(script);
				script->sp = qa;
				if( debugmode )
					printf("popsp: sp is now %" PRIWord ".\n", script->sp);
				DISPATCH();
			
			exec_popbp:;
				script->bp = _TaghaScript_pop_int64(script);
				if( debugmode )
					printf("popbp: bp is now %" PRIWord ".\n", script->bp);
				DISPATCH();
			
			exec_popip:;
				script->ip = _TaghaScript_pop_int64(script);
				if( debugmode )
					printf("popip: ip is now at address: %" PRIWord ".\n", script->ip);
				continue;
			
			exec_loadspq:;
				addr = _TaghaScript_pop_int64(script);
				if( safemode and (addr+7) >= script->uiMemsize ) {
					printf("exec_loadspq reported: Invalid memory access! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, addr);
					goto *dispatch[halt];
				}
				qa = *(uint64_t *)(script->pbMemory + addr);
				_TaghaScript_push_int64(script, qa);
				if( debugmode )
					printf("loaded 8-byte SP address data to T.O.S. - %" PRIu64 " from sp address [%" PRIWord "]\n", qa, addr);
				DISPATCH();
			
			exec_loadspl:;
				addr = _TaghaScript_pop_int64(script);
				if( safemode and (addr+3) >= script->uiMemsize ) {
					printf("exec_loadspl reported: Invalid memory access! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, addr);
					goto *dispatch[halt];
				}
				a = *(uint32_t *)(script->pbMemory + addr);
				_TaghaScript_push_int32(script, a);
				if( debugmode )
					printf("loaded 4-byte SP address data to T.O.S. - %" PRIu32 " from sp address [%" PRIWord "]\n", a, addr);
				DISPATCH();
			
			exec_loadsps:;
				addr = _TaghaScript_pop_int64(script);
				if( safemode and (addr+1) >= script->uiMemsize ) {
					printf("exec_loadsps reported: Invalid memory access! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, addr);
					goto *dispatch[halt];
				}
				sa = *(uint16_t *)(script->pbMemory + addr);
				_TaghaScript_push_short(script, sa);
				if( debugmode )
					printf("loaded 2-byte SP address data to T.O.S. - %" PRIu32 " from sp address [%" PRIWord "]\n", sa, addr);
				DISPATCH();
			
			exec_loadspb:;
				addr = _TaghaScript_pop_int64(script);
				if( safemode and addr >= script->uiMemsize ) {
					printf("exec_loadspb reported: Invalid memory access! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, addr);
					goto *dispatch[halt];
				}
				_TaghaScript_push_byte(script, script->pbMemory[addr]);
				if( debugmode )
					printf("loaded byte SP address data to T.O.S. - %" PRIu32 " from sp address [%" PRIWord "]\n", script->pbMemory[addr], addr);
				DISPATCH();
			
			exec_storespq:;
				addr = _TaghaScript_pop_int64(script);
				if( safemode and addr+7 >= script->uiMemsize ) {
					printf("exec_storespq reported: Invalid memory access! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, addr);
					goto *dispatch[halt];
				}
				qa = _TaghaScript_pop_int64(script);
				*(uint64_t *)(script->pbMemory + addr) = qa;
				if( debugmode )
					printf("stored 8-byte data from T.O.S. - %" PRIu64 " to sp address [%" PRIWord "]\n", qa, addr);
				DISPATCH();
			
			exec_storespl:;		// store TOS into another part of the data stack.
				addr = _TaghaScript_pop_int64(script);
				if( safemode and addr+3 >= script->uiMemsize ) {
					printf("exec_storespl reported: Invalid memory access! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, addr);
					goto *dispatch[halt];
				}
				a = _TaghaScript_pop_int32(script);
				*(uint32_t *)(script->pbMemory + addr) = a;
				if( debugmode )
					printf("stored 4-byte data from T.O.S. - %" PRIu32 " to sp address [%" PRIWord "]\n", a, addr);
				DISPATCH();
			
			exec_storesps:;
				addr = _TaghaScript_pop_int64(script);
				if( safemode and addr+1 >= script->uiMemsize ) {
					printf("exec_storesps reported: Invalid memory access! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, addr);
					goto *dispatch[halt];
				}
				sa = _TaghaScript_pop_short(script);
				*(uint16_t *)(script->pbMemory + addr) = sa;
				if( debugmode )
					printf("stored 2-byte data from T.O.S. - %" PRIu32 " to sp address [%" PRIWord "]\n", sa, addr);
				DISPATCH();
			
			exec_storespb:;
				addr = _TaghaScript_pop_int64(script);
				if( safemode and addr >= script->uiMemsize ) {
					printf("exec_storespb reported: Invalid memory access! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, addr);
					goto *dispatch[halt];
				}
				script->pbMemory[addr] = _TaghaScript_pop_byte(script);
				if( debugmode )
					printf("stored byte data from T.O.S. - %" PRIu32 " to sp address [%" PRIWord "]\n", script->pbMemory[addr], addr);
				DISPATCH();
			
			exec_copyq:;
				if( safemode and script->sp+7 >= script->uiMemsize ) {
					printf("exec_copyq reported: stack overflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
					goto *dispatch[halt];
				}
				qa = *(uint64_t *)(script->pbMemory + script->sp);
				if( debugmode )
					printf("copied 8-byte data from T.O.S. - %" PRIu64 "\n", qa);
				_TaghaScript_push_int64(script, qa);
				DISPATCH();
			
			exec_copyl:;	// copy 4 bytes of top of stack and put as new top of stack.
				if( safemode and script->sp+3 >= script->uiMemsize ) {
					printf("exec_copyl reported: stack overflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
					goto *dispatch[halt];
				}
				a = *(uint32_t *)(script->pbMemory + script->sp);
				if( debugmode )
					printf("copied 4-byte data from T.O.S. - %" PRIu32 "\n", a);
				_TaghaScript_push_int32(script, a);
				DISPATCH();
			
			exec_copys:;
				if( safemode and script->sp+1 >= script->uiMemsize ) {
					printf("exec_copys reported: stack overflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
					goto *dispatch[halt];
				}
				sa = *(uint16_t *)(script->pbMemory + script->sp);
				_TaghaScript_push_short(script, sa);
				if( debugmode )
					printf("copied 2-byte data from T.O.S. - %" PRIu32 "\n", sa);
				DISPATCH();
			
			exec_copyb:;
				_TaghaScript_push_byte(script, script->pbMemory[script->sp]);
				DISPATCH();
			
			exec_addq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				conv.ll = (int64_t)qa + (int64_t)qb;
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
				conv.i = (int32_t)a + (int32_t)b;
				if( debugmode )
					printf("signed 4 byte addition result: %" PRIi32 " == %" PRIi32 " + %" PRIi32 "\n", conv.i, a,b);
				_TaghaScript_push_int32(script, conv.ui);
				DISPATCH();
			
			exec_uaddl:;	// In C, all integers in an expression are promoted to int32, if number is bigger then uint32_t32 or int64
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
				conv.ll = (int64_t)qa - (int64_t)qb;
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
				conv.i = (int32_t)a - (int32_t)b;
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
				conv.ll = (int64_t)qa * (int64_t)qb;
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
				conv.i = (int32_t)a * (int32_t)b;
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
				conv.ll = (int64_t)qa / (int64_t)qb;
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
				conv.i = (int32_t)a / (int32_t)b;
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
				conv.ll = (int64_t)qa % (int64_t)qb;
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
				conv.i = (int32_t)a % (int32_t)b;
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
				conv.ui = (int64_t)qa < (int64_t)qb;
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
				conv.ui = (int32_t)a < (int32_t)b;
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
				conv.ui = (int64_t)qa > (int64_t)qb;
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
				conv.ui = (int32_t)a > (int32_t)b;
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
				conv.ui = (int64_t)qa == (int64_t)qb;
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
				conv.ui = (int32_t)a == (int32_t)b;
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
				conv.ui = (int64_t)qa <= (int64_t)qb;
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
				conv.ui = (int32_t)a <= (int32_t)b;
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
				conv.ui = (int64_t)qa >= (int64_t)qb;
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
				conv.ui = (int32_t)a >= (int32_t)b;
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
				conv.ui = (int64_t)qa != (int64_t)qb;
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
				conv.ui = (int32_t)a != (int32_t)b;
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
				script->ip = _TaghaScript_get_imm8(script);
				if( debugmode )
					printf("jmping to instruction address: %" PRIWord "\n", script->ip);
				continue;
			
			exec_jzq:;
				qa = _TaghaScript_pop_int64(script);
				qb = _TaghaScript_get_imm8(script);
				script->ip = (!qa) ? qb : script->ip+1;
				
				if( debugmode )
					printf("jzq'ing to instruction address: %" PRIWord "\n", script->ip);
				continue;
			
			exec_jnzq:;
				qa = _TaghaScript_pop_int64(script);
				qb = _TaghaScript_get_imm8(script);
				script->ip = (qa) ? qb : script->ip+1;
				
				if( debugmode )
					printf("jnzq'ing to instruction address: %" PRIWord "\n", script->ip);
				continue;
			
			exec_jzl:;		// check if the first 4 bytes on stack are zero, if yes then jump it.
				a = _TaghaScript_pop_int32(script);
				qb = _TaghaScript_get_imm8(script);
				script->ip = (!a) ? qb : script->ip+1;
				
				if( debugmode )
					printf("jzl'ing to instruction address: %" PRIWord "\n", script->ip);	//opcode2str[script->pInstrStream[script->ip]]
				continue;
			
			exec_jnzl:;
				a = _TaghaScript_pop_int32(script);
				qb = _TaghaScript_get_imm8(script);
				script->ip = (a) ? qb : script->ip+1;
				
				if( debugmode )
					printf("jnzl'ing to instruction address: %" PRIWord "\n", script->ip);
				continue;
			
			exec_call:;		// support functions
				qa = _TaghaScript_get_imm8(script);	// get func address
				if( debugmode )
					printf("call :: calling address: %" PRIWord "\n", qa);
				
				_TaghaScript_push_int64(script, script->ip+1);	// save return address.
				if( debugmode )
					printf("call :: return addr: %" PRIWord "\n", script->ip+1);
				
				script->ip = qa;	// jump to instruction
				
				_TaghaScript_push_int64(script, script->bp);	// push ebp;
				if( debugmode )
					printf("call :: pushing bp: %" PRIWord "\n", script->bp);
				script->bp = script->sp;	// mov ebp, esp;
				
				if( debugmode )
					printf("call :: bp set to sp: %" PRIWord "\n", script->bp);
				continue;
			
			exec_calls:;	// support local function pointers
				qa = _TaghaScript_pop_int64(script);	// get func address
				if( debugmode )
					printf("calls: calling address: %" PRIWord "\n", qa);
				
				_TaghaScript_push_int64(script, script->ip+1);	// save return address.
				
				if( debugmode )
					printf("call return addr: %" PRIWord "\n", script->ip+1);
				
				_TaghaScript_push_int64(script, script->bp);	// push ebp
				script->bp = script->sp;	// mov ebp, esp
				script->ip = qa;	// jump to instruction
				
				if( debugmode )
					printf("script->bp: %" PRIWord "\n", script->sp);
				continue;
			
			exec_ret:;
				script->sp = script->bp;	// mov esp, ebp
				if( debugmode )
					printf("ret ;: sp set to bp, sp == %" PRIWord "\n", script->sp);
				
				script->bp = _TaghaScript_pop_int64(script);	// pop ebp
				if( debugmode )
					printf("ret ;: popped to bp, bp == %" PRIWord "\n", script->bp);
				
				script->ip = _TaghaScript_pop_int64(script);	// pop return address.
				if( debugmode )
					printf("returning to address: %" PRIWord "\n", script->ip);
				continue;
			
			exec_retx:; {	// for functions that return something.
				a = _TaghaScript_get_imm4(script);
				uint8_t bytebuffer[a];
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
					printf("retx :: sp set to bp, sp == %" PRIWord "\n", script->sp);
					
				script->bp = _TaghaScript_pop_int64(script);	// pop ebp
				if( debugmode )
					printf("retx :: popping bp, bp == %" PRIWord "\n", script->bp);
					
				script->ip = _TaghaScript_pop_int64(script);	// pop return address.
				if( debugmode )
					printf("retx :: popping ip, bp == %" PRIWord "\n", script->ip);
					
				_TaghaScript_push_nbytes(script, bytebuffer, a);	// push back return data!
				if( debugmode )
					printf("retx :: pushed back %" PRIu32 " bytes\n", a);
				continue;
			}
			
			exec_pushnataddr:;
				if( safemode and !script->pstrNatives ) {
					printf("exec_pushnataddr reported: native table is NULL! Current instruction address: %" PRIWord "\n", script->ip);
					goto *dispatch[halt];
				}
				// match native name to get an index.
				a = _TaghaScript_get_imm4(script);
				if( safemode and a >= script->uiNatives  ) {
					printf("exec_pushnataddr reported: native index \'%" PRIu32 "\' is out of bounds! Current instruction address: %" PRIWord "\n", a, script->ip);
					goto *dispatch[halt];
				}
				pfNative = (fnNative_t)(uintptr_t)map_find(vm->pmapNatives, script->pstrNatives[a]);
				if( safemode and !pfNative ) {
					printf("exec_pushnataddr reported: native \'%s\' not registered! Current instruction address: %" PRIWord "\n", script->pstrNatives[a], script->ip);
					goto *dispatch[halt];
				}
				_TaghaScript_push_int64(script, (uintptr_t)pfNative);
				if( debugmode )
					printf("pushnataddr: pushed native func addr: %" PRIWord "\n", (uint64_t)(uintptr_t)pfNative);
				DISPATCH();
			
			exec_callnat:; {	// call a native
				if( safemode and !script->pstrNatives ) {
					printf("exec_callnat reported: native table is NULL! Current instruction address: %" PRIWord "\n", script->ip);
					goto *dispatch[halt];
				}
				a = _TaghaScript_get_imm4(script);
				if( safemode and a >= script->uiNatives  ) {
					printf("exec_callnat reported: native index \'%" PRIu32 "\' is out of bounds! Current instruction address: %" PRIWord "\n", a, script->ip);
					goto *dispatch[halt];
				}
				
				pfNative = (fnNative_t) (uintptr_t)map_find(vm->pmapNatives, script->pstrNatives[a]);
				if( safemode and !pfNative ) {
					printf("exec_callnat reported: native \'%s\' not registered! Current instruction address: %" PRIWord "\n", script->pstrNatives[a], script->ip);
					goto *dispatch[halt];
				}
				// how many bytes to push to native.
				const uint32_t bytes = _TaghaScript_get_imm4(script);
				// how many arguments pushed as native args
				const uint32_t argcount = _TaghaScript_get_imm4(script);
				if( debugmode ) {
					printf("callnat: Calling func addr: %"PRIWord" ", (uint64_t)(uintptr_t)pfNative);
					printf("with %"PRIu32" amount of bytes pushed ", bytes);
					printf("and %"PRIu32" args / parameters.\n", argcount);
				}
				//uint8_t params[bytes];
				//_TaghaScript_pop_nbytes(script, params, bytes);
				(*pfNative)(script, argcount, bytes/*, params*/);
				DISPATCH();
			}
			/* support calling natives via function pointers */
			exec_callnats:; {	// call native by func ptr allocated on stack
				pfNative = (fnNative_t)(uintptr_t) _TaghaScript_pop_int64(script);
				if( safemode and !pfNative ) {
					printf("exec_callnats reported: native \'%s\' not registered! Current instruction address: %" PRIWord "\n", script->pstrNatives[a], script->ip);
					goto *dispatch[halt];
				}
				const uint32_t bytes = _TaghaScript_get_imm4(script);
				const uint32_t argcount = _TaghaScript_get_imm4(script);
				if( debugmode ) {
					printf("callnats: Calling func addr: %"PRIWord" ", (uint64_t)(uintptr_t)pfNative);
					printf("with %"PRIu32" amount of bytes pushed ", bytes);
					printf("and %"PRIu32" args / parameters.\n", argcount);
				}
				//uint8_t params[bytes];
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
