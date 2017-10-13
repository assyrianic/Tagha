
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include "tagha.h"


// This is strictly for long doubles
static inline void _TaghaScript_get_immn(struct TaghaScript *restrict script, void *restrict pBuffer, const uint32_t bytesize)
{
	if( !script or !pBuffer )
		return;
	
	if( script->m_bSafeMode and (script->ip+bytesize) >= script->m_uiInstrSize ) {
		printf("_TaghaScript_get_immn reported: instr overflow! Current instruction address: %" PRIWord "\n", script->ip);
		return;
	}
	
	script->IP++;
	memcpy(pBuffer, script->IP, bytesize);
	script->IP += bytesize-1;
	script->ip += bytesize;
}


/*
 * Private, inlined implementations of getting values from
 * the instruction stream to help with speeding up code.
 */

static inline uint64_t _TaghaScript_get_imm8(struct TaghaScript *restrict script)
{
	if( !script )
		return 0L;
	if( script->m_bSafeMode and (script->ip+8) >= script->m_uiInstrSize ) {
		printf("_TaghaScript_get_imm8 reported: instr overflow! Current instruction address: %" PRIWord "\n", script->ip);
		return 0L;
	}
	script->IP++;
	uint64_t val = *(uint64_t *)(script->IP);
	script->IP += 7;
	script->ip += 8;
	return val;
}

static inline uint32_t _TaghaScript_get_imm4(struct TaghaScript *restrict script)
{
	if( !script )
		return 0;
	if( script->m_bSafeMode and (script->ip+4) >= script->m_uiInstrSize ) {
		printf("_TaghaScript_get_imm4 reported: instr overflow! Current instruction address: %" PRIWord "\n", script->ip);
		return 0;
	}
	script->IP++;
	uint32_t val = *(uint32_t *)(script->IP);
	script->IP += 3;
	script->ip += 4;
	return val;
}

static inline uint16_t _TaghaScript_get_imm2(struct TaghaScript *restrict script)
{
	if( !script )
		return 0;
	if( script->m_bSafeMode and (script->ip+2) >= script->m_uiInstrSize ) {
		printf("_TaghaScript_get_imm2 reported: instr overflow! Current instruction address: %" PRIWord "\n", script->ip);
		return 0;
	}
	script->IP++;
	uint16_t val = *(uint16_t *)(script->IP);
	script->IP += 1;
	script->ip += 2;
	return val;
}


/*
 * private, inlined versions of the Tagha API for optimization
 */

static inline void _TaghaScript_push_int64(struct TaghaScript *restrict script, const uint64_t val)
{
	if( !script )
		return;
	uint32_t size = sizeof(uint64_t);
	if( script->m_bSafeMode and (script->sp-size) >= script->m_uiMemsize ) {
		printf("_TaghaScript_push_int64 reported: stack overflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return;
	}
	script->sp -= size;
	script->SP -= size;
	*(uint64_t *)(script->SP) = val;
}
static inline uint64_t _TaghaScript_pop_int64(struct TaghaScript *script)
{
	if( !script )
		return 0L;
	uint32_t size = sizeof(uint64_t);
	if( script->m_bSafeMode and (script->sp+size) >= script->m_uiMemsize ) {
		printf("_TaghaScript_pop_int64 reported: stack underflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return 0L;
	}
	uint64_t val = *(uint64_t *)(script->SP);
	script->sp += size;
	script->SP += size;
	return val;
}

static inline void _TaghaScript_push_double(struct TaghaScript *restrict script, const double val)
{
	if( !script )
		return;
	uint32_t size = sizeof(double);
	if( script->m_bSafeMode and (script->sp-size) >= script->m_uiMemsize ) {
		printf("_TaghaScript_push_double reported: stack overflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return;
	}
	script->sp -= size;
	script->SP -= size;
	*(double *)(script->SP) = val;
}
static inline double _TaghaScript_pop_double(struct TaghaScript *script)
{
	if( !script )
		return 0;
	uint32_t size = sizeof(double);
	if( script->m_bSafeMode and (script->sp+size) >= script->m_uiMemsize ) {
		printf("_TaghaScript_pop_double reported: stack underflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return 0;
	}
	double val = *(double *)(script->SP);
	script->sp += size;
	script->SP += size;
	return val;
}

static inline void _TaghaScript_push_int32(struct TaghaScript *restrict script, const uint32_t val)
{
	if( !script )
		return;
	uint32_t size = sizeof(uint32_t);
	if( script->m_bSafeMode and (script->sp-size) >= script->m_uiMemsize ) {
		printf("_TaghaScript_push_int32 reported: stack overflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return;
	}
	script->sp -= size;
	script->SP -= size;
	*(uint32_t *)(script->SP) = val;
}
static inline uint32_t _TaghaScript_pop_int32(struct TaghaScript *script)
{
	if( !script )
		return 0;
	uint32_t size = sizeof(uint32_t);
	if( script->m_bSafeMode and (script->sp+size) >= script->m_uiMemsize ) {	// we're subtracting, did we integer underflow?
		printf("_TaghaScript_pop_int32 reported: stack underflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return 0;
	}
	uint32_t val = *(uint32_t *)(script->SP);
	script->sp += size;
	script->SP += size;
	return val;
}

static inline void _TaghaScript_push_float(struct TaghaScript *restrict script, const float val)
{
	if( !script )
		return;
	uint32_t size = sizeof(float);
	if( script->m_bSafeMode and (script->sp-size) >= script->m_uiMemsize ) {
		printf("_TaghaScript_push_float reported: stack overflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return;
	}
	script->sp -= size;
	script->SP -= size;
	*(float *)(script->SP) = val;
}
static inline float _TaghaScript_pop_float(struct TaghaScript *script)
{
	if( !script )
		return 0;
	uint32_t size = sizeof(float);
	if( script->m_bSafeMode and (script->sp+size) >= script->m_uiMemsize ) {
		printf("_TaghaScript_pop_float reported: stack underflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return 0;
	}
	float val = *(float *)(script->SP);
	script->sp += size;
	script->SP += size;
	return val;
}

static inline void _TaghaScript_push_short(struct TaghaScript *restrict script, const uint16_t val)
{
	if( !script )
		return;
	uint32_t size = sizeof(uint16_t);
	if( script->m_bSafeMode and (script->sp-size) >= script->m_uiMemsize ) {
		printf("_TaghaScript_push_short reported: stack overflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return;
	}
	script->sp -= size;
	script->SP -= size;
	*(uint16_t *)(script->SP) = val;
}
static inline uint16_t _TaghaScript_pop_short(struct TaghaScript *script)
{
	if( !script )
		return 0;
	uint32_t size = sizeof(uint16_t);
	if( script->m_bSafeMode and (script->sp+size) >= script->m_uiMemsize ) {
		printf("_TaghaScript_pop_short reported: stack underflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return 0;
	}
	uint16_t val = *(uint16_t *)(script->SP);
	script->sp += size;
	script->SP += size;
	return val;
}

static inline void _TaghaScript_push_byte(struct TaghaScript *restrict script, const uint8_t val)
{
	if( !script )
		return;
	uint32_t size = sizeof(uint8_t);
	if( script->m_bSafeMode and (script->sp-size) >= script->m_uiMemsize ) {
		printf("_TaghaScript_push_byte reported: stack overflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return;
	}
	script->sp -= size;
	script->SP -= size;
	*(uint8_t *)(script->SP) = val;
}
static inline uint8_t _TaghaScript_pop_byte(struct TaghaScript *script)
{
	if( !script )
		return 0;
	uint32_t size = sizeof(uint8_t);
	if( script->m_bSafeMode and (script->sp+size) >= script->m_uiMemsize ) {
		printf("_TaghaScript_pop_byte reported: stack underflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return 0;
	}
	uint8_t val = *(uint8_t *)(script->SP);
	script->sp += size;
	script->SP += size;
	return val;
}

static inline void _TaghaScript_push_nbytes(struct TaghaScript *restrict script, void *restrict pItem, const uint32_t bytesize)
{
	if( !script )
		return;
	if( script->m_bSafeMode and (script->sp-bytesize) >= script->m_uiMemsize ) {
		printf("_TaghaScript_push_nbytes reported: stack overflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return;
	}
	script->sp -= bytesize;
	script->SP -= bytesize;
	memcpy(script->SP, pItem, bytesize);
	/*
	uint64_t i=0;
	for( i=bytesize-1 ; i<bytesize ; i-- )
		script->m_pMemory[--script->sp] = ((uint8_t *)pItem)[i];
	*/
}
static inline void _TaghaScript_pop_nbytes(struct TaghaScript *restrict script, void *restrict pBuffer, const uint32_t bytesize)
{
	if( !script )
		return;
	if( script->m_bSafeMode and (script->sp+bytesize) >= script->m_uiMemsize ) {
		printf("_TaghaScript_pop_nbytes reported: stack underflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return;
	}
	memcpy(pBuffer, script->SP, bytesize);
	script->sp += bytesize;
	script->SP += bytesize;
	/*
	uint64_t i=0;
	for( i=0 ; i<bytesize ; i++ )
		((uint8_t *)pBuffer)[i] = script->m_pMemory[script->sp++];
	*/
}


static inline uint64_t _TaghaScript_peek_int64(struct TaghaScript *script)
{
	if( !script )
		return 0;
	if( script->m_bSafeMode and (script->sp+7) >= script->m_uiMemsize ) {
		printf("Tagha_peek_int32 reported: stack underflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return 0;
	}
	return *(uint64_t *)( script->SP );
}
static inline double _TaghaScript_peek_double(struct TaghaScript *script)
{
	if( !script )
		return 0;
	if( script->m_bSafeMode and (script->sp+7) >= script->m_uiMemsize ) {
		printf("Tagha_peek_int32 reported: stack underflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return 0;
	}
	return *(double *)( script->SP );
}
static inline uint32_t _TaghaScript_peek_int32(struct TaghaScript *script)
{
	if( !script )
		return 0;
	if( script->m_bSafeMode and (script->sp+3) >= script->m_uiMemsize ) {
		printf("Tagha_peek_int32 reported: stack underflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return 0;
	}
	return *(uint32_t *)( script->SP );
}

static inline float _TaghaScript_peek_float(struct TaghaScript *script)
{
	if( !script )
		return 0;
	if( script->m_bSafeMode and (script->sp+3) >= script->m_uiMemsize ) {
		printf("Tagha_peek_float reported: stack underflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return 0;
	}
	return *(float *)( script->SP );
}

static inline uint16_t _TaghaScript_peek_short(struct TaghaScript *script)
{
	if( !script )
		return 0;
	if( script->m_bSafeMode and (script->sp+1) >= script->m_uiMemsize ) {
		printf("Tagha_peek_short reported: stack underflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return 0;
	}
	return *(uint16_t *)( script->SP );
}

static inline uint8_t _TaghaScript_peek_byte(struct TaghaScript *script)
{
	if( !script )
		return 0;
	if( script->m_bSafeMode and (script->sp) >= script->m_uiMemsize ) {
		printf("Tagha_peek_byte reported: stack underflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return 0;
	}
	return *(uint8_t *)( script->SP );
}



//#include <unistd.h>	// sleep() func
void Tagha_exec(struct TaghaVM *vm)
{
	//printf("instruction set size == %" PRIu32 "\n", nop);
	if( !vm )
		return;
	else if( !vm->m_pvecScripts )
		return;
	
	uint32_t nScripts = vector_count(vm->m_pvecScripts);
	struct TaghaScript *script = NULL;
	
	// our value temporaries
	uint32_t	b,a;
	uint64_t	qb,qa;
	double		db,da;
	float		fb,fa;
	uint16_t	sb,sa;
	fnNative_t	pfNative = NULL;
	bool		safemode;
	bool		debugmode;
	uint8_t		*addr;
	
#define X(x) #x ,
	// for debugging purposes.
	const char *opcode2str[] = { INSTR_SET };
#undef X
	
#define X(x) &&exec_##x ,
	// our instruction dispatch table.
	static const void *dispatch[] = { INSTR_SET };
#undef X
#undef INSTR_SET
	
	
#define DISPATCH()	++script->ip; ++script->IP; continue
	
	for( uint32_t x=0 ; x<nScripts ; x++ ) {
		script = vector_get(vm->m_pvecScripts, x);
		if( !script )
			continue;
		else if( !script->m_pText )
			continue;
		
		while( 1 ) {
			script->m_uiMaxInstrs--;
			if( !script->m_uiMaxInstrs )
				break;
			
			safemode = script->m_bSafeMode;
			debugmode = script->m_bDebugMode;
			if( safemode ) {
				if( script->ip >= script->m_uiInstrSize ) {
					printf("instruction address out of bounds! instruction == \'%" PRIWord "\'\n", script->ip);
					goto *dispatch[halt];
				}
				else if( *script->IP > nop ) {
					printf("illegal instruction exception! instruction == \'%" PRIu32 "\' @ %" PRIWord "\n", *script->IP, script->ip);
					goto *dispatch[halt];
				}
			}
#ifdef _UNISTD_H
			sleep(1);
#endif
			//printf( "current instruction == \"%s\" @ ip == %" PRIu32 "\n", opcode2str[script->m_pText[script->ip]], script->ip );
			goto *dispatch[ *script->IP ];
			
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
					printf("pushs: pushed %" PRIu32 "\n", a);
				DISPATCH();
			
			exec_pushb:;	// push a byte onto the stack
				_TaghaScript_push_byte(script, *++script->IP);
				++script->ip;
				if( debugmode )
					printf("pushb: pushed %" PRIu32 "\n", script->m_pMemory[script->sp]);
				DISPATCH();
			
			exec_pushsp:;	// push sp onto the stack, uses 4 bytes since 'sp' is uint32_t32
				qa = script->sp;
				_TaghaScript_push_int64(script, qa);
				if( debugmode )
					printf("pushsp: pushed sp : %" PRIWord "\n", qa);
				DISPATCH();
			
			exec_puship:;
				qa = (uintptr_t)script->IP;
				_TaghaScript_push_int64(script, qa);
				if( debugmode )
					printf("puship: pushed ip : %" PRIWord "\n", qa);
				DISPATCH();
			
			exec_pushbp:;
				_TaghaScript_push_int64(script, (uintptr_t)script->BP);
				if( debugmode )
					printf("pushbp: pushed bp : %" PRIuPTR "\n", (uintptr_t)script->BP);
				DISPATCH();
			
			exec_pushoffset:;
				qa = _TaghaScript_get_imm8(script);
				_TaghaScript_push_int64(script, (uintptr_t)(script->m_pMemory + qa));
				if( debugmode )
					printf("pushoffset: pushed offset index - %" PRIWord " : %p\n", qa, (script->m_pMemory + qa));
				DISPATCH();
			
			exec_pushspadd:;
				qa = (uintptr_t)script->SP;
				qb = _TaghaScript_pop_int64(script);
				_TaghaScript_push_int64(script, qa+qb);
				if( debugmode )
					printf("pushspadd: added sp with %" PRIWord ", result: %" PRIWord "\n", qb, qa+qb);
				DISPATCH();
			
			exec_pushspsub:;
				qa = (uintptr_t)script->SP;
				qb = _TaghaScript_pop_int64(script);
				_TaghaScript_push_int64(script, qa-qb);
				if( debugmode )
					printf("pushspsub: subbed sp with %" PRIWord ", result: %" PRIWord "\n", qb, qa-qb);
				DISPATCH();
			
			exec_pushbpadd:;
				qa = (uintptr_t)script->BP;
				qb = _TaghaScript_pop_int64(script);
				_TaghaScript_push_int64(script, qa+qb);
				if( debugmode )
					printf("pushbpadd: added bp with %" PRIWord ", result: %" PRIWord "\n", qb, qa+qb);
				DISPATCH();
			
			exec_pushbpsub:;
				qa = (uintptr_t)script->BP;
				qb = _TaghaScript_pop_int64(script);
				_TaghaScript_push_int64(script, qa-qb);
				if( debugmode )
					printf("pushbpsub: subbed bp with %" PRIWord ", result: %" PRIWord "\n", qb, qa-qb);
				DISPATCH();
			
			exec_pushipadd:;
				qa = (uintptr_t)script->IP;
				qb = _TaghaScript_pop_int64(script);
				_TaghaScript_push_int64(script, qa+qb);
				if( debugmode )
					printf("pushipadd: added ip with %" PRIWord ", result: %" PRIWord "\n", qb, qa+qb);
				DISPATCH();
			
			exec_pushipsub:;
				qa = (uintptr_t)script->IP;
				qb = _TaghaScript_pop_int64(script);
				_TaghaScript_push_int64(script, qa-qb);
				if( debugmode )
					printf("pushipsub: subbed ip with %" PRIWord ", result: %" PRIWord "\n", qb, qa-qb);
				DISPATCH();
			
			exec_popq:;
				if( safemode and (script->sp+8) >= script->m_uiMemsize ) {
					printf("exec_popq reported: stack underflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
					goto *dispatch[halt];
				}
				script->SP += 8;
				script->sp = script->SP - script->m_pMemory;
				if( debugmode )
					printf("popq\n");
				DISPATCH();
			
			exec_popsp:;
				script->SP = (uint8_t *)(uintptr_t)_TaghaScript_pop_int64(script);
				script->sp = script->SP - script->m_pMemory;
				if( debugmode )
					printf("popsp: sp is now %" PRIWord ".\n", script->sp);
				DISPATCH();
			
			exec_popbp:;
				script->BP = (uint8_t *)(uintptr_t)_TaghaScript_pop_int64(script);
				script->bp = script->BP - script->m_pMemory;
				if( debugmode )
					printf("popbp: bp is now %" PRIWord ".\n", script->bp);
				DISPATCH();
			
			exec_popip:;
				script->IP = (uint8_t *)(uintptr_t)_TaghaScript_pop_int64(script);
				script->ip = script->IP - script->m_pText;
				if( debugmode )
					printf("popip: ip is now at address: %" PRIWord ".\n", script->ip);
				continue;
			
			exec_loadspq:;
				addr = (uint8_t *)(uintptr_t)_TaghaScript_pop_int64(script);
				if( safemode and !addr ) {
					printf("exec_loadspq reported: Invalid memory access! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, (uint64_t)(addr-script->m_pMemory));
					goto *dispatch[halt];
				}
				qa = *(uint64_t *)addr;
				_TaghaScript_push_int64(script, qa);
				if( debugmode )
					printf("loaded 8-byte data to T.O.S. - %" PRIu64 " from sp offset [%" PRIWord "]\n", qa, (uint64_t)(addr-script->m_pMemory));
				DISPATCH();
			
			exec_loadspl:;
				addr = (uint8_t *)(uintptr_t)_TaghaScript_pop_int64(script);
				if( safemode and !addr ) {
					printf("exec_loadspl reported: Invalid memory access! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, (uint64_t)(addr-script->m_pMemory));
					goto *dispatch[halt];
				}
				a = *(uint32_t *)addr;
				_TaghaScript_push_int32(script, a);
				if( debugmode )
					printf("loaded 4-byte data to T.O.S. - %" PRIu32 " from sp offset [%" PRIWord "]\n", a, (uint64_t)(addr-script->m_pMemory));
				DISPATCH();
			
			exec_loadsps:;
				addr = (uint8_t *)(uintptr_t)_TaghaScript_pop_int64(script);
				if( safemode and !addr ) {
					printf("exec_loadsps reported: Invalid memory access! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, (uint64_t)(addr-script->m_pMemory));
					goto *dispatch[halt];
				}
				sa = *(uint16_t *)addr;
				_TaghaScript_push_short(script, sa);
				if( debugmode )
					printf("loaded 2-byte data to T.O.S. - %" PRIu32 " from sp offset [%" PRIWord "]\n", sa, (uint64_t)(addr-script->m_pMemory));
				DISPATCH();
			
			exec_loadspb:;
				addr = (uint8_t *)(uintptr_t)_TaghaScript_pop_int64(script);
				if( safemode and !addr ) {
					printf("exec_loadspb reported: Invalid memory access! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, (uint64_t)(addr-script->m_pMemory));
					goto *dispatch[halt];
				}
				_TaghaScript_push_byte(script, *addr);
				if( debugmode )
					printf("loaded byte data to T.O.S. - %" PRIu32 " from sp offset [%" PRIWord "]\n", *addr, (uint64_t)(addr-script->m_pMemory));
				DISPATCH();
			
			exec_storespq:;
				addr = (uint8_t *)(uintptr_t)_TaghaScript_pop_int64(script);
				if( safemode and !addr ) {
					printf("exec_storespq reported: Invalid memory access! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, (uint64_t)(addr-script->m_pMemory));
					goto *dispatch[halt];
				}
				qa = _TaghaScript_pop_int64(script);
				*(uint64_t *)addr = qa;
				if( debugmode )
					printf("stored 8-byte data from T.O.S. - %" PRIu64 " to sp offset [%" PRIWord "]\n", qa, (uint64_t)(addr-script->m_pMemory));
				DISPATCH();
			
			exec_storespl:;		// store TOS into another part of the data stack.
				addr = (uint8_t *)(uintptr_t)_TaghaScript_pop_int64(script);
				if( safemode and !addr ) {
					printf("exec_storespl reported: Invalid memory access! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, (uint64_t)(addr-script->m_pMemory));
					goto *dispatch[halt];
				}
				a = _TaghaScript_pop_int32(script);
				*(uint32_t *)addr = a;
				if( debugmode )
					printf("stored 4-byte data from T.O.S. - %" PRIu32 " to sp offset [%" PRIWord "]\n", a, (uint64_t)(addr-script->m_pMemory));
				DISPATCH();
			
			exec_storesps:;
				addr = (uint8_t *)(uintptr_t)_TaghaScript_pop_int64(script);
				if( safemode and !addr ) {
					printf("exec_storesps reported: Invalid memory access! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, (uint64_t)(addr-script->m_pMemory));
					goto *dispatch[halt];
				}
				sa = _TaghaScript_pop_short(script);
				*(uint16_t *)addr = sa;
				if( debugmode )
					printf("stored 2-byte data from T.O.S. - %" PRIu32 " to sp offset [%" PRIWord "]\n", sa, (uint64_t)(addr-script->m_pMemory));
				DISPATCH();
			
			exec_storespb:;
				addr = (uint8_t *)(uintptr_t)_TaghaScript_pop_int64(script);
				if( safemode and !addr ) {
					printf("exec_storespb reported: Invalid memory access! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, (uint64_t)(addr-script->m_pMemory));
					goto *dispatch[halt];
				}
				*addr = _TaghaScript_pop_byte(script);
				if( debugmode )
					printf("stored byte data from T.O.S. - %" PRIu32 " to sp offset [%" PRIWord "]\n", *addr, (uint64_t)(addr-script->m_pMemory));
				DISPATCH();
			
			exec_copyq:;
				if( safemode and script->sp+7 >= script->m_uiMemsize ) {
					printf("exec_copyq reported: stack overflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
					goto *dispatch[halt];
				}
				qa = *(uint64_t *)(script->m_pMemory + script->sp);
				if( debugmode )
					printf("copied 8-byte data from T.O.S. - %" PRIu64 "\n", qa);
				_TaghaScript_push_int64(script, qa);
				DISPATCH();
			
			exec_copyl:;	// copy 4 bytes of top of stack and put as new top of stack.
				if( safemode and script->sp+3 >= script->m_uiMemsize ) {
					printf("exec_copyl reported: stack overflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
					goto *dispatch[halt];
				}
				a = *(uint32_t *)(script->m_pMemory + script->sp);
				if( debugmode )
					printf("copied 4-byte data from T.O.S. - %" PRIu32 "\n", a);
				_TaghaScript_push_int32(script, a);
				DISPATCH();
			
			exec_copys:;
				if( safemode and script->sp+1 >= script->m_uiMemsize ) {
					printf("exec_copys reported: stack overflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
					goto *dispatch[halt];
				}
				a = *(uint16_t *)(script->m_pMemory + script->sp);
				_TaghaScript_push_short(script, (uint16_t)a);
				if( debugmode )
					printf("copied 2-byte data from T.O.S. - %" PRIu32 "\n", a);
				DISPATCH();
			
			exec_copyb:;
				_TaghaScript_push_byte(script, script->m_pMemory[script->sp]);
				DISPATCH();
			
			exec_addq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				if( debugmode )
					printf("signed 8 byte addition result: %" PRIi64 " == %" PRIi64 " + %" PRIi64 "\n", (int64_t)qa + (int64_t)qb, qa,qb);
				_TaghaScript_push_int64(script, (int64_t)qa + (int64_t)qb);
				DISPATCH();
				
			exec_uaddq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				if( debugmode )
					printf("unsigned 8 byte addition result: %" PRIu64 " == %" PRIu64 " + %" PRIu64 "\n", qa+qb, qa,qb);
				_TaghaScript_push_int64(script, qa+qb);
				DISPATCH();
			
			exec_addl:;		// pop 8 bytes, signed addition, and push 4 byte result to top of stack
				b = _TaghaScript_pop_int32(script);
				a = _TaghaScript_pop_int32(script);
				if( debugmode )
					printf("signed 4 byte addition result: %" PRIi32 " == %" PRIi32 " + %" PRIi32 "\n", (int32_t)a + (int32_t)b, a,b);
				_TaghaScript_push_int32(script, (int32_t)a + (int32_t)b);
				DISPATCH();
			
			exec_uaddl:;	// In C, all integers in an expression are promoted to int32, if number is bigger then uint32_t32 or int64
				b = _TaghaScript_pop_int32(script);
				a = _TaghaScript_pop_int32(script);
				if( debugmode )
					printf("unsigned 4 byte addition result: %" PRIu32 " == %" PRIu32 " + %" PRIu32 "\n", a+b, a,b);
				_TaghaScript_push_int32(script, a+b);
				DISPATCH();
			
			exec_addf:;
				fb = _TaghaScript_pop_float(script);
				fa = _TaghaScript_pop_float(script);
				if( debugmode )
					printf("4-byte float addition result: %f == %f + %f\n", fa+fb, fa,fb);
				_TaghaScript_push_float(script, fa+fb);
				DISPATCH();
			
			exec_addf64:;
				db = _TaghaScript_pop_double(script);
				da = _TaghaScript_pop_double(script);
				if( debugmode )
					printf("8-byte float addition result: %f == %f + %f\n", da+db, da,db);
				_TaghaScript_push_double(script, da+db);
				DISPATCH();
			
			exec_subq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				if( debugmode )
					printf("signed 8 byte subtraction result: %" PRIi64 " == %" PRIi64 " - %" PRIi64 "\n", (int64_t)qa - (int64_t)qb, qa,qb);
				_TaghaScript_push_int64(script, (int64_t)qa - (int64_t)qb);
				DISPATCH();
			
			exec_usubq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				if( debugmode )
					printf("unsigned 8 byte subtraction result: %" PRIu64 " == %" PRIu64 " - %" PRIu64 "\n", qa-qb, qa,qb);
				_TaghaScript_push_int64(script, qa-qb);
				DISPATCH();
			
			exec_subl:;
				b = _TaghaScript_pop_int32(script);
				a = _TaghaScript_pop_int32(script);
				if( debugmode )
					printf("signed 4 byte subtraction result: %" PRIi32 " == %" PRIi32 " - %" PRIi32 "\n", (int32_t)a - (int32_t)b, a,b);
				_TaghaScript_push_int32(script, (int32_t)a - (int32_t)b);
				DISPATCH();
			
			exec_usubl:;
				b = _TaghaScript_pop_int32(script);
				a = _TaghaScript_pop_int32(script);
				if( debugmode )
					printf("unsigned 4 byte subtraction result: %" PRIu32 " == %" PRIu32 " - %" PRIu32 "\n", a-b, a,b);
				_TaghaScript_push_int32(script, a-b);
				DISPATCH();
			
			exec_subf:;
				fb = _TaghaScript_pop_float(script);
				fa = _TaghaScript_pop_float(script);
				if( debugmode )
					printf("4-byte float subtraction result: %f == %f - %f\n", fa-fb, fa,fb);
				_TaghaScript_push_float(script, fa-fb);
				DISPATCH();
			
			exec_subf64:;
				db = _TaghaScript_pop_double(script);
				da = _TaghaScript_pop_double(script);
				if( debugmode )
					printf("8-byte float subtraction result: %f == %f - %f\n", da-db, da,db);
				_TaghaScript_push_double(script, da-db);
				DISPATCH();
			
			exec_mulq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				if( debugmode )
					printf("signed 8 byte mult result: %" PRIi64 " == %" PRIi64 " * %" PRIi64 "\n", (int64_t)qa * (int64_t)qb, qa,qb);
				_TaghaScript_push_int64(script, (int64_t)qa * (int64_t)qb);
				DISPATCH();
			
			exec_umulq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				if( debugmode )
					printf("unsigned 8 byte mult result: %" PRIu64 " == %" PRIu64 " * %" PRIu64 "\n", qa*qb, qa,qb);
				_TaghaScript_push_int64(script, qa*qb);
				DISPATCH();
			
			exec_mull:;
				b = _TaghaScript_pop_int32(script);
				a = _TaghaScript_pop_int32(script);
				if( debugmode )
					printf("signed 4 byte mult result: %" PRIi32 " == %" PRIi32 " * %" PRIi32 "\n", (int32_t)a * (int32_t)b, a,b);
				_TaghaScript_push_int32(script, (int32_t)a * (int32_t)b);
				DISPATCH();
			
			exec_umull:;
				b = _TaghaScript_pop_int32(script);
				a = _TaghaScript_pop_int32(script);
				if( debugmode )
					printf("unsigned 4 byte mult result: %" PRIu32 " == %" PRIu32 " * %" PRIu32 "\n", a*b, a,b);
				_TaghaScript_push_int32(script, a*b);
				DISPATCH();
			
			exec_mulf:;
				fb = _TaghaScript_pop_float(script);
				fa = _TaghaScript_pop_float(script);
				if( debugmode )
					printf("4-byte float mult result: %f == %f * %f\n", fa*fb, fa,fb);
				_TaghaScript_push_float(script, fa*fb);
				DISPATCH();
			
			exec_mulf64:;
				db = _TaghaScript_pop_double(script);
				da = _TaghaScript_pop_double(script);
				if( debugmode )
					printf("8-byte float mult result: %f == %f * %f\n", da*db, da,db);
				_TaghaScript_push_double(script, da*db);
				DISPATCH();
			
			exec_divq:;
				qb = _TaghaScript_pop_int64(script);
				if( !qb ) {
					printf("divq: divide by 0 error.\n");
					qb = 1;
				}
				qa = _TaghaScript_pop_int64(script);
				if( debugmode )
					printf("signed 8 byte division result: %" PRIi64 " == %" PRIi64 " / %" PRIi64 "\n", (int64_t)qa / (int64_t)qb, qa,qb);
				_TaghaScript_push_int64(script, (int64_t)qa / (int64_t)qb);
				DISPATCH();
			
			exec_udivq:;
				qb = _TaghaScript_pop_int64(script);
				if( !qb ) {
					printf("udivq: divide by 0 error.\n");
					qb = 1;
				}
				qa = _TaghaScript_pop_int64(script);
				if( debugmode )
					printf("unsigned 8 byte division result: %" PRIu64 " == %" PRIu64 " / %" PRIu64 "\n", qa/qb, qa,qb);
				_TaghaScript_push_int64(script, qa/qb);
				DISPATCH();
			
			exec_divl:;
				b = _TaghaScript_pop_int32(script);
				if( !b ) {
					printf("divl: divide by 0 error.\n");
					b=1;
				}
				a = _TaghaScript_pop_int32(script);
				if( debugmode )
					printf("signed 4 byte division result: %" PRIi32 " == %" PRIi32 " / %" PRIi32 "\n", (int32_t)a / (int32_t)b, a,b);
				_TaghaScript_push_int32(script, (int32_t)a / (int32_t)b);
				DISPATCH();
			
			exec_udivl:;
				b = _TaghaScript_pop_int32(script);
				if( !b ) {
					printf("udivl: divide by 0 error.\n");
					b=1;
				}
				a = _TaghaScript_pop_int32(script);
				if( debugmode )
					printf("unsigned 4 byte division result: %" PRIu32 " == %" PRIu32 " / %" PRIu32 "\n", a/b, a,b);
				_TaghaScript_push_int32(script, a/b);
				DISPATCH();
			
			exec_divf:;
				fb = _TaghaScript_pop_float(script);
				if( !fb ) {
					printf("divf: divide by 0.0 error.\n");
					fb = 1.f;
				}
				fa = _TaghaScript_pop_float(script);
				if( debugmode )
					printf("4-byte float division result: %f == %f / %f\n", fa/fb, fa,fb);
				_TaghaScript_push_float(script, fa/fb);
				DISPATCH();
			
			exec_divf64:;
				db = _TaghaScript_pop_double(script);
				if( !db ) {
					printf("divf64: divide by 0.0 error.\n");
					db=1.0;
				}
				da = _TaghaScript_pop_double(script);
				if( debugmode )
					printf("8-byte float division result: %f == %f / %f\n", da/db, da,db);
				_TaghaScript_push_double(script, da/db);
				DISPATCH();
			
			exec_modq:;
				qb = _TaghaScript_pop_int64(script);
				if( !qb ) {
					printf("modq: divide by 0 error.\n");
					qb = 1;
				}
				qa = _TaghaScript_pop_int64(script);
				if( debugmode )
					printf("signed 8 byte modulo result: %" PRIi64 " == %" PRIi64 " %% %" PRIi64 "\n", (int64_t)qa % (int64_t)qb, qa,qb);
				_TaghaScript_push_int64(script, (int64_t)qa % (int64_t)qb);
				DISPATCH();
			
			exec_umodq:;
				qb = _TaghaScript_pop_int64(script);
				if( !qb ) {
					printf("umodq: divide by 0 error.\n");
					qb = 1;
				}
				qa = _TaghaScript_pop_int64(script);
				if( debugmode )
					printf("unsigned 8 byte modulo result: %" PRIu64 " == %" PRIu64 " %% %" PRIu64 "\n", qa % qb, qa,qb);
				_TaghaScript_push_int64(script, qa % qb);
				DISPATCH();
			
			exec_modl:;
				b = _TaghaScript_pop_int32(script);
				if( !b ) {
					printf("modl: divide by 0 error.\n");
					b=1;
				}
				a = _TaghaScript_pop_int32(script);
				if( debugmode )
					printf("signed 4 byte modulo result: %" PRIi32 " == %" PRIi32 " %% %" PRIi32 "\n", (int32_t)a % (int32_t)b, a,b);
				_TaghaScript_push_int32(script, (int32_t)a % (int32_t)b);
				DISPATCH();
			
			exec_umodl:;
				b = _TaghaScript_pop_int32(script);
				if( !b ) {
					printf("umodl: divide by 0 error.\n");
					b=1;
				}
				a = _TaghaScript_pop_int32(script);
				if( debugmode )
					printf("unsigned 4 byte modulo result: %" PRIu32 " == %" PRIu32 " %% %" PRIu32 "\n", a % b, a,b);
				_TaghaScript_push_int32(script, a % b);
				DISPATCH();
			
			exec_andq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				if( safemode )
					printf("8 byte AND result: %" PRIu64 " == %" PRIu64 " & %" PRIu64 "\n", qa & qb, qa,qb);
				_TaghaScript_push_int64(script, qa & qb);
				DISPATCH();
			
			exec_orq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				if( debugmode )
					printf("8 byte OR result: %" PRIu64 " == %" PRIu64 " | %" PRIu64 "\n", qa | qb, qa,qb);
				_TaghaScript_push_int64(script, qa | qb);
				DISPATCH();
			
			exec_xorq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				if( debugmode )
					printf("8 byte XOR result: %" PRIu64 " == %" PRIu64 " ^ %" PRIu64 "\n", qa ^ qb, qa,qb);
				_TaghaScript_push_int64(script, qa ^ qb);
				DISPATCH();
			
			exec_notq:;
				qa = _TaghaScript_pop_int64(script);
				if( debugmode )
					printf("8 byte NOT result: %" PRIu64 "\n", ~qa);
				_TaghaScript_push_int64(script, ~qa);
				DISPATCH();
			
			exec_shlq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				if( debugmode )
					printf("8 byte Shift Left result: %" PRIu64 " == %" PRIu64 " << %" PRIu64 "\n", qa << qb, qa,qb);
				_TaghaScript_push_int64(script, qa << qb);
				DISPATCH();
			
			exec_shrq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				if( debugmode )
					printf("8 byte Shift Right result: %" PRIu64 " == %" PRIu64 " >> %" PRIu64 "\n", qa >> qb, qa,qb);
				_TaghaScript_push_int64(script, qa >> qb);
				DISPATCH();
			
			exec_andl:;
				b = _TaghaScript_pop_int32(script);
				a = _TaghaScript_pop_int32(script);
				if( debugmode )
					printf("4 byte AND result: %" PRIu32 " == %" PRIu32 " & %" PRIu32 "\n", a & b, a,b);
				_TaghaScript_push_int32(script, a & b);
				DISPATCH();
			
			exec_orl:;
				b = _TaghaScript_pop_int32(script);
				a = _TaghaScript_pop_int32(script);
				if( debugmode )
					printf("4 byte OR result: %" PRIu32 " == %" PRIu32 " | %" PRIu32 "\n", a | b, a,b);
				_TaghaScript_push_int32(script, a | b);
				DISPATCH();
			
			exec_xorl:;
				b = _TaghaScript_pop_int32(script);
				a = _TaghaScript_pop_int32(script);
				if( debugmode )
					printf("4 byte XOR result: %" PRIu32 " == %" PRIu32 " ^ %" PRIu32 "\n", a ^ b, a,b);
				_TaghaScript_push_int32(script, a ^ b);
				DISPATCH();
			
			exec_notl:;
				a = _TaghaScript_pop_int32(script);
				if( debugmode )
					printf("4 byte NOT result: %" PRIu32 "\n", ~a);
				_TaghaScript_push_int32(script, ~a);
				DISPATCH();
			
			exec_shll:;
				b = _TaghaScript_pop_int32(script);
				a = _TaghaScript_pop_int32(script);
				if( debugmode )
					printf("4 byte Shift Left result: %" PRIu32 " == %" PRIu32 " << %" PRIu32 "\n", a << b, a,b);
				_TaghaScript_push_int32(script, a << b);
				DISPATCH();
			
			exec_shrl:;
				b = _TaghaScript_pop_int32(script);
				a = _TaghaScript_pop_int32(script);
				if( debugmode )
					printf("4 byte Shift Right result: %" PRIu32 " == %" PRIu32 " >> %" PRIu32 "\n", a >> b, a,b);
				_TaghaScript_push_int32(script, a >> b);
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
				fa = _TaghaScript_pop_float(script);
				++fa;
				if( debugmode )
					printf("4-byte float Increment result: %f\n", fa);
				_TaghaScript_push_float(script, fa);
				DISPATCH();
			
			exec_incf64:;
				da = _TaghaScript_pop_double(script);
				++da;
				if( debugmode )
					printf("8-byte float Increment result: %f\n", da);
				_TaghaScript_push_double(script, da);
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
				fa = _TaghaScript_pop_float(script);
				--fa;
				if( debugmode )
					printf("4-byte float Decrement result: %f\n", fa);
				_TaghaScript_push_float(script, fa);
				DISPATCH();
			
			exec_decf64:;
				da = _TaghaScript_pop_double(script);
				--da;
				if( debugmode )
					printf("8-byte float Decrement result: %f\n", da);
				_TaghaScript_push_double(script, da);
				DISPATCH();
			
			exec_negq:;
				qa = _TaghaScript_pop_int64(script);
				if( debugmode )
					printf("8 byte Negate result: %" PRIu64 "\n", -qa);
				_TaghaScript_push_int64(script, -qa);
				DISPATCH();
			
			exec_negl:;
				a = _TaghaScript_pop_int32(script);
				if( debugmode )
					printf("4 byte Negate result: %" PRIu32 "\n", -a);
				_TaghaScript_push_int32(script, -a);
				DISPATCH();
			
			exec_negf:;
				fa = _TaghaScript_pop_float(script);
				if( debugmode )
					printf("4-byte float Negate result: %f\n", -fa);
				_TaghaScript_push_float(script, -fa);
				DISPATCH();
			
			exec_negf64:;
				da = _TaghaScript_pop_double(script);
				if( debugmode )
					printf("8-byte float Negate result: %f\n", -da);
				_TaghaScript_push_double(script, -da);
				DISPATCH();
			
			exec_ltq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				if( debugmode )
					printf("signed 8 byte Less Than result: %" PRIu32 " == %" PRIi64 " < %" PRIi64 "\n", (int64_t)qa < (int64_t)qb, qa,qb);
				_TaghaScript_push_int32(script, (int64_t)qa < (int64_t)qb);
				DISPATCH();
			
			exec_ultq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				if( debugmode )
					printf("unsigned 8 byte Less Than result: %" PRIu32 " == %" PRIu64 " < %" PRIu64 "\n", qa < qb, qa,qb);
				_TaghaScript_push_int32(script, qa < qb);
				DISPATCH();
			
			exec_ltl:;
				b = _TaghaScript_pop_int32(script);
				a = _TaghaScript_pop_int32(script);
				if( debugmode )
					printf("4 byte Signed Less Than result: %" PRIu32 " == %" PRIi32 " < %" PRIi32 "\n", (int32_t)a < (int32_t)b, a,b);
				_TaghaScript_push_int32(script, (int32_t)a < (int32_t)b);
				DISPATCH();
			
			exec_ultl:;
				b = _TaghaScript_pop_int32(script);
				a = _TaghaScript_pop_int32(script);
				if( debugmode )
					printf("4 byte Unsigned Less Than result: %" PRIu32 " == %" PRIu32 " < %" PRIu32 "\n", a < b, a,b);
				_TaghaScript_push_int32(script, a < b);
				DISPATCH();
			
			exec_ltf:;
				fb = _TaghaScript_pop_float(script);
				fa = _TaghaScript_pop_float(script);
				if( debugmode )
					printf("4 byte Less Than Float result: %" PRIu32 " == %f < %f\n", fa < fb, fa,fb);
				_TaghaScript_push_int32(script, fa < fb);
				DISPATCH();
			
			exec_ltf64:;
				db = _TaghaScript_pop_double(script);
				da = _TaghaScript_pop_double(script);
				if( debugmode )
					printf("8 byte Less Than Float result: %" PRIu32 " == %f < %f\n", da < db, da,db);
				_TaghaScript_push_int32(script, da < db);
				DISPATCH();
			
			exec_gtq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				if( debugmode )
					printf("signed 8 byte Greater Than result: %" PRIu32 " == %" PRIi64 " > %" PRIi64 "\n", (int64_t)qa > (int64_t)qb, qa,qb);
				_TaghaScript_push_int32(script, (int64_t)qa > (int64_t)qb);
				DISPATCH();
			
			exec_ugtq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				if( debugmode )
					printf("unsigned 8 byte Greater Than result: %" PRIu32 " == %" PRIu64 " > %" PRIu64 "\n", qa > qb, qa,qb);
				_TaghaScript_push_int32(script, qa > qb);
				DISPATCH();
			
			exec_gtl:;
				b = _TaghaScript_pop_int32(script);
				a = _TaghaScript_pop_int32(script);
				if( debugmode )
					printf("4 byte Signed Greater Than result: %" PRIu32 " == %" PRIi32 " > %" PRIi32 "\n", (int32_t)a > (int32_t)b, a,b);
				_TaghaScript_push_int32(script, (int32_t)a > (int32_t)b);
				DISPATCH();
			
			exec_ugtl:;
				b = _TaghaScript_pop_int32(script);
				a = _TaghaScript_pop_int32(script);
				if( debugmode )
					printf("4 byte Unigned Greater Than result: %" PRIu32 " == %" PRIu32 " > %" PRIu32 "\n", a > b, a,b);
				_TaghaScript_push_int32(script, a > b);
				DISPATCH();
			
			exec_gtf:;
				fb = _TaghaScript_pop_float(script);
				fa = _TaghaScript_pop_float(script);
				if( debugmode )
					printf("4 byte Greater Than Float result: %" PRIu32 " == %f > %f\n", fa > fb, fa,fb);
				_TaghaScript_push_int32(script, fa > fb);
				DISPATCH();
			
			exec_gtf64:;
				db = _TaghaScript_pop_double(script);
				da = _TaghaScript_pop_double(script);
				if( debugmode )
					printf("8 byte Greater Than Float result: %" PRIu32 " == %f > %f\n", da > db, da,db);
				_TaghaScript_push_int32(script, da > db);
				DISPATCH();
			
			exec_cmpq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				if( debugmode )
					printf("signed 8 byte Compare result: %" PRIu32 " == %" PRIi64 " == %" PRIi64 "\n", (int64_t)qa == (int64_t)qb, qa,qb);
				_TaghaScript_push_int32(script, (int64_t)qa == (int64_t)qb);
				DISPATCH();
			
			exec_ucmpq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				if( debugmode )
					printf("unsigned 8 byte Compare result: %" PRIu32 " == %" PRIu64 " == %" PRIu64 "\n", qa == qb, qa,qb);
				_TaghaScript_push_int32(script, qa == qb);
				DISPATCH();
			
			exec_cmpl:;
				b = _TaghaScript_pop_int32(script);
				a = _TaghaScript_pop_int32(script);
				if( debugmode )
					printf("4 byte Signed Compare result: %" PRIu32 " == %" PRIi32 " == %" PRIi32 "\n", (int32_t)a == (int32_t)b, a,b);
				_TaghaScript_push_int32(script, (int32_t)a == (int32_t)b);
				DISPATCH();
			
			exec_ucmpl:;
				b = _TaghaScript_pop_int32(script);
				a = _TaghaScript_pop_int32(script);
				if( debugmode )
					printf("4 byte Unsigned Compare result: %" PRIu32 " == %" PRIu32 " == %" PRIu32 "\n", a == b, a,b);
				_TaghaScript_push_int32(script, a == b);
				DISPATCH();
			
			exec_compf:;
				fb = _TaghaScript_pop_float(script);
				fa = _TaghaScript_pop_float(script);
				if( debugmode )
					printf("4 byte Compare Float result: %" PRIu32 " == %f == %f\n", fa == fb, fa,fb);
				_TaghaScript_push_int32(script, fa == fb);
				DISPATCH();
			
			exec_cmpf64:;
				db = _TaghaScript_pop_double(script);
				da = _TaghaScript_pop_double(script);
				if( debugmode )
					printf("8 byte Compare Float result: %" PRIu32 " == %f == %f\n", da == db, da,db);
				_TaghaScript_push_int32(script, da == db);
				DISPATCH();
			
			exec_leqq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				if( debugmode )
					printf("8 byte Signed Less Equal result: %" PRIu32 " == %" PRIi64 " <= %" PRIi64 "\n", (int64_t)qa <= (int64_t)qb, qa,qb);
				_TaghaScript_push_int32(script, (int64_t)qa <= (int64_t)qb);
				DISPATCH();
			
			exec_uleqq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				if( debugmode )
					printf("8 byte Unsigned Less Equal result: %" PRIu32 " == %" PRIu64 " <= %" PRIu64 "\n", qa <= qb, qa,qb);
				_TaghaScript_push_int32(script, qa <= qb);
				DISPATCH();
			
			exec_leql:;
				b = _TaghaScript_pop_int32(script);
				a = _TaghaScript_pop_int32(script);
				if( debugmode )
					printf("4 byte Signed Less Equal result: %" PRIu32 " == %" PRIi32 " <= %" PRIi32 "\n", (int32_t)a <= (int32_t)b, a,b);
				_TaghaScript_push_int32(script, (int32_t)a <= (int32_t)b);
				DISPATCH();
			
			exec_uleql:;
				b = _TaghaScript_pop_int32(script);
				a = _TaghaScript_pop_int32(script);
				if( debugmode )
					printf("4 byte Unsigned Less Equal result: %" PRIu32 " == %" PRIu32 " <= %" PRIu32 "\n", a <= b, a,b);
				_TaghaScript_push_int32(script, a <= b);
				DISPATCH();
			
			exec_leqf:;
				fb = _TaghaScript_pop_float(script);
				fa = _TaghaScript_pop_float(script);
				if( debugmode )
					printf("4 byte Less Equal Float result: %" PRIu32 " == %f <= %f\n", fa <= fb, fa, fb);
				_TaghaScript_push_int32(script, fa <= fb);
				DISPATCH();
			
			exec_leqf64:;
				db = _TaghaScript_pop_double(script);
				da = _TaghaScript_pop_double(script);
				if( debugmode )
					printf("8 byte Less Equal Float result: %" PRIu32 " == %f <= %f\n", da <= db, da,db);
				_TaghaScript_push_int32(script, da <= db);
				DISPATCH();
			
			exec_geqq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				if( debugmode )
					printf("8 byte Signed Greater Equal result: %" PRIu32 " == %" PRIi64 " >= %" PRIi64 "\n", (int64_t)qa >= (int64_t)qb, qa,qb);
				_TaghaScript_push_int32(script, (int64_t)qa >= (int64_t)qb);
				DISPATCH();
			
			exec_ugeqq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				if( debugmode )
					printf("8 byte Unsigned Greater Equal result: %" PRIu32 " == %" PRIu64 " >= %" PRIu64 "\n", qa >= qb, qa,qb);
				_TaghaScript_push_int32(script, qa >= qb);
				DISPATCH();
			
			exec_geql:;
				b = _TaghaScript_pop_int32(script);
				a = _TaghaScript_pop_int32(script);
				if( debugmode )
					printf("4 byte Signed Greater Equal result: %" PRIu32 " == %" PRIi32 " >= %" PRIi32 "\n", (int32_t)a >= (int32_t)b, a,b);
				_TaghaScript_push_int32(script, (int32_t)a >= (int32_t)b);
				DISPATCH();
			
			exec_ugeql:;
				b = _TaghaScript_pop_int32(script);
				a = _TaghaScript_pop_int32(script);
				if( debugmode )
					printf("4 byte Unsigned Greater Equal result: %" PRIu32 " == %" PRIu32 " >= %" PRIu32 "\n", a >= b, a,b);
				_TaghaScript_push_int32(script, a >= b);
				DISPATCH();
			
			exec_geqf:;
				fb = _TaghaScript_pop_float(script);
				fa = _TaghaScript_pop_float(script);
				if( debugmode )
					printf("4 byte Greater Equal Float result: %" PRIu32 " == %f >= %f\n", fa >= fb, fa, fb);
				_TaghaScript_push_int32(script, fa >= fb);
				DISPATCH();
			
			exec_geqf64:;
				db = _TaghaScript_pop_double(script);
				da = _TaghaScript_pop_double(script);
				if( debugmode )
					printf("8 byte Greater Equal Float result: %" PRIu32 " == %f >= %f\n", da >= db, da,db);
				_TaghaScript_push_int32(script, da >= db);
				DISPATCH();
			
			exec_neqq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				if( debugmode )
					printf("8 byte Signed Not Equal result: %" PRIu32 " == %" PRIi64 " != %" PRIi64 "\n", (int64_t)qa != (int64_t)qb, qa,qb);
				_TaghaScript_push_int32(script, (int64_t)qa != (int64_t)qb);
				DISPATCH();
			
			exec_uneqq:;
				qb = _TaghaScript_pop_int64(script);
				qa = _TaghaScript_pop_int64(script);
				if( debugmode )
					printf("8 byte Unsigned Not Equal result: %" PRIu32 " == %" PRIi64 " != %" PRIi64 "\n", qa != qb, qa,qb);
				_TaghaScript_push_int32(script, qa != qb);
				DISPATCH();
			
			exec_neql:;
				b = _TaghaScript_pop_int32(script);
				a = _TaghaScript_pop_int32(script);
				if( debugmode )
					printf("4 byte Signed Not Equal result: %" PRIu32 " == %" PRIi32 " != %" PRIi32 "\n", (int32_t)a != (int32_t)b, a,b);
				_TaghaScript_push_int32(script, (int32_t)a != (int32_t)b);
				DISPATCH();
			
			exec_uneql:;
				b = _TaghaScript_pop_int32(script);
				a = _TaghaScript_pop_int32(script);
				if( debugmode )
					printf("4 byte Unsigned Not Equal result: %" PRIu32 " == %" PRIu32 " != %" PRIu32 "\n", a != b, a,b);
				_TaghaScript_push_int32(script, a != b);
				DISPATCH();
			
			exec_neqf:;
				fb = _TaghaScript_pop_float(script);
				fa = _TaghaScript_pop_float(script);
				if( debugmode )
					printf("4 byte Not Equal Float result: %" PRIu32 " == %f != %f\n", fa != fb, fa, fb);
				_TaghaScript_push_int32(script, fa != fb);
				DISPATCH();
			
			exec_neqf64:;
				db = _TaghaScript_pop_double(script);
				da = _TaghaScript_pop_double(script);
				if( debugmode )
					printf("8 byte Not Equal Float result: %" PRIu32 " == %f != %f\n", da != db, da,db);
				_TaghaScript_push_int32(script, da != db);
				DISPATCH();
			
			exec_jmp:;		// addresses are word sized bytes.
				script->ip = _TaghaScript_get_imm8(script);
				script->IP = script->m_pText + script->ip;
				if( debugmode )
					printf("jmping to instruction address: %" PRIWord "\n", script->ip);
				continue;
			
			exec_jzq:;
				qa = _TaghaScript_pop_int64(script);
				qb = _TaghaScript_get_imm8(script);
				script->IP = (!qa) ? (script->m_pText + qb) : script->IP+1;
				script->ip = script->IP - script->m_pText;
				
				if( debugmode )
					printf("jzq'ing to instruction address: %" PRIWord "\n", script->ip);
				continue;
			
			exec_jnzq:;
				qa = _TaghaScript_pop_int64(script);
				qb = _TaghaScript_get_imm8(script);
				script->IP = (qa) ? (script->m_pText + qb) : script->IP+1;
				script->ip = script->IP - script->m_pText;
				
				if( debugmode )
					printf("jnzq'ing to instruction address: %" PRIWord "\n", script->ip);
				continue;
			
			exec_jzl:;	// check if the first 4 bytes on stack are zero, if yes then jump it.
				a = _TaghaScript_pop_int32(script);
				qb = _TaghaScript_get_imm8(script);
				script->IP = (!a) ? (script->m_pText + qb) : script->IP+1;
				script->ip = script->IP - script->m_pText;
				
				if( debugmode )
					printf("jzl'ing to instruction address: %" PRIWord "\n", script->ip);	//opcode2str[script->m_pText[script->ip]]
				continue;
			
			exec_jnzl:;
				a = _TaghaScript_pop_int32(script);
				qb = _TaghaScript_get_imm8(script);
				script->IP = (a) ? (script->m_pText + qb) : script->IP+1;
				script->ip = script->IP - script->m_pText;
				
				if( debugmode )
					printf("jnzl'ing to instruction address: %" PRIWord "\n", script->ip);
				continue;
			
			exec_call:;		// support functions
				qa = _TaghaScript_get_imm8(script);	// get func address
				if( debugmode )
					printf("call :: calling address: %" PRIWord "\n", qa);
				
				_TaghaScript_push_int64(script, (uintptr_t)script->IP+1);	// save return address.
				if( debugmode )
					printf("call :: return addr: %" PRIWord "\n", script->ip+1);
				
				script->ip = qa;
				script->IP = script->m_pText + qa;	// jump to instruction
				
				_TaghaScript_push_int64(script, (uintptr_t)script->BP);	// push ebp;
				if( debugmode )
					printf("call :: pushing bp: %" PRIuPTR "\n", (uintptr_t)script->BP);
				script->bp = script->sp;	// mov ebp, esp;
				script->BP = script->SP;
				
				if( debugmode )
					printf("call :: bp set to sp: %" PRIuPTR "\n", (uintptr_t)script->BP);
				continue;
			
			exec_calls:;	// support local function pointers
				qa = _TaghaScript_pop_int64(script);	// get func address
				if( debugmode )
					printf("calls: calling address: %" PRIWord "\n", qa);
				
				_TaghaScript_push_int64(script, (uintptr_t)script->IP+1);	// save return address.
				
				if( debugmode )
					printf("call return addr: %" PRIWord "\n", script->ip+1);
				
				_TaghaScript_push_int64(script, (uintptr_t)script->BP);	// push ebp
				script->bp = script->sp;	// mov ebp, esp;
				script->BP = script->SP;
				
				script->ip = qa;
				script->IP = script->m_pText + qa;	// jump to instruction
				
				if( debugmode )
					printf("script->bp: %" PRIWord "\n", script->sp);
				continue;
			
			exec_ret:;
				script->sp = script->bp;	// mov esp, ebp
				script->SP = script->BP;
				if( debugmode )
					printf("ret ;: sp set to bp, sp == %" PRIWord "\n", script->sp);
				
				script->BP = (uint8_t *)(uintptr_t)_TaghaScript_pop_int64(script);	// pop ebp
				script->bp = script->BP - script->m_pMemory;
				if( debugmode )
					printf("ret ;: popped to bp, bp == %" PRIWord "\n", script->bp);
				
				script->IP = (uint8_t *)(uintptr_t)_TaghaScript_pop_int64(script);	// pop return address.
				script->ip = script->IP - script->m_pText;
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
				script->SP = script->BP;
				
				if( debugmode )
					printf("retx :: sp set to bp, sp == %" PRIWord "\n", script->sp);
				
				script->BP = (uint8_t *)(uintptr_t)_TaghaScript_pop_int64(script);
				script->bp = script->BP - script->m_pMemory;
				
				if( debugmode )
					printf("retx :: popping bp, bp == %" PRIWord "\n", script->bp);
				
				script->IP = (uint8_t *)(uintptr_t)_TaghaScript_pop_int64(script);	// pop return address.
				script->ip = script->IP - script->m_pText;
				if( debugmode )
					printf("retx :: popping ip, bp == %" PRIWord "\n", script->ip);
				
				_TaghaScript_push_nbytes(script, bytebuffer, a);	// push back return data!
				if( debugmode )
					printf("retx :: pushed back %" PRIu32 " bytes\n", a);
				continue;
			}
			
			exec_pushnataddr:;
				if( safemode and !script->m_pstrNatives ) {
					printf("exec_pushnataddr reported: native table is NULL! Current instruction address: %" PRIWord "\n", script->ip);
					goto *dispatch[halt];
				}
				// match native name to get an index.
				a = _TaghaScript_get_imm4(script);
				if( safemode and a >= script->m_uiNatives  ) {
					printf("exec_pushnataddr reported: native index \'%" PRIu32 "\' is out of bounds! Current instruction address: %" PRIWord "\n", a, script->ip);
					goto *dispatch[halt];
				}
				pfNative = (fnNative_t)(uintptr_t)map_find(vm->m_pmapNatives, script->m_pstrNatives[a]);
				if( safemode and !pfNative ) {
					printf("exec_pushnataddr reported: native \'%s\' not registered! Current instruction address: %" PRIWord "\n", script->m_pstrNatives[a], script->ip);
					goto *dispatch[halt];
				}
				_TaghaScript_push_int64(script, (uintptr_t)pfNative);
				if( debugmode )
					printf("pushnataddr: pushed native func addr: %p\n", pfNative);
				DISPATCH();
			
			exec_callnat:; {	// call a native
				if( safemode and !script->m_pstrNatives ) {
					printf("exec_callnat reported: native table is NULL! Current instruction address: %" PRIWord "\n", script->ip);
					goto *dispatch[halt];
				}
				a = _TaghaScript_get_imm4(script);
				if( safemode and a >= script->m_uiNatives  ) {
					printf("exec_callnat reported: native index \'%" PRIu32 "\' is out of bounds! Current instruction address: %" PRIWord "\n", a, script->ip);
					goto *dispatch[halt];
				}
				
				pfNative = (fnNative_t) (uintptr_t)map_find(vm->m_pmapNatives, script->m_pstrNatives[a]);
				if( safemode and !pfNative ) {
					printf("exec_callnat reported: native \'%s\' not registered! Current instruction address: %" PRIWord "\n", script->m_pstrNatives[a], script->ip);
					goto *dispatch[halt];
				}
				// how many bytes to push to native.
				const uint32_t bytes = _TaghaScript_get_imm4(script);
				// how many arguments pushed as native args
				const uint32_t argcount = _TaghaScript_get_imm4(script);
				if( debugmode ) {
					printf("callnat: Calling func addr: %p ", pfNative);
					printf("with %" PRIu32 " bytes pushed ", bytes);
					printf("and %" PRIu32 " args.\n", argcount);
				}
				/*
				uint64_t *parms = &(uint64_t){0};
				if( bytes ) {
					if( bytes<=8 ) {
						_TaghaScript_pop_nbytes(script, parms, bytes);
					}
					else if( bytes>8 ) {
						uint64_t tempparams[bytes/8+1];
						parms = tempparams;
						_TaghaScript_pop_nbytes(script, parms, bytes);
					}
				}
				*/
				(*pfNative)(script, argcount, bytes/*, parms*/);
				DISPATCH();
			}
			/* support calling natives via function pointers */
			exec_callnats:; {	// call native by func ptr allocated on stack
				pfNative = (fnNative_t)(uintptr_t) _TaghaScript_pop_int64(script);
				if( safemode and !pfNative ) {
					printf("exec_callnats reported: native \'%s\' not registered! Current instruction address: %" PRIWord "\n", script->m_pstrNatives[a], script->ip);
					goto *dispatch[halt];
				}
				const uint32_t bytes = _TaghaScript_get_imm4(script);
				const uint32_t argcount = _TaghaScript_get_imm4(script);
				if( debugmode ) {
					printf("callnats: Calling func addr: %p ", pfNative);
					printf("with %" PRIu32 " bytes pushed ", bytes);
					printf("and %" PRIu32 " args.\n", argcount);
				}
				/*
				uint64_t *parms = &(uint64_t){0};
				if( bytes ) {
					if( bytes<=8 ) {
						_TaghaScript_pop_nbytes(script, parms, bytes);
					}
					else if( bytes>8 ) {
						uint64_t tempparams[bytes/8+1];
						parms = tempparams;
						_TaghaScript_pop_nbytes(script, parms, bytes);
					}
				}
				*/
				(*pfNative)(script, argcount, bytes/*, parms*/);
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
