
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
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

static uint8_t *Tagha_LoadModule(const char *restrict module_name)
{
	if( !module_name )
		return NULL;
	
	FILE *restrict tbcfile = fopen(module_name, "rb");
	if( !tbcfile )
		return NULL;
	
	size_t filesize = 0L;
	if( !fseek(tbcfile, 0, SEEK_END) ) {
		int64_t size = ftell(tbcfile);
		if( size == -1LL ) {
			fclose(tbcfile), tbcfile=NULL;
			return NULL;
		}
		rewind(tbcfile);
		filesize = (size_t)size;
	}
	
	uint8_t buffer[filesize];
	const size_t val = fread(buffer, sizeof(uint8_t), filesize, tbcfile);
	(void)val;
	fclose(tbcfile), tbcfile=NULL;
	
	if( !(buffer[0] == 0xDE and buffer[1]==0xC0) )
		return NULL;
	
	uint8_t *module = calloc(filesize, sizeof *module);
	if( !module )
		return NULL;
	
	memcpy(module, buffer, filesize);
	return module;
}

/* void *Tagha_LoadModule(const char *tbc_module_name); */
static void Native_TaghaLoadModule(struct Tagha *const restrict sys, union Value *const restrict retval, const size_t args, union Value params[static args])
{
	(void)sys; (void)args;
	const char *restrict module_name = params[0].Ptr;
	retval->Ptr = Tagha_LoadModule(module_name);
}

/* void *Tagha_GetGlobal(void *module, const char *symname); */
static void Native_TaghaGetGlobal(struct Tagha *const restrict sys, union Value *const restrict retval, const size_t args, union Value params[static args])
{
	(void)sys; (void)args;
	void *restrict module = params[0].Ptr;
	const char *restrict symname = params[1].Ptr;
	
	retval->Ptr = GetVariableOffsetByName(module, symname);
}

/* bool Tagha_InvokeFunc(void *, const char *, union Value *, size_t, union Value []); */
static void Native_TaghaInvoke(struct Tagha *const restrict sys, union Value *const restrict retval, const size_t args, union Value params[static args])
{
	(void)sys; (void)args;
	void *module = params[0].Ptr;
	if( !module ) {
		retval->Bool = false;
		return;
	}
	const char *funcname = params[1].Ptr;
	if( !funcname ) {
		retval->Bool = false;
		return;
	}
	const size_t argcount = params[3].UInt64;
	union Value *array = params[4].SelfPtr;
	if( !array ) {
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
	PrepModule(script);
	
	const struct NativeInfo dynamic_loading[] = {
		{"Tagha_LoadModule", Native_TaghaLoadModule},
		{"Tagha_GetGlobal", Native_TaghaGetGlobal},
		{"Tagha_InvokeFunc", Native_TaghaInvoke},
		{"Tagha_FreeModule", Native_TaghaFreeModule},
		{NULL, NULL}
	};
	Tagha_RegisterNatives(vm, dynamic_loading);
}

void Tagha_InitN(struct Tagha *const restrict vm, void *script, const struct NativeInfo natives[])
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
	
	union Value reader = (union Value){.Ptr = script};
	reader.UCharPtr += 11;
	
	const size_t funcs = *reader.UInt32Ptr++;
	
	for( size_t i=0 ; i<funcs ; i++ ) {
		const struct TaghaItem *const restrict item = reader.Ptr;
		reader.UCharPtr += sizeof *item;
		if( !strcmp(funcname, reader.Ptr) )
			return reader.UCharPtr + item->StrLen;
		else reader.UCharPtr += (item->StrLen + item->DataLen);
	}
	return NULL;
}

inline static void *GetFunctionOffsetByIndex(uint8_t *const script, const size_t index)
{
	if( !script )
		return NULL;
	
	union Value reader = (union Value){.Ptr = script};
	reader.UCharPtr += 11;
	
	const size_t funcs = *reader.UInt32Ptr++;
	if( index >= funcs )
		return NULL;
	
	for( size_t i=0 ; i<funcs ; i++ ) {
		const struct TaghaItem *const restrict item = reader.Ptr;
		reader.UCharPtr += sizeof *item;
		
		if( i==index )
			return reader.UCharPtr + item->StrLen;
		else reader.UCharPtr += (item->StrLen + item->DataLen);
	}
	return NULL;
}

inline static void *GetVariableOffsetByName(uint8_t *const script, const char *restrict varname)
{
	if( !script or !varname )
		return NULL;
	
	union Value reader = (union Value){.Ptr = script};
	reader.UCharPtr += 7;
	const size_t vartable_offset = *reader.UInt32Ptr++;
	reader.UCharPtr += vartable_offset;
	
	const size_t globalvars = *reader.UInt32Ptr++;
	for( size_t i=0 ; i<globalvars ; i++ ) {
		const struct TaghaItem *const restrict item = reader.Ptr;
		reader.UCharPtr += sizeof *item;
		if( !strcmp(varname, reader.Ptr) )
			return reader.UCharPtr + item->StrLen;
		else reader.UCharPtr += (item->StrLen + item->DataLen);
	}
	return NULL;
}

inline static void *GetVariableOffsetByIndex(uint8_t *const script, const size_t index)
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
		const struct TaghaItem *const restrict item = reader.Ptr;
		reader.UCharPtr += sizeof *item;
		if( i==index )
			return reader.UCharPtr + item->StrLen;
		
		/* skip to the next global var index */
		reader.UCharPtr += (item->StrLen + item->DataLen);
	}
	return NULL;
}

/* #include <unistd.h>	// sleep() func */

int32_t Tagha_Exec(struct Tagha *const restrict vm)
{
	if( !vm or !vm->Module ) {
		return ErrInstrBounds;
	}
	union Value *const restrict regs = vm->Regs;
	const union Value *const restrict MainBasePtr = regs[regBase].SelfPtr;
	regs[regBase] = regs[regStk];
	
	uint8_t instr=0, addrmode=0;
	uint16_t opcode = 0;
	
#define X(x) #x ,
	/* for debugging purposes. */
	/*const char *const restrict opcode2str[] = { INSTR_SET };*/
#undef X
	
	
#define X(x) &&exec_##x ,
	/* our instruction dispatch table. */
	const void *const restrict dispatch[] = { INSTR_SET };
#undef X
#undef INSTR_SET
	/* #ifdef _UNISTD_H */
	
	#define DISPATCH() \
		opcode = *regs[regInstr].UShortPtr++; \
		\
		/* get the instruction from the first byte. */ \
		instr = opcode & 255; \
		\
		/* get addressing mode from second byte. */ \
		addrmode = opcode >> 8; \
		\
		if( instr>nop ) { \
			/*puts("Tagha_Exec :: instr out of bounds.");*/ \
			return ErrInstrBounds; \
		} \
		\
		/*TaghaDebug_PrintRegisters(vm);*/ \
		/*sleep(1);*/ \
		/*printf("dispatching to '%s'\n", opcode2str[instr]);*/ \
		goto *dispatch[instr]
	
	DISPATCH();
	
	exec_halt:;
		return regs[regAlaf].Int32;
	
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
			*--regs[regStk].SelfPtr = *regs[regInstr].SelfPtr++;
		/* push a register's contents. */
		else if( addrmode & Register )
			*--regs[regStk].SelfPtr = regs[*regs[regInstr].UCharPtr++];
		/* push the contents of a memory address inside a register. */
		else if( addrmode & RegIndirect ) {
			const uint8_t regid = *regs[regInstr].UCharPtr++;
			const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
			*--regs[regStk].SelfPtr = *effective_address.SelfPtr;
		}
		DISPATCH();
	}
	
	/* pops a value from the stack into a register or memory then reduces stack by 8 bytes.
	 * pop reg
	 * pop [reg+offset]
	 */
	exec_pop:; {
		if( addrmode & Immediate )
			regs[regInstr].SelfPtr++;
		else if( addrmode & Register )
			regs[*regs[regInstr].UCharPtr++] = *regs[regStk].SelfPtr++;
		else if( addrmode & RegIndirect ) {
			const uint8_t regid = *regs[regInstr].UCharPtr++;
			const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
			*effective_address.SelfPtr = *regs[regStk].SelfPtr++;
		}
		DISPATCH();
	}
	
	/* loads a ptr value to a register.
	 * lea reg, global var
	 * lea reg, func
	 * lea reg, [reg+offset] (not dereferenced)
	 */
	exec_lea:; {
		const uint8_t regid = *regs[regInstr].UCharPtr++;
		
		if( addrmode & Immediate ) { /* Immediate mode will load a global value */
			regs[regid].Ptr = GetVariableOffsetByIndex(vm->CurrScript.Ptr, *regs[regInstr].UInt64Ptr++);
		}
		else if( addrmode & Register ) { /* Register mode will load a function address which could be a native */
			regs[regid].Int64 = *regs[regInstr].Int64Ptr++;
		}
		else if( addrmode & RegIndirect ) {
			const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
			regs[regid].UCharPtr = regs[sec_regid].UCharPtr + *regs[regInstr].Int32Ptr++;
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
		const uint8_t regid = *regs[regInstr].UCharPtr++;
		if( addrmode & Reserved ) {
			if( addrmode & Immediate )
				regs[regid].UInt64 = *regs[regInstr].UInt64Ptr++;
			else if( addrmode & Register )
				regs[regid] = regs[*regs[regInstr].UCharPtr++];
			else if( addrmode & RegIndirect ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = regs[sec_regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					regs[regid].UChar = *effective_address.UCharPtr;
				else if( addrmode & TwoBytes )
					regs[regid].UShort = *effective_address.UShortPtr;
				else if( addrmode & FourBytes )
					regs[regid].UInt32 = *effective_address.UInt32Ptr;
				else if( addrmode & EightBytes )
					regs[regid].UInt64 = *effective_address.UInt64Ptr;
			}
		}
		else {
			if( addrmode & Immediate ) {
				const union Value imm = (union Value){.UInt64 = *regs[regInstr].UInt64Ptr++};
				const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					*effective_address.UCharPtr = imm.UChar;
				else if( addrmode & TwoBytes )
					*effective_address.UShortPtr = imm.UShort;
				else if( addrmode & FourBytes )
					*effective_address.UInt32Ptr = imm.UInt32;
				else if( addrmode & EightBytes )
					*effective_address.UInt64Ptr = imm.UInt64;
			}
			else if( addrmode & Register ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					*effective_address.UCharPtr = regs[sec_regid].UChar;
				else if( addrmode & TwoBytes )
					*effective_address.UShortPtr = regs[sec_regid].UShort;
				else if( addrmode & FourBytes )
					*effective_address.UInt32Ptr = regs[sec_regid].UInt32;
				else if( addrmode & EightBytes )
					*effective_address.UInt64Ptr = regs[sec_regid].UInt64;
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
		const uint8_t regid = *regs[regInstr].UCharPtr++;
		if( addrmode & Reserved ) {
			if( addrmode & Immediate ) {
				regs[regid].UInt64 += *regs[regInstr].UInt64Ptr++;
			}
			else if( addrmode & Register ) {
				regs[regid].UInt64 += regs[*regs[regInstr].UCharPtr++].UInt64;
			}
			else if( addrmode & RegIndirect ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = regs[sec_regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					regs[regid].UChar += *effective_address.UCharPtr;
				else if( addrmode & TwoBytes )
					regs[regid].UShort += *effective_address.UShortPtr;
				else if( addrmode & FourBytes )
					regs[regid].UInt32 += *effective_address.UInt32Ptr;
				else if( addrmode & EightBytes )
					regs[regid].UInt64 += *effective_address.UInt64Ptr;
			}
		}
		else {
			if( addrmode & Immediate ) {
				const union Value imm = (union Value){.UInt64 = *regs[regInstr].UInt64Ptr++};
				const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					*effective_address.UCharPtr += imm.UChar;
				else if( addrmode & TwoBytes )
					*effective_address.UShortPtr += imm.UShort;
				else if( addrmode & FourBytes )
					*effective_address.UInt32Ptr += imm.UInt32;
				else if( addrmode & EightBytes )
					*effective_address.UInt64Ptr += imm.UInt64;
			}
			else if( addrmode & Register ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					*effective_address.UCharPtr += regs[sec_regid].UChar;
				else if( addrmode & TwoBytes )
					*effective_address.UShortPtr += regs[sec_regid].UShort;
				else if( addrmode & FourBytes )
					*effective_address.UInt32Ptr += regs[sec_regid].UInt32;
				else if( addrmode & EightBytes )
					*effective_address.UInt64Ptr += regs[sec_regid].UInt64;
			}
		}
		DISPATCH();
	}
	exec_sub:; {
		const uint8_t regid = *regs[regInstr].UCharPtr++;
		if( addrmode & Reserved ) {
			if( addrmode & Immediate )
				regs[regid].UInt64 -= *regs[regInstr].UInt64Ptr++;
			else if( addrmode & Register )
				regs[regid].UInt64 -= regs[*regs[regInstr].UCharPtr++].UInt64;
			else if( addrmode & RegIndirect ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = regs[sec_regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					regs[regid].UChar -= *effective_address.UCharPtr;
				else if( addrmode & TwoBytes )
					regs[regid].UShort -= *effective_address.UShortPtr;
				else if( addrmode & FourBytes )
					regs[regid].UInt32 -= *effective_address.UInt32Ptr;
				else if( addrmode & EightBytes )
					regs[regid].UInt64 -= *effective_address.UInt64Ptr;
			}
		}
		else {
			if( addrmode & Immediate ) {
				const union Value imm = (union Value){.UInt64 = *regs[regInstr].UInt64Ptr++};
				const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					*effective_address.UCharPtr -= imm.UChar;
				else if( addrmode & TwoBytes )
					*effective_address.UShortPtr -= imm.UShort;
				else if( addrmode & FourBytes )
					*effective_address.UInt32Ptr -= imm.UInt32;
				else if( addrmode & EightBytes )
					*effective_address.UInt64Ptr -= imm.UInt64;
			}
			else if( addrmode & Register ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					*effective_address.UCharPtr -= regs[sec_regid].UChar;
				else if( addrmode & TwoBytes )
					*effective_address.UShortPtr -= regs[sec_regid].UShort;
				else if( addrmode & FourBytes )
					*effective_address.UInt32Ptr -= regs[sec_regid].UInt32;
				else if( addrmode & EightBytes )
					*effective_address.UInt64Ptr -= regs[sec_regid].UInt64;
			}
		}
		DISPATCH();
	}
	exec_mul:; {
		const uint8_t regid = *regs[regInstr].UCharPtr++;
		if( addrmode & Reserved ) {
			if( addrmode & Immediate )
				regs[regid].UInt64 *= *regs[regInstr].UInt64Ptr++;
			else if( addrmode & Register )
				regs[regid].UInt64 *= regs[*regs[regInstr].UCharPtr++].UInt64;
			else if( addrmode & RegIndirect ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = regs[sec_regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					regs[regid].UChar *= *effective_address.UCharPtr;
				else if( addrmode & TwoBytes )
					regs[regid].UShort *= *effective_address.UShortPtr;
				else if( addrmode & FourBytes )
					regs[regid].UInt32 *= *effective_address.UInt32Ptr;
				else if( addrmode & EightBytes )
					regs[regid].UInt64 *= *effective_address.UInt64Ptr;
			}
		}
		else {
			if( addrmode & Immediate ) {
				const union Value imm = (union Value){.UInt64 = *regs[regInstr].UInt64Ptr++};
				const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					*effective_address.UCharPtr *= imm.UChar;
				else if( addrmode & TwoBytes )
					*effective_address.UShortPtr *= imm.UShort;
				else if( addrmode & FourBytes )
					*effective_address.UInt32Ptr *= imm.UInt32;
				else if( addrmode & EightBytes )
					*effective_address.UInt64Ptr *= imm.UInt64;
			}
			else if( addrmode & Register ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					*effective_address.UCharPtr *= regs[sec_regid].UChar;
				else if( addrmode & TwoBytes )
					*effective_address.UShortPtr *= regs[sec_regid].UShort;
				else if( addrmode & FourBytes )
					*effective_address.UInt32Ptr *= regs[sec_regid].UInt32;
				else if( addrmode & EightBytes )
					*effective_address.UInt64Ptr *= regs[sec_regid].UInt64;
			}
		}
		DISPATCH();
	}
	exec_divi:; {
		const uint8_t regid = *regs[regInstr].UCharPtr++;
		if( addrmode & Reserved ) {
			if( addrmode & Immediate )
				regs[regid].UInt64 /= *regs[regInstr].UInt64Ptr++;
			else if( addrmode & Register )
				regs[regid].UInt64 /= regs[*regs[regInstr].UCharPtr++].UInt64;
			else if( addrmode & RegIndirect ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = regs[sec_regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					regs[regid].UChar /= *effective_address.UCharPtr;
				else if( addrmode & TwoBytes )
					regs[regid].UShort /= *effective_address.UShortPtr;
				else if( addrmode & FourBytes )
					regs[regid].UInt32 /= *effective_address.UInt32Ptr;
				else if( addrmode & EightBytes )
					regs[regid].UInt64 /= *effective_address.UInt64Ptr;
			}
		}
		else {
			if( addrmode & Immediate ) {
				const union Value imm = (union Value){.UInt64 = *regs[regInstr].UInt64Ptr++};
				const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					*effective_address.UCharPtr /= imm.UChar;
				else if( addrmode & TwoBytes )
					*effective_address.UShortPtr /= imm.UShort;
				else if( addrmode & FourBytes )
					*effective_address.UInt32Ptr /= imm.UInt32;
				else if( addrmode & EightBytes )
					*effective_address.UInt64Ptr /= imm.UInt64;
			}
			else if( addrmode & Register ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					*effective_address.UCharPtr /= regs[sec_regid].UChar;
				else if( addrmode & TwoBytes )
					*effective_address.UShortPtr /= regs[sec_regid].UShort;
				else if( addrmode & FourBytes )
					*effective_address.UInt32Ptr /= regs[sec_regid].UInt32;
				else if( addrmode & EightBytes )
					*effective_address.UInt64Ptr /= regs[sec_regid].UInt64;
			}
		}
		DISPATCH();
	}
	exec_mod:; {
		const uint8_t regid = *regs[regInstr].UCharPtr++;
		if( addrmode & Reserved ) {
			if( addrmode & Immediate )
				regs[regid].UInt64 %= *regs[regInstr].UInt64Ptr++;
			else if( addrmode & Register )
				regs[regid].UInt64 %= regs[*regs[regInstr].UCharPtr++].UInt64;
			else if( addrmode & RegIndirect ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = regs[sec_regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					regs[regid].UChar %= *effective_address.UCharPtr;
				else if( addrmode & TwoBytes )
					regs[regid].UShort %= *effective_address.UShortPtr;
				else if( addrmode & FourBytes )
					regs[regid].UInt32 %= *effective_address.UInt32Ptr;
				else if( addrmode & EightBytes )
					regs[regid].UInt64 %= *effective_address.UInt64Ptr;
			}
		}
		else {
			if( addrmode & Immediate ) {
				const union Value imm = (union Value){.UInt64 = *regs[regInstr].UInt64Ptr++};
				const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					*effective_address.UCharPtr %= imm.UChar;
				else if( addrmode & TwoBytes )
					*effective_address.UShortPtr %= imm.UShort;
				else if( addrmode & FourBytes )
					*effective_address.UInt32Ptr %= imm.UInt32;
				else if( addrmode & EightBytes )
					*effective_address.UInt64Ptr %= imm.UInt64;
			}
			else if( addrmode & Register ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					*effective_address.UCharPtr %= regs[sec_regid].UChar;
				else if( addrmode & TwoBytes )
					*effective_address.UShortPtr %= regs[sec_regid].UShort;
				else if( addrmode & FourBytes )
					*effective_address.UInt32Ptr %= regs[sec_regid].UInt32;
				else if( addrmode & EightBytes )
					*effective_address.UInt64Ptr %= regs[sec_regid].UInt64;
			}
		}
		DISPATCH();
	}
	exec_andb:; {
		const uint8_t regid = *regs[regInstr].UCharPtr++;
		if( addrmode & Reserved ) {
			if( addrmode & Immediate )
				regs[regid].UInt64 &= *regs[regInstr].UInt64Ptr++;
			else if( addrmode & Register )
				regs[regid].UInt64 &= regs[*regs[regInstr].UCharPtr++].UInt64;
			else if( addrmode & RegIndirect ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = regs[sec_regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					regs[regid].UChar &= *effective_address.UCharPtr;
				else if( addrmode & TwoBytes )
					regs[regid].UShort &= *effective_address.UShortPtr;
				else if( addrmode & FourBytes )
					regs[regid].UInt32 &= *effective_address.UInt32Ptr;
				else if( addrmode & EightBytes )
					regs[regid].UInt64 &= *effective_address.UInt64Ptr;
			}
		}
		else {
			if( addrmode & Immediate ) {
				const union Value imm = (union Value){.UInt64 = *regs[regInstr].UInt64Ptr++};
				const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					*effective_address.UCharPtr &= imm.UChar;
				else if( addrmode & TwoBytes )
					*effective_address.UShortPtr &= imm.UShort;
				else if( addrmode & FourBytes )
					*effective_address.UInt32Ptr &= imm.UInt32;
				else if( addrmode & EightBytes )
					*effective_address.UInt64Ptr &= imm.UInt64;
			}
			else if( addrmode & Register ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					*effective_address.UCharPtr &= regs[sec_regid].UChar;
				else if( addrmode & TwoBytes )
					*effective_address.UShortPtr &= regs[sec_regid].UShort;
				else if( addrmode & FourBytes )
					*effective_address.UInt32Ptr &= regs[sec_regid].UInt32;
				else if( addrmode & EightBytes )
					*effective_address.UInt64Ptr &= regs[sec_regid].UInt64;
			}
		}
		DISPATCH();
	}
	exec_orb:; {
		const uint8_t regid = *regs[regInstr].UCharPtr++;
		if( addrmode & Reserved ) {
			if( addrmode & Immediate )
				regs[regid].UInt64 |= *regs[regInstr].UInt64Ptr++;
			else if( addrmode & Register )
				regs[regid].UInt64 |= regs[*regs[regInstr].UCharPtr++].UInt64;
			else if( addrmode & RegIndirect ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = regs[sec_regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					regs[regid].UChar |= *effective_address.UCharPtr;
				else if( addrmode & TwoBytes )
					regs[regid].UShort |= *effective_address.UShortPtr;
				else if( addrmode & FourBytes )
					regs[regid].UInt32 |= *effective_address.UInt32Ptr;
				else if( addrmode & EightBytes )
					regs[regid].UInt64 |= *effective_address.UInt64Ptr;
			}
		}
		else {
			if( addrmode & Immediate ) {
				const union Value imm = (union Value){.UInt64 = *regs[regInstr].UInt64Ptr++};
				const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					*effective_address.UCharPtr |= imm.UChar;
				else if( addrmode & TwoBytes )
					*effective_address.UShortPtr |= imm.UShort;
				else if( addrmode & FourBytes )
					*effective_address.UInt32Ptr |= imm.UInt32;
				else if( addrmode & EightBytes )
					*effective_address.UInt64Ptr |= imm.UInt64;
			}
			else if( addrmode & Register ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					*effective_address.UCharPtr |= regs[sec_regid].UChar;
				else if( addrmode & TwoBytes )
					*effective_address.UShortPtr |= regs[sec_regid].UShort;
				else if( addrmode & FourBytes )
					*effective_address.UInt32Ptr |= regs[sec_regid].UInt32;
				else if( addrmode & EightBytes )
					*effective_address.UInt64Ptr |= regs[sec_regid].UInt64;
			}
		}
		DISPATCH();
	}
	exec_xorb:; {
		const uint8_t regid = *regs[regInstr].UCharPtr++;
		if( addrmode & Reserved ) {
			if( addrmode & Immediate )
				regs[regid].UInt64 ^= *regs[regInstr].UInt64Ptr++;
			else if( addrmode & Register )
				regs[regid].UInt64 ^= regs[*regs[regInstr].UCharPtr++].UInt64;
			else if( addrmode & RegIndirect ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = regs[sec_regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					regs[regid].UChar ^= *effective_address.UCharPtr;
				else if( addrmode & TwoBytes )
					regs[regid].UShort ^= *effective_address.UShortPtr;
				else if( addrmode & FourBytes )
					regs[regid].UInt32 ^= *effective_address.UInt32Ptr;
				else if( addrmode & EightBytes )
					regs[regid].UInt64 ^= *effective_address.UInt64Ptr;
			}
		}
		else {
			if( addrmode & Immediate ) {
				const union Value imm = (union Value){.UInt64 = *regs[regInstr].UInt64Ptr++};
				const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					*effective_address.UCharPtr ^= imm.UChar;
				else if( addrmode & TwoBytes )
					*effective_address.UShortPtr ^= imm.UShort;
				else if( addrmode & FourBytes )
					*effective_address.UInt32Ptr ^= imm.UInt32;
				else if( addrmode & EightBytes )
					*effective_address.UInt64Ptr ^= imm.UInt64;
			}
			else if( addrmode & Register ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					*effective_address.UCharPtr ^= regs[sec_regid].UChar;
				else if( addrmode & TwoBytes )
					*effective_address.UShortPtr ^= regs[sec_regid].UShort;
				else if( addrmode & FourBytes )
					*effective_address.UInt32Ptr ^= regs[sec_regid].UInt32;
				else if( addrmode & EightBytes )
					*effective_address.UInt64Ptr ^= regs[sec_regid].UInt64;
			}
		}
		DISPATCH();
	}
	exec_notb:; {
		const uint8_t regid = *regs[regInstr].UCharPtr++;
		if( addrmode & Register )
			regs[regid].UInt64 = ~regs[regid].UInt64;
		else if( addrmode & RegIndirect ) {
			const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
			if( addrmode & Byte )
				*effective_address.UCharPtr = ~*effective_address.UCharPtr;
			else if( addrmode & TwoBytes )
				*effective_address.UShortPtr = ~*effective_address.UShortPtr;
			else if( addrmode & FourBytes )
				*effective_address.UInt32Ptr = ~*effective_address.UInt32Ptr;
			else if( addrmode & EightBytes )
				*effective_address.UInt64Ptr = ~*effective_address.UInt64Ptr;
		}
		DISPATCH();
	}
	exec_shl:; {
		const uint8_t regid = *regs[regInstr].UCharPtr++;
		if( addrmode & Reserved ) {
			if( addrmode & Immediate )
				regs[regid].UInt64 <<= *regs[regInstr].UInt64Ptr++;
			else if( addrmode & Register )
				regs[regid].UInt64 <<= regs[*regs[regInstr].UCharPtr++].UInt64;
			else if( addrmode & RegIndirect ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = regs[sec_regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					regs[regid].UChar <<= *effective_address.UCharPtr;
				else if( addrmode & TwoBytes )
					regs[regid].UShort <<= *effective_address.UShortPtr;
				else if( addrmode & FourBytes )
					regs[regid].UInt32 <<= *effective_address.UInt32Ptr;
				else if( addrmode & EightBytes )
					regs[regid].UInt64 <<= *effective_address.UInt64Ptr;
			}
		}
		else {
			if( addrmode & Immediate ) {
				const union Value imm = (union Value){.UInt64 = *regs[regInstr].UInt64Ptr++};
				const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					*effective_address.UCharPtr <<= imm.UChar;
				else if( addrmode & TwoBytes )
					*effective_address.UShortPtr <<= imm.UShort;
				else if( addrmode & FourBytes )
					*effective_address.UInt32Ptr <<= imm.UInt32;
				else if( addrmode & EightBytes )
					*effective_address.UInt64Ptr <<= imm.UInt64;
			}
			else if( addrmode & Register ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					*effective_address.UCharPtr <<= regs[sec_regid].UChar;
				else if( addrmode & TwoBytes )
					*effective_address.UShortPtr <<= regs[sec_regid].UShort;
				else if( addrmode & FourBytes )
					*effective_address.UInt32Ptr <<= regs[sec_regid].UInt32;
				else if( addrmode & EightBytes )
					*effective_address.UInt64Ptr <<= regs[sec_regid].UInt64;
			}
		}
		DISPATCH();
	}
	exec_shr:; {
		const uint8_t regid = *regs[regInstr].UCharPtr++;
		if( addrmode & Reserved ) {
			if( addrmode & Immediate )
				regs[regid].UInt64 >>= *regs[regInstr].UInt64Ptr++;
			else if( addrmode & Register )
				regs[regid].UInt64 >>= regs[*regs[regInstr].UCharPtr++].UInt64;
			else if( addrmode & RegIndirect ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = regs[sec_regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					regs[regid].UChar >>= *effective_address.UCharPtr;
				else if( addrmode & TwoBytes )
					regs[regid].UShort >>= *effective_address.UShortPtr;
				else if( addrmode & FourBytes )
					regs[regid].UInt32 >>= *effective_address.UInt32Ptr;
				else if( addrmode & EightBytes )
					regs[regid].UInt64 >>= *effective_address.UInt64Ptr;
			}
		}
		else {
			if( addrmode & Immediate ) {
				const union Value imm = (union Value){.UInt64 = *regs[regInstr].UInt64Ptr++};
				const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					*effective_address.UCharPtr >>= imm.UChar;
				else if( addrmode & TwoBytes )
					*effective_address.UShortPtr >>= imm.UShort;
				else if( addrmode & FourBytes )
					*effective_address.UInt32Ptr >>= imm.UInt32;
				else if( addrmode & EightBytes )
					*effective_address.UInt64Ptr >>= imm.UInt64;
			}
			else if( addrmode & Register ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					*effective_address.UCharPtr >>= regs[sec_regid].UChar;
				else if( addrmode & TwoBytes )
					*effective_address.UShortPtr >>= regs[sec_regid].UShort;
				else if( addrmode & FourBytes )
					*effective_address.UInt32Ptr >>= regs[sec_regid].UInt32;
				else if( addrmode & EightBytes )
					*effective_address.UInt64Ptr >>= regs[sec_regid].UInt64;
			}
		}
		DISPATCH();
	}
	exec_inc:; {
		const uint8_t regid = *regs[regInstr].UCharPtr++;
		if( addrmode & Register )
			++regs[regid].UInt64;
		else if( addrmode & RegIndirect ) {
			const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
			if( addrmode & Byte )
				++*effective_address.UCharPtr;
			else if( addrmode & TwoBytes )
				++*effective_address.UShortPtr;
			else if( addrmode & FourBytes )
				++*effective_address.UInt32Ptr;
			else if( addrmode & EightBytes )
				++*effective_address.UInt64Ptr;
		}
		DISPATCH();
	}
	exec_dec:; {
		const uint8_t regid = *regs[regInstr].UCharPtr++;
		if( addrmode & Register )
			--regs[regid].UInt64;
		else if( addrmode & RegIndirect ) {
			const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
			if( addrmode & Byte )
				--*effective_address.UCharPtr;
			else if( addrmode & TwoBytes )
				--*effective_address.UShortPtr;
			else if( addrmode & FourBytes )
				--*effective_address.UInt32Ptr;
			else if( addrmode & EightBytes )
				--*effective_address.UInt64Ptr;
		}
		DISPATCH();
	}
	exec_neg:; {
		const uint8_t regid = *regs[regInstr].UCharPtr++;
		if( addrmode & Register )
			regs[regid].UInt64 = -regs[regid].UInt64;
		else if( addrmode & RegIndirect ) {
			const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
			if( addrmode & Byte )
				*effective_address.UCharPtr = -*effective_address.UCharPtr;
			else if( addrmode & TwoBytes )
				*effective_address.UShortPtr = -*effective_address.UShortPtr;
			else if( addrmode & FourBytes )
				*effective_address.UInt32Ptr = -*effective_address.UInt32Ptr;
			else if( addrmode & EightBytes )
				*effective_address.UInt64Ptr = -*effective_address.UInt64Ptr;
		}
		DISPATCH();
	}
	exec_lt:; {
		const uint8_t regid = *regs[regInstr].UCharPtr++;
		if( addrmode & Reserved ) {
			if( addrmode & Immediate )
				vm->CondFlag = regs[regid].UInt64 < *regs[regInstr].UInt64Ptr++;
			else if( addrmode & Register )
				vm->CondFlag = regs[regid].UInt64 < regs[*regs[regInstr].UCharPtr++].UInt64;
			else if( addrmode & RegIndirect ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = regs[sec_regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					vm->CondFlag = regs[regid].UChar < *effective_address.UCharPtr;
				else if( addrmode & TwoBytes )
					vm->CondFlag = regs[regid].UShort < *effective_address.UShortPtr;
				else if( addrmode & FourBytes )
					vm->CondFlag = regs[regid].UInt32 < *effective_address.UInt32Ptr;
				else if( addrmode & EightBytes )
					vm->CondFlag = regs[regid].UInt64 < *effective_address.UInt64Ptr;
			}
		}
		else {
			if( addrmode & Immediate ) {
				const union Value imm = (union Value){.UInt64 = *regs[regInstr].UInt64Ptr++};
				const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					vm->CondFlag = *effective_address.UCharPtr < imm.UChar;
				else if( addrmode & TwoBytes )
					vm->CondFlag = *effective_address.UShortPtr < imm.UShort;
				else if( addrmode & FourBytes )
					vm->CondFlag = *effective_address.UInt32Ptr < imm.UInt32;
				else if( addrmode & EightBytes )
					vm->CondFlag = *effective_address.UInt64Ptr < imm.UInt64;
			}
			else if( addrmode & Register ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					vm->CondFlag = *effective_address.UCharPtr < regs[sec_regid].UChar;
				else if( addrmode & TwoBytes )
					vm->CondFlag = *effective_address.UShortPtr < regs[sec_regid].UShort;
				else if( addrmode & FourBytes )
					vm->CondFlag = *effective_address.UInt32Ptr < regs[sec_regid].UInt32;
				else if( addrmode & EightBytes )
					vm->CondFlag = *effective_address.UInt64Ptr < regs[sec_regid].UInt64;
			}
		}
		DISPATCH();
	}
	exec_gt:; {
		const uint8_t regid = *regs[regInstr].UCharPtr++;
		if( addrmode & Reserved ) {
			if( addrmode & Immediate )
				vm->CondFlag = regs[regid].UInt64 > *regs[regInstr].UInt64Ptr++;
			else if( addrmode & Register )
				vm->CondFlag = regs[regid].UInt64 > regs[*regs[regInstr].UCharPtr++].UInt64;
			else if( addrmode & RegIndirect ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = regs[sec_regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					vm->CondFlag = regs[regid].UChar > *effective_address.UCharPtr;
				else if( addrmode & TwoBytes )
					vm->CondFlag = regs[regid].UShort > *effective_address.UShortPtr;
				else if( addrmode & FourBytes )
					vm->CondFlag = regs[regid].UInt32 > *effective_address.UInt32Ptr;
				else if( addrmode & EightBytes )
					vm->CondFlag = regs[regid].UInt64 > *effective_address.UInt64Ptr;
			}
		}
		else {
			if( addrmode & Immediate ) {
				const union Value imm = (union Value){.UInt64 = *regs[regInstr].UInt64Ptr++};
				const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					vm->CondFlag = *effective_address.UCharPtr > imm.UChar;
				else if( addrmode & TwoBytes )
					vm->CondFlag = *effective_address.UShortPtr > imm.UShort;
				else if( addrmode & FourBytes )
					vm->CondFlag = *effective_address.UInt32Ptr > imm.UInt32;
				else if( addrmode & EightBytes )
					vm->CondFlag = *effective_address.UInt64Ptr > imm.UInt64;
			}
			else if( addrmode & Register ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					vm->CondFlag = *effective_address.UCharPtr > regs[sec_regid].UChar;
				else if( addrmode & TwoBytes )
					vm->CondFlag = *effective_address.UShortPtr > regs[sec_regid].UShort;
				else if( addrmode & FourBytes )
					vm->CondFlag = *effective_address.UInt32Ptr > regs[sec_regid].UInt32;
				else if( addrmode & EightBytes )
					vm->CondFlag = *effective_address.UInt64Ptr > regs[sec_regid].UInt64;
			}
		}
		DISPATCH();
	}
	exec_cmp:; {
		const uint8_t regid = *regs[regInstr].UCharPtr++;
		if( addrmode & Reserved ) {
			if( addrmode & Immediate )
				vm->CondFlag = regs[regid].UInt64 == *regs[regInstr].UInt64Ptr++;
			else if( addrmode & Register )
				vm->CondFlag = regs[regid].UInt64 == regs[*regs[regInstr].UCharPtr++].UInt64;
			else if( addrmode & RegIndirect ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = regs[sec_regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					vm->CondFlag = regs[regid].UChar == *effective_address.UCharPtr;
				else if( addrmode & TwoBytes )
					vm->CondFlag = regs[regid].UShort == *effective_address.UShortPtr;
				else if( addrmode & FourBytes )
					vm->CondFlag = regs[regid].UInt32 == *effective_address.UInt32Ptr;
				else if( addrmode & EightBytes )
					vm->CondFlag = regs[regid].UInt64 == *effective_address.UInt64Ptr;
			}
		}
		else {
			if( addrmode & Immediate ) {
				const union Value imm = (union Value){.UInt64 = *regs[regInstr].UInt64Ptr++};
				const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					vm->CondFlag = *effective_address.UCharPtr == imm.UChar;
				else if( addrmode & TwoBytes )
					vm->CondFlag = *effective_address.UShortPtr == imm.UShort;
				else if( addrmode & FourBytes )
					vm->CondFlag = *effective_address.UInt32Ptr == imm.UInt32;
				else if( addrmode & EightBytes )
					vm->CondFlag = *effective_address.UInt64Ptr == imm.UInt64;
			}
			else if( addrmode & Register ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					vm->CondFlag = *effective_address.UCharPtr == regs[sec_regid].UChar;
				else if( addrmode & TwoBytes )
					vm->CondFlag = *effective_address.UShortPtr == regs[sec_regid].UShort;
				else if( addrmode & FourBytes )
					vm->CondFlag = *effective_address.UInt32Ptr == regs[sec_regid].UInt32;
				else if( addrmode & EightBytes )
					vm->CondFlag = *effective_address.UInt64Ptr == regs[sec_regid].UInt64;
			}
		}
		DISPATCH();
	}
	exec_neq:; {
		const uint8_t regid = *regs[regInstr].UCharPtr++;
		if( addrmode & Reserved ) {
			if( addrmode & Immediate )
				vm->CondFlag = regs[regid].UInt64 != *regs[regInstr].UInt64Ptr++;
			else if( addrmode & Register )
				vm->CondFlag = regs[regid].UInt64 != regs[*regs[regInstr].UCharPtr++].UInt64;
			else if( addrmode & RegIndirect ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = regs[sec_regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					vm->CondFlag = regs[regid].UChar != *effective_address.UCharPtr;
				else if( addrmode & TwoBytes )
					vm->CondFlag = regs[regid].UShort != *effective_address.UShortPtr;
				else if( addrmode & FourBytes )
					vm->CondFlag = regs[regid].UInt32 != *effective_address.UInt32Ptr;
				else if( addrmode & EightBytes )
					vm->CondFlag = regs[regid].UInt64 != *effective_address.UInt64Ptr;
			}
		}
		else {
			if( addrmode & Immediate ) {
				const union Value imm = (union Value){.UInt64 = *regs[regInstr].UInt64Ptr++};
				const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
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
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & Byte )
					vm->CondFlag = *effective_address.UCharPtr != regs[sec_regid].UChar;
				else if( addrmode & TwoBytes )
					vm->CondFlag = *effective_address.UShortPtr != regs[sec_regid].UShort;
				else if( addrmode & FourBytes )
					vm->CondFlag = *effective_address.UInt32Ptr != regs[sec_regid].UInt32;
				else if( addrmode & EightBytes )
					vm->CondFlag = *effective_address.UInt64Ptr != regs[sec_regid].UInt64;
			}
		}
		DISPATCH();
	}
	exec_jmp:; {
		if( addrmode & Immediate ) {
			const int64_t offset = *regs[regInstr].Int64Ptr++;
			regs[regInstr].UCharPtr += offset;
		}
		else if( addrmode & Register ) {
			const uint8_t regid = *regs[regInstr].UCharPtr++;
			regs[regInstr].UCharPtr += regs[regid].Int64;
		}
		else if( addrmode & RegIndirect ) {
			const uint8_t regid = *regs[regInstr].UCharPtr++;
			const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
			
			if( addrmode & Byte )
				regs[regInstr].UCharPtr += *effective_address.CharPtr;
			else if( addrmode & TwoBytes )
				regs[regInstr].UCharPtr += *effective_address.ShortPtr;
			else if( addrmode & FourBytes )
				regs[regInstr].UCharPtr += *effective_address.Int32Ptr;
			else if( addrmode & EightBytes )
				regs[regInstr].UCharPtr += *effective_address.Int64Ptr;
		}
		DISPATCH();
	}
	exec_jz:; {
		if( addrmode & Immediate ) {
			const int64_t offset = *regs[regInstr].Int64Ptr++;
			!vm->CondFlag ? (regs[regInstr].UCharPtr += offset) : (void)vm->CondFlag;
		}
		else if( addrmode & Register ) {
			const uint8_t regid = *regs[regInstr].UCharPtr++;
			!vm->CondFlag ? (regs[regInstr].UCharPtr += regs[regid].Int64) : (void)vm->CondFlag;
		}
		else if( addrmode & RegIndirect ) {
			const uint8_t regid = *regs[regInstr].UCharPtr++;
			const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
			if( addrmode & Byte )
				!vm->CondFlag ? (regs[regInstr].UCharPtr += *effective_address.CharPtr) : (void)vm->CondFlag;
			else if( addrmode & TwoBytes )
				!vm->CondFlag ? (regs[regInstr].UCharPtr += *effective_address.ShortPtr) : (void)vm->CondFlag;
			else if( addrmode & FourBytes )
				!vm->CondFlag ? (regs[regInstr].UCharPtr += *effective_address.Int32Ptr) : (void)vm->CondFlag;
			else if( addrmode & EightBytes )
				!vm->CondFlag ? (regs[regInstr].UCharPtr += *effective_address.Int64Ptr) : (void)vm->CondFlag;
		}
		DISPATCH();
	}
	exec_jnz:; {
		if( addrmode & Immediate ) {
			const int64_t offset = *regs[regInstr].Int64Ptr++;
			vm->CondFlag ? (regs[regInstr].UCharPtr += offset) : (void)vm->CondFlag;
		}
		else if( addrmode & Register ) {
			const uint8_t regid = *regs[regInstr].UCharPtr++;
			vm->CondFlag ? (regs[regInstr].UCharPtr += regs[regid].Int64) : (void)vm->CondFlag;
		}
		else if( addrmode & RegIndirect ) {
			const uint8_t regid = *regs[regInstr].UCharPtr++;
			const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
			if( addrmode & Byte )
				vm->CondFlag ? (regs[regInstr].UCharPtr += *effective_address.CharPtr) : (void)vm->CondFlag;
			else if( addrmode & TwoBytes )
				vm->CondFlag ? (regs[regInstr].UCharPtr += *effective_address.ShortPtr) : (void)vm->CondFlag;
			else if( addrmode & FourBytes )
				vm->CondFlag ? (regs[regInstr].UCharPtr += *effective_address.Int32Ptr) : (void)vm->CondFlag;
			else if( addrmode & EightBytes )
				vm->CondFlag ? (regs[regInstr].UCharPtr += *effective_address.Int64Ptr) : (void)vm->CondFlag;
		}
		DISPATCH();
	}
	exec_call:; {
		size_t index = 0;
		if( addrmode & Immediate ) {
			index = ((*regs[regInstr].UInt64Ptr++) - 1);
		}
		else if( addrmode & Register )
			index = (regs[*regs[regInstr].UCharPtr++].UInt64 - 1);
		else if( addrmode & RegIndirect ) {
			const uint8_t regid = *regs[regInstr].UCharPtr++;
			const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
			if( addrmode & EightBytes )
				index = (*effective_address.UInt64Ptr - 1);
		}
		uint8_t *const call_addr = GetFunctionOffsetByIndex(vm->CurrScript.Ptr, index);
		if( !call_addr ) {
			//puts("Tagha_Exec :: exec_call reported 'call_addr' is NULL");
			DISPATCH();
		}
		
		*--regs[regStk].SelfPtr = regs[regInstr];	/* push rip */
		*--regs[regStk].SelfPtr = regs[regBase];	/* push rbp */
		regs[regBase] = regs[regStk];	/* mov rbp, rsp */
		regs[regInstr].UCharPtr = call_addr;
		DISPATCH();
	}
	exec_ret:; {
		regs[regStk] = regs[regBase]; /* mov rsp, rbp */
		regs[regBase] = *regs[regStk].SelfPtr++; /* pop rbp */
		
		/* if we're popping Main's (or whatever called func's) RBP, then halt the whole program. */
		if( regs[regBase].SelfPtr==MainBasePtr )
			goto *dispatch[halt];
		
		regs[regInstr] = *regs[regStk].SelfPtr++; /* pop rip */
		DISPATCH();
	}
	exec_syscall:; {
		/* how many args given to the native call. */
		const uint32_t argcount = *regs[regInstr].UInt32Ptr++;
		size_t index = -1;
		/* trying to directly call a specific native. Allow this by imm only! */
		if( addrmode & Immediate ) {
			index = (-1 - *regs[regInstr].Int64Ptr++);
		}
		else if( addrmode & Register ) {
			index = (-1 - regs[*regs[regInstr].UCharPtr++].Int64);
		}
		else if( addrmode & RegIndirect ) {
			const uint8_t regid = *regs[regInstr].UCharPtr++;
			const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
			index = (-1 - *effective_address.Int64Ptr);
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
		
		union Value params[argcount];
		const size_t bytecount = sizeof(union Value) * argcount;
		/*memset(params, 0, bytecount); */
		
		const size_t reg_params = 8;
		const enum RegID reg_param_initial = regSemkath;
		
		/* save stack space by using the registers for passing arguments. */
		/* the other registers can then be used for other data operations. */
		if( argcount <= reg_params ) {
			memcpy(params, regs+reg_param_initial, bytecount);
		}
		/* if the native has more than a certain num of params, get from both registers and stack. */
		else if( argcount > reg_params ) {
			memcpy(params, regs+reg_param_initial, sizeof(union Value) * reg_params);
			memcpy(params+reg_params, regs[regStk].SelfPtr, sizeof(union Value) * (argcount-reg_params));
			regs[regStk].SelfPtr += (argcount-reg_params);
		}
		regs[regAlaf].UInt64 = 0;
		(*nativeref->VoidFunc)(vm, regs+regAlaf, argcount, params);
		DISPATCH();
	}
#if FLOATING_POINT_OPS
	exec_flt2dbl:; {
		if( addrmode & Register ) {
			const uint8_t regid = *regs[regInstr].UCharPtr++;
			const float f = regs[regid].Float;
			regs[regid].Double = (double)f;
		}
		DISPATCH();
	}
	exec_dbl2flt:; {
		if( addrmode & Register ) {
			const uint8_t regid = *regs[regInstr].UCharPtr++;
			const double d = regs[regid].Double;
			regs[regid].Double = 0;
			regs[regid].Float = (float)d;
		}
		DISPATCH();
	}
	exec_int2dbl:; {
		if( addrmode & Register ) {
			const uint8_t regid = *regs[regInstr].UCharPtr++;
			const uint64_t i = regs[regid].UInt64;
			regs[regid].Double = (double)i;
		}
		DISPATCH();
	}
	exec_int2flt:; {
		if( addrmode & Register ) {
			const uint8_t regid = *regs[regInstr].UCharPtr++;
			const uint64_t i = regs[regid].UInt64;
			regs[regid].UInt64 = 0;
			regs[regid].Float = (float)i;
		}
		DISPATCH();
	}
	exec_addf:; {
		const uint8_t regid = *regs[regInstr].UCharPtr++;
		if( addrmode & Reserved ) {
			if( addrmode & Immediate ) {
				const union Value imm = (union Value){.UInt64 = *regs[regInstr].UInt64Ptr++};
				if( addrmode & FourBytes )
					regs[regid].Float += imm.Float;
				else if( addrmode & EightBytes )
					regs[regid].Double += imm.Double;
			}
			else if( addrmode & Register ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				if( addrmode & FourBytes )
					regs[regid].Float += regs[sec_regid].Float;
				else if( addrmode & EightBytes )
					regs[regid].Double += regs[sec_regid].Double;
			}
			else if( addrmode & RegIndirect ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = regs[sec_regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				regs[regid].Float += *effective_address.FloatPtr;
				
				if( addrmode & FourBytes )
					regs[regid].Float += *effective_address.FloatPtr;
				else if( addrmode & EightBytes )
					regs[regid].Double += *effective_address.DoublePtr;
			}
		}
		else {
			if( addrmode & Immediate ) {
				const union Value imm = (union Value){.UInt64 = *regs[regInstr].UInt64Ptr++};
				const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & FourBytes )
					*effective_address.FloatPtr += imm.Float;
				else if( addrmode & EightBytes )
					*effective_address.DoublePtr += imm.Double;
			}
			else if( addrmode & Register ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & FourBytes )
					*effective_address.FloatPtr += regs[sec_regid].Float;
				else if( addrmode & EightBytes )
					*effective_address.DoublePtr += regs[sec_regid].Double;
			}
		}
		DISPATCH();
	}
	exec_subf:; {
		const uint8_t regid = *regs[regInstr].UCharPtr++;
		if( addrmode & Reserved ) {
			if( addrmode & Immediate ) {
				const union Value imm = (union Value){.UInt64 = *regs[regInstr].UInt64Ptr++};
				if( addrmode & FourBytes )
					regs[regid].Float -= imm.Float;
				else if( addrmode & EightBytes )
					regs[regid].Double -= imm.Double;
			}
			else if( addrmode & Register ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				if( addrmode & FourBytes )
					regs[regid].Float -= regs[sec_regid].Float;
				else if( addrmode & EightBytes )
					regs[regid].Double -= regs[sec_regid].Double;
			}
			else if( addrmode & RegIndirect ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = regs[sec_regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & FourBytes )
					regs[regid].Float -= *effective_address.FloatPtr;
				else if( addrmode & EightBytes )
					regs[regid].Double -= *effective_address.DoublePtr;
			}
		}
		else {
			if( addrmode & Immediate ) {
				const union Value imm = (union Value){.Double = *regs[regInstr].DoublePtr++};
				const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & FourBytes )
					*effective_address.FloatPtr -= imm.Float;
				else if( addrmode & EightBytes )
					*effective_address.DoublePtr -= imm.Double;
			}
			else if( addrmode & Register ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & FourBytes )
					*effective_address.FloatPtr -= regs[sec_regid].Float;
				else if( addrmode & EightBytes )
					*effective_address.DoublePtr -= regs[sec_regid].Double;
			}
		}
		DISPATCH();
	}
	exec_mulf:; {
		const uint8_t regid = *regs[regInstr].UCharPtr++;
		if( addrmode & Reserved ) {
			if( addrmode & Immediate ) {
				const union Value imm = (union Value){.UInt64 = *regs[regInstr].UInt64Ptr++};
				if( addrmode & FourBytes )
					regs[regid].Float *= imm.Float;
				else if( addrmode & EightBytes )
					regs[regid].Double *= imm.Double;
			}
			else if( addrmode & Register ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				if( addrmode & FourBytes )
					regs[regid].Float *= regs[sec_regid].Float;
				else if( addrmode & EightBytes )
					regs[regid].Double *= regs[sec_regid].Double;
			}
			else if( addrmode & RegIndirect ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = regs[sec_regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & FourBytes )
					regs[regid].Float *= *effective_address.FloatPtr;
				else if( addrmode & EightBytes )
					regs[regid].Double *= *effective_address.DoublePtr;
			}
		}
		else {
			if( addrmode & Immediate ) {
				const union Value imm = (union Value){.Double = *regs[regInstr].DoublePtr++};
				const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & FourBytes )
					*effective_address.FloatPtr *= imm.Float;
				else if( addrmode & EightBytes )
					*effective_address.DoublePtr *= imm.Double;
			}
			else if( addrmode & Register ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & FourBytes )
					*effective_address.FloatPtr *= regs[sec_regid].Float;
				else if( addrmode & EightBytes )
					*effective_address.DoublePtr *= regs[sec_regid].Double;
			}
		}
		DISPATCH();
	}
	exec_divf:; {
		const uint8_t regid = *regs[regInstr].UCharPtr++;
		if( addrmode & Reserved ) {
			if( addrmode & Immediate ) {
				const union Value imm = (union Value){.UInt64 = *regs[regInstr].UInt64Ptr++};
				if( addrmode & FourBytes )
					regs[regid].Float /= imm.Float;
				else if( addrmode & EightBytes )
					regs[regid].Double /= imm.Double;
			}
			else if( addrmode & Register ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				if( addrmode & FourBytes )
					regs[regid].Float /= regs[sec_regid].Float;
				else if( addrmode & EightBytes )
					regs[regid].Double /= regs[sec_regid].Double;
			}
			else if( addrmode & RegIndirect ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = regs[sec_regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & FourBytes )
					regs[regid].Float /= *effective_address.FloatPtr;
				else if( addrmode & EightBytes )
					regs[regid].Double /= *effective_address.DoublePtr;
			}
		}
		else {
			if( addrmode & Immediate ) {
				const union Value imm = (union Value){.Double = *regs[regInstr].DoublePtr++};
				const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & FourBytes )
					*effective_address.FloatPtr /= imm.Float;
				else if( addrmode & EightBytes )
					*effective_address.DoublePtr /= imm.Double;
			}
			else if( addrmode & Register ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & FourBytes )
					*effective_address.FloatPtr /= regs[sec_regid].Float;
				else if( addrmode & EightBytes )
					*effective_address.DoublePtr /= regs[sec_regid].Double;
			}
		}
		DISPATCH();
	}
	exec_ltf:; {
		const uint8_t regid = *regs[regInstr].UCharPtr++;
		if( addrmode & Reserved ) {
			if( addrmode & Immediate ) {
				const union Value imm = (union Value){.UInt64 = *regs[regInstr].UInt64Ptr++};
				if( addrmode & FourBytes )
					vm->CondFlag = regs[regid].Float < imm.Float;
				else if( addrmode & EightBytes )
					vm->CondFlag = regs[regid].Double < imm.Double;
			}
			else if( addrmode & Register ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				if( addrmode & FourBytes )
					vm->CondFlag = regs[regid].Float < regs[sec_regid].Float;
				else if( addrmode & EightBytes )
					vm->CondFlag = regs[regid].Double < regs[sec_regid].Double;
			}
			else if( addrmode & RegIndirect ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = regs[sec_regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & FourBytes )
					vm->CondFlag = regs[regid].Float < *effective_address.FloatPtr;
				else if( addrmode & EightBytes )
					vm->CondFlag = regs[regid].Double < *effective_address.DoublePtr;
			}
		}
		else {
			if( addrmode & Immediate ) {
				const union Value imm = (union Value){.Double = *regs[regInstr].DoublePtr++};
				const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & FourBytes )
					vm->CondFlag = *effective_address.FloatPtr < imm.Float;
				else if( addrmode & EightBytes )
					vm->CondFlag = *effective_address.DoublePtr < imm.Double;
			}
			else if( addrmode & Register ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & FourBytes )
					vm->CondFlag = *effective_address.FloatPtr < regs[sec_regid].Float;
				else if( addrmode & EightBytes )
					vm->CondFlag = *effective_address.DoublePtr < regs[sec_regid].Double;
			}
		}
		DISPATCH();
	}
	exec_gtf:; {
		const uint8_t regid = *regs[regInstr].UCharPtr++;
		if( addrmode & Reserved ) {
			if( addrmode & Immediate ) {
				const union Value imm = (union Value){.UInt64 = *regs[regInstr].UInt64Ptr++};
				if( addrmode & FourBytes )
					vm->CondFlag = regs[regid].Float > imm.Float;
				else if( addrmode & EightBytes )
					vm->CondFlag = regs[regid].Double > imm.Double;
			}
			else if( addrmode & Register ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				if( addrmode & FourBytes )
					vm->CondFlag = regs[regid].Float > regs[sec_regid].Float;
				else if( addrmode & EightBytes )
					vm->CondFlag = regs[regid].Double > regs[sec_regid].Double;
			}
			else if( addrmode & RegIndirect ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = regs[sec_regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & FourBytes )
					vm->CondFlag = regs[regid].Float > *effective_address.FloatPtr;
				else if( addrmode & EightBytes )
					vm->CondFlag = regs[regid].Double > *effective_address.DoublePtr;
			}
		}
		else {
			if( addrmode & Immediate ) {
				const union Value imm = (union Value){.Double = *regs[regInstr].DoublePtr++};
				const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & FourBytes )
					vm->CondFlag = *effective_address.FloatPtr > imm.Float;
				else if( addrmode & EightBytes )
					vm->CondFlag = *effective_address.DoublePtr > imm.Double;
			}
			else if( addrmode & Register ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & FourBytes )
					vm->CondFlag = *effective_address.FloatPtr > regs[sec_regid].Float;
				else if( addrmode & EightBytes )
					vm->CondFlag = *effective_address.DoublePtr > regs[sec_regid].Double;
			}
		}
		DISPATCH();
	}
	exec_cmpf:; {
		const uint8_t regid = *regs[regInstr].UCharPtr++;
		if( addrmode & Reserved ) {
			if( addrmode & Immediate ) {
				const union Value imm = (union Value){.UInt64 = *regs[regInstr].UInt64Ptr++};
				if( addrmode & FourBytes )
					vm->CondFlag = regs[regid].Float == imm.Float;
				else if( addrmode & EightBytes )
					vm->CondFlag = regs[regid].Double == imm.Double;
			}
			else if( addrmode & Register ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				if( addrmode & FourBytes )
					vm->CondFlag = regs[regid].Float == regs[sec_regid].Float;
				else if( addrmode & EightBytes )
					vm->CondFlag = regs[regid].Double == regs[sec_regid].Double;
			}
			else if( addrmode & RegIndirect ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = regs[sec_regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & FourBytes )
					vm->CondFlag = regs[regid].Float == *effective_address.FloatPtr;
				else if( addrmode & EightBytes )
					vm->CondFlag = regs[regid].Double == *effective_address.DoublePtr;
			}
		}
		else {
			if( addrmode & Immediate ) {
				const union Value imm = (union Value){.Double = *regs[regInstr].DoublePtr++};
				const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & FourBytes )
					vm->CondFlag = *effective_address.FloatPtr == imm.Float;
				else if( addrmode & EightBytes )
					vm->CondFlag = *effective_address.DoublePtr == imm.Double;
			}
			else if( addrmode & Register ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & FourBytes )
					vm->CondFlag = *effective_address.FloatPtr == regs[sec_regid].Float;
				else if( addrmode & EightBytes )
					vm->CondFlag = *effective_address.DoublePtr == regs[sec_regid].Double;
			}
		}
		DISPATCH();
	}
	exec_neqf:; {
		const uint8_t regid = *regs[regInstr].UCharPtr++;
		if( addrmode & Reserved ) {
			if( addrmode & Immediate ) {
				const union Value imm = (union Value){.UInt64 = *regs[regInstr].UInt64Ptr++};
				if( addrmode & FourBytes )
					vm->CondFlag = regs[regid].Float != imm.Float;
				else if( addrmode & EightBytes )
					vm->CondFlag = regs[regid].Double != imm.Double;
			}
			else if( addrmode & Register ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				if( addrmode & FourBytes )
					vm->CondFlag = regs[regid].Float != regs[sec_regid].Float;
				else if( addrmode & EightBytes )
					vm->CondFlag = regs[regid].Double != regs[sec_regid].Double;
			}
			else if( addrmode & RegIndirect ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = regs[sec_regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & FourBytes )
					vm->CondFlag = regs[regid].Float != *effective_address.FloatPtr;
				else if( addrmode & EightBytes )
					vm->CondFlag = regs[regid].Double != *effective_address.DoublePtr;
			}
		}
		else {
			if( addrmode & Immediate ) {
				const union Value imm = (union Value){.Double = *regs[regInstr].DoublePtr++};
				const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & FourBytes )
					vm->CondFlag = *effective_address.FloatPtr != imm.Float;
				else if( addrmode & EightBytes )
					vm->CondFlag = *effective_address.DoublePtr != imm.Double;
			}
			else if( addrmode & Register ) {
				const uint8_t sec_regid = *regs[regInstr].UCharPtr++;
				const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
				if( addrmode & FourBytes )
					vm->CondFlag = *effective_address.FloatPtr != regs[sec_regid].Float;
				else if( addrmode & EightBytes )
					vm->CondFlag = *effective_address.DoublePtr != regs[sec_regid].Double;
			}
		}
		DISPATCH();
	}
	exec_incf:; {
		const uint8_t regid = *regs[regInstr].UCharPtr++;
		if( addrmode & Register ) {
			if( addrmode & FourBytes )
				++regs[regid].Float;
			else if( addrmode & EightBytes )
				++regs[regid].Double;
		}
		else if( addrmode & RegIndirect ) {
			const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
			if( addrmode & FourBytes )
				++*effective_address.FloatPtr;
			else if( addrmode & EightBytes )
				++*effective_address.DoublePtr;
		}
		DISPATCH();
	}
	exec_decf:; {
		const uint8_t regid = *regs[regInstr].UCharPtr++;
		if( addrmode & Register ) {
			if( addrmode & FourBytes )
				--regs[regid].Float;
			else if( addrmode & EightBytes )
				--regs[regid].Double;
		}
		else if( addrmode & RegIndirect ) {
			const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
			if( addrmode & FourBytes )
				--*effective_address.FloatPtr;
			else if( addrmode & EightBytes )
				--*effective_address.DoublePtr;
		}
		DISPATCH();
	}
	exec_negf:; {
		const uint8_t regid = *regs[regInstr].UCharPtr++;
		if( addrmode & Register ) {
			if( addrmode & FourBytes )
				regs[regid].Float = -regs[regid].Float;
			else if( addrmode & EightBytes )
				regs[regid].Double = -regs[regid].Double;
		}
		else if( addrmode & RegIndirect ) {
			const union Value effective_address = (union Value){.UCharPtr = regs[regid].UCharPtr + *regs[regInstr].Int32Ptr++};
			if( addrmode & FourBytes )
				*effective_address.FloatPtr = -*effective_address.FloatPtr;
			else if( addrmode & EightBytes )
				*effective_address.DoublePtr = -*effective_address.DoublePtr;
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
		//puts("Tagha_RunScript :: ERROR: script contains no 'main' function.");
		return ErrMissingFunc;
	}
	
	/* push argc, argv to registers. */
	union Value MainArgs[argc+1];
	for( int i=0 ; i<=argc ; i++ )
		MainArgs[i].Ptr = argv[i];
	
	vm->Regs[reg_Eh].Ptr = MainArgs;
	vm->Regs[regSemkath].Int32 = argc;
	
	/* check out stack size and align it by the size of union Value. */
	const size_t stacksize = (vm->Module->StackSize + (sizeof(union Value)-1)) & -(sizeof(union Value));
	if( !stacksize )
		return ErrStackSize;
	
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
