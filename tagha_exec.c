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
		case rls: return "rls";
		case rms: return "rms";
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
	//if( mode & IPRelative )
	//	strncat(str, "IPRelative|", 150);
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
int32_t Tagha_Exec(struct Tagha *restrict const pSys)
{
	if( !pSys )
		return -1;
	
	// make sure we have a valid instruction address.
	else if( !pSys->m_Regs[rip].UCharPtr ) {
		Tagha_PrintErr(pSys, __func__, "NULL instr ptr! if 'main' doesn't exist, call a function by name.");
		return -1;
	}
	
	// make our necessary local vars.
	fnNative_t	pfNative = NULL;
	bool
		safemode = pSys->m_bSafeMode,
		debugmode = pSys->m_bDebugMode
	;
	uint8_t
		instr=0,
		addrmode=0	// what addressing mode to use
	;
	union CValue
		a,	// 1st operand temporary
		b	// 2nd operand temporary
	;
	
#define X(x) #x ,
	// for debugging purposes.
	const char *restrict opcode2str[] = { INSTR_SET };
#undef X
	
#define X(x) &&exec_##x ,
	// our instruction dispatch table.
	const void *restrict dispatch[] = { INSTR_SET };
#undef X
#undef INSTR_SET
	
	// initialize our stack frame.
	// 'pMainRBP' is to track our initial stack frame.
	uint8_t *pMainRBP = pSys->m_Regs[rbp].UCharPtr;
	pSys->m_Regs[rbp] = pSys->m_Regs[rsp];
	
	int32_t offset;
	while( 1 ) {
		safemode = pSys->m_bSafeMode;
		debugmode = pSys->m_bDebugMode;
		if( safemode ) {
			if( pSys->m_Regs[rip].UCharPtr < pSys->m_pMemory ) {
				Tagha_PrintErr(pSys, __func__, "instruction address out of lower bounds!");
				goto *dispatch[halt];
			}
			else if( pSys->m_Regs[rip].UCharPtr > pSys->m_pTextSegment ) {
				Tagha_PrintErr(pSys, __func__, "instruction address out of upper bounds!");
				goto *dispatch[halt];
			}
			else if( *pSys->m_Regs[rip].UCharPtr > nop ) {
				Tagha_PrintErr(pSys, __func__, "illegal instruction exception! | instruction == \'%" PRIu32 "\'", *pSys->m_Regs[rip].UCharPtr);
				goto *dispatch[halt];
			}
		}
		// reset our temporaries.
		a.UInt64 = b.UInt64 = 0;
		offset = 0;
		
		// fetch opcode and addressing mode.
		//printf("instruction ptr offset: %u\n", pSys->m_Regs[rip].UCharPtr-pSys->m_pMemory);
		instr = *pSys->m_Regs[rip].UCharPtr++;
		addrmode = *pSys->m_Regs[rip].UCharPtr++;
		
		// for debugging.
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
				Tagha_PrintStack(pSys);
				Tagha_PrintData(pSys);
				Tagha_PrintInstrs(pSys);
				Tagha_PrintRegData(pSys);
			}
			return pSys->m_Regs[ras].Int32;
		
		exec_push:; {
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( pSys->m_Regs[rsp].UCharPtr-8 < pSys->m_pStackSegment ) {
				Tagha_PrintErr(pSys, __func__, "Stack Overflow!");
				continue;
			}
			if( addrmode & Immediate )
				(--pSys->m_Regs[rsp].SelfPtr)->UInt64 = a.UInt64;
			else if( addrmode & Register )
				(--pSys->m_Regs[rsp].SelfPtr)->UInt64 = pSys->m_Regs[a.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					(--pSys->m_Regs[rsp].SelfPtr)->UInt64 = 0;
					continue;
				}
				(--pSys->m_Regs[rsp].SelfPtr)->UInt64 = *(uint64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset);
			}
			continue;
		}
		exec_pop:; {
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( pSys->m_Regs[rsp].UCharPtr+8 > (pSys->m_pMemory + (pSys->m_uiMemsize-1)) ) {
				Tagha_PrintErr(pSys, __func__, "Stack Underflow!");
				continue;
			}
			if( addrmode & Register )
				pSys->m_Regs[a.UInt64].UInt64 = (*pSys->m_Regs[rsp].SelfPtr++).UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					pSys->m_Regs[rsp].SelfPtr++;
					continue;
				}
				*(uint64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) = (*pSys->m_Regs[rsp].SelfPtr++).UInt64;
			}
			continue;
		}
		exec_neg:; {
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Register )
				pSys->m_Regs[a.UInt64].UInt64 = -pSys->m_Regs[a.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].CharPtr+offset) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				
				if( addrmode & Byte )
					*(pSys->m_Regs[a.UInt64].CharPtr+offset) = -*(pSys->m_Regs[a.UInt64].CharPtr+offset);
				else if( addrmode & TwoBytes )
					*(int16_t *)(pSys->m_Regs[a.UInt64].CharPtr+offset) = -*(int16_t *)(pSys->m_Regs[a.UInt64].CharPtr+offset);
				else if( addrmode & FourBytes )
					*(int32_t *)(pSys->m_Regs[a.UInt64].CharPtr+offset) = -*(int32_t *)(pSys->m_Regs[a.UInt64].CharPtr+offset);
				else if( addrmode & EightBytes )
					*(int64_t *)(pSys->m_Regs[a.UInt64].CharPtr+offset) = -*(int64_t *)(pSys->m_Regs[a.UInt64].CharPtr+offset);
			}
			continue;
		}
		
		exec_inc:; {
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Register )
				pSys->m_Regs[a.UInt64].UInt64 += 1;
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Byte )
					*(pSys->m_Regs[a.UInt64].UCharPtr+offset) += 1;
				else if( addrmode & TwoBytes )
					*(uint16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) += 1;
				else if( addrmode & FourBytes )
					*(uint32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) += 1;
				else if( addrmode & EightBytes )
					*(uint64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) += 1;
			}
			continue;
		}
		exec_dec:; {
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Register )
				pSys->m_Regs[a.UInt64].UInt64 -= 1;
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Byte )
					*(pSys->m_Regs[a.UInt64].UCharPtr+offset) -= 1;
				else if( addrmode & TwoBytes )
					*(uint16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) -= 1;
				else if( addrmode & FourBytes )
					*(uint32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) -= 1;
				else if( addrmode & EightBytes )
					*(uint64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) -= 1;
			}
			continue;
		}
		exec_bnot:; {
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Register )
				pSys->m_Regs[a.UInt64].UInt64 = ~pSys->m_Regs[a.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Byte )
					*(pSys->m_Regs[a.UInt64].UCharPtr+offset) = ~*(pSys->m_Regs[a.UInt64].UCharPtr+offset);
				else if( addrmode & TwoBytes )
					*(uint16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) = ~*(uint16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset);
				else if( addrmode & FourBytes )
					*(uint32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) = ~*(uint32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset);
				else if( addrmode & EightBytes )
					*(uint64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) = ~*(uint64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset);
			}
			continue;
		}
		
		exec_jmp:; {
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pSys->m_Regs[rip].UCharPtr = pSys->m_pMemory + a.UInt64;
			else if( addrmode & Register )
				pSys->m_Regs[rip].UCharPtr = pSys->m_pMemory + pSys->m_Regs[a.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				pSys->m_Regs[rip].UCharPtr = pSys->m_pMemory + *(uint64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset);
			}
			// cheap, garbage way of stopping infinite loops.
			// probably a better idea to use the stack frame as a form of ID.
			if( *pSys->m_Regs[rip].UCharPtr==jmp )
				--pSys->m_uiMaxInstrs;
			else pSys->m_uiMaxInstrs = LOOP_COUNTER;
			
			if( !pSys->m_uiMaxInstrs ) {
				puts("Tagh Exec :: stopping suspected infinite loop.\n");
				goto *dispatch[halt];
			}
			continue;
		}
		
		exec_jz:; {
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pSys->m_Regs[rip].UCharPtr = (pSys->m_bZeroFlag) ? pSys->m_pMemory + a.UInt64 : pSys->m_Regs[rip].UCharPtr;
			else if( addrmode & Register )
				pSys->m_Regs[rip].UCharPtr = (pSys->m_bZeroFlag) ? pSys->m_pMemory + pSys->m_Regs[a.UInt64].UInt64 : pSys->m_Regs[rip].UCharPtr;
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				pSys->m_Regs[rip].UCharPtr = (pSys->m_bZeroFlag) ? pSys->m_pMemory + *(uint64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) : pSys->m_Regs[rip].UCharPtr;
			}
			continue;
		}
		exec_jnz:; {
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pSys->m_Regs[rip].UCharPtr = (!pSys->m_bZeroFlag) ? pSys->m_pMemory + a.UInt64 : pSys->m_Regs[rip].UCharPtr;
			else if( addrmode & Register )
				pSys->m_Regs[rip].UCharPtr = (!pSys->m_bZeroFlag) ? pSys->m_pMemory + pSys->m_Regs[a.UInt64].UInt64 : pSys->m_Regs[rip].UCharPtr;
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				pSys->m_Regs[rip].UCharPtr = (!pSys->m_bZeroFlag) ? pSys->m_pMemory + *(uint64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) : pSys->m_Regs[rip].UCharPtr;
			}
			continue;
		}
		
		exec_call:; {
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( pSys->m_Regs[rsp].UCharPtr-16 < pSys->m_pStackSegment ) {
				Tagha_PrintErr(pSys, __func__, "Stack Overflow!");
				continue;
			}
			(--pSys->m_Regs[rsp].SelfPtr)->UInt64 = pSys->m_Regs[rip].UInt64;	// push rip
			(--pSys->m_Regs[rsp].SelfPtr)->UInt64 = pSys->m_Regs[rbp].UInt64;	// push rbp
			pSys->m_Regs[rbp] = pSys->m_Regs[rsp];	// mov rbp, rsp
			if( addrmode & Immediate )
				pSys->m_Regs[rip].UCharPtr = pSys->m_pMemory + a.UInt64;
			else if( addrmode & Register )
				pSys->m_Regs[rip].UCharPtr = pSys->m_pMemory + pSys->m_Regs[a.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				pSys->m_Regs[rip].UCharPtr = pSys->m_pMemory + *(uint64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset);
			}
			continue;
		}
		exec_ret:; {
			pSys->m_Regs[rsp] = pSys->m_Regs[rbp];	// mov rsp, rbp
			if( pSys->m_Regs[rsp].UCharPtr+16 > (pSys->m_pMemory + (pSys->m_uiMemsize-1)) ) {
				Tagha_PrintErr(pSys, __func__, "Stack Underflow!");
				continue;
			}
			
			pSys->m_Regs[rbp].UInt64 = (*pSys->m_Regs[rsp].SelfPtr++).UInt64;	// pop rbp
			// if we're popping Main's (or whatever called func's) RBP, then halt the whole program.
			if( pSys->m_Regs[rbp].UCharPtr == pMainRBP ) {
				if( debugmode )
					puts("ret :: return on empty stack");
				goto *dispatch[halt];
			}
			pSys->m_Regs[rip].UInt64 = (*pSys->m_Regs[rsp].SelfPtr++).UInt64;	// pop rip
			if( addrmode & Immediate )
				pSys->m_Regs[rsp].UCharPtr += *pSys->m_Regs[rip].UInt64Ptr++;
			
			continue;
		}
		
		exec_callnat:; {
			if( safemode and !pSys->m_pstrNativeCalls ) {
				Tagha_PrintErr(pSys, __func__, "exec_callnat :: Native Table is NULL!");
				pSys->m_Regs[rip].UInt64Ptr++;
				pSys->m_Regs[rip].UInt32Ptr++;
				continue;
			}
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			uint64_t index;
			
			if( addrmode & Immediate )
				index = a.UInt64;
			else if( addrmode & Register )
				index = pSys->m_Regs[a.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					pSys->m_Regs[rip].UInt32Ptr++;
					continue;
				}
				index = *(uint64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset);
			}
			else index = -1;
			
			if( safemode and index >= pSys->m_uiNatives  ) {
				Tagha_PrintErr(pSys, __func__, "exec_callnat :: Native Index \'%" PRIu64 "\' is out of bounds!", index);
				pSys->m_Regs[rip].UInt32Ptr++;
				continue;
			}
			if( debugmode )
				printf("exec_callnat: index: %" PRIu64 " == \'%s\' @ %p.\n", index, pSys->m_pstrNativeCalls[index], pSys->m_pstrNativeCalls+index);
			
			pfNative = (fnNative_t)(uintptr_t) Map_Get(pSys->m_pmapNatives, pSys->m_pstrNativeCalls[index]);
			if( safemode and !pfNative ) {
				Tagha_PrintErr(pSys, __func__, "exec_callnat :: native \'%s\' not registered!", pSys->m_pstrNativeCalls[index]);
				pSys->m_Regs[rip].UInt32Ptr++;
				continue;
			}
			// how many arguments pushed for native to use.
			const uint32_t argcount = *pSys->m_Regs[rip].UInt32Ptr++;
			if( debugmode )
				printf("callnat: Calling func addr: %p with %" PRIu32 " args pushed.\n", pfNative, argcount);
			
			// ERROR: you can't initialize an array using a variable as size.
			// have no choice but to use memset.
			union CValue params[argcount]; memset(params, 0, sizeof(union CValue)*argcount);
			
			// we have 10 registers that are usable as function params for natives.
			// if all 10 are exhausted, dump rest of params to the stack.
			// if 10 or less params, dump from registers.
			if( argcount<=10 ) {
				memcpy(params, pSys->m_Regs+rds, sizeof(union CValue)*argcount);
			}
			// if more than 10 params, get from registers and then stack.
			else if( argcount>10 ) {
				memcpy(params, pSys->m_Regs+rds, sizeof(union CValue)*10);
				memcpy(params+10, pSys->m_Regs[rsp].SelfPtr, sizeof(union CValue)*(argcount-10));
				pSys->m_Regs[rsp].SelfPtr += (argcount-10);
			}
			printf("exec_callnat :: calling C function '%s'.\n", pSys->m_pstrNativeCalls[index]);
			pSys->m_Regs[ras].UInt64 = 0;
			(*pfNative)(pSys, params, pSys->m_Regs+ras, argcount);
			continue;
		}
		
		exec_movr:; {	// dest is a reg, src is reg imm or memory
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pSys->m_Regs[a.UInt64].UInt64 = b.UInt64;
			else if( addrmode & Register )
				pSys->m_Regs[a.UInt64].UInt64 = pSys->m_Regs[b.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[b.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					pSys->m_Regs[a.UInt64].UInt64 = 0;
					continue;
				}
				if( addrmode & Byte )
					pSys->m_Regs[a.UInt64].UChar = *(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & TwoBytes )
					pSys->m_Regs[a.UInt64].UShort = *(uint16_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & FourBytes )
					pSys->m_Regs[a.UInt64].UInt32 = *(uint32_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & EightBytes )
					pSys->m_Regs[a.UInt64].UInt64 = *(uint64_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset);
			}
			continue;
		}
		exec_movm:; {	// dest is memory and src is reg/imm/mem
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {	// moving value to register-based address
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Immediate ) { // value is immediate constant
					if( addrmode & Byte )
						*(pSys->m_Regs[a.UInt64].UCharPtr+offset) = b.UChar;
					else if( addrmode & TwoBytes )
						*(uint16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) = b.UShort;
					else if( addrmode & FourBytes )
						*(uint32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) = b.UInt32;
					else if( addrmode & EightBytes )
						*(uint64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) = b.UInt64;
				}
				else if( addrmode & Register ) { // value is in register
					if( addrmode & Byte )
						*(pSys->m_Regs[a.UInt64].UCharPtr+offset) = pSys->m_Regs[b.UInt64].UChar;
					else if( addrmode & TwoBytes )
						*(uint16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) = pSys->m_Regs[b.UInt64].UShort;
					else if( addrmode & FourBytes )
						*(uint32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) = pSys->m_Regs[b.UInt64].UInt32;
					else if( addrmode & EightBytes )
						*(uint64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) = pSys->m_Regs[b.UInt64].UInt64;
				}
			}
			continue;
		}
		exec_lea:; {	// load effective address
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Register )
				pSys->m_Regs[a.UInt64].UInt64 = pSys->m_Regs[b.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[b.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					pSys->m_Regs[a.UInt64].UInt64 = 0;
					continue;
				}
				pSys->m_Regs[a.UInt64].UInt64 = pSys->m_Regs[b.UInt64].UInt64+offset;
			}
			else if( addrmode & Immediate )
				pSys->m_Regs[a.UInt64].UInt64 = b.UInt64;
			continue;
		}
		/*
		exec_leo:; {	// load effective offset - loads a memory offset.
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pSys->m_Regs[a.UInt64].UInt64 = (uintptr_t)(pSys->m_pMemory + b.UInt64);
			else if( addrmode & Register )
				pSys->m_Regs[a.UInt64].UInt64 = (uintptr_t)(pSys->m_pMemory + pSys->m_Regs[b.UInt64].UInt64);
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[b.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					pSys->m_Regs[a.UInt64].UInt64 = 0;
					continue;
				}
				pSys->m_Regs[a.UInt64].UInt64 = (uintptr_t)(pSys->m_pMemory + pSys->m_Regs[b.UInt64].UInt64 + offset);
			}
			continue;
		}
		*/
		exec_addr:; {	// dest is reg, src is reg/memory/imm
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pSys->m_Regs[a.UInt64].Int64 += b.Int64;
			else if( addrmode & Register )
				pSys->m_Regs[a.UInt64].Int64 += pSys->m_Regs[b.UInt64].Int64;
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[b.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Byte )
					pSys->m_Regs[a.UInt64].Char += *(pSys->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & TwoBytes )
					pSys->m_Regs[a.UInt64].Short += *(int16_t *)(pSys->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & FourBytes )
					pSys->m_Regs[a.UInt64].Int32 += *(int32_t *)(pSys->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & EightBytes )
					pSys->m_Regs[a.UInt64].Int64 += *(int64_t *)(pSys->m_Regs[b.UInt64].CharPtr+offset);
			}
			continue;
		}
		exec_addm:; {	// dest is memory and src is reg/imm
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						*(pSys->m_Regs[a.UInt64].CharPtr+offset) += b.Char;
					else if( addrmode & TwoBytes )
						*(int16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) += b.Short;
					else if( addrmode & FourBytes )
						*(int32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) += b.Int32;
					else if( addrmode & EightBytes )
						*(int64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) += b.Int64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						*(pSys->m_Regs[a.UInt64].CharPtr+offset) += pSys->m_Regs[b.UInt64].Char;
					else if( addrmode & TwoBytes )
						*(int16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) += pSys->m_Regs[b.UInt64].Short;
					else if( addrmode & FourBytes )
						*(int32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) += pSys->m_Regs[b.UInt64].Int32;
					else if( addrmode & EightBytes )
						*(int64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) += pSys->m_Regs[b.UInt64].Int64;
				}
			}
			continue;
		}
		exec_uaddr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pSys->m_Regs[a.UInt64].UInt64 += b.UInt64;
			else if( addrmode & Register )
				pSys->m_Regs[a.UInt64].UInt64 += pSys->m_Regs[b.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[b.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Byte )
					pSys->m_Regs[a.UInt64].UChar += *(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & TwoBytes )
					pSys->m_Regs[a.UInt64].UShort += *(uint16_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & FourBytes )
					pSys->m_Regs[a.UInt64].UInt32 += *(uint32_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & EightBytes )
					pSys->m_Regs[a.UInt64].UInt64 += *(uint64_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset);
			}
			continue;
		}
		exec_uaddm:; {	// dest is memory and src is reg
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						*(pSys->m_Regs[a.UInt64].UCharPtr+offset) += b.UChar;
					else if( addrmode & TwoBytes )
						*(uint16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) += b.UShort;
					else if( addrmode & FourBytes )
						*(uint32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) += b.UInt32;
					else if( addrmode & EightBytes )
						*(uint64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) += b.UInt64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						*(pSys->m_Regs[a.UInt64].UCharPtr+offset) += pSys->m_Regs[b.UInt64].UChar;
					else if( addrmode & TwoBytes )
						*(uint16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) += pSys->m_Regs[b.UInt64].UShort;
					else if( addrmode & FourBytes )
						*(uint32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) += pSys->m_Regs[b.UInt64].UInt32;
					else if( addrmode & EightBytes )
						*(uint64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) += pSys->m_Regs[b.UInt64].UInt64;
				}
			}
			continue;
		}
		
		exec_subr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pSys->m_Regs[a.UInt64].Int64 -= b.Int64;
			else if( addrmode & Register )
				pSys->m_Regs[a.UInt64].Int64 -= pSys->m_Regs[b.UInt64].Int64;
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[b.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Byte )
					pSys->m_Regs[a.UInt64].Char -= *(pSys->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & TwoBytes )
					pSys->m_Regs[a.UInt64].Short -= *(int16_t *)(pSys->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & FourBytes )
					pSys->m_Regs[a.UInt64].Int32 -= *(int32_t *)(pSys->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & EightBytes )
					pSys->m_Regs[a.UInt64].Int64 -= *(int64_t *)(pSys->m_Regs[b.UInt64].CharPtr+offset);
			}
			continue;
		}
		exec_subm:; {	// dest is memory and src is reg
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						*(pSys->m_Regs[a.UInt64].CharPtr+offset) -= b.Char;
					else if( addrmode & TwoBytes )
						*(int16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) -= b.Short;
					else if( addrmode & FourBytes )
						*(int32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) -= b.Int32;
					else if( addrmode & EightBytes )
						*(int64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) -= b.Int64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						*(pSys->m_Regs[a.UInt64].CharPtr+offset) -= pSys->m_Regs[b.UInt64].Char;
					else if( addrmode & TwoBytes )
						*(int16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) -= pSys->m_Regs[b.UInt64].Short;
					else if( addrmode & FourBytes )
						*(int32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) -= pSys->m_Regs[b.UInt64].Int32;
					else if( addrmode & EightBytes )
						*(int64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) -= pSys->m_Regs[b.UInt64].Int64;
				}
			}
			continue;
		}
		exec_usubr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pSys->m_Regs[a.UInt64].UInt64 -= b.UInt64;
			else if( addrmode & Register )
				pSys->m_Regs[a.UInt64].UInt64 -= pSys->m_Regs[b.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[b.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Byte )
					pSys->m_Regs[a.UInt64].UChar -= *(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & TwoBytes )
					pSys->m_Regs[a.UInt64].UShort -= *(uint16_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & FourBytes )
					pSys->m_Regs[a.UInt64].UInt32 -= *(uint32_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & EightBytes )
					pSys->m_Regs[a.UInt64].UInt64 -= *(uint64_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset);
			}
			continue;
		}
		exec_usubm:; {	// dest is memory and src is reg
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						*(pSys->m_Regs[a.UInt64].UCharPtr+offset) -= b.UChar;
					else if( addrmode & TwoBytes )
						*(uint16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) -= b.UShort;
					else if( addrmode & FourBytes )
						*(uint32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) -= b.UInt32;
					else if( addrmode & EightBytes )
						*(uint64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) -= b.UInt64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						*(pSys->m_Regs[a.UInt64].UCharPtr+offset) -= pSys->m_Regs[b.UInt64].UChar;
					else if( addrmode & TwoBytes )
						*(uint16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) -= pSys->m_Regs[b.UInt64].UShort;
					else if( addrmode & FourBytes )
						*(uint32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) -= pSys->m_Regs[b.UInt64].UInt32;
					else if( addrmode & EightBytes )
						*(uint64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) -= pSys->m_Regs[b.UInt64].UInt64;
				}
			}
			continue;
		}
		
		exec_mulr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pSys->m_Regs[a.UInt64].Int64 *= b.Int64;
			else if( addrmode & Register )
				pSys->m_Regs[a.UInt64].Int64 *= pSys->m_Regs[b.UInt64].Int64;
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[b.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Byte )
					pSys->m_Regs[a.UInt64].Char *= *(pSys->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & TwoBytes )
					pSys->m_Regs[a.UInt64].Short *= *(int16_t *)(pSys->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & FourBytes )
					pSys->m_Regs[a.UInt64].Int32 *= *(int32_t *)(pSys->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & EightBytes )
					pSys->m_Regs[a.UInt64].Int64 *= *(int64_t *)(pSys->m_Regs[b.UInt64].CharPtr+offset);
			}
			continue;
		}
		exec_mulm:; {	// dest is memory and src is reg
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						*(pSys->m_Regs[a.UInt64].CharPtr+offset) *= b.Char;
					else if( addrmode & TwoBytes )
						*(int16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) *= b.Short;
					else if( addrmode & FourBytes )
						*(int32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) *= b.Int32;
					else if( addrmode & EightBytes )
						*(int64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) *= b.Int64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						*(pSys->m_Regs[a.UInt64].CharPtr+offset) *= pSys->m_Regs[b.UInt64].Char;
					else if( addrmode & TwoBytes )
						*(int16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) *= pSys->m_Regs[b.UInt64].Short;
					else if( addrmode & FourBytes )
						*(int32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) *= pSys->m_Regs[b.UInt64].Int32;
					else if( addrmode & EightBytes )
						*(int64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) *= pSys->m_Regs[b.UInt64].Int64;
				}
			}
			continue;
		}
		exec_umulr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pSys->m_Regs[a.UInt64].UInt64 *= b.UInt64;
			else if( addrmode & Register )
				pSys->m_Regs[a.UInt64].UInt64 *= pSys->m_Regs[b.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[b.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Byte )
					pSys->m_Regs[a.UInt64].UChar *= *(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & TwoBytes )
					pSys->m_Regs[a.UInt64].UShort *= *(uint16_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & FourBytes )
					pSys->m_Regs[a.UInt64].UInt32 *= *(uint32_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & EightBytes )
					pSys->m_Regs[a.UInt64].UInt64 *= *(uint64_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset);
			}
			continue;
		}
		exec_umulm:; {	// dest is memory and src is reg
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						*(pSys->m_Regs[a.UInt64].UCharPtr+offset) *= b.UChar;
					else if( addrmode & TwoBytes )
						*(uint16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) *= b.UShort;
					else if( addrmode & FourBytes )
						*(uint32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) *= b.UInt32;
					else if( addrmode & EightBytes )
						*(uint64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) *= b.UInt64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						*(pSys->m_Regs[a.UInt64].UCharPtr+offset) *= pSys->m_Regs[b.UInt64].UChar;
					else if( addrmode & TwoBytes )
						*(uint16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) *= pSys->m_Regs[b.UInt64].UShort;
					else if( addrmode & FourBytes )
						*(uint32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) *= pSys->m_Regs[b.UInt64].UInt32;
					else if( addrmode & EightBytes )
						*(uint64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) *= pSys->m_Regs[b.UInt64].UInt64;
				}
			}
			continue;
		}
		
		exec_divr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate ) {
				if( !b.Int64 )
					b.Int64 = 1;
				pSys->m_Regs[a.UInt64].Int64 /= b.Int64;
			}
			else if( addrmode & Register ) {
				if( !pSys->m_Regs[b.UInt64].Int64 )
					pSys->m_Regs[b.UInt64].Int64 = 1;
				pSys->m_Regs[a.UInt64].Int64 /= pSys->m_Regs[b.UInt64].Int64;
			}
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[b.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Byte ) {
					if( !*(pSys->m_Regs[b.UInt64].CharPtr+offset) )
						*(pSys->m_Regs[b.UInt64].CharPtr+offset) = 1;
					pSys->m_Regs[a.UInt64].Char /= *(pSys->m_Regs[b.UInt64].CharPtr+offset);
				}
				else if( addrmode & TwoBytes ) {
					if( !*(int16_t *)(pSys->m_Regs[b.UInt64].CharPtr+offset) )
						*(int16_t *)(pSys->m_Regs[b.UInt64].CharPtr+offset) = 1;
					pSys->m_Regs[a.UInt64].Short /= *(int16_t *)(pSys->m_Regs[b.UInt64].CharPtr+offset);
				}
				else if( addrmode & FourBytes ) {
					if( !*(int32_t *)(pSys->m_Regs[b.UInt64].CharPtr+offset) )
						*(int32_t *)(pSys->m_Regs[b.UInt64].CharPtr+offset) = 1;
					pSys->m_Regs[a.UInt64].Int32 /= *(int32_t *)(pSys->m_Regs[b.UInt64].CharPtr+offset);
				}
				else if( addrmode & EightBytes ) {
					if( !*(int64_t *)(pSys->m_Regs[b.UInt64].CharPtr+offset) )
						*(int64_t *)(pSys->m_Regs[b.UInt64].CharPtr+offset) = 1;
					pSys->m_Regs[a.UInt64].Int64 /= *(int64_t *)(pSys->m_Regs[b.UInt64].CharPtr+offset);
				}
			}
			continue;
		}
		exec_divm:; {	// dest is memory and src is reg
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Immediate ) {
					if( !b.UInt64 )
						b.UInt64 = 1;
					
					if( addrmode & Byte )
						*(pSys->m_Regs[a.UInt64].CharPtr+offset) /= b.Char;
					else if( addrmode & TwoBytes )
						*(int16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) /= b.Short;
					else if( addrmode & FourBytes )
						*(int32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) /= b.Int32;
					else if( addrmode & EightBytes )
						*(int64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) /= b.Int64;
				}
				else if( addrmode & Register ) {
					if( !pSys->m_Regs[b.UInt64].UInt64 )
						pSys->m_Regs[b.UInt64].UInt64 = 1;
					
					if( addrmode & Byte )
						*(pSys->m_Regs[a.UInt64].CharPtr+offset) /= pSys->m_Regs[b.UInt64].Char;
					else if( addrmode & TwoBytes )
						*(int16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) /= pSys->m_Regs[b.UInt64].Short;
					else if( addrmode & FourBytes )
						*(int32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) /= pSys->m_Regs[b.UInt64].Int32;
					else if( addrmode & EightBytes )
						*(int64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) /= pSys->m_Regs[b.UInt64].Int64;
				}
			}
			continue;
		}
		exec_udivr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate ) {
				if( !b.UInt64 )
					b.UInt64 = 1;
				pSys->m_Regs[a.UInt64].UInt64 /= b.UInt64;
			}
			else if( addrmode & Register ) {
				if( !pSys->m_Regs[b.UInt64].UInt64 )
					pSys->m_Regs[b.UInt64].UInt64 = 1;
				pSys->m_Regs[a.UInt64].UInt64 /= pSys->m_Regs[b.UInt64].UInt64;
			}
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[b.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Byte ) {
					if( !*(pSys->m_Regs[b.UInt64].UCharPtr+offset) )
						*(pSys->m_Regs[b.UInt64].UCharPtr+offset) = 1;
					pSys->m_Regs[a.UInt64].UChar /= *(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				}
				else if( addrmode & TwoBytes ) {
					if( !*(uint16_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset) )
						*(uint16_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset) = 1;
					pSys->m_Regs[a.UInt64].UShort /= *(uint16_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				}
				else if( addrmode & FourBytes ) {
					if( !*(uint32_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset) )
						*(uint32_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset) = 1;
					pSys->m_Regs[a.UInt64].UInt32 /= *(uint32_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				}
				else if( addrmode & EightBytes ) {
					if( !*(uint64_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset) )
						*(uint64_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset) = 1;
					pSys->m_Regs[a.UInt64].UInt64 /= *(uint64_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				}
			}
			continue;
		}
		exec_udivm:; {	// dest is memory and src is reg
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Immediate ) {
					if( !b.UInt64 )
						b.UInt64 = 1;
					
					if( addrmode & Byte )
						*(pSys->m_Regs[a.UInt64].UCharPtr+offset) /= b.UChar;
					else if( addrmode & TwoBytes )
						*(uint16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) /= b.UShort;
					else if( addrmode & FourBytes )
						*(uint32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) /= b.UInt32;
					else if( addrmode & EightBytes )
						*(uint64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) /= b.UInt64;
				}
				else if( addrmode & Register ) {
					if( !pSys->m_Regs[b.UInt64].UInt64 )
						pSys->m_Regs[b.UInt64].UInt64 = 1;
					
					if( addrmode & Byte )
						*(pSys->m_Regs[a.UInt64].UCharPtr+offset) /= pSys->m_Regs[b.UInt64].UChar;
					else if( addrmode & TwoBytes )
						*(uint16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) /= pSys->m_Regs[b.UInt64].UShort;
					else if( addrmode & FourBytes )
						*(uint32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) /= pSys->m_Regs[b.UInt64].UInt32;
					else if( addrmode & EightBytes )
						*(uint64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) /= pSys->m_Regs[b.UInt64].UInt64;
				}
			}
			continue;
		}
		
		exec_modr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate ) {
				if( !b.Int64 )
					b.Int64 = 1;
				pSys->m_Regs[a.UInt64].Int64 %= b.Int64;
			}
			if( addrmode & Register ) {
				if( !pSys->m_Regs[b.UInt64].Int64 )
					pSys->m_Regs[b.UInt64].Int64 = 1;
				pSys->m_Regs[a.UInt64].Int64 %= pSys->m_Regs[b.UInt64].Int64;
			}
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[b.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Byte ) {
					if( !*(pSys->m_Regs[b.UInt64].CharPtr+offset) )
						*(pSys->m_Regs[b.UInt64].CharPtr+offset) = 1;
					pSys->m_Regs[a.UInt64].Char %= *(pSys->m_Regs[b.UInt64].CharPtr+offset);
				}
				else if( addrmode & TwoBytes ) {
					if( !*(int16_t *)(pSys->m_Regs[b.UInt64].CharPtr+offset) )
						*(int16_t *)(pSys->m_Regs[b.UInt64].CharPtr+offset) = 1;
					pSys->m_Regs[a.UInt64].Short %= *(int16_t *)(pSys->m_Regs[b.UInt64].CharPtr+offset);
				}
				else if( addrmode & FourBytes ) {
					if( !*(int32_t *)(pSys->m_Regs[b.UInt64].CharPtr+offset) )
						*(int32_t *)(pSys->m_Regs[b.UInt64].CharPtr+offset) = 1;
					pSys->m_Regs[a.UInt64].Int32 %= *(int32_t *)(pSys->m_Regs[b.UInt64].CharPtr+offset);
				}
				else if( addrmode & EightBytes ) {
					if( !*(int64_t *)(pSys->m_Regs[b.UInt64].CharPtr+offset) )
						*(int64_t *)(pSys->m_Regs[b.UInt64].CharPtr+offset) = 1;
					pSys->m_Regs[a.UInt64].Int64 %= *(int64_t *)(pSys->m_Regs[b.UInt64].CharPtr+offset);
				}
			}
			continue;
		}
		exec_modm:; {	// dest is memory and src is reg
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Immediate ) {
					if( !b.UInt64 )
						b.UInt64 = 1;
					
					if( addrmode & Byte )
						*(pSys->m_Regs[a.UInt64].CharPtr+offset) %= b.Char;
					else if( addrmode & TwoBytes )
						*(int16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) %= b.Short;
					else if( addrmode & FourBytes )
						*(int32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) %= b.Int32;
					else if( addrmode & EightBytes )
						*(int64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) %= b.Int64;
				}
				else if( addrmode & Register ) {
					if( !pSys->m_Regs[b.UInt64].UInt64 )
						pSys->m_Regs[b.UInt64].UInt64 = 1;
					
					if( addrmode & Byte )
						*(pSys->m_Regs[a.UInt64].CharPtr+offset) %= pSys->m_Regs[b.UInt64].Char;
					else if( addrmode & TwoBytes )
						*(int16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) %= pSys->m_Regs[b.UInt64].Short;
					else if( addrmode & FourBytes )
						*(int32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) %= pSys->m_Regs[b.UInt64].Int32;
					else if( addrmode & EightBytes )
						*(int64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) %= pSys->m_Regs[b.UInt64].Int64;
				}
			}
			continue;
		}
		exec_umodr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate ) {
				if( !b.UInt64 )
					b.UInt64 = 1;
				pSys->m_Regs[a.UInt64].UInt64 %= b.UInt64;
			}
			else if( addrmode & Register ) {
				if( !pSys->m_Regs[b.UInt64].UInt64 )
					pSys->m_Regs[b.UInt64].UInt64 = 1;
				pSys->m_Regs[a.UInt64].UInt64 %= pSys->m_Regs[b.UInt64].UInt64;
			}
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[b.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Byte ) {
					if( !*(pSys->m_Regs[b.UInt64].UCharPtr+offset) )
						*(pSys->m_Regs[b.UInt64].UCharPtr+offset) = 1;
					pSys->m_Regs[a.UInt64].UChar %= *(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				}
				else if( addrmode & TwoBytes ) {
					if( !*(uint16_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset) )
						*(uint16_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset) = 1;
					pSys->m_Regs[a.UInt64].UShort %= *(uint16_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				}
				else if( addrmode & FourBytes ) {
					if( !*(uint32_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset) )
						*(uint32_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset) = 1;
					pSys->m_Regs[a.UInt64].UInt32 %= *(uint32_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				}
				else if( addrmode & EightBytes ) {
					if( !*(uint64_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset) )
						*(uint64_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset) = 1;
					pSys->m_Regs[a.UInt64].UInt64 %= *(uint64_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				}
			}
			continue;
		}
		exec_umodm:; {	// dest is memory and src is reg
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Immediate ) {
					if( !b.UInt64 )
						b.UInt64 = 1;
					
					if( addrmode & Byte )
						*(pSys->m_Regs[a.UInt64].UCharPtr+offset) %= b.UChar;
					else if( addrmode & TwoBytes )
						*(uint16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) %= b.UShort;
					else if( addrmode & FourBytes )
						*(uint32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) %= b.UInt32;
					else if( addrmode & EightBytes )
						*(uint64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) %= b.UInt64;
				}
				else if( addrmode & Register ) {
					if( !pSys->m_Regs[b.UInt64].UInt64 )
						pSys->m_Regs[b.UInt64].UInt64 = 1;
					
					if( addrmode & Byte )
						*(pSys->m_Regs[a.UInt64].UCharPtr+offset) %= pSys->m_Regs[b.UInt64].UChar;
					else if( addrmode & TwoBytes )
						*(uint16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) %= pSys->m_Regs[b.UInt64].UShort;
					else if( addrmode & FourBytes )
						*(uint32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) %= pSys->m_Regs[b.UInt64].UInt32;
					else if( addrmode & EightBytes )
						*(uint64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) %= pSys->m_Regs[b.UInt64].UInt64;
				}
			}
			continue;
		}
		
		exec_shrr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pSys->m_Regs[a.UInt64].UInt64 >>= b.UInt64;
			else if( addrmode & Register )
				pSys->m_Regs[a.UInt64].UInt64 >>= pSys->m_Regs[b.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[b.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Byte )
					pSys->m_Regs[a.UInt64].UChar >>= *(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & TwoBytes )
					pSys->m_Regs[a.UInt64].UShort >>= *(uint16_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & FourBytes )
					pSys->m_Regs[a.UInt64].UInt32 >>= *(uint32_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & EightBytes )
					pSys->m_Regs[a.UInt64].UInt64 >>= *(uint64_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset);
			}
			continue;
		}
		exec_shrm:; {	// dest is memory and src is reg
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						*(pSys->m_Regs[a.UInt64].UCharPtr+offset) >>= b.UChar;
					else if( addrmode & TwoBytes )
						*(uint16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) >>= b.UShort;
					else if( addrmode & FourBytes )
						*(uint32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) >>= b.UInt32;
					else if( addrmode & EightBytes )
						*(uint64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) >>= b.UInt64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						*(pSys->m_Regs[a.UInt64].UCharPtr+offset) >>= pSys->m_Regs[b.UInt64].UChar;
					else if( addrmode & TwoBytes )
						*(uint16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) >>= pSys->m_Regs[b.UInt64].UShort;
					else if( addrmode & FourBytes )
						*(uint32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) >>= pSys->m_Regs[b.UInt64].UInt32;
					else if( addrmode & EightBytes )
						*(uint64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) >>= pSys->m_Regs[b.UInt64].UInt64;
				}
			}
			continue;
		}
		
		exec_shlr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pSys->m_Regs[a.UInt64].UInt64 <<= b.UInt64;
			else if( addrmode & Register )
				pSys->m_Regs[a.UInt64].UInt64 <<= pSys->m_Regs[b.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[b.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Byte )
					pSys->m_Regs[a.UInt64].UChar <<= *(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & TwoBytes )
					pSys->m_Regs[a.UInt64].UShort <<= *(uint16_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & FourBytes )
					pSys->m_Regs[a.UInt64].UInt32 <<= *(uint32_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & EightBytes )
					pSys->m_Regs[a.UInt64].UInt64 <<= *(uint64_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset);
			}
			continue;
		}
		exec_shlm:; {	// dest is memory and src is reg
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						*(pSys->m_Regs[a.UInt64].UCharPtr+offset) <<= b.UChar;
					else if( addrmode & TwoBytes )
						*(uint16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) <<= b.UShort;
					else if( addrmode & FourBytes )
						*(uint32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) <<= b.UInt32;
					else if( addrmode & EightBytes )
						*(uint64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) <<= b.UInt64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						*(pSys->m_Regs[a.UInt64].UCharPtr+offset) <<= pSys->m_Regs[b.UInt64].UChar;
					else if( addrmode & TwoBytes )
						*(uint16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) <<= pSys->m_Regs[b.UInt64].UShort;
					else if( addrmode & FourBytes )
						*(uint32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) <<= pSys->m_Regs[b.UInt64].UInt32;
					else if( addrmode & EightBytes )
						*(uint64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) <<= pSys->m_Regs[b.UInt64].UInt64;
				}
			}
			continue;
		}
		
		exec_andr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pSys->m_Regs[a.UInt64].UInt64 &= b.UInt64;
			else if( addrmode & Register )
				pSys->m_Regs[a.UInt64].UInt64 &= pSys->m_Regs[b.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[b.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Byte )
					pSys->m_Regs[a.UInt64].UChar &= *(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & TwoBytes )
					pSys->m_Regs[a.UInt64].UShort &= *(uint16_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & FourBytes )
					pSys->m_Regs[a.UInt64].UInt32 &= *(uint32_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & EightBytes )
					pSys->m_Regs[a.UInt64].UInt64 &= *(uint64_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset);
			}
			continue;
		}
		exec_andm:; {	// dest is memory and src is reg
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						*(pSys->m_Regs[a.UInt64].UCharPtr+offset) &= b.UChar;
					else if( addrmode & TwoBytes )
						*(uint16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) &= b.UShort;
					else if( addrmode & FourBytes )
						*(uint32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) &= b.UInt32;
					else if( addrmode & EightBytes )
						*(uint64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) &= b.UInt64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						*(pSys->m_Regs[a.UInt64].UCharPtr+offset) &= pSys->m_Regs[b.UInt64].UChar;
					else if( addrmode & TwoBytes )
						*(uint16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) &= pSys->m_Regs[b.UInt64].UShort;
					else if( addrmode & FourBytes )
						*(uint32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) &= pSys->m_Regs[b.UInt64].UInt32;
					else if( addrmode & EightBytes )
						*(uint64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) &= pSys->m_Regs[b.UInt64].UInt64;
				}
			}
			continue;
		}
		
		exec_orr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pSys->m_Regs[a.UInt64].UInt64 |= b.UInt64;
			else if( addrmode & Register )
				pSys->m_Regs[a.UInt64].UInt64 |= pSys->m_Regs[b.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[b.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Byte )
					pSys->m_Regs[a.UInt64].UChar |= *(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & TwoBytes )
					pSys->m_Regs[a.UInt64].UShort |= *(uint16_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & FourBytes )
					pSys->m_Regs[a.UInt64].UInt32 |= *(uint32_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & EightBytes )
					pSys->m_Regs[a.UInt64].UInt64 |= *(uint64_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset);
			}
			continue;
		}
		exec_orm:; {	// dest is memory and src is reg
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						*(pSys->m_Regs[a.UInt64].UCharPtr+offset) |= b.UChar;
					else if( addrmode & TwoBytes )
						*(uint16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) |= b.UShort;
					else if( addrmode & FourBytes )
						*(uint32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) |= b.UInt32;
					else if( addrmode & EightBytes )
						*(uint64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) |= b.UInt64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						*(pSys->m_Regs[a.UInt64].UCharPtr+offset) |= pSys->m_Regs[b.UInt64].UChar;
					else if( addrmode & TwoBytes )
						*(uint16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) |= pSys->m_Regs[b.UInt64].UShort;
					else if( addrmode & FourBytes )
						*(uint32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) |= pSys->m_Regs[b.UInt64].UInt32;
					else if( addrmode & EightBytes )
						*(uint64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) |= pSys->m_Regs[b.UInt64].UInt64;
				}
			}
			continue;
		}
		
		exec_xorr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pSys->m_Regs[a.UInt64].UInt64 ^= b.UInt64;
			else if( addrmode & Register )
				pSys->m_Regs[a.UInt64].UInt64 ^= pSys->m_Regs[b.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[b.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Byte )
					pSys->m_Regs[a.UInt64].UChar ^= *(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & TwoBytes )
					pSys->m_Regs[a.UInt64].UShort ^= *(uint16_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & FourBytes )
					pSys->m_Regs[a.UInt64].UInt32 ^= *(uint32_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & EightBytes )
					pSys->m_Regs[a.UInt64].UInt64 ^= *(uint64_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset);
			}
			continue;
		}
		exec_xorm:; {	// dest is memory and src is reg
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						*(pSys->m_Regs[a.UInt64].UCharPtr+offset) ^= b.UChar;
					else if( addrmode & TwoBytes )
						*(uint16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) ^= b.UShort;
					else if( addrmode & FourBytes )
						*(uint32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) ^= b.UInt32;
					else if( addrmode & EightBytes )
						*(uint64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) ^= b.UInt64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						*(pSys->m_Regs[a.UInt64].UCharPtr+offset) ^= pSys->m_Regs[b.UInt64].UChar;
					else if( addrmode & TwoBytes )
						*(uint16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) ^= pSys->m_Regs[b.UInt64].UShort;
					else if( addrmode & FourBytes )
						*(uint32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) ^= pSys->m_Regs[b.UInt64].UInt32;
					else if( addrmode & EightBytes )
						*(uint64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) ^= pSys->m_Regs[b.UInt64].UInt64;
				}
			}
			continue;
		}
		
		exec_ltr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].Int64 < b.Int64;
			else if( addrmode & Register )
				pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].Int64 < pSys->m_Regs[b.UInt64].Int64;
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[b.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Byte )
					pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].Char < *(pSys->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & TwoBytes )
					pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].Short < *(int16_t *)(pSys->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & FourBytes )
					pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].Int32 < *(int32_t *)(pSys->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & EightBytes )
					pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].Int64 < *(int64_t *)(pSys->m_Regs[b.UInt64].CharPtr+offset);
			}
			continue;
		}
		exec_ltm:; {	// dest is memory and src is reg
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						pSys->m_bZeroFlag = *(pSys->m_Regs[a.UInt64].CharPtr+offset) < b.Char;
					else if( addrmode & TwoBytes )
						pSys->m_bZeroFlag = *(int16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) < b.Short;
					else if( addrmode & FourBytes )
						pSys->m_bZeroFlag = *(int32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) < b.Int32;
					else if( addrmode & EightBytes )
						pSys->m_bZeroFlag = *(int64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) < b.Int64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						pSys->m_bZeroFlag = *(pSys->m_Regs[a.UInt64].UCharPtr+offset) < pSys->m_Regs[b.UInt64].Char;
					else if( addrmode & TwoBytes )
						pSys->m_bZeroFlag = *(int16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) < pSys->m_Regs[b.UInt64].Short;
					else if( addrmode & FourBytes )
						pSys->m_bZeroFlag = *(int32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) < pSys->m_Regs[b.UInt64].Int32;
					else if( addrmode & EightBytes )
						pSys->m_bZeroFlag = *(int64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) < pSys->m_Regs[b.UInt64].Int64;
				}
			}
			continue;
		}
		exec_ultr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].UInt64 < b.UInt64;
			else if( addrmode & Register )
				pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].UInt64 < pSys->m_Regs[b.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[b.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Byte )
					pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].UChar < *(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & TwoBytes )
					pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].UShort < *(uint16_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & FourBytes )
					pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].UInt32 < *(uint32_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & EightBytes )
					pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].UInt64 < *(uint64_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset);
			}
			continue;
		}
		exec_ultm:; {	// dest is memory and src is reg
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						pSys->m_bZeroFlag = *(pSys->m_Regs[a.UInt64].UCharPtr+offset) < b.UChar;
					else if( addrmode & TwoBytes )
						pSys->m_bZeroFlag = *(uint16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) < b.UShort;
					else if( addrmode & FourBytes )
						pSys->m_bZeroFlag = *(uint32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) < b.UInt32;
					else if( addrmode & EightBytes )
						pSys->m_bZeroFlag = *(uint64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) < b.UInt64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						pSys->m_bZeroFlag = *(pSys->m_Regs[a.UInt64].UCharPtr+offset) < pSys->m_Regs[b.UInt64].UChar;
					else if( addrmode & TwoBytes )
						pSys->m_bZeroFlag = *(uint16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) < pSys->m_Regs[b.UInt64].UShort;
					else if( addrmode & FourBytes )
						pSys->m_bZeroFlag = *(uint32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) < pSys->m_Regs[b.UInt64].UInt32;
					else if( addrmode & EightBytes )
						pSys->m_bZeroFlag = *(uint64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) < pSys->m_Regs[b.UInt64].UInt64;
				}
			}
			continue;
		}
		
		exec_gtr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].Int64 > b.Int64;
			else if( addrmode & Register )
				pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].Int64 > pSys->m_Regs[b.UInt64].Int64;
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[b.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Byte )
					pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].Char > *(pSys->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & TwoBytes )
					pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].Short > *(int16_t *)(pSys->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & FourBytes )
					pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].Int32 > *(int32_t *)(pSys->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & EightBytes )
					pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].Int64 > *(int64_t *)(pSys->m_Regs[b.UInt64].CharPtr+offset);
			}
			continue;
		}
		exec_gtm:; {	// dest is memory and src is reg
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						pSys->m_bZeroFlag = *(pSys->m_Regs[a.UInt64].CharPtr+offset) > b.Char;
					else if( addrmode & TwoBytes )
						pSys->m_bZeroFlag = *(int16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) > b.Short;
					else if( addrmode & FourBytes )
						pSys->m_bZeroFlag = *(int32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) > b.Int32;
					else if( addrmode & EightBytes )
						pSys->m_bZeroFlag = *(int64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) > b.Int64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						pSys->m_bZeroFlag = *(pSys->m_Regs[a.UInt64].UCharPtr+offset) > pSys->m_Regs[b.UInt64].Char;
					else if( addrmode & TwoBytes )
						pSys->m_bZeroFlag = *(int16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) > pSys->m_Regs[b.UInt64].Short;
					else if( addrmode & FourBytes )
						pSys->m_bZeroFlag = *(int32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) > pSys->m_Regs[b.UInt64].Int32;
					else if( addrmode & EightBytes )
						pSys->m_bZeroFlag = *(int64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) > pSys->m_Regs[b.UInt64].Int64;
				}
			}
			continue;
		}
		exec_ugtr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].UInt64 > b.UInt64;
			else if( addrmode & Register )
				pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].UInt64 > pSys->m_Regs[b.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[b.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Byte )
					pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].UChar > *(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & TwoBytes )
					pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].UShort > *(uint16_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & FourBytes )
					pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].UInt32 > *(uint32_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & EightBytes )
					pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].UInt64 > *(uint64_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset);
			}
			continue;
		}
		exec_ugtm:; {	// dest is memory and src is reg
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						pSys->m_bZeroFlag = *(pSys->m_Regs[a.UInt64].UCharPtr+offset) > b.UChar;
					else if( addrmode & TwoBytes )
						pSys->m_bZeroFlag = *(uint16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) > b.UShort;
					else if( addrmode & FourBytes )
						pSys->m_bZeroFlag = *(uint32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) > b.UInt32;
					else if( addrmode & EightBytes )
						pSys->m_bZeroFlag = *(uint64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) > b.UInt64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						pSys->m_bZeroFlag = *(pSys->m_Regs[a.UInt64].UCharPtr+offset) > pSys->m_Regs[b.UInt64].UChar;
					else if( addrmode & TwoBytes )
						pSys->m_bZeroFlag = *(uint16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) > pSys->m_Regs[b.UInt64].UShort;
					else if( addrmode & FourBytes )
						pSys->m_bZeroFlag = *(uint32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) > pSys->m_Regs[b.UInt64].UInt32;
					else if( addrmode & EightBytes )
						pSys->m_bZeroFlag = *(uint64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) > pSys->m_Regs[b.UInt64].UInt64;
				}
			}
			continue;
		}
		
		exec_cmpr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].Int64 == b.Int64;
			else if( addrmode & Register )
				pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].Int64 == pSys->m_Regs[b.UInt64].Int64;
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[b.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Byte )
					pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].Char == *(pSys->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & TwoBytes )
					pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].Short == *(int16_t *)(pSys->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & FourBytes )
					pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].Int32 == *(int32_t *)(pSys->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & EightBytes )
					pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].Int64 == *(int64_t *)(pSys->m_Regs[b.UInt64].CharPtr+offset);
			}
			continue;
		}
		exec_cmpm:; {	// dest is memory and src is reg
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						pSys->m_bZeroFlag = *(pSys->m_Regs[a.UInt64].CharPtr+offset) == b.Char;
					else if( addrmode & TwoBytes )
						pSys->m_bZeroFlag = *(int16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) == b.Short;
					else if( addrmode & FourBytes )
						pSys->m_bZeroFlag = *(int32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) == b.Int32;
					else if( addrmode & EightBytes )
						pSys->m_bZeroFlag = *(int64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) == b.Int64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						pSys->m_bZeroFlag = *(pSys->m_Regs[a.UInt64].UCharPtr+offset) == pSys->m_Regs[b.UInt64].Char;
					else if( addrmode & TwoBytes )
						pSys->m_bZeroFlag = *(int16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) == pSys->m_Regs[b.UInt64].Short;
					else if( addrmode & FourBytes )
						pSys->m_bZeroFlag = *(int32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) == pSys->m_Regs[b.UInt64].Int32;
					else if( addrmode & EightBytes )
						pSys->m_bZeroFlag = *(int64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) == pSys->m_Regs[b.UInt64].Int64;
				}
			}
			continue;
		}
		exec_ucmpr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].UInt64 == b.UInt64;
			else if( addrmode & Register )
				pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].UInt64 == pSys->m_Regs[b.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[b.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Byte )
					pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].UChar == *(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & TwoBytes )
					pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].UShort == *(uint16_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & FourBytes )
					pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].UInt32 == *(uint32_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & EightBytes )
					pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].UInt64 == *(uint64_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset);
			}
			continue;
		}
		exec_ucmpm:; {	// dest is memory and src is reg
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						pSys->m_bZeroFlag = *(pSys->m_Regs[a.UInt64].UCharPtr+offset) == b.UChar;
					else if( addrmode & TwoBytes )
						pSys->m_bZeroFlag = *(uint16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) == b.UShort;
					else if( addrmode & FourBytes )
						pSys->m_bZeroFlag = *(uint32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) == b.UInt32;
					else if( addrmode & EightBytes )
						pSys->m_bZeroFlag = *(uint64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) == b.UInt64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						pSys->m_bZeroFlag = *(pSys->m_Regs[a.UInt64].UCharPtr+offset) == pSys->m_Regs[b.UInt64].UChar;
					else if( addrmode & TwoBytes )
						pSys->m_bZeroFlag = *(uint16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) == pSys->m_Regs[b.UInt64].UShort;
					else if( addrmode & FourBytes )
						pSys->m_bZeroFlag = *(uint32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) == pSys->m_Regs[b.UInt64].UInt32;
					else if( addrmode & EightBytes )
						pSys->m_bZeroFlag = *(uint64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) == pSys->m_Regs[b.UInt64].UInt64;
				}
			}
			continue;
		}
		
		exec_neqr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].Int64 != b.Int64;
			else if( addrmode & Register )
				pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].Int64 != pSys->m_Regs[b.UInt64].Int64;
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[b.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Byte )
					pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].Char != *(pSys->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & TwoBytes )
					pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].Short != *(int16_t *)(pSys->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & FourBytes )
					pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].Int32 != *(int32_t *)(pSys->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & EightBytes )
					pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].Int64 != *(int64_t *)(pSys->m_Regs[b.UInt64].CharPtr+offset);
			}
			continue;
		}
		exec_neqm:; {	// dest is memory and src is reg
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						pSys->m_bZeroFlag = *(pSys->m_Regs[a.UInt64].CharPtr+offset) != b.Char;
					else if( addrmode & TwoBytes )
						pSys->m_bZeroFlag = *(int16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) != b.Short;
					else if( addrmode & FourBytes )
						pSys->m_bZeroFlag = *(int32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) != b.Int32;
					else if( addrmode & EightBytes )
						pSys->m_bZeroFlag = *(int64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) != b.Int64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						pSys->m_bZeroFlag = *(pSys->m_Regs[a.UInt64].UCharPtr+offset) != pSys->m_Regs[b.UInt64].Char;
					else if( addrmode & TwoBytes )
						pSys->m_bZeroFlag = *(int16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) != pSys->m_Regs[b.UInt64].Short;
					else if( addrmode & FourBytes )
						pSys->m_bZeroFlag = *(int32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) != pSys->m_Regs[b.UInt64].Int32;
					else if( addrmode & EightBytes )
						pSys->m_bZeroFlag = *(int64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) != pSys->m_Regs[b.UInt64].Int64;
				}
			}
			continue;
		}
		exec_uneqr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].UInt64 != b.UInt64;
			else if( addrmode & Register )
				pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].UInt64 != pSys->m_Regs[b.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[b.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Byte )
					pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].UChar != *(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & TwoBytes )
					pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].UShort != *(uint16_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & FourBytes )
					pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].UInt32 != *(uint32_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset);
				else if( addrmode & EightBytes )
					pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].UInt64 != *(uint64_t *)(pSys->m_Regs[b.UInt64].UCharPtr+offset);
			}
			continue;
		}
		exec_uneqm:; {	// dest is memory and src is reg
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Immediate ) {
					if( addrmode & Byte )
						pSys->m_bZeroFlag = *(pSys->m_Regs[a.UInt64].UCharPtr+offset) != b.UChar;
					else if( addrmode & TwoBytes )
						pSys->m_bZeroFlag = *(uint16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) != b.UShort;
					else if( addrmode & FourBytes )
						pSys->m_bZeroFlag = *(uint32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) != b.UInt32;
					else if( addrmode & EightBytes )
						pSys->m_bZeroFlag = *(uint64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) != b.UInt64;
				}
				else if( addrmode & Register ) {
					if( addrmode & Byte )
						pSys->m_bZeroFlag = *(pSys->m_Regs[a.UInt64].UCharPtr+offset) != pSys->m_Regs[b.UInt64].UChar;
					else if( addrmode & TwoBytes )
						pSys->m_bZeroFlag = *(uint16_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) != pSys->m_Regs[b.UInt64].UShort;
					else if( addrmode & FourBytes )
						pSys->m_bZeroFlag = *(uint32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) != pSys->m_Regs[b.UInt64].UInt32;
					else if( addrmode & EightBytes )
						pSys->m_bZeroFlag = *(uint64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) != pSys->m_Regs[b.UInt64].UInt64;
				}
			}
			continue;
		}
		exec_reset:; {	// reset ALL registers except rip, rsp, and rbp to 0.
			Tagha_Reset(pSys);
			continue;
		}
		
		exec_int2float:; {
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate ) {
				b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
				pSys->m_Regs[a.UInt64].UInt64 = 0;
				pSys->m_Regs[a.UInt64].Float = (float) b.Int64;
			}
			else if( addrmode & Register ) {
				float temp = (float) pSys->m_Regs[a.UInt64].Int64;
				pSys->m_Regs[a.UInt64].UInt64 = 0;
				pSys->m_Regs[a.UInt64].Float = temp;
			}
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				float temp = (float) *(int32_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset);
				*(float *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) = temp;
			}
			continue;
		}
		exec_int2dbl:; {
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate ) {
				b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
				pSys->m_Regs[a.UInt64].UInt64 = 0;
				pSys->m_Regs[a.UInt64].Double = (double) b.UInt64;
			}
			else if( addrmode & Register ) {
				double temp = (double) pSys->m_Regs[a.UInt64].UInt64;
				pSys->m_Regs[a.UInt64].UInt64 = 0;
				pSys->m_Regs[a.UInt64].Double = temp;
			}
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				double temp = (double) *(int64_t *)(pSys->m_Regs[a.UInt64].UCharPtr+offset);
				*(double *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) = temp;
			}
			continue;
		}
		
		exec_float2dbl:; {
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			double temp;
			if( addrmode & Immediate ) {
				b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
				temp = (double) b.Float;
				pSys->m_Regs[a.UInt64].UInt64 = 0;
				pSys->m_Regs[a.UInt64].Double = temp;
			}
			else if( addrmode & Register ) {
				temp = (double) pSys->m_Regs[a.UInt64].Float;
				pSys->m_Regs[a.UInt64].UInt64 = 0;
				pSys->m_Regs[a.UInt64].Double = temp;
			}
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				temp = (double) *(float *)(pSys->m_Regs[a.UInt64].UCharPtr+offset);
				*(double *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) = temp;
			}
			continue;
		}
		exec_dbl2float:; {
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			float temp;
			if( addrmode & Immediate ) {
				b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
				temp = (float) b.Double;
				pSys->m_Regs[a.UInt64].UInt64 = 0;
				pSys->m_Regs[a.UInt64].Float = temp;
			}
			else if( addrmode & Register ) {
				temp = (float) pSys->m_Regs[a.UInt64].Double;
				pSys->m_Regs[a.UInt64].UInt64 = 0;
				pSys->m_Regs[a.UInt64].Float = temp;
			}
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				temp = (float) *(double *)(pSys->m_Regs[a.UInt64].UCharPtr+offset);
				*(double *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) = 0;
				*(float *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) = temp;
			}
			continue;
		}
		
		exec_faddr:; {
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pSys->m_Regs[a.UInt64].Double += b.Double;
			else if( addrmode & Register )
				pSys->m_Regs[a.UInt64].Double += pSys->m_Regs[b.UInt64].Double;
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[b.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & FourBytes )
					pSys->m_Regs[a.UInt64].Float += *(float *)(pSys->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & EightBytes )
					pSys->m_Regs[a.UInt64].Double += *(double *)(pSys->m_Regs[b.UInt64].CharPtr+offset);
			}
			continue;
		}
		exec_faddm:; {
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Immediate ) {
					if( addrmode & FourBytes ) {
						*(float *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) += b.Float;
					}
					else if( addrmode & EightBytes ) {
						*(double *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) += b.Double;
					}
				}
				else if( addrmode & Register ) {
					if( addrmode & FourBytes )
						*(float *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) += pSys->m_Regs[b.UInt64].Float;
					else if( addrmode & EightBytes )
						*(double *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) += pSys->m_Regs[b.UInt64].Double;
				}
			}
			continue;
		}
		exec_fsubr:; {
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pSys->m_Regs[a.UInt64].Double -= b.Double;
			else if( addrmode & Register )
				pSys->m_Regs[a.UInt64].Double -= pSys->m_Regs[b.UInt64].Double;
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[b.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & FourBytes )
					pSys->m_Regs[a.UInt64].Float -= *(float *)(pSys->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & EightBytes )
					pSys->m_Regs[a.UInt64].Double -= *(double *)(pSys->m_Regs[b.UInt64].CharPtr+offset);
			}
			continue;
		}
		exec_fsubm:; {
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Immediate ) {
					if( addrmode & FourBytes )
						*(float *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) -= b.Float;
					else if( addrmode & EightBytes )
						*(double *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) -= b.Double;
				}
				else if( addrmode & Register ) {
					if( addrmode & FourBytes )
						*(float *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) -= pSys->m_Regs[b.UInt64].Float;
					else if( addrmode & EightBytes )
						*(double *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) -= pSys->m_Regs[b.UInt64].Double;
				}
			}
			continue;
		}
		exec_fmulr:; {
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pSys->m_Regs[a.UInt64].Double *= b.Double;
			else if( addrmode & Register )
				pSys->m_Regs[a.UInt64].Double *= pSys->m_Regs[b.UInt64].Double;
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[b.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & FourBytes )
					pSys->m_Regs[a.UInt64].Float *= *(float *)(pSys->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & EightBytes )
					pSys->m_Regs[a.UInt64].Double *= *(double *)(pSys->m_Regs[b.UInt64].CharPtr+offset);
			}
			continue;
		}
		exec_fmulm:; {
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Immediate ) {
					if( addrmode & FourBytes )
						*(float *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) *= b.Float;
					else if( addrmode & EightBytes )
						*(double *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) *= b.Double;
				}
				else if( addrmode & Register ) {
					if( addrmode & FourBytes )
						*(float *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) *= pSys->m_Regs[b.UInt64].Float;
					else if( addrmode & EightBytes )
						*(double *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) *= pSys->m_Regs[b.UInt64].Double;
				}
			}
			continue;
		}
		exec_fdivr:; {
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate ) {
				if( !b.Double )
					b.Double=1.0;
				pSys->m_Regs[a.UInt64].Double /= b.Double;
			}
			else if( addrmode & Register ) {
				if( !pSys->m_Regs[b.UInt64].Double )
					pSys->m_Regs[b.UInt64].Double=1.0;
				pSys->m_Regs[a.UInt64].Double /= pSys->m_Regs[b.UInt64].Double;
			}
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[b.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & FourBytes ) {
					if( !*(float *)(pSys->m_Regs[b.UInt64].CharPtr+offset) )
						*(float *)(pSys->m_Regs[b.UInt64].CharPtr+offset) = 1.f;
					pSys->m_Regs[a.UInt64].Float /= *(float *)(pSys->m_Regs[b.UInt64].CharPtr+offset);
				}
				else if( addrmode & EightBytes ) {
					if( !*(double *)(pSys->m_Regs[b.UInt64].CharPtr+offset) )
						*(double *)(pSys->m_Regs[b.UInt64].CharPtr+offset) = 1.0;
					pSys->m_Regs[a.UInt64].Double /= *(double *)(pSys->m_Regs[b.UInt64].CharPtr+offset);
				}
			}
			continue;
		}
		exec_fdivm:; {
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Immediate ) {
					if( addrmode & FourBytes ) {
						if( !b.Float )
							b.Float = 1.f;
						*(float *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) /= b.Float;
					}
					else if( addrmode & EightBytes ) {
						if( !b.Double )
							b.Double = 1.0;
						*(double *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) /= b.Double;
					}
				}
				else if( addrmode & Register ) {
					if( addrmode & FourBytes ) {
						if( !b.Float )
							b.Float = 1.f;
						*(float *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) /= pSys->m_Regs[b.UInt64].Float;
					}
					else if( addrmode & EightBytes ) {
						if( !b.Double )
							b.Double = 1.f;
						*(double *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) /= pSys->m_Regs[b.UInt64].Double;
					}
				}
			}
			continue;
		}
		exec_fneg:; {
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Register )
				pSys->m_Regs[a.UInt64].Double = -pSys->m_Regs[a.UInt64].Double;
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & FourBytes )
					*(float *)(pSys->m_Regs[a.UInt64].CharPtr+offset) = -*(float *)(pSys->m_Regs[a.UInt64].CharPtr+offset);
				else if( addrmode & EightBytes )
					*(double *)(pSys->m_Regs[a.UInt64].CharPtr+offset) = -*(double *)(pSys->m_Regs[a.UInt64].CharPtr+offset);
			}
			continue;
		}
		exec_fltr:; {
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].Double < b.Double;
			else if( addrmode & Register )
				pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].Double < pSys->m_Regs[b.UInt64].Double;
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[b.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & FourBytes )
					pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].Float < *(float *)(pSys->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & EightBytes )
					pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].Double < *(double *)(pSys->m_Regs[b.UInt64].CharPtr+offset);
			}
			continue;
		}
		exec_fltm:; {
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Immediate ) {
					if( addrmode & FourBytes )
						pSys->m_bZeroFlag = *(float *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) < b.Float;
					else if( addrmode & EightBytes )
						pSys->m_bZeroFlag = *(double *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) < b.Double;
				}
				else if( addrmode & Register ) {
					if( addrmode & FourBytes )
						pSys->m_bZeroFlag = *(float *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) < pSys->m_Regs[b.UInt64].Float;
					else if( addrmode & EightBytes )
						pSys->m_bZeroFlag = *(double *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) < pSys->m_Regs[b.UInt64].Double;
				}
			}
			continue;
		}
		exec_fgtr:; {
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].Double > b.Double;
			else if( addrmode & Register )
				pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].Double > pSys->m_Regs[b.UInt64].Double;
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[b.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & FourBytes )
					pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].Float > *(float *)(pSys->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & EightBytes )
					pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].Double > *(double *)(pSys->m_Regs[b.UInt64].CharPtr+offset);
			}
			continue;
		}
		exec_fgtm:; {
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Immediate ) {
					if( addrmode & FourBytes )
						pSys->m_bZeroFlag = *(float *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) > b.Float;
					else if( addrmode & EightBytes )
						pSys->m_bZeroFlag = *(double *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) > b.Double;
				}
				else if( addrmode & Register ) {
					if( addrmode & FourBytes )
						pSys->m_bZeroFlag = *(float *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) > pSys->m_Regs[b.UInt64].Float;
					else if( addrmode & EightBytes )
						pSys->m_bZeroFlag = *(double *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) > pSys->m_Regs[b.UInt64].Double;
				}
			}
			continue;
		}
		exec_fcmpr:; {
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].Double == b.Double;
			else if( addrmode & Register )
				pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].Double == pSys->m_Regs[b.UInt64].Double;
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[b.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & FourBytes )
					pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].Float == *(float *)(pSys->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & EightBytes )
					pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].Double == *(double *)(pSys->m_Regs[b.UInt64].CharPtr+offset);
			}
			continue;
		}
		exec_fcmpm:; {
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Immediate ) {
					if( addrmode & FourBytes )
						pSys->m_bZeroFlag = *(float *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) == b.Float;
					else if( addrmode & EightBytes )
						pSys->m_bZeroFlag = *(double *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) == b.Double;
				}
				else if( addrmode & Register ) {
					if( addrmode & FourBytes )
						pSys->m_bZeroFlag = *(float *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) == pSys->m_Regs[b.UInt64].Float;
					else if( addrmode & EightBytes )
						pSys->m_bZeroFlag = *(double *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) == pSys->m_Regs[b.UInt64].Double;
				}
			}
			continue;
		}
		exec_fneqr:; {
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].Double != b.Double;
			else if( addrmode & Register )
				pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].Double != pSys->m_Regs[b.UInt64].Double;
			else if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[b.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & FourBytes )
					pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].Float != *(float *)(pSys->m_Regs[b.UInt64].CharPtr+offset);
				else if( addrmode & EightBytes )
					pSys->m_bZeroFlag = pSys->m_Regs[a.UInt64].Double != *(double *)(pSys->m_Regs[b.UInt64].CharPtr+offset);
			}
			continue;
		}
		exec_fneqm:; {
			a.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			b.UInt64 = *pSys->m_Regs[rip].UInt64Ptr++;
			if( addrmode & RegIndirect ) {
				offset = *pSys->m_Regs[rip].Int32Ptr++;
				if( safemode and !(pSys->m_Regs[a.UInt64].UCharPtr) ) {
					Tagha_PrintErr(pSys, __func__, "NULL Pointer exception!");
					continue;
				}
				if( addrmode & Immediate ) {
					if( addrmode & FourBytes )
						pSys->m_bZeroFlag = *(float *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) != b.Float;
					else if( addrmode & EightBytes )
						pSys->m_bZeroFlag = *(double *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) != b.Double;
				}
				else if( addrmode & Register ) {
					if( addrmode & FourBytes )
						pSys->m_bZeroFlag = *(float *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) != pSys->m_Regs[b.UInt64].Float;
					else if( addrmode & EightBytes )
						pSys->m_bZeroFlag = *(double *)(pSys->m_Regs[a.UInt64].UCharPtr+offset) != pSys->m_Regs[b.UInt64].Double;
				}
			}
			continue;
		}
	}
	return 0;
}
