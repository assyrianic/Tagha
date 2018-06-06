
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "tagha.h"


struct Tagha *Tagha_New(uint8_t *scripthdr)
{
	struct Tagha *__restrict vm = calloc(1, sizeof *vm);
	vm ? (vm->ScriptHdr.UCharPtr = scripthdr) : (void)vm;
	return vm;
}

void Tagha_Init(struct Tagha *const __restrict vm, uint8_t *scripthdr)
{
	if( !vm )
		return;
	
	*vm = (struct Tagha){0};
	vm->ScriptHdr.UCharPtr = scripthdr;
}

void Tagha_Del(struct Tagha *const __restrict vm)
{
	if( !vm )
		return;
	
	Map_Del(&vm->Natives);
}

void Tagha_Free(struct Tagha **__restrict vmref)
{
	if( !*vmref )
		return;
	
	Tagha_Del(*vmref);
	free(*vmref), *vmref=NULL;
}

void TaghaDebug_PrintRegisters(const struct Tagha *const __restrict vm)
{
	if( !vm )
		return;
	puts("\nTagha Debug: Printing Register Data.\n");
	for( size_t i=0 ; i<regsize ; i++ )
		printf("reg[%zu] == '%" PRIu64 "'\n", i, vm->Regs[i].UInt64);
	puts("\n");
}

bool Tagha_RegisterNatives(struct Tagha *const __restrict vm, struct NativeInfo natives[__restrict])
{
	if( !vm or !natives )
		return false;
	
	for( struct NativeInfo *n=natives ; n->NativeCFunc and n->Name ; n++ )
		Map_Insert(&vm->Natives, n->Name, (union Value){.VoidFunc = n->NativeCFunc});
	return true;
}

static uint8_t *GetFunctionOffsetByName(const char *__restrict funcname, uint8_t *script)
{
	if( !funcname or !script )
		return NULL;
	
	union Value reader = (union Value){.UCharPtr = script};
	// jump past header data to the first function table's entry.
	reader.UCharPtr += 7;
	
	const size_t funcs = *reader.UInt32Ptr++;
	reader.UCharPtr++;
	for( size_t i=0 ; i<funcs ; i++ ) {
		const size_t stringlen = *reader.UInt32Ptr++;
		const size_t instrlen = *reader.UInt32Ptr++;
		if( !strcmp(funcname, reader.Ptr) )
			return reader.UCharPtr + stringlen;
		
		// skip to the next 
		reader.UCharPtr += (stringlen + instrlen + 1);
	}
	return NULL;
}

static uint8_t *GetFunctionOffsetByIndex(const size_t index, uint8_t *script)
{
	if( !script )
		return NULL;
	
	union Value reader = (union Value){.UCharPtr = script};
	// jump past header data to the first function table's entry.
	reader.UCharPtr += 7;
	
	const size_t funcs = *reader.UInt32Ptr++;
	if( index >= funcs )
		return NULL;
	
	reader.UCharPtr++;
	for( size_t i=0 ; i<funcs ; i++ ) {
		const size_t stringlen = *reader.UInt32Ptr++;
		const size_t instrlen = *reader.UInt32Ptr++;
		if( i==index )
			return reader.UCharPtr + stringlen;
		
		// skip to the next 
		reader.UCharPtr += (stringlen + instrlen + 1);
	}
	return NULL;
}

static const char *GetFunctionNameByIndex(const size_t index, uint8_t *script)
{
	if( !script )
		return NULL;
	
	union Value reader = (union Value){.UCharPtr = script};
	// jump past header data to the first function table's entry.
	reader.UCharPtr += 7;
	
	const size_t funcs = *reader.UInt32Ptr++;
	if( index >= funcs )
		return NULL;
	
	reader.UCharPtr++;
	for( size_t i=0 ; i<funcs ; i++ ) {
		const size_t stringlen = *reader.UInt32Ptr++;
		const size_t instrlen = *reader.UInt32Ptr++;
		if( i==index )
			return reader.Ptr;
		
		// skip to the next 
		reader.UCharPtr += (stringlen + instrlen + 1);
	}
	return NULL;
}

int32_t Tagha_Exec(struct Tagha *const __restrict vm)
{
	if( !vm )
		return -1;
	
	const union Value *const MainBasePtr = vm->Regs[regBase].SelfPtr;
	
	uint8_t instr=0, addrmode=0;
	uint16_t opcode = 0;
	
#ifdef DEBUG
#  define X(x) #x ,
	// for debugging purposes.
	const char *const __restrict opcode2str[] = { INSTR_SET };
#  undef X
#endif
	
#define X(x) &&exec_##x ,
	// our instruction dispatch table.
	const void *const __restrict dispatch[] = { INSTR_SET };
#undef X
#undef INSTR_SET
	
	while( 1 ) {
		opcode = *vm->Regs[regInstr].UShortPtr++;
		
		// get the instruction from the first byte.
		instr = opcode & 255;
		
		// get addressing mode from second byte.
		addrmode = opcode >> 8;
		
		if( instr>nop )
			return -1;
		
		goto *dispatch[instr];
		
		exec_halt:;
			return vm->Regs[regAlaf].Int32;
		exec_nop:;
			continue;
		
		// pushes a value to the top of the stack, raises the stack pointer by 8 bytes.
		// push reg (1 byte for register id)
		// push imm (8 bytes for constant values)
		// push [reg+offset] (1 byte reg id + 4-byte signed offset)
		exec_push:; {
			// push an imm constant.
			if( addrmode & Immediate )
				(--vm->Regs[regStk].SelfPtr)->UInt64 = *vm->Regs[regInstr].UInt64Ptr++;
			// push a register's contents.
			else if( addrmode & Register )
				*--vm->Regs[regStk].SelfPtr = vm->Regs[*vm->Regs[regInstr].UCharPtr++];
			// push the contents of a memory address inside a register.
			else if( addrmode & RegIndirect ) {
				const uint8_t regindex = *vm->Regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regindex].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				(--vm->Regs[regStk].SelfPtr)->UInt64 = *effective_address.UInt64Ptr;
			}
			continue;
		}
		
		// pops a value from the stack into a register or memory then reduces stack by 8 bytes.
		// pop reg
		// pop [reg+offset]
		exec_pop:; {
			if( addrmode & Register )
				vm->Regs[*vm->Regs[regInstr].UCharPtr++].UInt64 = (*vm->Regs[regStk].SelfPtr++).UInt64;
			else if( addrmode & RegIndirect ) {
				const uint8_t regid = *vm->Regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				*effective_address.UInt64Ptr = (*vm->Regs[regStk].SelfPtr++).UInt64;
			}
			continue;
		}
		
		// loads a value that will be used as a memory address to a register.
		// lea reg, [reg+offset] (not dereferenced)
		exec_lea:; {
			const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
			vm->Regs[op_args & 255].UInt64 = vm->Regs[op_args >> 8].UInt64 + *vm->Regs[regInstr].Int32Ptr++;
			continue;
		}
		
		// copies a value to a register or memory address.
		// mov reg, [reg+offset]
		// mov reg, imm
		// mov reg, reg
		// mov [reg+offset], imm
		// mov [reg+offset], reg
		exec_mov:; {
			// first addressing mode determines the destination.
			const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
			
			// get secondary addressing mode for the second operand.
			const uint8_t sec_addrmode = op_args & 255;
			
			// get 1st register id.
			const uint8_t regid = op_args >> 8;
			if( addrmode & Register ) {
				// mov reg, imm
				if( sec_addrmode & Immediate )
					vm->Regs[regid].UInt64 = *vm->Regs[regInstr].UInt64Ptr++;
				// mov reg, reg
				else if( sec_addrmode & Register )
					vm->Regs[regid].UInt64 = vm->Regs[*vm->Regs[regInstr].UCharPtr++].UInt64;
				// mov reg, [reg+offset]
				else if( sec_addrmode & RegIndirect ) {
					const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[sec_regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( sec_addrmode & Byte )
						vm->Regs[regid].UChar = *effective_address.UCharPtr;
					else if( sec_addrmode & TwoBytes )
						vm->Regs[regid].UShort = *effective_address.UShortPtr;
					else if( sec_addrmode & FourBytes )
						vm->Regs[regid].UInt32 = *effective_address.UInt32Ptr;
					else if( sec_addrmode & EightBytes )
						vm->Regs[regid].UInt64 = *effective_address.UInt64Ptr;
				}
			}
			else if( addrmode & RegIndirect ) {
				// mov [reg+offset], imm
				if( sec_addrmode & Immediate ) {
					// have to store the imm value prior because the offset is stored AFTER the last operand.
					const union Value imm = (union Value){.UInt64 = *vm->Regs[regInstr].UInt64Ptr++};
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( addrmode & Byte )
						*effective_address.UCharPtr = imm.UChar;
					else if( addrmode & TwoBytes )
						*effective_address.UShortPtr = imm.UShort;
					else if( addrmode & FourBytes )
						*effective_address.UInt32Ptr = imm.UInt32;
					else if( addrmode & EightBytes )
						*effective_address.UInt64Ptr = imm.UInt64;
				}
				// mov [reg+offset], reg
				else if( sec_addrmode & Register ) {
					const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( addrmode & Byte )
						*effective_address.UCharPtr = vm->Regs[sec_regid].UChar;
					else if( addrmode & TwoBytes )
						*effective_address.UShortPtr = vm->Regs[sec_regid].UShort;
					else if( addrmode & FourBytes )
						*effective_address.UInt32Ptr = vm->Regs[sec_regid].UInt32;
					else if( addrmode & EightBytes )
						*effective_address.UInt64Ptr = vm->Regs[sec_regid].UInt64;
				}
			}
			continue;
		}
		
		// adds two values to the destination value which is either a register or memory address.
		// add reg, [reg+offset]
		// add reg, imm
		// add reg, reg
		// add [reg+offset], reg
		// add [reg+offset], imm
		exec_add:; {
			// first addressing mode determines the destination.
			const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
			const uint8_t sec_addrmode = op_args & 255; // get secondary addressing mode.
			const uint8_t regid = op_args >> 8; // get 1st register id.
			
			if( addrmode & Register ) {
				// add reg, imm
				if( sec_addrmode & Immediate )
					vm->Regs[regid].UInt64 += *vm->Regs[regInstr].UInt64Ptr++;
				// add reg, reg
				else if( sec_addrmode & Register )
					vm->Regs[regid].UInt64 += vm->Regs[*vm->Regs[regInstr].UCharPtr++].UInt64;
				// add reg, [reg+offset]
				else if( sec_addrmode & RegIndirect ) {
					const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[sec_regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( sec_addrmode & Byte )
						vm->Regs[regid].UChar += *effective_address.UCharPtr;
					else if( sec_addrmode & TwoBytes )
						vm->Regs[regid].UShort += *effective_address.UShortPtr;
					else if( sec_addrmode & FourBytes )
						vm->Regs[regid].UInt32 += *effective_address.UInt32Ptr;
					else if( sec_addrmode & EightBytes )
						vm->Regs[regid].UInt64 += *effective_address.UInt64Ptr;
				}
			}
			else if( addrmode & RegIndirect ) {
				// add [reg+offset], imm
				if( sec_addrmode & Immediate ) {
					// have to store the imm value prior because the offset is stored AFTER the last operand.
					const union Value imm = (union Value){.UInt64 = *vm->Regs[regInstr].UInt64Ptr++};
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( addrmode & Byte )
						*effective_address.UCharPtr += imm.UChar;
					else if( addrmode & TwoBytes )
						*effective_address.UShortPtr += imm.UShort;
					else if( addrmode & FourBytes )
						*effective_address.UInt32Ptr += imm.UInt32;
					else if( addrmode & EightBytes )
						*effective_address.UInt64Ptr += imm.UInt64;
				}
				// add [reg+offset], reg
				else if( sec_addrmode & Register ) {
					const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( addrmode & Byte )
						*effective_address.UCharPtr += vm->Regs[sec_regid].UChar;
					else if( addrmode & TwoBytes )
						*effective_address.UShortPtr += vm->Regs[sec_regid].UShort;
					else if( addrmode & FourBytes )
						*effective_address.UInt32Ptr += vm->Regs[sec_regid].UInt32;
					else if( addrmode & EightBytes )
						*effective_address.UInt64Ptr += vm->Regs[sec_regid].UInt64;
				}
			}
			continue;
		}
		exec_sub:; {
			// first addressing mode determines the destination.
			const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
			const uint8_t sec_addrmode = op_args & 255; // get secondary addressing mode.
			const uint8_t regid = op_args >> 8; // get 1st register id.
			if( addrmode & Register ) {
				// sub reg, imm
				if( sec_addrmode & Immediate )
					vm->Regs[regid].UInt64 -= *vm->Regs[regInstr].UInt64Ptr++;
				// sub reg, reg
				else if( sec_addrmode & Register )
					vm->Regs[regid].UInt64 -= vm->Regs[*vm->Regs[regInstr].UCharPtr++].UInt64;
				// sub reg, [reg+offset]
				else if( sec_addrmode & RegIndirect ) {
					const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[sec_regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( sec_addrmode & Byte )
						vm->Regs[regid].UChar -= *effective_address.UCharPtr;
					else if( sec_addrmode & TwoBytes )
						vm->Regs[regid].UShort -= *effective_address.UShortPtr;
					else if( sec_addrmode & FourBytes )
						vm->Regs[regid].UInt32 -= *effective_address.UInt32Ptr;
					else if( sec_addrmode & EightBytes )
						vm->Regs[regid].UInt64 -= *effective_address.UInt64Ptr;
				}
			}
			else if( addrmode & RegIndirect ) {
				// sub [reg+offset], imm
				if( sec_addrmode & Immediate ) {
					// have to store the imm value prior because the offset is stored AFTER the last operand.
					const union Value imm = (union Value){.UInt64 = *vm->Regs[regInstr].UInt64Ptr++};
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( addrmode & Byte )
						*effective_address.UCharPtr -= imm.UChar;
					else if( addrmode & TwoBytes )
						*effective_address.UShortPtr -= imm.UShort;
					else if( addrmode & FourBytes )
						*effective_address.UInt32Ptr -= imm.UInt32;
					else if( addrmode & EightBytes )
						*effective_address.UInt64Ptr -= imm.UInt64;
				}
				// sub [reg+offset], reg
				else if( sec_addrmode & Register ) {
					const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( addrmode & Byte )
						*effective_address.UCharPtr -= vm->Regs[sec_regid].UChar;
					else if( addrmode & TwoBytes )
						*effective_address.UShortPtr -= vm->Regs[sec_regid].UShort;
					else if( addrmode & FourBytes )
						*effective_address.UInt32Ptr -= vm->Regs[sec_regid].UInt32;
					else if( addrmode & EightBytes )
						*effective_address.UInt64Ptr -= vm->Regs[sec_regid].UInt64;
				}
			}
			continue;
		}
		exec_mul:; {
			// first addressing mode determines the destination.
			const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
			const uint8_t sec_addrmode = op_args & 255; // get secondary addressing mode.
			const uint8_t regid = op_args >> 8; // get 1st register id.
			if( addrmode & Register ) {
				// mul reg, imm
				if( sec_addrmode & Immediate )
					vm->Regs[regid].UInt64 *= *vm->Regs[regInstr].UInt64Ptr++;
				// mul reg, reg
				else if( sec_addrmode & Register )
					vm->Regs[regid].UInt64 *= vm->Regs[*vm->Regs[regInstr].UCharPtr++].UInt64;
				// mul reg, [reg+offset]
				else if( sec_addrmode & RegIndirect ) {
					const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[sec_regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( sec_addrmode & Byte )
						vm->Regs[regid].UChar *= *effective_address.UCharPtr;
					else if( sec_addrmode & TwoBytes )
						vm->Regs[regid].UShort *= *effective_address.UShortPtr;
					else if( sec_addrmode & FourBytes )
						vm->Regs[regid].UInt32 *= *effective_address.UInt32Ptr;
					else if( sec_addrmode & EightBytes )
						vm->Regs[regid].UInt64 *= *effective_address.UInt64Ptr;
				}
			}
			else if( addrmode & RegIndirect ) {
				// mul [reg+offset], imm
				if( sec_addrmode & Immediate ) {
					// have to store the imm value prior because the offset is stored AFTER the last operand.
					const union Value imm = (union Value){.UInt64 = *vm->Regs[regInstr].UInt64Ptr++};
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( addrmode & Byte )
						*effective_address.UCharPtr *= imm.UChar;
					else if( addrmode & TwoBytes )
						*effective_address.UShortPtr *= imm.UShort;
					else if( addrmode & FourBytes )
						*effective_address.UInt32Ptr *= imm.UInt32;
					else if( addrmode & EightBytes )
						*effective_address.UInt64Ptr *= imm.UInt64;
				}
				// mul [reg+offset], reg
				else if( sec_addrmode & Register ) {
					const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( addrmode & Byte )
						*effective_address.UCharPtr *= vm->Regs[sec_regid].UChar;
					else if( addrmode & TwoBytes )
						*effective_address.UShortPtr *= vm->Regs[sec_regid].UShort;
					else if( addrmode & FourBytes )
						*effective_address.UInt32Ptr *= vm->Regs[sec_regid].UInt32;
					else if( addrmode & EightBytes )
						*effective_address.UInt64Ptr *= vm->Regs[sec_regid].UInt64;
				}
			}
			continue;
		}
		exec_divi:; {
			// first addressing mode determines the destination.
			const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
			const uint8_t sec_addrmode = op_args & 255; // get secondary addressing mode.
			const uint8_t regid = op_args >> 8; // get 1st register id.
			if( addrmode & Register ) {
				// divi reg, imm
				if( sec_addrmode & Immediate )
					vm->Regs[regid].UInt64 /= *vm->Regs[regInstr].UInt64Ptr++;
				// divi reg, reg
				else if( sec_addrmode & Register )
					vm->Regs[regid].UInt64 /= vm->Regs[*vm->Regs[regInstr].UCharPtr++].UInt64;
				// divi reg, [reg+offset]
				else if( sec_addrmode & RegIndirect ) {
					const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[sec_regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( sec_addrmode & Byte )
						vm->Regs[regid].UChar /= *effective_address.UCharPtr;
					else if( sec_addrmode & TwoBytes )
						vm->Regs[regid].UShort /= *effective_address.UShortPtr;
					else if( sec_addrmode & FourBytes )
						vm->Regs[regid].UInt32 /= *effective_address.UInt32Ptr;
					else if( sec_addrmode & EightBytes )
						vm->Regs[regid].UInt64 /= *effective_address.UInt64Ptr;
				}
			}
			else if( addrmode & RegIndirect ) {
				// divi [reg+offset], imm
				if( sec_addrmode & Immediate ) {
					// have to store the imm value prior because the offset is stored AFTER the last operand.
					const union Value imm = (union Value){.UInt64 = *vm->Regs[regInstr].UInt64Ptr++};
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( addrmode & Byte )
						*effective_address.UCharPtr /= imm.UChar;
					else if( addrmode & TwoBytes )
						*effective_address.UShortPtr /= imm.UShort;
					else if( addrmode & FourBytes )
						*effective_address.UInt32Ptr /= imm.UInt32;
					else if( addrmode & EightBytes )
						*effective_address.UInt64Ptr /= imm.UInt64;
				}
				// divi [reg+offset], reg
				else if( sec_addrmode & Register ) {
					const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( addrmode & Byte )
						*effective_address.UCharPtr /= vm->Regs[sec_regid].UChar;
					else if( addrmode & TwoBytes )
						*effective_address.UShortPtr /= vm->Regs[sec_regid].UShort;
					else if( addrmode & FourBytes )
						*effective_address.UInt32Ptr /= vm->Regs[sec_regid].UInt32;
					else if( addrmode & EightBytes )
						*effective_address.UInt64Ptr /= vm->Regs[sec_regid].UInt64;
				}
			}
			continue;
		}
		exec_mod:; {
			// first addressing mode determines the destination.
			const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
			const uint8_t sec_addrmode = op_args & 255; // get secondary addressing mode.
			const uint8_t regid = op_args >> 8; // get 1st register id.
			if( addrmode & Register ) {
				// mod reg, imm
				if( sec_addrmode & Immediate )
					vm->Regs[regid].UInt64 %= *vm->Regs[regInstr].UInt64Ptr++;
				// mod reg, reg
				else if( sec_addrmode & Register )
					vm->Regs[regid].UInt64 %= vm->Regs[*vm->Regs[regInstr].UCharPtr++].UInt64;
				// mod reg, [reg+offset]
				else if( sec_addrmode & RegIndirect ) {
					const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[sec_regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( sec_addrmode & Byte )
						vm->Regs[regid].UChar %= *effective_address.UCharPtr;
					else if( sec_addrmode & TwoBytes )
						vm->Regs[regid].UShort %= *effective_address.UShortPtr;
					else if( sec_addrmode & FourBytes )
						vm->Regs[regid].UInt32 %= *effective_address.UInt32Ptr;
					else if( sec_addrmode & EightBytes )
						vm->Regs[regid].UInt64 %= *effective_address.UInt64Ptr;
				}
			}
			else if( addrmode & RegIndirect ) {
				// mod [reg+offset], imm
				if( sec_addrmode & Immediate ) {
					// have to store the imm value prior because the offset is stored AFTER the last operand.
					const union Value imm = (union Value){.UInt64 = *vm->Regs[regInstr].UInt64Ptr++};
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( addrmode & Byte )
						*effective_address.UCharPtr %= imm.UChar;
					else if( addrmode & TwoBytes )
						*effective_address.UShortPtr %= imm.UShort;
					else if( addrmode & FourBytes )
						*effective_address.UInt32Ptr %= imm.UInt32;
					else if( addrmode & EightBytes )
						*effective_address.UInt64Ptr %= imm.UInt64;
				}
				// mod [reg+offset], reg
				else if( sec_addrmode & Register ) {
					const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( addrmode & Byte )
						*effective_address.UCharPtr %= vm->Regs[sec_regid].UChar;
					else if( addrmode & TwoBytes )
						*effective_address.UShortPtr %= vm->Regs[sec_regid].UShort;
					else if( addrmode & FourBytes )
						*effective_address.UInt32Ptr %= vm->Regs[sec_regid].UInt32;
					else if( addrmode & EightBytes )
						*effective_address.UInt64Ptr %= vm->Regs[sec_regid].UInt64;
				}
			}
			continue;
		}
#if FLOATING_POINT_OPS
		exec_addf:; {
			// first addressing mode determines the destination.
			const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
			const uint8_t sec_addrmode = op_args & 255; // get secondary addressing mode.
			const uint8_t regid = op_args >> 8; // get 1st register id.
			
			if( addrmode & Register ) {
				// addf reg, imm
				if( sec_addrmode & Immediate ) {
					if( addrmode & FourBytes )
						vm->Regs[regid].Float += *vm->Regs[regInstr].FloatPtr++;
					else if( addrmode & EightBytes )
						vm->Regs[regid].Double += *vm->Regs[regInstr].DoublePtr++;
				}
				// addf reg, reg
				else if( sec_addrmode & Register ) {
					if( addrmode & FourBytes )
						vm->Regs[regid].Float += vm->Regs[*vm->Regs[regInstr].UCharPtr++].Float;
					else if( addrmode & EightBytes )
						vm->Regs[regid].Double += vm->Regs[*vm->Regs[regInstr].UCharPtr++].Double;
				}
				// addf reg, [reg+offset]
				else if( sec_addrmode & RegIndirect ) {
					const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[sec_regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( sec_addrmode & FourBytes )
						vm->Regs[regid].Float += *effective_address.FloatPtr;
					else if( sec_addrmode & EightBytes )
						vm->Regs[regid].Double += *effective_address.DoublePtr;
				}
			}
			else if( addrmode & RegIndirect ) {
				// addf [reg+offset], imm
				if( sec_addrmode & Immediate ) {
					// have to store the imm value prior because the offset is stored AFTER the last operand.
					const union Value imm = addrmode & FourBytes ? (union Value){.Float = *vm->Regs[regInstr].FloatPtr++} : (union Value){.Double = *vm->Regs[regInstr].DoublePtr++};
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( addrmode & FourBytes )
						*effective_address.FloatPtr += imm.Float;
					else if( addrmode & EightBytes )
						*effective_address.DoublePtr += imm.Double;
				}
				// addf [reg+offset], reg
				else if( sec_addrmode & Register ) {
					const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( addrmode & FourBytes )
						*effective_address.FloatPtr += vm->Regs[sec_regid].Float;
					else if( addrmode & EightBytes )
						*effective_address.DoublePtr += vm->Regs[sec_regid].Double;
				}
			}
			continue;
		}
		exec_subf:; {
			// first addressing mode determines the destination.
			const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
			const uint8_t sec_addrmode = op_args & 255; // get secondary addressing mode.
			const uint8_t regid = op_args >> 8; // get 1st register id.
			
			if( addrmode & Register ) {
				// subf reg, imm
				if( sec_addrmode & Immediate ) {
					if( addrmode & FourBytes )
						vm->Regs[regid].Float -= *vm->Regs[regInstr].FloatPtr++;
					else if( addrmode & EightBytes )
						vm->Regs[regid].Double -= *vm->Regs[regInstr].DoublePtr++;
				}
				// subf reg, reg
				else if( sec_addrmode & Register ) {
					if( addrmode & FourBytes )
						vm->Regs[regid].Float -= vm->Regs[*vm->Regs[regInstr].UCharPtr++].Float;
					else if( addrmode & EightBytes )
						vm->Regs[regid].Double -= vm->Regs[*vm->Regs[regInstr].UCharPtr++].Double;
				}
				// subf reg, [reg+offset]
				else if( sec_addrmode & RegIndirect ) {
					const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[sec_regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( sec_addrmode & FourBytes )
						vm->Regs[regid].Float -= *effective_address.FloatPtr;
					else if( sec_addrmode & EightBytes )
						vm->Regs[regid].Double -= *effective_address.DoublePtr;
				}
			}
			else if( addrmode & RegIndirect ) {
				// subf [reg+offset], imm
				if( sec_addrmode & Immediate ) {
					// have to store the imm value prior because the offset is stored AFTER the last operand.
					const union Value imm = addrmode & FourBytes ? (union Value){.Float = *vm->Regs[regInstr].FloatPtr++} : (union Value){.Double = *vm->Regs[regInstr].DoublePtr++};
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( addrmode & FourBytes )
						*effective_address.FloatPtr -= imm.Float;
					else if( addrmode & EightBytes )
						*effective_address.DoublePtr -= imm.Double;
				}
				// subf [reg+offset], reg
				else if( sec_addrmode & Register ) {
					const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( addrmode & FourBytes )
						*effective_address.FloatPtr -= vm->Regs[sec_regid].Float;
					else if( addrmode & EightBytes )
						*effective_address.DoublePtr -= vm->Regs[sec_regid].Double;
				}
			}
			continue;
		}
		exec_mulf:; {
			// first addressing mode determines the destination.
			const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
			const uint8_t sec_addrmode = op_args & 255; // get secondary addressing mode.
			const uint8_t regid = op_args >> 8; // get 1st register id.
			
			if( addrmode & Register ) {
				// mulf reg, imm
				if( sec_addrmode & Immediate ) {
					if( addrmode & FourBytes )
						vm->Regs[regid].Float *= *vm->Regs[regInstr].FloatPtr++;
					else if( addrmode & EightBytes )
						vm->Regs[regid].Double *= *vm->Regs[regInstr].DoublePtr++;
				}
				// mulf reg, reg
				else if( sec_addrmode & Register ) {
					if( addrmode & FourBytes )
						vm->Regs[regid].Float *= vm->Regs[*vm->Regs[regInstr].UCharPtr++].Float;
					else if( addrmode & EightBytes )
						vm->Regs[regid].Double *= vm->Regs[*vm->Regs[regInstr].UCharPtr++].Double;
				}
				// mulf reg, [reg+offset]
				else if( sec_addrmode & RegIndirect ) {
					const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[sec_regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( sec_addrmode & FourBytes )
						vm->Regs[regid].Float *= *effective_address.FloatPtr;
					else if( sec_addrmode & EightBytes )
						vm->Regs[regid].Double *= *effective_address.DoublePtr;
				}
			}
			else if( addrmode & RegIndirect ) {
				// mulf [reg+offset], imm
				if( sec_addrmode & Immediate ) {
					// have to store the imm value prior because the offset is stored AFTER the last operand.
					const union Value imm = addrmode & FourBytes ? (union Value){.Float = *vm->Regs[regInstr].FloatPtr++} : (union Value){.Double = *vm->Regs[regInstr].DoublePtr++};
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( addrmode & FourBytes )
						*effective_address.FloatPtr *= imm.Float;
					else if( addrmode & EightBytes )
						*effective_address.DoublePtr *= imm.Double;
				}
				// mulf [reg+offset], reg
				else if( sec_addrmode & Register ) {
					const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( addrmode & FourBytes )
						*effective_address.FloatPtr *= vm->Regs[sec_regid].Float;
					else if( addrmode & EightBytes )
						*effective_address.DoublePtr *= vm->Regs[sec_regid].Double;
				}
			}
			continue;
		}
		exec_divf:; {
			// first addressing mode determines the destination.
			const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
			const uint8_t sec_addrmode = op_args & 255; // get secondary addressing mode.
			const uint8_t regid = op_args >> 8; // get 1st register id.
			
			if( addrmode & Register ) {
				// divf reg, imm
				if( sec_addrmode & Immediate ) {
					if( addrmode & FourBytes )
						vm->Regs[regid].Float /= *vm->Regs[regInstr].FloatPtr++;
					else if( addrmode & EightBytes )
						vm->Regs[regid].Double /= *vm->Regs[regInstr].DoublePtr++;
				}
				// divf reg, reg
				else if( sec_addrmode & Register ) {
					if( addrmode & FourBytes )
						vm->Regs[regid].Float /= vm->Regs[*vm->Regs[regInstr].UCharPtr++].Float;
					else if( addrmode & EightBytes )
						vm->Regs[regid].Double /= vm->Regs[*vm->Regs[regInstr].UCharPtr++].Double;
				}
				// divf reg, [reg+offset]
				else if( sec_addrmode & RegIndirect ) {
					const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[sec_regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( sec_addrmode & FourBytes )
						vm->Regs[regid].Float /= *effective_address.FloatPtr;
					else if( sec_addrmode & EightBytes )
						vm->Regs[regid].Double /= *effective_address.DoublePtr;
				}
			}
			else if( addrmode & RegIndirect ) {
				// divf [reg+offset], imm
				if( sec_addrmode & Immediate ) {
					// have to store the imm value prior because the offset is stored AFTER the last operand.
					const union Value imm = addrmode & FourBytes ? (union Value){.Float = *vm->Regs[regInstr].FloatPtr++} : (union Value){.Double = *vm->Regs[regInstr].DoublePtr++};
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( addrmode & FourBytes )
						*effective_address.FloatPtr /= imm.Float;
					else if( addrmode & EightBytes )
						*effective_address.DoublePtr /= imm.Double;
				}
				// divf [reg+offset], reg
				else if( sec_addrmode & Register ) {
					const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( addrmode & FourBytes )
						*effective_address.FloatPtr /= vm->Regs[sec_regid].Float;
					else if( addrmode & EightBytes )
						*effective_address.DoublePtr /= vm->Regs[sec_regid].Double;
				}
			}
			continue;
		}
#endif
		exec_andb:; {
			// first addressing mode determines the destination.
			const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
			const uint8_t sec_addrmode = op_args & 255; // get secondary addressing mode.
			const uint8_t regid = op_args >> 8; // get 1st register id.
			if( addrmode & Register ) {
				// andb reg, imm
				if( sec_addrmode & Immediate )
					vm->Regs[regid].UInt64 &= *vm->Regs[regInstr].UInt64Ptr++;
				// andb reg, reg
				else if( sec_addrmode & Register )
					vm->Regs[regid].UInt64 &= vm->Regs[*vm->Regs[regInstr].UCharPtr++].UInt64;
				// andb reg, [reg+offset]
				else if( sec_addrmode & RegIndirect ) {
					const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[sec_regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( sec_addrmode & Byte )
						vm->Regs[regid].UChar &= *effective_address.UCharPtr;
					else if( sec_addrmode & TwoBytes )
						vm->Regs[regid].UShort &= *effective_address.UShortPtr;
					else if( sec_addrmode & FourBytes )
						vm->Regs[regid].UInt32 &= *effective_address.UInt32Ptr;
					else if( sec_addrmode & EightBytes )
						vm->Regs[regid].UInt64 &= *effective_address.UInt64Ptr;
				}
			}
			else if( addrmode & RegIndirect ) {
				// andb [reg+offset], imm
				if( sec_addrmode & Immediate ) {
					// have to store the imm value prior because the offset is stored AFTER the last operand.
					const union Value imm = (union Value){.UInt64 = *vm->Regs[regInstr].UInt64Ptr++};
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( addrmode & Byte )
						*effective_address.UCharPtr &= imm.UChar;
					else if( addrmode & TwoBytes )
						*effective_address.UShortPtr &= imm.UShort;
					else if( addrmode & FourBytes )
						*effective_address.UInt32Ptr &= imm.UInt32;
					else if( addrmode & EightBytes )
						*effective_address.UInt64Ptr &= imm.UInt64;
				}
				// andb [reg+offset], reg
				else if( sec_addrmode & Register ) {
					const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( addrmode & Byte )
						*effective_address.UCharPtr &= vm->Regs[sec_regid].UChar;
					else if( addrmode & TwoBytes )
						*effective_address.UShortPtr &= vm->Regs[sec_regid].UShort;
					else if( addrmode & FourBytes )
						*effective_address.UInt32Ptr &= vm->Regs[sec_regid].UInt32;
					else if( addrmode & EightBytes )
						*effective_address.UInt64Ptr &= vm->Regs[sec_regid].UInt64;
				}
			}
			continue;
		}
		exec_orb:; {
			// first addressing mode determines the destination.
			const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
			const uint8_t sec_addrmode = op_args & 255; // get secondary addressing mode.
			const uint8_t regid = op_args >> 8; // get 1st register id.
			if( addrmode & Register ) {
				// orb reg, imm
				if( sec_addrmode & Immediate )
					vm->Regs[regid].UInt64 |= *vm->Regs[regInstr].UInt64Ptr++;
				// orb reg, reg
				else if( sec_addrmode & Register )
					vm->Regs[regid].UInt64 |= vm->Regs[*vm->Regs[regInstr].UCharPtr++].UInt64;
				// orb reg, [reg+offset]
				else if( sec_addrmode & RegIndirect ) {
					const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[sec_regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( sec_addrmode & Byte )
						vm->Regs[regid].UChar |= *effective_address.UCharPtr;
					else if( sec_addrmode & TwoBytes )
						vm->Regs[regid].UShort |= *effective_address.UShortPtr;
					else if( sec_addrmode & FourBytes )
						vm->Regs[regid].UInt32 |= *effective_address.UInt32Ptr;
					else if( sec_addrmode & EightBytes )
						vm->Regs[regid].UInt64 |= *effective_address.UInt64Ptr;
				}
			}
			else if( addrmode & RegIndirect ) {
				// orb [reg+offset], imm
				if( sec_addrmode & Immediate ) {
					// have to store the imm value prior because the offset is stored AFTER the last operand.
					const union Value imm = (union Value){.UInt64 = *vm->Regs[regInstr].UInt64Ptr++};
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( addrmode & Byte )
						*effective_address.UCharPtr |= imm.UChar;
					else if( addrmode & TwoBytes )
						*effective_address.UShortPtr |= imm.UShort;
					else if( addrmode & FourBytes )
						*effective_address.UInt32Ptr |= imm.UInt32;
					else if( addrmode & EightBytes )
						*effective_address.UInt64Ptr |= imm.UInt64;
				}
				// orb [reg+offset], reg
				else if( sec_addrmode & Register ) {
					const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( addrmode & Byte )
						*effective_address.UCharPtr |= vm->Regs[sec_regid].UChar;
					else if( addrmode & TwoBytes )
						*effective_address.UShortPtr |= vm->Regs[sec_regid].UShort;
					else if( addrmode & FourBytes )
						*effective_address.UInt32Ptr |= vm->Regs[sec_regid].UInt32;
					else if( addrmode & EightBytes )
						*effective_address.UInt64Ptr |= vm->Regs[sec_regid].UInt64;
				}
			}
			continue;
		}
		exec_xorb:; {
			// first addressing mode determines the destination.
			const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
			const uint8_t sec_addrmode = op_args & 255; // get secondary addressing mode.
			const uint8_t regid = op_args >> 8; // get 1st register id.
			if( addrmode & Register ) {
				// xorb reg, imm
				if( sec_addrmode & Immediate )
					vm->Regs[regid].UInt64 ^= *vm->Regs[regInstr].UInt64Ptr++;
				// xorb reg, reg
				else if( sec_addrmode & Register )
					vm->Regs[regid].UInt64 ^= vm->Regs[*vm->Regs[regInstr].UCharPtr++].UInt64;
				// xorb reg, [reg+offset]
				else if( sec_addrmode & RegIndirect ) {
					const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[sec_regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( sec_addrmode & Byte )
						vm->Regs[regid].UChar ^= *effective_address.UCharPtr;
					else if( sec_addrmode & TwoBytes )
						vm->Regs[regid].UShort ^= *effective_address.UShortPtr;
					else if( sec_addrmode & FourBytes )
						vm->Regs[regid].UInt32 ^= *effective_address.UInt32Ptr;
					else if( sec_addrmode & EightBytes )
						vm->Regs[regid].UInt64 ^= *effective_address.UInt64Ptr;
				}
			}
			else if( addrmode & RegIndirect ) {
				// xorb [reg+offset], imm
				if( sec_addrmode & Immediate ) {
					// have to store the imm value prior because the offset is stored AFTER the last operand.
					const union Value imm = (union Value){.UInt64 = *vm->Regs[regInstr].UInt64Ptr++};
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( addrmode & Byte )
						*effective_address.UCharPtr ^= imm.UChar;
					else if( addrmode & TwoBytes )
						*effective_address.UShortPtr ^= imm.UShort;
					else if( addrmode & FourBytes )
						*effective_address.UInt32Ptr ^= imm.UInt32;
					else if( addrmode & EightBytes )
						*effective_address.UInt64Ptr ^= imm.UInt64;
				}
				// xorb [reg+offset], reg
				else if( sec_addrmode & Register ) {
					const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( addrmode & Byte )
						*effective_address.UCharPtr ^= vm->Regs[sec_regid].UChar;
					else if( addrmode & TwoBytes )
						*effective_address.UShortPtr ^= vm->Regs[sec_regid].UShort;
					else if( addrmode & FourBytes )
						*effective_address.UInt32Ptr ^= vm->Regs[sec_regid].UInt32;
					else if( addrmode & EightBytes )
						*effective_address.UInt64Ptr ^= vm->Regs[sec_regid].UInt64;
				}
			}
			continue;
		}
		exec_notb:; {
			const uint8_t regid = *vm->Regs[regInstr].UCharPtr++;
			// notb reg
			if( addrmode & Register ) // invert a register's contents.
				vm->Regs[regid].UInt64 = ~vm->Regs[regid].UInt64;
			else if( addrmode & RegIndirect ) { // invert the contents of a memory address inside a register.
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				*effective_address.UInt64Ptr = ~*effective_address.UInt64Ptr;
			}
			continue;
		}
		exec_shl:; {
			// first addressing mode determines the destination.
			const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
			const uint8_t sec_addrmode = op_args & 255; // get secondary addressing mode.
			const uint8_t regid = op_args >> 8; // get 1st register id.
			if( addrmode & Register ) {
				// shl reg, imm
				if( sec_addrmode & Immediate )
					vm->Regs[regid].UInt64 <<= *vm->Regs[regInstr].UInt64Ptr++;
				// shl reg, reg
				else if( sec_addrmode & Register )
					vm->Regs[regid].UInt64 <<= vm->Regs[*vm->Regs[regInstr].UCharPtr++].UInt64;
				// shl reg, [reg+offset]
				else if( sec_addrmode & RegIndirect ) {
					const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[sec_regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( sec_addrmode & Byte )
						vm->Regs[regid].UChar <<= *effective_address.UCharPtr;
					else if( sec_addrmode & TwoBytes )
						vm->Regs[regid].UShort <<= *effective_address.UShortPtr;
					else if( sec_addrmode & FourBytes )
						vm->Regs[regid].UInt32 <<= *effective_address.UInt32Ptr;
					else if( sec_addrmode & EightBytes )
						vm->Regs[regid].UInt64 <<= *effective_address.UInt64Ptr;
				}
			}
			else if( addrmode & RegIndirect ) {
				// shl [reg+offset], imm
				if( sec_addrmode & Immediate ) {
					// have to store the imm value prior because the offset is stored AFTER the last operand.
					const union Value imm = (union Value){.UInt64 = *vm->Regs[regInstr].UInt64Ptr++};
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( addrmode & Byte )
						*effective_address.UCharPtr <<= imm.UChar;
					else if( addrmode & TwoBytes )
						*effective_address.UShortPtr <<= imm.UShort;
					else if( addrmode & FourBytes )
						*effective_address.UInt32Ptr <<= imm.UInt32;
					else if( addrmode & EightBytes )
						*effective_address.UInt64Ptr <<= imm.UInt64;
				}
				// shl [reg+offset], reg
				else if( sec_addrmode & Register ) {
					const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( addrmode & Byte )
						*effective_address.UCharPtr <<= vm->Regs[sec_regid].UChar;
					else if( addrmode & TwoBytes )
						*effective_address.UShortPtr <<= vm->Regs[sec_regid].UShort;
					else if( addrmode & FourBytes )
						*effective_address.UInt32Ptr <<= vm->Regs[sec_regid].UInt32;
					else if( addrmode & EightBytes )
						*effective_address.UInt64Ptr <<= vm->Regs[sec_regid].UInt64;
				}
			}
			continue;
		}
		exec_shr:; {
			// first addressing mode determines the destination.
			const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
			const uint8_t sec_addrmode = op_args & 255; // get secondary addressing mode.
			const uint8_t regid = op_args >> 8; // get 1st register id.
			if( addrmode & Register ) {
				// shr reg, imm
				if( sec_addrmode & Immediate )
					vm->Regs[regid].UInt64 >>= *vm->Regs[regInstr].UInt64Ptr++;
				// shr reg, reg
				else if( sec_addrmode & Register )
					vm->Regs[regid].UInt64 >>= vm->Regs[*vm->Regs[regInstr].UCharPtr++].UInt64;
				// shr reg, [reg+offset]
				else if( sec_addrmode & RegIndirect ) {
					const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[sec_regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( sec_addrmode & Byte )
						vm->Regs[regid].UChar >>= *effective_address.UCharPtr;
					else if( sec_addrmode & TwoBytes )
						vm->Regs[regid].UShort >>= *effective_address.UShortPtr;
					else if( sec_addrmode & FourBytes )
						vm->Regs[regid].UInt32 >>= *effective_address.UInt32Ptr;
					else if( sec_addrmode & EightBytes )
						vm->Regs[regid].UInt64 >>= *effective_address.UInt64Ptr;
				}
			}
			else if( addrmode & RegIndirect ) {
				// shr [reg+offset], imm
				if( sec_addrmode & Immediate ) {
					// have to store the imm value prior because the offset is stored AFTER the last operand.
					const union Value imm = (union Value){.UInt64 = *vm->Regs[regInstr].UInt64Ptr++};
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( addrmode & Byte )
						*effective_address.UCharPtr >>= imm.UChar;
					else if( addrmode & TwoBytes )
						*effective_address.UShortPtr >>= imm.UShort;
					else if( addrmode & FourBytes )
						*effective_address.UInt32Ptr >>= imm.UInt32;
					else if( addrmode & EightBytes )
						*effective_address.UInt64Ptr >>= imm.UInt64;
				}
				// shr [reg+offset], reg
				else if( sec_addrmode & Register ) {
					const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( addrmode & Byte )
						*effective_address.UCharPtr >>= vm->Regs[sec_regid].UChar;
					else if( addrmode & TwoBytes )
						*effective_address.UShortPtr >>= vm->Regs[sec_regid].UShort;
					else if( addrmode & FourBytes )
						*effective_address.UInt32Ptr >>= vm->Regs[sec_regid].UInt32;
					else if( addrmode & EightBytes )
						*effective_address.UInt64Ptr >>= vm->Regs[sec_regid].UInt64;
				}
			}
			continue;
		}
		exec_inc:; {
			const uint8_t regid = *vm->Regs[regInstr].UCharPtr++;
			// inc reg
			if( addrmode & Register ) // increment a register's contents.
				++vm->Regs[regid].UInt64;
			// inc [reg+offset]
			else if( addrmode & RegIndirect ) { // increment the contents of a memory address inside a register.
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				++*effective_address.UInt64Ptr;
			}
			continue;
		}
		exec_dec:; {
			const uint8_t regid = *vm->Regs[regInstr].UCharPtr++;
			// dec reg
			if( addrmode & Register ) // decrement a register's contents.
				--vm->Regs[regid].UInt64;
			// dec [reg+offset]
			else if( addrmode & RegIndirect ) { // decrement the contents of a memory address inside a register.
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				--*effective_address.UInt64Ptr;
			}
			continue;
		}
		exec_neg:; {
			const uint8_t regid = *vm->Regs[regInstr].UCharPtr++;
			// neg reg
			if( addrmode & Register ) // negate a register's contents.
				vm->Regs[regid].UInt64 = -vm->Regs[regid].UInt64;
			// neg [reg+offset]
			else if( addrmode & RegIndirect ) { // negate the contents of a memory address inside a register.
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				*effective_address.UInt64Ptr = -*effective_address.UInt64Ptr;
			}
			continue;
		}
#if FLOATING_POINT_OPS
		exec_incf:; {
			const uint8_t regid = *vm->Regs[regInstr].UCharPtr++;
			if( addrmode & Register ) {
				// incf reg
				if( addrmode & FourBytes )
					++vm->Regs[regid].Float;
				else if( addrmode & EightBytes )
					++vm->Regs[regid].Double;
			}
			else if( addrmode & RegIndirect ) {
				// incf [reg+offset]
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				if( addrmode & FourBytes )
					++*effective_address.FloatPtr;
				else if( addrmode & EightBytes )
					++*effective_address.DoublePtr;
			}
			continue;
		}
		exec_decf:; {
			const uint8_t regid = *vm->Regs[regInstr].UCharPtr++;
			if( addrmode & Register ) {
				// decf reg
				if( addrmode & FourBytes )
					--vm->Regs[regid].Float;
				else if( addrmode & EightBytes )
					--vm->Regs[regid].Double;
			}
			else if( addrmode & RegIndirect ) {
				// decf [reg+offset]
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				if( addrmode & FourBytes )
					--*effective_address.FloatPtr;
				else if( addrmode & EightBytes )
					--*effective_address.DoublePtr;
			}
			continue;
		}
		exec_negf:; {
			const uint8_t regid = *vm->Regs[regInstr].UCharPtr++;
			if( addrmode & Register ) {
				// decf reg
				if( addrmode & FourBytes )
					vm->Regs[regid].Float = -vm->Regs[regid].Float;
				else if( addrmode & EightBytes )
					vm->Regs[regid].Double = -vm->Regs[regid].Double;
			}
			else if( addrmode & RegIndirect ) {
				// decf [reg+offset]
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				if( addrmode & FourBytes )
					*effective_address.FloatPtr = -*effective_address.FloatPtr;
				else if( addrmode & EightBytes )
					*effective_address.DoublePtr = -*effective_address.DoublePtr;
			}
			continue;
		}
#endif
		exec_lt:; {
			// first addressing mode determines the destination.
			const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
			const uint8_t sec_addrmode = op_args & 255; // get secondary addressing mode.
			const uint8_t regid = op_args >> 8; // get 1st register id.
			if( addrmode & Register ) {
				// lt reg, imm
				if( sec_addrmode & Immediate )
					vm->ZeroFlag = vm->Regs[regid].UInt64 < *vm->Regs[regInstr].UInt64Ptr++;
				// lt reg, reg
				else if( sec_addrmode & Register )
					vm->ZeroFlag = vm->Regs[regid].UInt64 < vm->Regs[*vm->Regs[regInstr].UCharPtr++].UInt64;
				// lt reg, [reg+offset]
				else if( sec_addrmode & RegIndirect ) {
					const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[sec_regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( sec_addrmode & Byte )
						vm->ZeroFlag = vm->Regs[regid].UChar < *effective_address.UCharPtr;
					else if( sec_addrmode & TwoBytes )
						vm->ZeroFlag = vm->Regs[regid].UShort < *effective_address.UShortPtr;
					else if( sec_addrmode & FourBytes )
						vm->ZeroFlag = vm->Regs[regid].UInt32 < *effective_address.UInt32Ptr;
					else if( sec_addrmode & EightBytes )
						vm->ZeroFlag = vm->Regs[regid].UInt64 < *effective_address.UInt64Ptr;
				}
			}
			else if( addrmode & RegIndirect ) {
				// lt [reg+offset], imm
				if( sec_addrmode & Immediate ) {
					// have to store the imm value prior because the offset is stored AFTER the last operand.
					const union Value imm = (union Value){.UInt64 = *vm->Regs[regInstr].UInt64Ptr++};
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( addrmode & Byte )
						vm->ZeroFlag = *effective_address.UCharPtr < imm.UChar;
					else if( addrmode & TwoBytes )
						vm->ZeroFlag = *effective_address.UShortPtr < imm.UShort;
					else if( addrmode & FourBytes )
						vm->ZeroFlag = *effective_address.UInt32Ptr < imm.UInt32;
					else if( addrmode & EightBytes )
						vm->ZeroFlag = *effective_address.UInt64Ptr < imm.UInt64;
				}
				// lt [reg+offset], reg
				else if( sec_addrmode & Register ) {
					const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( addrmode & Byte )
						vm->ZeroFlag = *effective_address.UCharPtr < vm->Regs[sec_regid].UChar;
					else if( addrmode & TwoBytes )
						vm->ZeroFlag = *effective_address.UShortPtr < vm->Regs[sec_regid].UShort;
					else if( addrmode & FourBytes )
						vm->ZeroFlag = *effective_address.UInt32Ptr < vm->Regs[sec_regid].UInt32;
					else if( addrmode & EightBytes )
						vm->ZeroFlag = *effective_address.UInt64Ptr < vm->Regs[sec_regid].UInt64;
				}
			}
			continue;
		}
		exec_gt:; {
			// first addressing mode determines the destination.
			const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
			const uint8_t sec_addrmode = op_args & 255; // get secondary addressing mode.
			const uint8_t regid = op_args >> 8; // get 1st register id.
			if( addrmode & Register ) {
				// gt reg, imm
				if( sec_addrmode & Immediate )
					vm->ZeroFlag = vm->Regs[regid].UInt64 > *vm->Regs[regInstr].UInt64Ptr++;
				// gt reg, reg
				else if( sec_addrmode & Register )
					vm->ZeroFlag = vm->Regs[regid].UInt64 > vm->Regs[*vm->Regs[regInstr].UCharPtr++].UInt64;
				// gt reg, [reg+offset]
				else if( sec_addrmode & RegIndirect ) {
					const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[sec_regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( sec_addrmode & Byte )
						vm->ZeroFlag = vm->Regs[regid].UChar > *effective_address.UCharPtr;
					else if( sec_addrmode & TwoBytes )
						vm->ZeroFlag = vm->Regs[regid].UShort > *effective_address.UShortPtr;
					else if( sec_addrmode & FourBytes )
						vm->ZeroFlag = vm->Regs[regid].UInt32 > *effective_address.UInt32Ptr;
					else if( sec_addrmode & EightBytes )
						vm->ZeroFlag = vm->Regs[regid].UInt64 > *effective_address.UInt64Ptr;
				}
			}
			else if( addrmode & RegIndirect ) {
				// gt [reg+offset], imm
				if( sec_addrmode & Immediate ) {
					// have to store the imm value prior because the offset is stored AFTER the last operand.
					const union Value imm = (union Value){.UInt64 = *vm->Regs[regInstr].UInt64Ptr++};
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( addrmode & Byte )
						vm->ZeroFlag = *effective_address.UCharPtr > imm.UChar;
					else if( addrmode & TwoBytes )
						vm->ZeroFlag = *effective_address.UShortPtr > imm.UShort;
					else if( addrmode & FourBytes )
						vm->ZeroFlag = *effective_address.UInt32Ptr > imm.UInt32;
					else if( addrmode & EightBytes )
						vm->ZeroFlag = *effective_address.UInt64Ptr > imm.UInt64;
				}
				// gt [reg+offset], reg
				else if( sec_addrmode & Register ) {
					const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( addrmode & Byte )
						vm->ZeroFlag = *effective_address.UCharPtr > vm->Regs[sec_regid].UChar;
					else if( addrmode & TwoBytes )
						vm->ZeroFlag = *effective_address.UShortPtr > vm->Regs[sec_regid].UShort;
					else if( addrmode & FourBytes )
						vm->ZeroFlag = *effective_address.UInt32Ptr > vm->Regs[sec_regid].UInt32;
					else if( addrmode & EightBytes )
						vm->ZeroFlag = *effective_address.UInt64Ptr > vm->Regs[sec_regid].UInt64;
				}
			}
			continue;
		}
		exec_cmp:; {
			// first addressing mode determines the destination.
			const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
			const uint8_t sec_addrmode = op_args & 255; // get secondary addressing mode.
			const uint8_t regid = op_args >> 8; // get 1st register id.
			if( addrmode & Register ) {
				// cmp reg, imm
				if( sec_addrmode & Immediate )
					vm->ZeroFlag = vm->Regs[regid].UInt64 == *vm->Regs[regInstr].UInt64Ptr++;
				// cmp reg, reg
				else if( sec_addrmode & Register )
					vm->ZeroFlag = vm->Regs[regid].UInt64 == vm->Regs[*vm->Regs[regInstr].UCharPtr++].UInt64;
				// cmp reg, [reg+offset]
				else if( sec_addrmode & RegIndirect ) {
					const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[sec_regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( sec_addrmode & Byte )
						vm->ZeroFlag = vm->Regs[regid].UChar == *effective_address.UCharPtr;
					else if( sec_addrmode & TwoBytes )
						vm->ZeroFlag = vm->Regs[regid].UShort == *effective_address.UShortPtr;
					else if( sec_addrmode & FourBytes )
						vm->ZeroFlag = vm->Regs[regid].UInt32 == *effective_address.UInt32Ptr;
					else if( sec_addrmode & EightBytes )
						vm->ZeroFlag = vm->Regs[regid].UInt64 == *effective_address.UInt64Ptr;
				}
			}
			else if( addrmode & RegIndirect ) {
				// cmp [reg+offset], imm
				if( sec_addrmode & Immediate ) {
					// have to store the imm value prior because the offset is stored AFTER the last operand.
					const union Value imm = (union Value){.UInt64 = *vm->Regs[regInstr].UInt64Ptr++};
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( addrmode & Byte )
						vm->ZeroFlag = *effective_address.UCharPtr == imm.UChar;
					else if( addrmode & TwoBytes )
						vm->ZeroFlag = *effective_address.UShortPtr == imm.UShort;
					else if( addrmode & FourBytes )
						vm->ZeroFlag = *effective_address.UInt32Ptr == imm.UInt32;
					else if( addrmode & EightBytes )
						vm->ZeroFlag = *effective_address.UInt64Ptr == imm.UInt64;
				}
				// cmp [reg+offset], reg
				else if( sec_addrmode & Register ) {
					const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( addrmode & Byte )
						vm->ZeroFlag = *effective_address.UCharPtr == vm->Regs[sec_regid].UChar;
					else if( addrmode & TwoBytes )
						vm->ZeroFlag = *effective_address.UShortPtr == vm->Regs[sec_regid].UShort;
					else if( addrmode & FourBytes )
						vm->ZeroFlag = *effective_address.UInt32Ptr == vm->Regs[sec_regid].UInt32;
					else if( addrmode & EightBytes )
						vm->ZeroFlag = *effective_address.UInt64Ptr == vm->Regs[sec_regid].UInt64;
				}
			}
			continue;
		}
		exec_neq:; {
			// first addressing mode determines the destination.
			const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
			const uint8_t sec_addrmode = op_args & 255; // get secondary addressing mode.
			const uint8_t regid = op_args >> 8; // get 1st register id.
			if( addrmode & Register ) {
				// neq reg, imm
				if( sec_addrmode & Immediate )
					vm->ZeroFlag = vm->Regs[regid].UInt64 != *vm->Regs[regInstr].UInt64Ptr++;
				// neq reg, reg
				else if( sec_addrmode & Register )
					vm->ZeroFlag = vm->Regs[regid].UInt64 != vm->Regs[*vm->Regs[regInstr].UCharPtr++].UInt64;
				// neq reg, [reg+offset]
				else if( sec_addrmode & RegIndirect ) {
					const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[sec_regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( sec_addrmode & Byte )
						vm->ZeroFlag = vm->Regs[regid].UChar != *effective_address.UCharPtr;
					else if( sec_addrmode & TwoBytes )
						vm->ZeroFlag = vm->Regs[regid].UShort != *effective_address.UShortPtr;
					else if( sec_addrmode & FourBytes )
						vm->ZeroFlag = vm->Regs[regid].UInt32 != *effective_address.UInt32Ptr;
					else if( sec_addrmode & EightBytes )
						vm->ZeroFlag = vm->Regs[regid].UInt64 != *effective_address.UInt64Ptr;
				}
			}
			else if( addrmode & RegIndirect ) {
				// neq [reg+offset], imm
				if( sec_addrmode & Immediate ) {
					// have to store the imm value prior because the offset is stored AFTER the last operand.
					const union Value imm = (union Value){.UInt64 = *vm->Regs[regInstr].UInt64Ptr++};
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( addrmode & Byte )
						vm->ZeroFlag = *effective_address.UCharPtr != imm.UChar;
					else if( addrmode & TwoBytes )
						vm->ZeroFlag = *effective_address.UShortPtr != imm.UShort;
					else if( addrmode & FourBytes )
						vm->ZeroFlag = *effective_address.UInt32Ptr != imm.UInt32;
					else if( addrmode & EightBytes )
						vm->ZeroFlag = *effective_address.UInt64Ptr != imm.UInt64;
				}
				// neq [reg+offset], reg
				else if( addrmode & Register ) {
					const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( addrmode & Byte )
						vm->ZeroFlag = *effective_address.UCharPtr != vm->Regs[sec_regid].UChar;
					else if( addrmode & TwoBytes )
						vm->ZeroFlag = *effective_address.UShortPtr != vm->Regs[sec_regid].UShort;
					else if( addrmode & FourBytes )
						vm->ZeroFlag = *effective_address.UInt32Ptr != vm->Regs[sec_regid].UInt32;
					else if( addrmode & EightBytes )
						vm->ZeroFlag = *effective_address.UInt64Ptr != vm->Regs[sec_regid].UInt64;
				}
			}
			continue;
		}
#if FLOATING_POINT_OPS
		exec_ltf:; {
			// first addressing mode determines the destination.
			const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
			const uint8_t sec_addrmode = op_args & 255; // get secondary addressing mode.
			const uint8_t regid = op_args >> 8; // get 1st register id.
			
			if( addrmode & Register ) {
				// ltf reg, imm
				if( sec_addrmode & Immediate ) {
					if( addrmode & FourBytes )
						vm->ZeroFlag = vm->Regs[regid].Float < *vm->Regs[regInstr].FloatPtr++;
					else if( addrmode & EightBytes )
						vm->ZeroFlag = vm->Regs[regid].Double < *vm->Regs[regInstr].DoublePtr++;
				}
				// ltf reg, reg
				else if( sec_addrmode & Register ) {
					if( addrmode & FourBytes )
						vm->ZeroFlag = vm->Regs[regid].Float < vm->Regs[*vm->Regs[regInstr].UCharPtr++].Float;
					else if( addrmode & EightBytes )
						vm->ZeroFlag = vm->Regs[regid].Double < vm->Regs[*vm->Regs[regInstr].UCharPtr++].Double;
				}
				// ltf reg, [reg+offset]
				else if( sec_addrmode & RegIndirect ) {
					const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[sec_regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( sec_addrmode & FourBytes )
						vm->ZeroFlag = vm->Regs[regid].Float < *effective_address.FloatPtr;
					else if( sec_addrmode & EightBytes )
						vm->ZeroFlag = vm->Regs[regid].Double < *effective_address.DoublePtr;
				}
			}
			else if( addrmode & RegIndirect ) {
				// ltf [reg+offset], imm
				if( sec_addrmode & Immediate ) {
					// have to store the imm value prior because the offset is stored AFTER the last operand.
					const union Value imm = addrmode & FourBytes ? (union Value){.Float = *vm->Regs[regInstr].FloatPtr++} : (union Value){.Double = *vm->Regs[regInstr].DoublePtr++};
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( addrmode & FourBytes )
						vm->ZeroFlag = *effective_address.FloatPtr < imm.Float;
					else if( addrmode & EightBytes )
						vm->ZeroFlag = *effective_address.DoublePtr < imm.Double;
				}
				// ltf [reg+offset], reg
				else if( sec_addrmode & Register ) {
					const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( addrmode & FourBytes )
						vm->ZeroFlag = *effective_address.FloatPtr < vm->Regs[sec_regid].Float;
					else if( addrmode & EightBytes )
						vm->ZeroFlag = *effective_address.DoublePtr < vm->Regs[sec_regid].Double;
				}
			}
			continue;
		}
		exec_gtf:; {
			// first addressing mode determines the destination.
			const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
			const uint8_t sec_addrmode = op_args & 255; // get secondary addressing mode.
			const uint8_t regid = op_args >> 8; // get 1st register id.
			
			if( addrmode & Register ) {
				// gtf reg, imm
				if( sec_addrmode & Immediate ) {
					if( addrmode & FourBytes )
						vm->ZeroFlag = vm->Regs[regid].Float > *vm->Regs[regInstr].FloatPtr++;
					else if( addrmode & EightBytes )
						vm->ZeroFlag = vm->Regs[regid].Double > *vm->Regs[regInstr].DoublePtr++;
				}
				// gtf reg, reg
				else if( sec_addrmode & Register ) {
					if( addrmode & FourBytes )
						vm->ZeroFlag = vm->Regs[regid].Float > vm->Regs[*vm->Regs[regInstr].UCharPtr++].Float;
					else if( addrmode & EightBytes )
						vm->ZeroFlag = vm->Regs[regid].Double > vm->Regs[*vm->Regs[regInstr].UCharPtr++].Double;
				}
				// gtf reg, [reg+offset]
				else if( sec_addrmode & RegIndirect ) {
					const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[sec_regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( sec_addrmode & FourBytes )
						vm->ZeroFlag = vm->Regs[regid].Float > *effective_address.FloatPtr;
					else if( sec_addrmode & EightBytes )
						vm->ZeroFlag = vm->Regs[regid].Double > *effective_address.DoublePtr;
				}
			}
			else if( addrmode & RegIndirect ) {
				// gtf [reg+offset], imm
				if( sec_addrmode & Immediate ) {
					// have to store the imm value prior because the offset is stored AFTER the last operand.
					const union Value imm = addrmode & FourBytes ? (union Value){.Float = *vm->Regs[regInstr].FloatPtr++} : (union Value){.Double = *vm->Regs[regInstr].DoublePtr++};
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( addrmode & FourBytes )
						vm->ZeroFlag = *effective_address.FloatPtr > imm.Float;
					else if( addrmode & EightBytes )
						vm->ZeroFlag = *effective_address.DoublePtr > imm.Double;
				}
				// gtf [reg+offset], reg
				else if( sec_addrmode & Register ) {
					const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( addrmode & FourBytes )
						vm->ZeroFlag = *effective_address.FloatPtr > vm->Regs[sec_regid].Float;
					else if( addrmode & EightBytes )
						vm->ZeroFlag = *effective_address.DoublePtr > vm->Regs[sec_regid].Double;
				}
			}
			continue;
		}
		exec_cmpf:; {
			// first addressing mode determines the destination.
			const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
			const uint8_t sec_addrmode = op_args & 255; // get secondary addressing mode.
			const uint8_t regid = op_args >> 8; // get 1st register id.
			
			if( addrmode & Register ) {
				// cmpf reg, imm
				if( sec_addrmode & Immediate ) {
					if( addrmode & FourBytes )
						vm->ZeroFlag = vm->Regs[regid].Float == *vm->Regs[regInstr].FloatPtr++;
					else if( addrmode & EightBytes )
						vm->ZeroFlag = vm->Regs[regid].Double == *vm->Regs[regInstr].DoublePtr++;
				}
				// cmpf reg, reg
				else if( sec_addrmode & Register ) {
					if( addrmode & FourBytes )
						vm->ZeroFlag = vm->Regs[regid].Float == vm->Regs[*vm->Regs[regInstr].UCharPtr++].Float;
					else if( addrmode & EightBytes )
						vm->ZeroFlag = vm->Regs[regid].Double == vm->Regs[*vm->Regs[regInstr].UCharPtr++].Double;
				}
				// cmpf reg, [reg+offset]
				else if( sec_addrmode & RegIndirect ) {
					const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[sec_regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( sec_addrmode & FourBytes )
						vm->ZeroFlag = vm->Regs[regid].Float == *effective_address.FloatPtr;
					else if( sec_addrmode & EightBytes )
						vm->ZeroFlag = vm->Regs[regid].Double == *effective_address.DoublePtr;
				}
			}
			else if( addrmode & RegIndirect ) {
				// cmpf [reg+offset], imm
				if( sec_addrmode & Immediate ) {
					// have to store the imm value prior because the offset is stored AFTER the last operand.
					const union Value imm = addrmode & FourBytes ? (union Value){.Float = *vm->Regs[regInstr].FloatPtr++} : (union Value){.Double = *vm->Regs[regInstr].DoublePtr++};
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( addrmode & FourBytes )
						vm->ZeroFlag = *effective_address.FloatPtr == imm.Float;
					else if( addrmode & EightBytes )
						vm->ZeroFlag = *effective_address.DoublePtr == imm.Double;
				}
				// cmpf [reg+offset], reg
				else if( sec_addrmode & Register ) {
					const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( addrmode & FourBytes )
						vm->ZeroFlag = *effective_address.FloatPtr == vm->Regs[sec_regid].Float;
					else if( addrmode & EightBytes )
						vm->ZeroFlag = *effective_address.DoublePtr == vm->Regs[sec_regid].Double;
				}
			}
			continue;
		}
		exec_neqf:; {
			// first addressing mode determines the destination.
			const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
			const uint8_t sec_addrmode = op_args & 255; // get secondary addressing mode.
			const uint8_t regid = op_args >> 8; // get 1st register id.
			
			if( addrmode & Register ) {
				// neqf reg, imm
				if( sec_addrmode & Immediate ) {
					if( addrmode & FourBytes )
						vm->ZeroFlag = vm->Regs[regid].Float != *vm->Regs[regInstr].FloatPtr++;
					else if( addrmode & EightBytes )
						vm->ZeroFlag = vm->Regs[regid].Double != *vm->Regs[regInstr].DoublePtr++;
				}
				// neqf reg, reg
				else if( sec_addrmode & Register ) {
					if( addrmode & FourBytes )
						vm->ZeroFlag = vm->Regs[regid].Float != vm->Regs[*vm->Regs[regInstr].UCharPtr++].Float;
					else if( addrmode & EightBytes )
						vm->ZeroFlag = vm->Regs[regid].Double != vm->Regs[*vm->Regs[regInstr].UCharPtr++].Double;
				}
				// neqf reg, [reg+offset]
				else if( sec_addrmode & RegIndirect ) {
					const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[sec_regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( sec_addrmode & FourBytes )
						vm->ZeroFlag = vm->Regs[regid].Float != *effective_address.FloatPtr;
					else if( sec_addrmode & EightBytes )
						vm->ZeroFlag = vm->Regs[regid].Double != *effective_address.DoublePtr;
				}
			}
			else if( addrmode & RegIndirect ) {
				// neqf [reg+offset], imm
				if( sec_addrmode & Immediate ) {
					// have to store the imm value prior because the offset is stored AFTER the last operand.
					const union Value imm = addrmode & FourBytes ? (union Value){.Float = *vm->Regs[regInstr].FloatPtr++} : (union Value){.Double = *vm->Regs[regInstr].DoublePtr++};
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( addrmode & FourBytes )
						vm->ZeroFlag = *effective_address.FloatPtr != imm.Float;
					else if( addrmode & EightBytes )
						vm->ZeroFlag = *effective_address.DoublePtr != imm.Double;
				}
				// neqf [reg+offset], reg
				else if( sec_addrmode & Register ) {
					const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
					const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
					if( addrmode & FourBytes )
						vm->ZeroFlag = *effective_address.FloatPtr != vm->Regs[sec_regid].Float;
					else if( addrmode & EightBytes )
						vm->ZeroFlag = *effective_address.DoublePtr != vm->Regs[sec_regid].Double;
				}
			}
			continue;
		}
#endif
		exec_jmp:; {
			// jmp imm
			if( addrmode & Immediate ) {
				const int64_t offset = *vm->Regs[regInstr].Int64Ptr++;
				vm->Regs[regInstr].UCharPtr += offset;
			}
			// jmp reg
			else if( addrmode & Register ) {
				const uint8_t regid = *vm->Regs[regInstr].UCharPtr++;
				vm->Regs[regInstr].UCharPtr += vm->Regs[regid].Int64;
			}
			// jmp [reg+offset]
			else if( addrmode & RegIndirect ) {
				const uint8_t regid = *vm->Regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					vm->Regs[regInstr].UCharPtr += *effective_address.CharPtr;
				else if( addrmode & TwoBytes )
					vm->Regs[regInstr].UCharPtr += *effective_address.ShortPtr;
				else if( addrmode & FourBytes )
					vm->Regs[regInstr].UCharPtr += *effective_address.Int32Ptr;
				else if( addrmode & EightBytes )
					vm->Regs[regInstr].UCharPtr += *effective_address.Int64Ptr;
			}
			continue;
		}
		exec_jz:; {
			// jz imm
			if( addrmode & Immediate ) {
				const int64_t offset = *vm->Regs[regInstr].Int64Ptr++;
				!vm->ZeroFlag ? (vm->Regs[regInstr].UCharPtr += offset) : (void)vm->ZeroFlag;
			}
			// jz reg
			else if( addrmode & Register ) {
				const uint8_t regid = *vm->Regs[regInstr].UCharPtr++;
				!vm->ZeroFlag ? (vm->Regs[regInstr].UCharPtr += vm->Regs[regid].Int64) : (void)vm->ZeroFlag;
			}
			// jz [reg+offset]
			else if( addrmode & RegIndirect ) {
				const uint8_t regid = *vm->Regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					!vm->ZeroFlag ? (vm->Regs[regInstr].UCharPtr += *effective_address.CharPtr) : (void)vm->ZeroFlag;
				else if( addrmode & TwoBytes )
					!vm->ZeroFlag ? (vm->Regs[regInstr].UCharPtr += *effective_address.ShortPtr) : (void)vm->ZeroFlag;
				else if( addrmode & FourBytes )
					!vm->ZeroFlag ? (vm->Regs[regInstr].UCharPtr += *effective_address.Int32Ptr) : (void)vm->ZeroFlag;
				else if( addrmode & EightBytes )
					!vm->ZeroFlag ? (vm->Regs[regInstr].UCharPtr += *effective_address.Int64Ptr) : (void)vm->ZeroFlag;
			}
			continue;
		}
		exec_jnz:; {
			// jnz imm
			if( addrmode & Immediate ) {
				const int64_t offset = *vm->Regs[regInstr].Int64Ptr++;
				vm->ZeroFlag ? (vm->Regs[regInstr].UCharPtr += offset) : (void)vm->ZeroFlag;
			}
			// jnz reg
			else if( addrmode & Register ) {
				const uint8_t regid = *vm->Regs[regInstr].UCharPtr++;
				vm->ZeroFlag ? (vm->Regs[regInstr].UCharPtr += vm->Regs[regid].Int64) : (void)vm->ZeroFlag;
			}
			// jnz [reg+offset]
			else if( addrmode & RegIndirect ) {
				const uint8_t regid = *vm->Regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					vm->ZeroFlag ? (vm->Regs[regInstr].UCharPtr += *effective_address.CharPtr) : (void)vm->ZeroFlag;
				else if( addrmode & TwoBytes )
					vm->ZeroFlag ? (vm->Regs[regInstr].UCharPtr += *effective_address.ShortPtr) : (void)vm->ZeroFlag;
				else if( addrmode & FourBytes )
					vm->ZeroFlag ? (vm->Regs[regInstr].UCharPtr += *effective_address.Int32Ptr) : (void)vm->ZeroFlag;
				else if( addrmode & EightBytes )
					vm->ZeroFlag ? (vm->Regs[regInstr].UCharPtr += *effective_address.Int64Ptr) : (void)vm->ZeroFlag;
			}
			continue;
		}
		exec_call:; {
			*--vm->Regs[regStk].SelfPtr = vm->Regs[regInstr];	// push rip
			*--vm->Regs[regStk].SelfPtr = vm->Regs[regBase];	// push rbp
			vm->Regs[regBase] = vm->Regs[regStk];	// mov rbp, rsp
			
			// we have to truncate the index from our data contents to an unsigned 32-bit int
			// because the function table is limited by 4 billion.
			// call imm
			if( addrmode & Immediate ) {
				uint8_t *offset = GetFunctionOffsetByIndex(*vm->Regs[regInstr].UInt32Ptr++, vm->ScriptHdr.UCharPtr);
				if( !offset )
					continue;
				vm->Regs[regInstr].UCharPtr = offset;
			}
			// call reg
			else if( addrmode & Register ) {
				const uint8_t regid = *vm->Regs[regInstr].UCharPtr++;
				uint8_t *offset = GetFunctionOffsetByIndex(vm->Regs[regid].UInt32, vm->ScriptHdr.UCharPtr);
				if( !offset )
					continue;
				vm->Regs[regInstr].UCharPtr = offset;
			}
			// call [reg+offset]
			else if( addrmode & RegIndirect ) {
				const uint8_t regid = *vm->Regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				uint8_t *offset = NULL;
				if( addrmode & Byte )
					offset = GetFunctionOffsetByIndex(*effective_address.UCharPtr, vm->ScriptHdr.UCharPtr);
				else if( addrmode & TwoBytes )
					offset = GetFunctionOffsetByIndex(*effective_address.UShortPtr, vm->ScriptHdr.UCharPtr);
				else if( addrmode & FourBytes )
					offset = GetFunctionOffsetByIndex(*effective_address.UInt32Ptr, vm->ScriptHdr.UCharPtr);
				else if( addrmode & EightBytes )
					offset = GetFunctionOffsetByIndex(*effective_address.UInt64Ptr, vm->ScriptHdr.UCharPtr);
				if( !offset )
					continue;
				
				vm->Regs[regInstr].UCharPtr = offset;
			}
			continue;
		}
		exec_ret:; {
			vm->Regs[regStk] = vm->Regs[regBase];	// mov rsp, rbp
			vm->Regs[regBase] = *vm->Regs[regStk].SelfPtr++;	// pop rbp
			
			// if we're popping Main's (or whatever called func's) RBP, then halt the whole program.
			if( vm->Regs[regBase].SelfPtr==MainBasePtr )
				goto *dispatch[halt];
			
			vm->Regs[regInstr] = *vm->Regs[regStk].SelfPtr++;	// pop rip
			continue;
		}
		exec_syscall:; {
			//const uint64_t calldata = *vm->Regs[regInstr].UInt64Ptr++;
			// how many args given to the native call.
			const size_t argcount = *vm->Regs[regInstr].UInt32Ptr++; //calldata & 0xFFffFFff;
			// how many bytes does the native return?
			//const size_t retbytes = calldata >> 32;
			
			uint32_t index = 0xFFFFFFFF;
			// syscall argcount, imm
			if( addrmode & Immediate )
				index = (uint32_t) *vm->Regs[regInstr].UInt64Ptr++;
			// syscall argcount, reg
			else if( addrmode & Register ) {
				const uint8_t regid = *vm->Regs[regInstr].UCharPtr++;
				index = vm->Regs[regid].UInt32;
			}
			// syscall argcount, [reg+offset]
			else if( addrmode & RegIndirect ) {
				const uint8_t regid = *vm->Regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					index = *effective_address.UCharPtr;
				else if( addrmode & TwoBytes )
					index = *effective_address.UShortPtr;
				else if( addrmode & FourBytes )
					index = *effective_address.UInt32Ptr;
				else if( addrmode & EightBytes )
					index = (uint32_t) *effective_address.UInt64Ptr;
			}
			// native call interface
			// Limitations:
			//  - any argument larger than 8 bytes must be passed as a pointer.
			//  - any return value larger than 8 bytes must be passed as a hidden pointer argument and render the function as void.
			// void NativeFunc(struct Tagha *sys, union Value *retval, const size_t args, union Value params[static args]);
			void (*const NativeFunc)() = Map_Get(&vm->Natives, GetFunctionNameByIndex(index, vm->ScriptHdr.UCharPtr)).VoidFunc;
			
			union Value params[argcount];
			const size_t bytecount = sizeof(union Value) * argcount;
			memset(params, 0, bytecount);
			
			// save stack space by using the registers, starting at Zain, for passing arguments.
			// the first 6 registers can then be used for other data operations.
			if( argcount <= 16 ) {
				memcpy(params, vm->Regs+regZain, bytecount);
			}
			// if the native has more than 16 params, get from both registers and stack.
			else if( argcount > 16 ) {
				memcpy(params, vm->Regs+regZain, sizeof(union Value) * 16);
				memcpy(params+16, vm->Regs[regStk].SelfPtr, sizeof(union Value) * (argcount-16));
				vm->Regs[regStk].SelfPtr += (argcount-16);
			}
			vm->Regs[regAlaf].UInt64 = 0;
			(*NativeFunc)(vm, vm->Regs+regAlaf, argcount, params);
			continue;
		}
	}
	return -1;
}

int32_t Tagha_RunScript(struct Tagha *const __restrict vm)
{
	if( !vm )
		return -1;
	
	if( *vm->ScriptHdr.UShortPtr != 0xC0DE ) {
		puts("Tagha_RunScript :: ERROR: Script has invalid main verifier.");
		return -1;
	}
	
	uint8_t *main_offset = GetFunctionOffsetByName("main", vm->ScriptHdr.UCharPtr);
	if( !main_offset ) {
		puts("Tagha_RunScript :: ERROR: script contains no 'main' function.");
		return -1;
	}
	vm->Regs[regInstr].UCharPtr = main_offset;
	
	// push argv and argc to registers.
	// TODO: use a vector for these.
	//vm->Regs[res].UInt64 = (uintptr_t)vm->Argv;
	//vm->Regs[rds].Int64 = vm->Argc;
	
	// check out stack size and align it by the size of union Value.
	size_t stacksize = *(uint32_t *)(vm->ScriptHdr.UCharPtr+2);
	stacksize = (stacksize + (sizeof(union Value)-1)) & -(sizeof(union Value));
	if( !stacksize )
		return -1;
	
	union Value Stack[stacksize+1];
	//union Value *StackLimit = Stack + stacksize+1;
	vm->Regs[regStk].SelfPtr = vm->Regs[regBase].SelfPtr = Stack + stacksize;
	
	(--vm->Regs[regStk].SelfPtr)->Int64 = -1L;	// push bullshit ret address.
	*--vm->Regs[regStk].SelfPtr = vm->Regs[regBase]; // push rbp
	return Tagha_Exec(vm);
}

int32_t Tagha_CallFunc(struct Tagha *const __restrict vm, const char *__restrict funcname)
{
	if( !vm or !funcname )
		return -1;
	
	if( *vm->ScriptHdr.UShortPtr != 0xD11 ) {
		puts("Tagha_RunScript :: ERROR: Script has invalid dll verifier.");
		return -1;
	}
	
	uint8_t *func_offset = GetFunctionOffsetByName(funcname, vm->ScriptHdr.UCharPtr);
	if( !func_offset ) {
		puts("Tagha_CallFunc :: ERROR: cannot find function: '%s'.");
		return -1;
	}
	vm->Regs[regInstr].UCharPtr = func_offset;
	
	// push argv and argc to registers.
	// TODO: use a vector for these.
	//vm->Regs[res].UInt64 = (uintptr_t)vm->Argv;
	//vm->Regs[rds].Int64 = vm->Argc;
	
	// check out stack size and align it by the size of union Value.
	size_t stacksize = *(uint32_t *)(vm->ScriptHdr.UCharPtr+2);
	stacksize = (stacksize + (sizeof(union Value)-1)) & -(sizeof(union Value));
	if( !stacksize )
		return -1;
	
	union Value Stack[stacksize+1];
	//union Value *StackLimit = Stack + stacksize+1;
	vm->Regs[regStk].SelfPtr = vm->Regs[regBase].SelfPtr = Stack + stacksize;
	
	(--vm->Regs[regStk].SelfPtr)->UCharPtr = vm->Regs[regInstr].UCharPtr+1;	// push return address.
	vm->Regs[regInstr].UCharPtr = func_offset;
	*--vm->Regs[regStk].SelfPtr = vm->Regs[regBase]; // push rbp
	return Tagha_Exec(vm);
}

/////////////////////////////////////////////////////////////////////////////////
