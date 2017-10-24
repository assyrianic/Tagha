
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
	
	if( script->m_bSafeMode and ((script->m_pIP-script->m_pText)+bytesize) >= script->m_uiInstrSize ) {
		TaghaScript_PrintErr(script, __func__, "instr overflow!");
		return;
	}
	
	script->m_pIP++;
	memcpy(pBuffer, script->m_pIP, bytesize);
	script->m_pIP += bytesize-1;
}


/*
 * Private, inlined implementations of getting values from
 * the instruction stream to help with speeding up code.
 */

static inline uint64_t _TaghaScript_get_imm8(struct TaghaScript *restrict script)
{
	if( !script )
		return 0L;
	if( script->m_bSafeMode and ((script->m_pIP-script->m_pText)+8) >= script->m_uiInstrSize ) {
		TaghaScript_PrintErr(script, __func__, "instr overflow!");
		return 0L;
	}
	script->m_pIP++;
	uint64_t val = *(uint64_t *)script->m_pIP;
	script->m_pIP += 7;
	return val;
}

static inline uint32_t _TaghaScript_get_imm4(struct TaghaScript *restrict script)
{
	if( !script )
		return 0;
	if( script->m_bSafeMode and ((script->m_pIP-script->m_pText)+4) >= script->m_uiInstrSize ) {
		TaghaScript_PrintErr(script, __func__, "instr overflow!");
		return 0;
	}
	script->m_pIP++;
	uint32_t val = *(uint32_t *)script->m_pIP;
	script->m_pIP += 3;
	return val;
}

static inline uint16_t _TaghaScript_get_imm2(struct TaghaScript *restrict script)
{
	if( !script )
		return 0;
	if( script->m_bSafeMode and ((script->m_pIP-script->m_pText)+2) >= script->m_uiInstrSize ) {
		TaghaScript_PrintErr(script, __func__, "instr overflow!");
		return 0;
	}
	script->m_pIP++;
	uint16_t val = *(uint16_t *)script->m_pIP;
	script->m_pIP++;
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
	if( script->m_bSafeMode and ((script->m_pSP-script->m_pMemory)-size) >= script->m_uiMemsize ) {
		TaghaScript_PrintErr(script, __func__, "stack overflow!");
		return;
	}
	script->m_pSP -= size;
	*(uint64_t *)script->m_pSP = val;
}
static inline uint64_t _TaghaScript_pop_int64(struct TaghaScript *script)
{
	if( !script )
		return 0;
	uint32_t size = sizeof(uint64_t);
	if( script->m_bSafeMode and ((script->m_pSP-script->m_pMemory)+size) >= script->m_uiMemsize ) {
		TaghaScript_PrintErr(script, __func__, "stack underflow!");
		return 0;
	}
	uint64_t val = *(uint64_t *)script->m_pSP;
	script->m_pSP += size;
	return val;
}

static inline void _TaghaScript_push_double(struct TaghaScript *restrict script, const double val)
{
	if( !script )
		return;
	uint32_t size = sizeof(double);
	if( script->m_bSafeMode and ((script->m_pSP-script->m_pMemory)-size) >= script->m_uiMemsize ) {
		TaghaScript_PrintErr(script, __func__, "stack overflow!");
		return;
	}
	script->m_pSP -= size;
	*(double *)script->m_pSP = val;
}
static inline double _TaghaScript_pop_double(struct TaghaScript *script)
{
	if( !script )
		return 0;
	uint32_t size = sizeof(double);
	if( script->m_bSafeMode and ((script->m_pSP-script->m_pMemory)+size) >= script->m_uiMemsize ) {
		TaghaScript_PrintErr(script, __func__, "stack underflow!");
		return 0;
	}
	double val = *(double *)script->m_pSP;
	script->m_pSP += size;
	return val;
}

static inline void _TaghaScript_push_int32(struct TaghaScript *restrict script, const uint32_t val)
{
	if( !script )
		return;
	uint32_t size = sizeof(uint32_t);
	if( script->m_bSafeMode and ((script->m_pSP-script->m_pMemory)-size) >= script->m_uiMemsize ) {
		TaghaScript_PrintErr(script, __func__, "stack overflow!");
		return;
	}
	script->m_pSP -= size;
	*(uint32_t *)script->m_pSP = val;
}
static inline uint32_t _TaghaScript_pop_int32(struct TaghaScript *script)
{
	if( !script )
		return 0;
	uint32_t size = sizeof(uint32_t);
	if( script->m_bSafeMode and ((script->m_pSP-script->m_pMemory)+size) >= script->m_uiMemsize ) {	// we're subtracting, did we integer underflow?
		TaghaScript_PrintErr(script, __func__, "stack underflow!");
		return 0;
	}
	uint32_t val = *(uint32_t *)script->m_pSP;
	script->m_pSP += size;
	return val;
}

static inline void _TaghaScript_push_float(struct TaghaScript *restrict script, const float val)
{
	if( !script )
		return;
	uint32_t size = sizeof(float);
	if( script->m_bSafeMode and ((script->m_pSP-script->m_pMemory)-size) >= script->m_uiMemsize ) {
		TaghaScript_PrintErr(script, __func__, "stack overflow!");
		return;
	}
	script->m_pSP -= size;
	*(float *)script->m_pSP = val;
}
static inline float _TaghaScript_pop_float(struct TaghaScript *script)
{
	if( !script )
		return 0;
	uint32_t size = sizeof(float);
	if( script->m_bSafeMode and ((script->m_pSP-script->m_pMemory)+size) >= script->m_uiMemsize ) {
		TaghaScript_PrintErr(script, __func__, "stack underflow!");
		return 0;
	}
	float val = *(float *)script->m_pSP;
	script->m_pSP += size;
	return val;
}

static inline void _TaghaScript_push_short(struct TaghaScript *restrict script, const uint16_t val)
{
	if( !script )
		return;
	uint32_t size = sizeof(uint16_t);
	if( script->m_bSafeMode and ((script->m_pSP-script->m_pMemory)-size) >= script->m_uiMemsize ) {
		TaghaScript_PrintErr(script, __func__, "stack overflow!");
		return;
	}
	script->m_pSP -= size;
	*(uint16_t *)script->m_pSP = val;
}
static inline uint16_t _TaghaScript_pop_short(struct TaghaScript *script)
{
	if( !script )
		return 0;
	uint32_t size = sizeof(uint16_t);
	if( script->m_bSafeMode and ((script->m_pSP-script->m_pMemory)+size) >= script->m_uiMemsize ) {
		TaghaScript_PrintErr(script, __func__, "stack underflow!");
		return 0;
	}
	uint16_t val = *(uint16_t *)script->m_pSP;
	script->m_pSP += size;
	return val;
}

static inline void _TaghaScript_push_byte(struct TaghaScript *restrict script, const uint8_t val)
{
	if( !script )
		return;
	uint32_t size = sizeof(uint8_t);
	if( script->m_bSafeMode and ((script->m_pSP-script->m_pMemory)-size) >= script->m_uiMemsize ) {
		TaghaScript_PrintErr(script, __func__, "stack overflow!");
		return;
	}
	script->m_pSP -= size;
	*script->m_pSP = val;
}
static inline uint8_t _TaghaScript_pop_byte(struct TaghaScript *script)
{
	if( !script )
		return 0;
	uint32_t size = sizeof(uint8_t);
	if( script->m_bSafeMode and ((script->m_pSP-script->m_pMemory)+size) >= script->m_uiMemsize ) {
		TaghaScript_PrintErr(script, __func__, "stack underflow!");
		return 0;
	}
	uint8_t val = *script->m_pSP;
	script->m_pSP += size;
	return val;
}

static inline void _TaghaScript_push_nbytes(struct TaghaScript *restrict script, void *restrict pItem, const uint32_t bytesize)
{
	if( !script )
		return;
	if( script->m_bSafeMode and ((script->m_pSP-script->m_pMemory)-bytesize) >= script->m_uiMemsize ) {
		TaghaScript_PrintErr(script, __func__, "stack overflow!");
		return;
	}
	script->m_pSP -= bytesize; //(bytesize + 3) & -4;
	memcpy(script->m_pSP, pItem, bytesize);
}
static inline void _TaghaScript_pop_nbytes(struct TaghaScript *restrict script, void *restrict pBuffer, const uint32_t bytesize)
{
	if( !script )
		return;
	if( script->m_bSafeMode and ((script->m_pSP-script->m_pMemory)+bytesize) >= script->m_uiMemsize ) {
		TaghaScript_PrintErr(script, __func__, "stack underflow!");
		return;
	}
	memcpy(pBuffer, script->m_pSP, bytesize);
	script->m_pSP += bytesize;
}


static inline uint64_t _TaghaScript_peek_int64(struct TaghaScript *script)
{
	if( !script )
		return 0;
	if( script->m_bSafeMode and ((script->m_pSP-script->m_pMemory)+7) >= script->m_uiMemsize ) {
		TaghaScript_PrintErr(script, __func__, "stack underflow!");
		return 0;
	}
	return *(uint64_t *)script->m_pSP;
}
static inline double _TaghaScript_peek_double(struct TaghaScript *script)
{
	if( !script )
		return 0;
	if( script->m_bSafeMode and ((script->m_pSP-script->m_pMemory)+7) >= script->m_uiMemsize ) {
		TaghaScript_PrintErr(script, __func__, "stack underflow!");
		return 0;
	}
	return *(double *)script->m_pSP;
}
static inline uint32_t _TaghaScript_peek_int32(struct TaghaScript *script)
{
	if( !script )
		return 0;
	if( script->m_bSafeMode and ((script->m_pSP-script->m_pMemory)+3) >= script->m_uiMemsize ) {
		TaghaScript_PrintErr(script, __func__, "stack underflow!");
		return 0;
	}
	return *(uint32_t *)script->m_pSP;
}

static inline float _TaghaScript_peek_float(struct TaghaScript *script)
{
	if( !script )
		return 0;
	if( script->m_bSafeMode and ((script->m_pSP-script->m_pMemory)+3) >= script->m_uiMemsize ) {
		TaghaScript_PrintErr(script, __func__, "stack underflow!");
		return 0;
	}
	return *(float *)script->m_pSP;
}

static inline uint16_t _TaghaScript_peek_short(struct TaghaScript *script)
{
	if( !script )
		return 0;
	if( script->m_bSafeMode and ((script->m_pSP-script->m_pMemory)+1) >= script->m_uiMemsize ) {
		TaghaScript_PrintErr(script, __func__, "stack underflow!");
		return 0;
	}
	return *(uint16_t *)script->m_pSP;
}

static inline uint8_t _TaghaScript_peek_byte(struct TaghaScript *script)
{
	if( !script )
		return 0;
	if( script->m_bSafeMode and (script->m_pSP-script->m_pMemory) >= script->m_uiMemsize ) {
		TaghaScript_PrintErr(script, __func__, "stack underflow!");
		return 0;
	}
	return *script->m_pSP;
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
	const void *dispatch[] = { INSTR_SET };
#undef X
#undef INSTR_SET
	
	
#define DISPATCH()	++script->m_pIP; continue
	
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
				if( script->m_pIP - script->m_pText >= script->m_uiInstrSize ) {
					TaghaScript_PrintErr(script, __func__, "instruction address out of bounds!");
					goto *dispatch[halt];
				}
				else if( *script->m_pIP > nop ) {
					TaghaScript_PrintErr(script, __func__, "illegal instruction exception!  instruction == \'%" PRIu32 "\'", *script->m_pIP);
					goto *dispatch[halt];
				}
			}
#ifdef _UNISTD_H
			sleep(1);
#endif
			//printf( "current instruction == \"%s\" @ m_pIP == %" PRIu32 "\n", opcode2str[script->m_pText[script->m_pIP]], script->m_pIP );
			goto *dispatch[ *script->m_pIP ];
			
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
				_TaghaScript_push_byte(script, *++script->m_pIP);
				if( debugmode )
					printf("pushb: pushed %" PRIu32 "\n", *script->m_pSP);
				DISPATCH();
			
			exec_pushsp:;	// push m_pSP onto the stack, uses 4 bytes since 'm_pSP' is uint32_t32
				qa = (uintptr_t)script->m_pSP;
				_TaghaScript_push_int64(script, qa);
				if( debugmode )
					printf("pushsp: pushed sp : %" PRIu64 "\n", qa);
				DISPATCH();
			
			exec_puship:;
				_TaghaScript_push_int64(script, (uintptr_t)script->m_pIP);
				if( debugmode )
					printf("puship: pushed ip : %p\n", script->m_pIP);
				DISPATCH();
			
			exec_pushbp:;
				_TaghaScript_push_int64(script, (uintptr_t)script->m_pBP);
				if( debugmode )
					printf("pushbp: pushed bp : %p\n", script->m_pBP);
				DISPATCH();
			
			exec_pushoffset:;
				qa = _TaghaScript_get_imm8(script);
				_TaghaScript_push_int64(script, (uintptr_t)(script->m_pMemory + qa));
				if( debugmode )
					printf("pushoffset: pushed offset index - %" PRIu64 " : %p\n", qa, (script->m_pMemory + qa));
				DISPATCH();
			
			exec_pushspadd:;
				qa = (uintptr_t)script->m_pSP;
				qb = _TaghaScript_pop_int64(script);
				_TaghaScript_push_int64(script, qa+qb);
				if( debugmode )
					printf("pushspadd: added sp with %" PRIu64 ", result: %llx\n", qb, qa+qb);
				DISPATCH();
			
			exec_pushspsub:;
				qa = (uintptr_t)script->m_pSP;
				qb = _TaghaScript_pop_int64(script);
				_TaghaScript_push_int64(script, qa-qb);
				if( debugmode )
					printf("pushspsub: subbed sp with %" PRIu64 ", result: %llx\n", qb, qa-qb);
				DISPATCH();
			
			exec_pushbpadd:;
				qa = (uintptr_t)script->m_pBP;
				qb = _TaghaScript_pop_int64(script);
				_TaghaScript_push_int64(script, qa+qb);
				if( debugmode )
					printf("pushbpadd: added bp with %" PRIu64 ", result: %llx\n", qb, qa+qb);
				DISPATCH();
			
			exec_pushbpsub:;
				qa = (uintptr_t)script->m_pBP;
				qb = _TaghaScript_pop_int64(script);
				_TaghaScript_push_int64(script, qa-qb);
				if( debugmode )
					printf("pushbpsub: subbed bp with %" PRIu64 ", result: %llx\n", qb, qa-qb);
				DISPATCH();
			
			exec_pushipadd:;
				qa = (uintptr_t)script->m_pIP;
				qb = _TaghaScript_pop_int64(script);
				_TaghaScript_push_int64(script, qa+qb);
				if( debugmode )
					printf("pushipadd: added ip with %" PRIu64 ", result: %llx\n", qb, qa+qb);
				DISPATCH();
			
			exec_pushipsub:;
				qa = (uintptr_t)script->m_pIP;
				qb = _TaghaScript_pop_int64(script);
				_TaghaScript_push_int64(script, qa-qb);
				if( debugmode )
					printf("pushipsub: subbed ip with %" PRIu64 ", result: %llx\n", qb, qa-qb);
				DISPATCH();
			
			exec_popq:;
				_TaghaScript_pop_int64(script);
				if( debugmode )
					printf("popq\n");
				DISPATCH();
			
			exec_popsp:; {
				uint8_t *p = (uint8_t *)(uintptr_t)_TaghaScript_pop_int64(script);
				script->m_pSP = p;
				if( debugmode )
					printf("popsp: sp is now %p.\n", script->m_pSP);
				DISPATCH();
			}
			exec_popbp:; {
				script->m_pBP = (uint8_t *)(uintptr_t)_TaghaScript_pop_int64(script);
				if( debugmode )
					printf("popbp: bp is now %p.\n", script->m_pBP);
				DISPATCH();
			}
			exec_popip:; {
				script->m_pIP = (uint8_t *)(uintptr_t)_TaghaScript_pop_int64(script);
				if( debugmode )
					printf("popip: ip is now at address: %p.\n", script->m_pIP);
				continue;
			}
			exec_loadspq:;
				addr = (uint8_t *)(uintptr_t)_TaghaScript_pop_int64(script);
				if( safemode and (!addr or addr < script->m_pMemory or (addr-script->m_pMemory)+7 >= script->m_uiMemsize) ) {
					TaghaScript_PrintErr(script, __func__, "exec_loadspq :: Invalid memory access!");
					_TaghaScript_push_int64(script, 0);
					DISPATCH();
				}
				qa = *(uint64_t *)addr;
				_TaghaScript_push_int64(script, qa);
				if( debugmode )
					printf("loaded 8-byte data to T.O.S. - %" PRIu64 " from sp offset [%" PRIu64 "]\n", qa, (uint64_t)(addr-script->m_pMemory));
				DISPATCH();
			
			exec_loadspl:;
				addr = (uint8_t *)(uintptr_t)_TaghaScript_pop_int64(script);
				if( safemode and (addr < script->m_pMemory or (addr-script->m_pMemory)+3 >= script->m_uiMemsize) ) {
					TaghaScript_PrintErr(script, __func__, "exec_loadspl :: Invalid memory access!");
					_TaghaScript_push_int32(script, 0);
					DISPATCH();
				}
				a = *(uint32_t *)addr;
				_TaghaScript_push_int32(script, a);
				if( debugmode )
					printf("loaded 4-byte data to T.O.S. - %" PRIu32 " from sp offset [%" PRIu64 "]\n", a, (uint64_t)(addr-script->m_pMemory));
				DISPATCH();
			
			exec_loadsps:;
				addr = (uint8_t *)(uintptr_t)_TaghaScript_pop_int64(script);
				if( safemode and (addr < script->m_pMemory or (addr-script->m_pMemory)+1 >= script->m_uiMemsize) ) {
					TaghaScript_PrintErr(script, __func__, "exec_loadsps :: Invalid memory access!");
					_TaghaScript_push_short(script, 0);
					DISPATCH();
				}
				sa = *(uint16_t *)addr;
				_TaghaScript_push_short(script, sa);
				if( debugmode )
					printf("loaded 2-byte data to T.O.S. - %" PRIu32 " from sp offset [%" PRIu64 "]\n", sa, (uint64_t)(addr-script->m_pMemory));
				DISPATCH();
			
			exec_loadspb:;
				addr = (uint8_t *)(uintptr_t)_TaghaScript_pop_int64(script);
				if( safemode and (addr < script->m_pMemory or (addr-script->m_pMemory) >= script->m_uiMemsize) ) {
					TaghaScript_PrintErr(script, __func__, "exec_loadspb :: Invalid memory access!");
					_TaghaScript_push_byte(script, 0);
					DISPATCH();
				}
				_TaghaScript_push_byte(script, *addr);
				if( debugmode )
					printf("loaded byte data to T.O.S. - %" PRIu32 " from sp offset [%" PRIu64 "]\n", *addr, (uint64_t)(addr-script->m_pMemory));
				DISPATCH();
			
			exec_storespq:;
				addr = (uint8_t *)(uintptr_t)_TaghaScript_pop_int64(script);
				if( safemode and (addr < script->m_pMemory or (addr-script->m_pMemory)+7 >= script->m_uiMemsize) ) {
					TaghaScript_PrintErr(script, __func__, "exec_storespq :: Invalid memory access!");
					_TaghaScript_pop_int64(script);
					DISPATCH();
				}
				qa = _TaghaScript_pop_int64(script);
				*(uint64_t *)addr = qa;
				if( debugmode )
					printf("stored 8-byte data from T.O.S. - %" PRIu64 " to sp offset [%" PRIu64 "]\n", qa, (uint64_t)(addr-script->m_pMemory));
				DISPATCH();
			
			exec_storespl:;		// store TOS into another part of the data stack.
				addr = (uint8_t *)(uintptr_t)_TaghaScript_pop_int64(script);
				if( safemode and (addr < script->m_pMemory or (addr-script->m_pMemory)+3 >= script->m_uiMemsize) ) {
					TaghaScript_PrintErr(script, __func__, "exec_storespl :: Invalid memory access!");
					_TaghaScript_pop_int32(script);
					DISPATCH();
				}
				a = _TaghaScript_pop_int32(script);
				*(uint32_t *)addr = a;
				if( debugmode )
					printf("stored 4-byte data from T.O.S. - %" PRIu32 " to sp offset [%" PRIu64 "]\n", a, (uint64_t)(addr-script->m_pMemory));
				DISPATCH();
			
			exec_storesps:;
				addr = (uint8_t *)(uintptr_t)_TaghaScript_pop_int64(script);
				if( safemode and (addr < script->m_pMemory or (addr-script->m_pMemory)+1 >= script->m_uiMemsize) ) {
					TaghaScript_PrintErr(script, __func__, "exec_storesps :: Invalid memory access!");
					_TaghaScript_pop_short(script);
					DISPATCH();
				}
				sa = _TaghaScript_pop_short(script);
				*(uint16_t *)addr = sa;
				if( debugmode )
					printf("stored 2-byte data from T.O.S. - %" PRIu32 " to sp offset [%" PRIu64 "]\n", sa, (uint64_t)(addr-script->m_pMemory));
				DISPATCH();
			
			exec_storespb:;
				addr = (uint8_t *)(uintptr_t)_TaghaScript_pop_int64(script);
				if( safemode and (addr < script->m_pMemory or (addr-script->m_pMemory) >= script->m_uiMemsize) ) {
					TaghaScript_PrintErr(script, __func__, "exec_storespb :: Invalid memory access!");
					_TaghaScript_pop_byte(script);
					DISPATCH();
				}
				*addr = _TaghaScript_pop_byte(script);
				if( debugmode )
					printf("stored byte data from T.O.S. - %" PRIu32 " to sp offset [%" PRIu64 "]\n", *addr, (uint64_t)(addr-script->m_pMemory));
				DISPATCH();
			
			exec_copyq:;
				if( safemode and (script->m_pSP-script->m_pMemory)+8 >= script->m_uiMemsize ) {
					TaghaScript_PrintErr(script, __func__, "exec_copyq :: stack overflow!");
					DISPATCH();
				}
				qa = *(uint64_t *)script->m_pSP;
				if( debugmode )
					printf("copied 8-byte data from T.O.S. - %" PRIu64 "\n", qa);
				_TaghaScript_push_int64(script, qa);
				DISPATCH();
			
			exec_copyl:;	// copy 4 bytes of top of stack and put as new top of stack.
				if( safemode and (script->m_pSP-script->m_pMemory)+4 >= script->m_uiMemsize ) {
					TaghaScript_PrintErr(script, __func__, "exec_copyl :: stack overflow!");
					DISPATCH();
				}
				a = *(uint32_t *)script->m_pSP;
				if( debugmode )
					printf("copied 4-byte data from T.O.S. - %" PRIu32 "\n", a);
				_TaghaScript_push_int32(script, a);
				DISPATCH();
			
			exec_copys:;
				if( safemode and (script->m_pSP-script->m_pMemory)+2 >= script->m_uiMemsize ) {
					TaghaScript_PrintErr(script, __func__, "exec_copys :: stack overflow!");
					DISPATCH();
				}
				a = *(uint16_t *)script->m_pSP;
				_TaghaScript_push_short(script, (uint16_t)a);
				if( debugmode )
					printf("copied 2-byte data from T.O.S. - %" PRIu32 "\n", a);
				DISPATCH();
			
			exec_copyb:;
				if( safemode and (script->m_pSP-script->m_pMemory)+1 >= script->m_uiMemsize ) {
					TaghaScript_PrintErr(script, __func__, "exec_copyb :: stack overflow!");
					DISPATCH();
				}
				_TaghaScript_push_byte(script, *script->m_pSP);
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
				qa = _TaghaScript_get_imm8(script);
				script->m_pIP = script->m_pMemory+qa;
				if( debugmode )
					printf("jmping to instruction address: %p\n", script->m_pIP);
				continue;
			
			exec_jzq:;
				qa = _TaghaScript_pop_int64(script);
				qb = _TaghaScript_get_imm8(script);
				script->m_pIP = (!qa) ? script->m_pText+qb : script->m_pIP+1;
				
				if( debugmode )
					printf("jzq'ing to instruction address: %p\n", script->m_pIP);
				continue;
			
			exec_jnzq:;
				qa = _TaghaScript_pop_int64(script);
				qb = _TaghaScript_get_imm8(script);
				script->m_pIP = (qa) ? script->m_pText+qb : script->m_pIP+1;
				
				if( debugmode )
					printf("jnzq'ing to instruction address: %p\n", script->m_pIP);
				continue;
			
			exec_jzl:;	// check if the first 4 bytes on stack are zero, if yes then jump it.
				a = _TaghaScript_pop_int32(script);
				qb = _TaghaScript_get_imm8(script);
				script->m_pIP = (!a) ? script->m_pText+qb : script->m_pIP+1;
				
				if( debugmode )
					printf("jzl'ing to instruction address: %p\n", script->m_pIP);	//opcode2str[script->m_pText[script->m_pIP]]
				continue;
			
			exec_jnzl:;
				a = _TaghaScript_pop_int32(script);
				qb = _TaghaScript_get_imm8(script);
				script->m_pIP = (a) ? script->m_pText+qb : script->m_pIP+1;
				
				if( debugmode )
					printf("jnzl'ing to instruction address: %p\n", script->m_pIP);
				continue;
			
			exec_call:;		// support functions
				qa = _TaghaScript_get_imm8(script);	// get func address
				if( debugmode )
					printf("call :: calling address: %" PRIu64 "\n", qa);
				
				_TaghaScript_push_int64(script, (uintptr_t)script->m_pIP+1);	// save return address.
				if( debugmode )
					printf("call :: return addr: %p\n", script->m_pIP+1);
				script->m_pIP = script->m_pText+qa;
				
				_TaghaScript_push_int64(script, (uintptr_t)script->m_pBP);	// push ebp;
				if( debugmode )
					printf("call :: pushing bp: %p\n", script->m_pBP);
				script->m_pBP = script->m_pSP;	// mov ebp, esp;
				
				if( debugmode )
					printf("call :: bp set to sp: %p\n", script->m_pBP);
				continue;
			
			exec_calls:;	// support local function pointers
				qa = _TaghaScript_pop_int64(script);	// get func address
				if( debugmode )
					printf("calls: calling address: %" PRIu64 "\n", qa);
				
				_TaghaScript_push_int64(script, (uintptr_t)script->m_pIP+1);	// save return address.
				
				if( debugmode )
					printf("call return addr: %p\n", script->m_pIP+1);
				
				_TaghaScript_push_int64(script, (uintptr_t)script->m_pBP);	// push ebp
				script->m_pBP = script->m_pSP;	// mov ebp, esp;
				
				script->m_pIP = script->m_pText+qa;
				
				if( debugmode )
					printf("bp is: %p\n", script->m_pBP);
				continue;
			
			exec_ret:;
				script->m_pSP = script->m_pBP;	// mov esp, ebp
				if( debugmode )
					printf("ret :: sp set to bp, sp == %p\n", script->m_pSP);
				
				script->m_pBP = (uint8_t *)(uintptr_t)_TaghaScript_pop_int64(script);	// pop ebp
				if( debugmode )
					printf("ret :: popped bp, bp == %p\n", script->m_pBP);
				
				script->m_pIP = (uint8_t *)(uintptr_t)_TaghaScript_pop_int64(script);	// pop return address.
				if( debugmode )
					printf("returning to address: %p\n", script->m_pIP);
				continue;
			
			exec_retn:; {	// for functions that return something.
				a = _TaghaScript_get_imm4(script);
				uint8_t bytebuffer[a];
				if( debugmode )
					printf("retn :: popping %" PRIu32 " to return\n", a);
				/* This opcode assumes all the data for return
				 * is on the near top of stack. In theory, you can
				 * use this method to return multiple pieces of data.
				 */
				_TaghaScript_pop_nbytes(script, bytebuffer, a); // store our needed data to a buffer.
				if( debugmode )
					printf("retn :: popped %" PRIu32 " bytes\n", a);
				// do our usual return code.
				script->m_pSP = script->m_pBP;	// mov esp, ebp
				
				if( debugmode )
					printf("retn :: sp set to bp, sp == %p\n", script->m_pSP);
				script->m_pBP = (uint8_t *)(uintptr_t)_TaghaScript_pop_int64(script);
				
				if( debugmode )
					printf("retn :: popping bp, bp == %p\n", script->m_pBP);
				
				script->m_pIP = (uint8_t *)(uintptr_t)_TaghaScript_pop_int64(script);	// pop return address.
				if( debugmode )
					printf("retn :: popping ip, ip == %p\n", script->m_pIP);
				
				_TaghaScript_push_nbytes(script, bytebuffer, a);	// push back return data!
				if( debugmode )
					printf("retn :: pushed back %" PRIu32 " bytes\n", a);
				continue;
			}
			
			exec_pushnataddr:;
				if( safemode and !script->m_pstrNatives ) {
					TaghaScript_PrintErr(script, __func__, "exec_pushnataddr :: native table is NULL!");
					script->m_pIP += 4;
					_TaghaScript_push_int64(script, 0L);
					DISPATCH();
				}
				// match native name to get an index.
				a = _TaghaScript_get_imm4(script);
				if( safemode and a >= script->m_uiNatives  ) {
					TaghaScript_PrintErr(script, __func__, "exec_pushnataddr :: native index \'%" PRIu32 "\' is out of bounds!", a);
					_TaghaScript_push_int64(script, 0L);
					DISPATCH();
				}
				pfNative = (fnNative_t)(uintptr_t)map_find(vm->m_pmapNatives, script->m_pstrNatives[a]);
				if( safemode and !pfNative )
					TaghaScript_PrintErr(script, __func__, "exec_pushnataddr :: native \'%s\' not registered!", script->m_pstrNatives[a]);
				
				_TaghaScript_push_int64(script, (uintptr_t)pfNative);
				if( debugmode )
					printf("pushnataddr: pushed native func addr: %p\n", pfNative);
				DISPATCH();
			
			exec_callnat:; {	// call a native
				if( safemode and !script->m_pstrNatives ) {
					TaghaScript_PrintErr(script, __func__, "exec_callnat :: native table is NULL!");
					script->m_pIP += 12;
					_TaghaScript_push_int64(script, 0L);
					DISPATCH();
				}
				a = _TaghaScript_get_imm4(script);
				if( safemode and a >= script->m_uiNatives  ) {
					TaghaScript_PrintErr(script, __func__, "exec_callnat :: native index \'%" PRIu32 "\' is out of bounds!", a);
					_TaghaScript_push_int64(script, 0L);
					DISPATCH();
				}
				
				pfNative = (fnNative_t) (uintptr_t)map_find(vm->m_pmapNatives, script->m_pstrNatives[a]);
				if( safemode and !pfNative ) {
					TaghaScript_PrintErr(script, __func__, "exec_callnat :: native \'%s\' not registered!", script->m_pstrNatives[a]);
					script->m_pIP += 8;
					DISPATCH();
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
				(*pfNative)(script, argcount, bytes);
				DISPATCH();
			}
			/* support calling natives via function pointers */
			exec_callnats:; {	// call native by func ptr allocated on stack
				pfNative = (fnNative_t)(uintptr_t) _TaghaScript_pop_int64(script);
				if( safemode and !pfNative ) {
					TaghaScript_PrintErr(script, __func__, "exec_callnat :: native \'%s\' not registered!", script->m_pstrNatives[a]);
					script->m_pIP += 8;
					DISPATCH();
				}
				const uint32_t bytes = _TaghaScript_get_imm4(script);
				const uint32_t argcount = _TaghaScript_get_imm4(script);
				if( debugmode ) {
					printf("callnats: Calling func addr: %p ", pfNative);
					printf("with %" PRIu32 " bytes pushed ", bytes);
					printf("and %" PRIu32 " args.\n", argcount);
				}
				(*pfNative)(script, argcount, bytes);
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
