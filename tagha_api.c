
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "tagha.h"

static uint8_t *GetFunctionOffsetByName(struct Tagha *, const char *);
static uint8_t *GetFunctionOffsetByIndex(struct Tagha *, size_t);
static const char *GetFunctionNameByIndex(struct Tagha *, size_t);

static uint8_t *GetVariableOffsetByName(struct Tagha *, const char *);
static uint8_t *GetVariableOffsetByIndex(struct Tagha *, size_t);
static const char *GetVariableNameByIndex(struct Tagha *, size_t);


struct Tagha *Tagha_New(void *scripthdr)
{
	struct Tagha *__restrict vm = calloc(1, sizeof *vm);
	Tagha_Init(vm, scripthdr);
	return vm;
}

void Tagha_Init(struct Tagha *const __restrict vm, void *scripthdr)
{
	if( !vm )
		return;
	
	*vm = (struct Tagha){0};
	// set up script header pointer.
	vm->ScriptHdr.Ptr = scripthdr;
	// set up func table pointer.
	
	// jump past header data to the first function table's entry.
	union Value reader = vm->ScriptHdr;
	reader.UCharPtr += 7;
	
	// points to the function table size.
	vm->FuncTable = reader;
	
	// set up global var table pointer.
	// first skip past the func table.
	const size_t funcs = *reader.UInt32Ptr++;
	for( size_t i=0 ; i<funcs ; i++ ) {
		reader.UCharPtr++; // skip the native bool byte.
		const uint64_t sizes = *reader.UInt64Ptr++;
		const size_t stringlen = sizes & 0xffFFffFF;
		const size_t instrlen = sizes >> 32;
		reader.UCharPtr += (stringlen + instrlen);
	}
	vm->GVarTable = reader; // points to the global var table size.
	
	FILE **stdinref = (FILE **)GetVariableOffsetByName(vm, "stdin");
	if( stdinref )
		*stdinref = stdin;
	
	FILE **stderrref = (FILE **)GetVariableOffsetByName(vm, "stderr");
	if( stderrref )
		*stderrref = stderr;
		
	FILE **stdoutref = (FILE **)GetVariableOffsetByName(vm, "stderr");
	if( stdoutref )
		*stdoutref = stdout;
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

bool Tagha_RegisterNatives(struct Tagha *const __restrict vm, struct NativeInfo natives[])
{
	if( !vm or !natives )
		return false;
	
	for( struct NativeInfo *n=natives ; n->NativeCFunc and n->Name ; n++ )
		Map_Insert(&vm->Natives, n->Name, (union Value){.VoidFunc = n->NativeCFunc});
	return true;
}

void Tagha_PushValues(struct Tagha *const __restrict vm, const size_t args, union Value values[static args])
{
	if( !vm or !args )
		return;
	
	// remember that arguments must be passed right to left.
	// we have enough args to fit in registers.
	const size_t reg_params = 8;
	const enum RegID reg_param_initial = regPeh;
	const size_t bytecount = sizeof(union Value) * args;
	
	// save stack space by using the registers for passing arguments.
	// the other registers can then be used for other data operations.
	if( args <= reg_params ) {
		memcpy(vm->Regs+reg_param_initial, values, bytecount);
	}
	// if the native has more than a certain num of params, get from both registers and stack.
	else if( args > reg_params ) {
		memcpy(vm->Regs+reg_param_initial, values, sizeof(union Value) * reg_params);
		memcpy(vm->Regs[regStk].SelfPtr, values+reg_params, sizeof(union Value) * (args-reg_params));
		vm->Regs[regStk].SelfPtr -= (args-reg_params);
	}
}

static uint8_t *GetFunctionOffsetByName(struct Tagha *const __restrict vm, const char *__restrict funcname)
{
	if( !funcname or !vm or !vm->FuncTable.UCharPtr )
		return NULL;
	
	union Value reader = vm->FuncTable;
	const size_t funcs = *reader.UInt32Ptr++;
	reader.UCharPtr++;
	for( size_t i=0 ; i<funcs ; i++ ) {
		const uint64_t sizes = *reader.UInt64Ptr++;
		const size_t stringlen = sizes & 0xffFFffFF;
		const size_t instrlen = sizes >> 32;
		if( !strcmp(funcname, reader.Ptr) )
			return reader.UCharPtr + stringlen;
		
		// skip to the next 
		reader.UCharPtr += (stringlen + instrlen + 1);
	}
	return NULL;
}

static uint8_t *GetFunctionOffsetByIndex(struct Tagha *const __restrict vm, const size_t index)
{
	if( !vm or !vm->FuncTable.UCharPtr )
		return NULL;
	
	union Value reader = vm->FuncTable;
	const size_t funcs = *reader.UInt32Ptr++;
	if( index >= funcs )
		return NULL;
	
	reader.UCharPtr++;
	for( size_t i=0 ; i<funcs ; i++ ) {
		const uint64_t sizes = *reader.UInt64Ptr++;
		const size_t stringlen = sizes & 0xffFFffFF;
		const size_t instrlen = sizes >> 32;
		if( i==index )
			return reader.UCharPtr + stringlen;
		
		// skip to the next 
		reader.UCharPtr += (stringlen + instrlen + 1);
	}
	return NULL;
}

static const char *GetFunctionNameByIndex(struct Tagha *const __restrict vm, const size_t index)
{
	if( !vm or !vm->FuncTable.UCharPtr )
		return NULL;
	
	union Value reader = vm->FuncTable;
	const size_t funcs = *reader.UInt32Ptr++;
	if( index >= funcs )
		return NULL;
	
	reader.UCharPtr++;
	for( size_t i=0 ; i<funcs ; i++ ) {
		const uint64_t sizes = *reader.UInt64Ptr++;
		const size_t stringlen = sizes & 0xffFFffFF;
		const size_t instrlen = sizes >> 32;
		if( i==index )
			return reader.Ptr;
		
		// skip to the next 
		reader.UCharPtr += (stringlen + instrlen + 1);
	}
	return NULL;
}

static uint8_t *GetVariableOffsetByName(struct Tagha *const __restrict vm, const char *varname)
{
	if( !vm or !vm->GVarTable.UCharPtr or !varname )
		return NULL;
	
	union Value reader = vm->GVarTable;
	const size_t globalvars = *reader.UInt32Ptr++;
	reader.UCharPtr++;
	for( size_t i=0 ; i<globalvars ; i++ ) {
		const uint64_t sizes = *reader.UInt64Ptr++;
		const size_t stringlen = sizes & 0xffFFffFF;
		const size_t bytelen = sizes >> 32;
		if( !strcmp(varname, reader.Ptr) )
			return reader.UCharPtr + stringlen;
		
		// skip to the next var
		reader.UCharPtr += (stringlen + bytelen + 1);
	}
	return NULL;
}

static uint8_t *GetVariableOffsetByIndex(struct Tagha *const __restrict vm, const size_t index)
{
	if( !vm or !vm->GVarTable.UCharPtr )
		return NULL;
	
	union Value reader = vm->GVarTable;
	const size_t globalvars = *reader.UInt32Ptr++;
	if( index >= globalvars )
		return NULL;
	
	reader.UCharPtr++;
	for( size_t i=0 ; i<globalvars ; i++ ) {
		const uint64_t sizes = *reader.UInt64Ptr++;
		const size_t stringlen = sizes & 0xffFFffFF;
		const size_t bytelen = sizes >> 32;
		if( i==index )
			return reader.UCharPtr + stringlen;
		
		// skip to the next global var index
		reader.UCharPtr += (stringlen + bytelen + 1);
	}
	return NULL;
}

static const char *GetVariableNameByIndex(struct Tagha *const __restrict vm, const size_t index)
{
	if( !vm or !vm->GVarTable.UCharPtr )
		return NULL;
	
	union Value reader = vm->GVarTable;
	const size_t globalvars = *reader.UInt32Ptr++;
	if( index >= globalvars )
		return NULL;
	
	reader.UCharPtr++;
	for( size_t i=0 ; i<globalvars ; i++ ) {
		const uint64_t sizes = *reader.UInt64Ptr++;
		const size_t stringlen = sizes & 0xffFFffFF;
		const size_t bytelen = sizes >> 32;
		if( i==index )
			return reader.Ptr;
		
		// skip to the next 
		reader.UCharPtr += (stringlen + bytelen + 1);
	}
	return NULL;
}

int32_t Tagha_Exec(struct Tagha *const __restrict vm)
{
	if( !vm ) {
		puts("Tagha_Exec :: vm ptr is NULL.");
		return -1;
	}
	const union Value *const MainBasePtr = vm->Regs[regBase].SelfPtr;
	vm->Regs[regBase] = vm->Regs[regStk];
	
	uint8_t instr=0, addrmode=0;
	uint16_t opcode = 0;
	
#define X(x) #x ,
	// for debugging purposes.
	const char *const __restrict opcode2str[] = { INSTR_SET };
#undef X
	
#define X(x) &&exec_##x ,
	// our instruction dispatch table.
	const void *const __restrict dispatch[] = { INSTR_SET };
#undef X
#undef INSTR_SET
	
	
	#define DISPATCH() \
	opcode = *vm->Regs[regInstr].UShortPtr++; \
	\
	/* get the instruction from the first byte. */ \
	instr = opcode & 255; \
	\
	/* get addressing mode from second byte. */ \
	addrmode = opcode >> 8; \
	\
	if( instr>nop ) { \
		puts("Tagha_Exec :: instr out of bounds."); \
		return instr; \
	} \
	\
	/*printf("dispatching to '%s'\n", opcode2str[instr]);*/ \
	goto *dispatch[instr]
	
	DISPATCH();
	
	exec_halt:;
		return vm->Regs[regAlaf].Int32;
		
	exec_nop:;
		DISPATCH();
	
	// pushes a value to the top of the stack, raises the stack pointer by 8 bytes.
	// push reg (1 byte for register id)
	// push imm (8 bytes for constant values)
	// push [reg+offset] (1 byte reg id + 4-byte signed offset)
	exec_push:; {
		// push an imm constant.
		if( addrmode & Immediate )
			*--vm->Regs[regStk].SelfPtr = *vm->Regs[regInstr].SelfPtr++;
		// push a register's contents.
		else if( addrmode & Register )
			*--vm->Regs[regStk].SelfPtr = vm->Regs[*vm->Regs[regInstr].UCharPtr++];
		// push the contents of a memory address inside a register.
		else if( addrmode & RegIndirect ) {
			const uint8_t regid = *vm->Regs[regInstr].UCharPtr++;
			const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
			*--vm->Regs[regStk].SelfPtr = *effective_address.SelfPtr;
		}
		DISPATCH();
	}
	
	// pops a value from the stack into a register or memory then reduces stack by 8 bytes.
	// pop reg
	// pop [reg+offset]
	exec_pop:; {
		if( addrmode & Register )
			vm->Regs[*vm->Regs[regInstr].UCharPtr++] = *vm->Regs[regStk].SelfPtr++;
		else if( addrmode & RegIndirect ) {
			const uint8_t regid = *vm->Regs[regInstr].UCharPtr++;
			const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
			*effective_address.SelfPtr = *vm->Regs[regStk].SelfPtr++;
		}
		DISPATCH();
	}
	
	// loads a value that will be used as a memory address to a register.
	// lea reg, [reg+offset] (not dereferenced)
	exec_lea:; {
		// first addressing mode determines the destination.
		const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
		const uint8_t sec_addrmode = op_args & 255;
		const uint8_t regid = op_args >> 8;
		const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
		
		vm->Regs[regid].UInt64 = vm->Regs[sec_regid].UInt64 + *vm->Regs[regInstr].Int32Ptr++;
		DISPATCH();
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
				vm->Regs[regid] = vm->Regs[*vm->Regs[regInstr].UCharPtr++];
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
		DISPATCH();
	}
	
	// copies a global var address by index to a register or memory address.
	// movgbl reg, [reg+offset]
	// movgbl reg, imm
	// movgbl reg, reg
	// movgbl [reg+offset], imm
	// movgbl [reg+offset], reg
	exec_movgbl:; {
		// first addressing mode determines the destination.
		const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
		
		// get secondary addressing mode for the second operand.
		const uint8_t sec_addrmode = op_args & 255;
		
		// get 1st register id.
		const uint8_t regid = op_args >> 8;
		if( addrmode & Register ) {
			// movgbl reg, imm
			if( sec_addrmode & Immediate ) {
				uint64_t i = *vm->Regs[regInstr].UInt64Ptr++;
				vm->Regs[regid].UCharPtr = GetVariableOffsetByIndex(vm, i);
			}
			// movgbl reg, reg
			else if( sec_addrmode & Register )
				vm->Regs[regid].UCharPtr = GetVariableOffsetByIndex(vm, vm->Regs[*vm->Regs[regInstr].UCharPtr++].UInt64);
			// movgbl reg, [reg+offset]
			else if( sec_addrmode & RegIndirect ) {
				const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[sec_regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				if( sec_addrmode & Byte )
					vm->Regs[regid].UCharPtr = GetVariableOffsetByIndex(vm, *effective_address.UCharPtr);
				else if( sec_addrmode & TwoBytes )
					vm->Regs[regid].UCharPtr = GetVariableOffsetByIndex(vm, *effective_address.UShortPtr);
				else if( sec_addrmode & FourBytes )
					vm->Regs[regid].UCharPtr = GetVariableOffsetByIndex(vm, *effective_address.UInt32Ptr);
				else if( sec_addrmode & EightBytes )
					vm->Regs[regid].UCharPtr = GetVariableOffsetByIndex(vm, *effective_address.UInt64Ptr);
			}
		}
		else if( addrmode & RegIndirect ) {
			// movgbl [reg+offset], imm
			if( sec_addrmode & Immediate ) {
				// have to store the imm value prior because the offset is stored AFTER the last operand.
				const union Value imm = (union Value){.UInt64 = *vm->Regs[regInstr].UInt64Ptr++};
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				uint8_t *gvar_offset = GetVariableOffsetByIndex(vm, imm.UInt64);
				if( !gvar_offset )
					DISPATCH();
				if( addrmode & EightBytes )
					effective_address.SelfPtr->UCharPtr = gvar_offset;
			}
			// movgbl [reg+offset], reg
			else if( sec_addrmode & Register ) {
				const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				uint8_t *gvar_offset = GetVariableOffsetByIndex(vm, vm->Regs[sec_regid].UInt64);
				if( !gvar_offset )
					DISPATCH();
				if( addrmode & EightBytes )
					effective_address.SelfPtr->UCharPtr = gvar_offset;
			}
		}
		DISPATCH();
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
			if( sec_addrmode & Immediate ) {
				/* if( sec_addrmode & FloatingPoint ) {
					if( sec_addrmode & FourBytes )
						vm->Regs[regid].Float += *vm->Regs[regInstr].FloatPtr++;
					else if( sec_addrmode & EightBytes )
						vm->Regs[regid].Double += *vm->Regs[regInstr].DoublePtr++;
				}
				else */
				vm->Regs[regid].UInt64 += *vm->Regs[regInstr].UInt64Ptr++;
			}
			// add reg, reg
			else if( sec_addrmode & Register ) {
				const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
				vm->Regs[regid].UInt64 += vm->Regs[sec_regid].UInt64;
			}
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
		DISPATCH();
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
		DISPATCH();
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
		DISPATCH();
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
		DISPATCH();
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
		DISPATCH();
	}
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
		DISPATCH();
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
		DISPATCH();
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
		DISPATCH();
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
		DISPATCH();
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
		DISPATCH();
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
		DISPATCH();
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
		DISPATCH();
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
		DISPATCH();
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
		DISPATCH();
	}
	exec_lt:; {
		// first addressing mode determines the destination.
		const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
		const uint8_t sec_addrmode = op_args & 255; // get secondary addressing mode.
		const uint8_t regid = op_args >> 8; // get 1st register id.
		if( addrmode & Register ) {
			// lt reg, imm
			if( sec_addrmode & Immediate )
				vm->CondFlag = vm->Regs[regid].UInt64 < *vm->Regs[regInstr].UInt64Ptr++;
			// lt reg, reg
			else if( sec_addrmode & Register )
				vm->CondFlag = vm->Regs[regid].UInt64 < vm->Regs[*vm->Regs[regInstr].UCharPtr++].UInt64;
			// lt reg, [reg+offset]
			else if( sec_addrmode & RegIndirect ) {
				const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[sec_regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				if( sec_addrmode & Byte )
					vm->CondFlag = vm->Regs[regid].UChar < *effective_address.UCharPtr;
				else if( sec_addrmode & TwoBytes )
					vm->CondFlag = vm->Regs[regid].UShort < *effective_address.UShortPtr;
				else if( sec_addrmode & FourBytes )
					vm->CondFlag = vm->Regs[regid].UInt32 < *effective_address.UInt32Ptr;
				else if( sec_addrmode & EightBytes )
					vm->CondFlag = vm->Regs[regid].UInt64 < *effective_address.UInt64Ptr;
			}
		}
		else if( addrmode & RegIndirect ) {
			// lt [reg+offset], imm
			if( sec_addrmode & Immediate ) {
				// have to store the imm value prior because the offset is stored AFTER the last operand.
				const union Value imm = (union Value){.UInt64 = *vm->Regs[regInstr].UInt64Ptr++};
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					vm->CondFlag = *effective_address.UCharPtr < imm.UChar;
				else if( addrmode & TwoBytes )
					vm->CondFlag = *effective_address.UShortPtr < imm.UShort;
				else if( addrmode & FourBytes )
					vm->CondFlag = *effective_address.UInt32Ptr < imm.UInt32;
				else if( addrmode & EightBytes )
					vm->CondFlag = *effective_address.UInt64Ptr < imm.UInt64;
			}
			// lt [reg+offset], reg
			else if( sec_addrmode & Register ) {
				const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					vm->CondFlag = *effective_address.UCharPtr < vm->Regs[sec_regid].UChar;
				else if( addrmode & TwoBytes )
					vm->CondFlag = *effective_address.UShortPtr < vm->Regs[sec_regid].UShort;
				else if( addrmode & FourBytes )
					vm->CondFlag = *effective_address.UInt32Ptr < vm->Regs[sec_regid].UInt32;
				else if( addrmode & EightBytes )
					vm->CondFlag = *effective_address.UInt64Ptr < vm->Regs[sec_regid].UInt64;
			}
		}
		DISPATCH();
	}
	exec_gt:; {
		// first addressing mode determines the destination.
		const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
		const uint8_t sec_addrmode = op_args & 255; // get secondary addressing mode.
		const uint8_t regid = op_args >> 8; // get 1st register id.
		if( addrmode & Register ) {
			// gt reg, imm
			if( sec_addrmode & Immediate )
				vm->CondFlag = vm->Regs[regid].UInt64 > *vm->Regs[regInstr].UInt64Ptr++;
			// gt reg, reg
			else if( sec_addrmode & Register )
				vm->CondFlag = vm->Regs[regid].UInt64 > vm->Regs[*vm->Regs[regInstr].UCharPtr++].UInt64;
			// gt reg, [reg+offset]
			else if( sec_addrmode & RegIndirect ) {
				const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[sec_regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				if( sec_addrmode & Byte )
					vm->CondFlag = vm->Regs[regid].UChar > *effective_address.UCharPtr;
				else if( sec_addrmode & TwoBytes )
					vm->CondFlag = vm->Regs[regid].UShort > *effective_address.UShortPtr;
				else if( sec_addrmode & FourBytes )
					vm->CondFlag = vm->Regs[regid].UInt32 > *effective_address.UInt32Ptr;
				else if( sec_addrmode & EightBytes )
					vm->CondFlag = vm->Regs[regid].UInt64 > *effective_address.UInt64Ptr;
			}
		}
		else if( addrmode & RegIndirect ) {
			// gt [reg+offset], imm
			if( sec_addrmode & Immediate ) {
				// have to store the imm value prior because the offset is stored AFTER the last operand.
				const union Value imm = (union Value){.UInt64 = *vm->Regs[regInstr].UInt64Ptr++};
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					vm->CondFlag = *effective_address.UCharPtr > imm.UChar;
				else if( addrmode & TwoBytes )
					vm->CondFlag = *effective_address.UShortPtr > imm.UShort;
				else if( addrmode & FourBytes )
					vm->CondFlag = *effective_address.UInt32Ptr > imm.UInt32;
				else if( addrmode & EightBytes )
					vm->CondFlag = *effective_address.UInt64Ptr > imm.UInt64;
			}
			// gt [reg+offset], reg
			else if( sec_addrmode & Register ) {
				const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					vm->CondFlag = *effective_address.UCharPtr > vm->Regs[sec_regid].UChar;
				else if( addrmode & TwoBytes )
					vm->CondFlag = *effective_address.UShortPtr > vm->Regs[sec_regid].UShort;
				else if( addrmode & FourBytes )
					vm->CondFlag = *effective_address.UInt32Ptr > vm->Regs[sec_regid].UInt32;
				else if( addrmode & EightBytes )
					vm->CondFlag = *effective_address.UInt64Ptr > vm->Regs[sec_regid].UInt64;
			}
		}
		DISPATCH();
	}
	exec_cmp:; {
		// first addressing mode determines the destination.
		const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
		const uint8_t sec_addrmode = op_args & 255; // get secondary addressing mode.
		const uint8_t regid = op_args >> 8; // get 1st register id.
		if( addrmode & Register ) {
			// cmp reg, imm
			if( sec_addrmode & Immediate )
				vm->CondFlag = vm->Regs[regid].UInt64 == *vm->Regs[regInstr].UInt64Ptr++;
			// cmp reg, reg
			else if( sec_addrmode & Register )
				vm->CondFlag = vm->Regs[regid].UInt64 == vm->Regs[*vm->Regs[regInstr].UCharPtr++].UInt64;
			// cmp reg, [reg+offset]
			else if( sec_addrmode & RegIndirect ) {
				const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[sec_regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				if( sec_addrmode & Byte )
					vm->CondFlag = vm->Regs[regid].UChar == *effective_address.UCharPtr;
				else if( sec_addrmode & TwoBytes )
					vm->CondFlag = vm->Regs[regid].UShort == *effective_address.UShortPtr;
				else if( sec_addrmode & FourBytes )
					vm->CondFlag = vm->Regs[regid].UInt32 == *effective_address.UInt32Ptr;
				else if( sec_addrmode & EightBytes )
					vm->CondFlag = vm->Regs[regid].UInt64 == *effective_address.UInt64Ptr;
			}
		}
		else if( addrmode & RegIndirect ) {
			// cmp [reg+offset], imm
			if( sec_addrmode & Immediate ) {
				// have to store the imm value prior because the offset is stored AFTER the last operand.
				const union Value imm = (union Value){.UInt64 = *vm->Regs[regInstr].UInt64Ptr++};
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					vm->CondFlag = *effective_address.UCharPtr == imm.UChar;
				else if( addrmode & TwoBytes )
					vm->CondFlag = *effective_address.UShortPtr == imm.UShort;
				else if( addrmode & FourBytes )
					vm->CondFlag = *effective_address.UInt32Ptr == imm.UInt32;
				else if( addrmode & EightBytes )
					vm->CondFlag = *effective_address.UInt64Ptr == imm.UInt64;
			}
			// cmp [reg+offset], reg
			else if( sec_addrmode & Register ) {
				const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					vm->CondFlag = *effective_address.UCharPtr == vm->Regs[sec_regid].UChar;
				else if( addrmode & TwoBytes )
					vm->CondFlag = *effective_address.UShortPtr == vm->Regs[sec_regid].UShort;
				else if( addrmode & FourBytes )
					vm->CondFlag = *effective_address.UInt32Ptr == vm->Regs[sec_regid].UInt32;
				else if( addrmode & EightBytes )
					vm->CondFlag = *effective_address.UInt64Ptr == vm->Regs[sec_regid].UInt64;
			}
		}
		DISPATCH();
	}
	exec_neq:; {
		// first addressing mode determines the destination.
		const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
		const uint8_t sec_addrmode = op_args & 255; // get secondary addressing mode.
		const uint8_t regid = op_args >> 8; // get 1st register id.
		if( addrmode & Register ) {
			// neq reg, imm
			if( sec_addrmode & Immediate )
				vm->CondFlag = vm->Regs[regid].UInt64 != *vm->Regs[regInstr].UInt64Ptr++;
			// neq reg, reg
			else if( sec_addrmode & Register )
				vm->CondFlag = vm->Regs[regid].UInt64 != vm->Regs[*vm->Regs[regInstr].UCharPtr++].UInt64;
			// neq reg, [reg+offset]
			else if( sec_addrmode & RegIndirect ) {
				const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[sec_regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				if( sec_addrmode & Byte )
					vm->CondFlag = vm->Regs[regid].UChar != *effective_address.UCharPtr;
				else if( sec_addrmode & TwoBytes )
					vm->CondFlag = vm->Regs[regid].UShort != *effective_address.UShortPtr;
				else if( sec_addrmode & FourBytes )
					vm->CondFlag = vm->Regs[regid].UInt32 != *effective_address.UInt32Ptr;
				else if( sec_addrmode & EightBytes )
					vm->CondFlag = vm->Regs[regid].UInt64 != *effective_address.UInt64Ptr;
			}
		}
		else if( addrmode & RegIndirect ) {
			// neq [reg+offset], imm
			if( sec_addrmode & Immediate ) {
				// have to store the imm value prior because the offset is stored AFTER the last operand.
				const union Value imm = (union Value){.UInt64 = *vm->Regs[regInstr].UInt64Ptr++};
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					vm->CondFlag = *effective_address.UCharPtr != imm.UChar;
				else if( addrmode & TwoBytes )
					vm->CondFlag = *effective_address.UShortPtr != imm.UShort;
				else if( addrmode & FourBytes )
					vm->CondFlag = *effective_address.UInt32Ptr != imm.UInt32;
				else if( addrmode & EightBytes )
					vm->CondFlag = *effective_address.UInt64Ptr != imm.UInt64;
			}
			// neq [reg+offset], reg
			else if( addrmode & Register ) {
				const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					vm->CondFlag = *effective_address.UCharPtr != vm->Regs[sec_regid].UChar;
				else if( addrmode & TwoBytes )
					vm->CondFlag = *effective_address.UShortPtr != vm->Regs[sec_regid].UShort;
				else if( addrmode & FourBytes )
					vm->CondFlag = *effective_address.UInt32Ptr != vm->Regs[sec_regid].UInt32;
				else if( addrmode & EightBytes )
					vm->CondFlag = *effective_address.UInt64Ptr != vm->Regs[sec_regid].UInt64;
			}
		}
		DISPATCH();
	}
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
		DISPATCH();
	}
	exec_jz:; {
		// jz imm
		if( addrmode & Immediate ) {
			const int64_t offset = *vm->Regs[regInstr].Int64Ptr++;
			!vm->CondFlag ? (vm->Regs[regInstr].UCharPtr += offset) : (void)vm->CondFlag;
		}
		// jz reg
		else if( addrmode & Register ) {
			const uint8_t regid = *vm->Regs[regInstr].UCharPtr++;
			!vm->CondFlag ? (vm->Regs[regInstr].UCharPtr += vm->Regs[regid].Int64) : (void)vm->CondFlag;
		}
		// jz [reg+offset]
		else if( addrmode & RegIndirect ) {
			const uint8_t regid = *vm->Regs[regInstr].UCharPtr++;
			const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
			if( addrmode & Byte )
				!vm->CondFlag ? (vm->Regs[regInstr].UCharPtr += *effective_address.CharPtr) : (void)vm->CondFlag;
			else if( addrmode & TwoBytes )
				!vm->CondFlag ? (vm->Regs[regInstr].UCharPtr += *effective_address.ShortPtr) : (void)vm->CondFlag;
			else if( addrmode & FourBytes )
				!vm->CondFlag ? (vm->Regs[regInstr].UCharPtr += *effective_address.Int32Ptr) : (void)vm->CondFlag;
			else if( addrmode & EightBytes )
				!vm->CondFlag ? (vm->Regs[regInstr].UCharPtr += *effective_address.Int64Ptr) : (void)vm->CondFlag;
		}
		DISPATCH();
	}
	exec_jnz:; {
		// jnz imm
		if( addrmode & Immediate ) {
			const int64_t offset = *vm->Regs[regInstr].Int64Ptr++;
			vm->CondFlag ? (vm->Regs[regInstr].UCharPtr += offset) : (void)vm->CondFlag;
		}
		// jnz reg
		else if( addrmode & Register ) {
			const uint8_t regid = *vm->Regs[regInstr].UCharPtr++;
			vm->CondFlag ? (vm->Regs[regInstr].UCharPtr += vm->Regs[regid].Int64) : (void)vm->CondFlag;
		}
		// jnz [reg+offset]
		else if( addrmode & RegIndirect ) {
			const uint8_t regid = *vm->Regs[regInstr].UCharPtr++;
			const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
			if( addrmode & Byte )
				vm->CondFlag ? (vm->Regs[regInstr].UCharPtr += *effective_address.CharPtr) : (void)vm->CondFlag;
			else if( addrmode & TwoBytes )
				vm->CondFlag ? (vm->Regs[regInstr].UCharPtr += *effective_address.ShortPtr) : (void)vm->CondFlag;
			else if( addrmode & FourBytes )
				vm->CondFlag ? (vm->Regs[regInstr].UCharPtr += *effective_address.Int32Ptr) : (void)vm->CondFlag;
			else if( addrmode & EightBytes )
				vm->CondFlag ? (vm->Regs[regInstr].UCharPtr += *effective_address.Int64Ptr) : (void)vm->CondFlag;
		}
		DISPATCH();
	}
	exec_call:; {
		uint8_t *offset = NULL;
		// call imm
		if( addrmode & Immediate )
			offset = GetFunctionOffsetByIndex(vm, *vm->Regs[regInstr].UInt64Ptr++);
		// call reg
		else if( addrmode & Register )
			offset = GetFunctionOffsetByIndex(vm, vm->Regs[*vm->Regs[regInstr].UCharPtr++].UInt64);
		// call [reg+offset]
		else if( addrmode & RegIndirect ) {
			const uint8_t regid = *vm->Regs[regInstr].UCharPtr++;
			const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
			if( addrmode & Byte )
				offset = GetFunctionOffsetByIndex(vm, *effective_address.UCharPtr);
			else if( addrmode & TwoBytes )
				offset = GetFunctionOffsetByIndex(vm, *effective_address.UShortPtr);
			else if( addrmode & FourBytes )
				offset = GetFunctionOffsetByIndex(vm, *effective_address.UInt32Ptr);
			else if( addrmode & EightBytes )
				offset = GetFunctionOffsetByIndex(vm, *effective_address.UInt64Ptr);
		}
		if( !offset )
			goto *dispatch[halt];
		
		*--vm->Regs[regStk].SelfPtr = vm->Regs[regInstr];	// push rip
		*--vm->Regs[regStk].SelfPtr = vm->Regs[regBase];	// push rbp
		vm->Regs[regBase] = vm->Regs[regStk];	// mov rbp, rsp
		vm->Regs[regInstr].UCharPtr = offset;
		DISPATCH();
	}
	exec_ret:; {
		vm->Regs[regStk] = vm->Regs[regBase]; // mov rsp, rbp
		vm->Regs[regBase] = *vm->Regs[regStk].SelfPtr++; // pop rbp
		
		// if we're popping Main's (or whatever called func's) RBP, then halt the whole program.
		if( vm->Regs[regBase].SelfPtr==MainBasePtr )
			goto *dispatch[halt];
		
		vm->Regs[regInstr] = *vm->Regs[regStk].SelfPtr++; // pop rip
		DISPATCH();
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
			index = vm->Regs[*vm->Regs[regInstr].UCharPtr++].UInt32;
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
		void (*NativeFunc)() = Map_Get(&vm->Natives, GetFunctionNameByIndex(vm, index)).VoidFunc;
		//printf("calling native func: '%s', %p\n", GetFunctionNameByIndex(vm, index), Map_Get(&vm->Natives, GetFunctionNameByIndex(vm, index)).VoidFunc);
		if( !NativeFunc ) {
			puts("Tagha_Exec :: exec_syscall reported 'NativeFunc' is NULL");
			DISPATCH();
		}
		
		union Value params[argcount];
		const size_t bytecount = sizeof(union Value) * argcount;
		memset(params, 0, bytecount);
		
		const size_t reg_params = 8;
		const enum RegID reg_param_initial = regPeh;
		
		// save stack space by using the registers for passing arguments.
		// the other registers can then be used for other data operations.
		if( argcount <= reg_params ) {
			memcpy(params, vm->Regs+reg_param_initial, bytecount);
		}
		// if the native has more than a certain num of params, get from both registers and stack.
		else if( argcount > reg_params ) {
			memcpy(params, vm->Regs+reg_param_initial, sizeof(union Value) * reg_params);
			memcpy(params+reg_params, vm->Regs[regStk].SelfPtr, sizeof(union Value) * (argcount-reg_params));
			vm->Regs[regStk].SelfPtr += (argcount-reg_params);
		}
		vm->Regs[regAlaf].UInt64 = 0;
		(*NativeFunc)(vm, vm->Regs+regAlaf, argcount, params);
		DISPATCH();
	}
#if FLOATING_POINT_OPS
	exec_flt2dbl:; {
		// flt2dbl reg
		if( addrmode & Register ) {
			const uint8_t regid = *vm->Regs[regInstr].UCharPtr++;
			const float f = vm->Regs[regid].Float;
			vm->Regs[regid].Double = (double)f;
		}
		DISPATCH();
	}
	exec_dbl2flt:; {
		// flt2dbl reg
		if( addrmode & Register ) {
			const uint8_t regid = *vm->Regs[regInstr].UCharPtr++;
			const double d = vm->Regs[regid].Double;
			vm->Regs[regid].Double = 0;
			vm->Regs[regid].Float = (float)d;
		}
		DISPATCH();
	}
	exec_addf:; {
		// first addressing mode determines the destination.
		const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
		const uint8_t sec_addrmode = op_args & 255; // get secondary addressing mode.
		const uint8_t regid = op_args >> 8; // get 1st register id.
		
		if( addrmode & Register ) {
			// addf reg, imm
			if( sec_addrmode & Immediate ) {
				const union Value convert = (union Value){.UInt64 = *vm->Regs[regInstr].UInt64Ptr++};
				if( sec_addrmode & FourBytes )
					vm->Regs[regid].Float += convert.Float;
				else if( sec_addrmode & EightBytes )
					vm->Regs[regid].Double += convert.Double;
			}
			// addf reg, reg
			else if( sec_addrmode & Register ) {
				const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
				vm->Regs[regid].Float += vm->Regs[sec_regid].Float;
				
				if( sec_addrmode & FourBytes )
					vm->Regs[regid].Float += vm->Regs[sec_regid].Float;
				else if( sec_addrmode & EightBytes )
					vm->Regs[regid].Double += vm->Regs[sec_regid].Double;
			}
			// addf reg, [reg+offset]
			else if( sec_addrmode & RegIndirect ) {
				const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[sec_regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				vm->Regs[regid].Float += *effective_address.FloatPtr;
				
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
				const union Value imm = (union Value){.UInt64 = *vm->Regs[regInstr].UInt64Ptr++};
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
		DISPATCH();
	}
	exec_subf:; {
		// first addressing mode determines the destination.
		const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
		const uint8_t sec_addrmode = op_args & 255; // get secondary addressing mode.
		const uint8_t regid = op_args >> 8; // get 1st register id.
		
		if( addrmode & Register ) {
			// subf reg, imm
			if( sec_addrmode & Immediate ) {
				const union Value convert = (union Value){.UInt64 = *vm->Regs[regInstr].UInt64Ptr++};
				if( sec_addrmode & FourBytes )
					vm->Regs[regid].Float -= convert.Float;
				else if( sec_addrmode & EightBytes )
					vm->Regs[regid].Double -= convert.Double;
			}
			// subf reg, reg
			else if( sec_addrmode & Register ) {
				const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
				if( sec_addrmode & FourBytes )
					vm->Regs[regid].Float -= vm->Regs[sec_regid].Float;
				else if( sec_addrmode & EightBytes )
					vm->Regs[regid].Double -= vm->Regs[sec_regid].Double;
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
				const union Value imm = (union Value){.Double = *vm->Regs[regInstr].DoublePtr++};
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
		DISPATCH();
	}
	exec_mulf:; {
		// first addressing mode determines the destination.
		const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
		const uint8_t sec_addrmode = op_args & 255; // get secondary addressing mode.
		const uint8_t regid = op_args >> 8; // get 1st register id.
		
		if( addrmode & Register ) {
			// mulf reg, imm
			if( sec_addrmode & Immediate ) {
				if( sec_addrmode & FourBytes )
					vm->Regs[regid].Float *= *vm->Regs[regInstr].FloatPtr++;
				else if( sec_addrmode & EightBytes )
					vm->Regs[regid].Double *= *vm->Regs[regInstr].DoublePtr++;
			}
			// mulf reg, reg
			else if( sec_addrmode & Register ) {
				const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
				if( sec_addrmode & FourBytes )
					vm->Regs[regid].Float *= vm->Regs[sec_regid].Float;
				else if( sec_addrmode & EightBytes )
					vm->Regs[regid].Double *= vm->Regs[sec_regid].Double;
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
				const union Value imm = (union Value){.Double = *vm->Regs[regInstr].DoublePtr++};
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
		DISPATCH();
	}
	exec_divf:; {
		// first addressing mode determines the destination.
		const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
		const uint8_t sec_addrmode = op_args & 255; // get secondary addressing mode.
		const uint8_t regid = op_args >> 8; // get 1st register id.
		
		if( addrmode & Register ) {
			// divf reg, imm
			if( sec_addrmode & Immediate ) {
				if( sec_addrmode & FourBytes )
					vm->Regs[regid].Float /= *vm->Regs[regInstr].FloatPtr++;
				else if( sec_addrmode & EightBytes )
					vm->Regs[regid].Double /= *vm->Regs[regInstr].DoublePtr++;
			}
			// divf reg, reg
			else if( sec_addrmode & Register ) {
				const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
				if( sec_addrmode & FourBytes )
					vm->Regs[regid].Float /= vm->Regs[sec_regid].Float;
				else if( sec_addrmode & EightBytes )
					vm->Regs[regid].Double /= vm->Regs[sec_regid].Double;
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
				const union Value imm = (union Value){.Double = *vm->Regs[regInstr].DoublePtr++};
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
		DISPATCH();
	}
	exec_ltf:; {
		// first addressing mode determines the destination.
		const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
		const uint8_t sec_addrmode = op_args & 255; // get secondary addressing mode.
		const uint8_t regid = op_args >> 8; // get 1st register id.
		
		if( addrmode & Register ) {
			// ltf reg, imm
			if( sec_addrmode & Immediate ) {
				if( sec_addrmode & FourBytes )
					vm->CondFlag = vm->Regs[regid].Float < *vm->Regs[regInstr].FloatPtr++;
				else if( sec_addrmode & EightBytes )
					vm->CondFlag = vm->Regs[regid].Double < *vm->Regs[regInstr].DoublePtr++;
			}
			// ltf reg, reg
			else if( sec_addrmode & Register ) {
				const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
				if( sec_addrmode & FourBytes )
					vm->CondFlag = vm->Regs[regid].Float < vm->Regs[sec_regid].Float;
				else if( sec_addrmode & EightBytes )
					vm->CondFlag = vm->Regs[regid].Double < vm->Regs[sec_regid].Double;
			}
			// ltf reg, [reg+offset]
			else if( sec_addrmode & RegIndirect ) {
				const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[sec_regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				if( sec_addrmode & FourBytes )
					vm->CondFlag = vm->Regs[regid].Float < *effective_address.FloatPtr;
				else if( sec_addrmode & EightBytes )
					vm->CondFlag = vm->Regs[regid].Double < *effective_address.DoublePtr;
			}
		}
		else if( addrmode & RegIndirect ) {
			// ltf [reg+offset], imm
			if( sec_addrmode & Immediate ) {
				// have to store the imm value prior because the offset is stored AFTER the last operand.
				const union Value imm = (union Value){.Double = *vm->Regs[regInstr].DoublePtr++};
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				if( addrmode & FourBytes )
					vm->CondFlag = *effective_address.FloatPtr < imm.Float;
				else if( addrmode & EightBytes )
					vm->CondFlag = *effective_address.DoublePtr < imm.Double;
			}
			// ltf [reg+offset], reg
			else if( sec_addrmode & Register ) {
				const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				if( addrmode & FourBytes )
					vm->CondFlag = *effective_address.FloatPtr < vm->Regs[sec_regid].Float;
				else if( addrmode & EightBytes )
					vm->CondFlag = *effective_address.DoublePtr < vm->Regs[sec_regid].Double;
			}
		}
		DISPATCH();
	}
	exec_gtf:; {
		// first addressing mode determines the destination.
		const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
		const uint8_t sec_addrmode = op_args & 255; // get secondary addressing mode.
		const uint8_t regid = op_args >> 8; // get 1st register id.
		
		if( addrmode & Register ) {
			// gtf reg, imm
			if( sec_addrmode & Immediate ) {
				if( sec_addrmode & FourBytes )
					vm->CondFlag = vm->Regs[regid].Float > *vm->Regs[regInstr].FloatPtr++;
				else if( sec_addrmode & EightBytes )
					vm->CondFlag = vm->Regs[regid].Double > *vm->Regs[regInstr].DoublePtr++;
			}
			// gtf reg, reg
			else if( sec_addrmode & Register ) {
				const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
				if( sec_addrmode & FourBytes )
					vm->CondFlag = vm->Regs[regid].Float > vm->Regs[sec_regid].Float;
				else if( sec_addrmode & EightBytes )
					vm->CondFlag = vm->Regs[regid].Double > vm->Regs[sec_regid].Double;
			}
			// gtf reg, [reg+offset]
			else if( sec_addrmode & RegIndirect ) {
				const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[sec_regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				if( sec_addrmode & FourBytes )
					vm->CondFlag = vm->Regs[regid].Float > *effective_address.FloatPtr;
				else if( sec_addrmode & EightBytes )
					vm->CondFlag = vm->Regs[regid].Double > *effective_address.DoublePtr;
			}
		}
		else if( addrmode & RegIndirect ) {
			// gtf [reg+offset], imm
			if( sec_addrmode & Immediate ) {
				// have to store the imm value prior because the offset is stored AFTER the last operand.
				const union Value imm = (union Value){.Double = *vm->Regs[regInstr].DoublePtr++};
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				if( addrmode & FourBytes )
					vm->CondFlag = *effective_address.FloatPtr > imm.Float;
				else if( addrmode & EightBytes )
					vm->CondFlag = *effective_address.DoublePtr > imm.Double;
			}
			// gtf [reg+offset], reg
			else if( sec_addrmode & Register ) {
				const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				if( addrmode & FourBytes )
					vm->CondFlag = *effective_address.FloatPtr > vm->Regs[sec_regid].Float;
				else if( addrmode & EightBytes )
					vm->CondFlag = *effective_address.DoublePtr > vm->Regs[sec_regid].Double;
			}
		}
		DISPATCH();
	}
	exec_cmpf:; {
		// first addressing mode determines the destination.
		const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
		const uint8_t sec_addrmode = op_args & 255; // get secondary addressing mode.
		const uint8_t regid = op_args >> 8; // get 1st register id.
		
		if( addrmode & Register ) {
			// cmpf reg, imm
			if( sec_addrmode & Immediate ) {
				if( sec_addrmode & FourBytes )
					vm->CondFlag = vm->Regs[regid].Float == *vm->Regs[regInstr].FloatPtr++;
				else if( sec_addrmode & EightBytes )
					vm->CondFlag = vm->Regs[regid].Double == *vm->Regs[regInstr].DoublePtr++;
			}
			// cmpf reg, reg
			else if( sec_addrmode & Register ) {
				const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
				if( sec_addrmode & FourBytes )
					vm->CondFlag = vm->Regs[regid].Float == vm->Regs[sec_regid].Float;
				else if( sec_addrmode & EightBytes )
					vm->CondFlag = vm->Regs[regid].Double == vm->Regs[sec_regid].Double;
			}
			// cmpf reg, [reg+offset]
			else if( sec_addrmode & RegIndirect ) {
				const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[sec_regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				if( sec_addrmode & FourBytes )
					vm->CondFlag = vm->Regs[regid].Float == *effective_address.FloatPtr;
				else if( sec_addrmode & EightBytes )
					vm->CondFlag = vm->Regs[regid].Double == *effective_address.DoublePtr;
			}
		}
		else if( addrmode & RegIndirect ) {
			// cmpf [reg+offset], imm
			if( sec_addrmode & Immediate ) {
				// have to store the imm value prior because the offset is stored AFTER the last operand.
				const union Value imm = (union Value){.Double = *vm->Regs[regInstr].DoublePtr++};
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				if( addrmode & FourBytes )
					vm->CondFlag = *effective_address.FloatPtr == imm.Float;
				else if( addrmode & EightBytes )
					vm->CondFlag = *effective_address.DoublePtr == imm.Double;
			}
			// cmpf [reg+offset], reg
			else if( sec_addrmode & Register ) {
				const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				if( addrmode & FourBytes )
					vm->CondFlag = *effective_address.FloatPtr == vm->Regs[sec_regid].Float;
				else if( addrmode & EightBytes )
					vm->CondFlag = *effective_address.DoublePtr == vm->Regs[sec_regid].Double;
			}
		}
		DISPATCH();
	}
	exec_neqf:; {
		// first addressing mode determines the destination.
		const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
		const uint8_t sec_addrmode = op_args & 255; // get secondary addressing mode.
		const uint8_t regid = op_args >> 8; // get 1st register id.
		
		if( addrmode & Register ) {
			// neqf reg, imm
			if( sec_addrmode & Immediate ) {
				if( sec_addrmode & FourBytes )
					vm->CondFlag = vm->Regs[regid].Float != *vm->Regs[regInstr].FloatPtr++;
				else if( sec_addrmode & EightBytes )
					vm->CondFlag = vm->Regs[regid].Double != *vm->Regs[regInstr].DoublePtr++;
			}
			// neqf reg, reg
			else if( sec_addrmode & Register ) {
				const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
				if( sec_addrmode & FourBytes )
					vm->CondFlag = vm->Regs[regid].Float != vm->Regs[sec_regid].Float;
				else if( sec_addrmode & EightBytes )
					vm->CondFlag = vm->Regs[regid].Double != vm->Regs[sec_regid].Double;
			}
			// neqf reg, [reg+offset]
			else if( sec_addrmode & RegIndirect ) {
				const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[sec_regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				if( sec_addrmode & FourBytes )
					vm->CondFlag = vm->Regs[regid].Float != *effective_address.FloatPtr;
				else if( sec_addrmode & EightBytes )
					vm->CondFlag = vm->Regs[regid].Double != *effective_address.DoublePtr;
			}
		}
		else if( addrmode & RegIndirect ) {
			// neqf [reg+offset], imm
			if( sec_addrmode & Immediate ) {
				// have to store the imm value prior because the offset is stored AFTER the last operand.
				const union Value imm = (union Value){.Double = *vm->Regs[regInstr].DoublePtr++};
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				if( addrmode & FourBytes )
					vm->CondFlag = *effective_address.FloatPtr != imm.Float;
				else if( addrmode & EightBytes )
					vm->CondFlag = *effective_address.DoublePtr != imm.Double;
			}
			// neqf [reg+offset], reg
			else if( sec_addrmode & Register ) {
				const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				if( addrmode & FourBytes )
					vm->CondFlag = *effective_address.FloatPtr != vm->Regs[sec_regid].Float;
				else if( addrmode & EightBytes )
					vm->CondFlag = *effective_address.DoublePtr != vm->Regs[sec_regid].Double;
			}
		}
		DISPATCH();
	}
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
		DISPATCH();
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
		DISPATCH();
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
		DISPATCH();
	}
#endif
	return -1;
}

int32_t Tagha_RunScript(struct Tagha *const __restrict vm, const int32_t argc, char *argv[])
{
	if( !vm )
		return -1;
	
	if( *vm->ScriptHdr.UShortPtr != 0xC0DE ) {
		puts("Tagha_RunScript :: ERROR: Script has invalid main verifier.");
		return -1;
	}
	
	uint8_t *main_offset = GetFunctionOffsetByName(vm, "main");
	if( !main_offset ) {
		puts("Tagha_RunScript :: ERROR: script contains no 'main' function.");
		return -1;
	}
	
	// push argc, argv, and possibly envp, to registers.
	union Value MainArgs[argc+1];
	for( int i=0 ; i<=argc ; i++ )
		MainArgs[i].Ptr = argv[i];
	
	vm->Regs[regFeh].Ptr = MainArgs;
	vm->Regs[regPeh].Int32 = argc;
	
	// check out stack size and align it by the size of union Value.
	size_t stacksize = *(uint32_t *)(vm->ScriptHdr.UCharPtr+2);
	stacksize = (stacksize + (sizeof(union Value)-1)) & -(sizeof(union Value));
	if( !stacksize )
		return -1;
	
	union Value Stack[stacksize+1];
	memset(Stack, 0, sizeof(union Value) * stacksize+1);
	//union Value *StackLimit = Stack + stacksize+1;
	vm->Regs[regStk].SelfPtr = vm->Regs[regBase].SelfPtr = Stack + stacksize;
			
	(--vm->Regs[regStk].SelfPtr)->Int64 = -1LL;	// push bullshit ret address.
	*--vm->Regs[regStk].SelfPtr = vm->Regs[regBase]; // push rbp
	vm->Regs[regInstr].UCharPtr = main_offset;
	return Tagha_Exec(vm);
}

int32_t Tagha_CallFunc(struct Tagha *const __restrict vm, const char *__restrict funcname, const size_t args, union Value values[static args])
{
	if( !vm or !funcname )
		return -1;
	
	if( *vm->ScriptHdr.UShortPtr != 0xD11 ) {
		puts("Tagha_CallFunc :: ERROR: Script has invalid dll verifier.");
		return -1;
	}
	
	uint8_t *func_offset = GetFunctionOffsetByName(vm, funcname);
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
	
	// remember that arguments must be passed right to left.
	// we have enough args to fit in registers.
	const size_t reg_params = 8;
	const enum RegID reg_param_initial = regPeh;
	const size_t bytecount = sizeof(union Value) * args;
	
	// save stack space by using the registers for passing arguments.
	// the other registers can then be used for other data operations.
	if( args <= reg_params ) {
		memcpy(vm->Regs+reg_param_initial, values, bytecount);
	}
	// if the native has more than a certain num of params, get from both registers and stack.
	else if( args > reg_params ) {
		memcpy(vm->Regs+reg_param_initial, values, sizeof(union Value) * reg_params);
		memcpy(vm->Regs[regStk].SelfPtr, values+reg_params, sizeof(union Value) * (args-reg_params));
		vm->Regs[regStk].SelfPtr -= (args-reg_params);
	}
	
	(--vm->Regs[regStk].SelfPtr)->UCharPtr = vm->Regs[regInstr].UCharPtr+1;	// push return address.
	vm->Regs[regInstr].UCharPtr = func_offset;
	*--vm->Regs[regStk].SelfPtr = vm->Regs[regBase]; // push rbp
	return Tagha_Exec(vm);
}

union Value Tagha_GetReturnValue(const struct Tagha *const __restrict vm)
{
	if( !vm )
		return (union Value){0};
	return vm->Regs[regAlaf];
}

void *Tagha_GetGlobalVarByName(struct Tagha *const __restrict vm, const char *__restrict varname)
{
	if( !vm or !varname )
		return NULL;
	
	return GetVariableOffsetByName(vm, varname);
}

/////////////////////////////////////////////////////////////////////////////////
