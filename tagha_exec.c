
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
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

static inline uint64_t _TaghaScript_get_imm8(struct TaghaScript *script)
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

static inline uint32_t _TaghaScript_get_imm4(struct TaghaScript *script)
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

static inline uint16_t _TaghaScript_get_imm2(struct TaghaScript *script)
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

static inline void _TaghaScript_push_int64(struct TaghaScript *script, const uint64_t val)
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

static inline void _TaghaScript_push_double(struct TaghaScript *script, const double val)
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

static inline void _TaghaScript_push_int32(struct TaghaScript *script, const uint32_t val)
{
	if( !script )
		return;
	uint32_t size = sizeof(uint64_t);
	if( script->m_bSafeMode and ((script->m_pSP-script->m_pMemory)-size) >= script->m_uiMemsize ) {
		TaghaScript_PrintErr(script, __func__, "stack overflow!");
		return;
	}
	script->m_pSP -= size;
	*(uint64_t *)script->m_pSP = 0;
	*(uint32_t *)script->m_pSP = val;
}
static inline uint32_t _TaghaScript_pop_int32(struct TaghaScript *script)
{
	if( !script )
		return 0;
	uint32_t size = sizeof(uint64_t);
	if( script->m_bSafeMode and ((script->m_pSP-script->m_pMemory)+size) >= script->m_uiMemsize ) {	// we're subtracting, did we integer underflow?
		TaghaScript_PrintErr(script, __func__, "stack underflow!");
		return 0;
	}
	uint32_t val = *(uint32_t *)script->m_pSP;
	script->m_pSP += size;
	return val;
}

static inline void _TaghaScript_push_float(struct TaghaScript *script, const float val)
{
	if( !script )
		return;
	uint32_t size = sizeof(double);
	if( script->m_bSafeMode and ((script->m_pSP-script->m_pMemory)-size) >= script->m_uiMemsize ) {
		TaghaScript_PrintErr(script, __func__, "stack overflow!");
		return;
	}
	script->m_pSP -= size;
	*(uint64_t *)script->m_pSP = 0;
	*(float *)script->m_pSP = val;
}
static inline float _TaghaScript_pop_float(struct TaghaScript *script)
{
	if( !script )
		return 0;
	uint32_t size = sizeof(double);
	if( script->m_bSafeMode and ((script->m_pSP-script->m_pMemory)+size) >= script->m_uiMemsize ) {
		TaghaScript_PrintErr(script, __func__, "stack underflow!");
		return 0;
	}
	float val = *(float *)script->m_pSP;
	script->m_pSP += size;
	return val;
}

static inline void _TaghaScript_push_short(struct TaghaScript *script, const uint16_t val)
{
	if( !script )
		return;
	uint32_t size = sizeof(uint64_t);
	if( script->m_bSafeMode and ((script->m_pSP-script->m_pMemory)-size) >= script->m_uiMemsize ) {
		TaghaScript_PrintErr(script, __func__, "stack overflow!");
		return;
	}
	script->m_pSP -= size;
	*(uint64_t *)script->m_pSP = 0;
	*(uint16_t *)script->m_pSP = val;
}
static inline uint16_t _TaghaScript_pop_short(struct TaghaScript *script)
{
	if( !script )
		return 0;
	uint32_t size = sizeof(uint64_t);
	if( script->m_bSafeMode and ((script->m_pSP-script->m_pMemory)+size) >= script->m_uiMemsize ) {
		TaghaScript_PrintErr(script, __func__, "stack underflow!");
		return 0;
	}
	uint16_t val = *(uint16_t *)script->m_pSP;
	script->m_pSP += size;
	return val;
}

static inline void _TaghaScript_push_byte(struct TaghaScript *script, const uint8_t val)
{
	if( !script )
		return;
	uint32_t size = sizeof(uint64_t);
	if( script->m_bSafeMode and ((script->m_pSP-script->m_pMemory)-size) >= script->m_uiMemsize ) {
		TaghaScript_PrintErr(script, __func__, "stack overflow!");
		return;
	}
	script->m_pSP -= size;
	*(uint64_t *)script->m_pSP = 0;
	*script->m_pSP = val;
}
static inline uint8_t _TaghaScript_pop_byte(struct TaghaScript *script)
{
	if( !script )
		return 0;
	uint32_t size = sizeof(uint64_t);
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
	
	//uint32_t alignedbytes = (bytesize + 7) & -8;
	if( script->m_bSafeMode and ((script->m_pSP-script->m_pMemory)-bytesize) >= script->m_uiMemsize ) {
		TaghaScript_PrintErr(script, __func__, "stack overflow!");
		return;
	}
	script->m_pSP -= bytesize;
	memcpy(script->m_pSP, pItem, bytesize);
}
static inline void _TaghaScript_pop_nbytes(struct TaghaScript *restrict script, void *restrict pBuffer, const uint32_t bytesize)
{
	if( !script )
		return;
	//uint32_t alignedbytes = (bytesize + 7) & -8;
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

static inline void _TaghaScript_exec_ret(struct TaghaScript *script)
{
	if( !script )
		return;
	bool debugmode = script->m_bDebugMode;
	script->m_pSP = script->m_pBP;	// mov esp, ebp
	if( debugmode )
		printf("ret :: sp set to bp, sp == %p | offset: %" PRIuPTR "\n", script->m_pSP, script->m_pSP-script->m_pMemory);
	
	script->m_pBP = (uint8_t *)(uintptr_t)_TaghaScript_pop_int64(script);	// pop ebp
	if( debugmode )
		printf("ret :: popped bp, bp == %p | offset: %" PRIuPTR "\n", script->m_pBP, script->m_pBP-script->m_pMemory);
	
	script->m_pIP = (uint8_t *)(uintptr_t)_TaghaScript_pop_int64(script);	// pop return address.
	if( debugmode )
		printf("ret :: ret'ing to address: %p | offset: %" PRIuPTR "\n", script->m_pIP, script->m_pIP-script->m_pText);
}




//#include <unistd.h>	// sleep() func
void Tagha_exec(struct TaghaVM *restrict vm, uint8_t *restrict oldbp)
{
	//printf("instruction set size == %" PRIu32 "\n", nop);
	if( !vm or !vm->m_pScript )
		return;
	
	struct TaghaScript *script=NULL;
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
	
	script = vm->m_pScript;
	if( !script or !script->m_pText )
		return;
	else if( !oldbp and !map_find(script->m_pmapFuncs, "main") ) {
		TaghaScript_PrintErr(script, __func__, "Cannot freely execute script missing 'main()'!");
		return;
	}
	
	while( 1 ) {
		script->m_uiMaxInstrs--;
		if( !script->m_uiMaxInstrs )
			break;
		
		safemode = script->m_bSafeMode;
		debugmode = script->m_bDebugMode;
		if( safemode ) {
			if( script->m_pIP < script->m_pText or script->m_pIP - script->m_pText >= script->m_uiInstrSize ) {
				TaghaScript_PrintErr(script, __func__, "instruction address out of bounds!");
				goto *dispatch[halt];
			}
			else if( *script->m_pIP > nop ) {
				TaghaScript_PrintErr(script, __func__, "illegal instruction exception! | instruction == \'%" PRIu32 "\'", *script->m_pIP);
				goto *dispatch[halt];
			}
		}
#ifdef _UNISTD_H
		sleep(1);
#endif
		//printf( "current instruction == \"%s\" @ m_pIP == %" PRIu32 "\n", opcode2str[script->m_pText[script->m_pIP]], script->m_pIP );
		goto *dispatch[ *script->m_pIP ];
		
		exec_nop:;
			if( debugmode )
				puts("nop\n");
			DISPATCH();
		
		exec_halt:;
			if( debugmode ) {
				puts("========================= [script done] =========================\n\n");
				//TaghaScript_debug_print_memory(script);
			}
			break;
		
		exec_pushq:;	// push 8 bytes onto stack.
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
		
		exec_pushsp:;	// push current value of stack onto the stack.
			addr = script->m_pSP;
			_TaghaScript_push_int64(script, (uintptr_t)addr);
			if( debugmode )
				printf("pushsp: pushed sp : %p | offset: %" PRIuPTR "\n", addr, addr-script->m_pMemory);
			DISPATCH();
		
		exec_puship:;	// push current value of instruction pointer
			_TaghaScript_push_int64(script, (uintptr_t)script->m_pIP);
			if( debugmode )
				printf("puship: pushed ip : %p\n", script->m_pIP);
			DISPATCH();
		
		exec_pushbp:;	// push value of base pointer.
			_TaghaScript_push_int64(script, (uintptr_t)script->m_pBP);
			if( debugmode )
				printf("pushbp: pushed bp : %p\n", script->m_pBP);
			DISPATCH();
		
		exec_pushoffset:;	// think of this like LEA (load effective address)
			qa = _TaghaScript_get_imm8(script);
			_TaghaScript_push_int64(script, (uintptr_t)(script->m_pMemory + qa));
			if( debugmode )
				printf("pushoffset: pushed offset index - %" PRIu64 " : %p\n", qa, (script->m_pMemory + qa));
			DISPATCH();
		
		exec_pushspadd:;
			addr = script->m_pSP;
			qb = _TaghaScript_pop_int64(script);
			_TaghaScript_push_int64(script, (uintptr_t)(addr+qb));
			if( debugmode )
				printf("pushspadd: added sp with %" PRIu64 ", result: %p | offset: %" PRIuPTR "\n", qb, addr+qb, (addr+qb)-script->m_pMemory);
			DISPATCH();
		
		exec_pushspsub:;
			addr = script->m_pSP;
			qb = _TaghaScript_pop_int64(script);
			_TaghaScript_push_int64(script, (uintptr_t)(addr-qb));
			if( debugmode )
				printf("pushspsub: subbed sp with %" PRIu64 ", result: %p | offset: %" PRIuPTR "\n", qb, (addr-qb), (addr-qb)-script->m_pMemory);
			DISPATCH();
		
		exec_pushbpadd:;
			addr = script->m_pBP;
			qb = _TaghaScript_pop_int64(script);
			_TaghaScript_push_int64(script, (uintptr_t)(addr+qb));
			if( debugmode )
				printf("pushbpadd: added bp with %" PRIu64 ", result: %p | offset: %" PRIuPTR "\n", qb, (addr+qb), (addr+qb)-script->m_pMemory);
			DISPATCH();
		
		exec_pushbpsub:;
			addr = script->m_pBP;
			qb = _TaghaScript_pop_int64(script);
			_TaghaScript_push_int64(script, (uintptr_t)(addr-qb));
			if( debugmode )
				printf("pushbpsub: subbed bp with %" PRIu64 ", result: %p | offset: %" PRIuPTR "\n", qb, (addr-qb), (addr-qb)-script->m_pMemory);
			DISPATCH();
		
		exec_pushipadd:;
			addr = script->m_pIP;
			qb = _TaghaScript_pop_int64(script);
			_TaghaScript_push_int64(script, (uintptr_t)(addr+qb));
			if( debugmode )
				printf("pushipadd: added ip with %" PRIu64 ", result: %p | offset: %" PRIuPTR "\n", qb, (addr+qb), (addr+qb)-script->m_pText);
			DISPATCH();
		
		exec_pushipsub:;
			addr = script->m_pIP;
			qb = _TaghaScript_pop_int64(script);
			_TaghaScript_push_int64(script, (uintptr_t)(addr-qb));
			if( debugmode )
				printf("pushipsub: subbed ip with %" PRIu64 ", result: %p | offset: %" PRIuPTR "\n", qb, (addr-qb), (addr-qb)-script->m_pText);
			DISPATCH();
		
		exec_popq:;	// every value pushed is counted as 8 bytes, all we need is one pop operation.
			_TaghaScript_pop_int64(script);
			if( debugmode )
				printf("popq\n");
			DISPATCH();
		
		exec_popsp:; {
			uint8_t *p = (uint8_t *)(uintptr_t)_TaghaScript_pop_int64(script);
			if( safemode and (p < script->m_pMemory or (p-script->m_pMemory) >= script->m_uiMemsize) ) {
				TaghaScript_PrintErr(script, __func__, "exec_popsp :: Invalid memory access!");
				DISPATCH();
			}
			script->m_pSP = p;
			if( debugmode )
				printf("popsp: sp is now %p.\n", script->m_pSP);
			DISPATCH();
		}
		exec_popbp:; {
			uint8_t *p = (uint8_t *)(uintptr_t)_TaghaScript_pop_int64(script);
			if( safemode and (p < script->m_pMemory or (p-script->m_pMemory) >= script->m_uiMemsize) ) {
				TaghaScript_PrintErr(script, __func__, "exec_popbp :: Invalid memory access!");
				DISPATCH();
			}
			script->m_pBP = p;
			if( debugmode )
				printf("popbp: bp is now %p.\n", script->m_pBP);
			DISPATCH();
		}
		exec_popip:; {
			uint8_t *p = (uint8_t *)(uintptr_t)_TaghaScript_pop_int64(script);
			if( safemode and (p < script->m_pText or (p-script->m_pText) >= script->m_uiInstrSize) ) {
				TaghaScript_PrintErr(script, __func__, "exec_popip :: Invalid memory access!");
				DISPATCH();
			}
			script->m_pIP = p;
			if( debugmode )
				printf("popip: ip is now at address: %p.\n", script->m_pIP);
			continue;
		}
		exec_loadspq:;	// take an 8-byte value from a part of the deep stack and push it to the top.
			addr = (uint8_t *)(uintptr_t)_TaghaScript_pop_int64(script);
			if( safemode and !addr /*(addr < script->m_pMemory or (addr-script->m_pMemory)+7 >= script->m_uiMemsize)*/ ) {
				TaghaScript_PrintErr(script, __func__, "exec_loadspq :: Invalid memory access!");
				_TaghaScript_push_int64(script, 0);
				DISPATCH();
			}
			_TaghaScript_push_int64(script, *(uint64_t *)addr);
			if( debugmode )
				printf("loaded 8-byte data to T.O.S. - %" PRIu64 " | offset [%" PRIu64 "]\n", *(uint64_t *)addr, (uint64_t)(addr-script->m_pMemory));
			DISPATCH();
		
		exec_loadspl:;	// take a 4-byte value from a part of the deep stack and push it to the top.
			addr = (uint8_t *)(uintptr_t)_TaghaScript_pop_int64(script);
			if( safemode and !addr /*(addr < script->m_pMemory or (addr-script->m_pMemory)+7 >= script->m_uiMemsize)*/ ) {
				TaghaScript_PrintErr(script, __func__, "exec_loadspl :: Invalid memory access!");
				_TaghaScript_push_int32(script, 0);
				DISPATCH();
			}
			_TaghaScript_push_int32(script, *(uint32_t *)addr);
			if( debugmode )
				printf("loaded 4-byte data to T.O.S. - %" PRIu32 " | offset [%" PRIu64 "]\n", *(uint32_t *)addr, (uint64_t)(addr-script->m_pMemory));
			DISPATCH();
		
		exec_loadsps:;	// take a 2-byte value from a part of the deep stack and push it to the top.
			addr = (uint8_t *)(uintptr_t)_TaghaScript_pop_int64(script);
			if( safemode and !addr /*(addr < script->m_pMemory or (addr-script->m_pMemory)+7 >= script->m_uiMemsize)*/ ) {
				TaghaScript_PrintErr(script, __func__, "exec_loadsps :: Invalid memory access!");
				_TaghaScript_push_short(script, 0);
				DISPATCH();
			}
			_TaghaScript_push_short(script, *(uint16_t *)addr);
			if( debugmode )
				printf("loaded 2-byte data to T.O.S. - %" PRIu32 " | offset [%" PRIu64 "]\n", *(uint16_t *)addr, (uint64_t)(addr-script->m_pMemory));
			DISPATCH();
		
		exec_loadspb:;	// take a single byte from a part of the deep stack and push it to the top.
			addr = (uint8_t *)(uintptr_t)_TaghaScript_pop_int64(script);
			if( safemode and !addr /*(addr < script->m_pMemory or (addr-script->m_pMemory)+7 >= script->m_uiMemsize)*/ ) {
				TaghaScript_PrintErr(script, __func__, "exec_loadspb :: Invalid memory access!");
				_TaghaScript_push_byte(script, 0);
				DISPATCH();
			}
			_TaghaScript_push_byte(script, *addr);
			if( debugmode )
				printf("loaded byte data to T.O.S. - %" PRIu32 " | offset [%" PRIu64 "]\n", *addr, (uint64_t)(addr-script->m_pMemory));
			DISPATCH();
		
		exec_storespq:;	// store 8 bytes of the TOS into another part of the stack.
			addr = (uint8_t *)(uintptr_t)_TaghaScript_pop_int64(script);
			if( safemode and !addr /*(addr < script->m_pMemory or (addr-script->m_pMemory)+7 >= script->m_uiMemsize)*/ ) {
				TaghaScript_PrintErr(script, __func__, "exec_storespq :: Invalid memory access!");
				_TaghaScript_pop_int64(script);
				DISPATCH();
			}
			*(uint64_t *)addr = _TaghaScript_pop_int64(script);
			if( debugmode )
				printf("stored 8-byte data from T.O.S. - %" PRIu64 " | offset [%" PRIu64 "]\n", *(uint64_t *)addr, (uint64_t)(addr-script->m_pMemory));
			DISPATCH();
		
		exec_storespl:;	// store 4 bytes TOS into another part of the data stack.
			addr = (uint8_t *)(uintptr_t)_TaghaScript_pop_int64(script);
			if( safemode and !addr /*(addr < script->m_pMemory or (addr-script->m_pMemory)+7 >= script->m_uiMemsize)*/ ) {
				TaghaScript_PrintErr(script, __func__, "exec_storespl :: Invalid memory access!");
				_TaghaScript_pop_int32(script);
				DISPATCH();
			}
			*(uint32_t *)addr = _TaghaScript_pop_int32(script);
			if( debugmode )
				printf("stored 4-byte data from T.O.S. - %" PRIu32 " | offset [%" PRIu64 "]\n", *(uint32_t *)addr, (uint64_t)(addr-script->m_pMemory));
			DISPATCH();
		
		exec_storesps:;	// store 2 bytes TOS into another part of the data stack.
			addr = (uint8_t *)(uintptr_t)_TaghaScript_pop_int64(script);
			if( safemode and !addr /*(addr < script->m_pMemory or (addr-script->m_pMemory)+7 >= script->m_uiMemsize)*/ ) {
				TaghaScript_PrintErr(script, __func__, "exec_storesps :: Invalid memory access!");
				_TaghaScript_pop_short(script);
				DISPATCH();
			}
			*(uint16_t *)addr = _TaghaScript_pop_short(script);
			if( debugmode )
				printf("stored 2-byte data from T.O.S. - %" PRIu32 " | offset [%" PRIu64 "]\n", *(uint16_t *)addr, (uint64_t)(addr-script->m_pMemory));
			DISPATCH();
		
		exec_storespb:;	// store a byte TOS into another part of the data stack.
			addr = (uint8_t *)(uintptr_t)_TaghaScript_pop_int64(script);
			if( safemode and !addr /*(addr < script->m_pMemory or (addr-script->m_pMemory)+7 >= script->m_uiMemsize)*/ ) {
				TaghaScript_PrintErr(script, __func__, "exec_storespb :: Invalid memory access!");
				_TaghaScript_pop_byte(script);
				DISPATCH();
			}
			*addr = _TaghaScript_pop_byte(script);
			if( debugmode )
				printf("stored byte data from T.O.S. - %" PRIu32 " | offset [%" PRIu64 "]\n", *addr, (uint64_t)(addr-script->m_pMemory));
			DISPATCH();
		
		exec_copyq:;
			if( safemode and (script->m_pSP-script->m_pMemory)+8 >= script->m_uiMemsize ) {
				TaghaScript_PrintErr(script, __func__, "exec_copyq :: stack overflow!");
				DISPATCH();
			}
			if( debugmode )
				printf("copying 8-byte data from T.O.S. - %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			_TaghaScript_push_int64(script, *(uint64_t *)script->m_pSP);
			DISPATCH();
		
		exec_copyl:;	// copy 4 bytes of top of stack and put as new top of stack.
			if( safemode and (script->m_pSP-script->m_pMemory)+4 >= script->m_uiMemsize ) {
				TaghaScript_PrintErr(script, __func__, "exec_copyl :: stack overflow!");
				DISPATCH();
			}
			if( debugmode )
				printf("copying 4-byte data from T.O.S. - %" PRIu32 "\n", *(uint32_t *)script->m_pSP);
			_TaghaScript_push_int32(script, *(uint32_t *)script->m_pSP);
			DISPATCH();
		
		exec_copys:;
			if( safemode and (script->m_pSP-script->m_pMemory)+2 >= script->m_uiMemsize ) {
				TaghaScript_PrintErr(script, __func__, "exec_copys :: stack overflow!");
				DISPATCH();
			}
			if( debugmode )
				printf("copying 2-byte data from T.O.S. - %" PRIu32 "\n", *(uint16_t *)script->m_pSP);
			_TaghaScript_push_short(script, *(uint16_t *)script->m_pSP);
			DISPATCH();
		
		exec_copyb:;
			if( safemode and (script->m_pSP-script->m_pMemory)+1 >= script->m_uiMemsize ) {
				TaghaScript_PrintErr(script, __func__, "exec_copyb :: stack overflow!");
				DISPATCH();
			}
			if( debugmode )
				printf("copying byte of data from T.O.S. - %" PRIu32 "\n", *script->m_pSP);
			_TaghaScript_push_byte(script, *script->m_pSP);
			DISPATCH();
		
		exec_addq:;
			qb = _TaghaScript_pop_int64(script);
			*(int64_t *)script->m_pSP += (int64_t)qb;
			if( debugmode )
				printf("signed 8 byte addition result: %" PRIi64 "\n", *(int64_t *)script->m_pSP);
			//_TaghaScript_push_int64(script, (int64_t)qa + (int64_t)qb);
			DISPATCH();
		
		exec_uaddq:;
			qb = _TaghaScript_pop_int64(script);
			*(uint64_t *)script->m_pSP += qb;
			if( debugmode )
				printf("unsigned 8 byte addition result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int64(script, qa+qb);
			DISPATCH();
		
		exec_addl:;		// pop 8 bytes, signed addition, and push 4 byte result to top of stack
			b = _TaghaScript_pop_int32(script);
			*(int32_t *)script->m_pSP += (int32_t)b;
			if( debugmode )
				printf("signed 4 byte addition result: %" PRIi32 "\n", *(int32_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, (int32_t)a + (int32_t)b);
			DISPATCH();
		
		exec_uaddl:;	// In C, all integers in an expression are promoted to int32, if number is smaller then int32.
			b = _TaghaScript_pop_int32(script);
			*(uint32_t *)script->m_pSP += b;
			if( debugmode )
				printf("unsigned 4 byte addition result: %" PRIu32 "\n", *(uint32_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, a+b);
			DISPATCH();
		
		exec_addf:;
			fb = _TaghaScript_pop_float(script);
			*(float *)script->m_pSP += fb;
			//fa = _TaghaScript_pop_float(script);
			if( debugmode )
				printf("4-byte float addition result: %f\n", *(float *)script->m_pSP);
			//_TaghaScript_push_float(script, fa+fb);
			DISPATCH();
		
		exec_addf64:;
			db = _TaghaScript_pop_double(script);
			*(double *)script->m_pSP += db;
			if( debugmode )
				printf("8-byte float addition result: %f\n", *(double *)script->m_pSP);
			//_TaghaScript_push_double(script, da+db);
			DISPATCH();
		
		exec_subq:;
			qb = _TaghaScript_pop_int64(script);
			*(int64_t *)script->m_pSP -= (int64_t)qb;
			if( debugmode )
				printf("signed 8 byte subtraction result: %" PRIi64 "\n", *(int64_t *)script->m_pSP);
			//_TaghaScript_push_int64(script, (int64_t)qa - (int64_t)qb);
			DISPATCH();
		
		exec_usubq:;
			qb = _TaghaScript_pop_int64(script);
			*(uint64_t *)script->m_pSP -= qb;
			if( debugmode )
				printf("unsigned 8 byte subtraction result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int64(script, qa-qb);
			DISPATCH();
		
		exec_subl:;
			b = _TaghaScript_pop_int32(script);
			*(int32_t *)script->m_pSP -= (int32_t)b;
			if( debugmode )
				printf("signed 4 byte subtraction result: %" PRIi32 "\n", *(int32_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, (int32_t)a - (int32_t)b);
			DISPATCH();
		
		exec_usubl:;
			b = _TaghaScript_pop_int32(script);
			*(uint32_t *)script->m_pSP -= b;
			if( debugmode )
				printf("unsigned 4 byte subtraction result: %" PRIu32 "\n", *(uint32_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, a-b);
			DISPATCH();
		
		exec_subf:;
			fb = _TaghaScript_pop_float(script);
			*(float *)script->m_pSP -= fb;
			if( debugmode )
				printf("4-byte float subtraction result: %f\n", *(float *)script->m_pSP);
			//_TaghaScript_push_float(script, fa-fb);
			DISPATCH();
		
		exec_subf64:;
			db = _TaghaScript_pop_double(script);
			*(double *)script->m_pSP -= db;
			if( debugmode )
				printf("8-byte float subtraction result: %f\n", *(double *)script->m_pSP);
			//_TaghaScript_push_double(script, da-db);
			DISPATCH();
		
		exec_mulq:;
			qb = _TaghaScript_pop_int64(script);
			*(int64_t *)script->m_pSP *= (int64_t)qb;
			if( debugmode )
				printf("signed 8 byte mult result: %" PRIi64 "\n", *(int64_t *)script->m_pSP);
			//_TaghaScript_push_int64(script, (int64_t)qa * (int64_t)qb);
			DISPATCH();
		
		exec_umulq:;
			qb = _TaghaScript_pop_int64(script);
			*(uint64_t *)script->m_pSP *= qb;
			if( debugmode )
				printf("unsigned 8 byte mult result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int64(script, qa*qb);
			DISPATCH();
		
		exec_mull:;
			b = _TaghaScript_pop_int32(script);
			*(int32_t *)script->m_pSP *= (int32_t)b;
			if( debugmode )
				printf("signed 4 byte mult result: %" PRIi32 "\n", *(int32_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, (int32_t)a * (int32_t)b);
			DISPATCH();
		
		exec_umull:;
			b = _TaghaScript_pop_int32(script);
			*(uint32_t *)script->m_pSP *= b;
			if( debugmode )
				printf("unsigned 4 byte mult result: %" PRIu32 "\n", *(uint32_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, a*b);
			DISPATCH();
		
		exec_mulf:;
			fb = _TaghaScript_pop_float(script);
			*(float *)script->m_pSP *= fb;
			if( debugmode )
				printf("4-byte float mult result: %f\n", *(float *)script->m_pSP);
			//_TaghaScript_push_float(script, fa*fb);
			DISPATCH();
		
		exec_mulf64:;
			db = _TaghaScript_pop_double(script);
			*(double *)script->m_pSP *= db;
			if( debugmode )
				printf("8-byte float mult result: %f\n", *(double *)script->m_pSP);
			//_TaghaScript_push_double(script, da*db);
			DISPATCH();
		
		exec_divq:;
			qb = _TaghaScript_pop_int64(script);
			if( !qb ) {
				TaghaScript_PrintErr(script, __func__, "divq :: divide by 0 error!");
				qb = 1;
			}
			*(int64_t *)script->m_pSP /= (int64_t)qb;
			if( debugmode )
				printf("signed 8 byte division result: %" PRIi64 "\n", *(int64_t *)script->m_pSP);
			//_TaghaScript_push_int64(script, (int64_t)qa / (int64_t)qb);
			DISPATCH();
		
		exec_udivq:;
			qb = _TaghaScript_pop_int64(script);
			if( !qb ) {
				TaghaScript_PrintErr(script, __func__, "udivq :: divide by 0 error!");
				qb = 1;
			}
			*(uint64_t *)script->m_pSP /= qb;
			if( debugmode )
				printf("unsigned 8 byte division result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int64(script, qa/qb);
			DISPATCH();
		
		exec_divl:;
			b = _TaghaScript_pop_int32(script);
			if( !b ) {
				TaghaScript_PrintErr(script, __func__, "divl :: divide by 0 error!");
				b=1;
			}
			*(int32_t *)script->m_pSP /= (int32_t)b;
			if( debugmode )
				printf("signed 4 byte division result: %" PRIi32 "\n", *(int32_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, (int32_t)a / (int32_t)b);
			DISPATCH();
		
		exec_udivl:;
			b = _TaghaScript_pop_int32(script);
			if( !b ) {
				TaghaScript_PrintErr(script, __func__, "udivl :: divide by 0 error!");
				b=1;
			}
			*(uint32_t *)script->m_pSP /= b;
			if( debugmode )
				printf("unsigned 4 byte division result: %" PRIu32 "\n", *(uint32_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, a/b);
			DISPATCH();
		
		exec_divf:;
			fb = _TaghaScript_pop_float(script);
			if( !fb ) {
				TaghaScript_PrintErr(script, __func__, "divf :: divide by 0 error!");
				fb = 1.f;
			}
			*(float *)script->m_pSP /= fb;
			if( debugmode )
				printf("4-byte float division result: %f\n", *(float *)script->m_pSP);
			//_TaghaScript_push_float(script, fa/fb);
			DISPATCH();
		
		exec_divf64:;
			db = _TaghaScript_pop_double(script);
			if( !db ) {
				TaghaScript_PrintErr(script, __func__, "divf64 :: divide by 0 error!");
				db=1.0;
			}
			*(double *)script->m_pSP /= db;
			if( debugmode )
				printf("8-byte float division result: %f\n", *(double *)script->m_pSP);
			//_TaghaScript_push_double(script, da/db);
			DISPATCH();
		
		exec_modq:;
			qb = _TaghaScript_pop_int64(script);
			if( !qb ) {
				TaghaScript_PrintErr(script, __func__, "modq :: divide by 0 error!");
				qb = 1;
			}
			*(int64_t *)script->m_pSP %= (int64_t)qb;
			if( debugmode )
				printf("signed 8 byte modulo result: %" PRIi64 "\n", *(int64_t *)script->m_pSP);
			//_TaghaScript_push_int64(script, (int64_t)qa % (int64_t)qb);
			DISPATCH();
		
		exec_umodq:;
			qb = _TaghaScript_pop_int64(script);
			if( !qb ) {
				TaghaScript_PrintErr(script, __func__, "umodq :: divide by 0 error!");
				qb = 1;
			}
			*(uint64_t *)script->m_pSP %= qb;
			if( debugmode )
				printf("unsigned 8 byte modulo result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int64(script, qa % qb);
			DISPATCH();
		
		exec_modl:;
			b = _TaghaScript_pop_int32(script);
			if( !b ) {
				TaghaScript_PrintErr(script, __func__, "modl :: divide by 0 error!");
				b=1;
			}
			*(int32_t *)script->m_pSP %= (int32_t)b;
			if( debugmode )
				printf("signed 4 byte modulo result: %" PRIi32 "\n", *(int32_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, (int32_t)a % (int32_t)b);
			DISPATCH();
		
		exec_umodl:;
			b = _TaghaScript_pop_int32(script);
			if( !b ) {
				TaghaScript_PrintErr(script, __func__, "umodl :: divide by 0 error!");
				b=1;
			}
			*(uint32_t *)script->m_pSP %= b;
			if( debugmode )
				printf("unsigned 4 byte modulo result: %" PRIu32 "\n", *(uint32_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, a % b);
			DISPATCH();
		
		exec_andq:;
			qb = _TaghaScript_pop_int64(script);
			*(uint64_t *)script->m_pSP &= qb;
			if( safemode )
				printf("8 byte AND result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int64(script, qa & qb);
			DISPATCH();
		
		exec_orq:;
			qb = _TaghaScript_pop_int64(script);
			*(uint64_t *)script->m_pSP |= qb;
			if( debugmode )
				printf("8 byte OR result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int64(script, qa | qb);
			DISPATCH();
		
		exec_xorq:;
			qb = _TaghaScript_pop_int64(script);
			*(uint64_t *)script->m_pSP ^= qb;
			if( debugmode )
				printf("8 byte XOR result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int64(script, qa ^ qb);
			DISPATCH();
		
		exec_notq:;
			*(uint64_t *)script->m_pSP = ~*(uint64_t *)script->m_pSP;
			if( debugmode )
				printf("8 byte NOT result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			DISPATCH();
		
		exec_shlq:;
			qb = _TaghaScript_pop_int64(script);
			*(uint64_t *)script->m_pSP <<= qb;
			if( debugmode )
				printf("8 byte Shift Left result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int64(script, qa << qb);
			DISPATCH();
		
		exec_shrq:;
			qb = _TaghaScript_pop_int64(script);
			*(uint64_t *)script->m_pSP >>= qb;
			if( debugmode )
				printf("8 byte Shift Right result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int64(script, qa >> qb);
			DISPATCH();
		
		exec_andl:;
			b = _TaghaScript_pop_int32(script);
			*(uint32_t *)script->m_pSP &= b;
			if( debugmode )
				printf("4 byte AND result: %" PRIu32 "\n", *(uint32_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, a & b);
			DISPATCH();
		
		exec_orl:;
			b = _TaghaScript_pop_int32(script);
			*(uint32_t *)script->m_pSP |= b;
			if( debugmode )
				printf("4 byte OR result: %" PRIu32 "\n", *(uint32_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, a | b);
			DISPATCH();
		
		exec_xorl:;
			b = _TaghaScript_pop_int32(script);
			*(uint32_t *)script->m_pSP ^= b;
			if( debugmode )
				printf("4 byte XOR result: %" PRIu32 "\n", *(uint32_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, a ^ b);
			DISPATCH();
		
		exec_notl:;
			*(uint32_t *)script->m_pSP = ~*(uint32_t *)script->m_pSP;
			if( debugmode )
				printf("4 byte NOT result: %" PRIu32 "\n", *(uint32_t *)script->m_pSP);
			DISPATCH();
		
		exec_shll:;
			b = _TaghaScript_pop_int32(script);
			*(uint32_t *)script->m_pSP <<= b;
			if( debugmode )
				printf("4 byte Shift Left result: %" PRIu32 "\n", *(uint32_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, a << b);
			DISPATCH();
		
		exec_shrl:;
			b = _TaghaScript_pop_int32(script);
			*(uint32_t *)script->m_pSP >>= b;
			if( debugmode )
				printf("4 byte Shift Right result: %" PRIu32 "\n", *(uint32_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, a >> b);
			DISPATCH();
		
		exec_incq:;
			*(uint64_t *)script->m_pSP += 1;
			if( debugmode )
				printf("8 byte Increment result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int64(script, qa);
			DISPATCH();
		
		exec_incl:;
			*(uint32_t *)script->m_pSP += 1;
			if( debugmode )
				printf("4 byte Increment result: %" PRIu32 "\n", *(uint32_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, a);
			DISPATCH();
		
		exec_incf:;
			*(float *)script->m_pSP += 1.f;
			if( debugmode )
				printf("4-byte float Increment result: %f\n", *(float *)script->m_pSP);
			//_TaghaScript_push_float(script, fa);
			DISPATCH();
		
		exec_incf64:;
			*(double *)script->m_pSP += 1.0;
			if( debugmode )
				printf("8-byte float Increment result: %f\n", *(double *)script->m_pSP);
			//_TaghaScript_push_double(script, da);
			DISPATCH();
		
		exec_decq:;
			*(uint64_t *)script->m_pSP -= 1;
			if( debugmode )
				printf("8 byte Decrement result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int64(script, qa);
			DISPATCH();
		
		exec_decl:;
			*(uint32_t *)script->m_pSP -= 1;
			if( debugmode )
				printf("4 byte Decrement result: %" PRIu32 "\n", *(uint32_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, a);
			DISPATCH();
		
		exec_decf:;
			*(float *)script->m_pSP -= 1.f;
			if( debugmode )
				printf("4-byte float Decrement result: %f\n", *(float *)script->m_pSP);
			//_TaghaScript_push_float(script, fa);
			DISPATCH();
		
		exec_decf64:;
			*(double *)script->m_pSP -= 1.0;
			if( debugmode )
				printf("8-byte float Decrement result: %f\n", *(double *)script->m_pSP);
			//_TaghaScript_push_double(script, da);
			DISPATCH();
		
		exec_negq:;
			*(uint64_t *)script->m_pSP = -*(uint64_t *)script->m_pSP;
			if( debugmode )
				printf("8 byte Negate result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			DISPATCH();
		
		exec_negl:;
			*(uint32_t *)script->m_pSP = -*(uint32_t *)script->m_pSP;
			if( debugmode )
				printf("4 byte Negate result: %" PRIi32 "\n", *(int32_t *)script->m_pSP);
			DISPATCH();
		
		exec_negf:;
			*(float *)script->m_pSP = -*(float *)script->m_pSP;
			if( debugmode )
				printf("4-byte float Negate result: %f\n", *(float *)script->m_pSP);
			DISPATCH();
		
		exec_negf64:;
			*(double *)script->m_pSP = -*(double *)script->m_pSP;
			if( debugmode )
				printf("8-byte float Negate result: %f\n", *(double *)script->m_pSP);
			DISPATCH();
		
		exec_ltq:;
			qb = _TaghaScript_pop_int64(script);
			*(uint64_t *)script->m_pSP = *(int64_t *)script->m_pSP < (int64_t)qb;
			if( debugmode )
				printf("signed 8 byte Less Than result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, (int64_t)qa < (int64_t)qb);
			DISPATCH();
		
		exec_ultq:;
			qb = _TaghaScript_pop_int64(script);
			*(uint64_t *)script->m_pSP = *(uint64_t *)script->m_pSP < qb;
			if( debugmode )
				printf("unsigned 8 byte Less Than result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, qa < qb);
			DISPATCH();
		
		exec_ltl:;
			b = _TaghaScript_pop_int32(script);
			*(uint64_t *)script->m_pSP = *(int32_t *)script->m_pSP < (int32_t)b;
			if( debugmode )
				printf("4 byte Signed Less Than result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, (int32_t)a < (int32_t)b);
			DISPATCH();
		
		exec_ultl:;
			b = _TaghaScript_pop_int32(script);
			*(uint64_t *)script->m_pSP = *(uint32_t *)script->m_pSP < b;
			if( debugmode )
				printf("4 byte Unsigned Less Than result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, a < b);
			DISPATCH();
		
		exec_ltf:;
			fb = _TaghaScript_pop_float(script);
			*(uint64_t *)script->m_pSP = *(float *)script->m_pSP < fb;
			if( debugmode )
				printf("4 byte Less Than Float result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, fa < fb);
			DISPATCH();
		
		exec_ltf64:;
			db = _TaghaScript_pop_double(script);
			*(uint64_t *)script->m_pSP = *(double *)script->m_pSP < db;
			if( debugmode )
				printf("8 byte Less Than Float result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, da < db);
			DISPATCH();
		
		exec_gtq:;
			qb = _TaghaScript_pop_int64(script);
			*(uint64_t *)script->m_pSP = *(int64_t *)script->m_pSP > (int64_t)qb;
			if( debugmode )
				printf("signed 8 byte Greater Than result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, (int64_t)qa > (int64_t)qb);
			DISPATCH();
		
		exec_ugtq:;
			qb = _TaghaScript_pop_int64(script);
			*(uint64_t *)script->m_pSP = *(uint64_t *)script->m_pSP > qb;
			if( debugmode )
				printf("unsigned 8 byte Greater Than result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, qa > qb);
			DISPATCH();
		
		exec_gtl:;
			b = _TaghaScript_pop_int32(script);
			*(uint64_t *)script->m_pSP = *(int32_t *)script->m_pSP > (int32_t)b;
			if( debugmode )
				printf("4 byte Signed Greater Than result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, (int32_t)a > (int32_t)b);
			DISPATCH();
		
		exec_ugtl:;
			b = _TaghaScript_pop_int32(script);
			*(uint64_t *)script->m_pSP = *(uint32_t *)script->m_pSP > b;
			if( debugmode )
				printf("4 byte Unigned Greater Than result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, a > b);
			DISPATCH();
		
		exec_gtf:;
			fb = _TaghaScript_pop_float(script);
			*(uint64_t *)script->m_pSP = *(float *)script->m_pSP > fb;
			if( debugmode )
				printf("4 byte Greater Than Float result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, fa > fb);
			DISPATCH();
		
		exec_gtf64:;
			db = _TaghaScript_pop_double(script);
			*(uint64_t *)script->m_pSP = *(double *)script->m_pSP > db;
			if( debugmode )
				printf("8 byte Greater Than Float result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, da > db);
			DISPATCH();
		
		exec_cmpq:;
			qb = _TaghaScript_pop_int64(script);
			*(uint64_t *)script->m_pSP = *(int64_t *)script->m_pSP == (int64_t)qb;
			if( debugmode )
				printf("signed 8 byte Compare result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, (int64_t)qa == (int64_t)qb);
			DISPATCH();
		
		exec_ucmpq:;
			qb = _TaghaScript_pop_int64(script);
			*(uint64_t *)script->m_pSP = *(uint64_t *)script->m_pSP == qb;
			if( debugmode )
				printf("unsigned 8 byte Compare result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, qa == qb);
			DISPATCH();
		
		exec_cmpl:;
			b = _TaghaScript_pop_int32(script);
			*(uint64_t *)script->m_pSP = *(int32_t *)script->m_pSP == (int32_t)b;
			if( debugmode )
				printf("4 byte Signed Compare result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, (int32_t)a == (int32_t)b);
			DISPATCH();
		
		exec_ucmpl:;
			b = _TaghaScript_pop_int32(script);
			*(uint64_t *)script->m_pSP = *(uint32_t *)script->m_pSP == b;
			if( debugmode )
				printf("4 byte Unsigned Compare result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, a == b);
			DISPATCH();
		
		exec_compf:;
			fb = _TaghaScript_pop_float(script);
			*(uint64_t *)script->m_pSP = *(float *)script->m_pSP == fb;
			if( debugmode )
				printf("4 byte Compare Float result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, fa == fb);
			DISPATCH();
		
		exec_cmpf64:;
			db = _TaghaScript_pop_double(script);
			*(uint64_t *)script->m_pSP = *(double *)script->m_pSP == db;
			if( debugmode )
				printf("8 byte Compare Float result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, da == db);
			DISPATCH();
		
		exec_leqq:;
			qb = _TaghaScript_pop_int64(script);
			*(uint64_t *)script->m_pSP = *(int64_t *)script->m_pSP <= (int64_t)qb;
			if( debugmode )
				printf("8 byte Signed Less Equal result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, (int64_t)qa <= (int64_t)qb);
			DISPATCH();
		
		exec_uleqq:;
			qb = _TaghaScript_pop_int64(script);
			*(uint64_t *)script->m_pSP = *(uint64_t *)script->m_pSP <= qb;
			if( debugmode )
				printf("8 byte Unsigned Less Equal result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, qa <= qb);
			DISPATCH();
		
		exec_leql:;
			b = _TaghaScript_pop_int32(script);
			*(uint64_t *)script->m_pSP = *(int32_t *)script->m_pSP <= (int32_t)b;
			if( debugmode )
				printf("4 byte Signed Less Equal result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, (int32_t)a <= (int32_t)b);
			DISPATCH();
		
		exec_uleql:;
			b = _TaghaScript_pop_int32(script);
			*(uint64_t *)script->m_pSP = *(uint32_t *)script->m_pSP <= b;
			if( debugmode )
				printf("4 byte Unsigned Less Equal result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, a <= b);
			DISPATCH();
		
		exec_leqf:;
			fb = _TaghaScript_pop_float(script);
			*(uint64_t *)script->m_pSP = *(float *)script->m_pSP <= fb;
			if( debugmode )
				printf("4 byte Less Equal Float result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, fa <= fb);
			DISPATCH();
		
		exec_leqf64:;
			db = _TaghaScript_pop_double(script);
			*(uint64_t *)script->m_pSP = *(double *)script->m_pSP <= db;
			if( debugmode )
				printf("8 byte Less Equal Float result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, da <= db);
			DISPATCH();
		
		exec_geqq:;
			qb = _TaghaScript_pop_int64(script);
			*(uint64_t *)script->m_pSP = *(int64_t *)script->m_pSP >= (int64_t)qb;
			if( debugmode )
				printf("8 byte Signed Greater Equal result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, (int64_t)qa >= (int64_t)qb);
			DISPATCH();
		
		exec_ugeqq:;
			qb = _TaghaScript_pop_int64(script);
			*(uint64_t *)script->m_pSP = *(uint64_t *)script->m_pSP >= qb;
			if( debugmode )
				printf("8 byte Unsigned Greater Equal result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, qa >= qb);
			DISPATCH();
		
		exec_geql:;
			b = _TaghaScript_pop_int32(script);
			*(uint64_t *)script->m_pSP = *(int32_t *)script->m_pSP >= (int32_t)b;
			if( debugmode )
				printf("4 byte Signed Greater Equal result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, (int32_t)a >= (int32_t)b);
			DISPATCH();
		
		exec_ugeql:;
			b = _TaghaScript_pop_int32(script);
			*(uint64_t *)script->m_pSP = *(uint32_t *)script->m_pSP >= b;
			if( debugmode )
				printf("4 byte Unsigned Greater Equal result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, a >= b);
			DISPATCH();
		
		exec_geqf:;
			fb = _TaghaScript_pop_float(script);
			*(uint64_t *)script->m_pSP = *(float *)script->m_pSP >= fb;
			if( debugmode )
				printf("4 byte Greater Equal Float result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, fa >= fb);
			DISPATCH();
		
		exec_geqf64:;
			db = _TaghaScript_pop_double(script);
			*(uint64_t *)script->m_pSP = *(double *)script->m_pSP >= db;
			if( debugmode )
				printf("8 byte Greater Equal Float result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, da >= db);
			DISPATCH();
		
		exec_neqq:;
			qb = _TaghaScript_pop_int64(script);
			*(uint64_t *)script->m_pSP = *(int64_t *)script->m_pSP != (int64_t)qb;
			if( debugmode )
				printf("8 byte Signed Not Equal result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, (int64_t)qa != (int64_t)qb);
			DISPATCH();
		
		exec_uneqq:;
			qb = _TaghaScript_pop_int64(script);
			*(uint64_t *)script->m_pSP = *(uint64_t *)script->m_pSP != qb;
			if( debugmode )
				printf("8 byte Unsigned Not Equal result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, qa != qb);
			DISPATCH();
		
		exec_neql:;
			b = _TaghaScript_pop_int32(script);
			*(uint64_t *)script->m_pSP = *(int32_t *)script->m_pSP != (int32_t)b;
			if( debugmode )
				printf("4 byte Signed Not Equal result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, (int32_t)a != (int32_t)b);
			DISPATCH();
		
		exec_uneql:;
			b = _TaghaScript_pop_int32(script);
			*(uint64_t *)script->m_pSP = *(uint32_t *)script->m_pSP != b;
			if( debugmode )
				printf("4 byte Unsigned Not Equal result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, a != b);
			DISPATCH();
		
		exec_neqf:;
			fb = _TaghaScript_pop_float(script);
			*(uint64_t *)script->m_pSP = *(float *)script->m_pSP != fb;
			if( debugmode )
				printf("4 byte Not Equal Float result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, fa != fb);
			DISPATCH();
		
		exec_neqf64:;
			db = _TaghaScript_pop_double(script);
			*(uint64_t *)script->m_pSP = *(double *)script->m_pSP != db;
			if( debugmode )
				printf("8 byte Not Equal Float result: %" PRIu64 "\n", *(uint64_t *)script->m_pSP);
			//_TaghaScript_push_int32(script, da != db);
			DISPATCH();
		
		exec_jmp:;		// addresses are word sized bytes.
			script->m_pIP = script->m_pMemory + _TaghaScript_get_imm8(script);
			if( debugmode )
				printf("jmp: instruction address: %p\n", script->m_pIP);
			continue;
		
		exec_jmps:;
			script->m_pIP = (uint8_t *)(uintptr_t)_TaghaScript_pop_int64(script);
			if( debugmode )
				printf("jmps: instruction address: %p\n", script->m_pIP);
			continue;
		
		exec_jz:;
			qa = _TaghaScript_pop_int64(script);
			qb = _TaghaScript_get_imm8(script);
			script->m_pIP = (!qa) ? script->m_pText+qb : script->m_pIP+1;
			
			if( debugmode )
				printf("jz'ing to instruction address: %p\n", script->m_pIP);
			continue;
		
		exec_jnz:;
			qa = _TaghaScript_pop_int64(script);
			qb = _TaghaScript_get_imm8(script);
			script->m_pIP = (qa) ? script->m_pText+qb : script->m_pIP+1;
			
			if( debugmode )
				printf("jnz'ing to instruction address: %p\n", script->m_pIP);
			continue;
		
		exec_call:;		// support functions
			qa = _TaghaScript_get_imm8(script);	// get func address
			if( debugmode )
				printf("call :: calling address: %" PRIu64 "\n", qa);
			
			_TaghaScript_push_int64(script, (uintptr_t)script->m_pIP+1);	// save return address.
			if( debugmode )
				printf("call :: return addr: %p\n", script->m_pIP+1);
			script->m_pIP = script->m_pText + qa;
			
			_TaghaScript_push_int64(script, (uintptr_t)script->m_pBP);	// push ebp;
			if( debugmode )
				printf("call :: pushing bp: %p\n", script->m_pBP);
			script->m_pBP = script->m_pSP;	// mov ebp, esp;
			
			if( debugmode )
				printf("call :: bp set to sp: %p\n", script->m_pBP);
			continue;
		
		exec_calls:;	// support function pointers
			addr = (uint8_t *)(uintptr_t)_TaghaScript_pop_int64(script);	// get func address
			if( debugmode )
				printf("calls: calling address: %p | offset: %" PRIuPTR "\n", addr, addr-script->m_pText);
			
			_TaghaScript_push_int64(script, (uintptr_t)script->m_pIP+1);	// save return address.
			script->m_pIP = addr;
			
			if( debugmode )
				printf("call return addr: %p\n", script->m_pIP+1);
			
			_TaghaScript_push_int64(script, (uintptr_t)script->m_pBP);	// push ebp
			script->m_pBP = script->m_pSP;	// mov ebp, esp;
			
			if( debugmode )
				printf("bp is: %p\n", script->m_pBP);
			continue;
		
		exec_ret:;	// for void functions
			_TaghaScript_exec_ret(script);
			if( oldbp and script->m_pBP == oldbp )
				break;
			continue;
		
		exec_retq:;	// function that returns a long, double, pointer, or struct type that's 8 bytes.
			qa = _TaghaScript_pop_int64(script); // store our needed data to a buffer.
			if( debugmode )
				printf("retq :: popped %" PRIu64 "\n", qa);
			
			// do our usual return code.
			_TaghaScript_exec_ret(script);
			
			_TaghaScript_push_int64(script, qa);	// push back return data!
			if( debugmode )
				printf("retq :: pushed back %" PRIu64 "\n", qa);
			if( oldbp and script->m_pBP == oldbp )
				break;
			continue;
		
		exec_retl:;	// returns int or float value
			a = _TaghaScript_pop_int32(script);		// store our needed data to a buffer.
			if( debugmode )
				printf("retl :: popped %" PRIu32 "\n", a);
			
			// do our usual return code.
			_TaghaScript_exec_ret(script);
			
			_TaghaScript_push_int32(script, a);		// push back return data!
			if( debugmode )
				printf("retl :: pushed back %" PRIu32 "\n", a);
			if( oldbp and script->m_pBP == oldbp )
				break;
			continue;
		
		exec_rets:;
			a = _TaghaScript_pop_short(script); // store our needed data to a buffer.
			if( debugmode )
				printf("rets :: popped %" PRIu32 "\n", a);
			
			// do our usual return code.
			_TaghaScript_exec_ret(script);
			
			_TaghaScript_push_short(script, a);	// push back return data!
			if( debugmode )
				printf("rets :: pushed back %" PRIu32 "\n", a);
			if( oldbp and script->m_pBP == oldbp )
				break;
			continue;
		
		exec_retb:;
			a = _TaghaScript_pop_byte(script); // store our needed data to a buffer.
			if( debugmode )
				printf("retb :: popped %" PRIu32 "\n", a);
			
			// do our usual return code.
			_TaghaScript_exec_ret(script);
			
			_TaghaScript_push_byte(script, a);	// push back return data!
			if( debugmode )
				printf("retb :: pushed back %" PRIu32 "\n", a);
			if( oldbp and script->m_pBP == oldbp )
				break;
			continue;
		
		exec_pushnataddr:;
			if( safemode and !script->m_pstrNatives ) {
				TaghaScript_PrintErr(script, __func__, "exec_pushnataddr :: native table is NULL!");
				script->m_pIP += 4;
				_TaghaScript_push_int64(script, 0);
				DISPATCH();
			}
			// match native name to get an index.
			a = _TaghaScript_get_imm4(script);
			if( safemode and a >= script->m_uiNatives  ) {
				TaghaScript_PrintErr(script, __func__, "exec_pushnataddr :: native index \'%" PRIu32 "\' is out of bounds!", a);
				_TaghaScript_push_int64(script, 0);
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
				script->m_pIP += 8;
				DISPATCH();
			}
			a = _TaghaScript_get_imm4(script);
			if( safemode and a >= script->m_uiNatives  ) {
				TaghaScript_PrintErr(script, __func__, "exec_callnat :: native index \'%" PRIu32 "\' is out of bounds!", a);
				DISPATCH();
			}
			
			pfNative = (fnNative_t)(uintptr_t) map_find(vm->m_pmapNatives, script->m_pstrNatives[a]);
			if( safemode and !pfNative ) {
				TaghaScript_PrintErr(script, __func__, "exec_callnat :: native \'%s\' not registered!", script->m_pstrNatives[a]);
				script->m_pIP += 4;
				DISPATCH();
			}
			// how many arguments pushed as native args
			const uint32_t argcount = _TaghaScript_get_imm4(script);
			if( debugmode )
				printf("callnat: Calling func addr: %p with %" PRIu32 " args pushed.\n", pfNative, argcount);
			
			// ERROR: you can't initialize an array of variable size.
			// have no choice but to use memset.
			Param_t params[argcount];
			memset(params, 0, sizeof(Param_t)*argcount);
			_TaghaScript_pop_nbytes(script, params, sizeof(Param_t)*argcount);
			
			Param_t *result = &(Param_t){ .UInt64=0 };
			(*pfNative)(script, params, &result, argcount, vm);
			if( result )
				_TaghaScript_push_nbytes(script, result, sizeof(Param_t));
			
			DISPATCH();
		}
		/* support calling natives via function pointers */
		exec_callnats:; {	// call native by func ptr allocated on stack
			pfNative = (fnNative_t)(uintptr_t) _TaghaScript_pop_int64(script);
			if( safemode and !pfNative ) {
				TaghaScript_PrintErr(script, __func__, "exec_callnat :: native \'%s\' not registered!", script->m_pstrNatives[a]);
				script->m_pIP += 4;
				DISPATCH();
			}
			const uint32_t argcount = _TaghaScript_get_imm4(script);
			if( debugmode )
				printf("callnats: Calling func addr: %p with %" PRIu32 " args pushed.\n", pfNative, argcount);
			
			Param_t params[argcount];
			memset(params, 0, sizeof(Param_t)*argcount);
			_TaghaScript_pop_nbytes(script, params, sizeof(Param_t)*argcount);
			
			Param_t *result = &(Param_t){ .UInt64=0 };
			(*pfNative)(script, params, &result, argcount, vm);
			if( result )
				_TaghaScript_push_nbytes(script, result, sizeof(Param_t));
			
			DISPATCH();
		}
	} /* while( 1 ) */
	script = NULL;
}
