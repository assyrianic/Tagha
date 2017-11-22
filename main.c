/*
 * main.c
 * 
 * Copyright 2017 KEVIN <kevin@KEVRAMPAGE>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */


#include <stdio.h>
#include <string.h>
#include "tagha.h"

const char *vm_regid_to_str(const enum RegID id)
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

// Reduce from 64-bit int but preserve sign bit.
static int32_t Long2Int(const int64_t i)
{
	int32_t val = (int32_t)i;
	if( i<0 )
		val |= 0x80000000;
	return val;
}
static int16_t Long2Short(const int64_t i)
{
	int16_t val = (int16_t)i;
	if( i<0 )
		val |= 0x8000;
	return val;
}
static int8_t Long2Char(const int64_t i)
{
	int8_t val = (int8_t)i;
	if( i<0 )
		val |= 0x80;
	return val;
}

#include <unistd.h>	// sleep() func
void vm_exec(uint8_t text[])
{
	if( !text )
		return;
	
	const uint16_t memsize = 63;
	union Val mem[memsize+1];
	memset(mem, 0, (memsize+1) * sizeof(union Val));
	struct VM vm;
	struct VM *vm_ptr = &vm;
	memset(&vm, 0, sizeof(struct VM));
	
	vm.reg[rip].UCharPtr = text,
	vm.reg[rsp].SelfVal = mem+memsize,
	vm.reg[rbp].SelfVal = vm.reg[rsp].SelfVal
	;
	uint8_t
		instr,
		addrmode	// what addressing mode to use
	;
	union Val
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
	
	
	while( 1 ) {
		printf("R E G I S T E R:[%s] == %" PRIu64 "\n", vm_regid_to_str(ras), vm.reg[ras].UInt64);
		a.UInt64 = b.UInt64 = 0;	// reset our temporaries.
		int32_t offset = 0;
		
		// fetch opcode and addressing mode.
		instr = *vm.reg[rip].UCharPtr++;
		addrmode = *vm.reg[rip].UCharPtr++;
		
		// this is for debugging.
#ifdef _UNISTD_H
		sleep(1);
#endif
		puts(opcode2str[instr]);
		
		// assert the instr is within bounds.
		if( instr > nop )
			break;
		
		goto *dispatch[instr];
		
		exec_nop:;
			continue;
		
		exec_halt:;
			puts("DEBUG :: PRINTING MEMORY::");
			for( uint32_t i=0 ; i<memsize+1 ; i++ )
				printf("memory[%u] == %" PRIu64 "\n", i, mem[i].UInt64);
			puts("\nDEBUG :: PRINTING REGISTERS::");
			for( uint8_t i=0 ; i<regsize ; i++ )
				printf("[%s] == %" PRIu64 "\n", vm_regid_to_str(i), vm.reg[i].UInt64);
			return;
		
		exec_push:; {
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				(*--vm.reg[rsp].SelfVal).UInt64 = a.UInt64;
			else if( addrmode & Register )
				*--vm.reg[rsp].SelfVal = vm.reg[a.UInt64];
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				(*--vm.reg[rsp].SelfVal).UInt64 = *(uint64_t *)(vm.reg[a.UInt64].UCharPtr+offset);
			}
			else if( addrmode & Direct )
				(*--vm.reg[rsp].SelfVal).UInt64 = *a.UInt64Ptr;
			continue;
		}
		exec_pop:; {
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.reg[a.UInt64].UInt64 = (*vm.reg[rsp].SelfVal++).UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				*(uint64_t *)(vm.reg[a.UInt64].UCharPtr+offset) = (*vm.reg[rsp].SelfVal++).UInt64;
			}
			else if( addrmode & (Direct|Immediate) )
				*a.UInt64Ptr = (*vm.reg[rsp].SelfVal++).UInt64;
			continue;
		}
		exec_neg:; {
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.reg[a.UInt64].UInt64 = -vm.reg[a.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					*(vm.reg[a.UInt64].CharPtr+offset) = -*(vm.reg[a.UInt64].CharPtr+offset);
				else if( addrmode & Half )
					*(int16_t *)(vm.reg[a.UInt64].CharPtr+offset) = -*(int16_t *)(vm.reg[a.UInt64].CharPtr+offset);
				else if( addrmode & Long )
					*(int32_t *)(vm.reg[a.UInt64].CharPtr+offset) = -*(int32_t *)(vm.reg[a.UInt64].CharPtr+offset);
				else if( addrmode & Quad )
					*(int64_t *)(vm.reg[a.UInt64].CharPtr+offset) = -*(int64_t *)(vm.reg[a.UInt64].CharPtr+offset);
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					*a.CharPtr = -*a.CharPtr;
				else if( addrmode & Half )
					*a.ShortPtr = -*a.ShortPtr;
				else if( addrmode & Long )
					*a.Int32Ptr = -*a.Int32Ptr;
				else if( addrmode & Quad )
					*a.Int64Ptr = -*a.Int64Ptr;
			}
			continue;
		}
		
		exec_inc:; {
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.reg[a.UInt64].UInt64 += 1;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					*(vm.reg[a.UInt64].UCharPtr+offset) += 1;
				else if( addrmode & Half )
					*(uint16_t *)(vm.reg[a.UInt64].UCharPtr+offset) += 1;
				else if( addrmode & Long )
					*(uint32_t *)(vm.reg[a.UInt64].UCharPtr+offset) += 1;
				else if( addrmode & Quad )
					*(uint64_t *)(vm.reg[a.UInt64].UCharPtr+offset) += 1;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					*a.UCharPtr += 1;
				else if( addrmode & Half )
					*a.UShortPtr += 1;
				else if( addrmode & Long )
					*a.UInt32Ptr += 1;
				else if( addrmode & Quad )
					*a.UInt64Ptr += 1;
			}
			continue;
		}
		exec_dec:; {
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.reg[a.UInt64].UInt64 -= 1;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					*(vm.reg[a.UInt64].UCharPtr+offset) -= 1;
				else if( addrmode & Half )
					*(uint16_t *)(vm.reg[a.UInt64].UCharPtr+offset) -= 1;
				else if( addrmode & Long )
					*(uint32_t *)(vm.reg[a.UInt64].UCharPtr+offset) -= 1;
				else if( addrmode & Quad )
					*(uint64_t *)(vm.reg[a.UInt64].UCharPtr+offset) -= 1;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					*a.UCharPtr -= 1;
				else if( addrmode & Half )
					*a.UShortPtr -= 1;
				else if( addrmode & Long )
					*a.UInt32Ptr -= 1;
				else if( addrmode & Quad )
					*a.UInt64Ptr -= 1;
			}
			continue;
		}
		exec_bnot:; {
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.reg[a.UInt64].UInt64 = ~vm.reg[a.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					*(vm.reg[a.UInt64].UCharPtr+offset) = ~*(vm.reg[a.UInt64].UCharPtr+offset);
				else if( addrmode & Half )
					*(uint16_t *)(vm.reg[a.UInt64].UCharPtr+offset) = ~*(uint16_t *)(vm.reg[a.UInt64].UCharPtr+offset);
				else if( addrmode & Long )
					*(uint32_t *)(vm.reg[a.UInt64].UCharPtr+offset) = ~*(uint32_t *)(vm.reg[a.UInt64].UCharPtr+offset);
				else if( addrmode & Quad )
					*(uint64_t *)(vm.reg[a.UInt64].UCharPtr+offset) = ~*(uint64_t *)(vm.reg[a.UInt64].UCharPtr+offset);
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					*a.UCharPtr = ~*a.UCharPtr;
				else if( addrmode & Half )
					*a.UShortPtr = ~*a.UShortPtr;
				else if( addrmode & Long )
					*a.UInt32Ptr = ~*a.UInt32Ptr;
				else if( addrmode & Quad )
					*a.UInt64Ptr = ~*a.UInt64Ptr;
			}
			continue;
		}
		
		exec_long2int:; {
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.reg[a.UInt64].Int32 = Long2Int(vm.reg[a.UInt64].Int64);
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				*(int32_t *)(vm.reg[a.UInt64].UCharPtr+offset) = Long2Int(*(int64_t *)(vm.reg[a.UInt64].UCharPtr+offset));
			}
			else if( addrmode & (Direct|Immediate) )
				*a.Int32Ptr = Long2Int(*a.Int64Ptr);
			continue;
		}
		exec_long2short:; {
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.reg[a.UInt64].Short = Long2Short(vm.reg[a.UInt64].Int64);
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				*(int16_t *)(vm.reg[a.UInt64].UCharPtr+offset) = Long2Short(*(int64_t *)(vm.reg[a.UInt64].UCharPtr+offset));
			}
			else if( addrmode & (Direct|Immediate) )
				*a.ShortPtr = Long2Short(*a.Int64Ptr);
			continue;
		}
		exec_long2byte:; {
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.reg[a.UInt64].Char = Long2Char(vm.reg[a.UInt64].Int64);
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				*(vm.reg[a.UInt64].CharPtr+offset) = Long2Char(*(int64_t *)(vm.reg[a.UInt64].UCharPtr+offset));
			}
			else if( addrmode & (Direct|Immediate) )
				*a.CharPtr = Long2Char(*a.Int64Ptr);
			continue;
		}
		exec_int2long:; {
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.reg[a.UInt64].Int64 = (int64_t) vm.reg[a.UInt64].Int32;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				*(int64_t *)(vm.reg[a.UInt64].UCharPtr+offset) = (int64_t) *(int32_t *)(vm.reg[a.UInt64].UCharPtr+offset);
			}
			else if( addrmode & (Direct|Immediate) )
				*a.Int64Ptr = (int64_t) *a.Int32Ptr;
			continue;
		}
		exec_short2long:; {
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.reg[a.UInt64].Int64 = (int64_t) vm.reg[a.UInt64].Short;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				*(int64_t *)(vm.reg[a.UInt64].UCharPtr+offset) = (int64_t) *(int16_t *)(vm.reg[a.UInt64].UCharPtr+offset);
			}
			else if( addrmode & (Direct|Immediate) )
				*a.Int64Ptr = (int64_t) *a.ShortPtr;
			continue;
		}
		exec_byte2long:; {
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.reg[a.UInt64].Int64 = (int64_t)vm.reg[a.UInt64].Char;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				*(int64_t *)(vm.reg[a.UInt64].UCharPtr+offset) = (int64_t) *(vm.reg[a.UInt64].CharPtr+offset);
			}
			else if( addrmode & (Direct|Immediate) )
				*a.Int64Ptr = (int64_t) *a.CharPtr;
			continue;
		}
		
		exec_jmp:; {
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				vm.reg[rip].UCharPtr = text+a.UInt64;
			else if( addrmode & Register )
				vm.reg[rip].UCharPtr = text+vm.reg[a.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				vm.reg[rip].UCharPtr = (uint8_t *)(uintptr_t) *(uint64_t *)(vm.reg[a.UInt64].UCharPtr+offset);
			}
			else if( addrmode & Direct )
				vm.reg[rip].UCharPtr = (uint8_t *)(uintptr_t) *a.UInt64Ptr;
			continue;
		}
		
		exec_jz:; {
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				vm.reg[rip].UCharPtr = (!vm.zero_flag) ? text+a.UInt64 : vm.reg[rip].UCharPtr;
			else if( addrmode & Register )
				vm.reg[rip].UCharPtr = (!vm.zero_flag) ? text+vm.reg[a.UInt64].UInt64 : vm.reg[rip].UCharPtr;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				vm.reg[rip].UCharPtr = (!vm.zero_flag) ? (uint8_t *)(uintptr_t) *(uint64_t *)(vm.reg[a.UInt64].UCharPtr+offset) : vm.reg[rip].UCharPtr;
			}
			else if( addrmode & Direct )
				vm.reg[rip].UCharPtr = (!vm.zero_flag) ? (uint8_t *)(uintptr_t) *a.UInt64Ptr : vm.reg[rip].UCharPtr;
			continue;
		}
		exec_jnz:; {
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Immediate )
				vm.reg[rip].UCharPtr = (vm.zero_flag) ? text+a.UInt64 : vm.reg[rip].UCharPtr;
			else if( addrmode & Register )
				vm.reg[rip].UCharPtr = (vm.zero_flag) ? text+vm.reg[a.UInt64].UInt64 : vm.reg[rip].UCharPtr;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				vm.reg[rip].UCharPtr = (vm.zero_flag) ? (uint8_t *)(uintptr_t) *(uint64_t *)(vm.reg[a.UInt64].UCharPtr+offset) : vm.reg[rip].UCharPtr;
			}
			else if( addrmode & Direct )
				vm.reg[rip].UCharPtr = (vm.zero_flag) ? (uint8_t *)(uintptr_t) *a.UInt64Ptr : vm.reg[rip].UCharPtr;
			continue;
		}
		
		exec_call:; {
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			(*--vm.reg[rsp].SelfVal).UInt64= vm.reg[rip].UInt64;	// push rip
			//printf("call :: pushing rip == %" PRIx64 "\n", vm.reg[rip].UInt64);
			(*--vm.reg[rsp].SelfVal).UInt64 = vm.reg[rbp].UInt64;	// push rbp
			//printf("call :: pushing rbp == %" PRIx64 "\n", vm.reg[rbp].UInt64);
			vm.reg[rbp] = vm.reg[rsp];	// mov rbp, rsp
			if( addrmode & Immediate )
				vm.reg[rip].UCharPtr = text + a.UInt64;
			else if( addrmode & Register )
				vm.reg[rip].UCharPtr = text + vm.reg[a.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				vm.reg[rip].UCharPtr = (uint8_t *)(uintptr_t) *(uint64_t *)(vm.reg[a.UInt64].UCharPtr+offset);
			}
			else if( addrmode & Direct )
				vm.reg[rip].UCharPtr = (uint8_t *)(uintptr_t) *a.UInt64Ptr;
			continue;
		}
		exec_ret:; {
			vm.reg[rsp] = vm.reg[rbp];	// mov rsp, rbp
			vm.reg[rbp].UInt64 = (*vm.reg[rsp].SelfVal++).UInt64;	// pop rbp
			vm.reg[rip].UInt64 = (*vm.reg[rsp].SelfVal++).UInt64;
			printf("ret :: popped rip == %p\n", vm.reg[rip].UCharPtr);
			if( addrmode & Immediate )
				vm.reg[rsp].UCharPtr += *vm.reg[rip].UInt64Ptr++;
			continue;
		}
		
		exec_callnat:; {
			continue;
		}
		
		exec_movi:; {	// dest is reg or mem and src is an imm
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			printf("addrmode == %" PRIu8 "\na == %" PRIu64 "\nb == %" PRIu64 "\n", addrmode, a.UInt64, b.UInt64);
			if( addrmode & Register )
				vm.reg[a.UInt64].UInt64 = b.UInt64;
			else if( addrmode & RegIndirect ) {
				int32_t offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					*(vm.reg[a.UInt64].UCharPtr) = b.UChar;
				else if( addrmode & Half )
					*(uint16_t *)(vm.reg[a.UInt64].UCharPtr+offset) = b.UShort;
				else if( addrmode & Long )
					*(uint32_t *)(vm.reg[a.UInt64].UCharPtr+offset) = b.UInt32;
				else if( addrmode & Quad )
					*(uint64_t *)(vm.reg[a.UInt64].UCharPtr+offset) = b.UInt64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					*a.UCharPtr = b.UChar;
				else if( addrmode & Half )
					*a.UShortPtr = b.UShort;
				else if( addrmode & Long )
					*a.UInt32Ptr = b.UInt32;
				else if( addrmode & Quad )
					*a.UInt64Ptr = b.UInt64;
			}
			continue;
		}
		exec_movr:; {	// dest is a reg, src is reg or memory
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.reg[a.UInt64].UInt64 = vm.reg[b.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					vm.reg[a.UInt64].UChar = *(vm.reg[b.UInt64].UCharPtr+offset);
				else if( addrmode & Half )
					vm.reg[a.UInt64].UShort = *(uint16_t *)(vm.reg[b.UInt64].UCharPtr+offset);
				else if( addrmode & Long )
					vm.reg[a.UInt64].UInt32 = *(uint32_t *)(vm.reg[b.UInt64].UCharPtr+offset);
				else if( addrmode & Quad )
					vm.reg[a.UInt64].UInt64 = *(uint64_t *)(vm.reg[b.UInt64].UCharPtr+offset);
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					vm.reg[a.UInt64].UChar = *b.UCharPtr;
				else if( addrmode & Half )
					vm.reg[a.UInt64].UShort = *b.UShortPtr;
				else if( addrmode & Long )
					vm.reg[a.UInt64].UInt32 = *b.UInt32Ptr;
				else if( addrmode & Quad )
					vm.reg[a.UInt64].UInt64 = *b.UInt64Ptr;
			}
			continue;
		}
		exec_movm:; {	// dest is memory and src is reg
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & (Register|RegIndirect) ) {
				offset = *vm.reg[rip].Int32Ptr++;
				printf("movm :: offset == %i\n", offset);
				if( addrmode & Byte )
					*(vm.reg[a.UInt64].UCharPtr+offset) = vm.reg[b.UInt64].UChar;
				else if( addrmode & Half )
					*(uint16_t *)(vm.reg[a.UInt64].UCharPtr+offset) = vm.reg[b.UInt64].UShort;
				else if( addrmode & Long )
					*(uint32_t *)(vm.reg[a.UInt64].UCharPtr+offset) = vm.reg[b.UInt64].UInt32;
				else if( addrmode & Quad )
					*(uint64_t *)(vm.reg[a.UInt64].UCharPtr+offset) = vm.reg[b.UInt64].UInt64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					*a.UCharPtr = vm.reg[b.UInt64].UChar;
				else if( addrmode & Half )
					*a.UShortPtr = vm.reg[b.UInt64].UShort;
				else if( addrmode & Long )
					*a.UInt32Ptr = vm.reg[b.UInt64].UInt32;
				else if( addrmode & Quad )
					*a.UInt64Ptr = vm.reg[b.UInt64].UInt64;
			}
			continue;
		}
		exec_lea:; {
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.reg[a.UInt64].UInt64 = vm.reg[b.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				vm.reg[a.UInt64].UInt64 = vm.reg[b.UInt64].UInt64;
			}
			else if( addrmode & (Direct|Immediate) )
				vm.reg[a.UInt64].UInt64 = b.UInt64;
			continue;
		}
		exec_addi:; {	// dest is reg or memory, src is imm
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.reg[a.UInt64].Int64 += b.Int64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					*(vm.reg[a.UInt64].CharPtr+offset) += b.Char;
				else if( addrmode & Half )
					*(int16_t *)(vm.reg[a.UInt64].UCharPtr+offset) += b.Short;
				else if( addrmode & Long )
					*(int32_t *)(vm.reg[a.UInt64].UCharPtr+offset) += b.Int32;
				else if( addrmode & Quad )
					*(int64_t *)(vm.reg[a.UInt64].UCharPtr+offset) += b.Int64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					*a.CharPtr += b.Char;
				else if( addrmode & Half )
					*a.ShortPtr += b.Short;
				else if( addrmode & Long )
					*a.Int32Ptr += b.Int32;
				else if( addrmode & Quad )
					*a.Int64Ptr += b.Int64;
			}
			continue;
		}
		exec_addr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			printf("addrmode == %" PRIu8 "\na == %" PRIu64 "\nb == %" PRIu64 "\n", addrmode, a.UInt64, b.UInt64);
			if( addrmode & Register )
				vm.reg[a.UInt64].Int64 += vm.reg[b.UInt64].Int64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					vm.reg[a.UInt64].Char += *(vm.reg[b.UInt64].CharPtr+offset);
				else if( addrmode & Half )
					vm.reg[a.UInt64].Short += *(int16_t *)(vm.reg[b.UInt64].CharPtr+offset);
				else if( addrmode & Long )
					vm.reg[a.UInt64].Int32 += *(int32_t *)(vm.reg[b.UInt64].CharPtr+offset);
				else if( addrmode & Quad )
					printf("*vm.reg[%s].Int64Ptr == %" PRIu64 "\nvm.reg[%s].Int64 == %" PRIu64 "\n", vm_regid_to_str(b.UInt64), *(int64_t *)(vm.reg[b.UInt64].CharPtr+offset), vm_regid_to_str(a.UInt64), vm.reg[a.UInt64].Int64),
					vm.reg[a.UInt64].Int64 += *(int64_t *)(vm.reg[b.UInt64].CharPtr+offset);
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					vm.reg[a.UInt64].Char += *b.CharPtr;
				else if( addrmode & Half )
					vm.reg[a.UInt64].Short += *b.ShortPtr;
				else if( addrmode & Long )
					vm.reg[a.UInt64].Int32 += *b.Int32Ptr;
				else if( addrmode & Quad )
					vm.reg[a.UInt64].Int64 += *b.Int64Ptr;
			}
			continue;
		}
		exec_addm:; {	// dest is memory and src is reg
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & (Register|RegIndirect) ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					*(vm.reg[a.UInt64].CharPtr+offset) += vm.reg[b.UInt64].Char;
				else if( addrmode & Half )
					*(int16_t *)(vm.reg[a.UInt64].UCharPtr+offset) += vm.reg[b.UInt64].Short;
				else if( addrmode & Long )
					*(int32_t *)(vm.reg[a.UInt64].UCharPtr+offset) += vm.reg[b.UInt64].Int32;
				else if( addrmode & Quad )
					*(int64_t *)(vm.reg[a.UInt64].UCharPtr+offset) += vm.reg[b.UInt64].Int64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					*a.CharPtr += vm.reg[b.UInt64].Char;
				else if( addrmode & Half )
					*a.ShortPtr += vm.reg[b.UInt64].Short;
				else if( addrmode & Long )
					*a.Int32Ptr += vm.reg[b.UInt64].Int32;
				else if( addrmode & Quad )
					*a.Int64Ptr += vm.reg[b.UInt64].Int64;
			}
			continue;
		}
		exec_uaddi:; {	// dest is reg or memory, src is imm
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.reg[a.UInt64].UInt64 += b.UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					*(vm.reg[a.UInt64].UCharPtr+offset) += b.UChar;
				else if( addrmode & Half )
					*(uint16_t *)(vm.reg[a.UInt64].UCharPtr+offset) += b.UShort;
				else if( addrmode & Long )
					*(uint32_t *)(vm.reg[a.UInt64].UCharPtr+offset) += b.UInt32;
				else if( addrmode & Quad )
					*(uint64_t *)(vm.reg[a.UInt64].UCharPtr+offset) += b.UInt64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					*a.UCharPtr += b.UChar;
				else if( addrmode & Half )
					*a.UShortPtr += b.UShort;
				else if( addrmode & Long )
					*a.UInt32Ptr += b.UInt32;
				else if( addrmode & Quad )
					*a.UInt64Ptr += b.UInt64;
			}
			continue;
		}
		exec_uaddr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.reg[a.UInt64].UInt64 += vm.reg[b.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					vm.reg[a.UInt64].UChar += *(vm.reg[b.UInt64].UCharPtr+offset);
				else if( addrmode & Half )
					vm.reg[a.UInt64].UShort += *(uint16_t *)(vm.reg[b.UInt64].UCharPtr+offset);
				else if( addrmode & Long )
					vm.reg[a.UInt64].UInt32 += *(uint32_t *)(vm.reg[b.UInt64].UCharPtr+offset);
				else if( addrmode & Quad )
					vm.reg[a.UInt64].UInt64 += *(uint64_t *)(vm.reg[b.UInt64].UCharPtr+offset);
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					vm.reg[a.UInt64].UChar += *b.UCharPtr;
				else if( addrmode & Half )
					vm.reg[a.UInt64].UShort += *b.UShortPtr;
				else if( addrmode & Long )
					vm.reg[a.UInt64].UInt32 += *b.UInt32Ptr;
				else if( addrmode & Quad )
					vm.reg[a.UInt64].UInt64 += *b.UInt64Ptr;
			}
			continue;
		}
		exec_uaddm:; {	// dest is memory and src is reg
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & (Register|RegIndirect) ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					*(vm.reg[a.UInt64].UCharPtr+offset) += vm.reg[b.UInt64].UChar;
				else if( addrmode & Half )
					*(uint16_t *)(vm.reg[a.UInt64].UCharPtr+offset) += vm.reg[b.UInt64].UShort;
				else if( addrmode & Long )
					*(uint32_t *)(vm.reg[a.UInt64].UCharPtr+offset) += vm.reg[b.UInt64].UInt32;
				else if( addrmode & Quad )
					*(uint64_t *)(vm.reg[a.UInt64].UCharPtr+offset) += vm.reg[b.UInt64].UInt64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					*a.UCharPtr += vm.reg[b.UInt64].UChar;
				else if( addrmode & Half )
					*a.UShortPtr += vm.reg[b.UInt64].UShort;
				else if( addrmode & Long )
					*a.UInt32Ptr += vm.reg[b.UInt64].UInt32;
				else if( addrmode & Quad )
					*a.UInt64Ptr += vm.reg[b.UInt64].UInt64;
			}
			continue;
		}
		
		exec_subi:; {	// dest is reg or memory, src is imm
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.reg[a.UInt64].Int64 -= b.Int64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					*(vm.reg[a.UInt64].CharPtr+offset) -= b.Char;
				else if( addrmode & Half )
					*(int16_t *)(vm.reg[a.UInt64].UCharPtr+offset) -= b.Short;
				else if( addrmode & Long )
					*(int32_t *)(vm.reg[a.UInt64].UCharPtr+offset) -= b.Int32;
				else if( addrmode & Quad )
					*(int64_t *)(vm.reg[a.UInt64].UCharPtr+offset) -= b.Int64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					*a.CharPtr -= b.Char;
				else if( addrmode & Half )
					*a.ShortPtr -= b.Short;
				else if( addrmode & Long )
					*a.Int32Ptr -= b.Int32;
				else if( addrmode & Quad )
					*a.Int64Ptr -= b.Int64;
			}
			continue;
		}
		exec_subr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.reg[a.UInt64].Int64 -= vm.reg[b.UInt64].Int64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					vm.reg[a.UInt64].Char -= *(vm.reg[b.UInt64].CharPtr+offset);
				else if( addrmode & Half )
					vm.reg[a.UInt64].Short -= *(int16_t *)(vm.reg[b.UInt64].CharPtr+offset);
				else if( addrmode & Long )
					vm.reg[a.UInt64].Int32 -= *(int32_t *)(vm.reg[b.UInt64].CharPtr+offset);
				else if( addrmode & Quad )
					vm.reg[a.UInt64].Int64 -= *(int64_t *)(vm.reg[b.UInt64].CharPtr+offset);
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					vm.reg[a.UInt64].Char -= *b.CharPtr;
				else if( addrmode & Half )
					vm.reg[a.UInt64].Short -= *b.ShortPtr;
				else if( addrmode & Long )
					vm.reg[a.UInt64].Int32 -= *b.Int32Ptr;
				else if( addrmode & Quad )
					vm.reg[a.UInt64].Int64 -= *b.Int64Ptr;
			}
			continue;
		}
		exec_subm:; {	// dest is memory and src is reg
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & (Register|RegIndirect) ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					*(vm.reg[a.UInt64].CharPtr+offset) -= vm.reg[b.UInt64].Char;
				else if( addrmode & Half )
					*(int16_t *)(vm.reg[a.UInt64].UCharPtr+offset) -= vm.reg[b.UInt64].Short;
				else if( addrmode & Long )
					*(int32_t *)(vm.reg[a.UInt64].UCharPtr+offset) -= vm.reg[b.UInt64].Int32;
				else if( addrmode & Quad )
					*(int64_t *)(vm.reg[a.UInt64].UCharPtr+offset) -= vm.reg[b.UInt64].Int64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					*a.CharPtr -= vm.reg[b.UInt64].Char;
				else if( addrmode & Half )
					*a.ShortPtr -= vm.reg[b.UInt64].Short;
				else if( addrmode & Long )
					*a.Int32Ptr -= vm.reg[b.UInt64].Int32;
				else if( addrmode & Quad )
					*a.Int64Ptr -= vm.reg[b.UInt64].Int64;
			}
			continue;
		}
		exec_usubi:; {	// dest is reg or memory, src is imm
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.reg[a.UInt64].UInt64 -= b.UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					*(vm.reg[a.UInt64].UCharPtr+offset) -= b.UChar;
				else if( addrmode & Half )
					*(uint16_t *)(vm.reg[a.UInt64].UCharPtr+offset) -= b.UShort;
				else if( addrmode & Long )
					*(uint32_t *)(vm.reg[a.UInt64].UCharPtr+offset) -= b.UInt32;
				else if( addrmode & Quad )
					*(uint64_t *)(vm.reg[a.UInt64].UCharPtr+offset) -= b.UInt64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					*a.UCharPtr -= b.UChar;
				else if( addrmode & Half )
					*a.UShortPtr -= b.UShort;
				else if( addrmode & Long )
					*a.UInt32Ptr -= b.UInt32;
				else if( addrmode & Quad )
					*a.UInt64Ptr -= b.UInt64;
			}
			continue;
		}
		exec_usubr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.reg[a.UInt64].UInt64 -= vm.reg[b.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					vm.reg[a.UInt64].UChar -= *(vm.reg[b.UInt64].UCharPtr+offset);
				else if( addrmode & Half )
					vm.reg[a.UInt64].UShort -= *(uint16_t *)(vm.reg[b.UInt64].UCharPtr+offset);
				else if( addrmode & Long )
					vm.reg[a.UInt64].UInt32 -= *(uint32_t *)(vm.reg[b.UInt64].UCharPtr+offset);
				else if( addrmode & Quad )
					vm.reg[a.UInt64].UInt64 -= *(uint64_t *)(vm.reg[b.UInt64].UCharPtr+offset);
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					vm.reg[a.UInt64].UChar -= *b.UCharPtr;
				else if( addrmode & Half )
					vm.reg[a.UInt64].UShort -= *b.UShortPtr;
				else if( addrmode & Long )
					vm.reg[a.UInt64].UInt32 -= *b.UInt32Ptr;
				else if( addrmode & Quad )
					vm.reg[a.UInt64].UInt64 -= *b.UInt64Ptr;
			}
			continue;
		}
		exec_usubm:; {	// dest is memory and src is reg
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & (Register|RegIndirect) ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					*(vm.reg[a.UInt64].UCharPtr+offset) -= vm.reg[b.UInt64].UChar;
				else if( addrmode & Half )
					*(uint16_t *)(vm.reg[a.UInt64].UCharPtr+offset) -= vm.reg[b.UInt64].UShort;
				else if( addrmode & Long )
					*(uint32_t *)(vm.reg[a.UInt64].UCharPtr+offset) -= vm.reg[b.UInt64].UInt32;
				else if( addrmode & Quad )
					*(uint64_t *)(vm.reg[a.UInt64].UCharPtr+offset) -= vm.reg[b.UInt64].UInt64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					*a.UCharPtr -= vm.reg[b.UInt64].UChar;
				else if( addrmode & Half )
					*a.UShortPtr -= vm.reg[b.UInt64].UShort;
				else if( addrmode & Long )
					*a.UInt32Ptr -= vm.reg[b.UInt64].UInt32;
				else if( addrmode & Quad )
					*a.UInt64Ptr -= vm.reg[b.UInt64].UInt64;
			}
			continue;
		}
		
		exec_muli:; {	// dest is reg or memory, src is imm
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.reg[a.UInt64].Int64 *= b.Int64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					*(vm.reg[a.UInt64].CharPtr+offset) *= b.Char;
				else if( addrmode & Half )
					*(int16_t *)(vm.reg[a.UInt64].UCharPtr+offset) *= b.Short;
				else if( addrmode & Long )
					*(int32_t *)(vm.reg[a.UInt64].UCharPtr+offset) *= b.Int32;
				else if( addrmode & Quad )
					*(int64_t *)(vm.reg[a.UInt64].UCharPtr+offset) *= b.Int64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					*a.CharPtr *= b.Char;
				else if( addrmode & Half )
					*a.ShortPtr *= b.Short;
				else if( addrmode & Long )
					*a.Int32Ptr *= b.Int32;
				else if( addrmode & Quad )
					*a.Int64Ptr *= b.Int64;
			}
			continue;
		}
		exec_mulr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.reg[a.UInt64].Int64 *= vm.reg[b.UInt64].Int64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					vm.reg[a.UInt64].Char *= *(vm.reg[b.UInt64].CharPtr+offset);
				else if( addrmode & Half )
					vm.reg[a.UInt64].Short *= *(int16_t *)(vm.reg[b.UInt64].CharPtr+offset);
				else if( addrmode & Long )
					vm.reg[a.UInt64].Int32 *= *(int32_t *)(vm.reg[b.UInt64].CharPtr+offset);
				else if( addrmode & Quad )
					vm.reg[a.UInt64].Int64 *= *(int64_t *)(vm.reg[b.UInt64].CharPtr+offset);
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					vm.reg[a.UInt64].Char *= *b.CharPtr;
				else if( addrmode & Half )
					vm.reg[a.UInt64].Short *= *b.ShortPtr;
				else if( addrmode & Long )
					vm.reg[a.UInt64].Int32 *= *b.Int32Ptr;
				else if( addrmode & Quad )
					vm.reg[a.UInt64].Int64 *= *b.Int64Ptr;
			}
			continue;
		}
		exec_mulm:; {	// dest is memory and src is reg
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & (Register|RegIndirect) ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					*(vm.reg[a.UInt64].CharPtr+offset) *= vm.reg[b.UInt64].Char;
				else if( addrmode & Half )
					*(int16_t *)(vm.reg[a.UInt64].UCharPtr+offset) *= vm.reg[b.UInt64].Short;
				else if( addrmode & Long )
					*(int32_t *)(vm.reg[a.UInt64].UCharPtr+offset) *= vm.reg[b.UInt64].Int32;
				else if( addrmode & Quad )
					*(int64_t *)(vm.reg[a.UInt64].UCharPtr+offset) *= vm.reg[b.UInt64].Int64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					*a.CharPtr *= vm.reg[b.UInt64].Char;
				else if( addrmode & Half )
					*a.ShortPtr *= vm.reg[b.UInt64].Short;
				else if( addrmode & Long )
					*a.Int32Ptr *= vm.reg[b.UInt64].Int32;
				else if( addrmode & Quad )
					*a.Int64Ptr *= vm.reg[b.UInt64].Int64;
			}
			continue;
		}
		exec_umuli:; {	// dest is reg or memory, src is imm
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.reg[a.UInt64].UInt64 *= b.UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					*(vm.reg[a.UInt64].UCharPtr+offset) *= b.UChar;
				else if( addrmode & Half )
					*(uint16_t *)(vm.reg[a.UInt64].UCharPtr+offset) *= b.UShort;
				else if( addrmode & Long )
					*(uint32_t *)(vm.reg[a.UInt64].UCharPtr+offset) *= b.UInt32;
				else if( addrmode & Quad )
					*(uint64_t *)(vm.reg[a.UInt64].UCharPtr+offset) *= b.UInt64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					*a.UCharPtr *= b.UChar;
				else if( addrmode & Half )
					*a.UShortPtr *= b.UShort;
				else if( addrmode & Long )
					*a.UInt32Ptr *= b.UInt32;
				else if( addrmode & Quad )
					*a.UInt64Ptr *= b.UInt64;
			}
			continue;
		}
		exec_umulr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.reg[a.UInt64].UInt64 *= vm.reg[b.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					vm.reg[a.UInt64].UChar *= *(vm.reg[b.UInt64].UCharPtr+offset);
				else if( addrmode & Half )
					vm.reg[a.UInt64].UShort *= *(uint16_t *)(vm.reg[b.UInt64].UCharPtr+offset);
				else if( addrmode & Long )
					vm.reg[a.UInt64].UInt32 *= *(uint32_t *)(vm.reg[b.UInt64].UCharPtr+offset);
				else if( addrmode & Quad )
					vm.reg[a.UInt64].UInt64 *= *(uint64_t *)(vm.reg[b.UInt64].UCharPtr+offset);
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					vm.reg[a.UInt64].UChar *= *b.UCharPtr;
				else if( addrmode & Half )
					vm.reg[a.UInt64].UShort *= *b.UShortPtr;
				else if( addrmode & Long )
					vm.reg[a.UInt64].UInt32 *= *b.UInt32Ptr;
				else if( addrmode & Quad )
					vm.reg[a.UInt64].UInt64 *= *b.UInt64Ptr;
			}
			continue;
		}
		exec_umulm:; {	// dest is memory and src is reg
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & (Register|RegIndirect) ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					*(vm.reg[a.UInt64].UCharPtr+offset) *= vm.reg[b.UInt64].UChar;
				else if( addrmode & Half )
					*(uint16_t *)(vm.reg[a.UInt64].UCharPtr+offset) *= vm.reg[b.UInt64].UShort;
				else if( addrmode & Long )
					*(uint32_t *)(vm.reg[a.UInt64].UCharPtr+offset) *= vm.reg[b.UInt64].UInt32;
				else if( addrmode & Quad )
					*(uint64_t *)(vm.reg[a.UInt64].UCharPtr+offset) *= vm.reg[b.UInt64].UInt64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					*a.UCharPtr *= vm.reg[b.UInt64].UChar;
				else if( addrmode & Half )
					*a.UShortPtr *= vm.reg[b.UInt64].UShort;
				else if( addrmode & Long )
					*a.UInt32Ptr *= vm.reg[b.UInt64].UInt32;
				else if( addrmode & Quad )
					*a.UInt64Ptr *= vm.reg[b.UInt64].UInt64;
			}
			continue;
		}
		
		exec_divi:; {	// dest is reg or memory, src is imm
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( !b.Int64 ) // prevent divide by 0
				b.Int64 = 1;
			if( addrmode & Register )
				vm.reg[a.UInt64].Int64 /= b.Int64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					*(vm.reg[a.UInt64].CharPtr+offset) /= b.Char;
				else if( addrmode & Half )
					*(int16_t *)(vm.reg[a.UInt64].UCharPtr+offset) /= b.Short;
				else if( addrmode & Long )
					*(int32_t *)(vm.reg[a.UInt64].UCharPtr+offset) /= b.Int32;
				else if( addrmode & Quad )
					*(int64_t *)(vm.reg[a.UInt64].UCharPtr+offset) /= b.Int64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					*a.CharPtr /= b.Char;
				else if( addrmode & Half )
					*a.ShortPtr /= b.Short;
				else if( addrmode & Long )
					*a.Int32Ptr /= b.Int32;
				else if( addrmode & Quad )
					*a.Int64Ptr /= b.Int64;
			}
			continue;
		}
		exec_divr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register ) {
				if( !vm.reg[b.UInt64].Int64 )
					vm.reg[b.UInt64].Int64 = 1;
				vm.reg[a.UInt64].Int64 /= vm.reg[b.UInt64].Int64;
			}
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte ) {
					if( !*(vm.reg[b.UInt64].CharPtr+offset) )
						*(vm.reg[b.UInt64].CharPtr+offset) = 1;
					vm.reg[a.UInt64].Char /= *(vm.reg[b.UInt64].CharPtr+offset);
				}
				else if( addrmode & Half ) {
					if( !*(int16_t *)(vm.reg[b.UInt64].CharPtr+offset) )
						*(int16_t *)(vm.reg[b.UInt64].CharPtr+offset) = 1;
					vm.reg[a.UInt64].Short /= *(int16_t *)(vm.reg[b.UInt64].CharPtr+offset);
				}
				else if( addrmode & Long ) {
					if( !*(int32_t *)(vm.reg[b.UInt64].CharPtr+offset) )
						*(int32_t *)(vm.reg[b.UInt64].CharPtr+offset) = 1;
					vm.reg[a.UInt64].Int32 /= *(int32_t *)(vm.reg[b.UInt64].CharPtr+offset);
				}
				else if( addrmode & Quad ) {
					if( !*(int64_t *)(vm.reg[b.UInt64].CharPtr+offset) )
						*(int64_t *)(vm.reg[b.UInt64].CharPtr+offset) = 1;
					vm.reg[a.UInt64].Int64 /= *(int64_t *)(vm.reg[b.UInt64].CharPtr+offset);
				}
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte ) {
					if( !*b.CharPtr )
						*b.CharPtr = 1;
					vm.reg[a.UInt64].Char /= *b.CharPtr;
				}
				else if( addrmode & Half ) {
					if( !*b.ShortPtr )
						*b.ShortPtr = 1;
					vm.reg[a.UInt64].Short /= *b.ShortPtr;
				}
				else if( addrmode & Long ) {
					if( !*b.Int32Ptr )
						*b.Int32Ptr = 1;
					vm.reg[a.UInt64].Int32 /= *b.Int32Ptr;
				}
				else if( addrmode & Quad ) {
					if( !*b.Int64Ptr )
						*b.Int64Ptr = 1;
					vm.reg[a.UInt64].Int64 /= *b.Int64Ptr;
				}
			}
			continue;
		}
		exec_divm:; {	// dest is memory and src is reg
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( !vm.reg[b.UInt64].Int64 )
				vm.reg[b.UInt64].Int64 = 1;
			if( addrmode & (Register|RegIndirect) ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					*(vm.reg[a.UInt64].CharPtr+offset) /= vm.reg[b.UInt64].Char;
				else if( addrmode & Half )
					*(int16_t *)(vm.reg[a.UInt64].UCharPtr+offset) /= vm.reg[b.UInt64].Short;
				else if( addrmode & Long )
					*(int32_t *)(vm.reg[a.UInt64].UCharPtr+offset) /= vm.reg[b.UInt64].Int32;
				else if( addrmode & Quad )
					*(int64_t *)(vm.reg[a.UInt64].UCharPtr+offset) /= vm.reg[b.UInt64].Int64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					*a.CharPtr /= vm.reg[b.UInt64].Char;
				else if( addrmode & Half )
					*a.ShortPtr /= vm.reg[b.UInt64].Short;
				else if( addrmode & Long )
					*a.Int32Ptr /= vm.reg[b.UInt64].Int32;
				else if( addrmode & Quad )
					*a.Int64Ptr /= vm.reg[b.UInt64].Int64;
			}
			continue;
		}
		exec_udivi:; {	// dest is reg or memory, src is imm
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( !b.UInt64 )
				b.UInt64 = 1;
			if( addrmode & Register )
				vm.reg[a.UInt64].UInt64 /= b.UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					*(vm.reg[a.UInt64].UCharPtr+offset) /= b.UChar;
				else if( addrmode & Half )
					*(uint16_t *)(vm.reg[a.UInt64].UCharPtr+offset) /= b.UShort;
				else if( addrmode & Long )
					*(uint32_t *)(vm.reg[a.UInt64].UCharPtr+offset) /= b.UInt32;
				else if( addrmode & Quad )
					*(uint64_t *)(vm.reg[a.UInt64].UCharPtr+offset) /= b.UInt64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					*a.UCharPtr /= b.UChar;
				else if( addrmode & Half )
					*a.UShortPtr /= b.UShort;
				else if( addrmode & Long )
					*a.UInt32Ptr /= b.UInt32;
				else if( addrmode & Quad )
					*a.UInt64Ptr /= b.UInt64;
			}
			continue;
		}
		exec_udivr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register ) {
				if( !vm.reg[b.UInt64].UInt64 )
					vm.reg[b.UInt64].UInt64 = 1;
				vm.reg[a.UInt64].UInt64 /= vm.reg[b.UInt64].UInt64;
			}
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte ) {
					if( !*(vm.reg[b.UInt64].UCharPtr+offset) )
						*(vm.reg[b.UInt64].UCharPtr+offset) = 1;
					vm.reg[a.UInt64].UChar /= *(vm.reg[b.UInt64].UCharPtr+offset);
				}
				else if( addrmode & Half ) {
					if( !*(uint16_t *)(vm.reg[b.UInt64].UCharPtr+offset) )
						*(uint16_t *)(vm.reg[b.UInt64].UCharPtr+offset) = 1;
					vm.reg[a.UInt64].UShort /= *(uint16_t *)(vm.reg[b.UInt64].UCharPtr+offset);
				}
				else if( addrmode & Long ) {
					if( !*(uint32_t *)(vm.reg[b.UInt64].UCharPtr+offset) )
						*(uint32_t *)(vm.reg[b.UInt64].UCharPtr+offset) = 1;
					vm.reg[a.UInt64].UInt32 /= *(uint32_t *)(vm.reg[b.UInt64].UCharPtr+offset);
				}
				else if( addrmode & Quad ) {
					if( !*(uint64_t *)(vm.reg[b.UInt64].UCharPtr+offset) )
						*(uint64_t *)(vm.reg[b.UInt64].UCharPtr+offset) = 1;
					vm.reg[a.UInt64].UInt64 /= *(uint64_t *)(vm.reg[b.UInt64].UCharPtr+offset);
				}
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte ) {
					if( !*b.UCharPtr )
						*b.UCharPtr = 1;
					vm.reg[a.UInt64].UChar /= *b.UCharPtr;
				}
				else if( addrmode & Half ) {
					if( !*b.UShortPtr )
						*b.UShortPtr = 1;
					vm.reg[a.UInt64].UShort /= *b.UShortPtr;
				}
				else if( addrmode & Long ) {
					if( !*b.UInt32Ptr )
						*b.UInt32Ptr = 1;
					vm.reg[a.UInt64].UInt32 /= *b.UInt32Ptr;
				}
				else if( addrmode & Quad ) {
					if( !*b.UInt64Ptr )
						*b.UInt64Ptr = 1;
					vm.reg[a.UInt64].UInt64 /= *b.UInt64Ptr;
				}
			}
			continue;
		}
		exec_udivm:; {	// dest is memory and src is reg
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( !vm.reg[b.UInt64].UInt64 )
				vm.reg[b.UInt64].UInt64 = 1;
			if( addrmode & (Register|RegIndirect) ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					*(vm.reg[a.UInt64].UCharPtr+offset) /= vm.reg[b.UInt64].UChar;
				else if( addrmode & Half )
					*(uint16_t *)(vm.reg[a.UInt64].UCharPtr+offset) /= vm.reg[b.UInt64].UShort;
				else if( addrmode & Long )
					*(uint32_t *)(vm.reg[a.UInt64].UCharPtr+offset) /= vm.reg[b.UInt64].UInt32;
				else if( addrmode & Quad )
					*(uint64_t *)(vm.reg[a.UInt64].UCharPtr+offset) /= vm.reg[b.UInt64].UInt64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					*a.UCharPtr /= vm.reg[b.UInt64].UChar;
				else if( addrmode & Half )
					*a.UShortPtr /= vm.reg[b.UInt64].UShort;
				else if( addrmode & Long )
					*a.UInt32Ptr /= vm.reg[b.UInt64].UInt32;
				else if( addrmode & Quad )
					*a.UInt64Ptr /= vm.reg[b.UInt64].UInt64;
			}
			continue;
		}
		
		exec_modi:; {	// dest is reg or memory, src is imm
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( !b.Int64 ) // prevent divide by 0
				b.Int64 = 1;
			if( addrmode & Register )
				vm.reg[a.UInt64].Int64 %= b.Int64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					*(vm.reg[a.UInt64].CharPtr+offset) %= b.Char;
				else if( addrmode & Half )
					*(int16_t *)(vm.reg[a.UInt64].UCharPtr+offset) %= b.Short;
				else if( addrmode & Long )
					*(int32_t *)(vm.reg[a.UInt64].UCharPtr+offset) %= b.Int32;
				else if( addrmode & Quad )
					*(int64_t *)(vm.reg[a.UInt64].UCharPtr+offset) %= b.Int64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					*a.CharPtr %= b.Char;
				else if( addrmode & Half )
					*a.ShortPtr %= b.Short;
				else if( addrmode & Long )
					*a.Int32Ptr %= b.Int32;
				else if( addrmode & Quad )
					*a.Int64Ptr %= b.Int64;
			}
			continue;
		}
		exec_modr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register ) {
				if( !vm.reg[b.UInt64].Int64 )
					vm.reg[b.UInt64].Int64 = 1;
				vm.reg[a.UInt64].Int64 %= vm.reg[b.UInt64].Int64;
			}
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte ) {
					if( !*(vm.reg[b.UInt64].CharPtr+offset) )
						*(vm.reg[b.UInt64].CharPtr+offset) = 1;
					vm.reg[a.UInt64].Char %= *(vm.reg[b.UInt64].CharPtr+offset);
				}
				else if( addrmode & Half ) {
					if( !*(int16_t *)(vm.reg[b.UInt64].CharPtr+offset) )
						*(int16_t *)(vm.reg[b.UInt64].CharPtr+offset) = 1;
					vm.reg[a.UInt64].Short %= *(int16_t *)(vm.reg[b.UInt64].CharPtr+offset);
				}
				else if( addrmode & Long ) {
					if( !*(int32_t *)(vm.reg[b.UInt64].CharPtr+offset) )
						*(int32_t *)(vm.reg[b.UInt64].CharPtr+offset) = 1;
					vm.reg[a.UInt64].Int32 %= *(int32_t *)(vm.reg[b.UInt64].CharPtr+offset);
				}
				else if( addrmode & Quad ) {
					if( !*(int64_t *)(vm.reg[b.UInt64].CharPtr+offset) )
						*(int64_t *)(vm.reg[b.UInt64].CharPtr+offset) = 1;
					vm.reg[a.UInt64].Int64 %= *(int64_t *)(vm.reg[b.UInt64].CharPtr+offset);
				}
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte ) {
					if( !*b.CharPtr )
						*b.CharPtr = 1;
					vm.reg[a.UInt64].Char %= *b.CharPtr;
				}
				else if( addrmode & Half ) {
					if( !*b.ShortPtr )
						*b.ShortPtr = 1;
					vm.reg[a.UInt64].Short %= *b.ShortPtr;
				}
				else if( addrmode & Long ) {
					if( !*b.Int32Ptr )
						*b.Int32Ptr = 1;
					vm.reg[a.UInt64].Int32 %= *b.Int32Ptr;
				}
				else if( addrmode & Quad ) {
					if( !*b.Int64Ptr )
						*b.Int64Ptr = 1;
					vm.reg[a.UInt64].Int64 %= *b.Int64Ptr;
				}
			}
			continue;
		}
		exec_modm:; {	// dest is memory and src is reg
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( !vm.reg[b.UInt64].Int64 )
				vm.reg[b.UInt64].Int64 = 1;
			if( addrmode & (Register|RegIndirect) ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					*(vm.reg[a.UInt64].CharPtr+offset) %= vm.reg[b.UInt64].Char;
				else if( addrmode & Half )
					*(int16_t *)(vm.reg[a.UInt64].UCharPtr+offset) %= vm.reg[b.UInt64].Short;
				else if( addrmode & Long )
					*(int32_t *)(vm.reg[a.UInt64].UCharPtr+offset) %= vm.reg[b.UInt64].Int32;
				else if( addrmode & Quad )
					*(int64_t *)(vm.reg[a.UInt64].UCharPtr+offset) %= vm.reg[b.UInt64].Int64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					*a.CharPtr %= vm.reg[b.UInt64].Char;
				else if( addrmode & Half )
					*a.ShortPtr %= vm.reg[b.UInt64].Short;
				else if( addrmode & Long )
					*a.Int32Ptr %= vm.reg[b.UInt64].Int32;
				else if( addrmode & Quad )
					*a.Int64Ptr %= vm.reg[b.UInt64].Int64;
			}
			continue;
		}
		exec_umodi:; {	// dest is reg or memory, src is imm
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( !b.UInt64 )
				b.UInt64 = 1;
			if( addrmode & Register )
				vm.reg[a.UInt64].UInt64 %= b.UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					*(vm.reg[a.UInt64].UCharPtr+offset) %= b.UChar;
				else if( addrmode & Half )
					*(uint16_t *)(vm.reg[a.UInt64].UCharPtr+offset) %= b.UShort;
				else if( addrmode & Long )
					*(uint32_t *)(vm.reg[a.UInt64].UCharPtr+offset) %= b.UInt32;
				else if( addrmode & Quad )
					*(uint64_t *)(vm.reg[a.UInt64].UCharPtr+offset) %= b.UInt64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					*a.UCharPtr %= b.UChar;
				else if( addrmode & Half )
					*a.UShortPtr %= b.UShort;
				else if( addrmode & Long )
					*a.UInt32Ptr %= b.UInt32;
				else if( addrmode & Quad )
					*a.UInt64Ptr %= b.UInt64;
			}
			continue;
		}
		exec_umodr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register ) {
				if( !vm.reg[b.UInt64].UInt64 )
					vm.reg[b.UInt64].UInt64 = 1;
				vm.reg[a.UInt64].UInt64 %= vm.reg[b.UInt64].UInt64;
			}
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte ) {
					if( !*(vm.reg[b.UInt64].UCharPtr+offset) )
						*(vm.reg[b.UInt64].UCharPtr+offset) = 1;
					vm.reg[a.UInt64].UChar %= *(vm.reg[b.UInt64].UCharPtr+offset);
				}
				else if( addrmode & Half ) {
					if( !*(uint16_t *)(vm.reg[b.UInt64].UCharPtr+offset) )
						*(uint16_t *)(vm.reg[b.UInt64].UCharPtr+offset) = 1;
					vm.reg[a.UInt64].UShort %= *(uint16_t *)(vm.reg[b.UInt64].UCharPtr+offset);
				}
				else if( addrmode & Long ) {
					if( !*(uint32_t *)(vm.reg[b.UInt64].UCharPtr+offset) )
						*(uint32_t *)(vm.reg[b.UInt64].UCharPtr+offset) = 1;
					vm.reg[a.UInt64].UInt32 %= *(uint32_t *)(vm.reg[b.UInt64].UCharPtr+offset);
				}
				else if( addrmode & Quad ) {
					if( !*(uint64_t *)(vm.reg[b.UInt64].UCharPtr+offset) )
						*(uint64_t *)(vm.reg[b.UInt64].UCharPtr+offset) = 1;
					vm.reg[a.UInt64].UInt64 %= *(uint64_t *)(vm.reg[b.UInt64].UCharPtr+offset);
				}
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte ) {
					if( !*b.UCharPtr )
						*b.UCharPtr = 1;
					vm.reg[a.UInt64].UChar %= *b.UCharPtr;
				}
				else if( addrmode & Half ) {
					if( !*b.UShortPtr )
						*b.UShortPtr = 1;
					vm.reg[a.UInt64].UShort %= *b.UShortPtr;
				}
				else if( addrmode & Long ) {
					if( !*b.UInt32Ptr )
						*b.UInt32Ptr = 1;
					vm.reg[a.UInt64].UInt32 %= *b.UInt32Ptr;
				}
				else if( addrmode & Quad ) {
					if( !*b.UInt64Ptr )
						*b.UInt64Ptr = 1;
					vm.reg[a.UInt64].UInt64 %= *b.UInt64Ptr;
				}
			}
			continue;
		}
		exec_umodm:; {	// dest is memory and src is reg
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( !vm.reg[b.UInt64].UInt64 )
				vm.reg[b.UInt64].UInt64 = 1;
			if( addrmode & (Register|RegIndirect) ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					*(vm.reg[a.UInt64].UCharPtr+offset) %= vm.reg[b.UInt64].UChar;
				else if( addrmode & Half )
					*(uint16_t *)(vm.reg[a.UInt64].UCharPtr+offset) %= vm.reg[b.UInt64].UShort;
				else if( addrmode & Long )
					*(uint32_t *)(vm.reg[a.UInt64].UCharPtr+offset) %= vm.reg[b.UInt64].UInt32;
				else if( addrmode & Quad )
					*(uint64_t *)(vm.reg[a.UInt64].UCharPtr+offset) %= vm.reg[b.UInt64].UInt64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					*a.UCharPtr %= vm.reg[b.UInt64].UChar;
				else if( addrmode & Half )
					*a.UShortPtr %= vm.reg[b.UInt64].UShort;
				else if( addrmode & Long )
					*a.UInt32Ptr %= vm.reg[b.UInt64].UInt32;
				else if( addrmode & Quad )
					*a.UInt64Ptr %= vm.reg[b.UInt64].UInt64;
			}
			continue;
		}
		
		exec_shri:; {	// dest is reg or memory, src is imm
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.reg[a.UInt64].UInt64 >>= b.UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					*(vm.reg[a.UInt64].UCharPtr+offset) >>= b.UChar;
				else if( addrmode & Half )
					*(uint16_t *)(vm.reg[a.UInt64].UCharPtr+offset) >>= b.UShort;
				else if( addrmode & Long )
					*(uint32_t *)(vm.reg[a.UInt64].UCharPtr+offset) >>= b.UInt32;
				else if( addrmode & Quad )
					*(uint64_t *)(vm.reg[a.UInt64].UCharPtr+offset) >>= b.UInt64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					*a.UCharPtr >>= b.UChar;
				else if( addrmode & Half )
					*a.UShortPtr >>= b.UShort;
				else if( addrmode & Long )
					*a.UInt32Ptr >>= b.UInt32;
				else if( addrmode & Quad )
					*a.UInt64Ptr >>= b.UInt64;
			}
			continue;
		}
		exec_shrr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.reg[a.UInt64].UInt64 >>= vm.reg[b.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					vm.reg[a.UInt64].UChar >>= *(vm.reg[b.UInt64].UCharPtr+offset);
				else if( addrmode & Half )
					vm.reg[a.UInt64].UShort >>= *(uint16_t *)(vm.reg[b.UInt64].UCharPtr+offset);
				else if( addrmode & Long )
					vm.reg[a.UInt64].UInt32 >>= *(uint32_t *)(vm.reg[b.UInt64].UCharPtr+offset);
				else if( addrmode & Quad )
					vm.reg[a.UInt64].UInt64 >>= *(uint64_t *)(vm.reg[b.UInt64].UCharPtr+offset);
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					vm.reg[a.UInt64].UChar >>= *b.UCharPtr;
				else if( addrmode & Half )
					vm.reg[a.UInt64].UShort >>= *b.UShortPtr;
				else if( addrmode & Long )
					vm.reg[a.UInt64].UInt32 >>= *b.UInt32Ptr;
				else if( addrmode & Quad )
					vm.reg[a.UInt64].UInt64 >>= *b.UInt64Ptr;
			}
			continue;
		}
		exec_shrm:; {	// dest is memory and src is reg
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & (Register|RegIndirect) ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					*(vm.reg[a.UInt64].UCharPtr+offset) >>= vm.reg[b.UInt64].UChar;
				else if( addrmode & Half )
					*(uint16_t *)(vm.reg[a.UInt64].UCharPtr+offset) >>= vm.reg[b.UInt64].UShort;
				else if( addrmode & Long )
					*(uint32_t *)(vm.reg[a.UInt64].UCharPtr+offset) >>= vm.reg[b.UInt64].UInt32;
				else if( addrmode & Quad )
					*(uint64_t *)(vm.reg[a.UInt64].UCharPtr+offset) >>= vm.reg[b.UInt64].UInt64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					*a.UCharPtr >>= vm.reg[b.UInt64].UChar;
				else if( addrmode & Half )
					*a.UShortPtr >>= vm.reg[b.UInt64].UShort;
				else if( addrmode & Long )
					*a.UInt32Ptr >>= vm.reg[b.UInt64].UInt32;
				else if( addrmode & Quad )
					*a.UInt64Ptr >>= vm.reg[b.UInt64].UInt64;
			}
			continue;
		}
		
		exec_shli:; {	// dest is reg or memory, src is imm
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.reg[a.UInt64].UInt64 <<= b.UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					*(vm.reg[a.UInt64].UCharPtr+offset) <<= b.UChar;
				else if( addrmode & Half )
					*(uint16_t *)(vm.reg[a.UInt64].UCharPtr+offset) <<= b.UShort;
				else if( addrmode & Long )
					*(uint32_t *)(vm.reg[a.UInt64].UCharPtr+offset) <<= b.UInt32;
				else if( addrmode & Quad )
					*(uint64_t *)(vm.reg[a.UInt64].UCharPtr+offset) <<= b.UInt64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					*a.UCharPtr <<= b.UChar;
				else if( addrmode & Half )
					*a.UShortPtr <<= b.UShort;
				else if( addrmode & Long )
					*a.UInt32Ptr <<= b.UInt32;
				else if( addrmode & Quad )
					*a.UInt64Ptr <<= b.UInt64;
			}
			continue;
		}
		exec_shlr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.reg[a.UInt64].UInt64 <<= vm.reg[b.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					vm.reg[a.UInt64].UChar <<= *(vm.reg[b.UInt64].UCharPtr+offset);
				else if( addrmode & Half )
					vm.reg[a.UInt64].UShort <<= *(uint16_t *)(vm.reg[b.UInt64].UCharPtr+offset);
				else if( addrmode & Long )
					vm.reg[a.UInt64].UInt32 <<= *(uint32_t *)(vm.reg[b.UInt64].UCharPtr+offset);
				else if( addrmode & Quad )
					vm.reg[a.UInt64].UInt64 <<= *(uint64_t *)(vm.reg[b.UInt64].UCharPtr+offset);
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					vm.reg[a.UInt64].UChar <<= *b.UCharPtr;
				else if( addrmode & Half )
					vm.reg[a.UInt64].UShort <<= *b.UShortPtr;
				else if( addrmode & Long )
					vm.reg[a.UInt64].UInt32 <<= *b.UInt32Ptr;
				else if( addrmode & Quad )
					vm.reg[a.UInt64].UInt64 <<= *b.UInt64Ptr;
			}
			continue;
		}
		exec_shlm:; {	// dest is memory and src is reg
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & (Register|RegIndirect) ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					*(vm.reg[a.UInt64].UCharPtr+offset) <<= vm.reg[b.UInt64].UChar;
				else if( addrmode & Half )
					*(uint16_t *)(vm.reg[a.UInt64].UCharPtr+offset) <<= vm.reg[b.UInt64].UShort;
				else if( addrmode & Long )
					*(uint32_t *)(vm.reg[a.UInt64].UCharPtr+offset) <<= vm.reg[b.UInt64].UInt32;
				else if( addrmode & Quad )
					*(uint64_t *)(vm.reg[a.UInt64].UCharPtr+offset) <<= vm.reg[b.UInt64].UInt64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					*a.UCharPtr <<= vm.reg[b.UInt64].UChar;
				else if( addrmode & Half )
					*a.UShortPtr <<= vm.reg[b.UInt64].UShort;
				else if( addrmode & Long )
					*a.UInt32Ptr <<= vm.reg[b.UInt64].UInt32;
				else if( addrmode & Quad )
					*a.UInt64Ptr <<= vm.reg[b.UInt64].UInt64;
			}
			continue;
		}
		
		exec_andi:; {	// dest is reg or memory, src is imm
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.reg[a.UInt64].UInt64 &= b.UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					*(vm.reg[a.UInt64].UCharPtr+offset) &= b.UChar;
				else if( addrmode & Half )
					*(uint16_t *)(vm.reg[a.UInt64].UCharPtr+offset) &= b.UShort;
				else if( addrmode & Long )
					*(uint32_t *)(vm.reg[a.UInt64].UCharPtr+offset) &= b.UInt32;
				else if( addrmode & Quad )
					*(uint64_t *)(vm.reg[a.UInt64].UCharPtr+offset) &= b.UInt64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					*a.UCharPtr &= b.UChar;
				else if( addrmode & Half )
					*a.UShortPtr &= b.UShort;
				else if( addrmode & Long )
					*a.UInt32Ptr &= b.UInt32;
				else if( addrmode & Quad )
					*a.UInt64Ptr &= b.UInt64;
			}
			continue;
		}
		exec_andr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.reg[a.UInt64].UInt64 &= vm.reg[b.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					vm.reg[a.UInt64].UChar &= *(vm.reg[b.UInt64].UCharPtr+offset);
				else if( addrmode & Half )
					vm.reg[a.UInt64].UShort &= *(uint16_t *)(vm.reg[b.UInt64].UCharPtr+offset);
				else if( addrmode & Long )
					vm.reg[a.UInt64].UInt32 &= *(uint32_t *)(vm.reg[b.UInt64].UCharPtr+offset);
				else if( addrmode & Quad )
					vm.reg[a.UInt64].UInt64 &= *(uint64_t *)(vm.reg[b.UInt64].UCharPtr+offset);
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					vm.reg[a.UInt64].UChar &= *b.UCharPtr;
				else if( addrmode & Half )
					vm.reg[a.UInt64].UShort &= *b.UShortPtr;
				else if( addrmode & Long )
					vm.reg[a.UInt64].UInt32 &= *b.UInt32Ptr;
				else if( addrmode & Quad )
					vm.reg[a.UInt64].UInt64 &= *b.UInt64Ptr;
			}
			continue;
		}
		exec_andm:; {	// dest is memory and src is reg
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & (Register|RegIndirect) ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					*(vm.reg[a.UInt64].UCharPtr+offset) &= vm.reg[b.UInt64].UChar;
				else if( addrmode & Half )
					*(uint16_t *)(vm.reg[a.UInt64].UCharPtr+offset) &= vm.reg[b.UInt64].UShort;
				else if( addrmode & Long )
					*(uint32_t *)(vm.reg[a.UInt64].UCharPtr+offset) &= vm.reg[b.UInt64].UInt32;
				else if( addrmode & Quad )
					*(uint64_t *)(vm.reg[a.UInt64].UCharPtr+offset) &= vm.reg[b.UInt64].UInt64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					*a.UCharPtr &= vm.reg[b.UInt64].UChar;
				else if( addrmode & Half )
					*a.UShortPtr &= vm.reg[b.UInt64].UShort;
				else if( addrmode & Long )
					*a.UInt32Ptr &= vm.reg[b.UInt64].UInt32;
				else if( addrmode & Quad )
					*a.UInt64Ptr &= vm.reg[b.UInt64].UInt64;
			}
			continue;
		}
		
		exec_ori:; {	// dest is reg or memory, src is imm
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.reg[a.UInt64].UInt64 |= b.UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					*(vm.reg[a.UInt64].UCharPtr+offset) |= b.UChar;
				else if( addrmode & Half )
					*(uint16_t *)(vm.reg[a.UInt64].UCharPtr+offset) |= b.UShort;
				else if( addrmode & Long )
					*(uint32_t *)(vm.reg[a.UInt64].UCharPtr+offset) |= b.UInt32;
				else if( addrmode & Quad )
					*(uint64_t *)(vm.reg[a.UInt64].UCharPtr+offset) |= b.UInt64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					*a.UCharPtr |= b.UChar;
				else if( addrmode & Half )
					*a.UShortPtr |= b.UShort;
				else if( addrmode & Long )
					*a.UInt32Ptr |= b.UInt32;
				else if( addrmode & Quad )
					*a.UInt64Ptr |= b.UInt64;
			}
			continue;
		}
		exec_orr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.reg[a.UInt64].UInt64 |= vm.reg[b.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					vm.reg[a.UInt64].UChar |= *(vm.reg[b.UInt64].UCharPtr+offset);
				else if( addrmode & Half )
					vm.reg[a.UInt64].UShort |= *(uint16_t *)(vm.reg[b.UInt64].UCharPtr+offset);
				else if( addrmode & Long )
					vm.reg[a.UInt64].UInt32 |= *(uint32_t *)(vm.reg[b.UInt64].UCharPtr+offset);
				else if( addrmode & Quad )
					vm.reg[a.UInt64].UInt64 |= *(uint64_t *)(vm.reg[b.UInt64].UCharPtr+offset);
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					vm.reg[a.UInt64].UChar |= *b.UCharPtr;
				else if( addrmode & Half )
					vm.reg[a.UInt64].UShort |= *b.UShortPtr;
				else if( addrmode & Long )
					vm.reg[a.UInt64].UInt32 |= *b.UInt32Ptr;
				else if( addrmode & Quad )
					vm.reg[a.UInt64].UInt64 |= *b.UInt64Ptr;
			}
			continue;
		}
		exec_orm:; {	// dest is memory and src is reg
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & (Register|RegIndirect) ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					*(vm.reg[a.UInt64].UCharPtr+offset) |= vm.reg[b.UInt64].UChar;
				else if( addrmode & Half )
					*(uint16_t *)(vm.reg[a.UInt64].UCharPtr+offset) |= vm.reg[b.UInt64].UShort;
				else if( addrmode & Long )
					*(uint32_t *)(vm.reg[a.UInt64].UCharPtr+offset) |= vm.reg[b.UInt64].UInt32;
				else if( addrmode & Quad )
					*(uint64_t *)(vm.reg[a.UInt64].UCharPtr+offset) |= vm.reg[b.UInt64].UInt64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					*a.UCharPtr |= vm.reg[b.UInt64].UChar;
				else if( addrmode & Half )
					*a.UShortPtr |= vm.reg[b.UInt64].UShort;
				else if( addrmode & Long )
					*a.UInt32Ptr |= vm.reg[b.UInt64].UInt32;
				else if( addrmode & Quad )
					*a.UInt64Ptr |= vm.reg[b.UInt64].UInt64;
			}
			continue;
		}
		
		exec_xori:; {	// dest is reg or memory, src is imm
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.reg[a.UInt64].UInt64 ^= b.UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					*(vm.reg[a.UInt64].UCharPtr+offset) ^= b.UChar;
				else if( addrmode & Half )
					*(uint16_t *)(vm.reg[a.UInt64].UCharPtr+offset) ^= b.UShort;
				else if( addrmode & Long )
					*(uint32_t *)(vm.reg[a.UInt64].UCharPtr+offset) ^= b.UInt32;
				else if( addrmode & Quad )
					*(uint64_t *)(vm.reg[a.UInt64].UCharPtr+offset) ^= b.UInt64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					*a.UCharPtr ^= b.UChar;
				else if( addrmode & Half )
					*a.UShortPtr ^= b.UShort;
				else if( addrmode & Long )
					*a.UInt32Ptr ^= b.UInt32;
				else if( addrmode & Quad )
					*a.UInt64Ptr ^= b.UInt64;
			}
			continue;
		}
		exec_xorr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.reg[a.UInt64].UInt64 ^= vm.reg[b.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					vm.reg[a.UInt64].UChar ^= *(vm.reg[b.UInt64].UCharPtr+offset);
				else if( addrmode & Half )
					vm.reg[a.UInt64].UShort ^= *(uint16_t *)(vm.reg[b.UInt64].UCharPtr+offset);
				else if( addrmode & Long )
					vm.reg[a.UInt64].UInt32 ^= *(uint32_t *)(vm.reg[b.UInt64].UCharPtr+offset);
				else if( addrmode & Quad )
					vm.reg[a.UInt64].UInt64 ^= *(uint64_t *)(vm.reg[b.UInt64].UCharPtr+offset);
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					vm.reg[a.UInt64].UChar ^= *b.UCharPtr;
				else if( addrmode & Half )
					vm.reg[a.UInt64].UShort ^= *b.UShortPtr;
				else if( addrmode & Long )
					vm.reg[a.UInt64].UInt32 ^= *b.UInt32Ptr;
				else if( addrmode & Quad )
					vm.reg[a.UInt64].UInt64 ^= *b.UInt64Ptr;
			}
			continue;
		}
		exec_xorm:; {	// dest is memory and src is reg
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & (Register|RegIndirect) ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					*(vm.reg[a.UInt64].UCharPtr+offset) ^= vm.reg[b.UInt64].UChar;
				else if( addrmode & Half )
					*(uint16_t *)(vm.reg[a.UInt64].UCharPtr+offset) ^= vm.reg[b.UInt64].UShort;
				else if( addrmode & Long )
					*(uint32_t *)(vm.reg[a.UInt64].UCharPtr+offset) ^= vm.reg[b.UInt64].UInt32;
				else if( addrmode & Quad )
					*(uint64_t *)(vm.reg[a.UInt64].UCharPtr+offset) ^= vm.reg[b.UInt64].UInt64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					*a.UCharPtr ^= vm.reg[b.UInt64].UChar;
				else if( addrmode & Half )
					*a.UShortPtr ^= vm.reg[b.UInt64].UShort;
				else if( addrmode & Long )
					*a.UInt32Ptr ^= vm.reg[b.UInt64].UInt32;
				else if( addrmode & Quad )
					*a.UInt64Ptr ^= vm.reg[b.UInt64].UInt64;
			}
			continue;
		}
		
		exec_lti:; {	// dest is reg or memory, src is imm
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.zero_flag = vm.reg[a.UInt64].Int64 < b.Int64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					vm.zero_flag = *(vm.reg[a.UInt64].CharPtr+offset) < b.Char;
				else if( addrmode & Half )
					vm.zero_flag = *(int16_t *)(vm.reg[a.UInt64].UCharPtr+offset) < b.Short;
				else if( addrmode & Long )
					vm.zero_flag = *(int32_t *)(vm.reg[a.UInt64].UCharPtr+offset) < b.Int32;
				else if( addrmode & Quad )
					vm.zero_flag = *(int64_t *)(vm.reg[a.UInt64].UCharPtr+offset) < b.Int64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					vm.zero_flag = *a.CharPtr < b.Char;
				else if( addrmode & Half )
					vm.zero_flag = *a.ShortPtr < b.Short;
				else if( addrmode & Long )
					vm.zero_flag = *a.Int32Ptr < b.Int32;
				else if( addrmode & Quad )
					vm.zero_flag = *a.Int64Ptr < b.Int64;
			}
			continue;
		}
		exec_ltr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.zero_flag = vm.reg[a.UInt64].Int64 < vm.reg[b.UInt64].Int64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					vm.zero_flag = vm.reg[a.UInt64].Char < *(vm.reg[b.UInt64].CharPtr+offset);
				else if( addrmode & Half )
					vm.zero_flag = vm.reg[a.UInt64].Short < *(int16_t *)(vm.reg[b.UInt64].CharPtr+offset);
				else if( addrmode & Long )
					vm.zero_flag = vm.reg[a.UInt64].Int32 < *(int32_t *)(vm.reg[b.UInt64].CharPtr+offset);
				else if( addrmode & Quad )
					vm.zero_flag = vm.reg[a.UInt64].Int64 < *(int64_t *)(vm.reg[b.UInt64].CharPtr+offset);
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					vm.zero_flag = vm.reg[a.UInt64].Char < *b.CharPtr;
				else if( addrmode & Half )
					vm.zero_flag = vm.reg[a.UInt64].Short < *b.ShortPtr;
				else if( addrmode & Long )
					vm.zero_flag = vm.reg[a.UInt64].Int32 < *b.Int32Ptr;
				else if( addrmode & Quad )
					vm.zero_flag = vm.reg[a.UInt64].Int64 < *b.Int64Ptr;
			}
			continue;
		}
		exec_ltm:; {	// dest is memory and src is reg
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & (Register|RegIndirect) ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					vm.zero_flag = *(vm.reg[a.UInt64].CharPtr+offset) < vm.reg[b.UInt64].Char;
				else if( addrmode & Half )
					vm.zero_flag = *(int16_t *)(vm.reg[a.UInt64].UCharPtr+offset) < vm.reg[b.UInt64].Short;
				else if( addrmode & Long )
					vm.zero_flag = *(int32_t *)(vm.reg[a.UInt64].UCharPtr+offset) < vm.reg[b.UInt64].Int32;
				else if( addrmode & Quad )
					vm.zero_flag = *(int64_t *)(vm.reg[a.UInt64].UCharPtr+offset) < vm.reg[b.UInt64].Int64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					vm.zero_flag = *a.CharPtr < vm.reg[b.UInt64].Char;
				else if( addrmode & Half )
					vm.zero_flag = *a.ShortPtr < vm.reg[b.UInt64].Short;
				else if( addrmode & Long )
					vm.zero_flag = *a.Int32Ptr < vm.reg[b.UInt64].Int32;
				else if( addrmode & Quad )
					vm.zero_flag = *a.Int64Ptr < vm.reg[b.UInt64].Int64;
			}
			continue;
		}
		exec_ulti:; {	// dest is reg or memory, src is imm
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.zero_flag = vm.reg[a.UInt64].UInt64 < b.UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					vm.zero_flag = *(vm.reg[a.UInt64].UCharPtr+offset) < b.UChar;
				else if( addrmode & Half )
					vm.zero_flag = *(uint16_t *)(vm.reg[a.UInt64].UCharPtr+offset) < b.UShort;
				else if( addrmode & Long )
					vm.zero_flag = *(uint32_t *)(vm.reg[a.UInt64].UCharPtr+offset) < b.UInt32;
				else if( addrmode & Quad )
					vm.zero_flag = *(uint64_t *)(vm.reg[a.UInt64].UCharPtr+offset) < b.UInt64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					vm.zero_flag = *a.UCharPtr < b.UChar;
				else if( addrmode & Half )
					vm.zero_flag = *a.UShortPtr < b.UShort;
				else if( addrmode & Long )
					vm.zero_flag = *a.UInt32Ptr < b.UInt32;
				else if( addrmode & Quad )
					vm.zero_flag = *a.UInt64Ptr < b.UInt64;
			}
			continue;
		}
		exec_ultr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.zero_flag = vm.reg[a.UInt64].UInt64 < vm.reg[b.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					vm.zero_flag = vm.reg[a.UInt64].UChar < *(vm.reg[b.UInt64].UCharPtr+offset);
				else if( addrmode & Half )
					vm.zero_flag = vm.reg[a.UInt64].UShort < *(uint16_t *)(vm.reg[b.UInt64].UCharPtr+offset);
				else if( addrmode & Long )
					vm.zero_flag = vm.reg[a.UInt64].UInt32 < *(uint32_t *)(vm.reg[b.UInt64].UCharPtr+offset);
				else if( addrmode & Quad )
					vm.zero_flag = vm.reg[a.UInt64].UInt64 < *(uint64_t *)(vm.reg[b.UInt64].UCharPtr+offset);
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					vm.zero_flag = vm.reg[a.UInt64].UChar < *b.UCharPtr;
				else if( addrmode & Half )
					vm.zero_flag = vm.reg[a.UInt64].UShort < *b.UShortPtr;
				else if( addrmode & Long )
					vm.zero_flag = vm.reg[a.UInt64].UInt32 < *b.UInt32Ptr;
				else if( addrmode & Quad )
					vm.zero_flag = vm.reg[a.UInt64].UInt64 < *b.UInt64Ptr;
			}
			continue;
		}
		exec_ultm:; {	// dest is memory and src is reg
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & (Register|RegIndirect) ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					vm.zero_flag = *(vm.reg[a.UInt64].UCharPtr+offset) < vm.reg[b.UInt64].UChar;
				else if( addrmode & Half )
					vm.zero_flag = *(uint16_t *)(vm.reg[a.UInt64].UCharPtr+offset) < vm.reg[b.UInt64].UShort;
				else if( addrmode & Long )
					vm.zero_flag = *(uint32_t *)(vm.reg[a.UInt64].UCharPtr+offset) < vm.reg[b.UInt64].UInt32;
				else if( addrmode & Quad )
					vm.zero_flag = *(uint64_t *)(vm.reg[a.UInt64].UCharPtr+offset) < vm.reg[b.UInt64].UInt64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					vm.zero_flag = *a.UCharPtr < vm.reg[b.UInt64].UChar;
				else if( addrmode & Half )
					vm.zero_flag = *a.UShortPtr < vm.reg[b.UInt64].UShort;
				else if( addrmode & Long )
					vm.zero_flag = *a.UInt32Ptr < vm.reg[b.UInt64].UInt32;
				else if( addrmode & Quad )
					vm.zero_flag = *a.UInt64Ptr < vm.reg[b.UInt64].UInt64;
			}
			continue;
		}
		
		exec_gti:; {	// dest is reg or memory, src is imm
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.zero_flag = vm.reg[a.UInt64].Int64 > b.Int64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					vm.zero_flag = *(vm.reg[a.UInt64].CharPtr+offset) > b.Char;
				else if( addrmode & Half )
					vm.zero_flag = *(int16_t *)(vm.reg[a.UInt64].UCharPtr+offset) > b.Short;
				else if( addrmode & Long )
					vm.zero_flag = *(int32_t *)(vm.reg[a.UInt64].UCharPtr+offset) > b.Int32;
				else if( addrmode & Quad )
					vm.zero_flag = *(int64_t *)(vm.reg[a.UInt64].UCharPtr+offset) > b.Int64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					vm.zero_flag = *a.CharPtr > b.Char;
				else if( addrmode & Half )
					vm.zero_flag = *a.ShortPtr > b.Short;
				else if( addrmode & Long )
					vm.zero_flag = *a.Int32Ptr > b.Int32;
				else if( addrmode & Quad )
					vm.zero_flag = *a.Int64Ptr > b.Int64;
			}
			continue;
		}
		exec_gtr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.zero_flag = vm.reg[a.UInt64].Int64 > vm.reg[b.UInt64].Int64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					vm.zero_flag = vm.reg[a.UInt64].Char > *(vm.reg[b.UInt64].CharPtr+offset);
				else if( addrmode & Half )
					vm.zero_flag = vm.reg[a.UInt64].Short > *(int16_t *)(vm.reg[b.UInt64].CharPtr+offset);
				else if( addrmode & Long )
					vm.zero_flag = vm.reg[a.UInt64].Int32 > *(int32_t *)(vm.reg[b.UInt64].CharPtr+offset);
				else if( addrmode & Quad )
					vm.zero_flag = vm.reg[a.UInt64].Int64 > *(int64_t *)(vm.reg[b.UInt64].CharPtr+offset);
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					vm.zero_flag = vm.reg[a.UInt64].Char > *b.CharPtr;
				else if( addrmode & Half )
					vm.zero_flag = vm.reg[a.UInt64].Short > *b.ShortPtr;
				else if( addrmode & Long )
					vm.zero_flag = vm.reg[a.UInt64].Int32 > *b.Int32Ptr;
				else if( addrmode & Quad )
					vm.zero_flag = vm.reg[a.UInt64].Int64 > *b.Int64Ptr;
			}
			continue;
		}
		exec_gtm:; {	// dest is memory and src is reg
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & (Register|RegIndirect) ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					vm.zero_flag = *(vm.reg[a.UInt64].CharPtr+offset) > vm.reg[b.UInt64].Char;
				else if( addrmode & Half )
					vm.zero_flag = *(int16_t *)(vm.reg[a.UInt64].UCharPtr+offset) > vm.reg[b.UInt64].Short;
				else if( addrmode & Long )
					vm.zero_flag = *(int32_t *)(vm.reg[a.UInt64].UCharPtr+offset) > vm.reg[b.UInt64].Int32;
				else if( addrmode & Quad )
					vm.zero_flag = *(int64_t *)(vm.reg[a.UInt64].UCharPtr+offset) > vm.reg[b.UInt64].Int64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					vm.zero_flag = *a.CharPtr > vm.reg[b.UInt64].Char;
				else if( addrmode & Half )
					vm.zero_flag = *a.ShortPtr > vm.reg[b.UInt64].Short;
				else if( addrmode & Long )
					vm.zero_flag = *a.Int32Ptr > vm.reg[b.UInt64].Int32;
				else if( addrmode & Quad )
					vm.zero_flag = *a.Int64Ptr > vm.reg[b.UInt64].Int64;
			}
			continue;
		}
		exec_ugti:; {	// dest is reg or memory, src is imm
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.zero_flag = vm.reg[a.UInt64].UInt64 > b.UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					vm.zero_flag = *(vm.reg[a.UInt64].UCharPtr+offset) > b.UChar;
				else if( addrmode & Half )
					vm.zero_flag = *(uint16_t *)(vm.reg[a.UInt64].UCharPtr+offset) > b.UShort;
				else if( addrmode & Long )
					vm.zero_flag = *(uint32_t *)(vm.reg[a.UInt64].UCharPtr+offset) > b.UInt32;
				else if( addrmode & Quad )
					vm.zero_flag = *(uint64_t *)(vm.reg[a.UInt64].UCharPtr+offset) > b.UInt64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					vm.zero_flag = *a.UCharPtr > b.UChar;
				else if( addrmode & Half )
					vm.zero_flag = *a.UShortPtr > b.UShort;
				else if( addrmode & Long )
					vm.zero_flag = *a.UInt32Ptr > b.UInt32;
				else if( addrmode & Quad )
					vm.zero_flag = *a.UInt64Ptr > b.UInt64;
			}
			continue;
		}
		exec_ugtr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.zero_flag = vm.reg[a.UInt64].UInt64 > vm.reg[b.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					vm.zero_flag = vm.reg[a.UInt64].UChar > *(vm.reg[b.UInt64].UCharPtr+offset);
				else if( addrmode & Half )
					vm.zero_flag = vm.reg[a.UInt64].UShort > *(uint16_t *)(vm.reg[b.UInt64].UCharPtr+offset);
				else if( addrmode & Long )
					vm.zero_flag = vm.reg[a.UInt64].UInt32 > *(uint32_t *)(vm.reg[b.UInt64].UCharPtr+offset);
				else if( addrmode & Quad )
					vm.zero_flag = vm.reg[a.UInt64].UInt64 > *(uint64_t *)(vm.reg[b.UInt64].UCharPtr+offset);
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					vm.zero_flag = vm.reg[a.UInt64].UChar > *b.UCharPtr;
				else if( addrmode & Half )
					vm.zero_flag = vm.reg[a.UInt64].UShort > *b.UShortPtr;
				else if( addrmode & Long )
					vm.zero_flag = vm.reg[a.UInt64].UInt32 > *b.UInt32Ptr;
				else if( addrmode & Quad )
					vm.zero_flag = vm.reg[a.UInt64].UInt64 > *b.UInt64Ptr;
			}
			break;
		}
		exec_ugtm:; {	// dest is memory and src is reg
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & (Register|RegIndirect) ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					vm.zero_flag = *(vm.reg[a.UInt64].UCharPtr+offset) > vm.reg[b.UInt64].UChar;
				else if( addrmode & Half )
					vm.zero_flag = *(uint16_t *)(vm.reg[a.UInt64].UCharPtr+offset) > vm.reg[b.UInt64].UShort;
				else if( addrmode & Long )
					vm.zero_flag = *(uint32_t *)(vm.reg[a.UInt64].UCharPtr+offset) > vm.reg[b.UInt64].UInt32;
				else if( addrmode & Quad )
					vm.zero_flag = *(uint64_t *)(vm.reg[a.UInt64].UCharPtr+offset) > vm.reg[b.UInt64].UInt64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					vm.zero_flag = *a.UCharPtr > vm.reg[b.UInt64].UChar;
				else if( addrmode & Half )
					vm.zero_flag = *a.UShortPtr > vm.reg[b.UInt64].UShort;
				else if( addrmode & Long )
					vm.zero_flag = *a.UInt32Ptr > vm.reg[b.UInt64].UInt32;
				else if( addrmode & Quad )
					vm.zero_flag = *a.UInt64Ptr > vm.reg[b.UInt64].UInt64;
			}
			continue;
		}
		
		exec_cmpi:; {	// dest is reg or memory, src is imm
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.zero_flag = vm.reg[a.UInt64].Int64 == b.Int64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					vm.zero_flag = *(vm.reg[a.UInt64].CharPtr+offset) == b.Char;
				else if( addrmode & Half )
					vm.zero_flag = *(int16_t *)(vm.reg[a.UInt64].UCharPtr+offset) == b.Short;
				else if( addrmode & Long )
					vm.zero_flag = *(int32_t *)(vm.reg[a.UInt64].UCharPtr+offset) == b.Int32;
				else if( addrmode & Quad )
					vm.zero_flag = *(int64_t *)(vm.reg[a.UInt64].UCharPtr+offset) == b.Int64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					vm.zero_flag = *a.CharPtr == b.Char;
				else if( addrmode & Half )
					vm.zero_flag = *a.ShortPtr == b.Short;
				else if( addrmode & Long )
					vm.zero_flag = *a.Int32Ptr == b.Int32;
				else if( addrmode & Quad )
					vm.zero_flag = *a.Int64Ptr == b.Int64;
			}
			continue;
		}
		exec_cmpr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.zero_flag = vm.reg[a.UInt64].Int64 == vm.reg[b.UInt64].Int64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					vm.zero_flag = vm.reg[a.UInt64].Char == *(vm.reg[b.UInt64].CharPtr+offset);
				else if( addrmode & Half )
					vm.zero_flag = vm.reg[a.UInt64].Short == *(int16_t *)(vm.reg[b.UInt64].CharPtr+offset);
				else if( addrmode & Long )
					vm.zero_flag = vm.reg[a.UInt64].Int32 == *(int32_t *)(vm.reg[b.UInt64].CharPtr+offset);
				else if( addrmode & Quad )
					vm.zero_flag = vm.reg[a.UInt64].Int64 == *(int64_t *)(vm.reg[b.UInt64].CharPtr+offset);
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					vm.zero_flag = vm.reg[a.UInt64].Char == *b.CharPtr;
				else if( addrmode & Half )
					vm.zero_flag = vm.reg[a.UInt64].Short == *b.ShortPtr;
				else if( addrmode & Long )
					vm.zero_flag = vm.reg[a.UInt64].Int32 == *b.Int32Ptr;
				else if( addrmode & Quad )
					vm.zero_flag = vm.reg[a.UInt64].Int64 == *b.Int64Ptr;
			}
			continue;
		}
		exec_cmpm:; {	// dest is memory and src is reg
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & (Register|RegIndirect) ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					vm.zero_flag = *(vm.reg[a.UInt64].CharPtr+offset) == vm.reg[b.UInt64].Char;
				else if( addrmode & Half )
					vm.zero_flag = *(int16_t *)(vm.reg[a.UInt64].UCharPtr+offset) == vm.reg[b.UInt64].Short;
				else if( addrmode & Long )
					vm.zero_flag = *(int32_t *)(vm.reg[a.UInt64].UCharPtr+offset) == vm.reg[b.UInt64].Int32;
				else if( addrmode & Quad )
					vm.zero_flag = *(int64_t *)(vm.reg[a.UInt64].UCharPtr+offset) == vm.reg[b.UInt64].Int64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					vm.zero_flag = *a.CharPtr == vm.reg[b.UInt64].Char;
				else if( addrmode & Half )
					vm.zero_flag = *a.ShortPtr == vm.reg[b.UInt64].Short;
				else if( addrmode & Long )
					vm.zero_flag = *a.Int32Ptr == vm.reg[b.UInt64].Int32;
				else if( addrmode & Quad )
					vm.zero_flag = *a.Int64Ptr == vm.reg[b.UInt64].Int64;
			}
			continue;
		}
		exec_ucmpi:; {	// dest is reg or memory, src is imm
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.zero_flag = vm.reg[a.UInt64].UInt64 == b.UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					vm.zero_flag = *(vm.reg[a.UInt64].UCharPtr+offset) == b.UChar;
				else if( addrmode & Half )
					vm.zero_flag = *(uint16_t *)(vm.reg[a.UInt64].UCharPtr+offset) == b.UShort;
				else if( addrmode & Long )
					vm.zero_flag = *(uint32_t *)(vm.reg[a.UInt64].UCharPtr+offset) == b.UInt32;
				else if( addrmode & Quad )
					vm.zero_flag = *(uint64_t *)(vm.reg[a.UInt64].UCharPtr+offset) == b.UInt64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					vm.zero_flag = *a.UCharPtr == b.UChar;
				else if( addrmode & Half )
					vm.zero_flag = *a.UShortPtr == b.UShort;
				else if( addrmode & Long )
					vm.zero_flag = *a.UInt32Ptr == b.UInt32;
				else if( addrmode & Quad )
					vm.zero_flag = *a.UInt64Ptr == b.UInt64;
			}
			continue;
		}
		exec_ucmpr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.zero_flag = vm.reg[a.UInt64].UInt64 == vm.reg[b.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					vm.zero_flag = vm.reg[a.UInt64].UChar == *(vm.reg[b.UInt64].UCharPtr+offset);
				else if( addrmode & Half )
					vm.zero_flag = vm.reg[a.UInt64].UShort == *(uint16_t *)(vm.reg[b.UInt64].UCharPtr+offset);
				else if( addrmode & Long )
					vm.zero_flag = vm.reg[a.UInt64].UInt32 == *(uint32_t *)(vm.reg[b.UInt64].UCharPtr+offset);
				else if( addrmode & Quad )
					vm.zero_flag = vm.reg[a.UInt64].UInt64 == *(uint64_t *)(vm.reg[b.UInt64].UCharPtr+offset);
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					vm.zero_flag = vm.reg[a.UInt64].UChar == *b.UCharPtr;
				else if( addrmode & Half )
					vm.zero_flag = vm.reg[a.UInt64].UShort == *b.UShortPtr;
				else if( addrmode & Long )
					vm.zero_flag = vm.reg[a.UInt64].UInt32 == *b.UInt32Ptr;
				else if( addrmode & Quad )
					vm.zero_flag = vm.reg[a.UInt64].UInt64 == *b.UInt64Ptr;
			}
			continue;
		}
		exec_ucmpm:; {	// dest is memory and src is reg
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & (Register|RegIndirect) ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					vm.zero_flag = *(vm.reg[a.UInt64].UCharPtr+offset) == vm.reg[b.UInt64].UChar;
				else if( addrmode & Half )
					vm.zero_flag = *(uint16_t *)(vm.reg[a.UInt64].UCharPtr+offset) == vm.reg[b.UInt64].UShort;
				else if( addrmode & Long )
					vm.zero_flag = *(uint32_t *)(vm.reg[a.UInt64].UCharPtr+offset) == vm.reg[b.UInt64].UInt32;
				else if( addrmode & Quad )
					vm.zero_flag = *(uint64_t *)(vm.reg[a.UInt64].UCharPtr+offset) == vm.reg[b.UInt64].UInt64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					vm.zero_flag = *a.UCharPtr == vm.reg[b.UInt64].UChar;
				else if( addrmode & Half )
					vm.zero_flag = *a.UShortPtr == vm.reg[b.UInt64].UShort;
				else if( addrmode & Long )
					vm.zero_flag = *a.UInt32Ptr == vm.reg[b.UInt64].UInt32;
				else if( addrmode & Quad )
					vm.zero_flag = *a.UInt64Ptr == vm.reg[b.UInt64].UInt64;
			}
			continue;
		}
		
		exec_neqi:; {	// dest is reg or memory, src is imm
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.zero_flag = vm.reg[a.UInt64].Int64 != b.Int64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					vm.zero_flag = *(vm.reg[a.UInt64].CharPtr+offset) != b.Char;
				else if( addrmode & Half )
					vm.zero_flag = *(int16_t *)(vm.reg[a.UInt64].UCharPtr+offset) != b.Short;
				else if( addrmode & Long )
					vm.zero_flag = *(int32_t *)(vm.reg[a.UInt64].UCharPtr+offset) != b.Int32;
				else if( addrmode & Quad )
					vm.zero_flag = *(int64_t *)(vm.reg[a.UInt64].UCharPtr+offset) != b.Int64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					vm.zero_flag = *a.CharPtr != b.Char;
				else if( addrmode & Half )
					vm.zero_flag = *a.ShortPtr != b.Short;
				else if( addrmode & Long )
					vm.zero_flag = *a.Int32Ptr != b.Int32;
				else if( addrmode & Quad )
					vm.zero_flag = *a.Int64Ptr != b.Int64;
			}
			continue;
		}
		exec_neqr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.zero_flag = vm.reg[a.UInt64].Int64 != vm.reg[b.UInt64].Int64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					vm.zero_flag = vm.reg[a.UInt64].Char != *(vm.reg[b.UInt64].CharPtr+offset);
				else if( addrmode & Half )
					vm.zero_flag = vm.reg[a.UInt64].Short != *(int16_t *)(vm.reg[b.UInt64].CharPtr+offset);
				else if( addrmode & Long )
					vm.zero_flag = vm.reg[a.UInt64].Int32 != *(int32_t *)(vm.reg[b.UInt64].CharPtr+offset);
				else if( addrmode & Quad )
					vm.zero_flag = vm.reg[a.UInt64].Int64 != *(int64_t *)(vm.reg[b.UInt64].CharPtr+offset);
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					vm.zero_flag = vm.reg[a.UInt64].Char != *b.CharPtr;
				else if( addrmode & Half )
					vm.zero_flag = vm.reg[a.UInt64].Short != *b.ShortPtr;
				else if( addrmode & Long )
					vm.zero_flag = vm.reg[a.UInt64].Int32 != *b.Int32Ptr;
				else if( addrmode & Quad )
					vm.zero_flag = vm.reg[a.UInt64].Int64 != *b.Int64Ptr;
			}
			continue;
		}
		exec_neqm:; {	// dest is memory and src is reg
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & (Register|RegIndirect) ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					vm.zero_flag = *(vm.reg[a.UInt64].CharPtr+offset) != vm.reg[b.UInt64].Char;
				else if( addrmode & Half )
					vm.zero_flag = *(int16_t *)(vm.reg[a.UInt64].UCharPtr+offset) != vm.reg[b.UInt64].Short;
				else if( addrmode & Long )
					vm.zero_flag = *(int32_t *)(vm.reg[a.UInt64].UCharPtr+offset) != vm.reg[b.UInt64].Int32;
				else if( addrmode & Quad )
					vm.zero_flag = *(int64_t *)(vm.reg[a.UInt64].UCharPtr+offset) != vm.reg[b.UInt64].Int64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					vm.zero_flag = *a.CharPtr != vm.reg[b.UInt64].Char;
				else if( addrmode & Half )
					vm.zero_flag = *a.ShortPtr != vm.reg[b.UInt64].Short;
				else if( addrmode & Long )
					vm.zero_flag = *a.Int32Ptr != vm.reg[b.UInt64].Int32;
				else if( addrmode & Quad )
					vm.zero_flag = *a.Int64Ptr != vm.reg[b.UInt64].Int64;
			}
			continue;
		}
		exec_uneqi:; {	// dest is reg or memory, src is imm
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.zero_flag = vm.reg[a.UInt64].UInt64 != b.UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					vm.zero_flag = *(vm.reg[a.UInt64].UCharPtr+offset) != b.UChar;
				else if( addrmode & Half )
					vm.zero_flag = *(uint16_t *)(vm.reg[a.UInt64].UCharPtr+offset) != b.UShort;
				else if( addrmode & Long )
					vm.zero_flag = *(uint32_t *)(vm.reg[a.UInt64].UCharPtr+offset) != b.UInt32;
				else if( addrmode & Quad )
					vm.zero_flag = *(uint64_t *)(vm.reg[a.UInt64].UCharPtr+offset) != b.UInt64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					vm.zero_flag = *a.UCharPtr != b.UChar;
				else if( addrmode & Half )
					vm.zero_flag = *a.UShortPtr != b.UShort;
				else if( addrmode & Long )
					vm.zero_flag = *a.UInt32Ptr != b.UInt32;
				else if( addrmode & Quad )
					vm.zero_flag = *a.UInt64Ptr != b.UInt64;
			}
			continue;
		}
		exec_uneqr:; {	// dest is reg, src is reg or memory
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & Register )
				vm.zero_flag = vm.reg[a.UInt64].UInt64 != vm.reg[b.UInt64].UInt64;
			else if( addrmode & RegIndirect ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					vm.zero_flag = vm.reg[a.UInt64].UChar != *(vm.reg[b.UInt64].UCharPtr+offset);
				else if( addrmode & Half )
					vm.zero_flag = vm.reg[a.UInt64].UShort != *(uint16_t *)(vm.reg[b.UInt64].UCharPtr+offset);
				else if( addrmode & Long )
					vm.zero_flag = vm.reg[a.UInt64].UInt32 != *(uint32_t *)(vm.reg[b.UInt64].UCharPtr+offset);
				else if( addrmode & Quad )
					vm.zero_flag = vm.reg[a.UInt64].UInt64 != *(uint64_t *)(vm.reg[b.UInt64].UCharPtr+offset);
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					vm.zero_flag = vm.reg[a.UInt64].UChar != *b.UCharPtr;
				else if( addrmode & Half )
					vm.zero_flag = vm.reg[a.UInt64].UShort != *b.UShortPtr;
				else if( addrmode & Long )
					vm.zero_flag = vm.reg[a.UInt64].UInt32 != *b.UInt32Ptr;
				else if( addrmode & Quad )
					vm.zero_flag = vm.reg[a.UInt64].UInt64 != *b.UInt64Ptr;
			}
			continue;
		}
		exec_uneqm:; {	// dest is memory and src is reg
			a.UInt64 = *vm.reg[rip].UInt64Ptr++;
			b.UInt64 = *vm.reg[rip].UInt64Ptr++;
			if( addrmode & (Register|RegIndirect) ) {
				offset = *vm.reg[rip].Int32Ptr++;
				if( addrmode & Byte )
					vm.zero_flag = *(vm.reg[a.UInt64].UCharPtr+offset) != vm.reg[b.UInt64].UChar;
				else if( addrmode & Half )
					vm.zero_flag = *(uint16_t *)(vm.reg[a.UInt64].UCharPtr+offset) != vm.reg[b.UInt64].UShort;
				else if( addrmode & Long )
					vm.zero_flag = *(uint32_t *)(vm.reg[a.UInt64].UCharPtr+offset) != vm.reg[b.UInt64].UInt32;
				else if( addrmode & Quad )
					vm.zero_flag = *(uint64_t *)(vm.reg[a.UInt64].UCharPtr+offset) != vm.reg[b.UInt64].UInt64;
			}
			else if( addrmode & (Direct|Immediate) ) {
				if( addrmode & Byte )
					vm.zero_flag = *a.UCharPtr != vm.reg[b.UInt64].UChar;
				else if( addrmode & Half )
					vm.zero_flag = *a.UShortPtr != vm.reg[b.UInt64].UShort;
				else if( addrmode & Long )
					vm.zero_flag = *a.UInt32Ptr != vm.reg[b.UInt64].UInt32;
				else if( addrmode & Quad )
					vm.zero_flag = *a.UInt64Ptr != vm.reg[b.UInt64].UInt64;
			}
			continue;
		}
		exec_reset:; {	// reset ALL registers except rip, rsp, and rbp to 0.
			vm.reg[ras].UInt64 = vm.reg[rbs].UInt64 =
			vm.reg[rcs].UInt64 = vm.reg[rds].UInt64 =
			vm.reg[res].UInt64 = vm.reg[rfs].UInt64 =
			vm.reg[rgs].UInt64 = vm.reg[rhs].UInt64 = 0;
		}
	}
}


int main(int argc, char **argv)
{
	char main = 11;
	char fact = 59;
	uint8_t code[] = {
		call, Immediate,		// 0-1
			main,0,0,0,0,0,0,0,	// 2-9
		halt, // 10
	// main:
		movi, Register, //11-12
			rcs,0,0,0,0,0,0,0, //13-20
			7,0,0,0,0,0,0,0, //21-28
		
		// factorial(7);
		call, Immediate, //29-30
			fact,0,0,0,0,0,0,0, //31-38
		
		// return 0;
		movi, Register, //39-40
			ras,0,0,0,0,0,0,0, //41-48
			0,0,0,0,0,0,0,0, //49-56
		ret, 0, //57-58
		
	// factorial:
		movm, RegIndirect|Long,	//59-60
			rbp,0,0,0,0,0,0,0, //61-68
			rcs,0,0,0,0,0,0,0, //69-76
				252,255,255,255, //77-80	// rbp + -4
		
		// i < 2 ?
		ulti, RegIndirect|Long, //81-82
			rbp,0,0,0,0,0,0,0, //83-90
			2,0,0,0,0,0,0,0, //91-98
				252,255,255,255, //99-102	// rbp + -4
		
		jz, Immediate, //103-104
			133,0,0,0,0,0,0,0, //105-112
		
		// return 1;
		movi, Register, //113-114
			ras,0,0,0,0,0,0,0, //115-122
			1,0,0,0,0,0,0,0, //123-130
		ret, 0, //131-132
		
		movr, RegIndirect|Long,
			ras,0,0,0,0,0,0,0, //
			rbp,0,0,0,0,0,0,0, //
				252,255,255,255, //	// rbp + -4
		
		usubi, Register,
			ras,0,0,0,0,0,0,0,
			1,0,0,0,0,0,0,0,
		
		movr, Register,
			rcs,0,0,0,0,0,0,0,
			ras,0,0,0,0,0,0,0,
		
		call, Immediate,
			fact,0,0,0,0,0,0,0,
		
		umulr, RegIndirect|Long,
			ras,0,0,0,0,0,0,0, //53-60
			rbp,0,0,0,0,0,0,0, //61-68
				252,255,255,255, //69-72	// rbp + -4
		
		ret, 0
	};
	vm_exec(code);
	printf("this VM has == %" PRIu32 " opcodes\n", nop+1);
}
