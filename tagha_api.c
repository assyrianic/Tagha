
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "tagha.h"

inline static void			*GetFunctionOffsetByName(uint8_t *, const char *);
inline static void			*GetFunctionOffsetByIndex(uint8_t *, size_t);
inline static void			*GetNativeByIndex(uint8_t *, size_t);

inline static void			*GetVariableOffsetByName(uint8_t *, const char *);
inline static void			*GetVariableOffsetByIndex(uint8_t *, size_t);

static void PrepModule(uint8_t *const restrict module)
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

static void InvokeNative(struct Tagha *const vm, const size_t argcount, void (*const NativeCall)())
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
	struct Tagha *vm = calloc(1, sizeof *vm);
	Tagha_Init(vm, script);
	return vm;
}

struct Tagha *Tagha_NewNatives(void *restrict script, const struct NativeInfo natives[restrict])
{
	struct Tagha *vm = Tagha_New(script);
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
	if( !vm )
		return;
	
	*vm = (struct Tagha){0};
	vm->Header = script;
	PrepModule(script);
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

bool Tagha_RegisterNatives(struct Tagha *const vm, const struct NativeInfo natives[])
{
	if( !vm || !natives )
		return false;
	
	for( const struct NativeInfo *restrict n=natives ; n->NativeCFunc && n->Name ; n++ ) {
		const union TaghaVal func_addr = (union TaghaVal){.Ptr = GetFunctionOffsetByName((uint8_t *)vm->Header, n->Name)};
		if( func_addr.Ptr )
			func_addr.SelfPtr->VoidFunc = n->NativeCFunc;
	}
	return true;
}

inline static void *GetFunctionOffsetByName(uint8_t *const script, const char *restrict funcname)
{
	if( !funcname || !script )
		return NULL;
	
	union Pointer reader = (union Pointer){.Ptr = script + 15};
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
	
	union Pointer reader = (union Pointer){.Ptr = script + 15};
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

inline static void *GetNativeByIndex(uint8_t *const script, const size_t index)
{
	if( !script )
		return NULL;
	
	union Pointer reader = (union Pointer){.Ptr = script + 15};
	const uint32_t funcs = *reader.UInt32Ptr++;
	if( index >= funcs )
		return NULL;
	
	for( uint32_t i=0 ; i<funcs ; i++ ) {
		reader.UInt8Ptr++;
		const uint64_t sizes = *reader.UInt64Ptr++;
		const uint32_t cstrlen = sizes & 0xffFFffFF;
		const uint32_t datalen = sizes >> 32;
		if( i==index )
			return *( void (**)() )(reader.UInt8Ptr + cstrlen);
		else reader.UInt8Ptr += (cstrlen + datalen);
	}
	return NULL;
}

inline static void *GetVariableOffsetByName(uint8_t *const script, const char *restrict varname)
{
	if( !script || !varname )
		return NULL;
	
	union Pointer reader = (union Pointer){.Ptr = script + 10};
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
	
	union Pointer reader = (union Pointer){.Ptr = script + 10};
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
	if( !vm )
		return -1;
	
	register union TaghaVal *const restrict regs = vm->Regs;
	union Pointer pc = (union Pointer){.UInt8Ptr = vm->regInstr.UInt8Ptr};
	
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
	
	#define DISPATCH() \
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
		if( decode.addrmode & Immediate ) {
			*--vm->regStk.SelfPtr = *pc.ValPtr++;
			DISPATCH();
		}
		/* push a register's contents. */
		else if( decode.addrmode & Register ) {
			*--vm->regStk.SelfPtr = regs[*pc.UInt8Ptr++];
			DISPATCH();
		}
		/* push the contents of a memory address inside a register. */
		else if( decode.addrmode & RegIndirect ) {
			const uint8_t regid = *pc.UInt8Ptr++;
			union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regid].UInt8Ptr + *pc.Int32Ptr++);
			*--vm->regStk.SelfPtr = *mem;
			DISPATCH();
		}
	}
	
	/* pops a value from the stack into a register || memory then reduces stack by 8 bytes.
	 * pop reg
	 * pop [reg+offset]
	 */
	exec_pop:; {
		if( decode.addrmode & Register ) {
			regs[*pc.UInt8Ptr++] = *vm->regStk.SelfPtr++;
			DISPATCH();
		}
		else if( decode.addrmode & RegIndirect ) {
			const uint8_t regid = *pc.UInt8Ptr++;
			union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regid].UInt8Ptr + *pc.Int32Ptr++);
			*mem = *vm->regStk.SelfPtr++;
			DISPATCH();
		}
	}
	
	/* loads a ptr value to a register.
	 * lea reg, global var
	 * lea reg, func
	 * lea reg, [reg+offset] (not dereferenced)
	 */
	exec_lea:; {
		if( decode.addrmode & Immediate ) { /* Immediate mode will load a global value */
			const uint8_t regid = *pc.UInt8Ptr++;
			regs[regid].Ptr = GetVariableOffsetByIndex((uint8_t *)vm->Header, *pc.UInt64Ptr++);
			DISPATCH();
		}
		else if( decode.addrmode & Register ) { /* Register mode will load a function address which could be a native */
			const uint8_t regid = *pc.UInt8Ptr++;
			regs[regid].Int64 = *pc.Int64Ptr++;
			DISPATCH();
		}
		else if( decode.addrmode & RegIndirect ) {
			const uint16_t regids = *pc.UInt16Ptr++;
			regs[regids & 255].UInt8Ptr = regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++;
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
	exec_mov:; {
		if( decode.addrmode & Reserved ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				regs[regid] = *pc.ValPtr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				regs[regids & 255] = regs[regids >> 8];
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					regs[regids & 255].UInt8 = mem->UInt8;
				else if( decode.addrmode & TwoBytes )
					regs[regids & 255].UInt16 = mem->UInt16;
				else if( decode.addrmode & FourBytes )
					regs[regids & 255].UInt32 = mem->UInt32;
				else if( decode.addrmode & EightBytes )
					regs[regids & 255].UInt64 = mem->UInt64;
				DISPATCH();
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regid].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					mem->UInt8 = imm.UInt8;
				else if( decode.addrmode & TwoBytes )
					mem->UInt16 = imm.UInt16;
				else if( decode.addrmode & FourBytes )
					mem->UInt32 = imm.UInt32;
				else if( decode.addrmode & EightBytes )
					mem->UInt64 = imm.UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					mem->UInt8 = regs[regids >> 8].UInt8;
				else if( decode.addrmode & TwoBytes )
					mem->UInt16 = regs[regids >> 8].UInt16;
				else if( decode.addrmode & FourBytes )
					mem->UInt32 = regs[regids >> 8].UInt32;
				else if( decode.addrmode & EightBytes )
					mem->UInt64 = regs[regids >> 8].UInt64;
				DISPATCH();
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
	exec_add:; {
		if( decode.addrmode & Reserved ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				regs[regid].UInt64 += *pc.UInt64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				regs[regids & 255].UInt64 += regs[regids >> 8].UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					regs[regids & 255].UInt8 += mem->UInt8;
				else if( decode.addrmode & TwoBytes )
					regs[regids & 255].UInt16 += mem->UInt16;
				else if( decode.addrmode & FourBytes )
					regs[regids & 255].UInt32 += mem->UInt32;
				else if( decode.addrmode & EightBytes )
					regs[regids & 255].UInt64 += mem->UInt64;
				DISPATCH();
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regid].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					mem->UInt8 += imm.UInt8;
				else if( decode.addrmode & TwoBytes )
					mem->UInt16 += imm.UInt16;
				else if( decode.addrmode & FourBytes )
					mem->UInt32 += imm.UInt32;
				else if( decode.addrmode & EightBytes )
					mem->UInt64 += imm.UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					mem->UInt8 += regs[regids >> 8].UInt8;
				else if( decode.addrmode & TwoBytes )
					mem->UInt16 += regs[regids >> 8].UInt16;
				else if( decode.addrmode & FourBytes )
					mem->UInt32 += regs[regids >> 8].UInt32;
				else if( decode.addrmode & EightBytes )
					mem->UInt64 += regs[regids >> 8].UInt64;
				DISPATCH();
			}
		}
	}
	exec_sub:; {
		if( decode.addrmode & Reserved ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				regs[regid].UInt64 -= *pc.UInt64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				regs[regids & 255].UInt64 -= regs[regids >> 8].UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					regs[regids & 255].UInt8 -= mem->UInt8;
				else if( decode.addrmode & TwoBytes )
					regs[regids & 255].UInt16 -= mem->UInt16;
				else if( decode.addrmode & FourBytes )
					regs[regids & 255].UInt32 -= mem->UInt32;
				else if( decode.addrmode & EightBytes )
					regs[regids & 255].UInt64 -= mem->UInt64;
				DISPATCH();
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regid].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					mem->UInt8 -= imm.UInt8;
				else if( decode.addrmode & TwoBytes )
					mem->UInt16 -= imm.UInt16;
				else if( decode.addrmode & FourBytes )
					mem->UInt32 -= imm.UInt32;
				else if( decode.addrmode & EightBytes )
					mem->UInt64 -= imm.UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					mem->UInt8 -= regs[regids >> 8].UInt8;
				else if( decode.addrmode & TwoBytes )
					mem->UInt16 -= regs[regids >> 8].UInt16;
				else if( decode.addrmode & FourBytes )
					mem->UInt32 -= regs[regids >> 8].UInt32;
				else if( decode.addrmode & EightBytes )
					mem->UInt64 -= regs[regids >> 8].UInt64;
				DISPATCH();
			}
		}
	}
	exec_mul:; {
		if( decode.addrmode & Reserved ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				regs[regid].UInt64 *= *pc.UInt64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				regs[regids & 255].UInt64 *= regs[regids >> 8].UInt64;
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					regs[regids & 255].UInt8 *= mem->UInt8;
				else if( decode.addrmode & TwoBytes )
					regs[regids & 255].UInt16 *= mem->UInt16;
				else if( decode.addrmode & FourBytes )
					regs[regids & 255].UInt32 *= mem->UInt32;
				else if( decode.addrmode & EightBytes )
					regs[regids & 255].UInt64 *= mem->UInt64;
				DISPATCH();
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regid].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					mem->UInt8 *= imm.UInt8;
				else if( decode.addrmode & TwoBytes )
					mem->UInt16 *= imm.UInt16;
				else if( decode.addrmode & FourBytes )
					mem->UInt32 *= imm.UInt32;
				else if( decode.addrmode & EightBytes )
					mem->UInt64 *= imm.UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					mem->UInt8 *= regs[regids >> 8].UInt8;
				else if( decode.addrmode & TwoBytes )
					mem->UInt16 *= regs[regids >> 8].UInt16;
				else if( decode.addrmode & FourBytes )
					mem->UInt32 *= regs[regids >> 8].UInt32;
				else if( decode.addrmode & EightBytes )
					mem->UInt64 *= regs[regids >> 8].UInt64;
				DISPATCH();
			}
		}
		DISPATCH();
	}
	exec_divi:; {
		if( decode.addrmode & Reserved ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				regs[regid].UInt64 /= *pc.UInt64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				regs[regids & 255].UInt64 /= regs[regids >> 8].UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					regs[regids & 255].UInt8 /= mem->UInt8;
				else if( decode.addrmode & TwoBytes )
					regs[regids & 255].UInt16 /= mem->UInt16;
				else if( decode.addrmode & FourBytes )
					regs[regids & 255].UInt32 /= mem->UInt32;
				else if( decode.addrmode & EightBytes )
					regs[regids & 255].UInt64 /= mem->UInt64;
				DISPATCH();
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regid].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					mem->UInt8 /= imm.UInt8;
				else if( decode.addrmode & TwoBytes )
					mem->UInt16 /= imm.UInt16;
				else if( decode.addrmode & FourBytes )
					mem->UInt32 /= imm.UInt32;
				else if( decode.addrmode & EightBytes )
					mem->UInt64 /= imm.UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					mem->UInt8 /= regs[regids >> 8].UInt8;
				else if( decode.addrmode & TwoBytes )
					mem->UInt16 /= regs[regids >> 8].UInt16;
				else if( decode.addrmode & FourBytes )
					mem->UInt32 /= regs[regids >> 8].UInt32;
				else if( decode.addrmode & EightBytes )
					mem->UInt64 /= regs[regids >> 8].UInt64;
				DISPATCH();
			}
		}
	}
	exec_mod:; {
		if( decode.addrmode & Reserved ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				regs[regid].UInt64 %= *pc.UInt64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				regs[regids & 255].UInt64 %= regs[regids >> 8].UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					regs[regids & 255].UInt8 %= mem->UInt8;
				else if( decode.addrmode & TwoBytes )
					regs[regids & 255].UInt16 %= mem->UInt16;
				else if( decode.addrmode & FourBytes )
					regs[regids & 255].UInt32 %= mem->UInt32;
				else if( decode.addrmode & EightBytes )
					regs[regids & 255].UInt64 %= mem->UInt64;
				DISPATCH();
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regid].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					mem->UInt8 %= imm.UInt8;
				else if( decode.addrmode & TwoBytes )
					mem->UInt16 %= imm.UInt16;
				else if( decode.addrmode & FourBytes )
					mem->UInt32 %= imm.UInt32;
				else if( decode.addrmode & EightBytes )
					mem->UInt64 %= imm.UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					mem->UInt8 %= regs[regids >> 8].UInt8;
				else if( decode.addrmode & TwoBytes )
					mem->UInt16 %= regs[regids >> 8].UInt16;
				else if( decode.addrmode & FourBytes )
					mem->UInt32 %= regs[regids >> 8].UInt32;
				else if( decode.addrmode & EightBytes )
					mem->UInt64 %= regs[regids >> 8].UInt64;
				DISPATCH();
			}
		}
	}
	exec_andb:; {
		if( decode.addrmode & Reserved ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				regs[regid].UInt64 &= *pc.UInt64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				regs[regids & 255].UInt64 &= regs[regids >> 8].UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					regs[regids & 255].UInt8 &= mem->UInt8;
				else if( decode.addrmode & TwoBytes )
					regs[regids & 255].UInt16 &= mem->UInt16;
				else if( decode.addrmode & FourBytes )
					regs[regids & 255].UInt32 &= mem->UInt32;
				else if( decode.addrmode & EightBytes )
					regs[regids & 255].UInt64 &= mem->UInt64;
				DISPATCH();
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regid].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					mem->UInt8 &= imm.UInt8;
				else if( decode.addrmode & TwoBytes )
					mem->UInt16 &= imm.UInt16;
				else if( decode.addrmode & FourBytes )
					mem->UInt32 &= imm.UInt32;
				else if( decode.addrmode & EightBytes )
					mem->UInt64 &= imm.UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					mem->UInt8 &= regs[regids >> 8].UInt8;
				else if( decode.addrmode & TwoBytes )
					mem->UInt16 &= regs[regids >> 8].UInt16;
				else if( decode.addrmode & FourBytes )
					mem->UInt32 &= regs[regids >> 8].UInt32;
				else if( decode.addrmode & EightBytes )
					mem->UInt64 &= regs[regids >> 8].UInt64;
				DISPATCH();
			}
		}
	}
	exec_orb:; {
		if( decode.addrmode & Reserved ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				regs[regid].UInt64 |= *pc.UInt64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				regs[regids & 255].UInt64 |= regs[regids >> 8].UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					regs[regids & 255].UInt8 |= mem->UInt8;
				else if( decode.addrmode & TwoBytes )
					regs[regids & 255].UInt16 |= mem->UInt16;
				else if( decode.addrmode & FourBytes )
					regs[regids & 255].UInt32 |= mem->UInt32;
				else if( decode.addrmode & EightBytes )
					regs[regids & 255].UInt64 |= mem->UInt64;
				DISPATCH();
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regid].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					mem->UInt8 |= imm.UInt8;
				else if( decode.addrmode & TwoBytes )
					mem->UInt16 |= imm.UInt16;
				else if( decode.addrmode & FourBytes )
					mem->UInt32 |= imm.UInt32;
				else if( decode.addrmode & EightBytes )
					mem->UInt64 |= imm.UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					mem->UInt8 |= regs[regids >> 8].UInt8;
				else if( decode.addrmode & TwoBytes )
					mem->UInt16 |= regs[regids >> 8].UInt16;
				else if( decode.addrmode & FourBytes )
					mem->UInt32 |= regs[regids >> 8].UInt32;
				else if( decode.addrmode & EightBytes )
					mem->UInt64 |= regs[regids >> 8].UInt64;
				DISPATCH();
			}
		}
	}
	exec_xorb:; {
		if( decode.addrmode & Reserved ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				regs[regid].UInt64 ^= *pc.UInt64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				regs[regids & 255].UInt64 ^= regs[regids >> 8].UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					regs[regids & 255].UInt8 ^= mem->UInt8;
				else if( decode.addrmode & TwoBytes )
					regs[regids & 255].UInt16 ^= mem->UInt16;
				else if( decode.addrmode & FourBytes )
					regs[regids & 255].UInt32 ^= mem->UInt32;
				else if( decode.addrmode & EightBytes )
					regs[regids & 255].UInt64 ^= mem->UInt64;
				DISPATCH();
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regid].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					mem->UInt8 ^= imm.UInt8;
				else if( decode.addrmode & TwoBytes )
					mem->UInt16 ^= imm.UInt16;
				else if( decode.addrmode & FourBytes )
					mem->UInt32 ^= imm.UInt32;
				else if( decode.addrmode & EightBytes )
					mem->UInt64 ^= imm.UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					mem->UInt8 ^= regs[regids >> 8].UInt8;
				else if( decode.addrmode & TwoBytes )
					mem->UInt16 ^= regs[regids >> 8].UInt16;
				else if( decode.addrmode & FourBytes )
					mem->UInt32 ^= regs[regids >> 8].UInt32;
				else if( decode.addrmode & EightBytes )
					mem->UInt64 ^= regs[regids >> 8].UInt64;
				DISPATCH();
			}
		}
	}
	exec_notb:; {
		const uint8_t regid = *pc.UInt8Ptr++;
		if( decode.addrmode & Register ) {
			regs[regid].UInt64 = ~regs[regid].UInt64;
			DISPATCH();
		}
		else if( decode.addrmode & RegIndirect ) {
			union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regid].UInt8Ptr + *pc.Int32Ptr++);
			if( decode.addrmode & Byte )
				mem->UInt8 = ~mem->UInt8;
			else if( decode.addrmode & TwoBytes )
				mem->UInt16 = ~mem->UInt16;
			else if( decode.addrmode & FourBytes )
				mem->UInt32 = ~mem->UInt32;
			else if( decode.addrmode & EightBytes )
				mem->UInt64 = ~mem->UInt64;
			DISPATCH();
		}
	}
	exec_shl:; {
		if( decode.addrmode & Reserved ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				regs[regid].UInt64 <<= *pc.UInt64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				regs[regids & 255].UInt64 <<= regs[regids >> 8].UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					regs[regids & 255].UInt8 <<= mem->UInt8;
				else if( decode.addrmode & TwoBytes )
					regs[regids & 255].UInt16 <<= mem->UInt16;
				else if( decode.addrmode & FourBytes )
					regs[regids & 255].UInt32 <<= mem->UInt32;
				else if( decode.addrmode & EightBytes )
					regs[regids & 255].UInt64 <<= mem->UInt64;
				DISPATCH();
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regid].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					mem->UInt8 <<= imm.UInt8;
				else if( decode.addrmode & TwoBytes )
					mem->UInt16 <<= imm.UInt16;
				else if( decode.addrmode & FourBytes )
					mem->UInt32 <<= imm.UInt32;
				else if( decode.addrmode & EightBytes )
					mem->UInt64 <<= imm.UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					mem->UInt8 <<= regs[regids >> 8].UInt8;
				else if( decode.addrmode & TwoBytes )
					mem->UInt16 <<= regs[regids >> 8].UInt16;
				else if( decode.addrmode & FourBytes )
					mem->UInt32 <<= regs[regids >> 8].UInt32;
				else if( decode.addrmode & EightBytes )
					mem->UInt64 <<= regs[regids >> 8].UInt64;
				DISPATCH();
			}
		}
		DISPATCH();
	}
	exec_shr:; {
		if( decode.addrmode & Reserved ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				regs[regid].UInt64 >>= *pc.UInt64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				regs[regids & 255].UInt64 >>= regs[regids >> 8].UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					regs[regids & 255].UInt8 >>= mem->UInt8;
				else if( decode.addrmode & TwoBytes )
					regs[regids & 255].UInt16 >>= mem->UInt16;
				else if( decode.addrmode & FourBytes )
					regs[regids & 255].UInt32 >>= mem->UInt32;
				else if( decode.addrmode & EightBytes )
					regs[regids & 255].UInt64 >>= mem->UInt64;
				DISPATCH();
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regid].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					mem->UInt8 >>= imm.UInt8;
				else if( decode.addrmode & TwoBytes )
					mem->UInt16 >>= imm.UInt16;
				else if( decode.addrmode & FourBytes )
					mem->UInt32 >>= imm.UInt32;
				else if( decode.addrmode & EightBytes )
					mem->UInt64 >>= imm.UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					mem->UInt8 >>= regs[regids >> 8].UInt8;
				else if( decode.addrmode & TwoBytes )
					mem->UInt16 >>= regs[regids >> 8].UInt16;
				else if( decode.addrmode & FourBytes )
					mem->UInt32 >>= regs[regids >> 8].UInt32;
				else if( decode.addrmode & EightBytes )
					mem->UInt64 >>= regs[regids >> 8].UInt64;
				DISPATCH();
			}
		}
	}
	exec_inc:; {
		const uint8_t regid = *pc.UInt8Ptr++;
		if( decode.addrmode & Register ) {
			++regs[regid].UInt64;
			DISPATCH();
		}
		else if( decode.addrmode & RegIndirect ) {
			union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regid].UInt8Ptr + *pc.Int32Ptr++);
			if( decode.addrmode & Byte )
				++mem->UInt8;
			else if( decode.addrmode & TwoBytes )
				++mem->UInt16;
			else if( decode.addrmode & FourBytes )
				++mem->UInt32;
			else if( decode.addrmode & EightBytes )
				++mem->UInt64;
			DISPATCH();
		}
	}
	exec_dec:; {
		const uint8_t regid = *pc.UInt8Ptr++;
		if( decode.addrmode & Register ) {
			--regs[regid].UInt64;
			DISPATCH();
		}
		else if( decode.addrmode & RegIndirect ) {
			union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regid].UInt8Ptr + *pc.Int32Ptr++);
			if( decode.addrmode & Byte )
				--mem->UInt8;
			else if( decode.addrmode & TwoBytes )
				--mem->UInt16;
			else if( decode.addrmode & FourBytes )
				--mem->UInt32;
			else if( decode.addrmode & EightBytes )
				--mem->UInt64;
			DISPATCH();
		}
	}
	exec_neg:; {
		const uint8_t regid = *pc.UInt8Ptr++;
		if( decode.addrmode & Register ) {
			regs[regid].UInt64 = -regs[regid].UInt64;
			DISPATCH();
		}
		else if( decode.addrmode & RegIndirect ) {
			union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regid].UInt8Ptr + *pc.Int32Ptr++);
			if( decode.addrmode & Byte )
				mem->UInt8 = -mem->UInt8;
			else if( decode.addrmode & TwoBytes )
				mem->UInt16 = -mem->UInt16;
			else if( decode.addrmode & FourBytes )
				mem->UInt32 = -mem->UInt32;
			else if( decode.addrmode & EightBytes )
				mem->UInt64 = -mem->UInt64;
			DISPATCH();
		}
	}
	exec_ilt:; {
		if( decode.addrmode & Reserved ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				vm->CondFlag = regs[regid].Int64 < *pc.Int64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				vm->CondFlag = regs[regids & 255].Int64 < regs[regids >> 8].Int64;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					vm->CondFlag = regs[regids & 255].Int8 < mem->Int8;
				else if( decode.addrmode & TwoBytes )
					vm->CondFlag = regs[regids & 255].Int16 < mem->Int16;
				else if( decode.addrmode & FourBytes )
					vm->CondFlag = regs[regids & 255].Int32 < mem->Int32;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = regs[regids & 255].Int64 < mem->Int64;
				DISPATCH();
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regid].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					vm->CondFlag = mem->Int8 < imm.Int8;
				else if( decode.addrmode & TwoBytes )
					vm->CondFlag = mem->Int16 < imm.Int16;
				else if( decode.addrmode & FourBytes )
					vm->CondFlag = mem->Int32 < imm.Int32;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = mem->Int64 < imm.Int64;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					vm->CondFlag = mem->Int8 < regs[regids >> 8].Int8;
				else if( decode.addrmode & TwoBytes )
					vm->CondFlag = mem->Int16 < regs[regids >> 8].Int16;
				else if( decode.addrmode & FourBytes )
					vm->CondFlag = mem->Int32 < regs[regids >> 8].Int32;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = mem->Int64 < regs[regids >> 8].Int64;
				DISPATCH();
			}
		}
	}
	exec_igt:; {
		if( decode.addrmode & Reserved ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				vm->CondFlag = regs[regid].Int64 > *pc.Int64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				vm->CondFlag = regs[regids & 255].Int64 > regs[regids >> 8].Int64;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					vm->CondFlag = regs[regids & 255].Int8 > mem->Int8;
				else if( decode.addrmode & TwoBytes )
					vm->CondFlag = regs[regids & 255].Int16 > mem->Int16;
				else if( decode.addrmode & FourBytes )
					vm->CondFlag = regs[regids & 255].Int32 > mem->Int32;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = regs[regids & 255].Int64 > mem->Int64;
				DISPATCH();
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regid].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					vm->CondFlag = mem->Int8 > imm.Int8;
				else if( decode.addrmode & TwoBytes )
					vm->CondFlag = mem->Int16 > imm.Int16;
				else if( decode.addrmode & FourBytes )
					vm->CondFlag = mem->Int32 > imm.Int32;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = mem->Int64 > imm.Int64;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					vm->CondFlag = mem->Int8 > regs[regids >> 8].Int8;
				else if( decode.addrmode & TwoBytes )
					vm->CondFlag = mem->Int16 > regs[regids >> 8].Int16;
				else if( decode.addrmode & FourBytes )
					vm->CondFlag = mem->Int32 > regs[regids >> 8].Int32;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = mem->Int64 > regs[regids >> 8].Int64;
				DISPATCH();
			}
		}
	}
	exec_ult:; {
		if( decode.addrmode & Reserved ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				vm->CondFlag = regs[regid].UInt64 < *pc.UInt64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				vm->CondFlag = regs[regids & 255].UInt64 < regs[regids >> 8].UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					vm->CondFlag = regs[regids & 255].UInt8 < mem->UInt8;
				else if( decode.addrmode & TwoBytes )
					vm->CondFlag = regs[regids & 255].UInt16 < mem->UInt16;
				else if( decode.addrmode & FourBytes )
					vm->CondFlag = regs[regids & 255].UInt32 < mem->UInt32;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = regs[regids & 255].UInt64 < mem->UInt64;
				DISPATCH();
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regid].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					vm->CondFlag = mem->UInt8 < imm.UInt8;
				else if( decode.addrmode & TwoBytes )
					vm->CondFlag = mem->UInt16 < imm.UInt16;
				else if( decode.addrmode & FourBytes )
					vm->CondFlag = mem->UInt32 < imm.UInt32;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = mem->UInt64 < imm.UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					vm->CondFlag = mem->UInt8 < regs[regids >> 8].UInt8;
				else if( decode.addrmode & TwoBytes )
					vm->CondFlag = mem->UInt16 < regs[regids >> 8].UInt16;
				else if( decode.addrmode & FourBytes )
					vm->CondFlag = mem->UInt32 < regs[regids >> 8].UInt32;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = mem->UInt64 < regs[regids >> 8].UInt64;
				DISPATCH();
			}
		}
	}
	exec_ugt:; {
		if( decode.addrmode & Reserved ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				vm->CondFlag = regs[regid].UInt64 > *pc.UInt64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				vm->CondFlag = regs[regids & 255].UInt64 > regs[regids >> 8].UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					vm->CondFlag = regs[regids & 255].UInt8 > mem->UInt8;
				else if( decode.addrmode & TwoBytes )
					vm->CondFlag = regs[regids & 255].UInt16 > mem->UInt16;
				else if( decode.addrmode & FourBytes )
					vm->CondFlag = regs[regids & 255].UInt32 > mem->UInt32;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = regs[regids & 255].UInt64 > mem->UInt64;
				DISPATCH();
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regid].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					vm->CondFlag = mem->UInt8 > imm.UInt8;
				else if( decode.addrmode & TwoBytes )
					vm->CondFlag = mem->UInt16 > imm.UInt16;
				else if( decode.addrmode & FourBytes )
					vm->CondFlag = mem->UInt32 > imm.UInt32;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = mem->UInt64 > imm.UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					vm->CondFlag = mem->UInt8 > regs[regids >> 8].UInt8;
				else if( decode.addrmode & TwoBytes )
					vm->CondFlag = mem->UInt16 > regs[regids >> 8].UInt16;
				else if( decode.addrmode & FourBytes )
					vm->CondFlag = mem->UInt32 > regs[regids >> 8].UInt32;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = mem->UInt64 > regs[regids >> 8].UInt64;
				DISPATCH();
			}
		}
	}
	exec_cmp:; {
		if( decode.addrmode & Reserved ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				vm->CondFlag = regs[regid].UInt64 == *pc.UInt64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				vm->CondFlag = regs[regids & 255].UInt64 == regs[regids >> 8].UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					vm->CondFlag = regs[regids & 255].UInt8 == mem->UInt8;
				else if( decode.addrmode & TwoBytes )
					vm->CondFlag = regs[regids & 255].UInt16 == mem->UInt16;
				else if( decode.addrmode & FourBytes )
					vm->CondFlag = regs[regids & 255].UInt32 == mem->UInt32;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = regs[regids & 255].UInt64 == mem->UInt64;
				DISPATCH();
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regid].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					vm->CondFlag = mem->UInt8 == imm.UInt8;
				else if( decode.addrmode & TwoBytes )
					vm->CondFlag = mem->UInt16 == imm.UInt16;
				else if( decode.addrmode & FourBytes )
					vm->CondFlag = mem->UInt32 == imm.UInt32;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = mem->UInt64 == imm.UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					vm->CondFlag = mem->UInt8 == regs[regids >> 8].UInt8;
				else if( decode.addrmode & TwoBytes )
					vm->CondFlag = mem->UInt16 == regs[regids >> 8].UInt16;
				else if( decode.addrmode & FourBytes )
					vm->CondFlag = mem->UInt32 == regs[regids >> 8].UInt32;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = mem->UInt64 == regs[regids >> 8].UInt64;
				DISPATCH();
			}
		}
	}
	exec_neq:; {
		if( decode.addrmode & Reserved ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				vm->CondFlag = regs[regid].UInt64 != *pc.UInt64Ptr++;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				vm->CondFlag = regs[regids & 255].UInt64 != regs[regids >> 8].UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					vm->CondFlag = regs[regids & 255].UInt8 != mem->UInt8;
				else if( decode.addrmode & TwoBytes )
					vm->CondFlag = regs[regids & 255].UInt16 != mem->UInt16;
				else if( decode.addrmode & FourBytes )
					vm->CondFlag = regs[regids & 255].UInt32 != mem->UInt32;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = regs[regids & 255].UInt64 != mem->UInt64;
				DISPATCH();
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regid].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					vm->CondFlag = mem->UInt8 != imm.UInt8;
				else if( decode.addrmode & TwoBytes )
					vm->CondFlag = mem->UInt16 != imm.UInt16;
				else if( decode.addrmode & FourBytes )
					vm->CondFlag = mem->UInt32 != imm.UInt32;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = mem->UInt64 != imm.UInt64;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & Byte )
					vm->CondFlag = mem->UInt8 != regs[regids >> 8].UInt8;
				else if( decode.addrmode & TwoBytes )
					vm->CondFlag = mem->UInt16 != regs[regids >> 8].UInt16;
				else if( decode.addrmode & FourBytes )
					vm->CondFlag = mem->UInt32 != regs[regids >> 8].UInt32;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = mem->UInt64 != regs[regids >> 8].UInt64;
				DISPATCH();
			}
		}
	}
	exec_jmp:; {
		if( decode.addrmode & Immediate ) {
			const int64_t offset = *pc.Int64Ptr++;
			pc.UInt8Ptr += offset;
			DISPATCH();
		}
		else if( decode.addrmode & Register ) {
			const uint8_t regid = *pc.UInt8Ptr++;
			pc.UInt8Ptr += regs[regid].Int64;
			DISPATCH();
		}
		else if( decode.addrmode & RegIndirect ) {
			const uint8_t regid = *pc.UInt8Ptr++;
			union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regid].UInt8Ptr + *pc.Int32Ptr++);
			
			if( decode.addrmode & Byte )
				pc.UInt8Ptr += mem->Int8;
			else if( decode.addrmode & TwoBytes )
				pc.UInt8Ptr += mem->Int16;
			else if( decode.addrmode & FourBytes )
				pc.UInt8Ptr += mem->Int32;
			else if( decode.addrmode & EightBytes )
				pc.UInt8Ptr += mem->Int64;
			DISPATCH();
		}
	}
	exec_jz:; {
		if( decode.addrmode & Immediate ) {
			const int64_t offset = *pc.Int64Ptr++;
			!vm->CondFlag ? (pc.UInt8Ptr += offset) : (void)vm->CondFlag;
			DISPATCH();
		}
		else if( decode.addrmode & Register ) {
			const uint8_t regid = *pc.UInt8Ptr++;
			!vm->CondFlag ? (pc.UInt8Ptr += regs[regid].Int64) : (void)vm->CondFlag;
			DISPATCH();
		}
		else if( decode.addrmode & RegIndirect ) {
			const uint8_t regid = *pc.UInt8Ptr++;
			union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regid].UInt8Ptr + *pc.Int32Ptr++);
			if( decode.addrmode & Byte )
				!vm->CondFlag ? (pc.UInt8Ptr += mem->Int8) : (void)vm->CondFlag;
			else if( decode.addrmode & TwoBytes )
				!vm->CondFlag ? (pc.UInt8Ptr += mem->Int16) : (void)vm->CondFlag;
			else if( decode.addrmode & FourBytes )
				!vm->CondFlag ? (pc.UInt8Ptr += mem->Int32) : (void)vm->CondFlag;
			else if( decode.addrmode & EightBytes )
				!vm->CondFlag ? (pc.UInt8Ptr += mem->Int64) : (void)vm->CondFlag;
			DISPATCH();
		}
	}
	exec_jnz:; {
		if( decode.addrmode & Immediate ) {
			const int64_t offset = *pc.Int64Ptr++;
			vm->CondFlag ? (pc.UInt8Ptr += offset) : (void)vm->CondFlag;
			DISPATCH();
		}
		else if( decode.addrmode & Register ) {
			const uint8_t regid = *pc.UInt8Ptr++;
			vm->CondFlag ? (pc.UInt8Ptr += regs[regid].Int64) : (void)vm->CondFlag;
			DISPATCH();
		}
		else if( decode.addrmode & RegIndirect ) {
			const uint8_t regid = *pc.UInt8Ptr++;
			union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regid].UInt8Ptr + *pc.Int32Ptr++);
			if( decode.addrmode & Byte )
				vm->CondFlag ? (pc.UInt8Ptr += mem->Int8) : (void)vm->CondFlag;
			else if( decode.addrmode & TwoBytes )
				vm->CondFlag ? (pc.UInt8Ptr += mem->Int16) : (void)vm->CondFlag;
			else if( decode.addrmode & FourBytes )
				vm->CondFlag ? (pc.UInt8Ptr += mem->Int32) : (void)vm->CondFlag;
			else if( decode.addrmode & EightBytes )
				vm->CondFlag ? (pc.UInt8Ptr += mem->Int64) : (void)vm->CondFlag;
			DISPATCH();
		}
		DISPATCH();
	}
	exec_call:; {
		uint64_t index = -1;
		if( decode.addrmode & Immediate ) {
			index = ((*pc.UInt64Ptr++) - 1);
		}
		else if( decode.addrmode & Register ) {
			index = (regs[*pc.UInt8Ptr++].UInt64 - 1);
		}
		else if( decode.addrmode & RegIndirect ) {
			const uint8_t regid = *pc.UInt8Ptr++;
			union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regid].UInt8Ptr + *pc.Int32Ptr++);
			index = (mem->UInt64 - 1);
		}
		
		uint8_t *const call_addr = GetFunctionOffsetByIndex((uint8_t *)vm->Header, index);
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
	exec_ret:; {
		vm->regStk = vm->regBase; /* mov rsp, rbp */
		vm->regBase = *vm->regStk.SelfPtr++; /* pop rbp */
		pc.Ptr = (*vm->regStk.SelfPtr++).Ptr; /* pop rip */
		if( !pc.Ptr ) {
			goto *dispatch[halt];
		}
		else { DISPATCH(); }
	}
	
	exec_syscall:; {
		/* how many args given to the native call. */
		//const uint32_t argcount = *pc.UInt32Ptr++;
		// syscall args size moved to alaf register.
		uint64_t index = -1;
		/* trying to directly call a specific native. Allow this by imm only! */
		if( decode.addrmode & Immediate ) {
			index = (-1 - *pc.Int64Ptr++);
		}
		else if( decode.addrmode & Register ) {
			index = (-1 - regs[*pc.UInt8Ptr++].Int64);
		}
		else if( decode.addrmode & RegIndirect ) {
			const uint8_t regid = *pc.UInt8Ptr++;
			union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regid].UInt8Ptr + *pc.Int32Ptr++);
			index = (-1 - mem->Int64);
		}
		/* native call interface
		 * Limitations:
		 *  - any argument larger than 8 bytes must be passed as a pointer.
		 *  - any return value larger than 8 bytes must be passed as a hidden pointer argument && render the function as void.
		 * void NativeFunc(struct Tagha *sys, union TaghaVal *retval, const size_t args, union TaghaVal params[static args]);
		 */
		void (*const nativeref)(struct Tagha *, union TaghaVal *, size_t, union TaghaVal[]) = GetNativeByIndex((uint8_t *)vm->Header, index);
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
				(*nativeref)(vm, &vm->regAlaf, argcount, regs+reg_param_initial);
			}
			/* if the native has more than a certain num of params, get from both registers && stack. */
			else InvokeNative(vm, argcount, nativeref);
			DISPATCH();
		}
	}
#if FLOATING_POINT_OPS
	exec_flt2dbl:; {
		if( decode.addrmode & Register ) {
			const uint8_t regid = *pc.UInt8Ptr++;
			const float f = regs[regid].Float;
			regs[regid].Double = (double)f;
			DISPATCH();
		}
	}
	exec_dbl2flt:; {
		if( decode.addrmode & Register ) {
			const uint8_t regid = *pc.UInt8Ptr++;
			const double d = regs[regid].Double;
			regs[regid].UInt64 = 0;
			regs[regid].Float = (float)d;
			DISPATCH();
		}
	}
	exec_int2dbl:; {
		if( decode.addrmode & Register ) {
			const uint8_t regid = *pc.UInt8Ptr++;
			const uint64_t i = regs[regid].UInt64;
			regs[regid].Double = (double)i;
			DISPATCH();
		}
	}
	exec_int2flt:; {
		if( decode.addrmode & Register ) {
			const uint8_t regid = *pc.UInt8Ptr++;
			const uint64_t i = regs[regid].UInt64;
			regs[regid].UInt64 = 0;
			regs[regid].Float = (float)i;
			DISPATCH();
		}
	}
	exec_addf:; {
		if( decode.addrmode & Reserved ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				if( decode.addrmode & FourBytes )
					regs[regid].Float += imm.Float;
				else if( decode.addrmode & EightBytes )
					regs[regid].Double += imm.Double;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				if( decode.addrmode & FourBytes )
					regs[regids & 255].Float += regs[regids >> 8].Float;
				else if( decode.addrmode & EightBytes )
					regs[regids & 255].Double += regs[regids >> 8].Double;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++);
				
				if( decode.addrmode & FourBytes )
					regs[regids & 255].Float += mem->Float;
				else if( decode.addrmode & EightBytes )
					regs[regids & 255].Double += mem->Double;
				DISPATCH();
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regid].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & FourBytes )
					mem->Float += imm.Float;
				else if( decode.addrmode & EightBytes )
					mem->Double += imm.Double;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & FourBytes )
					mem->Float += regs[regids >> 8].Float;
				else if( decode.addrmode & EightBytes )
					mem->Double += regs[regids >> 8].Double;
				DISPATCH();
			}
		}
	}
	exec_subf:; {
		if( decode.addrmode & Reserved ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				if( decode.addrmode & FourBytes )
					regs[regid].Float -= imm.Float;
				else if( decode.addrmode & EightBytes )
					regs[regid].Double -= imm.Double;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				if( decode.addrmode & FourBytes )
					regs[regids & 255].Float -= regs[regids >> 8].Float;
				else if( decode.addrmode & EightBytes )
					regs[regids & 255].Double -= regs[regids >> 8].Double;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++);
				
				if( decode.addrmode & FourBytes )
					regs[regids & 255].Float -= mem->Float;
				else if( decode.addrmode & EightBytes )
					regs[regids & 255].Double -= mem->Double;
				DISPATCH();
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regid].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & FourBytes )
					mem->Float -= imm.Float;
				else if( decode.addrmode & EightBytes )
					mem->Double -= imm.Double;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & FourBytes )
					mem->Float -= regs[regids >> 8].Float;
				else if( decode.addrmode & EightBytes )
					mem->Double -= regs[regids >> 8].Double;
				DISPATCH();
			}
		}
	}
	exec_mulf:; {
		if( decode.addrmode & Reserved ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				if( decode.addrmode & FourBytes )
					regs[regid].Float *= imm.Float;
				else if( decode.addrmode & EightBytes )
					regs[regid].Double *= imm.Double;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				if( decode.addrmode & FourBytes )
					regs[regids & 255].Float *= regs[regids >> 8].Float;
				else if( decode.addrmode & EightBytes )
					regs[regids & 255].Double *= regs[regids >> 8].Double;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++);
				
				if( decode.addrmode & FourBytes )
					regs[regids & 255].Float *= mem->Float;
				else if( decode.addrmode & EightBytes )
					regs[regids & 255].Double *= mem->Double;
				DISPATCH();
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regid].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & FourBytes )
					mem->Float *= imm.Float;
				else if( decode.addrmode & EightBytes )
					mem->Double *= imm.Double;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & FourBytes )
					mem->Float *= regs[regids >> 8].Float;
				else if( decode.addrmode & EightBytes )
					mem->Double *= regs[regids >> 8].Double;
				DISPATCH();
			}
		}
	}
	exec_divf:; {
		if( decode.addrmode & Reserved ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				if( decode.addrmode & FourBytes )
					regs[regid].Float /= imm.Float;
				else if( decode.addrmode & EightBytes )
					regs[regid].Double /= imm.Double;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				if( decode.addrmode & FourBytes )
					regs[regids & 255].Float /= regs[regids >> 8].Float;
				else if( decode.addrmode & EightBytes )
					regs[regids & 255].Double /= regs[regids >> 8].Double;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++);
				
				if( decode.addrmode & FourBytes )
					regs[regids & 255].Float /= mem->Float;
				else if( decode.addrmode & EightBytes )
					regs[regids & 255].Double /= mem->Double;
				DISPATCH();
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regid].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & FourBytes )
					mem->Float /= imm.Float;
				else if( decode.addrmode & EightBytes )
					mem->Double /= imm.Double;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & FourBytes )
					mem->Float /= regs[regids >> 8].Float;
				else if( decode.addrmode & EightBytes )
					mem->Double /= regs[regids >> 8].Double;
				DISPATCH();
			}
		}
	}
	exec_ltf:; {
		if( decode.addrmode & Reserved ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				if( decode.addrmode & FourBytes )
					vm->CondFlag = regs[regid].Float < imm.Float;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = regs[regid].Double < imm.Double;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				if( decode.addrmode & FourBytes )
					vm->CondFlag = regs[regids & 255].Float < regs[regids >> 8].Float;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = regs[regids & 255].Double < regs[regids >> 8].Double;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++);
				
				if( decode.addrmode & FourBytes )
					vm->CondFlag = regs[regids & 255].Float < mem->Float;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = regs[regids & 255].Double < mem->Double;
				DISPATCH();
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regid].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & FourBytes )
					vm->CondFlag = mem->Float < imm.Float;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = mem->Double < imm.Double;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & FourBytes )
					vm->CondFlag = mem->Float < regs[regids >> 8].Float;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = mem->Double < regs[regids >> 8].Double;
				DISPATCH();
			}
		}
	}
	exec_gtf:; {
		if( decode.addrmode & Reserved ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				if( decode.addrmode & FourBytes )
					vm->CondFlag = regs[regid].Float > imm.Float;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = regs[regid].Double > imm.Double;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				if( decode.addrmode & FourBytes )
					vm->CondFlag = regs[regids & 255].Float > regs[regids >> 8].Float;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = regs[regids & 255].Double > regs[regids >> 8].Double;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++);
				
				if( decode.addrmode & FourBytes )
					vm->CondFlag = regs[regids & 255].Float > mem->Float;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = regs[regids & 255].Double > mem->Double;
				DISPATCH();
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regid].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & FourBytes )
					vm->CondFlag = mem->Float > imm.Float;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = mem->Double > imm.Double;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & FourBytes )
					vm->CondFlag = mem->Float > regs[regids >> 8].Float;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = mem->Double > regs[regids >> 8].Double;
				DISPATCH();
			}
		}
	}
	exec_cmpf:; {
		if( decode.addrmode & Reserved ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				if( decode.addrmode & FourBytes )
					vm->CondFlag = regs[regid].Float == imm.Float;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = regs[regid].Double == imm.Double;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				if( decode.addrmode & FourBytes )
					vm->CondFlag = regs[regids & 255].Float == regs[regids >> 8].Float;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = regs[regids & 255].Double == regs[regids >> 8].Double;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++);
				
				if( decode.addrmode & FourBytes )
					vm->CondFlag = regs[regids & 255].Float == mem->Float;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = regs[regids & 255].Double == mem->Double;
				DISPATCH();
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regid].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & FourBytes )
					vm->CondFlag = mem->Float == imm.Float;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = mem->Double == imm.Double;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & FourBytes )
					vm->CondFlag = mem->Float == regs[regids >> 8].Float;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = mem->Double == regs[regids >> 8].Double;
				DISPATCH();
			}
		}
	}
	exec_neqf:; {
		if( decode.addrmode & Reserved ) {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				if( decode.addrmode & FourBytes )
					vm->CondFlag = regs[regid].Float != imm.Float;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = regs[regid].Double != imm.Double;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				if( decode.addrmode & FourBytes )
					vm->CondFlag = regs[regids & 255].Float != regs[regids >> 8].Float;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = regs[regids & 255].Double != regs[regids >> 8].Double;
				DISPATCH();
			}
			else if( decode.addrmode & RegIndirect ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids >> 8].UInt8Ptr + *pc.Int32Ptr++);
				
				if( decode.addrmode & FourBytes )
					vm->CondFlag = regs[regids & 255].Float != mem->Float;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = regs[regids & 255].Double != mem->Double;
				DISPATCH();
			}
		}
		else {
			if( decode.addrmode & Immediate ) {
				const uint8_t regid = *pc.UInt8Ptr++;
				const union TaghaVal imm = *pc.ValPtr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regid].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & FourBytes )
					vm->CondFlag = mem->Float != imm.Float;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = mem->Double != imm.Double;
				DISPATCH();
			}
			else if( decode.addrmode & Register ) {
				const uint16_t regids = *pc.UInt16Ptr++;
				union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regids & 255].UInt8Ptr + *pc.Int32Ptr++);
				if( decode.addrmode & FourBytes )
					vm->CondFlag = mem->Float != regs[regids >> 8].Float;
				else if( decode.addrmode & EightBytes )
					vm->CondFlag = mem->Double != regs[regids >> 8].Double;
				DISPATCH();
			}
		}
	}
	exec_incf:; {
		const uint8_t regid = *pc.UInt8Ptr++;
		if( decode.addrmode & Register ) {
			if( decode.addrmode & FourBytes )
				++regs[regid].Float;
			else if( decode.addrmode & EightBytes )
				++regs[regid].Double;
			DISPATCH();
		}
		else if( decode.addrmode & RegIndirect ) {
			union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regid].UInt8Ptr + *pc.Int32Ptr++);
			if( decode.addrmode & FourBytes )
				++mem->Float;
			else if( decode.addrmode & EightBytes )
				++mem->Double;
			DISPATCH();
		}
	}
	exec_decf:; {
		const uint8_t regid = *pc.UInt8Ptr++;
		if( decode.addrmode & Register ) {
			if( decode.addrmode & FourBytes )
				--regs[regid].Float;
			else if( decode.addrmode & EightBytes )
				--regs[regid].Double;
			DISPATCH();
		}
		else if( decode.addrmode & RegIndirect ) {
			union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regid].UInt8Ptr + *pc.Int32Ptr++);
			if( decode.addrmode & FourBytes )
				--mem->Float;
			else if( decode.addrmode & EightBytes )
				--mem->Double;
			DISPATCH();
		}
	}
	exec_negf:; {
		const uint8_t regid = *pc.UInt8Ptr++;
		if( decode.addrmode & Register ) {
			if( decode.addrmode & FourBytes )
				regs[regid].Float = -regs[regid].Float;
			else if( decode.addrmode & EightBytes )
				regs[regid].Double = -regs[regid].Double;
			DISPATCH();
		}
		else if( decode.addrmode & RegIndirect ) {
			union TaghaVal *const restrict mem = (union TaghaVal *)(regs[regid].UInt8Ptr + *pc.Int32Ptr++);
			if( decode.addrmode & FourBytes )
				mem->Float = -mem->Float;
			else if( decode.addrmode & EightBytes )
				mem->Double = -mem->Double;
			DISPATCH();
		}
	}
#endif
	exec_halt:;
	//Tagha_PrintVMState(vm);
	return vm->regAlaf.Int32;
}

int32_t Tagha_RunScript(struct Tagha *const restrict vm, const int32_t argc, char *argv[restrict static argc+1])
{
	if( !vm )
		return -1;
	
	struct TaghaHeader *const restrict hdr = vm->Header;
	if( !hdr || hdr->Magic != 0xC0DE ) {
		vm->Error = ErrInvalidScript;
		return -1;
	}
	
	uint8_t *const restrict main_offset = GetFunctionOffsetByName((uint8_t *)hdr, "main");
	if( !main_offset ) {
		vm->Error = ErrMissingFunc;
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
	
	/* check out stack size && align it by the size of union TaghaVal. */
	const size_t stacksize = hdr->StackSize; //(hdr->StackSize + (sizeof(union TaghaVal)-1)) & -(sizeof(union TaghaVal));
	if( !stacksize ) {
		vm->Error = ErrStackSize;
		return -1;
	}
	/*
	union TaghaVal reader = (union TaghaVal){.Ptr = (char *)hdr + 7};
	const uint32_t vartable_offset = *reader.UInt32Ptr++;
	reader.UInt8Ptr += vartable_offset;
	
	const uint32_t globalvars = *reader.UInt32Ptr++;
	for( uint32_t i=0 ; i<globalvars ; i++ ) {
		reader.UInt8Ptr++;
		const uint64_t sizes = *reader.UInt64Ptr++;
		reader.UInt8Ptr += ((sizes & 0xffFFffFF) + (sizes >> 32));
	}
	reader.UInt8Ptr++;
	*/
	//vm->regs[regStk].Ptr = vm->regs[regBase].Ptr = reader.UInt8Ptr + stacksize;
	
	union TaghaVal Stack[stacksize+1]; memset(Stack, 0, sizeof Stack[0] * stacksize+1);
	vm->regStk.SelfPtr = vm->regBase.SelfPtr = Stack + stacksize;
	
	(--vm->regStk.SelfPtr)->Int64 = 0LL;	/* push NULL ret address. */
	*--vm->regStk.SelfPtr = vm->regBase; /* push rbp */
	vm->regBase = vm->regStk; /* mov rbp, rsp */
	vm->regInstr.UInt8Ptr = main_offset;
	
	return Tagha_Exec(vm);
}

int32_t Tagha_CallFunc(struct Tagha *const restrict vm, const char *restrict funcname, const size_t args, union TaghaVal values[static args])
{
	if( !vm || !funcname || !values )
		return -1;
	
	struct TaghaHeader *const restrict hdr = vm->Header;
	if( !hdr || hdr->Magic != 0xC0DE ) {
		vm->Error = ErrInvalidScript;
		return -1;
	}
	
	uint8_t *const restrict func_offset = GetFunctionOffsetByName((uint8_t *)hdr, funcname);
	if( !func_offset ) {
		vm->Error = ErrMissingFunc;
		return -1;
	}
	
	/* check out stack size && align it by the size of union TaghaVal. */
	const size_t stacksize = hdr->StackSize; //(hdr->StackSize + (sizeof(union TaghaVal)-1)) & -(sizeof(union TaghaVal));
	if( !stacksize ) {
		vm->Error = ErrStackSize;
		return -1;
	}
	
	union TaghaVal Stack[stacksize+1]; memset(Stack, 0, sizeof Stack[0] * stacksize+1);
	vm->regStk.SelfPtr = vm->regBase.SelfPtr = Stack + stacksize;
	
	/* remember that arguments must be passed right to left.
	 we have enough args to fit in registers. */
	const uint8_t reg_params = 8;
	const uint8_t reg_param_initial = regSemkath;
	const uint16_t bytecount = sizeof(union TaghaVal) * args;
	
	/* save stack space by using the registers for passing arguments. */
	/* the other registers can be used for other data ops. */
	if( args <= reg_params ) {
		memcpy(vm->Regs+reg_param_initial, values, bytecount);
	}
	/* if the function has more than a certain num of params, push from both registers && stack. */
	else if( args > reg_params ) {
		memcpy(vm->Regs+reg_param_initial, values, sizeof Stack[0] * reg_params);
		memcpy(vm->regStk.SelfPtr, values+reg_params, sizeof Stack[0] * (args-reg_params));
		vm->regStk.SelfPtr -= (args-reg_params);
	}
	
	*--vm->regStk.SelfPtr = vm->regInstr;	/* push return address. */
	*--vm->regStk.SelfPtr = vm->regBase; /* push rbp */
	vm->regBase = vm->regStk; /* mov rbp, rsp */
	vm->regInstr.Ptr = func_offset;
	return Tagha_Exec(vm);
}

union TaghaVal Tagha_GetReturnValue(const struct Tagha *const vm)
{
	return vm ? vm->regAlaf : (union TaghaVal){0};
}

void *Tagha_GetGlobalVarByName(struct Tagha *const restrict vm, const char *restrict varname)
{
	return !vm || !varname ? NULL : GetVariableOffsetByName((uint8_t *)vm->Header, varname);
}

const char *Tagha_GetError(const struct Tagha *const vm)
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

/************************************************/



