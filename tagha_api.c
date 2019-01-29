#include <stdlib.h>
#include <stdio.h>
#ifdef OS_WINDOWS
#	define TAGHA_LIB
#endif
#include "tagha.h"

static int32_t _tagha_module_exec(struct TaghaModule *module)
#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
	__attribute__ ((hot))
#endif
;


#define CLEANUP(ptr)	free((ptr)), (ptr)=NULL

static uint8_t *_make_buffer_from_file(const char filename[restrict])
{
	FILE *restrict script = fopen(filename, "rb");
	if( !script ) {
		fprintf(stderr, "buffer from file Error :: **** file '%s' doesn't exist. ****\n", filename);
		return NULL;
	}
	fseek(script, 0, SEEK_END);
	const long filesize = ftell(script);
	if( filesize <= -1 ) {
		fclose(script);
		fprintf(stderr, "buffer from file Error :: **** filesize (%li) is less than 0. ****\n", filesize);
		return NULL;
	}
	rewind(script);
	
	uint8_t *restrict bytecode = calloc(filesize, sizeof *bytecode);
	const size_t readval = fread(bytecode, sizeof *bytecode, filesize, script);
	fclose(script), script=NULL;
	
	if( readval != (size_t)filesize ) {
		CLEANUP(bytecode);
		fprintf(stderr, "buffer from file Error :: **** fread value (%zu) is not same as filesize (%zu). ****\n", readval, (size_t)filesize);
		return NULL;
	}
	else return bytecode;
}

static bool _read_module_data(struct TaghaModule *const restrict module, uint8_t filedata[])
{
	union TaghaPtr iter = {filedata};
	iter.PtrUInt16++;
	
	const uint32_t stacksize = *iter.PtrUInt32++;
	const size_t allocnode_size = sizeof(struct HarbolAllocNode);
	size_t total_memory_needed = 0;
	total_memory_needed += (stacksize+1) * sizeof(union TaghaVal) + allocnode_size; // get stack size.
	//printf("stacksize == %zu\n", total_memory_needed);
	module->SafeMode = *iter.PtrUInt8++; // read flag byte.
	
	// iterate function table and get bytecode sizes.
	const uint32_t func_table_size = *iter.PtrUInt32++;
	total_memory_needed += (sizeof(struct TaghaItem) + allocnode_size) * func_table_size;
	for( uint32_t i=0 ; i<func_table_size ; i++ ) {
		const uint8_t flag = *iter.PtrUInt8++;
		const uint64_t sizes = *iter.PtrUInt64++;
		const uint32_t cstrlen = sizes & 0xffFFffFF;
		const uint32_t datalen = sizes >> 32;
		if( !flag )
			total_memory_needed += datalen + allocnode_size;
		iter.PtrUInt8 += (cstrlen + datalen);
	}
	//printf("after func table == %zu\n", total_memory_needed);
	
	// iterate global var table
	const uint32_t var_table_size = *iter.PtrUInt32++;
	total_memory_needed += (sizeof(struct TaghaItem) + allocnode_size) * var_table_size;
	for( uint32_t i=0 ; i<var_table_size ; i++ ) {
		iter.PtrUInt8++;
		const uint64_t sizes = *iter.PtrUInt64++;
		const uint32_t cstrlen = sizes & 0xffFFffFF;
		const uint32_t datalen = sizes >> 32;
		total_memory_needed += datalen + allocnode_size;
		iter.PtrUInt8 += (cstrlen + datalen);
	}
	//printf("after var table == %zu\n", total_memory_needed);
	
	harbol_mempool_init(&module->Heap, total_memory_needed);
	const size_t given_heapsize = harbol_mempool_get_heap_size(&module->Heap);
	//printf("total_memory_needed == %zu\ngiven_heapsize == %zu\n", total_memory_needed, given_heapsize);
	if( !given_heapsize ) {
		fprintf(stderr, "reading module file Error :: **** given_heapsize (%zu) is not same as total_memory_needed (%zu). ****\n", given_heapsize, total_memory_needed);
		return false;
	}
	
	iter = (union TaghaPtr){ filedata };
	iter.PtrUInt16++; // skip verifier
	iter.PtrUInt32++; // skip stack size
	iter.PtrUInt8++; // skip module flags
	iter.PtrUInt32++; // skip func table size.
	//printf("func_table_size == '%u'\n", func_table_size);
	for( uint32_t i=0 ; i<func_table_size ; i++ ) {
		const uint8_t flag = *iter.PtrUInt8++;
		const uint64_t sizes = *iter.PtrUInt64++;
		const uint32_t cstrlen = sizes & 0xffFFffFF;
		const uint32_t datalen = sizes >> 32;
		struct TaghaItem *funcitem = harbol_mempool_alloc(&module->Heap, sizeof *funcitem);
		//printf("allocating new func item == %zu\n", harbol_mempool_get_remaining(&module->Heap));
		funcitem->Bytes = datalen;
		funcitem->Flags = flag;
		const char *cstr = iter.Ptr;
		//printf("func cstr == '%s'\n", cstr);
		iter.PtrUInt8 += cstrlen;
		if( !flag ) {
			funcitem->Data = harbol_mempool_alloc(&module->Heap, datalen);
			memcpy(funcitem->Data, iter.Ptr, datalen);
			//printf("allocating new func data == %zu\n", harbol_mempool_get_remaining(&module->Heap));
		}
		harbol_linkmap_insert(&module->FuncMap, cstr, (const union HarbolValue){ .Ptr=funcitem });
		iter.PtrUInt8 += datalen;
	}
	module->FuncBound = module->Heap.HeapBottom - module->Heap.HeapMem;
	//printf("remaining after func table == %zu\n", harbol_mempool_get_remaining(&module->Heap));
	
	//printf("var_table_size == '%u'\n", var_table_size);
	iter.PtrUInt32++; // skip var table size.
	for( uint32_t i=0 ; i<var_table_size ; i++ ) {
		iter.PtrUInt8++;
		const uint64_t sizes = *iter.PtrUInt64++;
		const uint32_t cstrlen = sizes & 0xffFFffFF;
		const uint32_t datalen = sizes >> 32;
		struct TaghaItem *varitem = harbol_mempool_alloc(&module->Heap, sizeof *varitem);
		//printf("allocating new var item == %zu\n", harbol_mempool_get_remaining(&module->Heap));
		varitem->Bytes = datalen;
		const char *cstr = iter.Ptr;
		//printf("var cstr == '%s'\n", cstr);
		iter.PtrUInt8 += cstrlen;
		varitem->Data = harbol_mempool_alloc(&module->Heap, datalen);
		//printf("allocating new var data == %zu\n", harbol_mempool_get_remaining(&module->Heap));
		memcpy(varitem->Data, iter.Ptr, datalen);
		harbol_linkmap_insert(&module->VarMap, cstr, (const union HarbolValue){ .Ptr=varitem });
		iter.PtrUInt8 += datalen;
	}
	//printf("remaining heap pool after var table == %zu\n", harbol_mempool_get_remaining(&module->Heap));
	
	module->StackSize = stacksize;
	module->Stack = harbol_mempool_alloc(&module->Heap, sizeof *module->Stack * (stacksize+1));
	if( !module->Stack ) {
		fprintf(stderr, "reading module file Error :: **** module->Stack is NULL ****\n");
		harbol_linkmap_del(&module->FuncMap, NULL);
		harbol_linkmap_del(&module->VarMap, NULL);
		harbol_mempool_del(&module->Heap);
		return false;
	} else {
		module->regStk.PtrSelf = module->regBase.PtrSelf = module->Stack + stacksize;
		//printf("module->regStk.PtrSelf == '%p'\n", module->regStk.PtrSelf);
		return true;
	}
}

TAGHA_EXPORT struct TaghaModule *tagha_module_new_from_file(const char filename[restrict])
{
	if( !filename )
		return NULL;
	
	uint8_t *restrict bytecode = _make_buffer_from_file(filename);
	if( !bytecode ) {
		fprintf(stderr, "Tagha Module Error :: **** failed to create file data buffer. ****\n");
		return NULL;
	} else if( *(uint16_t *)bytecode != 0xC0DE ) {
		fprintf(stderr, "Tagha Module Error :: **** invalid tagha module: '%s' ****\n", filename);
		CLEANUP(bytecode);
		return NULL;
	}
	
	struct TaghaModule *restrict module = calloc(1, sizeof *module);
	if( !module ) {
		fprintf(stderr, "Tagha Module Error :: **** unable to allocate module ptr for file: '%s' ****\n", filename);
		CLEANUP(bytecode);
		return NULL;
	}
	
	const bool read_result = _read_module_data(module, bytecode);
	CLEANUP(bytecode);
	if( !read_result ) {
		fprintf(stderr, "Tagha Module Error :: **** couldn't allocate tables for file: '%s' ****\n", filename);
		tagha_module_del(module);
		CLEANUP(module);
		return NULL;
	}
	return module;
}

TAGHA_EXPORT struct TaghaModule *tagha_module_new_from_buffer(uint8_t buffer[restrict])
{
	if( !buffer )
		return NULL;
	else if( *(const uint16_t *)buffer != 0xC0DE ) {
		fprintf(stderr, "Tagha Module Error :: **** invalid module buffer '%p' ****\n", buffer);
		return NULL;
	}
	
	struct TaghaModule *restrict module = calloc(1, sizeof *module);
	if( !module ) {
		fprintf(stderr, "Tagha Module Error :: **** unable to allocate module ptr from buffer ****\n");
		return NULL;
	}
	
	const bool read_result = _read_module_data(module, buffer);
	if( !read_result ) {
		fprintf(stderr, "Tagha Module Error :: **** couldn't allocate tables from buffer ****\n");
		tagha_module_del(module);
		CLEANUP(module);
		return NULL;
	}
	else return module;
}

TAGHA_EXPORT bool tagha_module_free(struct TaghaModule **const modref)
{
	if( !modref || !*modref )
		return false;
	tagha_module_del(*modref);
	CLEANUP(*modref);
	return true;
}


TAGHA_EXPORT bool tagha_module_from_file(struct TaghaModule *const restrict module, const char filename[restrict])
{
	if( !module || !filename )
		return false;
	
	uint8_t *restrict bytecode = _make_buffer_from_file(filename);
	if( !bytecode ) {
		fprintf(stderr, "Tagha Module Error :: **** failed to create file data buffer. ****\n");
		return false;
	} else if( *(uint16_t *)bytecode != 0xC0DE ) {
		fprintf(stderr, "Tagha Module Error :: **** invalid tagha module: '%s' ****\n", filename);
		CLEANUP(bytecode);
		return false;
	}
	
	const bool read_result = _read_module_data(module, bytecode);
	CLEANUP(bytecode);
	if( !read_result ) {
		fprintf(stderr, "Tagha Module Error :: **** couldn't allocate tables for file: '%s' ****\n", filename);
		tagha_module_del(module);
		return read_result;
	} else {
		return read_result;
	}
}

TAGHA_EXPORT bool tagha_module_from_buffer(struct TaghaModule *const restrict module, uint8_t buffer[restrict])
{
	if( !module || !buffer )
		return false;
	else if( *(uint16_t *)buffer != 0xC0DE ) {
		fprintf(stderr, "Tagha Module Error :: **** invalid module buffer '%p' ****\n", buffer);
		return false;
	}
	
	const bool read_result = _read_module_data(module, buffer);
	if( !read_result ) {
		fprintf(stderr, "Tagha Module Error :: **** couldn't allocate tables from buffer ****\n");
		tagha_module_del(module);
		return read_result;
	} else {
		return read_result;
	}
}

TAGHA_EXPORT bool tagha_module_del(struct TaghaModule *const module)
{
	if( !module )
		return false;
	harbol_linkmap_del(&module->FuncMap, NULL);
	harbol_linkmap_del(&module->VarMap, NULL);
	harbol_mempool_del(&module->Heap);
	memset(module, 0, sizeof *module);
	return true;
}


TAGHA_EXPORT void tagha_module_print_vm_state(const struct TaghaModule *const module)
{
	if( !module )
		return;
	printf("=== Tagha VM State Info ===\n\nPrinting Registers:\nregister alaf: '%" PRIu64 "'\nregister beth: '%" PRIu64 "'\nregister gamal: '%" PRIu64 "'\nregister dalath: '%" PRIu64 "'\nregister heh: '%" PRIu64 "'\nregister waw: '%" PRIu64 "'\nregister zain: '%" PRIu64 "'\nregister heth: '%" PRIu64 "'\nregister teth: '%" PRIu64 "'\nregister yodh: '%" PRIu64 "'\nregister kaf: '%" PRIu64 "'\nregister lamadh: '%" PRIu64 "'\nregister meem: '%" PRIu64 "'\nregister noon: '%" PRIu64 "'\nregister semkath: '%" PRIu64 "'\nregister eh: '%" PRIu64 "'\nregister peh: '%" PRIu64 "'\nregister sadhe: '%" PRIu64 "'\nregister qof: '%" PRIu64 "'\nregister reesh: '%" PRIu64 "'\nregister sheen: '%" PRIu64 "'\nregister taw: '%" PRIu64 "'\nregister stack pointer: '%p'\nregister base pointer: '%p'\nregister instruction pointer: '%p'\n\nPrinting Condition Flag: %u\n=== End Tagha VM State Info ===\n",
	module->regAlaf.UInt64,
	module->regBeth.UInt64,
	module->regGamal.UInt64,
	module->regDalath.UInt64,
	module->regHeh.UInt64,
	module->regWaw.UInt64,
	module->regZain.UInt64,
	module->regHeth.UInt64,
	module->regTeth.UInt64,
	module->regYodh.UInt64,
	module->regKaf.UInt64,
	module->regLamadh.UInt64,
	module->regMeem.UInt64,
	module->regNoon.UInt64,
	module->regSemkath.UInt64,
	module->reg_Eh.UInt64,
	module->regPeh.UInt64,
	module->regSadhe.UInt64,
	module->regQof.UInt64,
	module->regReesh.UInt64,
	module->regSheen.UInt64,
	module->regTaw.UInt64,
	module->regStk.Ptr,
	module->regBase.Ptr,
	module->regInstr.Ptr,
	module->CondFlag);
}

TAGHA_EXPORT const char *tagha_module_get_error(const struct TaghaModule *const module)
{
	if( !module )
		return "Null VM Pointer";
	
	switch( module->Error ) {
		case ErrInstrBounds:	return "Out of Bound Instruction";
		case ErrNone:			return "None";
		case ErrBadPtr:			return "Null/Invalid Pointer";
		case ErrMissingFunc:	return "Missing Function";
		case ErrInvalidScript:	return "Null/Invalid Script";
		case ErrStackSize:		return "Bad Stack Size";
		case ErrMissingNative:	return "Missing Native";
		case ErrStackOver:		return "Stack Overflow";
		default:				return "User-Defined Error";
	}
}

TAGHA_EXPORT bool tagha_module_register_natives(struct TaghaModule *const module, const struct TaghaNative natives[])
{
	if( !module || !natives )
		return false;
	
	for( const struct TaghaNative *restrict n=natives ; n->NativeCFunc && n->Name ; n++ ) {
		struct TaghaItem *const item = harbol_linkmap_get(&module->FuncMap, n->Name).Ptr;
		if( !item || item->Flags != TAGHA_FLAG_NATIVE )
			continue;
		else {
			item->NativeFunc = n->NativeCFunc;
			item->Flags = TAGHA_FLAG_NATIVE + TAGHA_FLAG_LINKED;
		}
	}
	return true;
}

TAGHA_EXPORT bool tagha_module_register_ptr(struct TaghaModule *const module, const char varname[restrict], void *pvar)
{
	if( !module )
		return false;
	else {
		const uintptr_t
			mem_start = (uintptr_t)module->Heap.HeapMem,
			mem_end = (uintptr_t)(module->Heap.HeapMem + module->FuncBound)
		;
		void **pp_global = tagha_module_get_globalvar_by_name(module, varname);
		if( !pp_global || (uintptr_t)pp_global<mem_start || (uintptr_t)pp_global>=mem_end )
			return false;
		else {
			*pp_global = pvar;
			return true;
		}
	}
}

TAGHA_EXPORT void *tagha_module_get_globalvar_by_name(struct TaghaModule *const module, const char varname[restrict])
{
	if( !module || !varname )
		return NULL;
	else {
		const struct TaghaItem *const item = harbol_linkmap_get(&module->VarMap, varname).Ptr;
		return item ? item->RawPtr : NULL;
	}
}

TAGHA_EXPORT int32_t tagha_module_call(struct TaghaModule *const restrict module, const char func_name[restrict], const size_t args, union TaghaVal params[restrict static args], union TaghaVal *const restrict retval)
{
	if( !module || !func_name )
		return -1;
	else {
		struct TaghaItem *const item = harbol_linkmap_get(&module->FuncMap, func_name).Ptr;
		/* null item? we're missing a function then. */
		if( !item ) {
			module->Error = ErrMissingFunc;
			return -1;
		} else if( item->Flags >= TAGHA_FLAG_NATIVE ) {
			/* make sure our native function was registered first or else we crash ;) */
			if( item->Flags >= TAGHA_FLAG_LINKED ) {
				union TaghaVal ret = (union TaghaVal){0};
				(*item->NativeFunc)(module, &ret, args, params);
				if( retval )
					*retval = ret;
				return module->Error == ErrNone ? 0 : module->Error;
			} else {
				module->Error = ErrMissingNative;
				return -1;
			}
		} else {
			/* remember that arguments must be passed right to left.
				we have enough args to fit in registers. */
			const uint8_t reg_param_initial = TAGHA_FIRST_PARAM_REG;
			const uint8_t reg_params = TAGHA_REG_PARAMS_MAX;
			const size_t bytecount = sizeof(union TaghaVal) * args;
		
			/* save stack space by using the registers for passing arguments.
				the other registers can be used for different operations. */
			if( args <= reg_params ) {
				memcpy(&module->Regs[reg_param_initial], params, bytecount);
			}
			/* if the function has more than a certain num of params, push from both registers and stack. */
			else {
				/* make sure we have the stack space to acommodate the amount of args. */
				if( module->regStk.PtrSelf - (args-reg_params) < module->Stack ) {
					module->Error = ErrStackOver;
					return -1;
				} else {
					memcpy(&module->Regs[reg_param_initial], params, sizeof(union TaghaVal) * reg_params);
					memcpy(module->regStk.PtrSelf, &params[reg_params], sizeof(union TaghaVal) * (args-reg_params));
					module->regStk.PtrSelf -= (args-reg_params);
				}
			}
			(--module->regStk.PtrSelf)->Int64 = 0; /* push NULL return address. */
			*--module->regStk.PtrSelf = module->regBase; /* push rbp */
			module->regBase = module->regStk; /* mov rbp, rsp */
			module->regInstr.Ptr = item->RawPtr;
			if( module->regInstr.Ptr ) {
				const int32_t result = _tagha_module_exec(module);
				module->regInstr.Int64 = 0;
				if( retval )
					memcpy(retval, &module->regAlaf, sizeof *retval);
				if( args>reg_params )
					module->regStk.PtrSelf += (args-reg_params);
				return result;
			} else {
				module->regStk = module->regBase; /* unwind stack */
				module->regStk.PtrSelf += 2;
				/* pop off our params. */
				if( args>reg_params )
					module->regStk.PtrSelf += (args-reg_params);
				module->Error = ErrMissingFunc;
				return -1;
			}
		}
	}
}

TAGHA_EXPORT int32_t tagha_module_run(struct TaghaModule *const restrict module, const int32_t argc, char *sargv[restrict static argc+1])
{
	if( !module )
		return -1;
	else {
		union TaghaVal
			retval = {0},
			main_params[2] = {{0} , {0}} /* hey, it's an owl lmao */
		;
		main_params[0].Int32 = argc;
		int32_t result = 0;
		
		if( sizeof(intptr_t)<sizeof(union TaghaVal) ) {
		/* for 32-bit systems, gotta pad out the pointer values to 64-bit using TaghaVal array.
		 * Also for 32-bit systems, cap out the maximum arguments to 32 pointers.
		 */
#ifndef TAGHA_MAX_MAIN_ARGS_32BIT
#	define TAGHA_MAX_MAIN_ARGS_32BIT    32
#endif
			union TaghaVal argv[TAGHA_MAX_MAIN_ARGS_32BIT+1] = {{0x0}};
			if( sargv )
				for( int32_t i=0 ; i<argc && i<TAGHA_MAX_MAIN_ARGS_32BIT ; i++ )
					argv[i].Ptr = sargv[i];
			main_params[1].PtrSelf = argv;
			result = tagha_module_call(module, "main", 2, main_params, &retval);
		}
		else {
			main_params[1].Ptr = sargv;
			result = tagha_module_call(module, "main", 2, main_params, &retval);
		}
		return !result ? retval.Int32 : result;
	}
}

TAGHA_EXPORT void tagha_module_throw_error(struct TaghaModule *const module, const int32_t err)
{
	if( !module || !err )
		return;
	module->Error = err;
}

TAGHA_EXPORT void tagha_module_force_safemode(struct TaghaModule *const module)
{
	if( !module )
		return;
	module->SafeMode = true;
}

/*
TAGHA_EXPORT void tagha_module_jit_compile(struct TaghaModule *const module, void *(*const jitfunc)(const uint8_t *, size_t))
{
	if( !module || !jitfunc )
		return;
	else {
		const union HarbolValue *const end = harbol_linkmap_get_iter_end_count(&module->FuncMap);
		for( const union HarbolValue *iter=harbol_linkmap_get_iter(&module->FuncMap) ; iter && iter<end ; iter++ ) {
			struct TaghaItem *const i = iter->KvPairPtr->Data.Ptr;
			// trying to JIT something that's already compiled? lol.
			if( i->Flags & TAGHA_FLAG_NATIVE )
				continue;
			else {
				uint8_t *bytecode = i->Data;
				i->Data = NULL;
				void *func = (*jitfunc)(bytecode, i->Bytes);
				if( func ) {
					// return the memory back to the pool since it's useless now.
					harbol_mempool_dealloc(&module->Heap, bytecode);
					i->Flags = TAGHA_FLAG_NATIVE | TAGHA_FLAG_LINKED;
					i->Bytes = 8;
					i->RawPtr = func;
				} else {
					i->Data = bytecode;
				}
			}
		}
	}
}
*/
//#include <unistd.h>	/* sleep() func */

static int32_t _tagha_module_exec(struct TaghaModule *const vm)
{
	/* pc is restricted and must not access beyond the function table! */
	union TaghaPtr pc = {vm->regInstr.PtrUInt8};
	
	const uint8_t
		*const mem_start = vm->Heap.HeapMem,
		*const mem_end = vm->Heap.HeapMem + vm->FuncBound
	;
	
#define X(x) #x ,
	/* for debugging purposes. */
	//const char *const restrict opcode2str[] = { TAGHA_INSTR_SET };
#undef X
	
#define X(x) &&exec_##x ,
	/* our instruction dispatch table. */
	static const void *const restrict dispatch[] = { TAGHA_INSTR_SET };
#undef X
#undef TAGHA_INSTR_SET
	
#	define OLDDISPATCH() \
		const uint8_t instr = *pc.PtrUInt8++; \
		\
		if( instr>nop ) { \
			vm->Error = ErrInstrBounds; \
			return -1; \
		} \
		\
		/*usleep(100);*/ \
		printf("dispatching to '%s'\n", opcode2str[instr]); \
		/*tagha_module_print_vm_state(vm);*/ \
		goto *dispatch[instr]
	
#	define DISPATCH() goto *dispatch[*pc.PtrUInt8++]
	
	exec_nop: {
		DISPATCH();
	}
	/* push immediate value. */
	exec_pushi: { /* char: opcode | i64: imm */
		*--vm->regStk.PtrSelf = *pc.PtrVal++;
		DISPATCH();
	}
	/* push a register's contents. */
	exec_push: { /* char: opcode | char: regid */
		*--vm->regStk.PtrSelf = vm->Regs[*pc.PtrUInt8++];
		DISPATCH();
	}
	/* pushargs: pushes the function argument registers to the stack */
	/*
	exec_pushargs: {
		vm->regStk.PtrSelf -= TAGHA_REG_PARAMS_MAX;
		memcpy(vm->regStk.PtrSelf, &vm->TAGHA_FIRST_PARAM_REG, TAGHA_REG_PARAMS_MAX);
		DISPATCH();
	}
	*/
	/* pops a value from the stack into a register then reduces stack by 8 bytes. */
	exec_pop: { /* char: opcode | char: regid */
		vm->Regs[*pc.PtrUInt8++] = *vm->regStk.PtrSelf++;
		DISPATCH();
	}
	
	exec_loadglobal: { /* char: opcode | char: regid | u64: index */
		const uint8_t regid = *pc.PtrUInt8++;
		const struct TaghaItem *const global = vm->VarMap.Order.Table[*pc.PtrUInt64++].KvPairPtr->Data.Ptr;
		vm->Regs[regid].Ptr = global->RawPtr;
		DISPATCH();
	}
	/* loads a function index which could be a native */
	exec_loadfunc: { /* char: opcode | char: regid | i64: index */
		const uint8_t regid = *pc.PtrUInt8++;
		vm->Regs[regid] = *pc.PtrVal++;
		DISPATCH();
	}
	exec_loadaddr: { /* char: opcode | char: regid1 | char: regid2 | i32: offset */
		const uint16_t regids = *pc.PtrUInt16++;
		vm->Regs[regids & 0xff].PtrUInt8 = vm->Regs[regids >> 8].PtrUInt8 + *pc.PtrInt32++;
		DISPATCH();
	}
	
	exec_movi: { /* char: opcode | char: regid | i64: imm */
		const uint8_t regid = *pc.PtrUInt8++;
		vm->Regs[regid] = *pc.PtrVal++;
		DISPATCH();
	}
	exec_mov: { /* char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.PtrUInt16++;
		vm->Regs[regids & 0xff] = vm->Regs[regids >> 8];
		DISPATCH();
	}
	
	exec_ld1: { /* char: opcode | char: dest reg | char: src reg | i32: offset */
		const uint16_t regids = *pc.PtrUInt16++;
		const union TaghaPtr mem = {vm->Regs[regids >> 8].PtrUInt8 + *pc.PtrInt32++};
		if( vm->SafeMode && (mem.PtrUInt8 < mem_start || mem.PtrUInt8 >= mem_end) ) {
			vm->Error = ErrBadPtr;
			return -1;
		} else {
			vm->Regs[regids & 0xff].UInt64 = (uint64_t) *mem.PtrUInt8;
			DISPATCH();
		}
	}
	exec_ld2: { /* char: opcode | char: dest reg | char: src reg | i32: offset */
		const uint16_t regids = *pc.PtrUInt16++;
		const union TaghaPtr mem = {vm->Regs[regids >> 8].PtrUInt8 + *pc.PtrInt32++};
		if( vm->SafeMode && (mem.PtrUInt8 < mem_start || mem.PtrUInt8+1 >= mem_end) ) {
			vm->Error = ErrBadPtr;
			return -1;
		} else {
			vm->Regs[regids & 0xff].UInt64 = (uint64_t) *mem.PtrUInt16;
			DISPATCH();
		}
	}
	exec_ld4: { /* char: opcode | char: dest reg | char: src reg | i32: offset */
		const uint16_t regids = *pc.PtrUInt16++;
		const union TaghaPtr mem = {vm->Regs[regids >> 8].PtrUInt8 + *pc.PtrInt32++};
		if( vm->SafeMode && (mem.PtrUInt8 < mem_start || mem.PtrUInt8+3 >= mem_end) ) {
			vm->Error = ErrBadPtr;
			return -1;
		} else {
			vm->Regs[regids & 0xff].UInt64 = (uint64_t) *mem.PtrUInt32;
			DISPATCH();
		}
	}
	exec_ld8: { /* char: opcode | char: dest reg | char: src reg | i32: offset */
		const uint16_t regids = *pc.PtrUInt16++;
		const union TaghaPtr mem = {vm->Regs[regids >> 8].PtrUInt8 + *pc.PtrInt32++};
		if( vm->SafeMode && (mem.PtrUInt8 < mem_start || mem.PtrUInt8+7 >= mem_end) ) {
			vm->Error = ErrBadPtr;
			return -1;
		} else {
			vm->Regs[regids & 0xff].UInt64 = *mem.PtrUInt64;
			DISPATCH();
		}
	}
	
	exec_st1: { /* char: opcode | char: dest reg | char: src reg | i32: offset */
		const uint16_t regids = *pc.PtrUInt16++;
		const union TaghaPtr mem = {vm->Regs[regids & 0xff].PtrUInt8 + *pc.PtrInt32++};
		if( vm->SafeMode && (mem.PtrUInt8 < mem_start || mem.PtrUInt8 >= mem_end) ) {
			vm->Error = ErrBadPtr;
			return -1;
		} else {
			*mem.PtrUInt8 = vm->Regs[regids >> 8].UInt8;
			DISPATCH();
		}
	}
	exec_st2: { /* char: opcode | char: dest reg | char: src reg | i32: offset */
		const uint16_t regids = *pc.PtrUInt16++;
		const union TaghaPtr mem = {vm->Regs[regids & 0xff].PtrUInt8 + *pc.PtrInt32++};
		if( vm->SafeMode && (mem.PtrUInt8 < mem_start || mem.PtrUInt8+1 >= mem_end) ) {
			vm->Error = ErrBadPtr;
			return -1;
		} else {
			*mem.PtrUInt16 = vm->Regs[regids >> 8].UInt16;
			DISPATCH();
		}
	}
	exec_st4: { /* char: opcode | char: dest reg | char: src reg | i32: offset */
		const uint16_t regids = *pc.PtrUInt16++;
		const union TaghaPtr mem = {vm->Regs[regids & 0xff].PtrUInt8 + *pc.PtrInt32++};
		if( vm->SafeMode && (mem.PtrUInt8 < mem_start || mem.PtrUInt8+3 >= mem_end) ) {
			vm->Error = ErrBadPtr;
			return -1;
		} else {
			*mem.PtrUInt32 = vm->Regs[regids >> 8].UInt32;
			DISPATCH();
		}
	}
	exec_st8: { /* char: opcode | char: dest reg | char: src reg | i32: offset */
		const uint16_t regids = *pc.PtrUInt16++;
		const union TaghaPtr mem = {vm->Regs[regids & 0xff].PtrUInt8 + *pc.PtrInt32++};
		if( vm->SafeMode && (mem.PtrUInt8 < mem_start || mem.PtrUInt8+7 >= mem_end) ) {
			vm->Error = ErrBadPtr;
			return -1;
		} else {
			*mem.PtrUInt64 = vm->Regs[regids >> 8].UInt64;
			DISPATCH();
		}
	}
	
	exec_add: { /* char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.PtrUInt16++;
		vm->Regs[regids & 0xff].Int64 += vm->Regs[regids >> 8].Int64;
		DISPATCH();
	}
	exec_sub: { /* char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.PtrUInt16++;
		vm->Regs[regids & 0xff].Int64 -= vm->Regs[regids >> 8].Int64;
		DISPATCH();
	}
	exec_mul: { /* char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.PtrUInt16++;
		vm->Regs[regids & 0xff].Int64 *= vm->Regs[regids >> 8].Int64;
		DISPATCH();
	}
	exec_divi: { /* char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.PtrUInt16++;
		vm->Regs[regids & 0xff].Int64 /= vm->Regs[regids >> 8].Int64;
		DISPATCH();
	}
	exec_mod: { /* char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.PtrUInt16++;
		vm->Regs[regids & 0xff].Int64 %= vm->Regs[regids >> 8].Int64;
		DISPATCH();
	}
	exec_bit_and: { /* char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.PtrUInt16++;
		vm->Regs[regids & 0xff].UInt64 &= vm->Regs[regids >> 8].UInt64;
		DISPATCH();
	}
	exec_bit_or: { /* char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.PtrUInt16++;
		vm->Regs[regids & 0xff].UInt64 |= vm->Regs[regids >> 8].UInt64;
		DISPATCH();
	}
	exec_bit_xor: { /* char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.PtrUInt16++;
		vm->Regs[regids & 0xff].UInt64 ^= vm->Regs[regids >> 8].UInt64;
		DISPATCH();
	}
	exec_shl: { /* char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.PtrUInt16++;
		vm->Regs[regids & 0xff].UInt64 <<= vm->Regs[regids >> 8].UInt64;
		DISPATCH();
	}
	exec_shr: { /* char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.PtrUInt16++;
		vm->Regs[regids & 0xff].UInt64 >>= vm->Regs[regids >> 8].UInt64;
		DISPATCH();
	}
	exec_bit_not: { /* char: opcode | char: regid */
		const uint8_t regid = *pc.PtrUInt8++;
		vm->Regs[regid].UInt64 = ~vm->Regs[regid].UInt64;
		DISPATCH();
	}
	exec_inc: { /* char: opcode | char: regid */
		const uint8_t regid = *pc.PtrUInt8++;
		++vm->Regs[regid].UInt64;
		DISPATCH();
	}
	exec_dec: { /* char: opcode | char: regid */
		const uint8_t regid = *pc.PtrUInt8++;
		--vm->Regs[regid].UInt64;
		DISPATCH();
	}
	exec_neg: { /* char: opcode | char: regid */
		const uint8_t regid = *pc.PtrUInt8++;
		vm->Regs[regid].Int64 = -vm->Regs[regid].Int64;
		DISPATCH();
	}
	
	exec_ilt: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.PtrUInt16++;
		vm->CondFlag = vm->Regs[regids & 0xff].Int64 < vm->Regs[regids >> 8].Int64;
		DISPATCH();
	}
	exec_ile: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.PtrUInt16++;
		vm->CondFlag = vm->Regs[regids & 0xff].Int64 <= vm->Regs[regids >> 8].Int64;
		DISPATCH();
	}
	
	exec_igt: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.PtrUInt16++;
		vm->CondFlag = vm->Regs[regids & 0xff].Int64 > vm->Regs[regids >> 8].Int64;
		DISPATCH();
	}
	exec_ige: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.PtrUInt16++;
		vm->CondFlag = vm->Regs[regids & 0xff].Int64 >= vm->Regs[regids >> 8].Int64;
		DISPATCH();
	}
	
	exec_ult: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.PtrUInt16++;
		vm->CondFlag = vm->Regs[regids & 0xff].UInt64 < vm->Regs[regids >> 8].UInt64;
		DISPATCH();
	}
	exec_ule: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.PtrUInt16++;
		vm->CondFlag = vm->Regs[regids & 0xff].UInt64 <= vm->Regs[regids >> 8].UInt64;
		DISPATCH();
	}
	
	exec_ugt: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.PtrUInt16++;
		vm->CondFlag = vm->Regs[regids & 0xff].UInt64 > vm->Regs[regids >> 8].UInt64;
		DISPATCH();
	}
	exec_uge: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.PtrUInt16++;
		vm->CondFlag = vm->Regs[regids & 0xff].UInt64 >= vm->Regs[regids >> 8].UInt64;
		DISPATCH();
	}
	
	exec_cmp: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.PtrUInt16++;
		vm->CondFlag = vm->Regs[regids & 0xff].UInt64 == vm->Regs[regids >> 8].UInt64;
		DISPATCH();
	}
	exec_neq: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.PtrUInt16++;
		vm->CondFlag = vm->Regs[regids & 0xff].UInt64 != vm->Regs[regids >> 8].UInt64;
		DISPATCH();
	}
	exec_jmp: { /* char: opcode | i64: offset */
		const int64_t offset = *pc.PtrInt64++;
		pc.PtrUInt8 += offset;
		DISPATCH();
	}
	exec_jz: { /* char: opcode | i64: offset */
		const int64_t offset = *pc.PtrInt64++;
		if( !vm->CondFlag ) {
			pc.PtrUInt8 += offset;
			DISPATCH();
		} else {
			DISPATCH();
		}
	}
	exec_jnz: { /* char: opcode | i64: offset */
		const int64_t offset = *pc.PtrInt64++;
		if( vm->CondFlag ) {
			pc.PtrUInt8 += offset;
			DISPATCH();
		} else {
			DISPATCH();
		}
	}
	exec_call: { /* char: opcode | i64: offset */
		const int64_t index = *pc.PtrInt64++;
		if( !index ) {
			vm->Error = ErrMissingFunc;
			goto *dispatch[halt];
		} else {
			const struct TaghaItem *const func = vm->FuncMap.Order.Table[index>0 ? (index - 1) : (-1 - index)].KvPairPtr->Data.Ptr;
			if( func->Flags & TAGHA_FLAG_NATIVE ) {
				if( func->Flags != TAGHA_FLAG_NATIVE+TAGHA_FLAG_LINKED ) {
					vm->Error = ErrMissingNative;
					goto *dispatch[halt];
				} else {
					const uint8_t reg_param_initial = TAGHA_FIRST_PARAM_REG;
					const uint8_t reg_params = TAGHA_REG_PARAMS_MAX;
					union TaghaVal retval = (union TaghaVal){0};
					const size_t args = vm->regAlaf.SizeInt;
					
					/* save stack space by using the registers for passing arguments.
						the other registers can then be used for other data operations. */
					if( args <= reg_params ) {
						(*func->NativeFunc)(vm, &retval, args, vm->Regs+reg_param_initial);
					} else {
						/* if the native call has more than a certain num of params, get all params from stack. */
						(*func->NativeFunc)(vm, &retval, args, vm->regStk.PtrSelf);
						vm->regStk.PtrSelf += args;
					}
					memcpy(&vm->regAlaf, &retval, sizeof retval);
					
					if( vm->Error != ErrNone ) {
						return vm->Error;
					} else {
						DISPATCH();
					}
				}
			} else {
				(--vm->regStk.PtrSelf)->Ptr = pc.Ptr;	/* push rip */
				*--vm->regStk.PtrSelf = vm->regBase;	/* push rbp */
				vm->regBase = vm->regStk;	/* mov rbp, rsp */
				pc.PtrUInt8 = func->Data;
				DISPATCH();
			}
		}
	}
	exec_callr: { /* char: opcode | char: regid */
		const uint8_t regid = *pc.PtrUInt8++;
		const int64_t index = vm->Regs[regid].Int64;
		if( !index ) {
			vm->Error = ErrMissingFunc;
			goto *dispatch[halt];
		} else {
			const struct TaghaItem *const func = vm->FuncMap.Order.Table[index>0 ? (index - 1) : (-1 - index)].KvPairPtr->Data.Ptr;
			if( func->Flags & TAGHA_FLAG_NATIVE ) {
				if( func->Flags != TAGHA_FLAG_NATIVE+TAGHA_FLAG_LINKED ) {
					vm->Error = ErrMissingNative;
					goto *dispatch[halt];
				} else {
					const uint8_t reg_param_initial = TAGHA_FIRST_PARAM_REG;
					const uint8_t reg_params = TAGHA_REG_PARAMS_MAX;
					union TaghaVal retval = (union TaghaVal){0};
					const size_t args = vm->regAlaf.SizeInt;
					
					/* save stack space by using the registers for passing arguments.
						the other registers can then be used for other data operations. */
					if( args <= reg_params ) {
						(*func->NativeFunc)(vm, &retval, args, vm->Regs+reg_param_initial);
					} else {
						/* if the native call has more than a certain num of params, get all params from stack. */
						(*func->NativeFunc)(vm, &retval, args, vm->regStk.PtrSelf);
						vm->regStk.PtrSelf += args;
					}
					memcpy(&vm->regAlaf, &retval, sizeof retval);
					
					if( vm->Error != ErrNone ) {
						return vm->Error;
					} else {
						DISPATCH();
					}
				}
			} else {
				(--vm->regStk.PtrSelf)->Ptr = pc.Ptr;	/* push rip */
				*--vm->regStk.PtrSelf = vm->regBase;	/* push rbp */
				vm->regBase = vm->regStk;	/* mov rbp, rsp */
				pc.PtrUInt8 = func->Data;
				DISPATCH();
			}
		}
	}
	exec_ret: { /* char: opcode */
		vm->regStk = vm->regBase; /* mov rsp, rbp */
		vm->regBase = *vm->regStk.PtrSelf++; /* pop rbp */
		pc.Ptr = (*vm->regStk.PtrSelf++).Ptr; /* pop rip */
		if( !pc.Ptr ) {
			return vm->regAlaf.Int32;
		} else {
			DISPATCH();
		}
	}
	
#ifdef TAGHA_FLOATING_POINT_OPS
	exec_flt2dbl: { /* treat as no op if one float is defined but not the other. */
	 /* char: opcode | char: reg id */
		const uint8_t regid = *pc.PtrUInt8++;
#	if defined(__TAGHA_FLOAT32_DEFINED) && defined(__TAGHA_FLOAT64_DEFINED)
		const float f = vm->Regs[regid].Float;
		vm->Regs[regid].Double = (double)f;
#	endif
		DISPATCH();
	}
	exec_dbl2flt: { /* char: opcode | char: reg id */
		const uint8_t regid = *pc.PtrUInt8++;
#	if defined(__TAGHA_FLOAT32_DEFINED) && defined(__TAGHA_FLOAT64_DEFINED)
		const double d = vm->Regs[regid].Double;
		vm->Regs[regid].Int64 = 0;
		vm->Regs[regid].Float = (float)d;
#	endif
		DISPATCH();
	}
	exec_int2dbl: { /* char: opcode | char: reg id */
		const uint8_t regid = *pc.PtrUInt8++;
#	ifdef __TAGHA_FLOAT64_DEFINED
		const int64_t i = vm->Regs[regid].Int64;
		vm->Regs[regid].Double = (double)i;
#	endif
		DISPATCH();
	}
	exec_int2flt: { /* char: opcode | char: reg id */
		const uint8_t regid = *pc.PtrUInt8++;
#	ifdef __TAGHA_FLOAT32_DEFINED
		const int64_t i = vm->Regs[regid].Int64;
		vm->Regs[regid].Int64 = 0;
		vm->Regs[regid].Float = (float)i;
#	endif
		DISPATCH();
	}
	
	exec_addf: { /* char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.PtrUInt16++;
#	if defined(__TAGHA_FLOAT64_DEFINED) /* if doubles are defined, regardless whether float is or not */
		vm->Regs[regids & 0xff].Double += vm->Regs[regids >> 8].Double;
#	elif defined(__TAGHA_FLOAT32_DEFINED)
		vm->Regs[regids & 0xff].Float += vm->Regs[regids >> 8].Float;
#	endif
		DISPATCH();
	}
	exec_subf: { /* char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.PtrUInt16++;
#	if defined(__TAGHA_FLOAT64_DEFINED)
		vm->Regs[regids & 0xff].Double -= vm->Regs[regids >> 8].Double;
#	elif defined(__TAGHA_FLOAT32_DEFINED)
		vm->Regs[regids & 0xff].Float -= vm->Regs[regids >> 8].Float;
#	endif
		DISPATCH();
	}
	exec_mulf: { /* char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.PtrUInt16++;
#	if defined(__TAGHA_FLOAT64_DEFINED)
		vm->Regs[regids & 0xff].Double *= vm->Regs[regids >> 8].Double;
#	elif defined(__TAGHA_FLOAT32_DEFINED)
		vm->Regs[regids & 0xff].Float *= vm->Regs[regids >> 8].Float;
#	endif
		DISPATCH();
	}
	exec_divf: { /* char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.PtrUInt16++;
#	if defined(__TAGHA_FLOAT64_DEFINED)
		vm->Regs[regids & 0xff].Double /= vm->Regs[regids >> 8].Double;
#	elif defined(__TAGHA_FLOAT32_DEFINED)
		vm->Regs[regids & 0xff].Float /= vm->Regs[regids >> 8].Float;
#	endif
		DISPATCH();
	}
	
	exec_ltf: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.PtrUInt16++;
#	if defined(__TAGHA_FLOAT64_DEFINED)
		vm->CondFlag = vm->Regs[regids & 0xff].Double < vm->Regs[regids >> 8].Double;
#	elif defined(__TAGHA_FLOAT32_DEFINED)
		vm->CondFlag = vm->Regs[regids & 0xff].Float < vm->Regs[regids >> 8].Float;
#	endif
		DISPATCH();
	}
	exec_lef: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.PtrUInt16++;
#	if defined(__TAGHA_FLOAT64_DEFINED)
		vm->CondFlag = vm->Regs[regids & 0xff].Double <= vm->Regs[regids >> 8].Double;
#	elif defined(__TAGHA_FLOAT32_DEFINED)
		vm->CondFlag = vm->Regs[regids & 0xff].Float <= vm->Regs[regids >> 8].Float;
#	endif
		DISPATCH();
	}
	
	exec_gtf: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.PtrUInt16++;
#	if defined(__TAGHA_FLOAT64_DEFINED)
		vm->CondFlag = vm->Regs[regids & 0xff].Double > vm->Regs[regids >> 8].Double;
#	elif defined(__TAGHA_FLOAT32_DEFINED)
		vm->CondFlag = vm->Regs[regids & 0xff].Float > vm->Regs[regids >> 8].Float;
#	endif
		DISPATCH();
	}
	exec_gef: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.PtrUInt16++;
#	if defined(__TAGHA_FLOAT64_DEFINED)
		vm->CondFlag = vm->Regs[regids & 0xff].Double >= vm->Regs[regids >> 8].Double;
#	elif defined(__TAGHA_FLOAT32_DEFINED)
		vm->CondFlag = vm->Regs[regids & 0xff].Float >= vm->Regs[regids >> 8].Float;
#	endif
		DISPATCH();
	}
	
	exec_cmpf: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.PtrUInt16++;
#	if defined(__TAGHA_FLOAT64_DEFINED)
		vm->CondFlag = vm->Regs[regids & 0xff].Double == vm->Regs[regids >> 8].Double;
#	elif defined(__TAGHA_FLOAT32_DEFINED)
		vm->CondFlag = vm->Regs[regids & 0xff].Float == vm->Regs[regids >> 8].Float;
#	endif
		DISPATCH();
	}
	exec_neqf: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.PtrUInt16++;
#	if defined(__TAGHA_FLOAT64_DEFINED)
		vm->CondFlag = vm->Regs[regids & 0xff].Double != vm->Regs[regids >> 8].Double;
#	elif defined(__TAGHA_FLOAT32_DEFINED)
		vm->CondFlag = vm->Regs[regids & 0xff].Float != vm->Regs[regids >> 8].Float;
#	endif
		DISPATCH();
	}
	exec_incf: { /* char: opcode | char: regid */
		const uint8_t regid = *pc.PtrUInt8++;
#	if defined(__TAGHA_FLOAT64_DEFINED)
		++vm->Regs[regid].Double;
#	elif defined(__TAGHA_FLOAT32_DEFINED)
		++vm->Regs[regid].Float;
#	endif
		DISPATCH();
	}
	exec_decf: { /* char: opcode | char: regid */
		const uint8_t regid = *pc.PtrUInt8++;
#	if defined(__TAGHA_FLOAT64_DEFINED)
		--vm->Regs[regid].Double;
#	elif defined(__TAGHA_FLOAT32_DEFINED)
		--vm->Regs[regid].Float;
#	endif
		DISPATCH();
	}
	exec_negf: { /* char: opcode | char: regid */
		const uint8_t regid = *pc.PtrUInt8++;
#	if defined(__TAGHA_FLOAT64_DEFINED)
		vm->Regs[regid].Double = -vm->Regs[regid].Double;
#	elif defined(__TAGHA_FLOAT32_DEFINED)
		vm->Regs[regid].Float = -vm->Regs[regid].Float;
#	endif
		DISPATCH();
	}
#endif
	exec_halt:
	//tagha_module_print_vm_state(vm);
	return vm->regAlaf.Int32;
}
