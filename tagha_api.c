
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "tagha.h"

inline static void			*GetFunctionOffsetByName(void *, const char *, bool *);
inline static void			*GetFunctionOffsetByIndex(void *, size_t, bool *);
inline static size_t		GetFunctionIndexByName(void *, const char *, bool *);
inline static const char	*GetFunctionNameByIndex(void *, size_t, bool *);

inline static void			*GetVariableOffsetByName(void *, const char *);
inline static void			*GetVariableOffsetByIndex(void *, size_t);
inline static size_t		GetVariableIndexByName(void *, const char *);
inline static const char	*GetVariableNameByIndex(void *, size_t);


/* void *Tagha_LoadModule(const char *tbc_module_name); */
static void Native_TaghaLoadModule(struct Tagha *const restrict sys, union Value *const restrict retval, const size_t args, union Value params[static args])
{
	(void)sys; (void)args;
	const char *restrict module_name = params[0].Ptr;
	puts(module_name);
	
	FILE *tbcfile = fopen(module_name, "rb");
	if( !tbcfile ) {
		printf("Tagha_LoadModule :: cannot find module '%s'\n", module_name);
		return;
	}
	
	size_t filesize = 0L;
	if( !fseek(tbcfile, 0, SEEK_END) ) {
		int64_t size = ftell(tbcfile);
		if( size == -1LL ) {
			printf("Tagha_LoadModule :: cannot read module file '%s'!\n", module_name);
			fclose(tbcfile), tbcfile=NULL;
			return;
		}
		rewind(tbcfile);
		filesize = (size_t) size;
	}
	
	uint8_t *module = calloc(filesize, sizeof *module);
	if( !module ) {
		printf("Tagha_LoadModule :: failed to load module (\"%s\") into memory!\n", module_name);
		fclose(tbcfile), tbcfile=NULL;
		return;
	}
	const size_t val = fread(module, sizeof(uint8_t), filesize, tbcfile);
	(void)val;
	fclose(tbcfile), tbcfile=NULL;
	
	if( *(uint16_t *)module != 0xC0DE ) {
		printf("Tagha_LoadModule :: module (\"%s\") is not a valid TBC script!\n", module_name);
		free(module), module=NULL;
		return;
	}
	retval->Ptr = module;
}

/* void *Tagha_GetGlobal(void *module, const char *symname); */
static void Native_TaghaGetGlobal(struct Tagha *const restrict sys, union Value *const restrict retval, const size_t args, union Value params[static args])
{
	(void)sys; (void)args;
	void *restrict module = params[0].Ptr;
	const char *restrict symname = params[1].Ptr;
	puts(symname);
	
	retval->Ptr = GetVariableOffsetByName(module, symname);
}

/* bool Tagha_InvokeFunc(void *, const char *, union Value *, size_t, union Value []); */
static void Native_TaghaInvoke(struct Tagha *const restrict sys, union Value *const restrict retval, const size_t args, union Value params[static args])
{
	(void)sys; (void)args;
	void *module = params[0].Ptr;
	if( !module ) {
		puts("Tagha_InvokeFunc :: module ptr is NULL!");
		retval->Bool = false;
		return;
	}
	const char *funcname = params[1].Ptr;
	if( !funcname ) {
		puts("Tagha_InvokeFunc :: funcname ptr is NULL!");
		retval->Bool = false;
		return;
	}
	const size_t argcount = params[3].UInt64;
	union Value *array = params[4].SelfPtr;
	if( !array ) {
		puts("Tagha_InvokeFunc :: array ptr is NULL!");
		retval->Bool = false;
		return;
	}
	
	/* do a context switch to run the function. */
	struct Tagha context_switch;
	Tagha_Init(&context_switch, module);
	
	Tagha_CallFunc(&context_switch, funcname, argcount, array);
	*params[2].SelfPtr = context_switch.Regs[regAlaf];
	
	retval->Bool = true;
}
/* void Tagha_FreeModule(void **module); */
static void Native_TaghaFreeModule(struct Tagha *const restrict sys, union Value *const restrict retval, const size_t args, union Value params[static args])
{
	(void)sys; (void)args; (void)retval;
	void **module = params[0].PtrPtr;
	if( !module or !*module ) {
		puts("Tagha_FreeModule :: module reference is NULL!");
		return;
	}
	free(*module), *module = NULL;
}


void Tagha_Init(struct Tagha *const restrict vm, void *script)
{
	if( !vm )
		return;
	
	*vm = (struct Tagha){0};
	vm->CurrScript.Ptr = script;
	
	FILE **stdinref = GetVariableOffsetByName(script, "stdin");
	if( stdinref )
		*stdinref = stdin;
	
	FILE **stderrref = GetVariableOffsetByName(script, "stderr");
	if( stderrref )
		*stderrref = stderr;
		
	FILE **stdoutref = GetVariableOffsetByName(script, "stderr");
	if( stdoutref )
		*stdoutref = stdout;
	
	struct NativeInfo dynamic_loading[] = {
		{"Tagha_LoadModule", Native_TaghaLoadModule},
		{"Tagha_GetGlobal", Native_TaghaGetGlobal},
		{"Tagha_InvokeFunc", Native_TaghaInvoke},
		{"Tagha_FreeModule", Native_TaghaFreeModule},
		{NULL, NULL}
	};
	Tagha_RegisterNatives(vm, dynamic_loading);
}

void Tagha_InitN(struct Tagha *const restrict vm, void *script, struct NativeInfo natives[])
{
	Tagha_Init(vm, script);
	Tagha_RegisterNatives(vm, natives);
}


void TaghaDebug_PrintRegisters(const struct Tagha *const restrict vm)
{
	if( !vm )
		return;
	for( size_t i=0 ; i<regsize ; i++ )
		printf("register[%zu] == '%" PRIu64 "'\n", i, vm->Regs[i].UInt64);
	puts("\n");
}

bool Tagha_RegisterNatives(struct Tagha *const restrict vm, struct NativeInfo natives[])
{
	if( !vm or !natives )
		return false;
	
	union Value func_addr = (union Value){0};
	bool check_native = false;
	for( struct NativeInfo *n=natives ; n->NativeCFunc and n->Name ; n++ ) {
		func_addr.Ptr = GetFunctionOffsetByName(vm->CurrScript.Ptr, n->Name, &check_native);
		if( func_addr.Ptr and check_native )
			func_addr.SelfPtr->VoidFunc = n->NativeCFunc;
	}
	return true;
}

inline static void *GetFunctionOffsetByName(void *script, const char *restrict funcname, bool *const restrict isnative)
{
	if( !funcname or !script )
		return NULL;
	
	union Value reader = (union Value){.Ptr = script};
	reader.UCharPtr += 11;
	
	const size_t funcs = *reader.UInt32Ptr++;
	for( size_t i=0 ; i<funcs ; i++ ) {
		if( isnative )
			*isnative = *reader.UCharPtr;
		reader.UCharPtr++;
		const uint64_t sizes = *reader.UInt64Ptr++;
		const size_t stringlen = sizes & 0xffFFffFF;
		const size_t instrlen = sizes >> 32;
		if( !strcmp(funcname, reader.Ptr) )
			return reader.UCharPtr + stringlen;
		
		/* skip to the next */
		reader.UCharPtr += (stringlen + instrlen);
	}
	return NULL;
}

inline static size_t GetFunctionIndexByName(void *script, const char *restrict funcname, bool *const restrict isnative)
{
	if( !funcname or !script )
		return SIZE_MAX;
	
	union Value reader = (union Value){.Ptr = script};
	reader.UCharPtr += 11;
	
	const size_t funcs = *reader.UInt32Ptr++;
	for( size_t i=0 ; i<funcs ; i++ ) {
		if( isnative )
			*isnative = *reader.UCharPtr;
		reader.UCharPtr++;
		const uint64_t sizes = *reader.UInt64Ptr++;
		const size_t stringlen = sizes & 0xffFFffFF;
		const size_t instrlen = sizes >> 32;
		if( !strncmp(funcname, reader.Ptr, stringlen-1) )
			return i;
		
		/* skip to the next */
		reader.UCharPtr += (stringlen + instrlen);
	}
	return SIZE_MAX;
}

inline static void *GetFunctionOffsetByIndex(void *script, const size_t index, bool *const restrict isnative)
{
	if( !script )
		return NULL;
	
	union Value reader = (union Value){.Ptr = script};
	reader.UCharPtr += 11;
	
	const size_t funcs = *reader.UInt32Ptr++;
	if( index >= funcs )
		return NULL;
	
	for( size_t i=0 ; i<funcs ; i++ ) {
		if( isnative )
			*isnative = *reader.UCharPtr;
		reader.UCharPtr++;
		const uint64_t sizes = *reader.UInt64Ptr++;
		const size_t stringlen = sizes & 0xffFFffFF;
		const size_t instrlen = sizes >> 32;
		if( i==index )
			return reader.UCharPtr + stringlen;
		
		/* skip to the next */
		reader.UCharPtr += (stringlen + instrlen);
	}
	return NULL;
}

inline static const char *GetFunctionNameByIndex(void *script, const size_t index, bool *const restrict isnative)
{
	if( !script )
		return NULL;
	
	union Value reader = (union Value){.Ptr = script};
	reader.UCharPtr += 11;
	
	const size_t funcs = *reader.UInt32Ptr++;
	if( index >= funcs )
		return NULL;
	
	for( size_t i=0 ; i<funcs ; i++ ) {
		if( isnative )
			*isnative = *reader.UCharPtr;
		reader.UCharPtr++;
		const uint64_t sizes = *reader.UInt64Ptr++;
		const size_t stringlen = sizes & 0xffFFffFF;
		const size_t instrlen = sizes >> 32;
		if( i==index )
			return reader.Ptr;
		
		/* skip to the next */
		reader.UCharPtr += (stringlen + instrlen);
	}
	return NULL;
}

inline static void *GetVariableOffsetByName(void *script, const char *restrict varname)
{
	if( !script or !varname )
		return NULL;
	
	union Value reader = (union Value){.Ptr = script};
	reader.UCharPtr += 7;
	const size_t vartable_offset = *reader.UInt32Ptr++;
	reader.UCharPtr += vartable_offset;
	
	const size_t globalvars = *reader.UInt32Ptr++;
	for( size_t i=0 ; i<globalvars ; i++ ) {
		reader.UCharPtr++;
		const uint64_t sizes = *reader.UInt64Ptr++;
		const size_t stringlen = sizes & 0xffFFffFF;
		const size_t bytelen = sizes >> 32;
		if( !strncmp(varname, reader.Ptr, stringlen-1) )
			return reader.UCharPtr + stringlen;
		
		/* skip to the next var */
		reader.UCharPtr += (stringlen + bytelen);
	}
	return NULL;
}

inline static size_t GetVariableIndexByName(void *script, const char *restrict varname)
{
	if( !script or !varname )
		return SIZE_MAX;
	
	union Value reader = (union Value){.Ptr = script};
	reader.UCharPtr += 7;
	const size_t vartable_offset = *reader.UInt32Ptr++;
	reader.UCharPtr += vartable_offset;
	
	const size_t globalvars = *reader.UInt32Ptr++;
	for( size_t i=0 ; i<globalvars ; i++ ) {
		reader.UCharPtr++;
		const uint64_t sizes = *reader.UInt64Ptr++;
		const size_t stringlen = sizes & 0xffFFffFF;
		const size_t bytelen = sizes >> 32;
		if( !strncmp(varname, reader.Ptr, stringlen-1) )
			return i;
		
		/* skip to the next var */
		reader.UCharPtr += (stringlen + bytelen);
	}
	return SIZE_MAX;
}

inline static void *GetVariableOffsetByIndex(void *script, const size_t index)
{
	if( !script )
		return NULL;
	
	union Value reader = (union Value){.Ptr = script};
	reader.UCharPtr += 7;
	const size_t vartable_offset = *reader.UInt32Ptr++;
	reader.UCharPtr += vartable_offset;
	
	const size_t globalvars = *reader.UInt32Ptr++;
	if( index >= globalvars )
		return NULL;
	
	for( size_t i=0 ; i<globalvars ; i++ ) {
		reader.UCharPtr++;
		const uint64_t sizes = *reader.UInt64Ptr++;
		const size_t stringlen = sizes & 0xffFFffFF;
		const size_t bytelen = sizes >> 32;
		if( i==index )
			return reader.UCharPtr + stringlen;
		
		/* skip to the next global var index */
		reader.UCharPtr += (stringlen + bytelen);
	}
	return NULL;
}

inline static const char *GetVariableNameByIndex(void *script, const size_t index)
{
	if( !script )
		return NULL;
	
	union Value reader = (union Value){.Ptr = script};
	reader.UCharPtr += 7;
	const size_t vartable_offset = *reader.UInt32Ptr++;
	reader.UCharPtr += vartable_offset;
	
	const size_t globalvars = *reader.UInt32Ptr++;
	if( index >= globalvars )
		return NULL;
	
	for( size_t i=0 ; i<globalvars ; i++ ) {
		reader.UCharPtr++;
		const uint64_t sizes = *reader.UInt64Ptr++;
		const size_t stringlen = sizes & 0xffFFffFF;
		const size_t bytelen = sizes >> 32;
		if( i==index )
			return reader.Ptr;
		
		/* skip to the next */
		reader.UCharPtr += (stringlen + bytelen);
	}
	return NULL;
}

/* #include <unistd.h>	// sleep() func */

int32_t Tagha_Exec(struct Tagha *const restrict vm)
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
	/* for debugging purposes. */
	const char *const restrict opcode2str[] = { INSTR_SET };
#undef X
	
	
#define X(x) &&exec_##x ,
	/* our instruction dispatch table. */
	const void *const restrict dispatch[] = { INSTR_SET };
#undef X
#undef INSTR_SET
	/* #ifdef _UNISTD_H */
	
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
		/*TaghaDebug_PrintRegisters(vm);*/ \
		/*sleep(1);*/ \
		/*printf("dispatching to '%s'\n", opcode2str[instr]);*/ \
		goto *dispatch[instr]
	
	DISPATCH();
	
	exec_halt:;
		return vm->Regs[regAlaf].Int32;
	
	exec_nop:;
		DISPATCH();
	
	/* pushes a value to the top of the stack, raises the stack pointer by 8 bytes.
	 * push reg (1 byte for register id)
	 * push imm (8 bytes for constant values)
	 * push [reg+offset] (1 byte reg id + 4-byte signed offset)
	 */
	exec_push:; {
		/* push an imm constant. */
		if( addrmode & Immediate )
			*--vm->Regs[regStk].SelfPtr = *vm->Regs[regInstr].SelfPtr++;
		/* push a register's contents. */
		else if( addrmode & Register )
			*--vm->Regs[regStk].SelfPtr = vm->Regs[*vm->Regs[regInstr].UCharPtr++];
		/* push the contents of a memory address inside a register. */
		else if( addrmode & RegIndirect ) {
			const uint8_t regid = *vm->Regs[regInstr].UCharPtr++;
			const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
			*--vm->Regs[regStk].SelfPtr = *effective_address.SelfPtr;
		}
		DISPATCH();
	}
	
	/* pops a value from the stack into a register or memory then reduces stack by 8 bytes.
	 * pop reg
	 * pop [reg+offset]
	 */
	exec_pop:; {
		if( addrmode & Immediate )
			vm->Regs[regInstr].SelfPtr++;
		else if( addrmode & Register )
			vm->Regs[*vm->Regs[regInstr].UCharPtr++] = *vm->Regs[regStk].SelfPtr++;
		else if( addrmode & RegIndirect ) {
			const uint8_t regid = *vm->Regs[regInstr].UCharPtr++;
			const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
			*effective_address.SelfPtr = *vm->Regs[regStk].SelfPtr++;
		}
		DISPATCH();
	}
	
	/* loads a ptr value to a register.
	 * lea reg, global var
	 * lea reg, func
	 * lea reg, [reg+offset] (not dereferenced)
	 */
	exec_lea:; {
		const uint8_t regid = *vm->Regs[regInstr].UCharPtr++;
		
		if( addrmode & Immediate ) { /* Immediate mode will load a global value */
			vm->Regs[regid].Ptr = GetVariableOffsetByIndex(vm->CurrScript.Ptr, *vm->Regs[regInstr].UInt64Ptr++);
		}
		else if( addrmode & Register ) { /* Register mode will load a function address which could be a native */
			vm->Regs[regid].Int64 = *vm->Regs[regInstr].Int64Ptr++;
		}
		else if( addrmode & RegIndirect ) {
			const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
			vm->Regs[regid].UCharPtr = vm->Regs[sec_regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++;
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
		const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
		
		/* get secondary addressing mode for the second operand. */
		const uint8_t sec_addrmode = op_args & 255;
		
		/* get 1st register id. */
		const uint8_t regid = op_args >> 8;
		
		if( addrmode & Register ) {
			if( sec_addrmode & Immediate )
				vm->Regs[regid].UInt64 = *vm->Regs[regInstr].UInt64Ptr++;
			else if( sec_addrmode & Register )
				vm->Regs[regid] = vm->Regs[*vm->Regs[regInstr].UCharPtr++];
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
			if( sec_addrmode & Immediate ) {
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
	
	/* adds two values to the destination value which is either a register or memory address.
	 * add reg, [reg+offset]
	 * add reg, imm
	 * add reg, reg
	 * add [reg+offset], reg
	 * add [reg+offset], imm
	 */
	exec_add:; {
		const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
		const uint8_t sec_addrmode = op_args & 255;
		const uint8_t regid = op_args >> 8;
		
		if( addrmode & Register ) {
			if( sec_addrmode & Immediate ) {
				vm->Regs[regid].UInt64 += *vm->Regs[regInstr].UInt64Ptr++;
			}
			else if( sec_addrmode & Register ) {
				const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
				vm->Regs[regid].UInt64 += vm->Regs[sec_regid].UInt64;
			}
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
			if( sec_addrmode & Immediate ) {
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
		const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
		const uint8_t sec_addrmode = op_args & 255;
		const uint8_t regid = op_args >> 8;
		if( addrmode & Register ) {
			if( sec_addrmode & Immediate )
				vm->Regs[regid].UInt64 -= *vm->Regs[regInstr].UInt64Ptr++;
			else if( sec_addrmode & Register )
				vm->Regs[regid].UInt64 -= vm->Regs[*vm->Regs[regInstr].UCharPtr++].UInt64;
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
			if( sec_addrmode & Immediate ) {
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
		const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
		const uint8_t sec_addrmode = op_args & 255;
		const uint8_t regid = op_args >> 8;
		if( addrmode & Register ) {
			if( sec_addrmode & Immediate )
				vm->Regs[regid].UInt64 *= *vm->Regs[regInstr].UInt64Ptr++;
			else if( sec_addrmode & Register )
				vm->Regs[regid].UInt64 *= vm->Regs[*vm->Regs[regInstr].UCharPtr++].UInt64;
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
			if( sec_addrmode & Immediate ) {
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
		const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
		const uint8_t sec_addrmode = op_args & 255;
		const uint8_t regid = op_args >> 8;
		if( addrmode & Register ) {
			if( sec_addrmode & Immediate )
				vm->Regs[regid].UInt64 /= *vm->Regs[regInstr].UInt64Ptr++;
			else if( sec_addrmode & Register )
				vm->Regs[regid].UInt64 /= vm->Regs[*vm->Regs[regInstr].UCharPtr++].UInt64;
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
			if( sec_addrmode & Immediate ) {
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
		const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
		const uint8_t sec_addrmode = op_args & 255;
		const uint8_t regid = op_args >> 8;
		if( addrmode & Register ) {
			if( sec_addrmode & Immediate )
				vm->Regs[regid].UInt64 %= *vm->Regs[regInstr].UInt64Ptr++;
			else if( sec_addrmode & Register )
				vm->Regs[regid].UInt64 %= vm->Regs[*vm->Regs[regInstr].UCharPtr++].UInt64;
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
			if( sec_addrmode & Immediate ) {
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
		const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
		const uint8_t sec_addrmode = op_args & 255;
		const uint8_t regid = op_args >> 8;
		if( addrmode & Register ) {
			if( sec_addrmode & Immediate )
				vm->Regs[regid].UInt64 &= *vm->Regs[regInstr].UInt64Ptr++;
			else if( sec_addrmode & Register )
				vm->Regs[regid].UInt64 &= vm->Regs[*vm->Regs[regInstr].UCharPtr++].UInt64;
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
			if( sec_addrmode & Immediate ) {
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
		const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
		const uint8_t sec_addrmode = op_args & 255;
		const uint8_t regid = op_args >> 8;
		if( addrmode & Register ) {
			if( sec_addrmode & Immediate )
				vm->Regs[regid].UInt64 |= *vm->Regs[regInstr].UInt64Ptr++;
			else if( sec_addrmode & Register )
				vm->Regs[regid].UInt64 |= vm->Regs[*vm->Regs[regInstr].UCharPtr++].UInt64;
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
			if( sec_addrmode & Immediate ) {
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
		const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
		const uint8_t sec_addrmode = op_args & 255;
		const uint8_t regid = op_args >> 8;
		if( addrmode & Register ) {
			if( sec_addrmode & Immediate )
				vm->Regs[regid].UInt64 ^= *vm->Regs[regInstr].UInt64Ptr++;
			else if( sec_addrmode & Register )
				vm->Regs[regid].UInt64 ^= vm->Regs[*vm->Regs[regInstr].UCharPtr++].UInt64;
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
			if( sec_addrmode & Immediate ) {
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
		if( addrmode & Register )
			vm->Regs[regid].UInt64 = ~vm->Regs[regid].UInt64;
		else if( addrmode & RegIndirect ) {
			const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
			*effective_address.UInt64Ptr = ~*effective_address.UInt64Ptr;
		}
		DISPATCH();
	}
	exec_shl:; {
		const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
		const uint8_t sec_addrmode = op_args & 255;
		const uint8_t regid = op_args >> 8;
		if( addrmode & Register ) {
			if( sec_addrmode & Immediate )
				vm->Regs[regid].UInt64 <<= *vm->Regs[regInstr].UInt64Ptr++;
			else if( sec_addrmode & Register )
				vm->Regs[regid].UInt64 <<= vm->Regs[*vm->Regs[regInstr].UCharPtr++].UInt64;
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
			if( sec_addrmode & Immediate ) {
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
		const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
		const uint8_t sec_addrmode = op_args & 255;
		const uint8_t regid = op_args >> 8;
		if( addrmode & Register ) {
			if( sec_addrmode & Immediate )
				vm->Regs[regid].UInt64 >>= *vm->Regs[regInstr].UInt64Ptr++;
			else if( sec_addrmode & Register )
				vm->Regs[regid].UInt64 >>= vm->Regs[*vm->Regs[regInstr].UCharPtr++].UInt64;
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
			if( sec_addrmode & Immediate ) {
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
		if( addrmode & Register )
			++vm->Regs[regid].UInt64;
		else if( addrmode & RegIndirect ) {
			const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
			++*effective_address.UInt64Ptr;
		}
		DISPATCH();
	}
	exec_dec:; {
		const uint8_t regid = *vm->Regs[regInstr].UCharPtr++;
		if( addrmode & Register )
			--vm->Regs[regid].UInt64;
		else if( addrmode & RegIndirect ) {
			const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
			--*effective_address.UInt64Ptr;
		}
		DISPATCH();
	}
	exec_neg:; {
		const uint8_t regid = *vm->Regs[regInstr].UCharPtr++;
		if( addrmode & Register )
			vm->Regs[regid].UInt64 = -vm->Regs[regid].UInt64;
		else if( addrmode & RegIndirect ) {
			const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
			*effective_address.UInt64Ptr = -*effective_address.UInt64Ptr;
		}
		DISPATCH();
	}
	exec_lt:; {
		const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
		const uint8_t sec_addrmode = op_args & 255;
		const uint8_t regid = op_args >> 8;
		if( addrmode & Register ) {
			if( sec_addrmode & Immediate )
				vm->CondFlag = vm->Regs[regid].UInt64 < *vm->Regs[regInstr].UInt64Ptr++;
			else if( sec_addrmode & Register )
				vm->CondFlag = vm->Regs[regid].UInt64 < vm->Regs[*vm->Regs[regInstr].UCharPtr++].UInt64;
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
			if( sec_addrmode & Immediate ) {
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
		const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
		const uint8_t sec_addrmode = op_args & 255;
		const uint8_t regid = op_args >> 8;
		if( addrmode & Register ) {
			if( sec_addrmode & Immediate )
				vm->CondFlag = vm->Regs[regid].UInt64 > *vm->Regs[regInstr].UInt64Ptr++;
			else if( sec_addrmode & Register )
				vm->CondFlag = vm->Regs[regid].UInt64 > vm->Regs[*vm->Regs[regInstr].UCharPtr++].UInt64;
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
			if( sec_addrmode & Immediate ) {
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
		const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
		const uint8_t sec_addrmode = op_args & 255;
		const uint8_t regid = op_args >> 8;
		if( addrmode & Register ) {
			if( sec_addrmode & Immediate )
				vm->CondFlag = vm->Regs[regid].UInt64 == *vm->Regs[regInstr].UInt64Ptr++;
			else if( sec_addrmode & Register )
				vm->CondFlag = vm->Regs[regid].UInt64 == vm->Regs[*vm->Regs[regInstr].UCharPtr++].UInt64;
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
			if( sec_addrmode & Immediate ) {
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
		const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
		const uint8_t sec_addrmode = op_args & 255;
		const uint8_t regid = op_args >> 8;
		if( addrmode & Register ) {
			if( sec_addrmode & Immediate )
				vm->CondFlag = vm->Regs[regid].UInt64 != *vm->Regs[regInstr].UInt64Ptr++;
			else if( sec_addrmode & Register )
				vm->CondFlag = vm->Regs[regid].UInt64 != vm->Regs[*vm->Regs[regInstr].UCharPtr++].UInt64;
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
			if( sec_addrmode & Immediate ) {
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
		if( addrmode & Immediate ) {
			const int64_t offset = *vm->Regs[regInstr].Int64Ptr++;
			vm->Regs[regInstr].UCharPtr += offset;
		}
		else if( addrmode & Register ) {
			const uint8_t regid = *vm->Regs[regInstr].UCharPtr++;
			vm->Regs[regInstr].UCharPtr += vm->Regs[regid].Int64;
		}
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
		if( addrmode & Immediate ) {
			const int64_t offset = *vm->Regs[regInstr].Int64Ptr++;
			!vm->CondFlag ? (vm->Regs[regInstr].UCharPtr += offset) : (void)vm->CondFlag;
		}
		else if( addrmode & Register ) {
			const uint8_t regid = *vm->Regs[regInstr].UCharPtr++;
			!vm->CondFlag ? (vm->Regs[regInstr].UCharPtr += vm->Regs[regid].Int64) : (void)vm->CondFlag;
		}
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
		if( addrmode & Immediate ) {
			const int64_t offset = *vm->Regs[regInstr].Int64Ptr++;
			vm->CondFlag ? (vm->Regs[regInstr].UCharPtr += offset) : (void)vm->CondFlag;
		}
		else if( addrmode & Register ) {
			const uint8_t regid = *vm->Regs[regInstr].UCharPtr++;
			vm->CondFlag ? (vm->Regs[regInstr].UCharPtr += vm->Regs[regid].Int64) : (void)vm->CondFlag;
		}
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
		uint8_t *call_addr = NULL;
		if( addrmode & Immediate ) {
			call_addr = GetFunctionOffsetByIndex(vm->CurrScript.Ptr, (*vm->Regs[regInstr].UInt64Ptr++) - 1, NULL);
		}
		else if( addrmode & Register )
			call_addr = GetFunctionOffsetByIndex(vm->CurrScript.Ptr, vm->Regs[*vm->Regs[regInstr].UCharPtr++].UInt64 - 1, NULL);
		else if( addrmode & RegIndirect ) {
			const uint8_t regid = *vm->Regs[regInstr].UCharPtr++;
			const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
			if( addrmode & EightBytes )
				call_addr = GetFunctionOffsetByIndex(vm->CurrScript.Ptr, *effective_address.UInt64Ptr - 1, NULL);
		}
		if( !call_addr ) {
			puts("Tagha_Exec :: exec_call reported 'call_addr' is NULL");
			DISPATCH();
		}
		
		*--vm->Regs[regStk].SelfPtr = vm->Regs[regInstr];	/* push rip */
		*--vm->Regs[regStk].SelfPtr = vm->Regs[regBase];	/* push rbp */
		vm->Regs[regBase] = vm->Regs[regStk];	/* mov rbp, rsp */
		vm->Regs[regInstr].UCharPtr = call_addr;
		DISPATCH();
	}
	exec_ret:; {
		vm->Regs[regStk] = vm->Regs[regBase]; /* mov rsp, rbp */
		vm->Regs[regBase] = *vm->Regs[regStk].SelfPtr++; /* pop rbp */
		
		/* if we're popping Main's (or whatever called func's) RBP, then halt the whole program. */
		if( vm->Regs[regBase].SelfPtr==MainBasePtr )
			goto *dispatch[halt];
		
		vm->Regs[regInstr] = *vm->Regs[regStk].SelfPtr++; /* pop rip */
		DISPATCH();
	}
	exec_syscall:; {
		/* how many args given to the native call. */
		const uint32_t argcount = *vm->Regs[regInstr].UInt32Ptr++;
		/* how many bytes does the native return? */
		void (*NativeFunc)() = NULL;
		
		/* trying to directly call a specific native. Allow this by imm only! */
		if( addrmode & Immediate ) {
			bool native_check = false;
			union Value *nativref = GetFunctionOffsetByIndex(vm->CurrScript.Ptr, -1 - *vm->Regs[regInstr].Int64Ptr++, &native_check);
			if( !native_check ) {
				puts("exec_syscall :: averted trying to syscall a non-native function!");
			}
			else NativeFunc = nativref->VoidFunc;
		}
		else if( addrmode & Register ) {
			bool native_check = false;
			union Value *nativref = GetFunctionOffsetByIndex(vm->CurrScript.Ptr, -1 - vm->Regs[*vm->Regs[regInstr].UCharPtr++].Int64, &native_check);
			if( !native_check ) {
				puts("exec_syscall :: averted trying to syscall a non-native function!");
			}
			else NativeFunc = nativref->VoidFunc;
		}
		else if( addrmode & RegIndirect ) {
			const uint8_t regid = *vm->Regs[regInstr].UCharPtr++;
			const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
			bool native_check = false;
			union Value *nativref = GetFunctionOffsetByIndex(vm->CurrScript.Ptr, -1 - *effective_address.Int64Ptr, &native_check);
			if( !native_check ) {
				puts("exec_syscall :: averted trying to syscall a non-native function!");
			}
			else NativeFunc = nativref->VoidFunc;
		}
		/* native call interface
		 * Limitations:
		 *  - any argument larger than 8 bytes must be passed as a pointer.
		 *  - any return value larger than 8 bytes must be passed as a hidden pointer argument and render the function as void.
		 * void NativeFunc(struct Tagha *sys, void *restrict script union Value *retval, const size_t args, union Value params[static args]);
		 */
		if( !NativeFunc ) {
			puts("Tagha_Exec :: exec_syscall reported 'NativeFunc' is NULL");
			DISPATCH();
		}
		
		union Value params[argcount];
		const size_t bytecount = sizeof(union Value) * argcount;
		/*memset(params, 0, bytecount); */
		
		const size_t reg_params = 8;
		const enum RegID reg_param_initial = regSemkath;
		
		/* save stack space by using the registers for passing arguments. */
		/* the other registers can then be used for other data operations. */
		if( argcount <= reg_params ) {
			memcpy(params, vm->Regs+reg_param_initial, bytecount);
		}
		/* if the native has more than a certain num of params, get from both registers and stack. */
		else if( argcount > reg_params ) {
			memcpy(params, vm->Regs+reg_param_initial, sizeof(union Value) * reg_params);
			memcpy(params+reg_params, vm->Regs[regStk].SelfPtr, sizeof(union Value) * (argcount-reg_params));
			vm->Regs[regStk].SelfPtr += (argcount-reg_params);
		}
		vm->Regs[regAlaf].UInt64 = 0;
		(*NativeFunc)(vm, vm->Regs+regAlaf, argcount, params);
		DISPATCH();
	}
	/* import loads another tbc module to tagha. */
	exec_import:; {
		
		DISPATCH();
	}
	/* invoke calls a method or loads a global variable from another module. */
	exec_invoke:; {
		
		DISPATCH();
	}
#if FLOATING_POINT_OPS
	exec_flt2dbl:; {
		if( addrmode & Register ) {
			const uint8_t regid = *vm->Regs[regInstr].UCharPtr++;
			const float f = vm->Regs[regid].Float;
			vm->Regs[regid].Double = (double)f;
		}
		DISPATCH();
	}
	exec_dbl2flt:; {
		if( addrmode & Register ) {
			const uint8_t regid = *vm->Regs[regInstr].UCharPtr++;
			const double d = vm->Regs[regid].Double;
			vm->Regs[regid].Double = 0;
			vm->Regs[regid].Float = (float)d;
		}
		DISPATCH();
	}
	exec_addf:; {
		const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
		const uint8_t sec_addrmode = op_args & 255;
		const uint8_t regid = op_args >> 8;
		
		if( addrmode & Register ) {
			if( sec_addrmode & Immediate ) {
				const union Value convert = (union Value){.UInt64 = *vm->Regs[regInstr].UInt64Ptr++};
				if( sec_addrmode & FourBytes )
					vm->Regs[regid].Float += convert.Float;
				else if( sec_addrmode & EightBytes )
					vm->Regs[regid].Double += convert.Double;
			}
			else if( sec_addrmode & Register ) {
				const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
				if( sec_addrmode & FourBytes )
					vm->Regs[regid].Float += vm->Regs[sec_regid].Float;
				else if( sec_addrmode & EightBytes )
					vm->Regs[regid].Double += vm->Regs[sec_regid].Double;
			}
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
			if( sec_addrmode & Immediate ) {
				const union Value imm = (union Value){.UInt64 = *vm->Regs[regInstr].UInt64Ptr++};
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				if( addrmode & FourBytes )
					*effective_address.FloatPtr += imm.Float;
				else if( addrmode & EightBytes )
					*effective_address.DoublePtr += imm.Double;
			}
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
		const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
		const uint8_t sec_addrmode = op_args & 255;
		const uint8_t regid = op_args >> 8;
		
		if( addrmode & Register ) {
			if( sec_addrmode & Immediate ) {
				const union Value convert = (union Value){.UInt64 = *vm->Regs[regInstr].UInt64Ptr++};
				if( sec_addrmode & FourBytes )
					vm->Regs[regid].Float -= convert.Float;
				else if( sec_addrmode & EightBytes )
					vm->Regs[regid].Double -= convert.Double;
			}
			else if( sec_addrmode & Register ) {
				const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
				if( sec_addrmode & FourBytes )
					vm->Regs[regid].Float -= vm->Regs[sec_regid].Float;
				else if( sec_addrmode & EightBytes )
					vm->Regs[regid].Double -= vm->Regs[sec_regid].Double;
			}
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
			if( sec_addrmode & Immediate ) {
				const union Value imm = (union Value){.Double = *vm->Regs[regInstr].DoublePtr++};
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				if( addrmode & FourBytes )
					*effective_address.FloatPtr -= imm.Float;
				else if( addrmode & EightBytes )
					*effective_address.DoublePtr -= imm.Double;
			}
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
		const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
		const uint8_t sec_addrmode = op_args & 255;
		const uint8_t regid = op_args >> 8;
		
		if( addrmode & Register ) {
			if( sec_addrmode & Immediate ) {
				const union Value convert = (union Value){.UInt64 = *vm->Regs[regInstr].UInt64Ptr++};
				if( sec_addrmode & FourBytes )
					vm->Regs[regid].Float *= convert.Float;
				else if( sec_addrmode & EightBytes )
					vm->Regs[regid].Double *= convert.Double;
			}
			else if( sec_addrmode & Register ) {
				const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
				if( sec_addrmode & FourBytes )
					vm->Regs[regid].Float *= vm->Regs[sec_regid].Float;
				else if( sec_addrmode & EightBytes )
					vm->Regs[regid].Double *= vm->Regs[sec_regid].Double;
			}
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
			if( sec_addrmode & Immediate ) {
				const union Value imm = (union Value){.Double = *vm->Regs[regInstr].DoublePtr++};
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				if( addrmode & FourBytes )
					*effective_address.FloatPtr *= imm.Float;
				else if( addrmode & EightBytes )
					*effective_address.DoublePtr *= imm.Double;
			}
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
		const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
		const uint8_t sec_addrmode = op_args & 255;
		const uint8_t regid = op_args >> 8;
		
		if( addrmode & Register ) {
			if( sec_addrmode & Immediate ) {
				const union Value convert = (union Value){.UInt64 = *vm->Regs[regInstr].UInt64Ptr++};
				if( sec_addrmode & FourBytes )
					vm->Regs[regid].Float /= convert.Float;
				else if( sec_addrmode & EightBytes )
					vm->Regs[regid].Double /= convert.Double;
			}
			else if( sec_addrmode & Register ) {
				const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
				if( sec_addrmode & FourBytes )
					vm->Regs[regid].Float /= vm->Regs[sec_regid].Float;
				else if( sec_addrmode & EightBytes )
					vm->Regs[regid].Double /= vm->Regs[sec_regid].Double;
			}
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
			if( sec_addrmode & Immediate ) {
				const union Value imm = (union Value){.Double = *vm->Regs[regInstr].DoublePtr++};
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				if( addrmode & FourBytes )
					*effective_address.FloatPtr /= imm.Float;
				else if( addrmode & EightBytes )
					*effective_address.DoublePtr /= imm.Double;
			}
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
		const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
		const uint8_t sec_addrmode = op_args & 255;
		const uint8_t regid = op_args >> 8;
		
		if( addrmode & Register ) {
			if( sec_addrmode & Immediate ) {
				const union Value convert = (union Value){.UInt64 = *vm->Regs[regInstr].UInt64Ptr++};
				if( sec_addrmode & FourBytes )
					vm->CondFlag = vm->Regs[regid].Float < convert.Float;
				else if( sec_addrmode & EightBytes )
					vm->CondFlag = vm->Regs[regid].Double < convert.Double;
			}
			else if( sec_addrmode & Register ) {
				const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
				if( sec_addrmode & FourBytes )
					vm->CondFlag = vm->Regs[regid].Float < vm->Regs[sec_regid].Float;
				else if( sec_addrmode & EightBytes )
					vm->CondFlag = vm->Regs[regid].Double < vm->Regs[sec_regid].Double;
			}
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
			if( sec_addrmode & Immediate ) {
				const union Value imm = (union Value){.Double = *vm->Regs[regInstr].DoublePtr++};
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				if( addrmode & FourBytes )
					vm->CondFlag = *effective_address.FloatPtr < imm.Float;
				else if( addrmode & EightBytes )
					vm->CondFlag = *effective_address.DoublePtr < imm.Double;
			}
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
		const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
		const uint8_t sec_addrmode = op_args & 255;
		const uint8_t regid = op_args >> 8;
		
		if( addrmode & Register ) {
			if( sec_addrmode & Immediate ) {
				const union Value convert = (union Value){.UInt64 = *vm->Regs[regInstr].UInt64Ptr++};
				if( sec_addrmode & FourBytes )
					vm->CondFlag = vm->Regs[regid].Float > convert.Float;
				else if( sec_addrmode & EightBytes )
					vm->CondFlag = vm->Regs[regid].Double > convert.Double;
			}
			else if( sec_addrmode & Register ) {
				const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
				if( sec_addrmode & FourBytes )
					vm->CondFlag = vm->Regs[regid].Float > vm->Regs[sec_regid].Float;
				else if( sec_addrmode & EightBytes )
					vm->CondFlag = vm->Regs[regid].Double > vm->Regs[sec_regid].Double;
			}
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
			if( sec_addrmode & Immediate ) {
				const union Value imm = (union Value){.Double = *vm->Regs[regInstr].DoublePtr++};
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				if( addrmode & FourBytes )
					vm->CondFlag = *effective_address.FloatPtr > imm.Float;
				else if( addrmode & EightBytes )
					vm->CondFlag = *effective_address.DoublePtr > imm.Double;
			}
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
		const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
		const uint8_t sec_addrmode = op_args & 255;
		const uint8_t regid = op_args >> 8;
		
		if( addrmode & Register ) {
			if( sec_addrmode & Immediate ) {
				const union Value convert = (union Value){.UInt64 = *vm->Regs[regInstr].UInt64Ptr++};
				if( sec_addrmode & FourBytes )
					vm->CondFlag = vm->Regs[regid].Float == convert.Float;
				else if( sec_addrmode & EightBytes )
					vm->CondFlag = vm->Regs[regid].Double == convert.Double;
			}
			else if( sec_addrmode & Register ) {
				const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
				if( sec_addrmode & FourBytes )
					vm->CondFlag = vm->Regs[regid].Float == vm->Regs[sec_regid].Float;
				else if( sec_addrmode & EightBytes )
					vm->CondFlag = vm->Regs[regid].Double == vm->Regs[sec_regid].Double;
			}
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
			if( sec_addrmode & Immediate ) {
				const union Value imm = (union Value){.Double = *vm->Regs[regInstr].DoublePtr++};
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				if( addrmode & FourBytes )
					vm->CondFlag = *effective_address.FloatPtr == imm.Float;
				else if( addrmode & EightBytes )
					vm->CondFlag = *effective_address.DoublePtr == imm.Double;
			}
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
		const uint16_t op_args = *vm->Regs[regInstr].UShortPtr++;
		const uint8_t sec_addrmode = op_args & 255;
		const uint8_t regid = op_args >> 8;
		
		if( addrmode & Register ) {
			if( sec_addrmode & Immediate ) {
				const union Value convert = (union Value){.UInt64 = *vm->Regs[regInstr].UInt64Ptr++};
				if( sec_addrmode & FourBytes )
					vm->CondFlag = vm->Regs[regid].Float != convert.Float;
				else if( sec_addrmode & EightBytes )
					vm->CondFlag = vm->Regs[regid].Double != convert.Double;
			}
			else if( sec_addrmode & Register ) {
				const uint8_t sec_regid = *vm->Regs[regInstr].UCharPtr++;
				if( sec_addrmode & FourBytes )
					vm->CondFlag = vm->Regs[regid].Float != vm->Regs[sec_regid].Float;
				else if( sec_addrmode & EightBytes )
					vm->CondFlag = vm->Regs[regid].Double != vm->Regs[sec_regid].Double;
			}
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
			if( sec_addrmode & Immediate ) {
				const union Value imm = (union Value){.Double = *vm->Regs[regInstr].DoublePtr++};
				const union Value effective_address = (union Value){.UCharPtr = vm->Regs[regid].UCharPtr + *vm->Regs[regInstr].Int32Ptr++};
				if( addrmode & FourBytes )
					vm->CondFlag = *effective_address.FloatPtr != imm.Float;
				else if( addrmode & EightBytes )
					vm->CondFlag = *effective_address.DoublePtr != imm.Double;
			}
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
			if( addrmode & FourBytes )
				++vm->Regs[regid].Float;
			else if( addrmode & EightBytes )
				++vm->Regs[regid].Double;
		}
		else if( addrmode & RegIndirect ) {
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
			if( addrmode & FourBytes )
				--vm->Regs[regid].Float;
			else if( addrmode & EightBytes )
				--vm->Regs[regid].Double;
		}
		else if( addrmode & RegIndirect ) {
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
			if( addrmode & FourBytes )
				vm->Regs[regid].Float = -vm->Regs[regid].Float;
			else if( addrmode & EightBytes )
				vm->Regs[regid].Double = -vm->Regs[regid].Double;
		}
		else if( addrmode & RegIndirect ) {
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

int32_t Tagha_RunScript(struct Tagha *const restrict vm, const int32_t argc, char *argv[])
{
	if( !vm )
		return -1;
	
	else if( *vm->CurrScript.UShortPtr != 0xC0DE ) {
		puts("Tagha_RunScript :: ERROR: Script has invalid main verifier.");
		return -1;
	}
	
	uint8_t *main_offset = GetFunctionOffsetByName(vm->CurrScript.Ptr, "main", NULL);
	if( !main_offset ) {
		puts("Tagha_RunScript :: ERROR: script contains no 'main' function.");
		return -1;
	}
	
	/* push argc, argv to registers. */
	union Value MainArgs[argc+1];
	for( int i=0 ; i<=argc ; i++ )
		MainArgs[i].Ptr = argv[i];
	
	vm->Regs[reg_Eh].Ptr = MainArgs;
	vm->Regs[regSemkath].Int32 = argc;
	
	/* check out stack size and align it by the size of union Value. */
	size_t stacksize = *(uint32_t *)(vm->CurrScript.UCharPtr+2);
	stacksize = (stacksize + (sizeof(union Value)-1)) & -(sizeof(union Value));
	if( !stacksize )
		return -1;
	
	union Value Stack[stacksize];
	memset(Stack, 0, sizeof(union Value) * stacksize);
	/*union Value *StackLimit = Stack + stacksize+1; */
	vm->Regs[regStk].SelfPtr = vm->Regs[regBase].SelfPtr = Stack + stacksize;
	
	(--vm->Regs[regStk].SelfPtr)->Int64 = -1LL;	/* push bullshit ret address. */
	*--vm->Regs[regStk].SelfPtr = vm->Regs[regBase]; /* push rbp */
	vm->Regs[regInstr].UCharPtr = main_offset;
	return Tagha_Exec(vm);
}

int32_t Tagha_CallFunc(struct Tagha *const restrict vm, const char *restrict funcname, const size_t args, union Value values[static args])
{
	if( !vm or !funcname )
		return -1;
	
	else if( *vm->CurrScript.UShortPtr != 0xC0DE ) {
		puts("Tagha_RunScript :: ERROR: Script has invalid main verifier.");
		return -1;
	}
	
	uint8_t *func_offset = GetFunctionOffsetByName(vm->CurrScript.Ptr, funcname, NULL);
	if( !func_offset ) {
		printf("Tagha_CallFunc :: ERROR: cannot find function: '%s'.", funcname);
		return -1;
	}
	
	/* check out stack size and align it by the size of union Value. */
	size_t stacksize = *(uint32_t *)(vm->CurrScript.UCharPtr+2);
	stacksize = (stacksize + (sizeof(union Value)-1)) & -(sizeof(union Value));
	if( !stacksize ) {
		puts("Tagha_CallFunc :: ERROR: stack size is 0!");
		return -1;
	}
	
	union Value Stack[stacksize+1];
	/* union Value *StackLimit = Stack + stacksize+1; */
	vm->Regs[regStk].SelfPtr = vm->Regs[regBase].SelfPtr = Stack + stacksize;
	
	/* remember that arguments must be passed right to left. */
	/* we have enough args to fit in registers. */
	const size_t reg_params = 8;
	const enum RegID reg_param_initial = regSemkath;
	const size_t bytecount = sizeof(union Value) * args;
	
	/* save stack space by using the registers for passing arguments. */
	/* the other registers can be used for other data ops. */
	if( args <= reg_params ) {
		memcpy(vm->Regs+reg_param_initial, values, bytecount);
	}
	/* if the native has more than a certain num of params, get from both registers and stack. */
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
