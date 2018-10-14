
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "tagha.h"

static void			*GetFunctionOffsetByName(struct TaghaHeader *, const char *);
static void			*GetFunctionOffsetByIndex(struct TaghaHeader *, size_t);
static TaghaNative		*GetNativeByIndex(struct TaghaHeader *, size_t);

static void			*GetVariableOffsetByName(struct TaghaHeader *, const char *);
static void			*GetVariableOffsetByIndex(struct TaghaHeader *, size_t);

static void PrepModule(struct TaghaHeader *const restrict module)
{
	const size_t vartable_offset = module->VarTblOffs;
	{
		union TaghaPtr reader = (union TaghaPtr){.Ptr = (uint8_t *)module + vartable_offset};
		const uint32_t globalvars = *reader.UInt32Ptr++;
		for( uint32_t i=0 ; i<globalvars ; i++ ) {
			reader.UInt8Ptr++;
			const uint64_t sizes = *reader.UInt64Ptr++;
			const uint32_t cstrlen = sizes & 0xffFFffFF;
			const uint32_t datalen = sizes >> 32;
			if( !strcmp("stdin", reader.CStrPtr) ) {
				FILE **fileptr = (FILE **)(reader.UInt8Ptr + cstrlen);
				*fileptr = stdin;
			}
			else if( !strcmp("stdout", reader.CStrPtr) ) {
				FILE **fileptr = (FILE **)(reader.UInt8Ptr + cstrlen);
				*fileptr = stdout;
			}
			else if( !strcmp("stderr", reader.CStrPtr) ) {
				FILE **fileptr = (FILE **)(reader.UInt8Ptr + cstrlen);
				*fileptr = stderr;
			}
			reader.UInt8Ptr += (cstrlen + datalen);
		}
	}
}

static void InvokeNative(struct Tagha *const vm, const size_t argcount, TaghaNative *const NativeCall)
{
	const uint8_t reg_params = 8;
	const uint8_t first_param_register = regSemkath;
	
	/* save stack space by using the registers for passing arguments.
	 * the other registers can then be used for other data operations.
	 */
	union TaghaVal params[argcount];
	// copy native params from registers first.
	memcpy(params, vm->Regs+first_param_register, sizeof params[0] * reg_params);
	// now copy the remaining params off the stack && pop them.
	memcpy(params+reg_params, vm->regStk.SelfPtr, sizeof params[0] * (argcount-reg_params));
	vm->regStk.SelfPtr += (argcount-reg_params);
	// invoke!
	(*NativeCall)(vm, &vm->regAlaf, argcount, params);
}


static void *GetFunctionOffsetByName(struct TaghaHeader *const restrict hdr, const char *restrict funcname)
{
	{
		union TaghaPtr reader = (union TaghaPtr){.Ptr = (uint8_t *)hdr + sizeof *hdr};
		const uint32_t funcs = *reader.UInt32Ptr++;
		for( uint32_t i=0 ; i<funcs ; i++ ) {
			reader.UInt8Ptr++;
			const uint64_t sizes = *reader.UInt64Ptr++;
			const uint32_t cstrlen = sizes & 0xffFFffFF;
			const uint32_t datalen = sizes >> 32;
			if( !strcmp(funcname, reader.CStrPtr) )
				return reader.UInt8Ptr + cstrlen;
			else reader.UInt8Ptr += (cstrlen + datalen);
		}
	}
	return NULL;
}

static void *GetFunctionOffsetByIndex(struct TaghaHeader *const restrict hdr, const size_t index)
{
	{
		union TaghaPtr reader = (union TaghaPtr){.Ptr = (uint8_t *)hdr + sizeof *hdr};
		const uint32_t funcs = *reader.UInt32Ptr++;
		if( index >= funcs )
			return NULL;
		
		for( uint32_t i=0 ; i<funcs ; i++ ) {
			reader.UInt8Ptr++;
			const uint64_t sizes = *reader.UInt64Ptr++;
			const uint32_t cstrlen = sizes & 0xffFFffFF;
			const uint32_t datalen = sizes >> 32;
			if( i==index )
				return reader.UInt8Ptr + cstrlen;
			else reader.UInt8Ptr += (cstrlen + datalen);
		}
	}
	return NULL;
}

static TaghaNative *GetNativeByIndex(struct TaghaHeader *const restrict hdr, const size_t index)
{
	{
		union TaghaPtr reader = (union TaghaPtr){.Ptr = (uint8_t *)hdr + sizeof *hdr};
		const uint32_t funcs = *reader.UInt32Ptr++;
		if( index >= funcs )
			return NULL;
	
		for( uint32_t i=0 ; i<funcs ; i++ ) {
			reader.UInt8Ptr++;
			const uint64_t sizes = *reader.UInt64Ptr++;
			const uint32_t cstrlen = sizes & 0xffFFffFF;
			const uint32_t datalen = sizes >> 32;
			if( i==index )
				return *(TaghaNative **)(reader.UInt8Ptr + cstrlen);
			else reader.UInt8Ptr += (cstrlen + datalen);
		}
	}
	return NULL;
}

static void *GetVariableOffsetByName(struct TaghaHeader *const restrict hdr, const char *restrict varname)
{
	const size_t vartable_offset = hdr->VarTblOffs;
	{
		union TaghaPtr reader = (union TaghaPtr){.Ptr = (uint8_t *)hdr + vartable_offset};
		const uint32_t globalvars = *reader.UInt32Ptr++;
		for( uint32_t i=0 ; i<globalvars ; i++ ) {
			reader.UInt8Ptr++;
			const uint64_t sizes = *reader.UInt64Ptr++;
			const uint32_t cstrlen = sizes & 0xffFFffFF;
			const uint32_t datalen = sizes >> 32;
			if( !strcmp(varname, reader.CStrPtr) )
				return reader.UInt8Ptr + cstrlen;
			else reader.UInt8Ptr += (cstrlen + datalen);
		}
	}
	return NULL;
}

static void *GetVariableOffsetByIndex(struct TaghaHeader *const restrict hdr, const size_t index)
{
	const size_t vartable_offset = hdr->VarTblOffs;
	{
		union TaghaPtr reader = (union TaghaPtr){.Ptr = (uint8_t *)hdr + vartable_offset};
		const uint32_t globalvars = *reader.UInt32Ptr++;
		if( index >= globalvars )
			return NULL;
	
		for( uint32_t i=0 ; i<globalvars ; i++ ) {
			reader.UInt8Ptr++;
			const uint64_t sizes = *reader.UInt64Ptr++;
			const uint32_t cstrlen = sizes & 0xffFFffFF;
			const uint32_t datalen = sizes >> 32;
			if( i==index )
				return reader.UInt8Ptr + cstrlen;
			else reader.UInt8Ptr += (cstrlen + datalen);
		}
	}
	return NULL;
}

#include <sys/mman.h>
int32_t Tagha_JITCompile(struct Tagha *const restrict vm)
{
	const size_t memsize = 4096U;
	uint8_t *mem = mmap(NULL, memsize, PROT_READ | PROT_WRITE,
MAP_ANON | MAP_PRIVATE, -1, 0);
	if( !mem ) {
		vm->Error = ErrBadPtr;
		return -1;
	}
	
	union TaghaPtr reader = (union TaghaPtr){.Ptr = (uint8_t *)vm->Header + sizeof *vm->Header};
	const uint32_t funcs = *reader.UInt32Ptr++;
	for( uint32_t i=0 ; i<funcs ; i++ ) {
		reader.UInt8Ptr++;
		const uint64_t sizes = *reader.UInt64Ptr++;
		const uint32_t cstrlen = sizes & 0xffFFffFF;
		const uint32_t datalen = sizes >> 32;
		if( !strcmp(funcname, reader.CStrPtr) )
			return reader.UInt8Ptr + cstrlen;
		else reader.UInt8Ptr += (cstrlen + datalen);
	}
	
	mprotect(mem, memsize, PROT_READ | PROT_EXEC);
}

//#include <unistd.h>	// sleep() func

int32_t Tagha_Exec(struct Tagha *const restrict vm)
{
	if( !vm )
		return -1;
	
	
	#define DISPATCH() goto *dispatch[decode.opcode = *vm->InstrPtr.UInt16Ptr++, decode.instr]
	
	exec_nop: {
		DISPATCH();
	}
	/* pushes a value to the top of the stack, raises the stack pointer by 8 bytes.
	 * push reg (1 byte for register id)
	 * push imm (8 bytes for constant values)
	 * push [reg+offset] (1 byte reg id + 4-byte signed offset)
	 */
	exec_push: {
		/* push an imm constant. */
		if( decode.addrmode & Immediate ) {
			*--vm->regStk.SelfPtr = *vm->InstrPtr.ValPtr++;
			DISPATCH();
		}
		/* push a register's contents. */
		else if( decode.addrmode & Register ) {
			*--vm->regStk.SelfPtr = vm->Regs[*vm->InstrPtr.UInt8Ptr++];
			DISPATCH();
		}
		/* push the contents of a memory address inside a register. */
		else if( decode.addrmode & RegIndirect ) {
			const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
			const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
			*--vm->regStk.SelfPtr = *mem.ValPtr;
			DISPATCH();
		}
	}
	
	/* pops a value from the stack into a register || memory then reduces stack by 8 bytes.
	 * pop reg
	 * pop [reg+offset]
	 */
	exec_pop: {
		if( decode.addrmode & Register ) {
			vm->Regs[*vm->InstrPtr.UInt8Ptr++] = *vm->regStk.SelfPtr++;
			DISPATCH();
		}
		else if( decode.addrmode & RegIndirect ) {
			const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
			const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
			*mem.ValPtr = *vm->regStk.SelfPtr++;
			DISPATCH();
		}
	}
	
	/* loads a ptr value to a register.
	 * lea reg, global var
	 * lea reg, func
	 * lea reg, [reg+offset] (not dereferenced)
	 */
	exec_lea: {
		if( decode.addrmode & Immediate ) { /* Immediate mode will load a global value */
			const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
			vm->Regs[regid].Ptr = GetVariableOffsetByIndex(vm->Header, *vm->InstrPtr.UInt64Ptr++);
			DISPATCH();
		}
		else if( decode.addrmode & Register ) { /* Register mode will load a function address which could be a native */
			const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
			vm->Regs[regid].Int64 = *vm->InstrPtr.Int64Ptr++;
			DISPATCH();
		}
		else if( decode.addrmode & RegIndirect ) {
			const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
			vm->Regs[regids & 255].UInt8Ptr = vm->Regs[regids >> 8].UInt8Ptr + *vm->InstrPtr.Int32Ptr++;
			DISPATCH();
		}
	}
	
	/* copies a value to a register || memory address.
	 * mov reg, [reg+offset]
	 * mov reg, imm
	 * mov reg, reg
	 * mov [reg+offset], imm
	 * mov [reg+offset], reg
	 */
	exec_mov: {
		if( decode.addrmode & UseReg ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				vm->Regs[regid] = *vm->InstrPtr.ValPtr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				vm->Regs[regids & 255] = vm->Regs[regids >> 8];
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						vm->Regs[regids & 255].UInt8 = *mem.UInt8Ptr;
						DISPATCH();
					case TwoBytes:
						vm->Regs[regids & 255].UInt16 = *mem.UInt16Ptr;
						DISPATCH();
					case FourBytes:
						vm->Regs[regids & 255].UInt32 = *mem.UInt32Ptr;
						DISPATCH();
					case EightBytes:
						vm->Regs[regids & 255].UInt64 = *mem.UInt64Ptr;
						DISPATCH();
				}
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				const union TaghaVal imm = *vm->InstrPtr.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						*mem.UInt8Ptr = imm.UInt8;
						DISPATCH();
					case TwoBytes:
						*mem.UInt16Ptr = imm.UInt16;
						DISPATCH();
					case FourBytes:
						*mem.UInt32Ptr = imm.UInt32;
						DISPATCH();
					case EightBytes:
						*mem.UInt64Ptr = imm.UInt64;
						DISPATCH();
				}
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						*mem.UInt8Ptr = vm->Regs[regids >> 8].UInt8;
						DISPATCH();
					case TwoBytes:
						*mem.UInt16Ptr = vm->Regs[regids >> 8].UInt16;
						DISPATCH();
					case FourBytes:
						*mem.UInt32Ptr = vm->Regs[regids >> 8].UInt32;
						DISPATCH();
					case EightBytes:
						*mem.UInt64Ptr = vm->Regs[regids >> 8].UInt64;
						DISPATCH();
				}
			}
		}
	}
	
	/* adds two values to the destination value which is either a register || memory address.
	 * add reg, [reg+offset]
	 * add reg, imm
	 * add reg, reg
	 * add [reg+offset], reg
	 * add [reg+offset], imm
	 */
	exec_add: {
		if( decode.addrmode & UseReg ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				vm->Regs[regid].UInt64 += *vm->InstrPtr.UInt64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				vm->Regs[regids & 255].UInt64 += vm->Regs[regids >> 8].UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						vm->Regs[regids & 255].UInt8 += *mem.UInt8Ptr;
						DISPATCH();
					case TwoBytes:
						vm->Regs[regids & 255].UInt16 += *mem.UInt16Ptr;
						DISPATCH();
					case FourBytes:
						vm->Regs[regids & 255].UInt32 += *mem.UInt32Ptr;
						DISPATCH();
					case EightBytes:
						vm->Regs[regids & 255].UInt64 += *mem.UInt64Ptr;
						DISPATCH();
				}
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				const union TaghaVal imm = *vm->InstrPtr.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						*mem.UInt8Ptr += imm.UInt8;
						DISPATCH();
					case TwoBytes:
						*mem.UInt16Ptr += imm.UInt16;
						DISPATCH();
					case FourBytes:
						*mem.UInt32Ptr += imm.UInt32;
						DISPATCH();
					case EightBytes:
						*mem.UInt64Ptr += imm.UInt64;
						DISPATCH();
				}
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						*mem.UInt8Ptr += vm->Regs[regids >> 8].UInt8;
						DISPATCH();
					case TwoBytes:
						*mem.UInt16Ptr += vm->Regs[regids >> 8].UInt16;
						DISPATCH();
					case FourBytes:
						*mem.UInt32Ptr += vm->Regs[regids >> 8].UInt32;
						DISPATCH();
					case EightBytes:
						*mem.UInt64Ptr += vm->Regs[regids >> 8].UInt64;
						DISPATCH();
				}
			}
		}
	}
	exec_sub: {
		if( decode.addrmode & UseReg ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				vm->Regs[regid].UInt64 -= *vm->InstrPtr.UInt64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				vm->Regs[regids & 255].UInt64 -= vm->Regs[regids >> 8].UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						vm->Regs[regids & 255].UInt8 -= *mem.UInt8Ptr;
						DISPATCH();
					case TwoBytes:
						vm->Regs[regids & 255].UInt16 -= *mem.UInt16Ptr;
						DISPATCH();
					case FourBytes:
						vm->Regs[regids & 255].UInt32 -= *mem.UInt32Ptr;
						DISPATCH();
					case EightBytes:
						vm->Regs[regids & 255].UInt64 -= *mem.UInt64Ptr;
						DISPATCH();
				}
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				const union TaghaVal imm = *vm->InstrPtr.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						*mem.UInt8Ptr -= imm.UInt8;
						DISPATCH();
					case TwoBytes:
						*mem.UInt16Ptr -= imm.UInt16;
						DISPATCH();
					case FourBytes:
						*mem.UInt32Ptr -= imm.UInt32;
						DISPATCH();
					case EightBytes:
						*mem.UInt64Ptr -= imm.UInt64;
						DISPATCH();
				}
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						*mem.UInt8Ptr -= vm->Regs[regids >> 8].UInt8;
						DISPATCH();
					case TwoBytes:
						*mem.UInt16Ptr -= vm->Regs[regids >> 8].UInt16;
						DISPATCH();
					case FourBytes:
						*mem.UInt32Ptr -= vm->Regs[regids >> 8].UInt32;
						DISPATCH();
					case EightBytes:
						*mem.UInt64Ptr -= vm->Regs[regids >> 8].UInt64;
						DISPATCH();
				}
			}
		}
	}
	exec_mul: {
		if( decode.addrmode & UseReg ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				vm->Regs[regid].UInt64 *= *vm->InstrPtr.UInt64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				vm->Regs[regids & 255].UInt64 *= vm->Regs[regids >> 8].UInt64;
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						vm->Regs[regids & 255].UInt8 *= *mem.UInt8Ptr;
						DISPATCH();
					case TwoBytes:
						vm->Regs[regids & 255].UInt16 *= *mem.UInt16Ptr;
						DISPATCH();
					case FourBytes:
						vm->Regs[regids & 255].UInt32 *= *mem.UInt32Ptr;
						DISPATCH();
					case EightBytes:
						vm->Regs[regids & 255].UInt64 *= *mem.UInt64Ptr;
						DISPATCH();
				}
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				const union TaghaVal imm = *vm->InstrPtr.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						*mem.UInt8Ptr *= imm.UInt8;
						DISPATCH();
					case TwoBytes:
						*mem.UInt16Ptr *= imm.UInt16;
						DISPATCH();
					case FourBytes:
						*mem.UInt32Ptr *= imm.UInt32;
						DISPATCH();
					case EightBytes:
						*mem.UInt64Ptr *= imm.UInt64;
						DISPATCH();
				}
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						*mem.UInt8Ptr *= vm->Regs[regids >> 8].UInt8;
						DISPATCH();
					case TwoBytes:
						*mem.UInt16Ptr *= vm->Regs[regids >> 8].UInt16;
						DISPATCH();
					case FourBytes:
						*mem.UInt32Ptr *= vm->Regs[regids >> 8].UInt32;
						DISPATCH();
					case EightBytes:
						*mem.UInt64Ptr *= vm->Regs[regids >> 8].UInt64;
						DISPATCH();
				}
			}
		}
		DISPATCH();
	}
	exec_divi: {
		if( decode.addrmode & UseReg ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				vm->Regs[regid].UInt64 /= *vm->InstrPtr.UInt64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				vm->Regs[regids & 255].UInt64 /= vm->Regs[regids >> 8].UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						vm->Regs[regids & 255].UInt8 /= *mem.UInt8Ptr;
						DISPATCH();
					case TwoBytes:
						vm->Regs[regids & 255].UInt16 /= *mem.UInt16Ptr;
						DISPATCH();
					case FourBytes:
						vm->Regs[regids & 255].UInt32 /= *mem.UInt32Ptr;
						DISPATCH();
					case EightBytes:
						vm->Regs[regids & 255].UInt64 /= *mem.UInt64Ptr;
						DISPATCH();
				}
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				const union TaghaVal imm = *vm->InstrPtr.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						*mem.UInt8Ptr /= imm.UInt8;
						DISPATCH();
					case TwoBytes:
						*mem.UInt16Ptr /= imm.UInt16;
						DISPATCH();
					case FourBytes:
						*mem.UInt32Ptr /= imm.UInt32;
						DISPATCH();
					case EightBytes:
						*mem.UInt64Ptr /= imm.UInt64;
						DISPATCH();
				}
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						*mem.UInt8Ptr /= vm->Regs[regids >> 8].UInt8;
						DISPATCH();
					case TwoBytes:
						*mem.UInt16Ptr /= vm->Regs[regids >> 8].UInt16;
						DISPATCH();
					case FourBytes:
						*mem.UInt32Ptr /= vm->Regs[regids >> 8].UInt32;
						DISPATCH();
					case EightBytes:
						*mem.UInt64Ptr /= vm->Regs[regids >> 8].UInt64;
						DISPATCH();
				}
			}
		}
	}
	exec_mod: {
		if( decode.addrmode & UseReg ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				vm->Regs[regid].UInt64 %= *vm->InstrPtr.UInt64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				vm->Regs[regids & 255].UInt64 %= vm->Regs[regids >> 8].UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						vm->Regs[regids & 255].UInt8 %= *mem.UInt8Ptr;
						DISPATCH();
					case TwoBytes:
						vm->Regs[regids & 255].UInt16 %= *mem.UInt16Ptr;
						DISPATCH();
					case FourBytes:
						vm->Regs[regids & 255].UInt32 %= *mem.UInt32Ptr;
						DISPATCH();
					case EightBytes:
						vm->Regs[regids & 255].UInt64 %= *mem.UInt64Ptr;
						DISPATCH();
				}
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				const union TaghaVal imm = *vm->InstrPtr.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						*mem.UInt8Ptr %= imm.UInt8;
						DISPATCH();
					case TwoBytes:
						*mem.UInt16Ptr %= imm.UInt16;
						DISPATCH();
					case FourBytes:
						*mem.UInt32Ptr %= imm.UInt32;
						DISPATCH();
					case EightBytes:
						*mem.UInt64Ptr %= imm.UInt64;
						DISPATCH();
				}
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						*mem.UInt8Ptr %= vm->Regs[regids >> 8].UInt8;
						DISPATCH();
					case TwoBytes:
						*mem.UInt16Ptr %= vm->Regs[regids >> 8].UInt16;
						DISPATCH();
					case FourBytes:
						*mem.UInt32Ptr %= vm->Regs[regids >> 8].UInt32;
						DISPATCH();
					case EightBytes:
						*mem.UInt64Ptr %= vm->Regs[regids >> 8].UInt64;
						DISPATCH();
				}
			}
		}
	}
	exec_andb: {
		if( decode.addrmode & UseReg ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				vm->Regs[regid].UInt64 &= *vm->InstrPtr.UInt64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				vm->Regs[regids & 255].UInt64 &= vm->Regs[regids >> 8].UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						vm->Regs[regids & 255].UInt8 &= *mem.UInt8Ptr;
						DISPATCH();
					case TwoBytes:
						vm->Regs[regids & 255].UInt16 &= *mem.UInt16Ptr;
						DISPATCH();
					case FourBytes:
						vm->Regs[regids & 255].UInt32 &= *mem.UInt32Ptr;
						DISPATCH();
					case EightBytes:
						vm->Regs[regids & 255].UInt64 &= *mem.UInt64Ptr;
						DISPATCH();
				}
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				const union TaghaVal imm = *vm->InstrPtr.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						*mem.UInt8Ptr &= imm.UInt8;
						DISPATCH();
					case TwoBytes:
						*mem.UInt16Ptr &= imm.UInt16;
						DISPATCH();
					case FourBytes:
						*mem.UInt32Ptr &= imm.UInt32;
						DISPATCH();
					case EightBytes:
						*mem.UInt64Ptr &= imm.UInt64;
						DISPATCH();
				}
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						*mem.UInt8Ptr &= vm->Regs[regids >> 8].UInt8;
						DISPATCH();
					case TwoBytes:
						*mem.UInt16Ptr &= vm->Regs[regids >> 8].UInt16;
						DISPATCH();
					case FourBytes:
						*mem.UInt32Ptr &= vm->Regs[regids >> 8].UInt32;
						DISPATCH();
					case EightBytes:
						*mem.UInt64Ptr &= vm->Regs[regids >> 8].UInt64;
						DISPATCH();
				}
			}
		}
	}
	exec_orb: {
		if( decode.addrmode & UseReg ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				vm->Regs[regid].UInt64 |= *vm->InstrPtr.UInt64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				vm->Regs[regids & 255].UInt64 |= vm->Regs[regids >> 8].UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						vm->Regs[regids & 255].UInt8 |= *mem.UInt8Ptr;
						DISPATCH();
					case TwoBytes:
						vm->Regs[regids & 255].UInt16 |= *mem.UInt16Ptr;
						DISPATCH();
					case FourBytes:
						vm->Regs[regids & 255].UInt32 |= *mem.UInt32Ptr;
						DISPATCH();
					case EightBytes:
						vm->Regs[regids & 255].UInt64 |= *mem.UInt64Ptr;
						DISPATCH();
				}
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				const union TaghaVal imm = *vm->InstrPtr.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						*mem.UInt8Ptr |= imm.UInt8;
						DISPATCH();
					case TwoBytes:
						*mem.UInt16Ptr |= imm.UInt16;
						DISPATCH();
					case FourBytes:
						*mem.UInt32Ptr |= imm.UInt32;
						DISPATCH();
					case EightBytes:
						*mem.UInt64Ptr |= imm.UInt64;
						DISPATCH();
				}
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						*mem.UInt8Ptr |= vm->Regs[regids >> 8].UInt8;
						DISPATCH();
					case TwoBytes:
						*mem.UInt16Ptr |= vm->Regs[regids >> 8].UInt16;
						DISPATCH();
					case FourBytes:
						*mem.UInt32Ptr |= vm->Regs[regids >> 8].UInt32;
						DISPATCH();
					case EightBytes:
						*mem.UInt64Ptr |= vm->Regs[regids >> 8].UInt64;
						DISPATCH();
				}
			}
		}
	}
	exec_xorb: {
		if( decode.addrmode & UseReg ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				vm->Regs[regid].UInt64 ^= *vm->InstrPtr.UInt64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				vm->Regs[regids & 255].UInt64 ^= vm->Regs[regids >> 8].UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						vm->Regs[regids & 255].UInt8 ^= *mem.UInt8Ptr;
						DISPATCH();
					case TwoBytes:
						vm->Regs[regids & 255].UInt16 ^= *mem.UInt16Ptr;
						DISPATCH();
					case FourBytes:
						vm->Regs[regids & 255].UInt32 ^= *mem.UInt32Ptr;
						DISPATCH();
					case EightBytes:
						vm->Regs[regids & 255].UInt64 ^= *mem.UInt64Ptr;
						DISPATCH();
				}
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				const union TaghaVal imm = *vm->InstrPtr.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						*mem.UInt8Ptr ^= imm.UInt8;
						DISPATCH();
					case TwoBytes:
						*mem.UInt16Ptr ^= imm.UInt16;
						DISPATCH();
					case FourBytes:
						*mem.UInt32Ptr ^= imm.UInt32;
						DISPATCH();
					case EightBytes:
						*mem.UInt64Ptr ^= imm.UInt64;
						DISPATCH();
				}
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						*mem.UInt8Ptr ^= vm->Regs[regids >> 8].UInt8;
						DISPATCH();
					case TwoBytes:
						*mem.UInt16Ptr ^= vm->Regs[regids >> 8].UInt16;
						DISPATCH();
					case FourBytes:
						*mem.UInt32Ptr ^= vm->Regs[regids >> 8].UInt32;
						DISPATCH();
					case EightBytes:
						*mem.UInt64Ptr ^= vm->Regs[regids >> 8].UInt64;
						DISPATCH();
				}
			}
		}
	}
	exec_notb: {
		const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
		if( decode.addrmode & Register ) {
			vm->Regs[regid].UInt64 = ~vm->Regs[regid].UInt64;
			DISPATCH();
		}
		else if( decode.addrmode & RegIndirect ) {
			const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
			switch( decode.addrmode & 0xf0 ) {
				case Byte:
					*mem.UInt8Ptr = ~*mem.UInt8Ptr;
					DISPATCH();
				case TwoBytes:
					*mem.UInt16Ptr = ~*mem.UInt16Ptr;
					DISPATCH();
				case FourBytes:
					*mem.UInt32Ptr = ~*mem.UInt32Ptr;
					DISPATCH();
				case EightBytes:
					*mem.UInt64Ptr = ~*mem.UInt64Ptr;
					DISPATCH();
			}
		}
	}
	exec_shl: {
		if( decode.addrmode & UseReg ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				vm->Regs[regid].UInt64 <<= *vm->InstrPtr.UInt64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				vm->Regs[regids & 255].UInt64 <<= vm->Regs[regids >> 8].UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						vm->Regs[regids & 255].UInt8 <<= *mem.UInt8Ptr;
						DISPATCH();
					case TwoBytes:
						vm->Regs[regids & 255].UInt16 <<= *mem.UInt16Ptr;
						DISPATCH();
					case FourBytes:
						vm->Regs[regids & 255].UInt32 <<= *mem.UInt32Ptr;
						DISPATCH();
					case EightBytes:
						vm->Regs[regids & 255].UInt64 <<= *mem.UInt64Ptr;
						DISPATCH();
				}
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				const union TaghaVal imm = *vm->InstrPtr.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						*mem.UInt8Ptr <<= imm.UInt8;
						DISPATCH();
					case TwoBytes:
						*mem.UInt16Ptr <<= imm.UInt16;
						DISPATCH();
					case FourBytes:
						*mem.UInt32Ptr <<= imm.UInt32;
						DISPATCH();
					case EightBytes:
						*mem.UInt64Ptr <<= imm.UInt64;
						DISPATCH();
				}
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						*mem.UInt8Ptr <<= vm->Regs[regids >> 8].UInt8;
						DISPATCH();
					case TwoBytes:
						*mem.UInt16Ptr <<= vm->Regs[regids >> 8].UInt16;
						DISPATCH();
					case FourBytes:
						*mem.UInt32Ptr <<= vm->Regs[regids >> 8].UInt32;
						DISPATCH();
					case EightBytes:
						*mem.UInt64Ptr <<= vm->Regs[regids >> 8].UInt64;
						DISPATCH();
				}
			}
		}
		DISPATCH();
	}
	exec_shr: {
		if( decode.addrmode & UseReg ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				vm->Regs[regid].UInt64 >>= *vm->InstrPtr.UInt64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				vm->Regs[regids & 255].UInt64 >>= vm->Regs[regids >> 8].UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						vm->Regs[regids & 255].UInt8 >>= *mem.UInt8Ptr;
						DISPATCH();
					case TwoBytes:
						vm->Regs[regids & 255].UInt16 >>= *mem.UInt16Ptr;
						DISPATCH();
					case FourBytes:
						vm->Regs[regids & 255].UInt32 >>= *mem.UInt32Ptr;
						DISPATCH();
					case EightBytes:
						vm->Regs[regids & 255].UInt64 >>= *mem.UInt64Ptr;
						DISPATCH();
				}
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				const union TaghaVal imm = *vm->InstrPtr.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						*mem.UInt8Ptr >>= imm.UInt8;
						DISPATCH();
					case TwoBytes:
						*mem.UInt16Ptr >>= imm.UInt16;
						DISPATCH();
					case FourBytes:
						*mem.UInt32Ptr >>= imm.UInt32;
						DISPATCH();
					case EightBytes:
						*mem.UInt64Ptr >>= imm.UInt64;
						DISPATCH();
				}
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						*mem.UInt8Ptr >>= vm->Regs[regids >> 8].UInt8;
						DISPATCH();
					case TwoBytes:
						*mem.UInt16Ptr >>= vm->Regs[regids >> 8].UInt16;
						DISPATCH();
					case FourBytes:
						*mem.UInt32Ptr >>= vm->Regs[regids >> 8].UInt32;
						DISPATCH();
					case EightBytes:
						*mem.UInt64Ptr >>= vm->Regs[regids >> 8].UInt64;
						DISPATCH();
				}
			}
		}
	}
	exec_inc: {
		const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
		if( decode.addrmode & Register ) {
			++vm->Regs[regid].UInt64;
			DISPATCH();
		}
		else if( decode.addrmode & RegIndirect ) {
			const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
			switch( decode.addrmode & 0xf0 ) {
				case Byte:
					++*mem.UInt8Ptr;
					DISPATCH();
				case TwoBytes:
					++*mem.UInt16Ptr;
					DISPATCH();
				case FourBytes:
					++*mem.UInt32Ptr;
					DISPATCH();
				case EightBytes:
					++*mem.UInt64Ptr;
					DISPATCH();
			}
		}
	}
	exec_dec: {
		const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
		if( decode.addrmode & Register ) {
			--vm->Regs[regid].UInt64;
			DISPATCH();
		}
		else if( decode.addrmode & RegIndirect ) {
			const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
			switch( decode.addrmode & 0xf0 ) {
				case Byte:
					--*mem.UInt8Ptr;
					DISPATCH();
				case TwoBytes:
					--*mem.UInt16Ptr;
					DISPATCH();
				case FourBytes:
					--*mem.UInt32Ptr;
					DISPATCH();
				case EightBytes:
					--*mem.UInt64Ptr;
					DISPATCH();
			}
		}
	}
	exec_neg: {
		const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
		if( decode.addrmode & Register ) {
			vm->Regs[regid].UInt64 = -vm->Regs[regid].UInt64;
			DISPATCH();
		}
		else if( decode.addrmode & RegIndirect ) {
			const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
			switch( decode.addrmode & 0xf0 ) {
				case Byte:
					*mem.UInt8Ptr = -*mem.UInt8Ptr;
					DISPATCH();
				case TwoBytes:
					*mem.UInt16Ptr = -*mem.UInt16Ptr;
					DISPATCH();
				case FourBytes:
					*mem.UInt16Ptr = -*mem.UInt16Ptr;
					DISPATCH();
				case EightBytes:
					*mem.UInt64Ptr = -*mem.UInt64Ptr;
					DISPATCH();
			}
		}
	}
	exec_ilt: {
		if( decode.addrmode & UseReg ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				vm->CondFlag = vm->Regs[regid].Int64 < *vm->InstrPtr.Int64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				vm->CondFlag = vm->Regs[regids & 255].Int64 < vm->Regs[regids >> 8].Int64;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						vm->CondFlag = vm->Regs[regids & 255].Int8 < *mem.Int8Ptr;
						DISPATCH();
					case TwoBytes:
						vm->CondFlag = vm->Regs[regids & 255].Int16 < *mem.Int16Ptr;
						DISPATCH();
					case FourBytes:
						vm->CondFlag = vm->Regs[regids & 255].Int32 < *mem.Int32Ptr;
						DISPATCH();
					case EightBytes:
						vm->CondFlag = vm->Regs[regids & 255].Int64 < *mem.Int64Ptr;
						DISPATCH();
				}
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				const union TaghaVal imm = *vm->InstrPtr.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						vm->CondFlag = *mem.Int8Ptr < imm.Int8;
						DISPATCH();
					case TwoBytes:
						vm->CondFlag = *mem.Int16Ptr < imm.Int16;
						DISPATCH();
					case FourBytes:
						vm->CondFlag = *mem.Int32Ptr < imm.Int32;
						DISPATCH();
					case EightBytes:
						vm->CondFlag = *mem.Int64Ptr < imm.Int64;
						DISPATCH();
				}
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						vm->CondFlag = *mem.Int8Ptr < vm->Regs[regids >> 8].Int8;
						DISPATCH();
					case TwoBytes:
						vm->CondFlag = *mem.Int16Ptr < vm->Regs[regids >> 8].Int16;
						DISPATCH();
					case FourBytes:
						vm->CondFlag = *mem.Int32Ptr < vm->Regs[regids >> 8].Int32;
						DISPATCH();
					case EightBytes:
						vm->CondFlag = *mem.Int64Ptr < vm->Regs[regids >> 8].Int64;
						DISPATCH();
				}
			}
		}
	}
	exec_igt: {
		if( decode.addrmode & UseReg ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				vm->CondFlag = vm->Regs[regid].Int64 > *vm->InstrPtr.Int64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				vm->CondFlag = vm->Regs[regids & 255].Int64 > vm->Regs[regids >> 8].Int64;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						vm->CondFlag = vm->Regs[regids & 255].Int8 > *mem.Int8Ptr;
						DISPATCH();
					case TwoBytes:
						vm->CondFlag = vm->Regs[regids & 255].Int16 > *mem.Int16Ptr;
						DISPATCH();
					case FourBytes:
						vm->CondFlag = vm->Regs[regids & 255].Int32 > *mem.Int32Ptr;
						DISPATCH();
					case EightBytes:
						vm->CondFlag = vm->Regs[regids & 255].Int64 > *mem.Int64Ptr;
						DISPATCH();
				}
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				const union TaghaVal imm = *vm->InstrPtr.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						vm->CondFlag = *mem.Int8Ptr > imm.Int8;
						DISPATCH();
					case TwoBytes:
						vm->CondFlag = *mem.Int16Ptr > imm.Int16;
						DISPATCH();
					case FourBytes:
						vm->CondFlag = *mem.Int32Ptr > imm.Int32;
						DISPATCH();
					case EightBytes:
						vm->CondFlag = *mem.Int64Ptr > imm.Int64;
						DISPATCH();
				}
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						vm->CondFlag = *mem.Int8Ptr > vm->Regs[regids >> 8].Int8;
						DISPATCH();
					case TwoBytes:
						vm->CondFlag = *mem.Int16Ptr > vm->Regs[regids >> 8].Int16;
						DISPATCH();
					case FourBytes:
						vm->CondFlag = *mem.Int32Ptr > vm->Regs[regids >> 8].Int32;
						DISPATCH();
					case EightBytes:
						vm->CondFlag = *mem.Int64Ptr > vm->Regs[regids >> 8].Int64;
						DISPATCH();
				}
			}
		}
	}
	exec_ult: {
		if( decode.addrmode & UseReg ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				vm->CondFlag = vm->Regs[regid].UInt64 < *vm->InstrPtr.UInt64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				vm->CondFlag = vm->Regs[regids & 255].UInt64 < vm->Regs[regids >> 8].UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						vm->CondFlag = vm->Regs[regids & 255].UInt8 < *mem.UInt8Ptr;
						DISPATCH();
					case TwoBytes:
						vm->CondFlag = vm->Regs[regids & 255].UInt16 < *mem.UInt16Ptr;
						DISPATCH();
					case FourBytes:
						vm->CondFlag = vm->Regs[regids & 255].UInt32 < *mem.UInt32Ptr;
						DISPATCH();
					case EightBytes:
						vm->CondFlag = vm->Regs[regids & 255].UInt64 < *mem.UInt64Ptr;
						DISPATCH();
				}
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				const union TaghaVal imm = *vm->InstrPtr.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						vm->CondFlag = *mem.UInt8Ptr < imm.UInt8;
						DISPATCH();
					case TwoBytes:
						vm->CondFlag = *mem.UInt16Ptr < imm.UInt16;
						DISPATCH();
					case FourBytes:
						vm->CondFlag = *mem.UInt32Ptr < imm.UInt32;
						DISPATCH();
					case EightBytes:
						vm->CondFlag = *mem.UInt64Ptr < imm.UInt64;
						DISPATCH();
				}
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						vm->CondFlag = *mem.UInt8Ptr < vm->Regs[regids >> 8].UInt8;
						DISPATCH();
					case TwoBytes:
						vm->CondFlag = *mem.UInt16Ptr < vm->Regs[regids >> 8].UInt16;
						DISPATCH();
					case FourBytes:
						vm->CondFlag = *mem.UInt32Ptr < vm->Regs[regids >> 8].UInt32;
						DISPATCH();
					case EightBytes:
						vm->CondFlag = *mem.UInt64Ptr < vm->Regs[regids >> 8].UInt64;
						DISPATCH();
				}
			}
		}
	}
	exec_ugt: {
		if( decode.addrmode & UseReg ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				vm->CondFlag = vm->Regs[regid].UInt64 > *vm->InstrPtr.UInt64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				vm->CondFlag = vm->Regs[regids & 255].UInt64 > vm->Regs[regids >> 8].UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						vm->CondFlag = vm->Regs[regids & 255].UInt8 > *mem.UInt8Ptr;
						DISPATCH();
					case TwoBytes:
						vm->CondFlag = vm->Regs[regids & 255].UInt16 > *mem.UInt16Ptr;
						DISPATCH();
					case FourBytes:
						vm->CondFlag = vm->Regs[regids & 255].UInt32 > *mem.UInt32Ptr;
						DISPATCH();
					case EightBytes:
						vm->CondFlag = vm->Regs[regids & 255].UInt64 > *mem.UInt64Ptr;
						DISPATCH();
				}
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				const union TaghaVal imm = *vm->InstrPtr.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						vm->CondFlag = *mem.UInt8Ptr > imm.UInt8;
						DISPATCH();
					case TwoBytes:
						vm->CondFlag = *mem.UInt16Ptr > imm.UInt16;
						DISPATCH();
					case FourBytes:
						vm->CondFlag = *mem.UInt32Ptr > imm.UInt32;
						DISPATCH();
					case EightBytes:
						vm->CondFlag = *mem.UInt64Ptr > imm.UInt64;
						DISPATCH();
				}
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						vm->CondFlag = *mem.UInt8Ptr > vm->Regs[regids >> 8].UInt8;
						DISPATCH();
					case TwoBytes:
						vm->CondFlag = *mem.UInt16Ptr > vm->Regs[regids >> 8].UInt16;
						DISPATCH();
					case FourBytes:
						vm->CondFlag = *mem.UInt32Ptr > vm->Regs[regids >> 8].UInt32;
						DISPATCH();
					case EightBytes:
						vm->CondFlag = *mem.UInt64Ptr > vm->Regs[regids >> 8].UInt64;
						DISPATCH();
				}
			}
		}
	}
	exec_cmp: {
		if( decode.addrmode & UseReg ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				vm->CondFlag = vm->Regs[regid].UInt64 == *vm->InstrPtr.UInt64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				vm->CondFlag = vm->Regs[regids & 255].UInt64 == vm->Regs[regids >> 8].UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						vm->CondFlag = vm->Regs[regids & 255].UInt8 == *mem.UInt8Ptr;
						DISPATCH();
					case TwoBytes:
						vm->CondFlag = vm->Regs[regids & 255].UInt16 == *mem.UInt16Ptr;
						DISPATCH();
					case FourBytes:
						vm->CondFlag = vm->Regs[regids & 255].UInt32 == *mem.UInt32Ptr;
						DISPATCH();
					case EightBytes:
						vm->CondFlag = vm->Regs[regids & 255].UInt64 == *mem.UInt64Ptr;
						DISPATCH();
				}
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				const union TaghaVal imm = *vm->InstrPtr.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						vm->CondFlag = *mem.UInt8Ptr == imm.UInt8;
						DISPATCH();
					case TwoBytes:
						vm->CondFlag = *mem.UInt16Ptr == imm.UInt16;
						DISPATCH();
					case FourBytes:
						vm->CondFlag = *mem.UInt32Ptr == imm.UInt32;
						DISPATCH();
					case EightBytes:
						vm->CondFlag = *mem.UInt64Ptr == imm.UInt64;
						DISPATCH();
				}
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						vm->CondFlag = *mem.UInt8Ptr == vm->Regs[regids >> 8].UInt8;
						DISPATCH();
					case TwoBytes:
						vm->CondFlag = *mem.UInt16Ptr == vm->Regs[regids >> 8].UInt16;
						DISPATCH();
					case FourBytes:
						vm->CondFlag = *mem.UInt32Ptr == vm->Regs[regids >> 8].UInt32;
						DISPATCH();
					case EightBytes:
						vm->CondFlag = *mem.UInt64Ptr == vm->Regs[regids >> 8].UInt64;
						DISPATCH();
				}
			}
		}
	}
	exec_neq: {
		if( decode.addrmode & UseReg ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				vm->CondFlag = vm->Regs[regid].UInt64 != *vm->InstrPtr.UInt64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				vm->CondFlag = vm->Regs[regids & 255].UInt64 != vm->Regs[regids >> 8].UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						vm->CondFlag = vm->Regs[regids & 255].UInt8 != *mem.UInt8Ptr;
						DISPATCH();
					case TwoBytes:
						vm->CondFlag = vm->Regs[regids & 255].UInt16 != *mem.UInt16Ptr;
						DISPATCH();
					case FourBytes:
						vm->CondFlag = vm->Regs[regids & 255].UInt32 != *mem.UInt32Ptr;
						DISPATCH();
					case EightBytes:
						vm->CondFlag = vm->Regs[regids & 255].UInt64 != *mem.UInt64Ptr;
						DISPATCH();
				}
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				const union TaghaVal imm = *vm->InstrPtr.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						vm->CondFlag = *mem.UInt8Ptr != imm.UInt8;
						DISPATCH();
					case TwoBytes:
						vm->CondFlag = *mem.UInt16Ptr != imm.UInt16;
						DISPATCH();
					case FourBytes:
						vm->CondFlag = *mem.UInt32Ptr != imm.UInt32;
						DISPATCH();
					case EightBytes:
						vm->CondFlag = *mem.UInt64Ptr != imm.UInt64;
						DISPATCH();
				}
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case Byte:
						vm->CondFlag = *mem.UInt8Ptr != vm->Regs[regids >> 8].UInt8;
						DISPATCH();
					case TwoBytes:
						vm->CondFlag = *mem.UInt16Ptr != vm->Regs[regids >> 8].UInt16;
						DISPATCH();
					case FourBytes:
						vm->CondFlag = *mem.UInt32Ptr != vm->Regs[regids >> 8].UInt32;
						DISPATCH();
					case EightBytes:
						vm->CondFlag = *mem.UInt64Ptr != vm->Regs[regids >> 8].UInt64;
						DISPATCH();
				}
			}
		}
	}
	exec_jmp: {
		if( decode.addrmode & Immediate ) {
			const int64_t offset = *vm->InstrPtr.Int64Ptr++;
			vm->InstrPtr.UInt8Ptr += offset;
			DISPATCH();
		}
		else if( decode.addrmode & Register ) {
			const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
			vm->InstrPtr.UInt8Ptr += vm->Regs[regid].Int64;
			DISPATCH();
		}
		else if( decode.addrmode & RegIndirect ) {
			const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
			const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
			switch( decode.addrmode & 0xf0 ) {
				case Byte:
					vm->InstrPtr.UInt8Ptr += *mem.Int8Ptr;
					DISPATCH();
				case TwoBytes:
					vm->InstrPtr.UInt8Ptr += *mem.Int16Ptr;
					DISPATCH();
				case FourBytes:
					vm->InstrPtr.UInt8Ptr += *mem.Int32Ptr;
					DISPATCH();
				case EightBytes:
					vm->InstrPtr.UInt8Ptr += *mem.Int64Ptr;
					DISPATCH();
			}
		}
	}
	exec_jz: {
		if( decode.addrmode & Immediate ) {
			const int64_t offset = *vm->InstrPtr.Int64Ptr++;
			!vm->CondFlag ? (vm->InstrPtr.UInt8Ptr += offset) : (void)vm->InstrPtr;
			DISPATCH();
		}
		else if( decode.addrmode & Register ) {
			const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
			!vm->CondFlag ? (vm->InstrPtr.UInt8Ptr += vm->Regs[regid].Int64) : (void)vm->InstrPtr;
			DISPATCH();
		}
		else if( decode.addrmode & RegIndirect ) {
			const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
			const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
			switch( decode.addrmode & 0xf0 ) {
				case Byte:
					!vm->CondFlag ? (vm->InstrPtr.UInt8Ptr += *mem.Int8Ptr) : (void)vm->InstrPtr;
					DISPATCH();
				case TwoBytes:
					!vm->CondFlag ? (vm->InstrPtr.UInt8Ptr += *mem.Int16Ptr) : (void)vm->InstrPtr;
					DISPATCH();
				case FourBytes:
					!vm->CondFlag ? (vm->InstrPtr.UInt8Ptr += *mem.Int32Ptr) : (void)vm->InstrPtr;
					DISPATCH();
				case EightBytes:
					!vm->CondFlag ? (vm->InstrPtr.UInt8Ptr += *mem.Int64Ptr) : (void)vm->InstrPtr;
					DISPATCH();
			}
		}
	}
	exec_jnz: {
		if( decode.addrmode & Immediate ) {
			const int64_t offset = *vm->InstrPtr.Int64Ptr++;
			vm->CondFlag ? (vm->InstrPtr.UInt8Ptr += offset) : (void)vm->InstrPtr;
			DISPATCH();
		}
		else if( decode.addrmode & Register ) {
			const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
			vm->CondFlag ? (vm->InstrPtr.UInt8Ptr += vm->Regs[regid].Int64) : (void)vm->InstrPtr;
			DISPATCH();
		}
		else if( decode.addrmode & RegIndirect ) {
			const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
			const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
			switch( decode.addrmode & 0xf0 ) {
				case Byte:
					vm->CondFlag ? (vm->InstrPtr.UInt8Ptr += *mem.Int8Ptr) : (void)vm->InstrPtr;
					DISPATCH();
				case TwoBytes:
					vm->CondFlag ? (vm->InstrPtr.UInt8Ptr += *mem.Int16Ptr) : (void)vm->InstrPtr;
					DISPATCH();
				case FourBytes:
					vm->CondFlag ? (vm->InstrPtr.UInt8Ptr += *mem.Int32Ptr) : (void)vm->InstrPtr;
					DISPATCH();
				case EightBytes:
					vm->CondFlag ? (vm->InstrPtr.UInt8Ptr += *mem.Int64Ptr) : (void)vm->InstrPtr;
					DISPATCH();
			}
		}
		DISPATCH();
	}
	exec_call: {
		uint64_t index = -1;
		if( decode.addrmode & Immediate ) {
			index = ((*vm->InstrPtr.UInt64Ptr++) - 1);
		}
		else if( decode.addrmode & Register ) {
			index = (vm->Regs[*vm->InstrPtr.UInt8Ptr++].UInt64 - 1);
		}
		else if( decode.addrmode & RegIndirect ) {
			const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
			const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
			index = (*mem.UInt64Ptr - 1);
		}
		/* The restrict type qualifier is an indication to the compiler that,
		 * if the memory addressed by the restrict-qualified pointer is modified,
		 * no other pointer will access that same memory.
		 * Since we're pushing the restrict-qualified pointer's memory that it points to,
		 * This is NOT undefined behavior because it's not aliasing access of the instruction stream.
		 */
		(--vm->regStk.SelfPtr)->Ptr = vm->InstrPtr.Ptr;	/* push rip */
		*--vm->regStk.SelfPtr = vm->regBase;	/* push rbp */
		vm->regBase = vm->regStk;	/* mov rbp, rsp */
		vm->InstrPtr.UInt8Ptr = GetFunctionOffsetByIndex(vm->Header, index);
		DISPATCH();
	}
	exec_ret: {
		vm->regStk = vm->regBase; /* mov rsp, rbp */
		vm->regBase = *vm->regStk.SelfPtr++; /* pop rbp */
		vm->InstrPtr.Ptr = (*vm->regStk.SelfPtr++).Ptr; /* pop rip */
		if( !vm->InstrPtr.Ptr ) {
			goto *dispatch[halt];
		}
		else { DISPATCH(); }
	}
	
	exec_syscall: {
		/* how many args given to the native call. */
		//const uint32_t argcount = *vm->InstrPtr.UInt32Ptr++;
		// syscall args size moved to alaf register.
		uint64_t index = -1;
		/* trying to directly call a specific native. Allow this by imm only! */
		if( decode.addrmode & Immediate ) {
			index = (-1 - *vm->InstrPtr.Int64Ptr++);
		}
		else if( decode.addrmode & Register ) {
			index = (-1 - vm->Regs[*vm->InstrPtr.UInt8Ptr++].Int64);
		}
		else if( decode.addrmode & RegIndirect ) {
			const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
			const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
			index = (-1 - *mem.Int64Ptr);
		}
		/* native call interface
		 * Limitations:
		 *  - any argument larger than 8 bytes must be passed as a pointer.
		 *  - any return value larger than 8 bytes must be passed as a hidden pointer argument && render the function as void.
		 * void NativeFunc(struct Tagha *sys, union TaghaVal *retval, const size_t args, union TaghaVal params[static args]);
		 */
		TaghaNative *const nativeref = GetNativeByIndex(vm->Header, index);
		if( !nativeref ) {
			// commenting this out because it slows down the code for some reason...
			vm->Error = ErrMissingNative;
			goto *dispatch[halt];
		}
		else {
			const uint8_t reg_params = 8;
			const uint8_t reg_param_initial = regSemkath;
			const size_t argcount = vm->regAlaf.SizeInt;
			vm->regAlaf.UInt64 = 0;
			
			/* save stack space by using the registers for passing arguments. */
			/* the other registers can then be used for other data operations. */
			if( argcount <= reg_params ) {
				(*nativeref)(vm, &vm->regAlaf, argcount, vm->Regs+reg_param_initial);
			}
			/* if the native has more than a certain num of params, get from both registers && stack. */
			else InvokeNative(vm, argcount, nativeref);
			DISPATCH();
		}
	}
#if FLOATING_POINT_OPS
	exec_flt2dbl: {
		if( decode.addrmode & Register ) {
			const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
			const float f = vm->Regs[regid].Float;
			vm->Regs[regid].Double = (double)f;
			DISPATCH();
		}
	}
	exec_dbl2flt: {
		if( decode.addrmode & Register ) {
			const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
			const double d = vm->Regs[regid].Double;
			vm->Regs[regid].UInt64 = 0;
			vm->Regs[regid].Float = (float)d;
			DISPATCH();
		}
	}
	exec_int2dbl: {
		if( decode.addrmode & Register ) {
			const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
			const uint64_t i = vm->Regs[regid].UInt64;
			vm->Regs[regid].Double = (double)i;
			DISPATCH();
		}
	}
	exec_int2flt: {
		if( decode.addrmode & Register ) {
			const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
			const uint64_t i = vm->Regs[regid].UInt64;
			vm->Regs[regid].UInt64 = 0;
			vm->Regs[regid].Float = (float)i;
			DISPATCH();
		}
	}
	exec_addf: {
		if( decode.addrmode & UseReg ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				const union TaghaVal imm = *vm->InstrPtr.ValPtr++;
				if( decode.addrmode & FourBytes )
					vm->Regs[regid].Float += imm.Float;
				else if( decode.addrmode & EightBytes )
					vm->Regs[regid].Double += imm.Double;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				if( decode.addrmode & FourBytes )
					vm->Regs[regids & 255].Float += vm->Regs[regids >> 8].Float;
				else if( decode.addrmode & EightBytes )
					vm->Regs[regids & 255].Double += vm->Regs[regids >> 8].Double;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case FourBytes:
						vm->Regs[regids & 255].Float += *mem.FloatPtr;
						DISPATCH();
					case EightBytes:
						vm->Regs[regids & 255].Double += *mem.DoublePtr;
						DISPATCH();
				}
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				const union TaghaVal imm = *vm->InstrPtr.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case FourBytes:
						*mem.FloatPtr += imm.Float;
						DISPATCH();
					case EightBytes:
						*mem.DoublePtr += imm.Double;
						DISPATCH();
				}
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case FourBytes:
						*mem.FloatPtr += vm->Regs[regids >> 8].Float;
						DISPATCH();
					case EightBytes:
						*mem.DoublePtr += vm->Regs[regids >> 8].Double;
						DISPATCH();
				}
			}
		}
	}
	exec_subf: {
		if( decode.addrmode & UseReg ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				const union TaghaVal imm = *vm->InstrPtr.ValPtr++;
				if( decode.addrmode & FourBytes )
					vm->Regs[regid].Float -= imm.Float;
				else if( decode.addrmode & EightBytes )
					vm->Regs[regid].Double -= imm.Double;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				if( decode.addrmode & FourBytes )
					vm->Regs[regids & 255].Float -= vm->Regs[regids >> 8].Float;
				else if( decode.addrmode & EightBytes )
					vm->Regs[regids & 255].Double -= vm->Regs[regids >> 8].Double;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case FourBytes:
						vm->Regs[regids & 255].Float -= *mem.FloatPtr;
						DISPATCH();
					case EightBytes:
						vm->Regs[regids & 255].Double -= *mem.DoublePtr;
						DISPATCH();
				}
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				const union TaghaVal imm = *vm->InstrPtr.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case FourBytes:
						*mem.FloatPtr -= imm.Float;
						DISPATCH();
					case EightBytes:
						*mem.DoublePtr -= imm.Double;
						DISPATCH();
				}
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case FourBytes:
						*mem.FloatPtr -= vm->Regs[regids >> 8].Float;
						DISPATCH();
					case EightBytes:
						*mem.DoublePtr -= vm->Regs[regids >> 8].Double;
						DISPATCH();
				}
			}
		}
	}
	exec_mulf: {
		if( decode.addrmode & UseReg ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				const union TaghaVal imm = *vm->InstrPtr.ValPtr++;
				if( decode.addrmode & FourBytes )
					vm->Regs[regid].Float *= imm.Float;
				else if( decode.addrmode & EightBytes )
					vm->Regs[regid].Double *= imm.Double;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				if( decode.addrmode & FourBytes )
					vm->Regs[regids & 255].Float *= vm->Regs[regids >> 8].Float;
				else if( decode.addrmode & EightBytes )
					vm->Regs[regids & 255].Double *= vm->Regs[regids >> 8].Double;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case FourBytes:
						vm->Regs[regids & 255].Float *= *mem.FloatPtr;
						DISPATCH();
					case EightBytes:
						vm->Regs[regids & 255].Double *= *mem.DoublePtr;
						DISPATCH();
				}
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				const union TaghaVal imm = *vm->InstrPtr.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case FourBytes:
						*mem.FloatPtr *= imm.Float;
						DISPATCH();
					case EightBytes:
						*mem.DoublePtr *= imm.Double;
						DISPATCH();
				}
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case FourBytes:
						*mem.FloatPtr *= vm->Regs[regids >> 8].Float;
						DISPATCH();
					case EightBytes:
						*mem.DoublePtr *= vm->Regs[regids >> 8].Double;
						DISPATCH();
				}
			}
		}
	}
	exec_divf: {
		if( decode.addrmode & UseReg ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				const union TaghaVal imm = *vm->InstrPtr.ValPtr++;
				if( decode.addrmode & FourBytes )
					vm->Regs[regid].Float /= imm.Float;
				else if( decode.addrmode & EightBytes )
					vm->Regs[regid].Double /= imm.Double;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				if( decode.addrmode & FourBytes )
					vm->Regs[regids & 255].Float /= vm->Regs[regids >> 8].Float;
				else if( decode.addrmode & EightBytes )
					vm->Regs[regids & 255].Double /= vm->Regs[regids >> 8].Double;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case FourBytes:
						vm->Regs[regids & 255].Float /= *mem.FloatPtr;
						DISPATCH();
					case EightBytes:
						vm->Regs[regids & 255].Double /= *mem.DoublePtr;
						DISPATCH();
				}
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				const union TaghaVal imm = *vm->InstrPtr.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case FourBytes:
						*mem.FloatPtr /= imm.Float;
						DISPATCH();
					case EightBytes:
						*mem.DoublePtr /= imm.Double;
						DISPATCH();
				}
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case FourBytes:
						*mem.FloatPtr /= vm->Regs[regids >> 8].Float;
						DISPATCH();
					case EightBytes:
						*mem.DoublePtr /= vm->Regs[regids >> 8].Double;
						DISPATCH();
				}
			}
		}
	}
	exec_ltf: {
		if( decode.addrmode & UseReg ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				const union TaghaVal imm = *vm->InstrPtr.ValPtr++;
				if( decode.addrmode & FourBytes )
					vm->CondFlag = vm->Regs[regid].Float < imm.Float;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = vm->Regs[regid].Double < imm.Double;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				if( decode.addrmode & FourBytes )
					vm->CondFlag = vm->Regs[regids & 255].Float < vm->Regs[regids >> 8].Float;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = vm->Regs[regids & 255].Double < vm->Regs[regids >> 8].Double;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case FourBytes:
						vm->CondFlag = vm->Regs[regids & 255].Float < *mem.FloatPtr;
						DISPATCH();
					case EightBytes:
						vm->CondFlag = vm->Regs[regids & 255].Double < *mem.DoublePtr;
						DISPATCH();
				}
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				const union TaghaVal imm = *vm->InstrPtr.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case FourBytes:
						vm->CondFlag = *mem.FloatPtr < imm.Float;
						DISPATCH();
					case EightBytes:
						vm->CondFlag = *mem.DoublePtr < imm.Double;
						DISPATCH();
				}
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case FourBytes:
						vm->CondFlag = *mem.FloatPtr < vm->Regs[regids >> 8].Float;
						DISPATCH();
					case EightBytes:
						vm->CondFlag = *mem.DoublePtr < vm->Regs[regids >> 8].Double;
						DISPATCH();
				}
			}
		}
	}
	exec_gtf: {
		if( decode.addrmode & UseReg ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				const union TaghaVal imm = *vm->InstrPtr.ValPtr++;
				if( decode.addrmode & FourBytes )
					vm->CondFlag = vm->Regs[regid].Float > imm.Float;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = vm->Regs[regid].Double > imm.Double;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				if( decode.addrmode & FourBytes )
					vm->CondFlag = vm->Regs[regids & 255].Float > vm->Regs[regids >> 8].Float;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = vm->Regs[regids & 255].Double > vm->Regs[regids >> 8].Double;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case FourBytes:
						vm->CondFlag = vm->Regs[regids & 255].Float > *mem.FloatPtr;
						DISPATCH();
					case EightBytes:
						vm->CondFlag = vm->Regs[regids & 255].Double > *mem.DoublePtr;
						DISPATCH();
				}
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				const union TaghaVal imm = *vm->InstrPtr.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case FourBytes:
						vm->CondFlag = *mem.FloatPtr > imm.Float;
						DISPATCH();
					case EightBytes:
						vm->CondFlag = *mem.DoublePtr > imm.Double;
						DISPATCH();
				}
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case FourBytes:
						vm->CondFlag = *mem.FloatPtr > vm->Regs[regids >> 8].Float;
						DISPATCH();
					case EightBytes:
						vm->CondFlag = *mem.DoublePtr > vm->Regs[regids >> 8].Double;
						DISPATCH();
				}
			}
		}
	}
	exec_cmpf: {
		if( decode.addrmode & UseReg ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				const union TaghaVal imm = *vm->InstrPtr.ValPtr++;
				if( decode.addrmode & FourBytes )
					vm->CondFlag = vm->Regs[regid].Float == imm.Float;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = vm->Regs[regid].Double == imm.Double;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				if( decode.addrmode & FourBytes )
					vm->CondFlag = vm->Regs[regids & 255].Float == vm->Regs[regids >> 8].Float;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = vm->Regs[regids & 255].Double == vm->Regs[regids >> 8].Double;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case FourBytes:
						vm->CondFlag = vm->Regs[regids & 255].Float == *mem.FloatPtr;
						DISPATCH();
					case EightBytes:
						vm->CondFlag = vm->Regs[regids & 255].Double == *mem.DoublePtr;
						DISPATCH();
				}
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				const union TaghaVal imm = *vm->InstrPtr.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case FourBytes:
						vm->CondFlag = *mem.FloatPtr == imm.Float;
						DISPATCH();
					case EightBytes:
						vm->CondFlag = *mem.DoublePtr == imm.Double;
						DISPATCH();
				}
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case FourBytes:
						vm->CondFlag = *mem.FloatPtr == vm->Regs[regids >> 8].Float;
						DISPATCH();
					case EightBytes:
						vm->CondFlag = *mem.DoublePtr == vm->Regs[regids >> 8].Double;
						DISPATCH();
				}
			}
		}
	}
	exec_neqf: {
		if( decode.addrmode & UseReg ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				const union TaghaVal imm = *vm->InstrPtr.ValPtr++;
				if( decode.addrmode & FourBytes )
					vm->CondFlag = vm->Regs[regid].Float != imm.Float;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = vm->Regs[regid].Double != imm.Double;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				if( decode.addrmode & FourBytes )
					vm->CondFlag = vm->Regs[regids & 255].Float != vm->Regs[regids >> 8].Float;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = vm->Regs[regids & 255].Double != vm->Regs[regids >> 8].Double;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case FourBytes:
						vm->CondFlag = vm->Regs[regids & 255].Float != *mem.FloatPtr;
						DISPATCH();
					case EightBytes:
						vm->CondFlag = vm->Regs[regids & 255].Double != *mem.DoublePtr;
						DISPATCH();
				}
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
				const union TaghaVal imm = *vm->InstrPtr.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case FourBytes:
						vm->CondFlag = *mem.FloatPtr != imm.Float;
						DISPATCH();
					case EightBytes:
						vm->CondFlag = *mem.DoublePtr != imm.Double;
						DISPATCH();
				}
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *vm->InstrPtr.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
				switch( decode.addrmode & 0xf0 ) {
					case FourBytes:
						vm->CondFlag = *mem.FloatPtr != vm->Regs[regids >> 8].Float;
						DISPATCH();
					case EightBytes:
						vm->CondFlag = *mem.DoublePtr != vm->Regs[regids >> 8].Double;
						DISPATCH();
				}
			}
		}
	}
	exec_incf: {
		const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
		if( decode.addrmode & Register ) {
			if( decode.addrmode & FourBytes )
				++vm->Regs[regid].Float;
			else if( decode.addrmode & EightBytes )
				++vm->Regs[regid].Double;
			DISPATCH();
		}
		else if( decode.addrmode & RegIndirect ) {
			const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
			switch( decode.addrmode & 0xf0 ) {
				case FourBytes:
					++*mem.FloatPtr;
					DISPATCH();
				case EightBytes:
					++*mem.DoublePtr;
					DISPATCH();
			}
		}
	}
	exec_decf: {
		const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
		if( decode.addrmode & Register ) {
			if( decode.addrmode & FourBytes )
				--vm->Regs[regid].Float;
			else if( decode.addrmode & EightBytes )
				--vm->Regs[regid].Double;
			DISPATCH();
		}
		else if( decode.addrmode & RegIndirect ) {
			const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
			switch( decode.addrmode & 0xf0 ) {
				case FourBytes:
					--*mem.FloatPtr;
					DISPATCH();
				case EightBytes:
					--*mem.DoublePtr;
					DISPATCH();
			}
		}
	}
	exec_negf: {
		const uint8_t regid = *vm->InstrPtr.UInt8Ptr++;
		if( decode.addrmode & Register ) {
			if( decode.addrmode & FourBytes )
				vm->Regs[regid].Float = -vm->Regs[regid].Float;
			else if( decode.addrmode & EightBytes )
				vm->Regs[regid].Double = -vm->Regs[regid].Double;
			DISPATCH();
		}
		else if( decode.addrmode & RegIndirect ) {
			const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *vm->InstrPtr.Int32Ptr++};
			switch( decode.addrmode & 0xf0 ) {
				case FourBytes:
					*mem.FloatPtr = -*mem.FloatPtr;
					DISPATCH();
				case EightBytes:
					*mem.DoublePtr = -*mem.DoublePtr;
					DISPATCH();
			}
		}
	}
#endif
	exec_halt:
	//Tagha_PrintVMState(vm);
	return vm->regAlaf.Int32;
}

/************************************************/
