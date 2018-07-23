
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "tagha.h"

inline static void			*GetFunctionOffsetByName(uint8_t *, const char *);
inline static void			*GetFunctionOffsetByIndex(uint8_t *, size_t);

inline static void			*GetVariableOffsetByName(uint8_t *, const char *);
inline static void			*GetVariableOffsetByIndex(uint8_t *, size_t);

static void PrepModule(uint8_t *const module)
{
	if( !module )
		return;
	
	FILE **restrict fileptr = GetVariableOffsetByName(module, "stdin");
	if( fileptr )
		*fileptr = stdin;
	
	fileptr = GetVariableOffsetByName(module, "stderr");
	if( fileptr )
		*fileptr = stderr;
	
	fileptr = GetVariableOffsetByName(module, "stdout");
	if( fileptr )
		*fileptr = stdout;
}

static void InvokeNative(struct Tagha *const restrict vm, union Value *const restrict regs, const size_t argcount, void (*const NativeCall)())
{
	const uint8_t reg_params = 8;
	const uint8_t first_param_register = regSemkath;
	
	/* save stack space by using the registers for passing arguments.
	 * the other registers can then be used for other data operations.
	 */
	union Value params[argcount];
	// copy native params from registers first.
	memcpy(params, regs+first_param_register, sizeof params[0] * reg_params);
	// now copy the remaining params off the stack and pop them.
	memcpy(params+reg_params, regs[regStk].SelfPtr, sizeof params[0] * (argcount-reg_params));
	regs[regStk].SelfPtr += (argcount-reg_params);
	// invoke!
	(*NativeCall)(vm, regs+regAlaf, argcount, params);
}



void Tagha_Init(struct Tagha *const restrict vm, void *script)
{
	if( !vm )
		return;
	
	*vm = (struct Tagha){0};
	vm->CurrScript.Ptr = script;
	PrepModule(script);
}

void Tagha_InitN(struct Tagha *const restrict vm, void *script, const struct NativeInfo natives[])
{
	Tagha_Init(vm, script);
	Tagha_RegisterNatives(vm, natives);
}

void Tagha_PrintVMState(const struct Tagha *const restrict vm)
{
	if( !vm )
		return;
	const union Value *const restrict regs = vm->Regs;
	
	printf("=== Tagha VM State Info ===\n\nPrinting Registers:\nregister alaf: '%" PRIu64 "'\nregister beth: '%" PRIu64 "'\nregister gamal: '%" PRIu64 "'\nregister dalath: '%" PRIu64 "'\nregister heh: '%" PRIu64 "'\nregister waw: '%" PRIu64 "'\nregister zain: '%" PRIu64 "'\nregister heth: '%" PRIu64 "'\nregister teth: '%" PRIu64 "'\nregister yodh: '%" PRIu64 "'\nregister kaf: '%" PRIu64 "'\nregister lamadh: '%" PRIu64 "'\nregister meem: '%" PRIu64 "'\nregister noon: '%" PRIu64 "'\nregister semkath: '%" PRIu64 "'\nregister eh: '%" PRIu64 "'\nregister peh: '%" PRIu64 "'\nregister sadhe: '%" PRIu64 "'\nregister qof: '%" PRIu64 "'\nregister reesh: '%" PRIu64 "'\nregister sheen: '%" PRIu64 "'\nregister taw: '%" PRIu64 "'\nregister stack pointer: '%p'\nregister base pointer: '%p'\nregister instruction pointer: '%p'\n\nPrinting Condition Flag: %u\n=== End Tagha VM State Info ===\n",
	regs[regAlaf].UInt64,
	regs[regBeth].UInt64,
	regs[regGamal].UInt64,
	regs[regDalath].UInt64,
	regs[regHeh].UInt64,
	regs[regWaw].UInt64,
	regs[regZain].UInt64,
	regs[regHeth].UInt64,
	regs[regTeth].UInt64,
	regs[regYodh].UInt64,
	regs[regKaf].UInt64,
	regs[regLamadh].UInt64,
	regs[regMeem].UInt64,
	regs[regNoon].UInt64,
	regs[regSemkath].UInt64,
	regs[reg_Eh].UInt64,
	regs[regPeh].UInt64,
	regs[regSadhe].UInt64,
	regs[regQof].UInt64,
	regs[regReesh].UInt64,
	regs[regSheen].UInt64,
	regs[regTaw].UInt64,
	regs[regStk].Ptr,
	regs[regBase].Ptr,
	regs[regInstr].Ptr,
	vm->CondFlag);
}

bool Tagha_RegisterNatives(struct Tagha *const restrict vm, const struct NativeInfo natives[])
{
	if( !vm or !natives )
		return false;
	
	union Value func_addr = (union Value){0};
	for( const struct NativeInfo *restrict n=natives ; n->NativeCFunc and n->Name ; n++ ) {
		func_addr.Ptr = GetFunctionOffsetByName(vm->CurrScript.Ptr, n->Name);
		if( func_addr.Ptr )
			func_addr.SelfPtr->VoidFunc = n->NativeCFunc;
	}
	return true;
}

inline static void *GetFunctionOffsetByName(uint8_t *const script, const char *restrict funcname)
{
	if( !funcname or !script )
		return NULL;
	
	union Pointer reader = (union Pointer){.Ptr = script + 11};
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
	return NULL;
}

inline static void *GetFunctionOffsetByIndex(uint8_t *const script, const size_t index)
{
	if( !script )
		return NULL;
	
	union Pointer reader = (union Pointer){.Ptr = script + 11};
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
	return NULL;
}

inline static void *GetVariableOffsetByName(uint8_t *const script, const char *restrict varname)
{
	if( !script or !varname )
		return NULL;
	
	union Pointer reader = (union Pointer){.Ptr = script + 7};
	const uint32_t vartable_offset = *reader.UInt32Ptr++;
	reader.UInt8Ptr += vartable_offset;
	
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
	return NULL;
}

inline static void *GetVariableOffsetByIndex(uint8_t *const script, const size_t index)
{
	if( !script )
		return NULL;
	
	union Pointer reader = (union Pointer){.Ptr = script + 7};
	const uint32_t vartable_offset = *reader.UInt32Ptr++;
	reader.UInt8Ptr += vartable_offset;
	
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
	return NULL;
}

//#include <unistd.h>	// sleep() func

int32_t Tagha_Exec(struct Tagha *const restrict vm)
{
	if( !vm or !vm->Module ) {
		return ErrInstrBounds;
	}
	
	union Value *const restrict regs = vm->Regs;
	union Pointer pc = (union Pointer){.UInt8Ptr = regs[regInstr].UCharPtr};
	const union Value *const restrict MainBasePtr = regs[regBase].SelfPtr;
	regs[regBase] = regs[regStk];
	
	uint8_t instr=0, addrmode=0;
	uint16_t opcode = 0;
	
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
	
	#define DISPATCH() \
		opcode = *pc.UInt16Ptr++; \
		\
		/* get the instruction from the first byte. */ \
		instr = opcode & 255; \
		\
		/* get addressing mode from second byte. */ \
		addrmode = opcode >> 8; \
		\
		if( instr>nop ) { \
			return ErrInstrBounds; \
		} \
		\
		/*usleep(100); */\
		/*printf("dispatching to '%s'\n", opcode2str[instr]);*/ \
		/*Tagha_PrintVMState(vm);*/ \
		goto *dispatch[instr]
	
	DISPATCH();
	
	exec_halt:; {
		//TaghaDebug_PrintRegisters(vm);
		return regs[regAlaf].Int32;
	}
	exec_nop:; {
		DISPATCH();
	}
	
	/* pushes a value to the top of the stack, raises the stack pointer by 8 bytes.
	 * push reg (1 byte for register id)
	 * push imm (8 bytes for constant values)
	 * push [reg+offset] (1 byte reg id + 4-byte signed offset)
	 */
	exec_push:; {
		/* push an imm constant. */
		if( addrmode & Immediate )
			*--regs[regStk].SelfPtr = *pc.ValPtr++;
		/* push a register's contents. */
		else if( addrmode & Register )
			*--regs[regStk].SelfPtr = regs[*pc.UInt8Ptr++];
		/* push the contents of a memory address inside a register. */
		else if( addrmode & RegIndirect ) {
			const uint8_t regid = *pc.UInt8Ptr++;
			const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regid].UCharPtr + *pc.Int32Ptr++};
			*--regs[regStk].SelfPtr = *address_ptr.ValPtr;
		}
		DISPATCH();
	}
	
	/* pops a value from the stack into a register or memory then reduces stack by 8 bytes.
	 * pop reg
	 * pop [reg+offset]
	 */
	exec_pop:; {
		if( addrmode & Immediate )
			pc.ValPtr++;
		else if( addrmode & Register )
			regs[*pc.UInt8Ptr++] = *regs[regStk].SelfPtr++;
		else if( addrmode & RegIndirect ) {
			const uint8_t regid = *pc.UInt8Ptr++;
			const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regid].UCharPtr + *pc.Int32Ptr++};
			*address_ptr.ValPtr = *regs[regStk].SelfPtr++;
		}
		DISPATCH();
	}
	
	/* loads a ptr value to a register.
	 * lea reg, global var
	 * lea reg, func
	 * lea reg, [reg+offset] (not dereferenced)
	 */
	exec_lea:; {
		const uint8_t regid = *pc.UInt8Ptr++;
		if( addrmode & Immediate ) { /* Immediate mode will load a global value */
			regs[regid].Ptr = GetVariableOffsetByIndex(vm->CurrScript.Ptr, *pc.UInt64Ptr++);
		}
		else if( addrmode & Register ) { /* Register mode will load a function address which could be a native */
			regs[regid].Int64 = *pc.Int64Ptr++;
		}
		else if( addrmode & RegIndirect ) {
			const uint8_t sec_regid = *pc.UInt8Ptr++;
			regs[regid].UCharPtr = regs[sec_regid].UCharPtr + *pc.Int32Ptr++;
		}
		DISPATCH();
	}
	
	/* copies a value to a register or memory address.
	 * mov reg, [reg+offset]
	 * mov reg, imm
	 * mov reg, reg
	 * mov [reg+offset], imm
	 * mov [reg+offset], reg
	 */
	exec_mov:; {
		if( addrmode & Reserved ) {
			if( addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				regs[regid] = *pc.ValPtr++;
			}
			else if( addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				regs[regids & 255] = regs[regids >> 8];
			}
			else if( addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regids >> 8].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & Byte )
					regs[regids & 255].UChar = *address_ptr.UInt8Ptr;
				else if( addrmode & TwoBytes )
					regs[regids & 255].UShort = *address_ptr.UInt16Ptr;
				else if( addrmode & FourBytes )
					regs[regids & 255].UInt32 = *address_ptr.UInt32Ptr;
				else if( addrmode & EightBytes )
					regs[regids & 255].UInt64 = *address_ptr.UInt64Ptr;
			}
		}
		else {
			if( addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union Value imm = *pc.ValPtr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regid].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & Byte )
					*address_ptr.UInt8Ptr = imm.UChar;
				else if( addrmode & TwoBytes )
					*address_ptr.UInt16Ptr = imm.UShort;
				else if( addrmode & FourBytes )
					*address_ptr.UInt32Ptr = imm.UInt32;
				else if( addrmode & EightBytes )
					*address_ptr.UInt64Ptr = imm.UInt64;
			}
			else if( addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regids & 255].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & Byte )
					*address_ptr.UInt8Ptr = regs[regids >> 8].UChar;
				else if( addrmode & TwoBytes )
					*address_ptr.UInt16Ptr = regs[regids >> 8].UShort;
				else if( addrmode & FourBytes )
					*address_ptr.UInt32Ptr = regs[regids >> 8].UInt32;
				else if( addrmode & EightBytes )
					*address_ptr.UInt64Ptr = regs[regids >> 8].UInt64;
			}
		}
		DISPATCH();
	}
	
	/* adds two values to the destination value which is either a register or memory address.
	 * add reg, [reg+offset]
	 * add reg, imm
	 * add reg, reg
	 * add [reg+offset], reg
	 * add [reg+offset], imm
	 */
	exec_add:; {
		if( addrmode & Reserved ) {
			if( addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				regs[regid].UInt64 += *pc.UInt64Ptr++;
			}
			else if( addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				regs[regids & 255].UInt64 += regs[regids >> 8].UInt64;
			}
			else if( addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regids >> 8].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & Byte )
					regs[regids & 255].UChar += *address_ptr.UInt8Ptr;
				else if( addrmode & TwoBytes )
					regs[regids & 255].UShort += *address_ptr.UInt16Ptr;
				else if( addrmode & FourBytes )
					regs[regids & 255].UInt32 += *address_ptr.UInt32Ptr;
				else if( addrmode & EightBytes )
					regs[regids & 255].UInt64 += *address_ptr.UInt64Ptr;
			}
		}
		else {
			if( addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union Value imm = *pc.ValPtr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regid].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & Byte )
					*address_ptr.UInt8Ptr += imm.UChar;
				else if( addrmode & TwoBytes )
					*address_ptr.UInt16Ptr += imm.UShort;
				else if( addrmode & FourBytes )
					*address_ptr.UInt32Ptr += imm.UInt32;
				else if( addrmode & EightBytes )
					*address_ptr.UInt64Ptr += imm.UInt64;
			}
			else if( addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regids & 255].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & Byte )
					*address_ptr.UInt8Ptr += regs[regids >> 8].UChar;
				else if( addrmode & TwoBytes )
					*address_ptr.UInt16Ptr += regs[regids >> 8].UShort;
				else if( addrmode & FourBytes )
					*address_ptr.UInt32Ptr += regs[regids >> 8].UInt32;
				else if( addrmode & EightBytes )
					*address_ptr.UInt64Ptr += regs[regids >> 8].UInt64;
			}
		}
		DISPATCH();
	}
	exec_sub:; {
		if( addrmode & Reserved ) {
			if( addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				regs[regid].UInt64 -= *pc.UInt64Ptr++;
			}
			else if( addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				regs[regids & 255].UInt64 -= regs[regids >> 8].UInt64;
			}
			else if( addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regids >> 8].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & Byte )
					regs[regids & 255].UChar -= *address_ptr.UInt8Ptr;
				else if( addrmode & TwoBytes )
					regs[regids & 255].UShort -= *address_ptr.UInt16Ptr;
				else if( addrmode & FourBytes )
					regs[regids & 255].UInt32 -= *address_ptr.UInt32Ptr;
				else if( addrmode & EightBytes )
					regs[regids & 255].UInt64 -= *address_ptr.UInt64Ptr;
			}
		}
		else {
			if( addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union Value imm = *pc.ValPtr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regid].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & Byte )
					*address_ptr.UInt8Ptr -= imm.UChar;
				else if( addrmode & TwoBytes )
					*address_ptr.UInt16Ptr -= imm.UShort;
				else if( addrmode & FourBytes )
					*address_ptr.UInt32Ptr -= imm.UInt32;
				else if( addrmode & EightBytes )
					*address_ptr.UInt64Ptr -= imm.UInt64;
			}
			else if( addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regids & 255].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & Byte )
					*address_ptr.UInt8Ptr -= regs[regids >> 8].UChar;
				else if( addrmode & TwoBytes )
					*address_ptr.UInt16Ptr -= regs[regids >> 8].UShort;
				else if( addrmode & FourBytes )
					*address_ptr.UInt32Ptr -= regs[regids >> 8].UInt32;
				else if( addrmode & EightBytes )
					*address_ptr.UInt64Ptr -= regs[regids >> 8].UInt64;
			}
		}
		DISPATCH();
	}
	exec_mul:; {
		if( addrmode & Reserved ) {
			if( addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				regs[regid].UInt64 *= *pc.UInt64Ptr++;
			}
			else if( addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				regs[regids & 255].UInt64 *= regs[regids >> 8].UInt64;
			}
			else if( addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regids >> 8].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & Byte )
					regs[regids & 255].UChar *= *address_ptr.UInt8Ptr;
				else if( addrmode & TwoBytes )
					regs[regids & 255].UShort *= *address_ptr.UInt16Ptr;
				else if( addrmode & FourBytes )
					regs[regids & 255].UInt32 *= *address_ptr.UInt32Ptr;
				else if( addrmode & EightBytes )
					regs[regids & 255].UInt64 *= *address_ptr.UInt64Ptr;
			}
		}
		else {
			if( addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union Value imm = *pc.ValPtr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regid].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & Byte )
					*address_ptr.UInt8Ptr *= imm.UChar;
				else if( addrmode & TwoBytes )
					*address_ptr.UInt16Ptr *= imm.UShort;
				else if( addrmode & FourBytes )
					*address_ptr.UInt32Ptr *= imm.UInt32;
				else if( addrmode & EightBytes )
					*address_ptr.UInt64Ptr *= imm.UInt64;
			}
			else if( addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regids & 255].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & Byte )
					*address_ptr.UInt8Ptr *= regs[regids >> 8].UChar;
				else if( addrmode & TwoBytes )
					*address_ptr.UInt16Ptr *= regs[regids >> 8].UShort;
				else if( addrmode & FourBytes )
					*address_ptr.UInt32Ptr *= regs[regids >> 8].UInt32;
				else if( addrmode & EightBytes )
					*address_ptr.UInt64Ptr *= regs[regids >> 8].UInt64;
			}
		}
		DISPATCH();
	}
	exec_divi:; {
		if( addrmode & Reserved ) {
			if( addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				regs[regid].UInt64 /= *pc.UInt64Ptr++;
			}
			else if( addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				regs[regids & 255].UInt64 /= regs[regids >> 8].UInt64;
			}
			else if( addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regids >> 8].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & Byte )
					regs[regids & 255].UChar /= *address_ptr.UInt8Ptr;
				else if( addrmode & TwoBytes )
					regs[regids & 255].UShort /= *address_ptr.UInt16Ptr;
				else if( addrmode & FourBytes )
					regs[regids & 255].UInt32 /= *address_ptr.UInt32Ptr;
				else if( addrmode & EightBytes )
					regs[regids & 255].UInt64 /= *address_ptr.UInt64Ptr;
			}
		}
		else {
			if( addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union Value imm = *pc.ValPtr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regid].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & Byte )
					*address_ptr.UInt8Ptr /= imm.UChar;
				else if( addrmode & TwoBytes )
					*address_ptr.UInt16Ptr /= imm.UShort;
				else if( addrmode & FourBytes )
					*address_ptr.UInt32Ptr /= imm.UInt32;
				else if( addrmode & EightBytes )
					*address_ptr.UInt64Ptr /= imm.UInt64;
			}
			else if( addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regids & 255].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & Byte )
					*address_ptr.UInt8Ptr /= regs[regids >> 8].UChar;
				else if( addrmode & TwoBytes )
					*address_ptr.UInt16Ptr /= regs[regids >> 8].UShort;
				else if( addrmode & FourBytes )
					*address_ptr.UInt32Ptr /= regs[regids >> 8].UInt32;
				else if( addrmode & EightBytes )
					*address_ptr.UInt64Ptr /= regs[regids >> 8].UInt64;
			}
		}
		DISPATCH();
	}
	exec_mod:; {
		if( addrmode & Reserved ) {
			if( addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				regs[regid].UInt64 %= *pc.UInt64Ptr++;
			}
			else if( addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				regs[regids & 255].UInt64 %= regs[regids >> 8].UInt64;
			}
			else if( addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regids >> 8].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & Byte )
					regs[regids & 255].UChar %= *address_ptr.UInt8Ptr;
				else if( addrmode & TwoBytes )
					regs[regids & 255].UShort %= *address_ptr.UInt16Ptr;
				else if( addrmode & FourBytes )
					regs[regids & 255].UInt32 %= *address_ptr.UInt32Ptr;
				else if( addrmode & EightBytes )
					regs[regids & 255].UInt64 %= *address_ptr.UInt64Ptr;
			}
		}
		else {
			if( addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union Value imm = *pc.ValPtr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regid].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & Byte )
					*address_ptr.UInt8Ptr %= imm.UChar;
				else if( addrmode & TwoBytes )
					*address_ptr.UInt16Ptr %= imm.UShort;
				else if( addrmode & FourBytes )
					*address_ptr.UInt32Ptr %= imm.UInt32;
				else if( addrmode & EightBytes )
					*address_ptr.UInt64Ptr %= imm.UInt64;
			}
			else if( addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regids & 255].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & Byte )
					*address_ptr.UInt8Ptr %= regs[regids >> 8].UChar;
				else if( addrmode & TwoBytes )
					*address_ptr.UInt16Ptr %= regs[regids >> 8].UShort;
				else if( addrmode & FourBytes )
					*address_ptr.UInt32Ptr %= regs[regids >> 8].UInt32;
				else if( addrmode & EightBytes )
					*address_ptr.UInt64Ptr %= regs[regids >> 8].UInt64;
			}
		}
		DISPATCH();
	}
	exec_andb:; {
		if( addrmode & Reserved ) {
			if( addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				regs[regid].UInt64 &= *pc.UInt64Ptr++;
			}
			else if( addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				regs[regids & 255].UInt64 &= regs[regids >> 8].UInt64;
			}
			else if( addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regids >> 8].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & Byte )
					regs[regids & 255].UChar &= *address_ptr.UInt8Ptr;
				else if( addrmode & TwoBytes )
					regs[regids & 255].UShort &= *address_ptr.UInt16Ptr;
				else if( addrmode & FourBytes )
					regs[regids & 255].UInt32 &= *address_ptr.UInt32Ptr;
				else if( addrmode & EightBytes )
					regs[regids & 255].UInt64 &= *address_ptr.UInt64Ptr;
			}
		}
		else {
			if( addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union Value imm = *pc.ValPtr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regid].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & Byte )
					*address_ptr.UInt8Ptr &= imm.UChar;
				else if( addrmode & TwoBytes )
					*address_ptr.UInt16Ptr &= imm.UShort;
				else if( addrmode & FourBytes )
					*address_ptr.UInt32Ptr &= imm.UInt32;
				else if( addrmode & EightBytes )
					*address_ptr.UInt64Ptr &= imm.UInt64;
			}
			else if( addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regids & 255].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & Byte )
					*address_ptr.UInt8Ptr &= regs[regids >> 8].UChar;
				else if( addrmode & TwoBytes )
					*address_ptr.UInt16Ptr &= regs[regids >> 8].UShort;
				else if( addrmode & FourBytes )
					*address_ptr.UInt32Ptr &= regs[regids >> 8].UInt32;
				else if( addrmode & EightBytes )
					*address_ptr.UInt64Ptr &= regs[regids >> 8].UInt64;
			}
		}
		DISPATCH();
	}
	exec_orb:; {
		if( addrmode & Reserved ) {
			if( addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				regs[regid].UInt64 |= *pc.UInt64Ptr++;
			}
			else if( addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				regs[regids & 255].UInt64 |= regs[regids >> 8].UInt64;
			}
			else if( addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regids >> 8].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & Byte )
					regs[regids & 255].UChar |= *address_ptr.UInt8Ptr;
				else if( addrmode & TwoBytes )
					regs[regids & 255].UShort |= *address_ptr.UInt16Ptr;
				else if( addrmode & FourBytes )
					regs[regids & 255].UInt32 |= *address_ptr.UInt32Ptr;
				else if( addrmode & EightBytes )
					regs[regids & 255].UInt64 |= *address_ptr.UInt64Ptr;
			}
		}
		else {
			if( addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union Value imm = *pc.ValPtr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regid].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & Byte )
					*address_ptr.UInt8Ptr |= imm.UChar;
				else if( addrmode & TwoBytes )
					*address_ptr.UInt16Ptr |= imm.UShort;
				else if( addrmode & FourBytes )
					*address_ptr.UInt32Ptr |= imm.UInt32;
				else if( addrmode & EightBytes )
					*address_ptr.UInt64Ptr |= imm.UInt64;
			}
			else if( addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regids & 255].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & Byte )
					*address_ptr.UInt8Ptr |= regs[regids >> 8].UChar;
				else if( addrmode & TwoBytes )
					*address_ptr.UInt16Ptr |= regs[regids >> 8].UShort;
				else if( addrmode & FourBytes )
					*address_ptr.UInt32Ptr |= regs[regids >> 8].UInt32;
				else if( addrmode & EightBytes )
					*address_ptr.UInt64Ptr |= regs[regids >> 8].UInt64;
			}
		}
		DISPATCH();
	}
	exec_xorb:; {
		if( addrmode & Reserved ) {
			if( addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				regs[regid].UInt64 ^= *pc.UInt64Ptr++;
			}
			else if( addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				regs[regids & 255].UInt64 ^= regs[regids >> 8].UInt64;
			}
			else if( addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regids >> 8].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & Byte )
					regs[regids & 255].UChar ^= *address_ptr.UInt8Ptr;
				else if( addrmode & TwoBytes )
					regs[regids & 255].UShort ^= *address_ptr.UInt16Ptr;
				else if( addrmode & FourBytes )
					regs[regids & 255].UInt32 ^= *address_ptr.UInt32Ptr;
				else if( addrmode & EightBytes )
					regs[regids & 255].UInt64 ^= *address_ptr.UInt64Ptr;
			}
		}
		else {
			if( addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union Value imm = *pc.ValPtr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regid].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & Byte )
					*address_ptr.UInt8Ptr ^= imm.UChar;
				else if( addrmode & TwoBytes )
					*address_ptr.UInt16Ptr ^= imm.UShort;
				else if( addrmode & FourBytes )
					*address_ptr.UInt32Ptr ^= imm.UInt32;
				else if( addrmode & EightBytes )
					*address_ptr.UInt64Ptr ^= imm.UInt64;
			}
			else if( addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regids & 255].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & Byte )
					*address_ptr.UInt8Ptr ^= regs[regids >> 8].UChar;
				else if( addrmode & TwoBytes )
					*address_ptr.UInt16Ptr ^= regs[regids >> 8].UShort;
				else if( addrmode & FourBytes )
					*address_ptr.UInt32Ptr ^= regs[regids >> 8].UInt32;
				else if( addrmode & EightBytes )
					*address_ptr.UInt64Ptr ^= regs[regids >> 8].UInt64;
			}
		}
		DISPATCH();
	}
	exec_notb:; {
		const uint8_t regid = *pc.UInt8Ptr++;
		if( addrmode & Register )
			regs[regid].UInt64 = ~regs[regid].UInt64;
		else if( addrmode & RegIndirect ) {
			const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regid].UCharPtr + *pc.Int32Ptr++};
			if( addrmode & Byte )
				*address_ptr.UInt8Ptr = ~*address_ptr.UInt8Ptr;
			else if( addrmode & TwoBytes )
				*address_ptr.UInt16Ptr = ~*address_ptr.UInt16Ptr;
			else if( addrmode & FourBytes )
				*address_ptr.UInt32Ptr = ~*address_ptr.UInt32Ptr;
			else if( addrmode & EightBytes )
				*address_ptr.UInt64Ptr = ~*address_ptr.UInt64Ptr;
		}
		DISPATCH();
	}
	exec_shl:; {
		if( addrmode & Reserved ) {
			if( addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				regs[regid].UInt64 <<= *pc.UInt64Ptr++;
			}
			else if( addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				regs[regids & 255].UInt64 <<= regs[regids >> 8].UInt64;
			}
			else if( addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regids >> 8].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & Byte )
					regs[regids & 255].UChar <<= *address_ptr.UInt8Ptr;
				else if( addrmode & TwoBytes )
					regs[regids & 255].UShort <<= *address_ptr.UInt16Ptr;
				else if( addrmode & FourBytes )
					regs[regids & 255].UInt32 <<= *address_ptr.UInt32Ptr;
				else if( addrmode & EightBytes )
					regs[regids & 255].UInt64 <<= *address_ptr.UInt64Ptr;
			}
		}
		else {
			if( addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union Value imm = *pc.ValPtr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regid].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & Byte )
					*address_ptr.UInt8Ptr <<= imm.UChar;
				else if( addrmode & TwoBytes )
					*address_ptr.UInt16Ptr <<= imm.UShort;
				else if( addrmode & FourBytes )
					*address_ptr.UInt32Ptr <<= imm.UInt32;
				else if( addrmode & EightBytes )
					*address_ptr.UInt64Ptr <<= imm.UInt64;
			}
			else if( addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regids & 255].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & Byte )
					*address_ptr.UInt8Ptr <<= regs[regids >> 8].UChar;
				else if( addrmode & TwoBytes )
					*address_ptr.UInt16Ptr <<= regs[regids >> 8].UShort;
				else if( addrmode & FourBytes )
					*address_ptr.UInt32Ptr <<= regs[regids >> 8].UInt32;
				else if( addrmode & EightBytes )
					*address_ptr.UInt64Ptr <<= regs[regids >> 8].UInt64;
			}
		}
		DISPATCH();
	}
	exec_shr:; {
		if( addrmode & Reserved ) {
			if( addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				regs[regid].UInt64 >>= *pc.UInt64Ptr++;
			}
			else if( addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				regs[regids & 255].UInt64 >>= regs[regids >> 8].UInt64;
			}
			else if( addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regids >> 8].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & Byte )
					regs[regids & 255].UChar >>= *address_ptr.UInt8Ptr;
				else if( addrmode & TwoBytes )
					regs[regids & 255].UShort >>= *address_ptr.UInt16Ptr;
				else if( addrmode & FourBytes )
					regs[regids & 255].UInt32 >>= *address_ptr.UInt32Ptr;
				else if( addrmode & EightBytes )
					regs[regids & 255].UInt64 >>= *address_ptr.UInt64Ptr;
			}
		}
		else {
			if( addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union Value imm = *pc.ValPtr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regid].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & Byte )
					*address_ptr.UInt8Ptr >>= imm.UChar;
				else if( addrmode & TwoBytes )
					*address_ptr.UInt16Ptr >>= imm.UShort;
				else if( addrmode & FourBytes )
					*address_ptr.UInt32Ptr >>= imm.UInt32;
				else if( addrmode & EightBytes )
					*address_ptr.UInt64Ptr >>= imm.UInt64;
			}
			else if( addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regids & 255].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & Byte )
					*address_ptr.UInt8Ptr >>= regs[regids >> 8].UChar;
				else if( addrmode & TwoBytes )
					*address_ptr.UInt16Ptr >>= regs[regids >> 8].UShort;
				else if( addrmode & FourBytes )
					*address_ptr.UInt32Ptr >>= regs[regids >> 8].UInt32;
				else if( addrmode & EightBytes )
					*address_ptr.UInt64Ptr >>= regs[regids >> 8].UInt64;
			}
		}
		DISPATCH();
	}
	exec_inc:; {
		const uint8_t regid = *pc.UInt8Ptr++;
		if( addrmode & Register )
			++regs[regid].UInt64;
		else if( addrmode & RegIndirect ) {
			const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regid].UCharPtr + *pc.Int32Ptr++};
			if( addrmode & Byte )
				++*address_ptr.UInt8Ptr;
			else if( addrmode & TwoBytes )
				++*address_ptr.UInt16Ptr;
			else if( addrmode & FourBytes )
				++*address_ptr.UInt32Ptr;
			else if( addrmode & EightBytes )
				++*address_ptr.UInt64Ptr;
		}
		DISPATCH();
	}
	exec_dec:; {
		const uint8_t regid = *pc.UInt8Ptr++;
		if( addrmode & Register )
			--regs[regid].UInt64;
		else if( addrmode & RegIndirect ) {
			const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regid].UCharPtr + *pc.Int32Ptr++};
			if( addrmode & Byte )
				--*address_ptr.UInt8Ptr;
			else if( addrmode & TwoBytes )
				--*address_ptr.UInt16Ptr;
			else if( addrmode & FourBytes )
				--*address_ptr.UInt32Ptr;
			else if( addrmode & EightBytes )
				--*address_ptr.UInt64Ptr;
		}
		DISPATCH();
	}
	exec_neg:; {
		const uint8_t regid = *pc.UInt8Ptr++;
		if( addrmode & Register )
			regs[regid].UInt64 = -regs[regid].UInt64;
		else if( addrmode & RegIndirect ) {
			const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regid].UCharPtr + *pc.Int32Ptr++};
			if( addrmode & Byte )
				*address_ptr.UInt8Ptr = -*address_ptr.UInt8Ptr;
			else if( addrmode & TwoBytes )
				*address_ptr.UInt16Ptr = -*address_ptr.UInt16Ptr;
			else if( addrmode & FourBytes )
				*address_ptr.UInt32Ptr = -*address_ptr.UInt32Ptr;
			else if( addrmode & EightBytes )
				*address_ptr.UInt64Ptr = -*address_ptr.UInt64Ptr;
		}
		DISPATCH();
	}
	exec_lt:; {
		if( addrmode & Reserved ) {
			if( addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				vm->CondFlag = regs[regid].UInt64 < *pc.UInt64Ptr++;
			}
			else if( addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				vm->CondFlag = regs[regids & 255].UInt64 < regs[regids >> 8].UInt64;
			}
			else if( addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regids >> 8].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & Byte )
					vm->CondFlag = regs[regids & 255].UChar < *address_ptr.UInt8Ptr;
				else if( addrmode & TwoBytes )
					vm->CondFlag = regs[regids & 255].UShort < *address_ptr.UInt16Ptr;
				else if( addrmode & FourBytes )
					vm->CondFlag = regs[regids & 255].UInt32 < *address_ptr.UInt32Ptr;
				else if( addrmode & EightBytes )
					vm->CondFlag = regs[regids & 255].UInt64 < *address_ptr.UInt64Ptr;
			}
		}
		else {
			if( addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union Value imm = *pc.ValPtr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regid].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & Byte )
					vm->CondFlag = *address_ptr.UInt8Ptr < imm.UChar;
				else if( addrmode & TwoBytes )
					vm->CondFlag = *address_ptr.UInt16Ptr < imm.UShort;
				else if( addrmode & FourBytes )
					vm->CondFlag = *address_ptr.UInt32Ptr < imm.UInt32;
				else if( addrmode & EightBytes )
					vm->CondFlag = *address_ptr.UInt64Ptr < imm.UInt64;
			}
			else if( addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regids & 255].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & Byte )
					vm->CondFlag = *address_ptr.UInt8Ptr < regs[regids >> 8].UChar;
				else if( addrmode & TwoBytes )
					vm->CondFlag = *address_ptr.UInt16Ptr < regs[regids >> 8].UShort;
				else if( addrmode & FourBytes )
					vm->CondFlag = *address_ptr.UInt32Ptr < regs[regids >> 8].UInt32;
				else if( addrmode & EightBytes )
					vm->CondFlag = *address_ptr.UInt64Ptr < regs[regids >> 8].UInt64;
			}
		}
		DISPATCH();
	}
	exec_gt:; {
		if( addrmode & Reserved ) {
			if( addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				vm->CondFlag = regs[regid].UInt64 > *pc.UInt64Ptr++;
			}
			else if( addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				vm->CondFlag = regs[regids & 255].UInt64 > regs[regids >> 8].UInt64;
			}
			else if( addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regids >> 8].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & Byte )
					vm->CondFlag = regs[regids & 255].UChar > *address_ptr.UInt8Ptr;
				else if( addrmode & TwoBytes )
					vm->CondFlag = regs[regids & 255].UShort > *address_ptr.UInt16Ptr;
				else if( addrmode & FourBytes )
					vm->CondFlag = regs[regids & 255].UInt32 > *address_ptr.UInt32Ptr;
				else if( addrmode & EightBytes )
					vm->CondFlag = regs[regids & 255].UInt64 > *address_ptr.UInt64Ptr;
			}
		}
		else {
			if( addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union Value imm = *pc.ValPtr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regid].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & Byte )
					vm->CondFlag = *address_ptr.UInt8Ptr > imm.UChar;
				else if( addrmode & TwoBytes )
					vm->CondFlag = *address_ptr.UInt16Ptr > imm.UShort;
				else if( addrmode & FourBytes )
					vm->CondFlag = *address_ptr.UInt32Ptr > imm.UInt32;
				else if( addrmode & EightBytes )
					vm->CondFlag = *address_ptr.UInt64Ptr > imm.UInt64;
			}
			else if( addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regids & 255].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & Byte )
					vm->CondFlag = *address_ptr.UInt8Ptr > regs[regids >> 8].UChar;
				else if( addrmode & TwoBytes )
					vm->CondFlag = *address_ptr.UInt16Ptr > regs[regids >> 8].UShort;
				else if( addrmode & FourBytes )
					vm->CondFlag = *address_ptr.UInt32Ptr > regs[regids >> 8].UInt32;
				else if( addrmode & EightBytes )
					vm->CondFlag = *address_ptr.UInt64Ptr > regs[regids >> 8].UInt64;
			}
		}
		DISPATCH();
	}
	exec_cmp:; {
		if( addrmode & Reserved ) {
			if( addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				vm->CondFlag = regs[regid].UInt64 == *pc.UInt64Ptr++;
			}
			else if( addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				vm->CondFlag = regs[regids & 255].UInt64 == regs[regids >> 8].UInt64;
			}
			else if( addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regids >> 8].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & Byte )
					vm->CondFlag = regs[regids & 255].UChar == *address_ptr.UInt8Ptr;
				else if( addrmode & TwoBytes )
					vm->CondFlag = regs[regids & 255].UShort == *address_ptr.UInt16Ptr;
				else if( addrmode & FourBytes )
					vm->CondFlag = regs[regids & 255].UInt32 == *address_ptr.UInt32Ptr;
				else if( addrmode & EightBytes )
					vm->CondFlag = regs[regids & 255].UInt64 == *address_ptr.UInt64Ptr;
			}
		}
		else {
			if( addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union Value imm = *pc.ValPtr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regid].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & Byte )
					vm->CondFlag = *address_ptr.UInt8Ptr == imm.UChar;
				else if( addrmode & TwoBytes )
					vm->CondFlag = *address_ptr.UInt16Ptr == imm.UShort;
				else if( addrmode & FourBytes )
					vm->CondFlag = *address_ptr.UInt32Ptr == imm.UInt32;
				else if( addrmode & EightBytes )
					vm->CondFlag = *address_ptr.UInt64Ptr == imm.UInt64;
			}
			else if( addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regids & 255].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & Byte )
					vm->CondFlag = *address_ptr.UInt8Ptr == regs[regids >> 8].UChar;
				else if( addrmode & TwoBytes )
					vm->CondFlag = *address_ptr.UInt16Ptr == regs[regids >> 8].UShort;
				else if( addrmode & FourBytes )
					vm->CondFlag = *address_ptr.UInt32Ptr == regs[regids >> 8].UInt32;
				else if( addrmode & EightBytes )
					vm->CondFlag = *address_ptr.UInt64Ptr == regs[regids >> 8].UInt64;
			}
		}
		DISPATCH();
	}
	exec_neq:; {
		if( addrmode & Reserved ) {
			if( addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				vm->CondFlag = regs[regid].UInt64 != *pc.UInt64Ptr++;
			}
			else if( addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				vm->CondFlag = regs[regids & 255].UInt64 != regs[regids >> 8].UInt64;
			}
			else if( addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regids >> 8].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & Byte )
					vm->CondFlag = regs[regids & 255].UChar != *address_ptr.UInt8Ptr;
				else if( addrmode & TwoBytes )
					vm->CondFlag = regs[regids & 255].UShort != *address_ptr.UInt16Ptr;
				else if( addrmode & FourBytes )
					vm->CondFlag = regs[regids & 255].UInt32 != *address_ptr.UInt32Ptr;
				else if( addrmode & EightBytes )
					vm->CondFlag = regs[regids & 255].UInt64 != *address_ptr.UInt64Ptr;
			}
		}
		else {
			if( addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union Value imm = *pc.ValPtr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regid].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & Byte )
					vm->CondFlag = *address_ptr.UInt8Ptr != imm.UChar;
				else if( addrmode & TwoBytes )
					vm->CondFlag = *address_ptr.UInt16Ptr != imm.UShort;
				else if( addrmode & FourBytes )
					vm->CondFlag = *address_ptr.UInt32Ptr != imm.UInt32;
				else if( addrmode & EightBytes )
					vm->CondFlag = *address_ptr.UInt64Ptr != imm.UInt64;
			}
			else if( addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regids & 255].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & Byte )
					vm->CondFlag = *address_ptr.UInt8Ptr != regs[regids >> 8].UChar;
				else if( addrmode & TwoBytes )
					vm->CondFlag = *address_ptr.UInt16Ptr != regs[regids >> 8].UShort;
				else if( addrmode & FourBytes )
					vm->CondFlag = *address_ptr.UInt32Ptr != regs[regids >> 8].UInt32;
				else if( addrmode & EightBytes )
					vm->CondFlag = *address_ptr.UInt64Ptr != regs[regids >> 8].UInt64;
			}
		}
		DISPATCH();
	}
	exec_jmp:; {
		if( addrmode & Immediate ) {
			const int64_t offset = *pc.Int64Ptr++;
			pc.UInt8Ptr += offset;
		}
		else if( addrmode & Register ) {
			const uint8_t regid = *pc.UInt8Ptr++;
			pc.UInt8Ptr += regs[regid].Int64;
		}
		else if( addrmode & RegIndirect ) {
			const uint8_t regid = *pc.UInt8Ptr++;
			const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regid].UCharPtr + *pc.Int32Ptr++};
			
			if( addrmode & Byte )
				pc.UInt8Ptr += *address_ptr.Int8Ptr;
			else if( addrmode & TwoBytes )
				pc.UInt8Ptr += *address_ptr.Int16Ptr;
			else if( addrmode & FourBytes )
				pc.UInt8Ptr += *address_ptr.Int32Ptr;
			else if( addrmode & EightBytes )
				pc.UInt8Ptr += *address_ptr.Int64Ptr;
		}
		DISPATCH();
	}
	exec_jz:; {
		if( addrmode & Immediate ) {
			const int64_t offset = *pc.Int64Ptr++;
			!vm->CondFlag ? (pc.UInt8Ptr += offset) : (void)vm->CondFlag;
		}
		else if( addrmode & Register ) {
			const uint8_t regid = *pc.UInt8Ptr++;
			!vm->CondFlag ? (pc.UInt8Ptr += regs[regid].Int64) : (void)vm->CondFlag;
		}
		else if( addrmode & RegIndirect ) {
			const uint8_t regid = *pc.UInt8Ptr++;
			const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regid].UCharPtr + *pc.Int32Ptr++};
			if( addrmode & Byte )
				!vm->CondFlag ? (pc.UInt8Ptr += *address_ptr.Int8Ptr) : (void)vm->CondFlag;
			else if( addrmode & TwoBytes )
				!vm->CondFlag ? (pc.UInt8Ptr += *address_ptr.Int16Ptr) : (void)vm->CondFlag;
			else if( addrmode & FourBytes )
				!vm->CondFlag ? (pc.UInt8Ptr += *address_ptr.Int32Ptr) : (void)vm->CondFlag;
			else if( addrmode & EightBytes )
				!vm->CondFlag ? (pc.UInt8Ptr += *address_ptr.Int64Ptr) : (void)vm->CondFlag;
		}
		DISPATCH();
	}
	exec_jnz:; {
		if( addrmode & Immediate ) {
			const int64_t offset = *pc.Int64Ptr++;
			vm->CondFlag ? (pc.UInt8Ptr += offset) : (void)vm->CondFlag;
		}
		else if( addrmode & Register ) {
			const uint8_t regid = *pc.UInt8Ptr++;
			vm->CondFlag ? (pc.UInt8Ptr += regs[regid].Int64) : (void)vm->CondFlag;
		}
		else if( addrmode & RegIndirect ) {
			const uint8_t regid = *pc.UInt8Ptr++;
			const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regid].UCharPtr + *pc.Int32Ptr++};
			if( addrmode & Byte )
				vm->CondFlag ? (pc.UInt8Ptr += *address_ptr.Int8Ptr) : (void)vm->CondFlag;
			else if( addrmode & TwoBytes )
				vm->CondFlag ? (pc.UInt8Ptr += *address_ptr.Int16Ptr) : (void)vm->CondFlag;
			else if( addrmode & FourBytes )
				vm->CondFlag ? (pc.UInt8Ptr += *address_ptr.Int32Ptr) : (void)vm->CondFlag;
			else if( addrmode & EightBytes )
				vm->CondFlag ? (pc.UInt8Ptr += *address_ptr.Int64Ptr) : (void)vm->CondFlag;
		}
		DISPATCH();
	}
	exec_call:; {
		uint64_t index = -1;
		if( addrmode & Immediate ) {
			index = ((*pc.UInt64Ptr++) - 1);
		}
		else if( addrmode & Register )
			index = (regs[*pc.UInt8Ptr++].UInt64 - 1);
		else if( addrmode & RegIndirect ) {
			const uint8_t regid = *pc.UInt8Ptr++;
			const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regid].UCharPtr + *pc.Int32Ptr++};
			if( addrmode & EightBytes )
				index = (*address_ptr.UInt64Ptr - 1);
		}
		uint8_t *const call_addr = GetFunctionOffsetByIndex(vm->CurrScript.Ptr, index);
		if( !call_addr ) {
			//puts("Tagha_Exec :: exec_call reported 'call_addr' is NULL");
			DISPATCH();
		}
		/* The restrict type qualifier is an indication to the compiler that,
		 * if the memory addressed by the restrict-qualified pointer is modified,
		 * no other pointer will access that same memory.
		 * Since we're pushing the restrict-qualified pointer's memory that it points to,
		 * This is NOT undefined behavior because it's not aliasing access of the instruction stream.
		 */
		(--regs[regStk].SelfPtr)->Ptr = pc.Ptr;	/* push rip */
		*--regs[regStk].SelfPtr = regs[regBase];	/* push rbp */
		regs[regBase] = regs[regStk];	/* mov rbp, rsp */
		pc.UInt8Ptr = call_addr;
		DISPATCH();
	}
	exec_ret:; {
		regs[regStk] = regs[regBase]; /* mov rsp, rbp */
		regs[regBase] = *regs[regStk].SelfPtr++; /* pop rbp */
		
		/* if we're popping Main's (or whatever called func's) RBP, then halt the whole program. */
		if( regs[regBase].SelfPtr==MainBasePtr )
			goto *dispatch[halt];
		
		pc.Ptr = (*regs[regStk].SelfPtr++).Ptr; /* pop rip */
		DISPATCH();
	}
	exec_syscall:; {
		/* how many args given to the native call. */
		const uint32_t argcount = *pc.UInt32Ptr++;
		uint64_t index = -1;
		/* trying to directly call a specific native. Allow this by imm only! */
		if( addrmode & Immediate ) {
			index = (-1 - *pc.Int64Ptr++);
		}
		else if( addrmode & Register ) {
			index = (-1 - regs[*pc.UInt8Ptr++].Int64);
		}
		else if( addrmode & RegIndirect ) {
			const uint8_t regid = *pc.UInt8Ptr++;
			const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regid].UCharPtr + *pc.Int32Ptr++};
			index = (-1 - *address_ptr.Int64Ptr);
		}
		/* native call interface
		 * Limitations:
		 *  - any argument larger than 8 bytes must be passed as a pointer.
		 *  - any return value larger than 8 bytes must be passed as a hidden pointer argument and render the function as void.
		 * void NativeFunc(struct Tagha *sys, union Value *retval, const size_t args, union Value params[static args]);
		 */
		const union Value *const restrict nativeref = GetFunctionOffsetByIndex(vm->CurrScript.Ptr, index);
		if( !nativeref or !nativeref->VoidFunc ) {
			DISPATCH();
		}
		const uint8_t reg_params = 8;
		const uint8_t reg_param_initial = regSemkath;
		regs[regAlaf].UInt64 = 0;
		
		/* save stack space by using the registers for passing arguments. */
		/* the other registers can then be used for other data operations. */
		if( argcount <= reg_params ) {
			(*nativeref->VoidFunc)(vm, regs+regAlaf, argcount, regs+reg_param_initial);
		}
		/* if the native has more than a certain num of params, get from both registers and stack. */
		else if( argcount > reg_params ) {
			InvokeNative(vm, regs, argcount, nativeref->VoidFunc);
		}
		DISPATCH();
	}
#if FLOATING_POINT_OPS
	exec_flt2dbl:; {
		if( addrmode & Register ) {
			const uint8_t regid = *pc.UInt8Ptr++;
			const float f = regs[regid].Float;
			regs[regid].Double = (double)f;
		}
		DISPATCH();
	}
	exec_dbl2flt:; {
		if( addrmode & Register ) {
			const uint8_t regid = *pc.UInt8Ptr++;
			const double d = regs[regid].Double;
			regs[regid].Double = 0;
			regs[regid].Float = (float)d;
		}
		DISPATCH();
	}
	exec_int2dbl:; {
		if( addrmode & Register ) {
			const uint8_t regid = *pc.UInt8Ptr++;
			const uint64_t i = regs[regid].UInt64;
			regs[regid].Double = (double)i;
		}
		DISPATCH();
	}
	exec_int2flt:; {
		if( addrmode & Register ) {
			const uint8_t regid = *pc.UInt8Ptr++;
			const uint64_t i = regs[regid].UInt64;
			regs[regid].UInt64 = 0;
			regs[regid].Float = (float)i;
		}
		DISPATCH();
	}
	exec_addf:; {
		if( addrmode & Reserved ) {
			if( addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union Value imm = *pc.ValPtr++;
				if( addrmode & FourBytes )
					regs[regid].Float += imm.Float;
				else if( addrmode & EightBytes )
					regs[regid].Double += imm.Double;
			}
			else if( addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				if( addrmode & FourBytes )
					regs[regids & 255].Float += regs[regids >> 8].Float;
				else if( addrmode & EightBytes )
					regs[regids & 255].Double += regs[regids >> 8].Double;
			}
			else if( addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regids >> 8].UCharPtr + *pc.Int32Ptr++};
				
				if( addrmode & FourBytes )
					regs[regids & 255].Float += *address_ptr.FloatPtr;
				else if( addrmode & EightBytes )
					regs[regids & 255].Double += *address_ptr.DoublePtr;
			}
		}
		else {
			if( addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union Value imm = *pc.ValPtr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regid].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & FourBytes )
					*address_ptr.FloatPtr += imm.Float;
				else if( addrmode & EightBytes )
					*address_ptr.DoublePtr += imm.Double;
			}
			else if( addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regids & 255].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & FourBytes )
					*address_ptr.FloatPtr += regs[regids >> 8].Float;
				else if( addrmode & EightBytes )
					*address_ptr.DoublePtr += regs[regids >> 8].Double;
			}
		}
		DISPATCH();
	}
	exec_subf:; {
		if( addrmode & Reserved ) {
			if( addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union Value imm = *pc.ValPtr++;
				if( addrmode & FourBytes )
					regs[regid].Float -= imm.Float;
				else if( addrmode & EightBytes )
					regs[regid].Double -= imm.Double;
			}
			else if( addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				if( addrmode & FourBytes )
					regs[regids & 255].Float -= regs[regids >> 8].Float;
				else if( addrmode & EightBytes )
					regs[regids & 255].Double -= regs[regids >> 8].Double;
			}
			else if( addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regids >> 8].UCharPtr + *pc.Int32Ptr++};
				
				if( addrmode & FourBytes )
					regs[regids & 255].Float -= *address_ptr.FloatPtr;
				else if( addrmode & EightBytes )
					regs[regids & 255].Double -= *address_ptr.DoublePtr;
			}
		}
		else {
			if( addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union Value imm = *pc.ValPtr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regid].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & FourBytes )
					*address_ptr.FloatPtr -= imm.Float;
				else if( addrmode & EightBytes )
					*address_ptr.DoublePtr -= imm.Double;
			}
			else if( addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regids & 255].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & FourBytes )
					*address_ptr.FloatPtr -= regs[regids >> 8].Float;
				else if( addrmode & EightBytes )
					*address_ptr.DoublePtr -= regs[regids >> 8].Double;
			}
		}
		DISPATCH();
	}
	exec_mulf:; {
		if( addrmode & Reserved ) {
			if( addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union Value imm = *pc.ValPtr++;
				if( addrmode & FourBytes )
					regs[regid].Float *= imm.Float;
				else if( addrmode & EightBytes )
					regs[regid].Double *= imm.Double;
			}
			else if( addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				if( addrmode & FourBytes )
					regs[regids & 255].Float *= regs[regids >> 8].Float;
				else if( addrmode & EightBytes )
					regs[regids & 255].Double *= regs[regids >> 8].Double;
			}
			else if( addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regids >> 8].UCharPtr + *pc.Int32Ptr++};
				
				if( addrmode & FourBytes )
					regs[regids & 255].Float *= *address_ptr.FloatPtr;
				else if( addrmode & EightBytes )
					regs[regids & 255].Double *= *address_ptr.DoublePtr;
			}
		}
		else {
			if( addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union Value imm = *pc.ValPtr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regid].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & FourBytes )
					*address_ptr.FloatPtr *= imm.Float;
				else if( addrmode & EightBytes )
					*address_ptr.DoublePtr *= imm.Double;
			}
			else if( addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regids & 255].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & FourBytes )
					*address_ptr.FloatPtr *= regs[regids >> 8].Float;
				else if( addrmode & EightBytes )
					*address_ptr.DoublePtr *= regs[regids >> 8].Double;
			}
		}
		DISPATCH();
	}
	exec_divf:; {
		if( addrmode & Reserved ) {
			if( addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union Value imm = *pc.ValPtr++;
				if( addrmode & FourBytes )
					regs[regid].Float /= imm.Float;
				else if( addrmode & EightBytes )
					regs[regid].Double /= imm.Double;
			}
			else if( addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				if( addrmode & FourBytes )
					regs[regids & 255].Float /= regs[regids >> 8].Float;
				else if( addrmode & EightBytes )
					regs[regids & 255].Double /= regs[regids >> 8].Double;
			}
			else if( addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regids >> 8].UCharPtr + *pc.Int32Ptr++};
				
				if( addrmode & FourBytes )
					regs[regids & 255].Float /= *address_ptr.FloatPtr;
				else if( addrmode & EightBytes )
					regs[regids & 255].Double /= *address_ptr.DoublePtr;
			}
		}
		else {
			if( addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union Value imm = *pc.ValPtr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regid].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & FourBytes )
					*address_ptr.FloatPtr /= imm.Float;
				else if( addrmode & EightBytes )
					*address_ptr.DoublePtr /= imm.Double;
			}
			else if( addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regids & 255].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & FourBytes )
					*address_ptr.FloatPtr /= regs[regids >> 8].Float;
				else if( addrmode & EightBytes )
					*address_ptr.DoublePtr /= regs[regids >> 8].Double;
			}
		}
		DISPATCH();
	}
	exec_ltf:; {
		if( addrmode & Reserved ) {
			if( addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union Value imm = *pc.ValPtr++;
				if( addrmode & FourBytes )
					vm->CondFlag = regs[regid].Float < imm.Float;
				else if( addrmode & EightBytes )
					vm->CondFlag = regs[regid].Double < imm.Double;
			}
			else if( addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				if( addrmode & FourBytes )
					vm->CondFlag = regs[regids & 255].Float < regs[regids >> 8].Float;
				else if( addrmode & EightBytes )
					vm->CondFlag = regs[regids & 255].Double < regs[regids >> 8].Double;
			}
			else if( addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regids >> 8].UCharPtr + *pc.Int32Ptr++};
				
				if( addrmode & FourBytes )
					vm->CondFlag = regs[regids & 255].Float < *address_ptr.FloatPtr;
				else if( addrmode & EightBytes )
					vm->CondFlag = regs[regids & 255].Double < *address_ptr.DoublePtr;
			}
		}
		else {
			if( addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union Value imm = *pc.ValPtr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regid].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & FourBytes )
					vm->CondFlag = *address_ptr.FloatPtr < imm.Float;
				else if( addrmode & EightBytes )
					vm->CondFlag = *address_ptr.DoublePtr < imm.Double;
			}
			else if( addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regids & 255].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & FourBytes )
					vm->CondFlag = *address_ptr.FloatPtr < regs[regids >> 8].Float;
				else if( addrmode & EightBytes )
					vm->CondFlag = *address_ptr.DoublePtr < regs[regids >> 8].Double;
			}
		}
		DISPATCH();
	}
	exec_gtf:; {
		if( addrmode & Reserved ) {
			if( addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union Value imm = *pc.ValPtr++;
				if( addrmode & FourBytes )
					vm->CondFlag = regs[regid].Float > imm.Float;
				else if( addrmode & EightBytes )
					vm->CondFlag = regs[regid].Double > imm.Double;
			}
			else if( addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				if( addrmode & FourBytes )
					vm->CondFlag = regs[regids & 255].Float > regs[regids >> 8].Float;
				else if( addrmode & EightBytes )
					vm->CondFlag = regs[regids & 255].Double > regs[regids >> 8].Double;
			}
			else if( addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regids >> 8].UCharPtr + *pc.Int32Ptr++};
				
				if( addrmode & FourBytes )
					vm->CondFlag = regs[regids & 255].Float > *address_ptr.FloatPtr;
				else if( addrmode & EightBytes )
					vm->CondFlag = regs[regids & 255].Double > *address_ptr.DoublePtr;
			}
		}
		else {
			if( addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union Value imm = *pc.ValPtr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regid].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & FourBytes )
					vm->CondFlag = *address_ptr.FloatPtr > imm.Float;
				else if( addrmode & EightBytes )
					vm->CondFlag = *address_ptr.DoublePtr > imm.Double;
			}
			else if( addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regids & 255].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & FourBytes )
					vm->CondFlag = *address_ptr.FloatPtr > regs[regids >> 8].Float;
				else if( addrmode & EightBytes )
					vm->CondFlag = *address_ptr.DoublePtr > regs[regids >> 8].Double;
			}
		}
		DISPATCH();
	}
	exec_cmpf:; {
		if( addrmode & Reserved ) {
			if( addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union Value imm = *pc.ValPtr++;
				if( addrmode & FourBytes )
					vm->CondFlag = regs[regid].Float == imm.Float;
				else if( addrmode & EightBytes )
					vm->CondFlag = regs[regid].Double == imm.Double;
			}
			else if( addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				if( addrmode & FourBytes )
					vm->CondFlag = regs[regids & 255].Float == regs[regids >> 8].Float;
				else if( addrmode & EightBytes )
					vm->CondFlag = regs[regids & 255].Double == regs[regids >> 8].Double;
			}
			else if( addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regids >> 8].UCharPtr + *pc.Int32Ptr++};
				
				if( addrmode & FourBytes )
					vm->CondFlag = regs[regids & 255].Float == *address_ptr.FloatPtr;
				else if( addrmode & EightBytes )
					vm->CondFlag = regs[regids & 255].Double == *address_ptr.DoublePtr;
			}
		}
		else {
			if( addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union Value imm = *pc.ValPtr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regid].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & FourBytes )
					vm->CondFlag = *address_ptr.FloatPtr == imm.Float;
				else if( addrmode & EightBytes )
					vm->CondFlag = *address_ptr.DoublePtr == imm.Double;
			}
			else if( addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regids & 255].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & FourBytes )
					vm->CondFlag = *address_ptr.FloatPtr == regs[regids >> 8].Float;
				else if( addrmode & EightBytes )
					vm->CondFlag = *address_ptr.DoublePtr == regs[regids >> 8].Double;
			}
		}
		DISPATCH();
	}
	exec_neqf:; {
		if( addrmode & Reserved ) {
			if( addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union Value imm = *pc.ValPtr++;
				if( addrmode & FourBytes )
					vm->CondFlag = regs[regid].Float != imm.Float;
				else if( addrmode & EightBytes )
					vm->CondFlag = regs[regid].Double != imm.Double;
			}
			else if( addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				if( addrmode & FourBytes )
					vm->CondFlag = regs[regids & 255].Float != regs[regids >> 8].Float;
				else if( addrmode & EightBytes )
					vm->CondFlag = regs[regids & 255].Double != regs[regids >> 8].Double;
			}
			else if( addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regids >> 8].UCharPtr + *pc.Int32Ptr++};
				
				if( addrmode & FourBytes )
					vm->CondFlag = regs[regids & 255].Float != *address_ptr.FloatPtr;
				else if( addrmode & EightBytes )
					vm->CondFlag = regs[regids & 255].Double != *address_ptr.DoublePtr;
			}
		}
		else {
			if( addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union Value imm = *pc.ValPtr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regid].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & FourBytes )
					vm->CondFlag = *address_ptr.FloatPtr != imm.Float;
				else if( addrmode & EightBytes )
					vm->CondFlag = *address_ptr.DoublePtr != imm.Double;
			}
			else if( addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regids & 255].UCharPtr + *pc.Int32Ptr++};
				if( addrmode & FourBytes )
					vm->CondFlag = *address_ptr.FloatPtr != regs[regids >> 8].Float;
				else if( addrmode & EightBytes )
					vm->CondFlag = *address_ptr.DoublePtr != regs[regids >> 8].Double;
			}
		}
		DISPATCH();
	}
	exec_incf:; {
		const uint8_t regid = *pc.UInt8Ptr++;
		if( addrmode & Register ) {
			if( addrmode & FourBytes )
				++regs[regid].Float;
			else if( addrmode & EightBytes )
				++regs[regid].Double;
		}
		else if( addrmode & RegIndirect ) {
			const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regid].UCharPtr + *pc.Int32Ptr++};
			if( addrmode & FourBytes )
				++*address_ptr.FloatPtr;
			else if( addrmode & EightBytes )
				++*address_ptr.DoublePtr;
		}
		DISPATCH();
	}
	exec_decf:; {
		const uint8_t regid = *pc.UInt8Ptr++;
		if( addrmode & Register ) {
			if( addrmode & FourBytes )
				--regs[regid].Float;
			else if( addrmode & EightBytes )
				--regs[regid].Double;
		}
		else if( addrmode & RegIndirect ) {
			const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regid].UCharPtr + *pc.Int32Ptr++};
			if( addrmode & FourBytes )
				--*address_ptr.FloatPtr;
			else if( addrmode & EightBytes )
				--*address_ptr.DoublePtr;
		}
		DISPATCH();
	}
	exec_negf:; {
		const uint8_t regid = *pc.UInt8Ptr++;
		if( addrmode & Register ) {
			if( addrmode & FourBytes )
				regs[regid].Float = -regs[regid].Float;
			else if( addrmode & EightBytes )
				regs[regid].Double = -regs[regid].Double;
		}
		else if( addrmode & RegIndirect ) {
			const union Pointer address_ptr = (union Pointer){.UInt8Ptr = regs[regid].UCharPtr + *pc.Int32Ptr++};
			if( addrmode & FourBytes )
				*address_ptr.FloatPtr = -*address_ptr.FloatPtr;
			else if( addrmode & EightBytes )
				*address_ptr.DoublePtr = -*address_ptr.DoublePtr;
		}
		DISPATCH();
	}
#endif
	return ErrInstrBounds;
}

int32_t Tagha_RunScript(struct Tagha *const restrict vm, const int32_t argc, char *argv[])
{
	if( !vm )
		return ErrBadPtr;
	else if( !vm->Module or vm->Module->Magic != 0xC0DE ) {
		return ErrInvalidScript;
	}
	
	uint8_t *const main_offset = GetFunctionOffsetByName(vm->CurrScript.Ptr, "main");
	if( !main_offset ) {
		return ErrMissingFunc;
	}
	
	/* push argc, argv to registers. */
	union Value MainArgs[argc+1];
	MainArgs[argc].Ptr = NULL;
	if( argv )
		for( int i=0 ; i<=argc ; i++ )
			MainArgs[i].Ptr = argv[i];
	vm->Regs[reg_Eh].Ptr = MainArgs;
	vm->Regs[regSemkath].Int32 = argc;
	
	/* check out stack size and align it by the size of union Value. */
	const size_t stacksize = (vm->Module->StackSize + (sizeof(union Value)-1)) & -(sizeof(union Value));
	if( !stacksize )
		return ErrStackSize;
	
	union Value Stack[stacksize+1]; memset(Stack, 0, sizeof Stack[0] * stacksize+1);
	vm->Regs[regStk].SelfPtr = vm->Regs[regBase].SelfPtr = Stack + stacksize;
	
	(--vm->Regs[regStk].SelfPtr)->Int64 = -1LL;	/* push bullshit ret address. */
	*--vm->Regs[regStk].SelfPtr = vm->Regs[regBase]; /* push rbp */
	vm->Regs[regInstr].UCharPtr = main_offset;
	return Tagha_Exec(vm);
}

int32_t Tagha_CallFunc(struct Tagha *const restrict vm, const char *restrict funcname, const size_t args, union Value values[static args])
{
	if( !vm or !funcname )
		return ErrBadPtr;
	else if( !vm->Module or vm->Module->Magic != 0xC0DE ) {
		return ErrInvalidScript;
	}
	
	uint8_t *const func_offset = GetFunctionOffsetByName(vm->CurrScript.Ptr, funcname);
	if( !func_offset ) {
		return ErrMissingFunc;
	}
	
	/* check out stack size and align it by the size of union Value. */
	const size_t stacksize = (vm->Module->StackSize + (sizeof(union Value)-1)) & -(sizeof(union Value));
	if( !stacksize ) {
		return ErrStackSize;
	}
	
	union Value Stack[stacksize+1]; memset(Stack, 0, sizeof Stack[0] * stacksize+1);
	vm->Regs[regStk].SelfPtr = vm->Regs[regBase].SelfPtr = Stack + stacksize;
	
	/* remember that arguments must be passed right to left. */
	/* we have enough args to fit in registers. */
	const uint8_t reg_params = 8;
	const uint8_t reg_param_initial = regSemkath;
	const uint16_t bytecount = sizeof(union Value) * args;
	
	/* save stack space by using the registers for passing arguments. */
	/* the other registers can be used for other data ops. */
	if( args <= reg_params ) {
		memcpy(vm->Regs+reg_param_initial, values, bytecount);
	}
	/* if the function has more than a certain num of params, push from both registers and stack. */
	else if( args > reg_params ) {
		memcpy(vm->Regs+reg_param_initial, values, sizeof(union Value) * reg_params);
		memcpy(vm->Regs[regStk].SelfPtr, values+reg_params, sizeof(union Value) * (args-reg_params));
		vm->Regs[regStk].SelfPtr -= (args-reg_params);
	}
	
	*--vm->Regs[regStk].SelfPtr = vm->Regs[regInstr];	/* push return address. */
	*--vm->Regs[regStk].SelfPtr = vm->Regs[regBase]; /* push rbp */
	vm->Regs[regInstr].UCharPtr = func_offset;
	return Tagha_Exec(vm);
}

union Value Tagha_GetReturnValue(const struct Tagha *const restrict vm)
{
	return vm ? vm->Regs[regAlaf] : (union Value){0};
}

void *Tagha_GetGlobalVarByName(struct Tagha *const restrict vm, const char *restrict varname)
{
	return !vm or !varname ? NULL : GetVariableOffsetByName(vm->CurrScript.Ptr, varname);
}

/************************************************/

