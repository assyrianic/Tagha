#include <stdio.h>
#include <string.h>
#include "tagha.h"

const char *RegIDToStr(const enum RegID id)
{
	switch( id ) {
		case ras: return "ras";
		case rbs: return "rbs";
		case rcs: return "rcs";
		case rds: return "rds";
		case res: return "res";
		case rfs: return "rfs";
		case rgs: return "rgs";
		case rhs: return "rhs";
		case ris: return "ris";
		case rjs: return "rjs";
		case rks: return "rks";
		case rsp: return "rsp";
		case rbp: return "rbp";
		case rip: return "rip";
		default:  return "bad register id";
	}
}

void PrintAddrMode(const enum AddrMode mode)
{
	char str[150] = {0};
		strncat(str, "Addressing Modes: ", 150);
	if( mode & Immediate )
		strncat(str, "Immediate|", 150);
	if( mode & Register )
		strncat(str, "Register|", 150);
	if( mode & RegIndirect )
		strncat(str, "RegIndirect|", 150);
	if( mode & IPRelative )
		strncat(str, "IPRelative|", 150);
	if( mode & Byte )
		strncat(str, "Byte|", 150);
	if( mode & TwoBytes )
		strncat(str, "TwoBytes|", 150);
	if( mode & FourBytes )
		strncat(str, "FourBytes|", 150);
	if( mode & EightBytes )
		strncat(str, "EightBytes|", 150);
	if( !mode )
		strncat(str, "0", 150);
	puts(str);
}


//#include <unistd.h>	// sleep() func
int Tagha_Exec(struct TaghaVM *restrict pVM, int argc, union CValue argv[])
{
	if( !pVM or !pVM->m_pScript )
		return -1;
	
	struct TaghaScript *pScript = pVM->m_pScript;
	if( !pScript or !pScript->m_pmapFuncs ) {
		printf("Tagha_Exec :: script either NULL or no functions.\n");
		return -1;
	}
	else if( !pScript->m_Regs[rip].UCharPtr ) {
		TaghaScript_PrintErr(pScript, __func__, "NULL instruction ptr! if 'main' doesn't exist, call a function by name.");
		return -1;
	}
	
	fnNative_t	pfNative = NULL;
	bool
		safemode = pScript->m_bSafeMode,
		debugmode = pScript->m_bDebugMode
	;
	uint8_t
		instr,
		addrmode	// what addressing mode to use
	;
	union CValue
		a,	// 1st operand temporary
		b	// 2nd operand temporary
	;
	
#define X(x) #x ,
	// for debugging purposes.
	const char *opcode2str[] = { INSTR_SET };
#undef X
	
#define X(x) &&exec_##x ,
	// our instruction dispatch table.
	const void *dispatch[] = { INSTR_SET };
#undef X
#undef INSTR_SET
	
	(--pScript->m_Regs[rsp].SelfPtr)->UInt64 = (uintptr_t)argv;
	(--pScript->m_Regs[rsp].SelfPtr)->Int64 = argc;
	if( debugmode )
		printf("Tagha_Exec :: pushed argc: %i and pushed argv %p\n", argc, argv);
	
	int32_t offset;
	while( 1 ) {
		pScript->m_uiMaxInstrs--;
		if( !pScript->m_uiMaxInstrs )
			break;
		
		safemode = pScript->m_bSafeMode;
		debugmode = pScript->m_bDebugMode;
		if( safemode ) {
			if( pScript->m_Regs[rip].UCharPtr < pScript->m_pMemory ) {
				TaghaScript_PrintErr(pScript, __func__, "instruction address out of lower bounds!");
				goto *dispatch[halt];
			}
			else if( pScript->m_Regs[rip].UCharPtr > pScript->m_pTextSegment ) {
				TaghaScript_PrintErr(pScript, __func__, "instruction address out of upper bounds!");
				goto *dispatch[halt];
			}
			else if( *pScript->m_Regs[rip].UCharPtr > nop ) {
				TaghaScript_PrintErr(pScript, __func__, "illegal instruction exception! | instruction == \'%" PRIu32 "\'", *pScript->m_Regs[rip].UCharPtr);
				goto *dispatch[halt];
			}
		}
		a.UInt64 = b.UInt64 = 0;	// reset our temporaries.
		offset = 0;
		
		// fetch opcode and addressing mode.
		instr = *pScript->m_Regs[rip].UCharPtr++;
		addrmode = *pScript->m_Regs[rip].UCharPtr++;
		
		// this is for debugging.
#ifdef _UNISTD_H
		sleep(1);
#endif
		if( debugmode ) {
			printf("opcode == '%s' | ", opcode2str[instr]);
			PrintAddrMode(addrmode);
		}
		goto *dispatch[instr];
		
		exec_nop:;
			continue;
		
		exec_halt:;
			if( debugmode ) {
				TaghaScript_PrintStack(pScript);
				TaghaScript_PrintData(pScript);
				TaghaScript_PrintInstrs(pScript);
				TaghaScript_PrintRegData(pScript);
			}
			return pScript->m_Regs[ras].Int32;
		
		exec_push:; {
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( pScript->m_Regs[rsp].UCharPtr-8 < pScript->m_pStackSegment ) {
				TaghaScript_PrintErr(pScript, __func__, "Stack Overflow!");
				continue;
			}
			if( addrmode & Immediate )
				(--pScript->m_Regs[rsp].SelfPtr)->UInt64 = a.UInt64;
			else if( addrmode & Register )
				(--pScript->m_Regs[rsp].SelfPtr)->UInt64 = pScript->m_Regs[a.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				(--pScript->m_Regs[rsp].SelfPtr)->UInt64 = *(uint64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset);
			}
			else if( addrmode & IPRelative ) {
				(--pScript->m_Regs[rsp].SelfPtr)->UInt64 = *a.UInt64Ptr;
			}
			continue;
		}
		exec_pop:; {
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( pScript->m_Regs[rsp].UCharPtr+8 > (pScript->m_pMemory + (pScript->m_uiMemsize-1)) ) {
				TaghaScript_PrintErr(pScript, __func__, "Stack Underflow!");
				continue;
			}
			if( addrmode & Register )
				pScript->m_Regs[a.UInt64].UInt64 = (*pScript->m_Regs[rsp].SelfPtr++).UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				*(uint64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) = (*pScript->m_Regs[rsp].SelfPtr++).UInt64;
			}
			else if( addrmode & (IPRelative|Immediate) ) {
				*a.UInt64Ptr = (*pScript->m_Regs[rsp].SelfPtr++).UInt64;
			}
			continue;
		}
		exec_neg:; {
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Register )
				pScript->m_Regs[a.UInt64].UInt64 = -pScript->m_Regs[a.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Byte )
					*(pScript->m_Regs[a.UInt64].CharPtr+offset) = -*(pScript->m_Regs[a.UInt64].CharPtr+offset);
				else if( addrmode & TwoBytes )
					*(int16_t *)(pScript->m_Regs[a.UInt64].CharPtr+offset) = -*(int16_t *)(pScript->m_Regs[a.UInt64].CharPtr+offset);
				else if( addrmode & FourBytes )
					*(int32_t *)(pScript->m_Regs[a.UInt64].CharPtr+offset) = -*(int32_t *)(pScript->m_Regs[a.UInt64].CharPtr+offset);
				else if( addrmode & EightBytes )
					*(int64_t *)(pScript->m_Regs[a.UInt64].CharPtr+offset) = -*(int64_t *)(pScript->m_Regs[a.UInt64].CharPtr+offset);
			}
			else if( addrmode & (IPRelative|Immediate) ) {
				if( addrmode & Byte )
					*a.CharPtr = -*a.CharPtr;
				else if( addrmode & TwoBytes )
					*a.ShortPtr = -*a.ShortPtr;
				else if( addrmode & FourBytes )
					*a.Int32Ptr = -*a.Int32Ptr;
				else if( addrmode & EightBytes )
					*a.Int64Ptr = -*a.Int64Ptr;
			}
			continue;
		}
		
		exec_inc:; {
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Register )
				pScript->m_Regs[a.UInt64].UInt64 += 1;
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Byte )
					*(pScript->m_Regs[a.UInt64].UCharPtr+offset) += 1;
				else if( addrmode & TwoBytes )
					*(uint16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) += 1;
				else if( addrmode & FourBytes )
					*(uint32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) += 1;
				else if( addrmode & EightBytes )
					*(uint64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) += 1;
			}
			else if( addrmode & (IPRelative|Immediate) ) {
				if( addrmode & Byte )
					*a.UCharPtr += 1;
				else if( addrmode & TwoBytes )
					*a.UShortPtr += 1;
				else if( addrmode & FourBytes )
					*a.UInt32Ptr += 1;
				else if( addrmode & EightBytes )
					*a.UInt64Ptr += 1;
			}
			continue;
		}
		exec_dec:; {
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Register )
				pScript->m_Regs[a.UInt64].UInt64 -= 1;
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Byte )
					*(pScript->m_Regs[a.UInt64].UCharPtr+offset) -= 1;
				else if( addrmode & TwoBytes )
					*(uint16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) -= 1;
				else if( addrmode & FourBytes )
					*(uint32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) -= 1;
				else if( addrmode & EightBytes )
					*(uint64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) -= 1;
			}
			else if( addrmode & (IPRelative|Immediate) ) {
				if( addrmode & Byte )
					*a.UCharPtr -= 1;
				else if( addrmode & TwoBytes )
					*a.UShortPtr -= 1;
				else if( addrmode & FourBytes )
					*a.UInt32Ptr -= 1;
				else if( addrmode & EightBytes )
					*a.UInt64Ptr -= 1;
			}
			continue;
		}
		exec_bnot:; {
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Register )
				pScript->m_Regs[a.UInt64].UInt64 = ~pScript->m_Regs[a.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Byte )
					*(pScript->m_Regs[a.UInt64].UCharPtr+offset) = ~*(pScript->m_Regs[a.UInt64].UCharPtr+offset);
				else if( addrmode & TwoBytes )
					*(uint16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) = ~*(uint16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset);
				else if( addrmode & FourBytes )
					*(uint32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) = ~*(uint32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset);
				else if( addrmode & EightBytes )
					*(uint64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) = ~*(uint64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset);
			}
			else if( addrmode & (IPRelative|Immediate) ) {
				if( addrmode & Byte )
					*a.UCharPtr = ~*a.UCharPtr;
				else if( addrmode & TwoBytes )
					*a.UShortPtr = ~*a.UShortPtr;
				else if( addrmode & FourBytes )
					*a.UInt32Ptr = ~*a.UInt32Ptr;
				else if( addrmode & EightBytes )
					*a.UInt64Ptr = ~*a.UInt64Ptr;
			}
			continue;
		}
		
		exec_jmp:; {
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pScript->m_Regs[rip].UCharPtr = pScript->m_pMemory + a.UInt64;
			else if( addrmode & Register )
				pScript->m_Regs[rip].UCharPtr = pScript->m_pMemory + pScript->m_Regs[a.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				pScript->m_Regs[rip].UCharPtr = pScript->m_pMemory + *(uint64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset);
			}
			else if( addrmode & IPRelative ) {
				pScript->m_Regs[rip].UCharPtr = pScript->m_pMemory + *a.UInt64Ptr;
			}
			continue;
		}
		
		exec_jz:; {
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pScript->m_Regs[rip].UCharPtr = (pScript->m_bZeroFlag) ? pScript->m_pMemory + a.UInt64 : pScript->m_Regs[rip].UCharPtr;
			else if( addrmode & Register )
				pScript->m_Regs[rip].UCharPtr = (pScript->m_bZeroFlag) ? pScript->m_pMemory + pScript->m_Regs[a.UInt64].UInt64 : pScript->m_Regs[rip].UCharPtr;
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				pScript->m_Regs[rip].UCharPtr = (pScript->m_bZeroFlag) ? pScript->m_pMemory + *(uint64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) : pScript->m_Regs[rip].UCharPtr;
			}
			else if( addrmode & IPRelative ) {
				pScript->m_Regs[rip].UCharPtr = (pScript->m_bZeroFlag) ? pScript->m_pMemory + *a.UInt64Ptr : pScript->m_Regs[rip].UCharPtr;
			}
			continue;
		}
		exec_jnz:; {
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pScript->m_Regs[rip].UCharPtr = (!pScript->m_bZeroFlag) ? pScript->m_pMemory + a.UInt64 : pScript->m_Regs[rip].UCharPtr;
			else if( addrmode & Register )
				pScript->m_Regs[rip].UCharPtr = (!pScript->m_bZeroFlag) ? pScript->m_pMemory + pScript->m_Regs[a.UInt64].UInt64 : pScript->m_Regs[rip].UCharPtr;
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				pScript->m_Regs[rip].UCharPtr = (!pScript->m_bZeroFlag) ? pScript->m_pMemory + *(uint64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) : pScript->m_Regs[rip].UCharPtr;
			}
			else if( addrmode & IPRelative ) {
				pScript->m_Regs[rip].UCharPtr = (!pScript->m_bZeroFlag) ? pScript->m_pMemory + *a.UInt64Ptr : pScript->m_Regs[rip].UCharPtr;
			}
			continue;
		}
		
		exec_call:; {
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( pScript->m_Regs[rsp].UCharPtr-16 < pScript->m_pStackSegment ) {
				TaghaScript_PrintErr(pScript, __func__, "Stack Overflow!");
				continue;
			}
			(--pScript->m_Regs[rsp].SelfPtr)->UInt64 = pScript->m_Regs[rip].UInt64;	// push rip
			(--pScript->m_Regs[rsp].SelfPtr)->UInt64 = pScript->m_Regs[rbp].UInt64;	// push rbp
			pScript->m_Regs[rbp] = pScript->m_Regs[rsp];	// mov rbp, rsp
			if( addrmode & Immediate )
				pScript->m_Regs[rip].UCharPtr = pScript->m_pMemory + a.UInt64;
			else if( addrmode & Register )
				pScript->m_Regs[rip].UCharPtr = pScript->m_pMemory + pScript->m_Regs[a.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				pScript->m_Regs[rip].UCharPtr = pScript->m_pMemory + *(uint64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset);
			}
			else if( addrmode & IPRelative ) {
				pScript->m_Regs[rip].UCharPtr = pScript->m_pMemory + *a.UInt64Ptr;
			}
			continue;
		}
		exec_ret:; {
			// if we hit a return and stack is empty, break entirely.
			if( pScript->m_Regs[rbp].UCharPtr == (pScript->m_pMemory + (pScript->m_uiMemsize-1)) ) {
				if( debugmode )
					puts("ret :: return on empty stack");
				goto *dispatch[halt];
			}
			pScript->m_Regs[rsp] = pScript->m_Regs[rbp];	// mov rsp, rbp
			if( pScript->m_Regs[rsp].UCharPtr+16 > (pScript->m_pMemory + (pScript->m_uiMemsize-1)) ) {
				TaghaScript_PrintErr(pScript, __func__, "Stack Underflow!");
				continue;
			}
			pScript->m_Regs[rbp].UInt64 = (*pScript->m_Regs[rsp].SelfPtr++).UInt64;	// pop rbp
			pScript->m_Regs[rip].UInt64 = (*pScript->m_Regs[rsp].SelfPtr++).UInt64;	// pop rip
			if( addrmode & Immediate )
				pScript->m_Regs[rsp].UCharPtr += *pScript->m_Regs[rip].UInt64Ptr++;
			
			continue;
		}
		
		exec_callnat:; {
			if( safemode and !pScript->m_pstrNativeCalls ) {
				TaghaScript_PrintErr(pScript, __func__, "exec_callnat :: native table is NULL!");
				pScript->m_Regs[rip].UInt64Ptr++;
				pScript->m_Regs[rip].UInt32Ptr++;
				continue;
			}
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			char *nativestr;
			uint64_t index;
			
			if( addrmode & Immediate )
				index = a.UInt64;
			else if( addrmode & Register )
				index = pScript->m_Regs[a.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				index = *(uint64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset);
			}
			else if( addrmode & IPRelative ) {
				index = *a.UInt64Ptr;
			}
			
			if( safemode and index >= pScript->m_uiNatives  ) {
				TaghaScript_PrintErr(pScript, __func__, "exec_callnat :: native index \'%" PRIu64 "\' is out of bounds!", index);
				pScript->m_Regs[rip].UInt32Ptr++;
				continue;
			}
			nativestr = pScript->m_pstrNativeCalls[index];
			
			pfNative = (fnNative_t)(uintptr_t) map_find(pVM->m_pmapNatives, nativestr);
			if( safemode and !pfNative ) {
				TaghaScript_PrintErr(pScript, __func__, "exec_callnat :: native \'%s\' not registered!", nativestr);
				pScript->m_Regs[rip].UInt32Ptr++;
				continue;
			}
			// how many arguments pushed for native to use.
			const uint32_t argcount = *pScript->m_Regs[rip].UInt32Ptr++;
			if( debugmode )
				printf("callnat: Calling func addr: %p with %" PRIu32 " args pushed.\n", pfNative, argcount);
			
			// ERROR: you can't initialize an array using a variable as size.
			// have no choice but to use memset.
			CValue params[argcount];
			memset(params, 0, sizeof(CValue)*argcount);
			memcpy(params, pScript->m_Regs[rsp].SelfPtr, sizeof(CValue)*argcount);
			pScript->m_Regs[rsp].SelfPtr += argcount;
			printf("exec_callnat :: calling C function '%s'.\n", nativestr);
			pScript->m_Regs[ras].UInt64 = 0;
			(*pfNative)(pScript, params, pScript->m_Regs+ras, argcount, pVM);
			continue;
		}
		
		exec_movr:; {	// dest is a reg, src is reg or memory
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pScript->m_Regs[a.UInt64].UInt64 = b.UInt64;
			else if( addrmode & Register )
				pScript->m_Regs[a.UInt64].UInt64 = pScript->m_Regs[b.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Byte )
					pScript->m_Regs[a.UInt64].UChar = *(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & TwoBytes )
					pScript->m_Regs[a.UInt64].UShort = *(uint16_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & FourBytes )
					pScript->m_Regs[a.UInt64].UInt32 = *(uint32_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & EightBytes )
					pScript->m_Regs[a.UInt64].UInt64 = *(uint64_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset);
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Byte )
					pScript->m_Regs[a.UInt64].UChar = *b.UCharPtr;
				else if( addrmode & TwoBytes )
					pScript->m_Regs[a.UInt64].UShort = *b.UShortPtr;
				else if( addrmode & FourBytes )
					pScript->m_Regs[a.UInt64].UInt32 = *b.UInt32Ptr;
				else if( addrmode & EightBytes )
					pScript->m_Regs[a.UInt64].UInt64 = *b.UInt64Ptr;
			}
			continue;
		}
		exec_movm:; {	// dest is memory and src is reg/imm/mem
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {	// moving value to register-based address
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Immediate ) { // value is immediate constant
					if( addrmode & Byte )
						*(pScript->m_Regs[a.UInt64].UCharPtr+offset) = b.UChar;
					else if( addrmode & TwoBytes )
						*(uint16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) = b.UShort;
					else if( addrmode & FourBytes )
						*(uint32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) = b.UInt32;
					else if( addrmode & EightBytes )
						*(uint64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) = b.UInt64;
				}
				else if( addrmode & Register ) { // value is in register
					if( addrmode & Byte )
						*(pScript->m_Regs[a.UInt64].UCharPtr+offset) = pScript->m_Regs[b.UInt64].UChar;
					else if( addrmode & TwoBytes )
						*(uint16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) = pScript->m_Regs[b.UInt64].UShort;
					else if( addrmode & FourBytes )
						*(uint32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) = pScript->m_Regs[b.UInt64].UInt32;
					else if( addrmode & EightBytes )
						*(uint64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) = pScript->m_Regs[b.UInt64].UInt64;
				}
			}
			else if( addrmode & IPRelative ) {	// moving value to IPRelative address
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						*a.UCharPtr = b.UChar;
					else if( addrmode & TwoBytes )
						*a.UShortPtr = b.UShort;
					else if( addrmode & FourBytes )
						*a.UInt32Ptr = b.UInt32;
					else if( addrmode & EightBytes )
						*a.UInt64Ptr = b.UInt64;
				}
				else if( addrmode & Register ) { // moving register to IPRelative address
					if( addrmode & Byte )
						*a.UCharPtr = pScript->m_Regs[b.UInt64].UChar;
					else if( addrmode & TwoBytes )
						*a.UShortPtr = pScript->m_Regs[b.UInt64].UShort;
					else if( addrmode & FourBytes )
						*a.UInt32Ptr = pScript->m_Regs[b.UInt64].UInt32;
					else if( addrmode & EightBytes )
						*a.UInt64Ptr = pScript->m_Regs[b.UInt64].UInt64;
				}
			}
			continue;
		}
		exec_lea:; {	// load effective address
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Register )
				pScript->m_Regs[a.UInt64].UInt64 = pScript->m_Regs[b.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				pScript->m_Regs[a.UInt64].UInt64 = pScript->m_Regs[b.UInt64].UInt64+offset;
			}
			else if( addrmode & (IPRelative|Immediate) )
				pScript->m_Regs[a.UInt64].UInt64 = b.UInt64;
			continue;
		}
		/*
		exec_leo:; {	// load effective offset - loads a memory offset.
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & (IPRelative|Immediate) )
				pScript->m_Regs[a.UInt64].UInt64 = (uintptr_t)(pScript->m_pMemory + b.UInt64);
			else if( addrmode & Register )
				pScript->m_Regs[a.UInt64].UInt64 = (uintptr_t)(pScript->m_pMemory + pScript->m_Regs[b.UInt64].UInt64);
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				pScript->m_Regs[a.UInt64].UInt64 = (uintptr_t)(pScript->m_pMemory + pScript->m_Regs[b.UInt64].UInt64 + offset);
			}
			continue;
		}
		*/
		exec_addr:; {	// dest is reg, src is reg/memory/imm
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pScript->m_Regs[a.UInt64].Int64 += b.Int64;
			else if( addrmode & Register )
				pScript->m_Regs[a.UInt64].Int64 += pScript->m_Regs[b.UInt64].Int64;
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Byte )
					pScript->m_Regs[a.UInt64].Char += *(pScript->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & TwoBytes )
					pScript->m_Regs[a.UInt64].Short += *(int16_t *)(pScript->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & FourBytes )
					pScript->m_Regs[a.UInt64].Int32 += *(int32_t *)(pScript->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & EightBytes )
					pScript->m_Regs[a.UInt64].Int64 += *(int64_t *)(pScript->m_Regs[b.UInt64].CharPtr+offset);
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Byte )
					pScript->m_Regs[a.UInt64].Char += *b.CharPtr;
				else if( addrmode & TwoBytes )
					pScript->m_Regs[a.UInt64].Short += *b.ShortPtr;
				else if( addrmode & FourBytes )
					pScript->m_Regs[a.UInt64].Int32 += *b.Int32Ptr;
				else if( addrmode & EightBytes )
					pScript->m_Regs[a.UInt64].Int64 += *b.Int64Ptr;
			}
			continue;
		}
		exec_addm:; {	// dest is memory and src is reg/imm
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						*(pScript->m_Regs[a.UInt64].CharPtr+offset) += b.Char;
					else if( addrmode & TwoBytes )
						*(int16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) += b.Short;
					else if( addrmode & FourBytes )
						*(int32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) += b.Int32;
					else if( addrmode & EightBytes )
						*(int64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) += b.Int64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						*(pScript->m_Regs[a.UInt64].CharPtr+offset) += pScript->m_Regs[b.UInt64].Char;
					else if( addrmode & TwoBytes )
						*(int16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) += pScript->m_Regs[b.UInt64].Short;
					else if( addrmode & FourBytes )
						*(int32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) += pScript->m_Regs[b.UInt64].Int32;
					else if( addrmode & EightBytes )
						*(int64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) += pScript->m_Regs[b.UInt64].Int64;
				}
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						*a.CharPtr += b.Char;
					else if( addrmode & TwoBytes )
						*a.ShortPtr += b.Short;
					else if( addrmode & FourBytes )
						*a.Int32Ptr += b.Int32;
					else if( addrmode & EightBytes )
						*a.Int64Ptr += b.Int64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						*a.CharPtr += pScript->m_Regs[b.UInt64].Char;
					else if( addrmode & TwoBytes )
						*a.ShortPtr += pScript->m_Regs[b.UInt64].Short;
					else if( addrmode & FourBytes )
						*a.Int32Ptr += pScript->m_Regs[b.UInt64].Int32;
					else if( addrmode & EightBytes )
						*a.Int64Ptr += pScript->m_Regs[b.UInt64].Int64;
				}
			}
			continue;
		}
		exec_uaddr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pScript->m_Regs[a.UInt64].UInt64 += b.UInt64;
			else if( addrmode & Register )
				pScript->m_Regs[a.UInt64].UInt64 += pScript->m_Regs[b.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Byte )
					pScript->m_Regs[a.UInt64].UChar += *(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & TwoBytes )
					pScript->m_Regs[a.UInt64].UShort += *(uint16_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & FourBytes )
					pScript->m_Regs[a.UInt64].UInt32 += *(uint32_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & EightBytes )
					pScript->m_Regs[a.UInt64].UInt64 += *(uint64_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset);
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Byte )
					pScript->m_Regs[a.UInt64].UChar += *b.UCharPtr;
				else if( addrmode & TwoBytes )
					pScript->m_Regs[a.UInt64].UShort += *b.UShortPtr;
				else if( addrmode & FourBytes )
					pScript->m_Regs[a.UInt64].UInt32 += *b.UInt32Ptr;
				else if( addrmode & EightBytes )
					pScript->m_Regs[a.UInt64].UInt64 += *b.UInt64Ptr;
			}
			continue;
		}
		exec_uaddm:; {	// dest is memory and src is reg
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						*(pScript->m_Regs[a.UInt64].UCharPtr+offset) += b.UChar;
					else if( addrmode & TwoBytes )
						*(uint16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) += b.UShort;
					else if( addrmode & FourBytes )
						*(uint32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) += b.UInt32;
					else if( addrmode & EightBytes )
						*(uint64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) += b.UInt64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						*(pScript->m_Regs[a.UInt64].UCharPtr+offset) += pScript->m_Regs[b.UInt64].UChar;
					else if( addrmode & TwoBytes )
						*(uint16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) += pScript->m_Regs[b.UInt64].UShort;
					else if( addrmode & FourBytes )
						*(uint32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) += pScript->m_Regs[b.UInt64].UInt32;
					else if( addrmode & EightBytes )
						*(uint64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) += pScript->m_Regs[b.UInt64].UInt64;
				}
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						*a.UCharPtr += b.UChar;
					else if( addrmode & TwoBytes )
						*a.UShortPtr += b.UShort;
					else if( addrmode & FourBytes )
						*a.UInt32Ptr += b.UInt32;
					else if( addrmode & EightBytes )
						*a.UInt64Ptr += b.UInt64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						*a.UCharPtr += pScript->m_Regs[b.UInt64].UChar;
					else if( addrmode & TwoBytes )
						*a.UShortPtr += pScript->m_Regs[b.UInt64].UShort;
					else if( addrmode & FourBytes )
						*a.UInt32Ptr += pScript->m_Regs[b.UInt64].UInt32;
					else if( addrmode & EightBytes )
						*a.UInt64Ptr += pScript->m_Regs[b.UInt64].UInt64;
				}
			}
			continue;
		}
		
		exec_subr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pScript->m_Regs[a.UInt64].Int64 -= b.Int64;
			else if( addrmode & Register )
				pScript->m_Regs[a.UInt64].Int64 -= pScript->m_Regs[b.UInt64].Int64;
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Byte )
					pScript->m_Regs[a.UInt64].Char -= *(pScript->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & TwoBytes )
					pScript->m_Regs[a.UInt64].Short -= *(int16_t *)(pScript->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & FourBytes )
					pScript->m_Regs[a.UInt64].Int32 -= *(int32_t *)(pScript->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & EightBytes )
					pScript->m_Regs[a.UInt64].Int64 -= *(int64_t *)(pScript->m_Regs[b.UInt64].CharPtr+offset);
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Byte )
					pScript->m_Regs[a.UInt64].Char -= *b.CharPtr;
				else if( addrmode & TwoBytes )
					pScript->m_Regs[a.UInt64].Short -= *b.ShortPtr;
				else if( addrmode & FourBytes )
					pScript->m_Regs[a.UInt64].Int32 -= *b.Int32Ptr;
				else if( addrmode & EightBytes )
					pScript->m_Regs[a.UInt64].Int64 -= *b.Int64Ptr;
			}
			continue;
		}
		exec_subm:; {	// dest is memory and src is reg
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						*(pScript->m_Regs[a.UInt64].CharPtr+offset) -= b.Char;
					else if( addrmode & TwoBytes )
						*(int16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) -= b.Short;
					else if( addrmode & FourBytes )
						*(int32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) -= b.Int32;
					else if( addrmode & EightBytes )
						*(int64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) -= b.Int64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						*(pScript->m_Regs[a.UInt64].CharPtr+offset) -= pScript->m_Regs[b.UInt64].Char;
					else if( addrmode & TwoBytes )
						*(int16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) -= pScript->m_Regs[b.UInt64].Short;
					else if( addrmode & FourBytes )
						*(int32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) -= pScript->m_Regs[b.UInt64].Int32;
					else if( addrmode & EightBytes )
						*(int64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) -= pScript->m_Regs[b.UInt64].Int64;
				}
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						*a.CharPtr -= b.Char;
					else if( addrmode & TwoBytes )
						*a.ShortPtr -= b.Short;
					else if( addrmode & FourBytes )
						*a.Int32Ptr -= b.Int32;
					else if( addrmode & EightBytes )
						*a.Int64Ptr -= b.Int64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						*a.CharPtr -= pScript->m_Regs[b.UInt64].Char;
					else if( addrmode & TwoBytes )
						*a.ShortPtr -= pScript->m_Regs[b.UInt64].Short;
					else if( addrmode & FourBytes )
						*a.Int32Ptr -= pScript->m_Regs[b.UInt64].Int32;
					else if( addrmode & EightBytes )
						*a.Int64Ptr -= pScript->m_Regs[b.UInt64].Int64;
				}
			}
			continue;
		}
		exec_usubr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pScript->m_Regs[a.UInt64].UInt64 -= b.UInt64;
			else if( addrmode & Register )
				pScript->m_Regs[a.UInt64].UInt64 -= pScript->m_Regs[b.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Byte )
					pScript->m_Regs[a.UInt64].UChar -= *(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & TwoBytes )
					pScript->m_Regs[a.UInt64].UShort -= *(uint16_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & FourBytes )
					pScript->m_Regs[a.UInt64].UInt32 -= *(uint32_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & EightBytes )
					pScript->m_Regs[a.UInt64].UInt64 -= *(uint64_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset);
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Byte )
					pScript->m_Regs[a.UInt64].UChar -= *b.UCharPtr;
				else if( addrmode & TwoBytes )
					pScript->m_Regs[a.UInt64].UShort -= *b.UShortPtr;
				else if( addrmode & FourBytes )
					pScript->m_Regs[a.UInt64].UInt32 -= *b.UInt32Ptr;
				else if( addrmode & EightBytes )
					pScript->m_Regs[a.UInt64].UInt64 -= *b.UInt64Ptr;
			}
			continue;
		}
		exec_usubm:; {	// dest is memory and src is reg
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						*(pScript->m_Regs[a.UInt64].UCharPtr+offset) -= b.UChar;
					else if( addrmode & TwoBytes )
						*(uint16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) -= b.UShort;
					else if( addrmode & FourBytes )
						*(uint32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) -= b.UInt32;
					else if( addrmode & EightBytes )
						*(uint64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) -= b.UInt64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						*(pScript->m_Regs[a.UInt64].UCharPtr+offset) -= pScript->m_Regs[b.UInt64].UChar;
					else if( addrmode & TwoBytes )
						*(uint16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) -= pScript->m_Regs[b.UInt64].UShort;
					else if( addrmode & FourBytes )
						*(uint32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) -= pScript->m_Regs[b.UInt64].UInt32;
					else if( addrmode & EightBytes )
						*(uint64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) -= pScript->m_Regs[b.UInt64].UInt64;
				}
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						*a.UCharPtr -= b.UChar;
					else if( addrmode & TwoBytes )
						*a.UShortPtr -= b.UShort;
					else if( addrmode & FourBytes )
						*a.UInt32Ptr -= b.UInt32;
					else if( addrmode & EightBytes )
						*a.UInt64Ptr -= b.UInt64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						*a.UCharPtr -= pScript->m_Regs[b.UInt64].UChar;
					else if( addrmode & TwoBytes )
						*a.UShortPtr -= pScript->m_Regs[b.UInt64].UShort;
					else if( addrmode & FourBytes )
						*a.UInt32Ptr -= pScript->m_Regs[b.UInt64].UInt32;
					else if( addrmode & EightBytes )
						*a.UInt64Ptr -= pScript->m_Regs[b.UInt64].UInt64;
				}
			}
			continue;
		}
		
		exec_mulr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pScript->m_Regs[a.UInt64].Int64 *= b.Int64;
			else if( addrmode & Register )
				pScript->m_Regs[a.UInt64].Int64 *= pScript->m_Regs[b.UInt64].Int64;
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Byte )
					pScript->m_Regs[a.UInt64].Char *= *(pScript->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & TwoBytes )
					pScript->m_Regs[a.UInt64].Short *= *(int16_t *)(pScript->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & FourBytes )
					pScript->m_Regs[a.UInt64].Int32 *= *(int32_t *)(pScript->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & EightBytes )
					pScript->m_Regs[a.UInt64].Int64 *= *(int64_t *)(pScript->m_Regs[b.UInt64].CharPtr+offset);
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Byte )
					pScript->m_Regs[a.UInt64].Char *= *b.CharPtr;
				else if( addrmode & TwoBytes )
					pScript->m_Regs[a.UInt64].Short *= *b.ShortPtr;
				else if( addrmode & FourBytes )
					pScript->m_Regs[a.UInt64].Int32 *= *b.Int32Ptr;
				else if( addrmode & EightBytes )
					pScript->m_Regs[a.UInt64].Int64 *= *b.Int64Ptr;
			}
			continue;
		}
		exec_mulm:; {	// dest is memory and src is reg
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						*(pScript->m_Regs[a.UInt64].CharPtr+offset) *= b.Char;
					else if( addrmode & TwoBytes )
						*(int16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) *= b.Short;
					else if( addrmode & FourBytes )
						*(int32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) *= b.Int32;
					else if( addrmode & EightBytes )
						*(int64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) *= b.Int64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						*(pScript->m_Regs[a.UInt64].CharPtr+offset) *= pScript->m_Regs[b.UInt64].Char;
					else if( addrmode & TwoBytes )
						*(int16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) *= pScript->m_Regs[b.UInt64].Short;
					else if( addrmode & FourBytes )
						*(int32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) *= pScript->m_Regs[b.UInt64].Int32;
					else if( addrmode & EightBytes )
						*(int64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) *= pScript->m_Regs[b.UInt64].Int64;
				}
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						*a.CharPtr *= b.Char;
					else if( addrmode & TwoBytes )
						*a.ShortPtr *= b.Short;
					else if( addrmode & FourBytes )
						*a.Int32Ptr *= b.Int32;
					else if( addrmode & EightBytes )
						*a.Int64Ptr *= b.Int64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						*a.CharPtr *= pScript->m_Regs[b.UInt64].Char;
					else if( addrmode & TwoBytes )
						*a.ShortPtr *= pScript->m_Regs[b.UInt64].Short;
					else if( addrmode & FourBytes )
						*a.Int32Ptr *= pScript->m_Regs[b.UInt64].Int32;
					else if( addrmode & EightBytes )
						*a.Int64Ptr *= pScript->m_Regs[b.UInt64].Int64;
				}
			}
			continue;
		}
		exec_umulr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pScript->m_Regs[a.UInt64].UInt64 *= b.UInt64;
			else if( addrmode & Register )
				pScript->m_Regs[a.UInt64].UInt64 *= pScript->m_Regs[b.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Byte )
					pScript->m_Regs[a.UInt64].UChar *= *(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & TwoBytes )
					pScript->m_Regs[a.UInt64].UShort *= *(uint16_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & FourBytes )
					pScript->m_Regs[a.UInt64].UInt32 *= *(uint32_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & EightBytes )
					pScript->m_Regs[a.UInt64].UInt64 *= *(uint64_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset);
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Byte )
					pScript->m_Regs[a.UInt64].UChar *= *b.UCharPtr;
				else if( addrmode & TwoBytes )
					pScript->m_Regs[a.UInt64].UShort *= *b.UShortPtr;
				else if( addrmode & FourBytes )
					pScript->m_Regs[a.UInt64].UInt32 *= *b.UInt32Ptr;
				else if( addrmode & EightBytes )
					pScript->m_Regs[a.UInt64].UInt64 *= *b.UInt64Ptr;
			}
			continue;
		}
		exec_umulm:; {	// dest is memory and src is reg
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						*(pScript->m_Regs[a.UInt64].UCharPtr+offset) *= b.UChar;
					else if( addrmode & TwoBytes )
						*(uint16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) *= b.UShort;
					else if( addrmode & FourBytes )
						*(uint32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) *= b.UInt32;
					else if( addrmode & EightBytes )
						*(uint64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) *= b.UInt64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						*(pScript->m_Regs[a.UInt64].UCharPtr+offset) *= pScript->m_Regs[b.UInt64].UChar;
					else if( addrmode & TwoBytes )
						*(uint16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) *= pScript->m_Regs[b.UInt64].UShort;
					else if( addrmode & FourBytes )
						*(uint32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) *= pScript->m_Regs[b.UInt64].UInt32;
					else if( addrmode & EightBytes )
						*(uint64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) *= pScript->m_Regs[b.UInt64].UInt64;
				}
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						*a.UCharPtr *= b.UChar;
					else if( addrmode & TwoBytes )
						*a.UShortPtr *= b.UShort;
					else if( addrmode & FourBytes )
						*a.UInt32Ptr *= b.UInt32;
					else if( addrmode & EightBytes )
						*a.UInt64Ptr *= b.UInt64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						*a.UCharPtr *= pScript->m_Regs[b.UInt64].UChar;
					else if( addrmode & TwoBytes )
						*a.UShortPtr *= pScript->m_Regs[b.UInt64].UShort;
					else if( addrmode & FourBytes )
						*a.UInt32Ptr *= pScript->m_Regs[b.UInt64].UInt32;
					else if( addrmode & EightBytes )
						*a.UInt64Ptr *= pScript->m_Regs[b.UInt64].UInt64;
				}
			}
			continue;
		}
		
		exec_divr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate ) {
				if( !b.Int64 )
					b.Int64 = 1;
				pScript->m_Regs[a.UInt64].Int64 /= b.Int64;
			}
			else if( addrmode & Register ) {
				if( !pScript->m_Regs[b.UInt64].Int64 )
					pScript->m_Regs[b.UInt64].Int64 = 1;
				pScript->m_Regs[a.UInt64].Int64 /= pScript->m_Regs[b.UInt64].Int64;
			}
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Byte ) {
					if( !*(pScript->m_Regs[b.UInt64].CharPtr+offset) )
						*(pScript->m_Regs[b.UInt64].CharPtr+offset) = 1;
					pScript->m_Regs[a.UInt64].Char /= *(pScript->m_Regs[b.UInt64].CharPtr+offset);
				}
				else if( addrmode & TwoBytes ) {
					if( !*(int16_t *)(pScript->m_Regs[b.UInt64].CharPtr+offset) )
						*(int16_t *)(pScript->m_Regs[b.UInt64].CharPtr+offset) = 1;
					pScript->m_Regs[a.UInt64].Short /= *(int16_t *)(pScript->m_Regs[b.UInt64].CharPtr+offset);
				}
				else if( addrmode & FourBytes ) {
					if( !*(int32_t *)(pScript->m_Regs[b.UInt64].CharPtr+offset) )
						*(int32_t *)(pScript->m_Regs[b.UInt64].CharPtr+offset) = 1;
					pScript->m_Regs[a.UInt64].Int32 /= *(int32_t *)(pScript->m_Regs[b.UInt64].CharPtr+offset);
				}
				else if( addrmode & EightBytes ) {
					if( !*(int64_t *)(pScript->m_Regs[b.UInt64].CharPtr+offset) )
						*(int64_t *)(pScript->m_Regs[b.UInt64].CharPtr+offset) = 1;
					pScript->m_Regs[a.UInt64].Int64 /= *(int64_t *)(pScript->m_Regs[b.UInt64].CharPtr+offset);
				}
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Byte ) {
					if( !*b.CharPtr )
						*b.CharPtr = 1;
					pScript->m_Regs[a.UInt64].Char /= *b.CharPtr;
				}
				else if( addrmode & TwoBytes ) {
					if( !*b.ShortPtr )
						*b.ShortPtr = 1;
					pScript->m_Regs[a.UInt64].Short /= *b.ShortPtr;
				}
				else if( addrmode & FourBytes ) {
					if( !*b.Int32Ptr )
						*b.Int32Ptr = 1;
					pScript->m_Regs[a.UInt64].Int32 /= *b.Int32Ptr;
				}
				else if( addrmode & EightBytes ) {
					if( !*b.Int64Ptr )
						*b.Int64Ptr = 1;
					pScript->m_Regs[a.UInt64].Int64 /= *b.Int64Ptr;
				}
			}
			continue;
		}
		exec_divm:; {	// dest is memory and src is reg
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Immediate ) {
					if( !b.UInt64 )
						b.UInt64 = 1;
					
					if( addrmode & Byte )
						*(pScript->m_Regs[a.UInt64].CharPtr+offset) /= b.Char;
					else if( addrmode & TwoBytes )
						*(int16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) /= b.Short;
					else if( addrmode & FourBytes )
						*(int32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) /= b.Int32;
					else if( addrmode & EightBytes )
						*(int64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) /= b.Int64;
				}
				else if( addrmode & Register ) {
					if( !pScript->m_Regs[b.UInt64].UInt64 )
						pScript->m_Regs[b.UInt64].UInt64 = 1;
					
					if( addrmode & Byte )
						*(pScript->m_Regs[a.UInt64].CharPtr+offset) /= pScript->m_Regs[b.UInt64].Char;
					else if( addrmode & TwoBytes )
						*(int16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) /= pScript->m_Regs[b.UInt64].Short;
					else if( addrmode & FourBytes )
						*(int32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) /= pScript->m_Regs[b.UInt64].Int32;
					else if( addrmode & EightBytes )
						*(int64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) /= pScript->m_Regs[b.UInt64].Int64;
				}
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Immediate ) {
					if( !b.UInt64 )
						b.UInt64 = 1;
					
					if( addrmode & Byte )
						*a.CharPtr /= b.Char;
					else if( addrmode & TwoBytes )
						*a.ShortPtr /= b.Short;
					else if( addrmode & FourBytes )
						*a.Int32Ptr /= b.Int32;
					else if( addrmode & EightBytes )
						*a.Int64Ptr /= b.Int64;
				}
				else if( addrmode & Register ) {
					if( !pScript->m_Regs[b.UInt64].UInt64 )
						pScript->m_Regs[b.UInt64].UInt64 = 1;
					
					if( addrmode & Byte )
						*a.CharPtr /= pScript->m_Regs[b.UInt64].Char;
					else if( addrmode & TwoBytes )
						*a.ShortPtr /= pScript->m_Regs[b.UInt64].Short;
					else if( addrmode & FourBytes )
						*a.Int32Ptr /= pScript->m_Regs[b.UInt64].Int32;
					else if( addrmode & EightBytes )
						*a.Int64Ptr /= pScript->m_Regs[b.UInt64].Int64;
				}
			}
			continue;
		}
		exec_udivr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate ) {
				if( !b.UInt64 )
					b.UInt64 = 1;
				pScript->m_Regs[a.UInt64].UInt64 /= b.UInt64;
			}
			else if( addrmode & Register ) {
				if( !pScript->m_Regs[b.UInt64].UInt64 )
					pScript->m_Regs[b.UInt64].UInt64 = 1;
				pScript->m_Regs[a.UInt64].UInt64 /= pScript->m_Regs[b.UInt64].UInt64;
			}
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Byte ) {
					if( !*(pScript->m_Regs[b.UInt64].UCharPtr+offset) )
						*(pScript->m_Regs[b.UInt64].UCharPtr+offset) = 1;
					pScript->m_Regs[a.UInt64].UChar /= *(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				}
				else if( addrmode & TwoBytes ) {
					if( !*(uint16_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset) )
						*(uint16_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset) = 1;
					pScript->m_Regs[a.UInt64].UShort /= *(uint16_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				}
				else if( addrmode & FourBytes ) {
					if( !*(uint32_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset) )
						*(uint32_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset) = 1;
					pScript->m_Regs[a.UInt64].UInt32 /= *(uint32_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				}
				else if( addrmode & EightBytes ) {
					if( !*(uint64_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset) )
						*(uint64_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset) = 1;
					pScript->m_Regs[a.UInt64].UInt64 /= *(uint64_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				}
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Byte ) {
					if( !*b.UCharPtr )
						*b.UCharPtr = 1;
					pScript->m_Regs[a.UInt64].UChar /= *b.UCharPtr;
				}
				else if( addrmode & TwoBytes ) {
					if( !*b.UShortPtr )
						*b.UShortPtr = 1;
					pScript->m_Regs[a.UInt64].UShort /= *b.UShortPtr;
				}
				else if( addrmode & FourBytes ) {
					if( !*b.UInt32Ptr )
						*b.UInt32Ptr = 1;
					pScript->m_Regs[a.UInt64].UInt32 /= *b.UInt32Ptr;
				}
				else if( addrmode & EightBytes ) {
					if( !*b.UInt64Ptr )
						*b.UInt64Ptr = 1;
					pScript->m_Regs[a.UInt64].UInt64 /= *b.UInt64Ptr;
				}
			}
			continue;
		}
		exec_udivm:; {	// dest is memory and src is reg
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Immediate ) {
					if( !b.UInt64 )
						b.UInt64 = 1;
					
					if( addrmode & Byte )
						*(pScript->m_Regs[a.UInt64].UCharPtr+offset) /= b.UChar;
					else if( addrmode & TwoBytes )
						*(uint16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) /= b.UShort;
					else if( addrmode & FourBytes )
						*(uint32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) /= b.UInt32;
					else if( addrmode & EightBytes )
						*(uint64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) /= b.UInt64;
				}
				else if( addrmode & Register ) {
					if( !pScript->m_Regs[b.UInt64].UInt64 )
						pScript->m_Regs[b.UInt64].UInt64 = 1;
					
					if( addrmode & Byte )
						*(pScript->m_Regs[a.UInt64].UCharPtr+offset) /= pScript->m_Regs[b.UInt64].UChar;
					else if( addrmode & TwoBytes )
						*(uint16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) /= pScript->m_Regs[b.UInt64].UShort;
					else if( addrmode & FourBytes )
						*(uint32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) /= pScript->m_Regs[b.UInt64].UInt32;
					else if( addrmode & EightBytes )
						*(uint64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) /= pScript->m_Regs[b.UInt64].UInt64;
				}
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Immediate ) {
					if( !b.UInt64 )
						b.UInt64 = 1;
					
					if( addrmode & Byte )
						*a.UCharPtr /= b.UChar;
					else if( addrmode & TwoBytes )
						*a.UShortPtr /= b.UShort;
					else if( addrmode & FourBytes )
						*a.UInt32Ptr /= b.UInt32;
					else if( addrmode & EightBytes )
						*a.UInt64Ptr /= b.UInt64;
				}
				else if( addrmode & Register ) {
					if( !pScript->m_Regs[b.UInt64].UInt64 )
						pScript->m_Regs[b.UInt64].UInt64 = 1;
					
					if( addrmode & Byte )
						*a.UCharPtr /= pScript->m_Regs[b.UInt64].UChar;
					else if( addrmode & TwoBytes )
						*a.UShortPtr /= pScript->m_Regs[b.UInt64].UShort;
					else if( addrmode & FourBytes )
						*a.UInt32Ptr /= pScript->m_Regs[b.UInt64].UInt32;
					else if( addrmode & EightBytes )
						*a.UInt64Ptr /= pScript->m_Regs[b.UInt64].UInt64;
				}
			}
			continue;
		}
		
		exec_modr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate ) {
				if( !b.Int64 )
					b.Int64 = 1;
				pScript->m_Regs[a.UInt64].Int64 %= b.Int64;
			}
			if( addrmode & Register ) {
				if( !pScript->m_Regs[b.UInt64].Int64 )
					pScript->m_Regs[b.UInt64].Int64 = 1;
				pScript->m_Regs[a.UInt64].Int64 %= pScript->m_Regs[b.UInt64].Int64;
			}
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Byte ) {
					if( !*(pScript->m_Regs[b.UInt64].CharPtr+offset) )
						*(pScript->m_Regs[b.UInt64].CharPtr+offset) = 1;
					pScript->m_Regs[a.UInt64].Char %= *(pScript->m_Regs[b.UInt64].CharPtr+offset);
				}
				else if( addrmode & TwoBytes ) {
					if( !*(int16_t *)(pScript->m_Regs[b.UInt64].CharPtr+offset) )
						*(int16_t *)(pScript->m_Regs[b.UInt64].CharPtr+offset) = 1;
					pScript->m_Regs[a.UInt64].Short %= *(int16_t *)(pScript->m_Regs[b.UInt64].CharPtr+offset);
				}
				else if( addrmode & FourBytes ) {
					if( !*(int32_t *)(pScript->m_Regs[b.UInt64].CharPtr+offset) )
						*(int32_t *)(pScript->m_Regs[b.UInt64].CharPtr+offset) = 1;
					pScript->m_Regs[a.UInt64].Int32 %= *(int32_t *)(pScript->m_Regs[b.UInt64].CharPtr+offset);
				}
				else if( addrmode & EightBytes ) {
					if( !*(int64_t *)(pScript->m_Regs[b.UInt64].CharPtr+offset) )
						*(int64_t *)(pScript->m_Regs[b.UInt64].CharPtr+offset) = 1;
					pScript->m_Regs[a.UInt64].Int64 %= *(int64_t *)(pScript->m_Regs[b.UInt64].CharPtr+offset);
				}
			}
			else if( addrmode & (IPRelative|Immediate) ) {
				if( addrmode & Byte ) {
					if( !*b.CharPtr )
						*b.CharPtr = 1;
					pScript->m_Regs[a.UInt64].Char %= *b.CharPtr;
				}
				else if( addrmode & TwoBytes ) {
					if( !*b.ShortPtr )
						*b.ShortPtr = 1;
					pScript->m_Regs[a.UInt64].Short %= *b.ShortPtr;
				}
				else if( addrmode & FourBytes ) {
					if( !*b.Int32Ptr )
						*b.Int32Ptr = 1;
					pScript->m_Regs[a.UInt64].Int32 %= *b.Int32Ptr;
				}
				else if( addrmode & EightBytes ) {
					if( !*b.Int64Ptr )
						*b.Int64Ptr = 1;
					pScript->m_Regs[a.UInt64].Int64 %= *b.Int64Ptr;
				}
			}
			continue;
		}
		exec_modm:; {	// dest is memory and src is reg
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Immediate ) {
					if( !b.UInt64 )
						b.UInt64 = 1;
					
					if( addrmode & Byte )
						*(pScript->m_Regs[a.UInt64].CharPtr+offset) %= b.Char;
					else if( addrmode & TwoBytes )
						*(int16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) %= b.Short;
					else if( addrmode & FourBytes )
						*(int32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) %= b.Int32;
					else if( addrmode & EightBytes )
						*(int64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) %= b.Int64;
				}
				else if( addrmode & Register ) {
					if( !pScript->m_Regs[b.UInt64].UInt64 )
						pScript->m_Regs[b.UInt64].UInt64 = 1;
					
					if( addrmode & Byte )
						*(pScript->m_Regs[a.UInt64].CharPtr+offset) %= pScript->m_Regs[b.UInt64].Char;
					else if( addrmode & TwoBytes )
						*(int16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) %= pScript->m_Regs[b.UInt64].Short;
					else if( addrmode & FourBytes )
						*(int32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) %= pScript->m_Regs[b.UInt64].Int32;
					else if( addrmode & EightBytes )
						*(int64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) %= pScript->m_Regs[b.UInt64].Int64;
				}
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Immediate ) {
					if( !b.UInt64 )
						b.UInt64 = 1;
					
					if( addrmode & Byte )
						*a.CharPtr %= b.Char;
					else if( addrmode & TwoBytes )
						*a.ShortPtr %= b.Short;
					else if( addrmode & FourBytes )
						*a.Int32Ptr %= b.Int32;
					else if( addrmode & EightBytes )
						*a.Int64Ptr %= b.Int64;
				}
				else if( addrmode & Register ) {
					if( !pScript->m_Regs[b.UInt64].UInt64 )
						pScript->m_Regs[b.UInt64].UInt64 = 1;
					
					if( addrmode & Byte )
						*a.CharPtr %= pScript->m_Regs[b.UInt64].Char;
					else if( addrmode & TwoBytes )
						*a.ShortPtr %= pScript->m_Regs[b.UInt64].Short;
					else if( addrmode & FourBytes )
						*a.Int32Ptr %= pScript->m_Regs[b.UInt64].Int32;
					else if( addrmode & EightBytes )
						*a.Int64Ptr %= pScript->m_Regs[b.UInt64].Int64;
				}
			}
			continue;
		}
		exec_umodr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate ) {
				if( !b.UInt64 )
					b.UInt64 = 1;
				pScript->m_Regs[a.UInt64].UInt64 %= b.UInt64;
			}
			else if( addrmode & Register ) {
				if( !pScript->m_Regs[b.UInt64].UInt64 )
					pScript->m_Regs[b.UInt64].UInt64 = 1;
				pScript->m_Regs[a.UInt64].UInt64 %= pScript->m_Regs[b.UInt64].UInt64;
			}
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Byte ) {
					if( !*(pScript->m_Regs[b.UInt64].UCharPtr+offset) )
						*(pScript->m_Regs[b.UInt64].UCharPtr+offset) = 1;
					pScript->m_Regs[a.UInt64].UChar %= *(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				}
				else if( addrmode & TwoBytes ) {
					if( !*(uint16_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset) )
						*(uint16_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset) = 1;
					pScript->m_Regs[a.UInt64].UShort %= *(uint16_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				}
				else if( addrmode & FourBytes ) {
					if( !*(uint32_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset) )
						*(uint32_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset) = 1;
					pScript->m_Regs[a.UInt64].UInt32 %= *(uint32_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				}
				else if( addrmode & EightBytes ) {
					if( !*(uint64_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset) )
						*(uint64_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset) = 1;
					pScript->m_Regs[a.UInt64].UInt64 %= *(uint64_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				}
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Byte ) {
					if( !*b.UCharPtr )
						*b.UCharPtr = 1;
					pScript->m_Regs[a.UInt64].UChar %= *b.UCharPtr;
				}
				else if( addrmode & TwoBytes ) {
					if( !*b.UShortPtr )
						*b.UShortPtr = 1;
					pScript->m_Regs[a.UInt64].UShort %= *b.UShortPtr;
				}
				else if( addrmode & FourBytes ) {
					if( !*b.UInt32Ptr )
						*b.UInt32Ptr = 1;
					pScript->m_Regs[a.UInt64].UInt32 %= *b.UInt32Ptr;
				}
				else if( addrmode & EightBytes ) {
					if( !*b.UInt64Ptr )
						*b.UInt64Ptr = 1;
					pScript->m_Regs[a.UInt64].UInt64 %= *b.UInt64Ptr;
				}
			}
			continue;
		}
		exec_umodm:; {	// dest is memory and src is reg
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Immediate ) {
					if( !b.UInt64 )
						b.UInt64 = 1;
					
					if( addrmode & Byte )
						*(pScript->m_Regs[a.UInt64].UCharPtr+offset) %= b.UChar;
					else if( addrmode & TwoBytes )
						*(uint16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) %= b.UShort;
					else if( addrmode & FourBytes )
						*(uint32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) %= b.UInt32;
					else if( addrmode & EightBytes )
						*(uint64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) %= b.UInt64;
				}
				else if( addrmode & Register ) {
					if( !pScript->m_Regs[b.UInt64].UInt64 )
						pScript->m_Regs[b.UInt64].UInt64 = 1;
					
					if( addrmode & Byte )
						*(pScript->m_Regs[a.UInt64].UCharPtr+offset) %= pScript->m_Regs[b.UInt64].UChar;
					else if( addrmode & TwoBytes )
						*(uint16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) %= pScript->m_Regs[b.UInt64].UShort;
					else if( addrmode & FourBytes )
						*(uint32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) %= pScript->m_Regs[b.UInt64].UInt32;
					else if( addrmode & EightBytes )
						*(uint64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) %= pScript->m_Regs[b.UInt64].UInt64;
				}
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Immediate ) {
					if( !b.UInt64 )
						b.UInt64 = 1;
					
					if( addrmode & Byte )
						*a.UCharPtr %= b.UChar;
					else if( addrmode & TwoBytes )
						*a.UShortPtr %= b.UShort;
					else if( addrmode & FourBytes )
						*a.UInt32Ptr %= b.UInt32;
					else if( addrmode & EightBytes )
						*a.UInt64Ptr %= b.UInt64;
				}
				else if( addrmode & Register ) {
					if( !pScript->m_Regs[b.UInt64].UInt64 )
						pScript->m_Regs[b.UInt64].UInt64 = 1;
					
					if( addrmode & Byte )
						*a.UCharPtr %= pScript->m_Regs[b.UInt64].UChar;
					else if( addrmode & TwoBytes )
						*a.UShortPtr %= pScript->m_Regs[b.UInt64].UShort;
					else if( addrmode & FourBytes )
						*a.UInt32Ptr %= pScript->m_Regs[b.UInt64].UInt32;
					else if( addrmode & EightBytes )
						*a.UInt64Ptr %= pScript->m_Regs[b.UInt64].UInt64;
				}
			}
			continue;
		}
		
		exec_shrr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pScript->m_Regs[a.UInt64].UInt64 >>= b.UInt64;
			else if( addrmode & Register )
				pScript->m_Regs[a.UInt64].UInt64 >>= pScript->m_Regs[b.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Byte )
					pScript->m_Regs[a.UInt64].UChar >>= *(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & TwoBytes )
					pScript->m_Regs[a.UInt64].UShort >>= *(uint16_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & FourBytes )
					pScript->m_Regs[a.UInt64].UInt32 >>= *(uint32_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & EightBytes )
					pScript->m_Regs[a.UInt64].UInt64 >>= *(uint64_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset);
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Byte )
					pScript->m_Regs[a.UInt64].UChar >>= *b.UCharPtr;
				else if( addrmode & TwoBytes )
					pScript->m_Regs[a.UInt64].UShort >>= *b.UShortPtr;
				else if( addrmode & FourBytes )
					pScript->m_Regs[a.UInt64].UInt32 >>= *b.UInt32Ptr;
				else if( addrmode & EightBytes )
					pScript->m_Regs[a.UInt64].UInt64 >>= *b.UInt64Ptr;
			}
			continue;
		}
		exec_shrm:; {	// dest is memory and src is reg
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						*(pScript->m_Regs[a.UInt64].UCharPtr+offset) >>= b.UChar;
					else if( addrmode & TwoBytes )
						*(uint16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) >>= b.UShort;
					else if( addrmode & FourBytes )
						*(uint32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) >>= b.UInt32;
					else if( addrmode & EightBytes )
						*(uint64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) >>= b.UInt64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						*(pScript->m_Regs[a.UInt64].UCharPtr+offset) >>= pScript->m_Regs[b.UInt64].UChar;
					else if( addrmode & TwoBytes )
						*(uint16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) >>= pScript->m_Regs[b.UInt64].UShort;
					else if( addrmode & FourBytes )
						*(uint32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) >>= pScript->m_Regs[b.UInt64].UInt32;
					else if( addrmode & EightBytes )
						*(uint64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) >>= pScript->m_Regs[b.UInt64].UInt64;
				}
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						*a.UCharPtr >>= b.UChar;
					else if( addrmode & TwoBytes )
						*a.UShortPtr >>= b.UShort;
					else if( addrmode & FourBytes )
						*a.UInt32Ptr >>= b.UInt32;
					else if( addrmode & EightBytes )
						*a.UInt64Ptr >>= b.UInt64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						*a.UCharPtr >>= pScript->m_Regs[b.UInt64].UChar;
					else if( addrmode & TwoBytes )
						*a.UShortPtr >>= pScript->m_Regs[b.UInt64].UShort;
					else if( addrmode & FourBytes )
						*a.UInt32Ptr >>= pScript->m_Regs[b.UInt64].UInt32;
					else if( addrmode & EightBytes )
						*a.UInt64Ptr >>= pScript->m_Regs[b.UInt64].UInt64;
				}
			}
			continue;
		}
		
		exec_shlr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pScript->m_Regs[a.UInt64].UInt64 <<= b.UInt64;
			else if( addrmode & Register )
				pScript->m_Regs[a.UInt64].UInt64 <<= pScript->m_Regs[b.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Byte )
					pScript->m_Regs[a.UInt64].UChar <<= *(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & TwoBytes )
					pScript->m_Regs[a.UInt64].UShort <<= *(uint16_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & FourBytes )
					pScript->m_Regs[a.UInt64].UInt32 <<= *(uint32_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & EightBytes )
					pScript->m_Regs[a.UInt64].UInt64 <<= *(uint64_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset);
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Byte )
					pScript->m_Regs[a.UInt64].UChar <<= *b.UCharPtr;
				else if( addrmode & TwoBytes )
					pScript->m_Regs[a.UInt64].UShort <<= *b.UShortPtr;
				else if( addrmode & FourBytes )
					pScript->m_Regs[a.UInt64].UInt32 <<= *b.UInt32Ptr;
				else if( addrmode & EightBytes )
					pScript->m_Regs[a.UInt64].UInt64 <<= *b.UInt64Ptr;
			}
			continue;
		}
		exec_shlm:; {	// dest is memory and src is reg
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						*(pScript->m_Regs[a.UInt64].UCharPtr+offset) <<= b.UChar;
					else if( addrmode & TwoBytes )
						*(uint16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) <<= b.UShort;
					else if( addrmode & FourBytes )
						*(uint32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) <<= b.UInt32;
					else if( addrmode & EightBytes )
						*(uint64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) <<= b.UInt64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						*(pScript->m_Regs[a.UInt64].UCharPtr+offset) <<= pScript->m_Regs[b.UInt64].UChar;
					else if( addrmode & TwoBytes )
						*(uint16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) <<= pScript->m_Regs[b.UInt64].UShort;
					else if( addrmode & FourBytes )
						*(uint32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) <<= pScript->m_Regs[b.UInt64].UInt32;
					else if( addrmode & EightBytes )
						*(uint64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) <<= pScript->m_Regs[b.UInt64].UInt64;
				}
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						*a.UCharPtr <<= b.UChar;
					else if( addrmode & TwoBytes )
						*a.UShortPtr <<= b.UShort;
					else if( addrmode & FourBytes )
						*a.UInt32Ptr <<= b.UInt32;
					else if( addrmode & EightBytes )
						*a.UInt64Ptr <<= b.UInt64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						*a.UCharPtr <<= pScript->m_Regs[b.UInt64].UChar;
					else if( addrmode & TwoBytes )
						*a.UShortPtr <<= pScript->m_Regs[b.UInt64].UShort;
					else if( addrmode & FourBytes )
						*a.UInt32Ptr <<= pScript->m_Regs[b.UInt64].UInt32;
					else if( addrmode & EightBytes )
						*a.UInt64Ptr <<= pScript->m_Regs[b.UInt64].UInt64;
				}
			}
			continue;
		}
		
		exec_andr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pScript->m_Regs[a.UInt64].UInt64 &= b.UInt64;
			else if( addrmode & Register )
				pScript->m_Regs[a.UInt64].UInt64 &= pScript->m_Regs[b.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Byte )
					pScript->m_Regs[a.UInt64].UChar &= *(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & TwoBytes )
					pScript->m_Regs[a.UInt64].UShort &= *(uint16_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & FourBytes )
					pScript->m_Regs[a.UInt64].UInt32 &= *(uint32_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & EightBytes )
					pScript->m_Regs[a.UInt64].UInt64 &= *(uint64_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset);
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Byte )
					pScript->m_Regs[a.UInt64].UChar &= *b.UCharPtr;
				else if( addrmode & TwoBytes )
					pScript->m_Regs[a.UInt64].UShort &= *b.UShortPtr;
				else if( addrmode & FourBytes )
					pScript->m_Regs[a.UInt64].UInt32 &= *b.UInt32Ptr;
				else if( addrmode & EightBytes )
					pScript->m_Regs[a.UInt64].UInt64 &= *b.UInt64Ptr;
			}
			continue;
		}
		exec_andm:; {	// dest is memory and src is reg
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						*(pScript->m_Regs[a.UInt64].UCharPtr+offset) &= b.UChar;
					else if( addrmode & TwoBytes )
						*(uint16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) &= b.UShort;
					else if( addrmode & FourBytes )
						*(uint32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) &= b.UInt32;
					else if( addrmode & EightBytes )
						*(uint64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) &= b.UInt64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						*(pScript->m_Regs[a.UInt64].UCharPtr+offset) &= pScript->m_Regs[b.UInt64].UChar;
					else if( addrmode & TwoBytes )
						*(uint16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) &= pScript->m_Regs[b.UInt64].UShort;
					else if( addrmode & FourBytes )
						*(uint32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) &= pScript->m_Regs[b.UInt64].UInt32;
					else if( addrmode & EightBytes )
						*(uint64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) &= pScript->m_Regs[b.UInt64].UInt64;
				}
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						*a.UCharPtr &= b.UChar;
					else if( addrmode & TwoBytes )
						*a.UShortPtr &= b.UShort;
					else if( addrmode & FourBytes )
						*a.UInt32Ptr &= b.UInt32;
					else if( addrmode & EightBytes )
						*a.UInt64Ptr &= b.UInt64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						*a.UCharPtr &= pScript->m_Regs[b.UInt64].UChar;
					else if( addrmode & TwoBytes )
						*a.UShortPtr &= pScript->m_Regs[b.UInt64].UShort;
					else if( addrmode & FourBytes )
						*a.UInt32Ptr &= pScript->m_Regs[b.UInt64].UInt32;
					else if( addrmode & EightBytes )
						*a.UInt64Ptr &= pScript->m_Regs[b.UInt64].UInt64;
				}
			}
			continue;
		}
		
		exec_orr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pScript->m_Regs[a.UInt64].UInt64 |= b.UInt64;
			else if( addrmode & Register )
				pScript->m_Regs[a.UInt64].UInt64 |= pScript->m_Regs[b.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Byte )
					pScript->m_Regs[a.UInt64].UChar |= *(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & TwoBytes )
					pScript->m_Regs[a.UInt64].UShort |= *(uint16_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & FourBytes )
					pScript->m_Regs[a.UInt64].UInt32 |= *(uint32_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & EightBytes )
					pScript->m_Regs[a.UInt64].UInt64 |= *(uint64_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset);
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Byte )
					pScript->m_Regs[a.UInt64].UChar |= *b.UCharPtr;
				else if( addrmode & TwoBytes )
					pScript->m_Regs[a.UInt64].UShort |= *b.UShortPtr;
				else if( addrmode & FourBytes )
					pScript->m_Regs[a.UInt64].UInt32 |= *b.UInt32Ptr;
				else if( addrmode & EightBytes )
					pScript->m_Regs[a.UInt64].UInt64 |= *b.UInt64Ptr;
			}
			continue;
		}
		exec_orm:; {	// dest is memory and src is reg
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						*(pScript->m_Regs[a.UInt64].UCharPtr+offset) |= b.UChar;
					else if( addrmode & TwoBytes )
						*(uint16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) |= b.UShort;
					else if( addrmode & FourBytes )
						*(uint32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) |= b.UInt32;
					else if( addrmode & EightBytes )
						*(uint64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) |= b.UInt64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						*(pScript->m_Regs[a.UInt64].UCharPtr+offset) |= pScript->m_Regs[b.UInt64].UChar;
					else if( addrmode & TwoBytes )
						*(uint16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) |= pScript->m_Regs[b.UInt64].UShort;
					else if( addrmode & FourBytes )
						*(uint32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) |= pScript->m_Regs[b.UInt64].UInt32;
					else if( addrmode & EightBytes )
						*(uint64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) |= pScript->m_Regs[b.UInt64].UInt64;
				}
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						*a.UCharPtr |= b.UChar;
					else if( addrmode & TwoBytes )
						*a.UShortPtr |= b.UShort;
					else if( addrmode & FourBytes )
						*a.UInt32Ptr |= b.UInt32;
					else if( addrmode & EightBytes )
						*a.UInt64Ptr |= b.UInt64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						*a.UCharPtr |= pScript->m_Regs[b.UInt64].UChar;
					else if( addrmode & TwoBytes )
						*a.UShortPtr |= pScript->m_Regs[b.UInt64].UShort;
					else if( addrmode & FourBytes )
						*a.UInt32Ptr |= pScript->m_Regs[b.UInt64].UInt32;
					else if( addrmode & EightBytes )
						*a.UInt64Ptr |= pScript->m_Regs[b.UInt64].UInt64;
				}
			}
			continue;
		}
		
		exec_xorr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pScript->m_Regs[a.UInt64].UInt64 ^= b.UInt64;
			else if( addrmode & Register )
				pScript->m_Regs[a.UInt64].UInt64 ^= pScript->m_Regs[b.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Byte )
					pScript->m_Regs[a.UInt64].UChar ^= *(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & TwoBytes )
					pScript->m_Regs[a.UInt64].UShort ^= *(uint16_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & FourBytes )
					pScript->m_Regs[a.UInt64].UInt32 ^= *(uint32_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & EightBytes )
					pScript->m_Regs[a.UInt64].UInt64 ^= *(uint64_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset);
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Byte )
					pScript->m_Regs[a.UInt64].UChar ^= *b.UCharPtr;
				else if( addrmode & TwoBytes )
					pScript->m_Regs[a.UInt64].UShort ^= *b.UShortPtr;
				else if( addrmode & FourBytes )
					pScript->m_Regs[a.UInt64].UInt32 ^= *b.UInt32Ptr;
				else if( addrmode & EightBytes )
					pScript->m_Regs[a.UInt64].UInt64 ^= *b.UInt64Ptr;
			}
			continue;
		}
		exec_xorm:; {	// dest is memory and src is reg
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						*(pScript->m_Regs[a.UInt64].UCharPtr+offset) ^= b.UChar;
					else if( addrmode & TwoBytes )
						*(uint16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) ^= b.UShort;
					else if( addrmode & FourBytes )
						*(uint32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) ^= b.UInt32;
					else if( addrmode & EightBytes )
						*(uint64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) ^= b.UInt64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						*(pScript->m_Regs[a.UInt64].UCharPtr+offset) ^= pScript->m_Regs[b.UInt64].UChar;
					else if( addrmode & TwoBytes )
						*(uint16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) ^= pScript->m_Regs[b.UInt64].UShort;
					else if( addrmode & FourBytes )
						*(uint32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) ^= pScript->m_Regs[b.UInt64].UInt32;
					else if( addrmode & EightBytes )
						*(uint64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) ^= pScript->m_Regs[b.UInt64].UInt64;
				}
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						*a.UCharPtr ^= b.UChar;
					else if( addrmode & TwoBytes )
						*a.UShortPtr ^= b.UShort;
					else if( addrmode & FourBytes )
						*a.UInt32Ptr ^= b.UInt32;
					else if( addrmode & EightBytes )
						*a.UInt64Ptr ^= b.UInt64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						*a.UCharPtr ^= pScript->m_Regs[b.UInt64].UChar;
					else if( addrmode & TwoBytes )
						*a.UShortPtr ^= pScript->m_Regs[b.UInt64].UShort;
					else if( addrmode & FourBytes )
						*a.UInt32Ptr ^= pScript->m_Regs[b.UInt64].UInt32;
					else if( addrmode & EightBytes )
						*a.UInt64Ptr ^= pScript->m_Regs[b.UInt64].UInt64;
				}
			}
			continue;
		}
		
		exec_ltr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Int64 < b.Int64;
			else if( addrmode & Register )
				pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Int64 < pScript->m_Regs[b.UInt64].Int64;
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Byte )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Char < *(pScript->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & TwoBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Short < *(int16_t *)(pScript->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & FourBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Int32 < *(int32_t *)(pScript->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & EightBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Int64 < *(int64_t *)(pScript->m_Regs[b.UInt64].CharPtr+offset);
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Byte )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Char < *b.CharPtr;
				else if( addrmode & TwoBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Short < *b.ShortPtr;
				else if( addrmode & FourBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Int32 < *b.Int32Ptr;
				else if( addrmode & EightBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Int64 < *b.Int64Ptr;
			}
			continue;
		}
		exec_ltm:; {	// dest is memory and src is reg
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						pScript->m_bZeroFlag = *(pScript->m_Regs[a.UInt64].CharPtr+offset) < b.Char;
					else if( addrmode & TwoBytes )
						pScript->m_bZeroFlag = *(int16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) < b.Short;
					else if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *(int32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) < b.Int32;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *(int64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) < b.Int64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						pScript->m_bZeroFlag = *(pScript->m_Regs[a.UInt64].UCharPtr+offset) < pScript->m_Regs[b.UInt64].Char;
					else if( addrmode & TwoBytes )
						pScript->m_bZeroFlag = *(int16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) < pScript->m_Regs[b.UInt64].Short;
					else if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *(int32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) < pScript->m_Regs[b.UInt64].Int32;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *(int64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) < pScript->m_Regs[b.UInt64].Int64;
				}
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						pScript->m_bZeroFlag = *a.CharPtr < b.Char;
					else if( addrmode & TwoBytes )
						pScript->m_bZeroFlag = *a.ShortPtr < b.Short;
					else if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *a.Int32Ptr < b.Int32;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *a.Int64Ptr < b.Int64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						pScript->m_bZeroFlag = *a.CharPtr < pScript->m_Regs[b.UInt64].Char;
					else if( addrmode & TwoBytes )
						pScript->m_bZeroFlag = *a.ShortPtr < pScript->m_Regs[b.UInt64].Short;
					else if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *a.Int32Ptr < pScript->m_Regs[b.UInt64].Int32;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *a.Int64Ptr < pScript->m_Regs[b.UInt64].Int64;
				}
			}
			continue;
		}
		exec_ultr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].UInt64 < b.UInt64;
			else if( addrmode & Register )
				pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].UInt64 < pScript->m_Regs[b.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Byte )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].UChar < *(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & TwoBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].UShort < *(uint16_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & FourBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].UInt32 < *(uint32_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & EightBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].UInt64 < *(uint64_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset);
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Byte )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].UChar < *b.UCharPtr;
				else if( addrmode & TwoBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].UShort < *b.UShortPtr;
				else if( addrmode & FourBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].UInt32 < *b.UInt32Ptr;
				else if( addrmode & EightBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].UInt64 < *b.UInt64Ptr;
			}
			continue;
		}
		exec_ultm:; {	// dest is memory and src is reg
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						pScript->m_bZeroFlag = *(pScript->m_Regs[a.UInt64].UCharPtr+offset) < b.UChar;
					else if( addrmode & TwoBytes )
						pScript->m_bZeroFlag = *(uint16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) < b.UShort;
					else if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *(uint32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) < b.UInt32;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *(uint64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) < b.UInt64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						pScript->m_bZeroFlag = *(pScript->m_Regs[a.UInt64].UCharPtr+offset) < pScript->m_Regs[b.UInt64].UChar;
					else if( addrmode & TwoBytes )
						pScript->m_bZeroFlag = *(uint16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) < pScript->m_Regs[b.UInt64].UShort;
					else if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *(uint32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) < pScript->m_Regs[b.UInt64].UInt32;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *(uint64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) < pScript->m_Regs[b.UInt64].UInt64;
				}
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						pScript->m_bZeroFlag = *a.UCharPtr < b.UChar;
					else if( addrmode & TwoBytes )
						pScript->m_bZeroFlag = *a.UShortPtr < b.UShort;
					else if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *a.UInt32Ptr < b.UInt32;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *a.UInt64Ptr < b.UInt64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						pScript->m_bZeroFlag = *a.UCharPtr < pScript->m_Regs[b.UInt64].UChar;
					else if( addrmode & TwoBytes )
						pScript->m_bZeroFlag = *a.UShortPtr < pScript->m_Regs[b.UInt64].UShort;
					else if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *a.UInt32Ptr < pScript->m_Regs[b.UInt64].UInt32;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *a.UInt64Ptr < pScript->m_Regs[b.UInt64].UInt64;
				}
			}
			continue;
		}
		
		exec_gtr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Int64 > b.Int64;
			else if( addrmode & Register )
				pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Int64 > pScript->m_Regs[b.UInt64].Int64;
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Byte )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Char > *(pScript->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & TwoBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Short > *(int16_t *)(pScript->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & FourBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Int32 > *(int32_t *)(pScript->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & EightBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Int64 > *(int64_t *)(pScript->m_Regs[b.UInt64].CharPtr+offset);
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Byte )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Char > *b.CharPtr;
				else if( addrmode & TwoBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Short > *b.ShortPtr;
				else if( addrmode & FourBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Int32 > *b.Int32Ptr;
				else if( addrmode & EightBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Int64 > *b.Int64Ptr;
			}
			continue;
		}
		exec_gtm:; {	// dest is memory and src is reg
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						pScript->m_bZeroFlag = *(pScript->m_Regs[a.UInt64].CharPtr+offset) > b.Char;
					else if( addrmode & TwoBytes )
						pScript->m_bZeroFlag = *(int16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) > b.Short;
					else if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *(int32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) > b.Int32;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *(int64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) > b.Int64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						pScript->m_bZeroFlag = *(pScript->m_Regs[a.UInt64].UCharPtr+offset) > pScript->m_Regs[b.UInt64].Char;
					else if( addrmode & TwoBytes )
						pScript->m_bZeroFlag = *(int16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) > pScript->m_Regs[b.UInt64].Short;
					else if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *(int32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) > pScript->m_Regs[b.UInt64].Int32;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *(int64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) > pScript->m_Regs[b.UInt64].Int64;
				}
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						pScript->m_bZeroFlag = *a.CharPtr > b.Char;
					else if( addrmode & TwoBytes )
						pScript->m_bZeroFlag = *a.ShortPtr > b.Short;
					else if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *a.Int32Ptr > b.Int32;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *a.Int64Ptr > b.Int64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						pScript->m_bZeroFlag = *a.CharPtr > pScript->m_Regs[b.UInt64].Char;
					else if( addrmode & TwoBytes )
						pScript->m_bZeroFlag = *a.ShortPtr > pScript->m_Regs[b.UInt64].Short;
					else if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *a.Int32Ptr > pScript->m_Regs[b.UInt64].Int32;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *a.Int64Ptr > pScript->m_Regs[b.UInt64].Int64;
				}
			}
			continue;
		}
		exec_ugtr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].UInt64 > b.UInt64;
			else if( addrmode & Register )
				pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].UInt64 > pScript->m_Regs[b.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Byte )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].UChar > *(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & TwoBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].UShort > *(uint16_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & FourBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].UInt32 > *(uint32_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & EightBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].UInt64 > *(uint64_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset);
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Byte )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].UChar > *b.UCharPtr;
				else if( addrmode & TwoBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].UShort > *b.UShortPtr;
				else if( addrmode & FourBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].UInt32 > *b.UInt32Ptr;
				else if( addrmode & EightBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].UInt64 > *b.UInt64Ptr;
			}
			break;
		}
		exec_ugtm:; {	// dest is memory and src is reg
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						pScript->m_bZeroFlag = *(pScript->m_Regs[a.UInt64].UCharPtr+offset) > b.UChar;
					else if( addrmode & TwoBytes )
						pScript->m_bZeroFlag = *(uint16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) > b.UShort;
					else if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *(uint32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) > b.UInt32;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *(uint64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) > b.UInt64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						pScript->m_bZeroFlag = *(pScript->m_Regs[a.UInt64].UCharPtr+offset) > pScript->m_Regs[b.UInt64].UChar;
					else if( addrmode & TwoBytes )
						pScript->m_bZeroFlag = *(uint16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) > pScript->m_Regs[b.UInt64].UShort;
					else if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *(uint32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) > pScript->m_Regs[b.UInt64].UInt32;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *(uint64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) > pScript->m_Regs[b.UInt64].UInt64;
				}
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						pScript->m_bZeroFlag = *a.UCharPtr > b.UChar;
					else if( addrmode & TwoBytes )
						pScript->m_bZeroFlag = *a.UShortPtr > b.UShort;
					else if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *a.UInt32Ptr > b.UInt32;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *a.UInt64Ptr > b.UInt64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						pScript->m_bZeroFlag = *a.UCharPtr > pScript->m_Regs[b.UInt64].UChar;
					else if( addrmode & TwoBytes )
						pScript->m_bZeroFlag = *a.UShortPtr > pScript->m_Regs[b.UInt64].UShort;
					else if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *a.UInt32Ptr > pScript->m_Regs[b.UInt64].UInt32;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *a.UInt64Ptr > pScript->m_Regs[b.UInt64].UInt64;
				}
			}
			continue;
		}
		
		exec_cmpr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Int64 == b.Int64;
			else if( addrmode & Register )
				pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Int64 == pScript->m_Regs[b.UInt64].Int64;
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Byte )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Char == *(pScript->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & TwoBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Short == *(int16_t *)(pScript->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & FourBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Int32 == *(int32_t *)(pScript->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & EightBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Int64 == *(int64_t *)(pScript->m_Regs[b.UInt64].CharPtr+offset);
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Byte )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Char == *b.CharPtr;
				else if( addrmode & TwoBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Short == *b.ShortPtr;
				else if( addrmode & FourBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Int32 == *b.Int32Ptr;
				else if( addrmode & EightBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Int64 == *b.Int64Ptr;
			}
			continue;
		}
		exec_cmpm:; {	// dest is memory and src is reg
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						pScript->m_bZeroFlag = *(pScript->m_Regs[a.UInt64].CharPtr+offset) == b.Char;
					else if( addrmode & TwoBytes )
						pScript->m_bZeroFlag = *(int16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) == b.Short;
					else if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *(int32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) == b.Int32;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *(int64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) == b.Int64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						pScript->m_bZeroFlag = *(pScript->m_Regs[a.UInt64].UCharPtr+offset) == pScript->m_Regs[b.UInt64].Char;
					else if( addrmode & TwoBytes )
						pScript->m_bZeroFlag = *(int16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) == pScript->m_Regs[b.UInt64].Short;
					else if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *(int32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) == pScript->m_Regs[b.UInt64].Int32;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *(int64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) == pScript->m_Regs[b.UInt64].Int64;
				}
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						pScript->m_bZeroFlag = *a.CharPtr == b.Char;
					else if( addrmode & TwoBytes )
						pScript->m_bZeroFlag = *a.ShortPtr == b.Short;
					else if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *a.Int32Ptr == b.Int32;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *a.Int64Ptr == b.Int64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						pScript->m_bZeroFlag = *a.CharPtr == pScript->m_Regs[b.UInt64].Char;
					else if( addrmode & TwoBytes )
						pScript->m_bZeroFlag = *a.ShortPtr == pScript->m_Regs[b.UInt64].Short;
					else if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *a.Int32Ptr == pScript->m_Regs[b.UInt64].Int32;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *a.Int64Ptr == pScript->m_Regs[b.UInt64].Int64;
				}
			}
			continue;
		}
		exec_ucmpr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].UInt64 == b.UInt64;
			else if( addrmode & Register )
				pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].UInt64 == pScript->m_Regs[b.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Byte )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].UChar == *(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & TwoBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].UShort == *(uint16_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & FourBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].UInt32 == *(uint32_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & EightBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].UInt64 == *(uint64_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset);
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Byte )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].UChar == *b.UCharPtr;
				else if( addrmode & TwoBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].UShort == *b.UShortPtr;
				else if( addrmode & FourBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].UInt32 == *b.UInt32Ptr;
				else if( addrmode & EightBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].UInt64 == *b.UInt64Ptr;
			}
			continue;
		}
		exec_ucmpm:; {	// dest is memory and src is reg
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						pScript->m_bZeroFlag = *(pScript->m_Regs[a.UInt64].UCharPtr+offset) == b.UChar;
					else if( addrmode & TwoBytes )
						pScript->m_bZeroFlag = *(uint16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) == b.UShort;
					else if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *(uint32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) == b.UInt32;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *(uint64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) == b.UInt64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						pScript->m_bZeroFlag = *(pScript->m_Regs[a.UInt64].UCharPtr+offset) == pScript->m_Regs[b.UInt64].UChar;
					else if( addrmode & TwoBytes )
						pScript->m_bZeroFlag = *(uint16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) == pScript->m_Regs[b.UInt64].UShort;
					else if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *(uint32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) == pScript->m_Regs[b.UInt64].UInt32;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *(uint64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) == pScript->m_Regs[b.UInt64].UInt64;
				}
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						pScript->m_bZeroFlag = *a.UCharPtr == b.UChar;
					else if( addrmode & TwoBytes )
						pScript->m_bZeroFlag = *a.UShortPtr == b.UShort;
					else if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *a.UInt32Ptr == b.UInt32;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *a.UInt64Ptr == b.UInt64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						pScript->m_bZeroFlag = *a.UCharPtr == pScript->m_Regs[b.UInt64].UChar;
					else if( addrmode & TwoBytes )
						pScript->m_bZeroFlag = *a.UShortPtr == pScript->m_Regs[b.UInt64].UShort;
					else if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *a.UInt32Ptr == pScript->m_Regs[b.UInt64].UInt32;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *a.UInt64Ptr == pScript->m_Regs[b.UInt64].UInt64;
				}
			}
			continue;
		}
		
		exec_neqr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Int64 != b.Int64;
			else if( addrmode & Register )
				pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Int64 != pScript->m_Regs[b.UInt64].Int64;
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Byte )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Char != *(pScript->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & TwoBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Short != *(int16_t *)(pScript->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & FourBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Int32 != *(int32_t *)(pScript->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & EightBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Int64 != *(int64_t *)(pScript->m_Regs[b.UInt64].CharPtr+offset);
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Byte )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Char != *b.CharPtr;
				else if( addrmode & TwoBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Short != *b.ShortPtr;
				else if( addrmode & FourBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Int32 != *b.Int32Ptr;
				else if( addrmode & EightBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Int64 != *b.Int64Ptr;
			}
			continue;
		}
		exec_neqm:; {	// dest is memory and src is reg
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						pScript->m_bZeroFlag = *(pScript->m_Regs[a.UInt64].CharPtr+offset) != b.Char;
					else if( addrmode & TwoBytes )
						pScript->m_bZeroFlag = *(int16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) != b.Short;
					else if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *(int32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) != b.Int32;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *(int64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) != b.Int64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						pScript->m_bZeroFlag = *(pScript->m_Regs[a.UInt64].UCharPtr+offset) != pScript->m_Regs[b.UInt64].Char;
					else if( addrmode & TwoBytes )
						pScript->m_bZeroFlag = *(int16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) != pScript->m_Regs[b.UInt64].Short;
					else if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *(int32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) != pScript->m_Regs[b.UInt64].Int32;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *(int64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) != pScript->m_Regs[b.UInt64].Int64;
				}
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						pScript->m_bZeroFlag = *a.CharPtr != b.Char;
					else if( addrmode & TwoBytes )
						pScript->m_bZeroFlag = *a.ShortPtr != b.Short;
					else if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *a.Int32Ptr != b.Int32;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *a.Int64Ptr != b.Int64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						pScript->m_bZeroFlag = *a.CharPtr != pScript->m_Regs[b.UInt64].Char;
					else if( addrmode & TwoBytes )
						pScript->m_bZeroFlag = *a.ShortPtr != pScript->m_Regs[b.UInt64].Short;
					else if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *a.Int32Ptr != pScript->m_Regs[b.UInt64].Int32;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *a.Int64Ptr != pScript->m_Regs[b.UInt64].Int64;
				}
			}
			continue;
		}
		exec_uneqr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].UInt64 != b.UInt64;
			else if( addrmode & Register )
				pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].UInt64 != pScript->m_Regs[b.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Byte )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].UChar != *(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & TwoBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].UShort != *(uint16_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & FourBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].UInt32 != *(uint32_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & EightBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].UInt64 != *(uint64_t *)(pScript->m_Regs[b.UInt64].UCharPtr+offset);
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Byte )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].UChar != *b.UCharPtr;
				else if( addrmode & TwoBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].UShort != *b.UShortPtr;
				else if( addrmode & FourBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].UInt32 != *b.UInt32Ptr;
				else if( addrmode & EightBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].UInt64 != *b.UInt64Ptr;
			}
			continue;
		}
		exec_uneqm:; {	// dest is memory and src is reg
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						pScript->m_bZeroFlag = *(pScript->m_Regs[a.UInt64].UCharPtr+offset) != b.UChar;
					else if( addrmode & TwoBytes )
						pScript->m_bZeroFlag = *(uint16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) != b.UShort;
					else if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *(uint32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) != b.UInt32;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *(uint64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) != b.UInt64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						pScript->m_bZeroFlag = *(pScript->m_Regs[a.UInt64].UCharPtr+offset) != pScript->m_Regs[b.UInt64].UChar;
					else if( addrmode & TwoBytes )
						pScript->m_bZeroFlag = *(uint16_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) != pScript->m_Regs[b.UInt64].UShort;
					else if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *(uint32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) != pScript->m_Regs[b.UInt64].UInt32;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *(uint64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) != pScript->m_Regs[b.UInt64].UInt64;
				}
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						pScript->m_bZeroFlag = *a.UCharPtr != b.UChar;
					else if( addrmode & TwoBytes )
						pScript->m_bZeroFlag = *a.UShortPtr != b.UShort;
					else if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *a.UInt32Ptr != b.UInt32;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *a.UInt64Ptr != b.UInt64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						pScript->m_bZeroFlag = *a.UCharPtr != pScript->m_Regs[b.UInt64].UChar;
					else if( addrmode & TwoBytes )
						pScript->m_bZeroFlag = *a.UShortPtr != pScript->m_Regs[b.UInt64].UShort;
					else if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *a.UInt32Ptr != pScript->m_Regs[b.UInt64].UInt32;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *a.UInt64Ptr != pScript->m_Regs[b.UInt64].UInt64;
				}
			}
			continue;
		}
		exec_reset:; {	// reset ALL registers except rip, rsp, and rbp to 0.
			TaghaScript_Reset(pScript);
			continue;
		}
		
		exec_int2float:; {
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate ) {
				b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
				pScript->m_Regs[a.UInt64].UInt64 = 0;
				pScript->m_Regs[a.UInt64].Float = (float) b.Int64;
			}
			else if( addrmode & Register ) {
				float temp = (float) pScript->m_Regs[a.UInt64].Int64;
				pScript->m_Regs[a.UInt64].UInt64 = 0;
				pScript->m_Regs[a.UInt64].Float = temp;
			}
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				float temp = (float) *(int32_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset);
				*(float *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) = temp;
			}
			else if( addrmode & IPRelative ) {
				float temp = (float) *a.Int32Ptr;
				*a.FloatPtr = temp;
			}
			continue;
		}
		exec_int2dbl:; {
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate ) {
				b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
				pScript->m_Regs[a.UInt64].UInt64 = 0;
				pScript->m_Regs[a.UInt64].Double = (double) b.UInt64;
			}
			else if( addrmode & Register ) {
				double temp = (double) pScript->m_Regs[a.UInt64].UInt64;
				pScript->m_Regs[a.UInt64].UInt64 = 0;
				pScript->m_Regs[a.UInt64].Double = temp;
			}
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				double temp = (double) *(int64_t *)(pScript->m_Regs[a.UInt64].UCharPtr+offset);
				*(double *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) = temp;
			}
			else if( addrmode & IPRelative ) {
				double temp = (double) *a.Int64Ptr;
				*a.DoublePtr = temp;
			}
			continue;
		}
		
		exec_float2dbl:; {
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			double temp;
			if( addrmode & Immediate ) {
				b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
				temp = (double) b.Float;
				pScript->m_Regs[a.UInt64].UInt64 = 0;
				pScript->m_Regs[a.UInt64].Double = temp;
			}
			else if( addrmode & Register ) {
				temp = (double) pScript->m_Regs[a.UInt64].Float;
				pScript->m_Regs[a.UInt64].UInt64 = 0;
				pScript->m_Regs[a.UInt64].Double = temp;
			}
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				temp = (double) *(float *)(pScript->m_Regs[a.UInt64].UCharPtr+offset);
				*(double *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) = temp;
			}
			else if( addrmode & IPRelative ) {
				temp = (double) *a.FloatPtr;
				*a.DoublePtr = 0;
				*a.DoublePtr = temp;
			}
			continue;
		}
		exec_dbl2float:; {
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			float temp;
			if( addrmode & Immediate ) {
				b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
				temp = (float) b.Double;
				pScript->m_Regs[a.UInt64].UInt64 = 0;
				pScript->m_Regs[a.UInt64].Float = temp;
			}
			else if( addrmode & Register ) {
				temp = (float) pScript->m_Regs[a.UInt64].Double;
				pScript->m_Regs[a.UInt64].UInt64 = 0;
				pScript->m_Regs[a.UInt64].Float = temp;
			}
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				temp = (float) *(double *)(pScript->m_Regs[a.UInt64].UCharPtr+offset);
				*(double *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) = 0;
				*(float *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) = temp;
			}
			else if( addrmode & IPRelative ) {
				temp = (float) *a.DoublePtr;
				*a.FloatPtr = 0;
				*a.FloatPtr = temp;
			}
			continue;
		}
		
		exec_faddr:; {
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pScript->m_Regs[a.UInt64].Double += b.Double;
			else if( addrmode & Register )
				pScript->m_Regs[a.UInt64].Double += pScript->m_Regs[b.UInt64].Double;
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & FourBytes )
					pScript->m_Regs[a.UInt64].Float += *(float *)(pScript->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & EightBytes )
					pScript->m_Regs[a.UInt64].Double += *(double *)(pScript->m_Regs[b.UInt64].CharPtr+offset);
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & FourBytes )
					pScript->m_Regs[a.UInt64].Float += *b.FloatPtr;
				else if( addrmode & EightBytes )
					pScript->m_Regs[a.UInt64].Double += *b.DoublePtr;
			}
			continue;
		}
		exec_faddm:; {
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Immediate ) {
					if( addrmode & FourBytes ) {
						*(float *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) += b.Float;
					}
					else if( addrmode & EightBytes ) {
						*(double *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) += b.Double;
					}
				}
				else if( addrmode & Register ) {
					if( addrmode & FourBytes )
						*(float *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) += pScript->m_Regs[b.UInt64].Float;
					else if( addrmode & EightBytes )
						*(double *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) += pScript->m_Regs[b.UInt64].Double;
				}
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Immediate ) {
					if( addrmode & FourBytes )
						*a.FloatPtr += b.Float;
					else if( addrmode & EightBytes )
						*a.DoublePtr += b.Double;
				}
				else if( addrmode & Register ) {
					if( addrmode & FourBytes )
						*a.FloatPtr += pScript->m_Regs[b.UInt64].Float;
					else if( addrmode & EightBytes )
						*a.DoublePtr += pScript->m_Regs[b.UInt64].Double;
				}
			}
			continue;
		}
		exec_fsubr:; {
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pScript->m_Regs[a.UInt64].Double -= b.Double;
			else if( addrmode & Register )
				pScript->m_Regs[a.UInt64].Double -= pScript->m_Regs[b.UInt64].Double;
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & FourBytes )
					pScript->m_Regs[a.UInt64].Float -= *(float *)(pScript->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & EightBytes )
					pScript->m_Regs[a.UInt64].Double -= *(double *)(pScript->m_Regs[b.UInt64].CharPtr+offset);
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & FourBytes )
					pScript->m_Regs[a.UInt64].Float -= *b.FloatPtr;
				else if( addrmode & EightBytes )
					pScript->m_Regs[a.UInt64].Double -= *b.DoublePtr;
			}
			continue;
		}
		exec_fsubm:; {
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Immediate ) {
					if( addrmode & FourBytes )
						*(float *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) -= b.Float;
					else if( addrmode & EightBytes )
						*(double *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) -= b.Double;
				}
				else if( addrmode & Register ) {
					if( addrmode & FourBytes )
						*(float *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) -= pScript->m_Regs[b.UInt64].Float;
					else if( addrmode & EightBytes )
						*(double *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) -= pScript->m_Regs[b.UInt64].Double;
				}
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Immediate ) {
					if( addrmode & FourBytes )
						*a.FloatPtr -= b.Float;
					else if( addrmode & EightBytes )
						*a.DoublePtr -= b.Double;
				}
				else if( addrmode & Register ) {
					if( addrmode & FourBytes )
						*a.FloatPtr -= pScript->m_Regs[b.UInt64].Float;
					else if( addrmode & EightBytes )
						*a.DoublePtr -= pScript->m_Regs[b.UInt64].Double;
				}
			}
			continue;
		}
		exec_fmulr:; {
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pScript->m_Regs[a.UInt64].Double *= b.Double;
			else if( addrmode & Register )
				pScript->m_Regs[a.UInt64].Double *= pScript->m_Regs[b.UInt64].Double;
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & FourBytes )
					pScript->m_Regs[a.UInt64].Float *= *(float *)(pScript->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & EightBytes )
					pScript->m_Regs[a.UInt64].Double *= *(double *)(pScript->m_Regs[b.UInt64].CharPtr+offset);
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & FourBytes )
					pScript->m_Regs[a.UInt64].Float *= *b.FloatPtr;
				else if( addrmode & EightBytes )
					pScript->m_Regs[a.UInt64].Double *= *b.DoublePtr;
			}
			continue;
		}
		exec_fmulm:; {
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Immediate ) {
					if( addrmode & FourBytes )
						*(float *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) *= b.Float;
					else if( addrmode & EightBytes )
						*(double *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) *= b.Double;
				}
				else if( addrmode & Register ) {
					if( addrmode & FourBytes )
						*(float *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) *= pScript->m_Regs[b.UInt64].Float;
					else if( addrmode & EightBytes )
						*(double *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) *= pScript->m_Regs[b.UInt64].Double;
				}
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Immediate ) {
					if( addrmode & FourBytes )
						*a.FloatPtr *= b.Float;
					else if( addrmode & EightBytes )
						*a.DoublePtr *= b.Double;
				}
				else if( addrmode & Register ) {
					if( addrmode & FourBytes )
						*a.FloatPtr *= pScript->m_Regs[b.UInt64].Float;
					else if( addrmode & EightBytes )
						*a.DoublePtr *= pScript->m_Regs[b.UInt64].Double;
				}
			}
			continue;
		}
		exec_fdivr:; {
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate ) {
				if( !b.Double )
					b.Double=1.0;
				pScript->m_Regs[a.UInt64].Double /= b.Double;
			}
			else if( addrmode & Register ) {
				if( !pScript->m_Regs[b.UInt64].Double )
					pScript->m_Regs[b.UInt64].Double=1.0;
				pScript->m_Regs[a.UInt64].Double /= pScript->m_Regs[b.UInt64].Double;
			}
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & FourBytes ) {
					if( !*(float *)(pScript->m_Regs[b.UInt64].CharPtr+offset) )
						*(float *)(pScript->m_Regs[b.UInt64].CharPtr+offset) = 1.f;
					pScript->m_Regs[a.UInt64].Float /= *(float *)(pScript->m_Regs[b.UInt64].CharPtr+offset);
				}
				else if( addrmode & EightBytes ) {
					if( !*(double *)(pScript->m_Regs[b.UInt64].CharPtr+offset) )
						*(double *)(pScript->m_Regs[b.UInt64].CharPtr+offset) = 1.0;
					pScript->m_Regs[a.UInt64].Double /= *(double *)(pScript->m_Regs[b.UInt64].CharPtr+offset);
				}
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & FourBytes ) {
					if( !*b.FloatPtr )
						*b.FloatPtr = 1.f;
					pScript->m_Regs[a.UInt64].Float /= *b.FloatPtr;
				}
				else if( addrmode & EightBytes ) {
					if( !*b.DoublePtr )
						*b.DoublePtr = 1.0;
					pScript->m_Regs[a.UInt64].Double /= *b.DoublePtr;
				}
			}
			continue;
		}
		exec_fdivm:; {
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Immediate ) {
					if( addrmode & FourBytes ) {
						if( !b.Float )
							b.Float = 1.f;
						*(float *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) /= b.Float;
					}
					else if( addrmode & EightBytes ) {
						if( !b.Double )
							b.Double = 1.0;
						*(double *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) /= b.Double;
					}
				}
				else if( addrmode & Register ) {
					if( addrmode & FourBytes ) {
						if( !b.Float )
							b.Float = 1.f;
						*(float *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) /= pScript->m_Regs[b.UInt64].Float;
					}
					else if( addrmode & EightBytes ) {
						if( !b.Double )
							b.Double = 1.f;
						*(double *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) /= pScript->m_Regs[b.UInt64].Double;
					}
				}
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Immediate ) {
					if( addrmode & FourBytes ) {
						if( !b.Float )
							b.Float = 1.f;
						*a.FloatPtr /= b.Float;
					}
					else if( addrmode & EightBytes ) {
						if( !b.Double )
							b.Double = 1.0;
						*a.DoublePtr /= b.Double;
					}
				}
				else if( addrmode & Register ) {
					if( addrmode & FourBytes ) {
						if( !pScript->m_Regs[b.UInt64].Float )
							pScript->m_Regs[b.UInt64].Float = 1.f;
						*a.FloatPtr /= pScript->m_Regs[b.UInt64].Float;
					}
					else if( addrmode & EightBytes ) {
						if( !pScript->m_Regs[b.UInt64].Double )
							pScript->m_Regs[b.UInt64].Double = 1.0;
						*a.DoublePtr /= pScript->m_Regs[b.UInt64].Double;
					}
				}
			}
			continue;
		}
		exec_fneg:; {
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Register )
				pScript->m_Regs[a.UInt64].Double = -pScript->m_Regs[a.UInt64].Double;
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & FourBytes )
					*(float *)(pScript->m_Regs[a.UInt64].CharPtr+offset) = -*(float *)(pScript->m_Regs[a.UInt64].CharPtr+offset);
				else if( addrmode & EightBytes )
					*(double *)(pScript->m_Regs[a.UInt64].CharPtr+offset) = -*(double *)(pScript->m_Regs[a.UInt64].CharPtr+offset);
			}
			else if( addrmode & (IPRelative|Immediate) ) {
				if( addrmode & FourBytes )
					*a.FloatPtr = -*a.FloatPtr;
				else if( addrmode & EightBytes )
					*a.DoublePtr = -*a.DoublePtr;
			}
			continue;
		}
		exec_fltr:; {
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Double < b.Double;
			else if( addrmode & Register )
				pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Double < pScript->m_Regs[b.UInt64].Double;
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & FourBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Float < *(float *)(pScript->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & EightBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Double < *(double *)(pScript->m_Regs[b.UInt64].CharPtr+offset);
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & FourBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Float < *b.FloatPtr;
				else if( addrmode & EightBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Double < *b.DoublePtr;
			}
			continue;
		}
		exec_fltm:; {
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Immediate ) {
					if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *(float *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) < b.Float;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *(double *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) < b.Double;
				}
				else if( addrmode & Register ) {
					if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *(float *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) < pScript->m_Regs[b.UInt64].Float;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *(double *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) < pScript->m_Regs[b.UInt64].Double;
				}
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Immediate ) {
					if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *a.FloatPtr < b.Float;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *a.DoublePtr < b.Double;
				}
				else if( addrmode & Register ) {
					if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *a.FloatPtr < pScript->m_Regs[b.UInt64].Float;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *a.DoublePtr < pScript->m_Regs[b.UInt64].Double;
				}
			}
			continue;
		}
		exec_fgtr:; {
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Double > b.Double;
			else if( addrmode & Register )
				pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Double > pScript->m_Regs[b.UInt64].Double;
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & FourBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Float > *(float *)(pScript->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & EightBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Double > *(double *)(pScript->m_Regs[b.UInt64].CharPtr+offset);
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & FourBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Float > *b.FloatPtr;
				else if( addrmode & EightBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Double > *b.DoublePtr;
			}
			continue;
		}
		exec_fgtm:; {
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Immediate ) {
					if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *(float *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) > b.Float;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *(double *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) > b.Double;
				}
				else if( addrmode & Register ) {
					if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *(float *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) > pScript->m_Regs[b.UInt64].Float;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *(double *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) > pScript->m_Regs[b.UInt64].Double;
				}
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Immediate ) {
					if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *a.FloatPtr > b.Float;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *a.DoublePtr > b.Double;
				}
				else if( addrmode & Register ) {
					if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *a.FloatPtr > pScript->m_Regs[b.UInt64].Float;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *a.DoublePtr > pScript->m_Regs[b.UInt64].Double;
				}
			}
			continue;
		}
		exec_fcmpr:; {
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Double == b.Double;
			else if( addrmode & Register )
				pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Double == pScript->m_Regs[b.UInt64].Double;
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & FourBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Float == *(float *)(pScript->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & EightBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Double == *(double *)(pScript->m_Regs[b.UInt64].CharPtr+offset);
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & FourBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Float == *b.FloatPtr;
				else if( addrmode & EightBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Double == *b.DoublePtr;
			}
			continue;
		}
		exec_fcmpm:; {
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Immediate ) {
					if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *(float *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) == b.Float;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *(double *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) == b.Double;
				}
				else if( addrmode & Register ) {
					if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *(float *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) == pScript->m_Regs[b.UInt64].Float;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *(double *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) == pScript->m_Regs[b.UInt64].Double;
				}
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Immediate ) {
					if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *a.FloatPtr == b.Float;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *a.DoublePtr == b.Double;
				}
				else if( addrmode & Register ) {
					if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *a.FloatPtr == pScript->m_Regs[b.UInt64].Float;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *a.DoublePtr == pScript->m_Regs[b.UInt64].Double;
				}
			}
			continue;
		}
		exec_fneqr:; {
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Double != b.Double;
			else if( addrmode & Register )
				pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Double != pScript->m_Regs[b.UInt64].Double;
			else if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & FourBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Float != *(float *)(pScript->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & EightBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Double != *(double *)(pScript->m_Regs[b.UInt64].CharPtr+offset);
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & FourBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Float != *b.FloatPtr;
				else if( addrmode & EightBytes )
					pScript->m_bZeroFlag = pScript->m_Regs[a.UInt64].Double != *b.DoublePtr;
			}
			continue;
		}
		exec_fneqm:; {
			a.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pScript->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pScript->m_Regs[rip].Int32Ptr++;
				if( addrmode & Immediate ) {
					if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *(float *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) != b.Float;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *(double *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) != b.Double;
				}
				else if( addrmode & Register ) {
					if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *(float *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) != pScript->m_Regs[b.UInt64].Float;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *(double *)(pScript->m_Regs[a.UInt64].UCharPtr+offset) != pScript->m_Regs[b.UInt64].Double;
				}
			}
			else if( addrmode & IPRelative ) {
				if( addrmode & Immediate ) {
					if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *a.FloatPtr != b.Float;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *a.DoublePtr != b.Double;
				}
				else if( addrmode & Register ) {
					if( addrmode & FourBytes )
						pScript->m_bZeroFlag = *a.FloatPtr != pScript->m_Regs[b.UInt64].Float;
					else if( addrmode & EightBytes )
						pScript->m_bZeroFlag = *a.DoublePtr != pScript->m_Regs[b.UInt64].Double;
				}
			}
			continue;
		}
	}
	return 0;
}
