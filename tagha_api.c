
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


struct Tagha *Tagha_New(void *restrict script)
{
	struct Tagha *restrict vm = calloc(1, sizeof *vm);
	Tagha_Init(vm, script);
	return vm;
}

struct Tagha *Tagha_NewNatives(void *restrict script, const struct NativeInfo natives[restrict])
{
	struct Tagha *restrict vm = Tagha_New(script);
	Tagha_RegisterNatives(vm, natives);
	return vm;
}

void Tagha_Free(struct Tagha **vmref)
{
	if( !vmref || !*vmref )
		return;
	free(*vmref), *vmref = NULL;
}

void Tagha_Init(struct Tagha *const restrict vm, void *script)
{
	if( !vm || !script )
		return;
	
	*vm = (struct Tagha){0};
	PrepModule(script);
	vm->Header = script;
}

void Tagha_InitNatives(struct Tagha *const restrict vm, void *restrict script, const struct NativeInfo natives[restrict])
{
	Tagha_Init(vm, script);
	Tagha_RegisterNatives(vm, natives);
}



void Tagha_PrintVMState(const struct Tagha *const vm)
{
	if( !vm )
		return;
	
	printf("=== Tagha VM State Info ===\n\nPrinting Registers:\nregister alaf: '%" PRIu64 "'\nregister beth: '%" PRIu64 "'\nregister gamal: '%" PRIu64 "'\nregister dalath: '%" PRIu64 "'\nregister heh: '%" PRIu64 "'\nregister waw: '%" PRIu64 "'\nregister zain: '%" PRIu64 "'\nregister heth: '%" PRIu64 "'\nregister teth: '%" PRIu64 "'\nregister yodh: '%" PRIu64 "'\nregister kaf: '%" PRIu64 "'\nregister lamadh: '%" PRIu64 "'\nregister meem: '%" PRIu64 "'\nregister noon: '%" PRIu64 "'\nregister semkath: '%" PRIu64 "'\nregister eh: '%" PRIu64 "'\nregister peh: '%" PRIu64 "'\nregister sadhe: '%" PRIu64 "'\nregister qof: '%" PRIu64 "'\nregister reesh: '%" PRIu64 "'\nregister sheen: '%" PRIu64 "'\nregister taw: '%" PRIu64 "'\nregister stack pointer: '%p'\nregister base pointer: '%p'\nregister instruction pointer: '%p'\n\nPrinting Condition Flag: %u\n=== End Tagha VM State Info ===\n",
	vm->regAlaf.UInt64,
	vm->regBeth.UInt64,
	vm->regGamal.UInt64,
	vm->regDalath.UInt64,
	vm->regHeh.UInt64,
	vm->regWaw.UInt64,
	vm->regZain.UInt64,
	vm->regHeth.UInt64,
	vm->regTeth.UInt64,
	vm->regYodh.UInt64,
	vm->regKaf.UInt64,
	vm->regLamadh.UInt64,
	vm->regMeem.UInt64,
	vm->regNoon.UInt64,
	vm->regSemkath.UInt64,
	vm->reg_Eh.UInt64,
	vm->regPeh.UInt64,
	vm->regSadhe.UInt64,
	vm->regQof.UInt64,
	vm->regReesh.UInt64,
	vm->regSheen.UInt64,
	vm->regTaw.UInt64,
	vm->regStk.Ptr,
	vm->regBase.Ptr,
	vm->regInstr.Ptr,
	vm->CondFlag);
}

bool Tagha_RegisterNatives(struct Tagha *const restrict vm, const struct NativeInfo natives[])
{
	if( !vm || !vm->Header || !natives )
		return false;
	
	for( const struct NativeInfo *restrict n=natives ; n->NativeCFunc && n->Name ; n++ ) {
		const union TaghaVal func_addr = (union TaghaVal){.Ptr = GetFunctionOffsetByName(vm->Header, n->Name)};
		if( func_addr.Ptr )
			func_addr.SelfPtr->VoidFunc = n->NativeCFunc;
	}
	return true;
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

/*
#include <sys/mman.h>
int32_t Tagha_JITCompile(struct Tagha *const restrict vm)
{
	void *mem = mmap(NULL, memsize, PROT_READ | PROT_WRITE,
MAP_ANON | MAP_PRIVATE, -1, 0);
	if( !mem ) {
		vm->Error = ErrBadPtr;
		return -1;
	}
	
	mprotect(mem, memsize, PROT_READ | PROT_EXEC);
}
*/

//#include <unistd.h>	// sleep() func

int32_t Tagha_Exec(struct Tagha *const restrict vm)
{
	if( !vm )
		return -1;
	
	register union TaghaPtr pc = (union TaghaPtr){.UInt8Ptr = vm->regInstr.UInt8Ptr};
	
	register union {
		uint16_t opcode;
		struct { uint8_t instr, addrmode; };
	} decode;
	
#define X(x) #x ,
	/* for debugging purposes. */
	//const char *const restrict opcode2str[] = { INSTR_SET };
#undef X
	
#define X(x) &&exec_##x ,
	/* our instruction dispatch table. */
	static const void *const restrict dispatch[] = { INSTR_SET };
#undef X
#undef INSTR_SET
	/* #ifdef _UNISTD_H */
	
	#define DEBUGDISPATCH() \
		decode.opcode = *pc.UInt16Ptr++; \
		\
		if( decode.instr>nop ) { \
			vm->Error = ErrInstrBounds; \
			return -1; \
		} \
		\
		/*usleep(100);*/ \
		/*printf("dispatching to '%s'\n", opcode2str[decode.instr]);*/ \
		/*Tagha_PrintVMState(vm);*/ \
		goto *dispatch[decode.instr]
	
	#define DISPATCH() goto *dispatch[decode.opcode = *pc.UInt16Ptr++, decode.instr]
	
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
			*--vm->regStk.SelfPtr = *pc.ValPtr++;
			DISPATCH();
		}
		/* push a register's contents. */
		else if( decode.addrmode & Register ) {
			*--vm->regStk.SelfPtr = vm->Regs[*pc.UInt8Ptr++];
			DISPATCH();
		}
		/* push the contents of a memory address inside a register. */
		else if( decode.addrmode & RegIndirect ) {
			const uint8_t regid = *pc.UInt8Ptr++;
			const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *pc.Int32Ptr++};
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
			vm->Regs[*pc.UInt8Ptr++] = *vm->regStk.SelfPtr++;
			DISPATCH();
		}
		else if( decode.addrmode & RegIndirect ) {
			const uint8_t regid = *pc.UInt8Ptr++;
			const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *pc.Int32Ptr++};
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
			const uint8_t regid = *pc.UInt8Ptr++;
			vm->Regs[regid].Ptr = GetVariableOffsetByIndex(vm->Header, *pc.UInt64Ptr++);
			DISPATCH();
		}
		else if( decode.addrmode & Register ) { /* Register mode will load a function address which could be a native */
			const uint8_t regid = *pc.UInt8Ptr++;
			vm->Regs[regid].Int64 = *pc.Int64Ptr++;
			DISPATCH();
		}
		else if( decode.addrmode & RegIndirect ) {
			const uint16_t regids = *pc.UInt16Ptr++;
			vm->Regs[regids & 255].UInt8Ptr = vm->Regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++;
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
				const uint8_t regid = *pc.UInt8Ptr++;
				vm->Regs[regid] = *pc.ValPtr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				vm->Regs[regids & 255] = vm->Regs[regids >> 8];
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				vm->Regs[regid].UInt64 += *pc.UInt64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				vm->Regs[regids & 255].UInt64 += vm->Regs[regids >> 8].UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				vm->Regs[regid].UInt64 -= *pc.UInt64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				vm->Regs[regids & 255].UInt64 -= vm->Regs[regids >> 8].UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				vm->Regs[regid].UInt64 *= *pc.UInt64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				vm->Regs[regids & 255].UInt64 *= vm->Regs[regids >> 8].UInt64;
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				vm->Regs[regid].UInt64 /= *pc.UInt64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				vm->Regs[regids & 255].UInt64 /= vm->Regs[regids >> 8].UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				vm->Regs[regid].UInt64 %= *pc.UInt64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				vm->Regs[regids & 255].UInt64 %= vm->Regs[regids >> 8].UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				vm->Regs[regid].UInt64 &= *pc.UInt64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				vm->Regs[regids & 255].UInt64 &= vm->Regs[regids >> 8].UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				vm->Regs[regid].UInt64 |= *pc.UInt64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				vm->Regs[regids & 255].UInt64 |= vm->Regs[regids >> 8].UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				vm->Regs[regid].UInt64 ^= *pc.UInt64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				vm->Regs[regids & 255].UInt64 ^= vm->Regs[regids >> 8].UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++};
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
		const uint8_t regid = *pc.UInt8Ptr++;
		if( decode.addrmode & Register ) {
			vm->Regs[regid].UInt64 = ~vm->Regs[regid].UInt64;
			DISPATCH();
		}
		else if( decode.addrmode & RegIndirect ) {
			const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				vm->Regs[regid].UInt64 <<= *pc.UInt64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				vm->Regs[regids & 255].UInt64 <<= vm->Regs[regids >> 8].UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				vm->Regs[regid].UInt64 >>= *pc.UInt64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				vm->Regs[regids & 255].UInt64 >>= vm->Regs[regids >> 8].UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++};
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
		const uint8_t regid = *pc.UInt8Ptr++;
		if( decode.addrmode & Register ) {
			++vm->Regs[regid].UInt64;
			DISPATCH();
		}
		else if( decode.addrmode & RegIndirect ) {
			const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *pc.Int32Ptr++};
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
		const uint8_t regid = *pc.UInt8Ptr++;
		if( decode.addrmode & Register ) {
			--vm->Regs[regid].UInt64;
			DISPATCH();
		}
		else if( decode.addrmode & RegIndirect ) {
			const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *pc.Int32Ptr++};
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
		const uint8_t regid = *pc.UInt8Ptr++;
		if( decode.addrmode & Register ) {
			vm->Regs[regid].UInt64 = -vm->Regs[regid].UInt64;
			DISPATCH();
		}
		else if( decode.addrmode & RegIndirect ) {
			const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				vm->CondFlag = vm->Regs[regid].Int64 < *pc.Int64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				vm->CondFlag = vm->Regs[regids & 255].Int64 < vm->Regs[regids >> 8].Int64;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				vm->CondFlag = vm->Regs[regid].Int64 > *pc.Int64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				vm->CondFlag = vm->Regs[regids & 255].Int64 > vm->Regs[regids >> 8].Int64;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				vm->CondFlag = vm->Regs[regid].UInt64 < *pc.UInt64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				vm->CondFlag = vm->Regs[regids & 255].UInt64 < vm->Regs[regids >> 8].UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				vm->CondFlag = vm->Regs[regid].UInt64 > *pc.UInt64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				vm->CondFlag = vm->Regs[regids & 255].UInt64 > vm->Regs[regids >> 8].UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				vm->CondFlag = vm->Regs[regid].UInt64 == *pc.UInt64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				vm->CondFlag = vm->Regs[regids & 255].UInt64 == vm->Regs[regids >> 8].UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				vm->CondFlag = vm->Regs[regid].UInt64 != *pc.UInt64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				vm->CondFlag = vm->Regs[regids & 255].UInt64 != vm->Regs[regids >> 8].UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++};
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
			const int64_t offset = *pc.Int64Ptr++;
			pc.UInt8Ptr += offset;
			DISPATCH();
		}
		else if( decode.addrmode & Register ) {
			const uint8_t regid = *pc.UInt8Ptr++;
			pc.UInt8Ptr += vm->Regs[regid].Int64;
			DISPATCH();
		}
		else if( decode.addrmode & RegIndirect ) {
			const uint8_t regid = *pc.UInt8Ptr++;
			const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *pc.Int32Ptr++};
			switch( decode.addrmode & 0xf0 ) {
				case Byte:
					pc.UInt8Ptr += *mem.Int8Ptr;
					DISPATCH();
				case TwoBytes:
					pc.UInt8Ptr += *mem.Int16Ptr;
					DISPATCH();
				case FourBytes:
					pc.UInt8Ptr += *mem.Int32Ptr;
					DISPATCH();
				case EightBytes:
					pc.UInt8Ptr += *mem.Int64Ptr;
					DISPATCH();
			}
		}
	}
	exec_jz: {
		if( decode.addrmode & Immediate ) {
			const int64_t offset = *pc.Int64Ptr++;
			!vm->CondFlag ? (pc.UInt8Ptr += offset) : (void)pc;
			DISPATCH();
		}
		else if( decode.addrmode & Register ) {
			const uint8_t regid = *pc.UInt8Ptr++;
			!vm->CondFlag ? (pc.UInt8Ptr += vm->Regs[regid].Int64) : (void)pc;
			DISPATCH();
		}
		else if( decode.addrmode & RegIndirect ) {
			const uint8_t regid = *pc.UInt8Ptr++;
			const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *pc.Int32Ptr++};
			switch( decode.addrmode & 0xf0 ) {
				case Byte:
					!vm->CondFlag ? (pc.UInt8Ptr += *mem.Int8Ptr) : (void)pc;
					DISPATCH();
				case TwoBytes:
					!vm->CondFlag ? (pc.UInt8Ptr += *mem.Int16Ptr) : (void)pc;
					DISPATCH();
				case FourBytes:
					!vm->CondFlag ? (pc.UInt8Ptr += *mem.Int32Ptr) : (void)pc;
					DISPATCH();
				case EightBytes:
					!vm->CondFlag ? (pc.UInt8Ptr += *mem.Int64Ptr) : (void)pc;
					DISPATCH();
			}
		}
	}
	exec_jnz: {
		if( decode.addrmode & Immediate ) {
			const int64_t offset = *pc.Int64Ptr++;
			vm->CondFlag ? (pc.UInt8Ptr += offset) : (void)pc;
			DISPATCH();
		}
		else if( decode.addrmode & Register ) {
			const uint8_t regid = *pc.UInt8Ptr++;
			vm->CondFlag ? (pc.UInt8Ptr += vm->Regs[regid].Int64) : (void)pc;
			DISPATCH();
		}
		else if( decode.addrmode & RegIndirect ) {
			const uint8_t regid = *pc.UInt8Ptr++;
			const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *pc.Int32Ptr++};
			switch( decode.addrmode & 0xf0 ) {
				case Byte:
					vm->CondFlag ? (pc.UInt8Ptr += *mem.Int8Ptr) : (void)pc;
					DISPATCH();
				case TwoBytes:
					vm->CondFlag ? (pc.UInt8Ptr += *mem.Int16Ptr) : (void)pc;
					DISPATCH();
				case FourBytes:
					vm->CondFlag ? (pc.UInt8Ptr += *mem.Int32Ptr) : (void)pc;
					DISPATCH();
				case EightBytes:
					vm->CondFlag ? (pc.UInt8Ptr += *mem.Int64Ptr) : (void)pc;
					DISPATCH();
			}
		}
		DISPATCH();
	}
	exec_call: {
		uint64_t index = -1;
		if( decode.addrmode & Immediate ) {
			index = ((*pc.UInt64Ptr++) - 1);
		}
		else if( decode.addrmode & Register ) {
			index = (vm->Regs[*pc.UInt8Ptr++].UInt64 - 1);
		}
		else if( decode.addrmode & RegIndirect ) {
			const uint8_t regid = *pc.UInt8Ptr++;
			const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *pc.Int32Ptr++};
			index = (*mem.UInt64Ptr - 1);
		}
		
		uint8_t *const call_addr = GetFunctionOffsetByIndex(vm->Header, index);
		if( !call_addr ) {
			vm->Error = ErrMissingFunc;
			return -1;
		}
		/* The restrict type qualifier is an indication to the compiler that,
		 * if the memory addressed by the restrict-qualified pointer is modified,
		 * no other pointer will access that same memory.
		 * Since we're pushing the restrict-qualified pointer's memory that it points to,
		 * This is NOT undefined behavior because it's not aliasing access of the instruction stream.
		 */
		(--vm->regStk.SelfPtr)->Ptr = pc.Ptr;	/* push rip */
		*--vm->regStk.SelfPtr = vm->regBase;	/* push rbp */
		vm->regBase = vm->regStk;	/* mov rbp, rsp */
		pc.UInt8Ptr = call_addr;
		DISPATCH();
	}
	exec_ret: {
		vm->regStk = vm->regBase; /* mov rsp, rbp */
		vm->regBase = *vm->regStk.SelfPtr++; /* pop rbp */
		pc.Ptr = (*vm->regStk.SelfPtr++).Ptr; /* pop rip */
		if( !pc.Ptr ) {
			goto *dispatch[halt];
		}
		else { DISPATCH(); }
	}
	
	exec_syscall: {
		/* how many args given to the native call. */
		//const uint32_t argcount = *pc.UInt32Ptr++;
		// syscall args size moved to alaf register.
		uint64_t index = -1;
		/* trying to directly call a specific native. Allow this by imm only! */
		if( decode.addrmode & Immediate ) {
			index = (-1 - *pc.Int64Ptr++);
		}
		else if( decode.addrmode & Register ) {
			index = (-1 - vm->Regs[*pc.UInt8Ptr++].Int64);
		}
		else if( decode.addrmode & RegIndirect ) {
			const uint8_t regid = *pc.UInt8Ptr++;
			const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *pc.Int32Ptr++};
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
			const uint8_t regid = *pc.UInt8Ptr++;
			const float f = vm->Regs[regid].Float;
			vm->Regs[regid].Double = (double)f;
			DISPATCH();
		}
	}
	exec_dbl2flt: {
		if( decode.addrmode & Register ) {
			const uint8_t regid = *pc.UInt8Ptr++;
			const double d = vm->Regs[regid].Double;
			vm->Regs[regid].UInt64 = 0;
			vm->Regs[regid].Float = (float)d;
			DISPATCH();
		}
	}
	exec_int2dbl: {
		if( decode.addrmode & Register ) {
			const uint8_t regid = *pc.UInt8Ptr++;
			const uint64_t i = vm->Regs[regid].UInt64;
			vm->Regs[regid].Double = (double)i;
			DISPATCH();
		}
	}
	exec_int2flt: {
		if( decode.addrmode & Register ) {
			const uint8_t regid = *pc.UInt8Ptr++;
			const uint64_t i = vm->Regs[regid].UInt64;
			vm->Regs[regid].UInt64 = 0;
			vm->Regs[regid].Float = (float)i;
			DISPATCH();
		}
	}
	exec_addf: {
		if( decode.addrmode & UseReg ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				if( decode.addrmode & FourBytes )
					vm->Regs[regid].Float += imm.Float;
				else if( decode.addrmode & EightBytes )
					vm->Regs[regid].Double += imm.Double;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				if( decode.addrmode & FourBytes )
					vm->Regs[regids & 255].Float += vm->Regs[regids >> 8].Float;
				else if( decode.addrmode & EightBytes )
					vm->Regs[regids & 255].Double += vm->Regs[regids >> 8].Double;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				if( decode.addrmode & FourBytes )
					vm->Regs[regid].Float -= imm.Float;
				else if( decode.addrmode & EightBytes )
					vm->Regs[regid].Double -= imm.Double;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				if( decode.addrmode & FourBytes )
					vm->Regs[regids & 255].Float -= vm->Regs[regids >> 8].Float;
				else if( decode.addrmode & EightBytes )
					vm->Regs[regids & 255].Double -= vm->Regs[regids >> 8].Double;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				if( decode.addrmode & FourBytes )
					vm->Regs[regid].Float *= imm.Float;
				else if( decode.addrmode & EightBytes )
					vm->Regs[regid].Double *= imm.Double;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				if( decode.addrmode & FourBytes )
					vm->Regs[regids & 255].Float *= vm->Regs[regids >> 8].Float;
				else if( decode.addrmode & EightBytes )
					vm->Regs[regids & 255].Double *= vm->Regs[regids >> 8].Double;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				if( decode.addrmode & FourBytes )
					vm->Regs[regid].Float /= imm.Float;
				else if( decode.addrmode & EightBytes )
					vm->Regs[regid].Double /= imm.Double;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				if( decode.addrmode & FourBytes )
					vm->Regs[regids & 255].Float /= vm->Regs[regids >> 8].Float;
				else if( decode.addrmode & EightBytes )
					vm->Regs[regids & 255].Double /= vm->Regs[regids >> 8].Double;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				if( decode.addrmode & FourBytes )
					vm->CondFlag = vm->Regs[regid].Float < imm.Float;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = vm->Regs[regid].Double < imm.Double;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				if( decode.addrmode & FourBytes )
					vm->CondFlag = vm->Regs[regids & 255].Float < vm->Regs[regids >> 8].Float;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = vm->Regs[regids & 255].Double < vm->Regs[regids >> 8].Double;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				if( decode.addrmode & FourBytes )
					vm->CondFlag = vm->Regs[regid].Float > imm.Float;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = vm->Regs[regid].Double > imm.Double;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				if( decode.addrmode & FourBytes )
					vm->CondFlag = vm->Regs[regids & 255].Float > vm->Regs[regids >> 8].Float;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = vm->Regs[regids & 255].Double > vm->Regs[regids >> 8].Double;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				if( decode.addrmode & FourBytes )
					vm->CondFlag = vm->Regs[regid].Float == imm.Float;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = vm->Regs[regid].Double == imm.Double;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				if( decode.addrmode & FourBytes )
					vm->CondFlag = vm->Regs[regids & 255].Float == vm->Regs[regids >> 8].Float;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = vm->Regs[regids & 255].Double == vm->Regs[regids >> 8].Double;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				if( decode.addrmode & FourBytes )
					vm->CondFlag = vm->Regs[regid].Float != imm.Float;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = vm->Regs[regid].Double != imm.Double;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				if( decode.addrmode & FourBytes )
					vm->CondFlag = vm->Regs[regids & 255].Float != vm->Regs[regids >> 8].Float;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = vm->Regs[regids & 255].Double != vm->Regs[regids >> 8].Double;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *pc.Int32Ptr++};
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
				const uint16_t regids = *pc.UInt16Ptr++;
				const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++};
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
		const uint8_t regid = *pc.UInt8Ptr++;
		if( decode.addrmode & Register ) {
			if( decode.addrmode & FourBytes )
				++vm->Regs[regid].Float;
			else if( decode.addrmode & EightBytes )
				++vm->Regs[regid].Double;
			DISPATCH();
		}
		else if( decode.addrmode & RegIndirect ) {
			const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *pc.Int32Ptr++};
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
		const uint8_t regid = *pc.UInt8Ptr++;
		if( decode.addrmode & Register ) {
			if( decode.addrmode & FourBytes )
				--vm->Regs[regid].Float;
			else if( decode.addrmode & EightBytes )
				--vm->Regs[regid].Double;
			DISPATCH();
		}
		else if( decode.addrmode & RegIndirect ) {
			const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *pc.Int32Ptr++};
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
		const uint8_t regid = *pc.UInt8Ptr++;
		if( decode.addrmode & Register ) {
			if( decode.addrmode & FourBytes )
				vm->Regs[regid].Float = -vm->Regs[regid].Float;
			else if( decode.addrmode & EightBytes )
				vm->Regs[regid].Double = -vm->Regs[regid].Double;
			DISPATCH();
		}
		else if( decode.addrmode & RegIndirect ) {
			const union TaghaPtr mem = (union TaghaPtr){.Ptr = vm->Regs[regid].UInt8Ptr + *pc.Int32Ptr++};
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

int32_t Tagha_RunScript(struct Tagha *const restrict vm, const int32_t argc, char *argv[restrict static argc+1])
{
	if( !vm || !vm->Header )
		return -1;
	
	else if( vm->Header->Magic != 0xC0DE ) {
		vm->Error = ErrInvalidScript;
		return -1;
	}
	
	/* push argc, argv to registers. */
	union TaghaVal MainArgs[argc+1];
	MainArgs[argc].Ptr = NULL;
	if( argv )
		for( int32_t i=0 ; i<argc ; i++ )
			MainArgs[i].Ptr = argv[i];
	vm->reg_Eh.Ptr = MainArgs;
	vm->regSemkath.Int32 = argc;
	
	/* check out stack size and align it by the size of union TaghaVal. */
	const size_t stacksize = vm->Header->StackSize; //(vm->Header->StackSize + (sizeof(union TaghaVal)-1)) & -(sizeof(union TaghaVal));
	if( !stacksize ) {
		vm->Error = ErrStackSize;
		return -1;
	}
	
	//union TaghaVal Stack[stacksize+1]; memset(Stack, 0, sizeof Stack[0] * stacksize+1);
	//vm->regStk.SelfPtr = vm->regBase.SelfPtr = Stack + stacksize;
	vm->regStk.SelfPtr = vm->regBase.SelfPtr = &((union TaghaVal *)vm->Header)[vm->Header->StackOffs + stacksize - 1];
	
	(--vm->regStk.SelfPtr)->Int64 = 0LL;	/* push NULL return address. */
	*--vm->regStk.SelfPtr = vm->regBase; /* push rbp */
	vm->regBase = vm->regStk; /* mov rbp, rsp */
	vm->regInstr.Ptr = GetFunctionOffsetByName(vm->Header, "main");
	if( !vm->regInstr.Ptr ) {
		vm->Error = ErrMissingFunc;
		return -1;
	}
	else return Tagha_Exec(vm);
}

int32_t Tagha_CallFunc(struct Tagha *const restrict vm, const char *restrict funcname, const size_t args, union TaghaVal values[static args])
{
	if( !vm || !vm->Header || !funcname || !values )
		return -1;
	else if( vm->Header->Magic != 0xC0DE ) {
		vm->Error = ErrInvalidScript;
		return -1;
	}
	
	/* check out stack size && align it by the size of union TaghaVal. */
	const size_t stacksize = vm->Header->StackSize; //(vm->Header->StackSize + (sizeof(union TaghaVal)-1)) & -(sizeof(union TaghaVal));
	if( !stacksize ) {
		vm->Error = ErrStackSize;
		return -1;
	}
	
	//union TaghaVal Stack[stacksize+1]; memset(Stack, 0, sizeof Stack[0] * stacksize+1);
	//vm->regStk.SelfPtr = vm->regBase.SelfPtr = Stack + stacksize;
	vm->regStk.SelfPtr = vm->regBase.SelfPtr = &((union TaghaVal *)vm->Header)[vm->Header->StackOffs + stacksize - 1];
	
	/* remember that arguments must be passed right to left.
	 we have enough args to fit in registers. */
	const uint8_t reg_params = 8;
	const uint8_t reg_param_initial = regSemkath;
	const size_t bytecount = sizeof(union TaghaVal) * args;
	
	/* save stack space by using the registers for passing arguments. */
	/* the other registers can be used for other data ops. */
	if( args <= reg_params ) {
		memcpy(vm->Regs+reg_param_initial, values, bytecount);
	}
	/* if the function has more than a certain num of params, push from both registers && stack. */
	else {
		memcpy(vm->Regs+reg_param_initial, values, sizeof(union TaghaVal) * reg_params);
		memcpy(vm->regStk.SelfPtr, values+reg_params, sizeof(union TaghaVal) * (args-reg_params));
		vm->regStk.SelfPtr -= (args-reg_params);
	}
	
	(--vm->regStk.SelfPtr)->Int64 = 0LL;	/* push NULL return address. */
	*--vm->regStk.SelfPtr = vm->regBase; /* push rbp */
	vm->regBase = vm->regStk; /* mov rbp, rsp */
	vm->regInstr.Ptr = GetFunctionOffsetByName(vm->Header, funcname);
	if( !vm->regInstr.Ptr ) {
		vm->Error = ErrMissingFunc;
		return -1;
	}
	else return Tagha_Exec(vm);
}

union TaghaVal Tagha_GetReturnValue(const struct Tagha *const vm)
{
	return vm ? vm->regAlaf : (union TaghaVal){0};
}

void *Tagha_GetGlobalVarByName(struct Tagha *const restrict vm, const char *restrict varname)
{
	return !vm || !vm->Header || !varname ? NULL : GetVariableOffsetByName(vm->Header, varname);
}

const char *Tagha_GetError(const struct Tagha *const restrict vm)
{
	if( !vm )
		return "Null VM Pointer";
	
	switch( vm->Error ) {
		case ErrInstrBounds: return "Out of Bound Instruction";
		case ErrNone: return "None";
		case ErrBadPtr: return "Null || Invalid Pointer";
		case ErrMissingFunc: return "Missing Function";
		case ErrInvalidScript: return "Null || Invalid Script";
		case ErrStackSize: return "Bad Stack Size given";
		case ErrMissingNative: return "Missing Native";
		default: return "Unknown Error";
	}
}

void *Tagha_GetRawScriptPtr(const struct Tagha *const restrict vm)
{
	return !vm ? NULL : vm->Header;
}

/************************************************/
