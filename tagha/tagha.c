#include <stdlib.h>
#include <stdio.h>
#ifdef OS_WINDOWS
#	define TAGHA_LIB
#endif
#include "tagha.h"

static NO_NULL HOT int32_t _tagha_module_exec(struct TaghaModule *module);
static NEVER_NULL(1,2) int32_t _tagha_module_start(struct TaghaModule *module,
															const struct TaghaItem *func,
															size_t args,
															const union TaghaVal params[],
															union TaghaVal *retval);



static NO_NULL struct TaghaItem *_tagha_key_get_item(const struct TaghaItemMap itemmap,
															const char key[restrict static 1])
{
	if( itemmap.hashlen==0 )
		return NULL;
	else {
		const size_t hash = string_hash(key) % itemmap.hashlen;
		for( size_t i=0; i<itemmap.buckets[hash].len; i++ ) {
			if( !strncmp(itemmap.buckets[hash].table[i].key, key, itemmap.buckets[hash].table[i].keylen) )
				return itemmap.buckets[hash].table[i].val;
		}
		return NULL;
	}
}


static NO_NULL bool _read_module_data(struct TaghaModule *const restrict module, uint8_t filedata[const static 1])
{
	module->script = filedata;
	union HarbolBinIter iter = { .uint8 = filedata };
	iter.uint16++;
	
	size_t largest_funcs_hash=0, largest_vars_hash=0;
	const uint32_t stacksize = *iter.uint32++;
	const uint32_t memsize = *iter.uint32++;
	module->flags = *iter.uint8++;
	
	// iterate function table and get bytecode sizes.
	const uint32_t func_table_size = *iter.uint32++;
	module->funcs.arrlen = func_table_size;
	for( uint32_t i=0; i<func_table_size; i++ ) {
		const uint8_t flag = *iter.uint8++;
		const uint64_t sizes = *iter.uint64++;
		const uint32_t cstrlen = sizes & 0xffFFffFF;
		const uint32_t datalen = sizes >> 32;
		const char *cstr = iter.string;
		const size_t hash = string_hash(cstr) % module->funcs.arrlen;
		if( hash > largest_funcs_hash )
			largest_funcs_hash = hash;
		
		module->funcs.hashlen = largest_funcs_hash + 1;
		iter.uint8 += ( !flag ) ? (cstrlen + datalen) : cstrlen;
	}
	
	// iterate global var table
	const uint32_t var_table_size = *iter.uint32++;
	module->vars.arrlen = var_table_size;
	for( uint32_t i=0; i<var_table_size; i++ ) {
		iter.uint8++;
		const uint64_t sizes = *iter.uint64++;
		const uint32_t cstrlen = sizes & 0xffFFffFF;
		const uint32_t datalen = sizes >> 32;
		const char *cstr = iter.string;
		const size_t hash = string_hash(cstr) % module->vars.arrlen;
		if( hash > largest_vars_hash )
			largest_vars_hash = hash;
		
		module->vars.hashlen = largest_vars_hash + 1;
		iter.uint8 += (cstrlen + datalen);
	}
	
	module->heap = harbol_mempool_from_buffer(iter.uint8, memsize);
	module->heap.freelist.auto_defrag = true;
	module->heap.freelist.max_nodes = 0;
	const size_t given_heapsize = harbol_mempool_mem_remaining(&module->heap);
	if( !given_heapsize || given_heapsize != memsize ) {
		fprintf(stderr, "Tagha Module File Error :: **** given heapsize (%zu) is not same as required memory size! (%u). ****\n", given_heapsize, memsize);
		return false;
	}
	
	iter.uint8 = filedata;
	iter.uint8 += (sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint32_t));
	/*
	iter.uint16++; // skip verifier
	iter.uint32++; // skip stack size
	iter.uint32++; // skip mem size
	iter.uint8++; // skip module flags
	iter.uint32++; // skip func table size.
	*/
	
	module->funcs.buckets = harbol_mempool_alloc(&module->heap, sizeof *module->funcs.buckets * module->funcs.hashlen);
	module->funcs.array = harbol_mempool_alloc(&module->heap, sizeof *module->funcs.array * module->funcs.arrlen);
	for( uint32_t i=0; i<func_table_size; i++ ) {
		const uint8_t flag = *iter.uint8++;
		const uint64_t sizes = *iter.uint64++;
		const uint32_t cstrlen = sizes & 0xffFFffFF;
		const uint32_t datalen = sizes >> 32;
		const char *cstr = iter.string;
		const size_t hash = string_hash(cstr) % module->funcs.hashlen;
		iter.uint8 += cstrlen;
		const struct TaghaItem funcitem = { .bytes=datalen, .flags=flag, .item.stream=(!flag) ? iter.uint8 : NULL };
		
		module->funcs.array[i] = funcitem;
		module->funcs.buckets[hash].table = harbol_mempool_realloc(&module->heap, module->funcs.buckets[hash].table, sizeof *module->funcs.buckets[hash].table * (module->funcs.buckets[hash].len + 1));
		if( module->funcs.buckets[hash].table==NULL ) {
			fprintf(stderr, "Tagha Module File Error :: **** unable to allocate enough space for function table: %zu bytes ****\n", sizeof *module->vars.buckets[hash].table * (module->vars.buckets[hash].len + 1));
			return false;
		}
		module->funcs.buckets[hash].table[module->funcs.buckets[hash].len].key = cstr;
		module->funcs.buckets[hash].table[module->funcs.buckets[hash].len].keylen = cstrlen - 1;
		module->funcs.buckets[hash].table[module->funcs.buckets[hash].len].val = &module->funcs.array[i];
		module->funcs.buckets[hash].len++;
		
		if( !flag )
			iter.uint8 += datalen;
	}
	module->end_seg = module->heap.stack.base;
	
	iter.uint32++; // skip var table size.
	module->start_seg = iter.uint8;
	module->vars.buckets = harbol_mempool_alloc(&module->heap, sizeof *module->vars.buckets * module->vars.hashlen);
	module->vars.array = harbol_mempool_alloc(&module->heap, sizeof *module->vars.array * module->vars.arrlen);
	for( uint32_t i=0; i<var_table_size; i++ ) {
		const uint8_t flag = *iter.uint8++;
		const uint64_t sizes = *iter.uint64++;
		const uint32_t cstrlen = sizes & 0xffFFffFF;
		const uint32_t datalen = sizes >> 32;
		const char *cstr = iter.string;
		const size_t hash = string_hash(cstr) % module->vars.hashlen;
		iter.uint8 += cstrlen;
		const struct TaghaItem varitem = { .bytes=datalen, .flags=flag, .item.stream=iter.uint8 };
		
		module->vars.array[i] = varitem;
		module->vars.buckets[hash].table = harbol_mempool_realloc(&module->heap, module->vars.buckets[hash].table, sizeof *module->vars.buckets[hash].table * (module->vars.buckets[hash].len + 1));
		if( module->vars.buckets[hash].table==NULL ) {
			fprintf(stderr, "Tagha Module File Error :: **** unable to allocate enough space for var table: %zu bytes ****\n", sizeof *module->vars.buckets[hash].table * (module->vars.buckets[hash].len + 1));
			return false;
		}
		module->vars.buckets[hash].table[module->vars.buckets[hash].len].key = cstr;
		module->vars.buckets[hash].table[module->vars.buckets[hash].len].keylen = cstrlen - 1;
		module->vars.buckets[hash].table[module->vars.buckets[hash].len].val = &module->vars.array[i];
		module->vars.buckets[hash].len++;
		
		iter.uint8 += datalen;
	}
	harbol_mempool_defrag(&module->heap);
	
	module->stack.size = stacksize;
	module->stack.start = harbol_mempool_alloc(&module->heap, sizeof *module->stack.start * stacksize);
	if( module->stack.start==NULL ) {
		fprintf(stderr, "Tagha Module File Error :: **** couldn't allocate module stack size of %zu bytes, heap remaining: %zu | ('%zu')****\n", stacksize * sizeof *module->stack.start, harbol_mempool_mem_remaining(&module->heap), module->heap.stack.base - module->heap.stack.mem);
		return false;
	} else {
		module->cpu.regfile.struc.stkptr.ptrself = module->cpu.regfile.struc.baseptr.ptrself = module->stack.start + stacksize;
		return true;
	}
}

TAGHA_EXPORT struct TaghaModule *tagha_module_new_from_file(const char filename[restrict static 1])
{
	struct TaghaModule *restrict module = calloc(1, sizeof *module);
	if( module==NULL ) {
		fprintf(stderr, "Tagha Module Error :: **** Unable to allocate module for file '%s'. ****\n", filename);
		return NULL;
	} else {
		*module = tagha_module_create_from_file(filename);
		// check if module is completely empty.
		if( !memcmp(module, &(struct TaghaModule)EMPTY_TAGHA_MODULE, sizeof *module) )
			free(module), module = NULL;
	}
	return module;
}

TAGHA_EXPORT struct TaghaModule *tagha_module_new_from_buffer(uint8_t buffer[restrict static 1])
{
	struct TaghaModule *restrict module = calloc(1, sizeof *module);
	if( module==NULL ) {
		fprintf(stderr, "Tagha Module Error :: **** Unable to allocate module. ****\n");
		return NULL;
	} else {
		*module = tagha_module_create_from_buffer(buffer);
		if( !memcmp(module, &(struct TaghaModule)EMPTY_TAGHA_MODULE, sizeof *module) )
			free(module), module = NULL;
	}
	return module;
}

TAGHA_EXPORT bool tagha_module_free(struct TaghaModule **const modref)
{
	if( *modref==NULL )
		return false;
	else {
		tagha_module_clear(*modref);
		free(*modref), *modref=NULL;
		return true;
	}
}


TAGHA_EXPORT struct TaghaModule tagha_module_create_from_file(const char filename[restrict static 1])
{
	struct TaghaModule module = EMPTY_TAGHA_MODULE;
	uint8_t *restrict bytecode = make_buffer_from_binary(filename);
	if( bytecode==NULL ) {
		fprintf(stderr, "Tagha Module Error :: **** failed to create file data buffer from '%s'. ****\n", filename);
		return module;
	} else if( *(uint16_t *)bytecode != TAGHA_MAGIC_VERIFIER ) {
		fprintf(stderr, "Tagha Module Error :: **** invalid tagha module: '%s' ****\n", filename);
		free(bytecode);
		return module;
	}
	
	const bool read_result = _read_module_data(&module, bytecode);
	if( !read_result ) {
		fprintf(stderr, "Tagha Module Error :: **** couldn't allocate tables for file: '%s' ****\n", filename);
		tagha_module_clear(&module);
		return (struct TaghaModule)EMPTY_TAGHA_MODULE;
	}
	else return module;
}

TAGHA_EXPORT struct TaghaModule tagha_module_create_from_buffer(uint8_t buffer[restrict static 1])
{
	struct TaghaModule module = EMPTY_TAGHA_MODULE;
	if( *(uint16_t *)buffer != TAGHA_MAGIC_VERIFIER ) {
		fprintf(stderr, "Tagha Module Error :: **** invalid module buffer '%p' ****\n", buffer);
		return module;
	}
	
	const bool read_result = _read_module_data(&module, buffer);
	if( !read_result ) {
		fprintf(stderr, "Tagha Module Error :: **** couldn't allocate tables from buffer ****\n");
		tagha_module_clear(&module);
		return (struct TaghaModule)EMPTY_TAGHA_MODULE;
	}
	else return module;
}

TAGHA_EXPORT bool tagha_module_clear(struct TaghaModule *const module)
{
	if( module->script != NULL )
		free(module->script);
	*module = (struct TaghaModule)EMPTY_TAGHA_MODULE;
	return true;
}


TAGHA_EXPORT void tagha_module_print_vm_state(const struct TaghaModule *const module)
{
	printf("=== Tagha VM State Info ===\n\nPrinting regs:\nregister alaf: '%" PRIu64 "'\nregister beth: '%" PRIu64 "'\nregister gamal: '%" PRIu64 "'\nregister dalath: '%" PRIu64 "'\nregister heh: '%" PRIu64 "'\nregister waw: '%" PRIu64 "'\nregister zain: '%" PRIu64 "'\nregister heth: '%" PRIu64 "'\nregister teth: '%" PRIu64 "'\nregister yodh: '%" PRIu64 "'\nregister kaf: '%" PRIu64 "'\nregister lamadh: '%" PRIu64 "'\nregister meem: '%" PRIu64 "'\nregister noon: '%" PRIu64 "'\nregister semkath: '%" PRIu64 "'\nregister eh: '%" PRIu64 "'\nregister peh: '%" PRIu64 "'\nregister sadhe: '%" PRIu64 "'\nregister qof: '%" PRIu64 "'\nregister reesh: '%" PRIu64 "'\nregister sheen: '%" PRIu64 "'\nregister taw: '%" PRIu64 "'\nregister stack pointer: '%p'\nregister base pointer: '%p'\nregister instruction pointer: '%p'\n\nPrinting Condition Flag: %u\n=== End Tagha VM State Info ===\n",
	module->cpu.regfile.struc.alaf.uint64,
	module->cpu.regfile.struc.beth.uint64,
	module->cpu.regfile.struc.gamal.uint64,
	module->cpu.regfile.struc.dalath.uint64,
	module->cpu.regfile.struc.heh.uint64,
	module->cpu.regfile.struc.waw.uint64,
	module->cpu.regfile.struc.zain.uint64,
	module->cpu.regfile.struc.heth.uint64,
	module->cpu.regfile.struc.teth.uint64,
	module->cpu.regfile.struc.yodh.uint64,
	module->cpu.regfile.struc.kaf.uint64,
	module->cpu.regfile.struc.lamadh.uint64,
	module->cpu.regfile.struc.meem.uint64,
	module->cpu.regfile.struc.noon.uint64,
	module->cpu.regfile.struc.semkath.uint64,
	module->cpu.regfile.struc._eh.uint64,
	module->cpu.regfile.struc.peh.uint64,
	module->cpu.regfile.struc.sadhe.uint64,
	module->cpu.regfile.struc.qof.uint64,
	module->cpu.regfile.struc.reesh.uint64,
	module->cpu.regfile.struc.sheen.uint64,
	module->cpu.regfile.struc.taw.uint64,
	module->cpu.regfile.struc.stkptr.ptrvoid,
	module->cpu.regfile.struc.baseptr.ptrvoid,
	module->cpu.regfile.struc.instr.ptrvoid,
	module->cpu.condflag);
}

TAGHA_EXPORT const char *tagha_module_get_error(const struct TaghaModule *const module)
{
	switch( module->errcode ) {
		case tagha_err_instr_oob:    return "Out of Bound Instruction";
		case tagha_err_none:         return "None";
		case tagha_err_bad_ptr:      return "Null/Invalid Pointer";
		case tagha_err_no_func:      return "Missing Function";
		case tagha_err_bad_script:   return "Null/Invalid Script";
		case tagha_err_stk_size:     return "Bad stack Size";
		case tagha_err_no_cfunc:     return "Missing Native";
		case tagha_err_stk_overflow: return "Stack Overflow";
		default:                     return "User-Defined/Unknown Error";
	}
}

TAGHA_EXPORT bool tagha_module_register_natives(struct TaghaModule *const module,
												const struct TaghaNative natives[const static 1])
{
	for( const struct TaghaNative *restrict n=natives; n->cfunc != NULL && n->name != NULL; n++ ) {
		struct TaghaItem *const func = _tagha_key_get_item(module->funcs, n->name);
		if( func==NULL || func->flags != TAGHA_FLAG_NATIVE )
			continue;
		else {
			func->item.cfunc = n->cfunc;
			func->flags = TAGHA_FLAG_NATIVE + TAGHA_FLAG_LINKED;
		}
	}
	return true;
}

TAGHA_EXPORT bool tagha_module_register_ptr(struct TaghaModule *const module,
											const char name[restrict static 1],
											void *const var)
{
	void **restrict global_ref = tagha_module_get_var(module, name);
	if( global_ref==NULL ) {
		return false;
	} else {
		*global_ref = var;
		return true;
	}
}

TAGHA_EXPORT void *tagha_module_get_var(struct TaghaModule *const restrict module, const char name[restrict static 1])
{
	const struct TaghaItem *const restrict var = _tagha_key_get_item(module->vars, name);
	return( var != NULL ) ? var->item.datum : NULL;
}

TAGHA_EXPORT uint8_t tagha_module_get_flags(const struct TaghaModule *module)
{
	return module->flags;
}

TAGHA_EXPORT int32_t tagha_module_call(struct TaghaModule *const restrict module,
										const char name[restrict static 1],
										const size_t args,
										const union TaghaVal params[restrict],
										union TaghaVal *const restrict retval)
{
	const struct TaghaItem *const restrict item = _tagha_key_get_item(module->funcs, name);
	if( item==NULL ) {
		module->errcode = tagha_err_no_func;
		return -1;
	}
	else return _tagha_module_start(module, item, args, params, retval);
}

TAGHA_EXPORT int32_t tagha_module_invoke(struct TaghaModule *const module,
											const int64_t func_index,
											const size_t args,
											const union TaghaVal params[restrict],
											union TaghaVal *const restrict retval)
{
	if( !func_index ) {
		module->errcode = tagha_err_no_func;
		return -1;
	} else {
		const size_t real_index = ((func_index>0) ? (func_index - 1) : (-1 - func_index));
		if( real_index >= module->funcs.arrlen ) {
			module->errcode = tagha_err_no_func;
			return -1;
		} else {
			const struct TaghaItem item = module->funcs.array[real_index];
			return _tagha_module_start(module, &item, args, params, retval);
		}
	}
}

TAGHA_EXPORT inline int32_t tagha_module_run(struct TaghaModule *const module, const size_t argc, const union TaghaVal argv[const])
{
	return tagha_module_call(module, "main", argc, argv, NULL);
}

static int32_t _tagha_module_start(struct TaghaModule *const module,
											const struct TaghaItem *const func,
											const size_t args,
											const union TaghaVal params[const restrict],
											union TaghaVal *const restrict retval)
{
	if( func->flags >= TAGHA_FLAG_NATIVE ) {
		/* make sure our native function was registered first or else we crash ;) */
		if( func->flags >= TAGHA_FLAG_LINKED ) {
			const union TaghaVal ret = (*func->item.cfunc)(module, args, params);
			if( retval != NULL )
				*retval = ret;
			return( module->errcode == tagha_err_none ) ? 0 : module->errcode;
		} else {
			module->errcode = tagha_err_no_cfunc;
			return -1;
		}
	} else {
		/* remember that arguments must be passed right to left. we have enough args to fit in registers. */
		const uint8_t reg_param_initial = TAGHA_FIRST_PARAM_REG;
		const uint8_t reg_params = TAGHA_REG_PARAMS_MAX;
		const size_t bytecount = sizeof(union TaghaVal) * args;
		
		/* save stack space by using the registers for passing arguments.
			the other registers can be used for different operations. */
		if( args <= reg_params ) {
			memcpy(&module->cpu.regfile.array[reg_param_initial], &params[0], bytecount);
		}
		/* if the function has more than a certain num of params, push from both registers and stack. */
		else {
			/* make sure we have the stack space to acommodate the amount of args. */
			if( module->cpu.regfile.struc.stkptr.ptrself - args < module->stack.start ) {
				module->errcode = tagha_err_stk_overflow;
				return -1;
			} else {
				memcpy(module->cpu.regfile.struc.stkptr.ptrself, &params[0], bytecount);
				module->cpu.regfile.struc.stkptr.ptrself -= args;
			}
		}
		/* prep the stack frame. */
		*(--module->cpu.regfile.struc.stkptr.ptrself) = (union TaghaVal){.ptrvoid=NULL}; /* push 0 return address. */
		*--module->cpu.regfile.struc.stkptr.ptrself = module->cpu.regfile.struc.baseptr; /* push rbp */
		module->cpu.regfile.struc.baseptr = module->cpu.regfile.struc.stkptr; /* mov rbp, rsp */
		module->cpu.regfile.struc.instr.ptrvoid = func->item.datum;
		
		if( module->cpu.regfile.struc.instr.ptrvoid != NULL ) {
			const int32_t result = _tagha_module_exec(module);
			module->cpu.regfile.struc.instr.ptrvoid = NULL;
			if( retval != NULL )
				*retval = module->cpu.regfile.struc.alaf;
			if( args > reg_params )
				module->cpu.regfile.struc.stkptr.ptrself += args;
			return result;
		} else {
			/* unwind stack */
			module->cpu.regfile.struc.stkptr = module->cpu.regfile.struc.baseptr;
			module->cpu.regfile.struc.stkptr.ptrself += 2;
			/* pop off our params. */
			if( args > reg_params )
				module->cpu.regfile.struc.stkptr.ptrself += args;
			module->errcode = tagha_err_no_func;
			return -1;
		}
	}
}

TAGHA_EXPORT void tagha_module_throw_error(struct TaghaModule *const module, const int32_t err)
{
	if( !err )
		return;
	else module->errcode = err;
}

TAGHA_EXPORT void tagha_module_jit_compile(struct TaghaModule *const restrict module, FnTaghaNative *fn_jit_compile(const uint8_t*, size_t, void *), void *const restrict userdata)
{
	for( uindex_t i=0; i<module->funcs.arrlen; i++ ) {
		// trying to JIT compile something that's already compiled? lol.
		if( module->funcs.array[i].flags & TAGHA_FLAG_NATIVE )
			continue;
		else {
			FnTaghaNative *const func = fn_jit_compile(module->funcs.array[i].item.stream, module->funcs.array[i].bytes, userdata);
			if( func != NULL ) {
				module->funcs.array[i].flags = TAGHA_FLAG_NATIVE + TAGHA_FLAG_LINKED;
				module->funcs.array[i].bytes = 8;
				module->funcs.array[i].item.cfunc = func;
			}
		}
	}
}


//#include <unistd.h>    /* sleep() func */

static int32_t _tagha_module_exec(struct TaghaModule *const vm)
{
	/* pc is restricted and must not access beyond the function table! */
	union TaghaPtr pc = {.ptruint8 = vm->cpu.regfile.struc.instr.ptruint8};
	const uint8_t
		*const mem_start = vm->start_seg,
		*const mem_end = vm->end_seg
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
		const uint8_t instr = *pc.ptruint8++; \
		\
		if( instr>nop ) { \
			vm->errcode = tagha_err_instr_oob; \
			return -1; \
		} \
		\
		/*usleep(100);*/ \
		printf("dispatching to '%s'\n", opcode2str[instr]); \
		/*tagha_module_print_vm_state(vm);*/ \
		goto *dispatch[instr]
	
#	define DISPATCH()    goto *dispatch[*pc.ptruint8++]
	
	/* nop being first will make sure our VM starts with a dispatch! */
	exec_nop: {
		DISPATCH();
	}
	/* push immediate value. */
	exec_pushi: { /* char: opcode | i64: imm */
		*--vm->cpu.regfile.struc.stkptr.ptrself = *pc.ptrval++;
		DISPATCH();
	}
	/* push a register's contents. */
	exec_push: { /* char: opcode | char: regid */
		*--vm->cpu.regfile.struc.stkptr.ptrself = vm->cpu.regfile.array[*pc.ptruint8++];
		DISPATCH();
	}
	/* pops a value from the stack into a register then reduces stack by 8 bytes. */
	exec_pop: { /* char: opcode | char: regid */
		vm->cpu.regfile.array[*pc.ptruint8++] = *vm->cpu.regfile.struc.stkptr.ptrself++;
		DISPATCH();
	}
	
	exec_loadglobal: { /* char: opcode | char: regid | u64: index */
		const uint8_t regid = *pc.ptruint8++;
		vm->cpu.regfile.array[regid].ptrvoid = vm->vars.array[ *pc.ptruint64++ ].item.datum;
		DISPATCH();
	}
	/* loads a function index which could be a native */
	exec_loadfunc: { /* char: opcode | char: regid | i64: index */
		const uint8_t regid = *pc.ptruint8++;
		vm->cpu.regfile.array[regid] = *pc.ptrval++;
		DISPATCH();
	}
	exec_loadaddr: { /* char: opcode | char: regid1 | char: regid2 | i32: offset */
		const uint16_t regids = *pc.ptruint16++;
		vm->cpu.regfile.array[regids & 0xff].ptruint8 = vm->cpu.regfile.array[regids >> 8].ptruint8 + *pc.ptrint32++;
		DISPATCH();
	}
	
	exec_movi: { /* char: opcode | char: regid | i64: imm */
		const uint8_t regid = *pc.ptruint8++;
		vm->cpu.regfile.array[regid] = *pc.ptrval++;
		DISPATCH();
	}
	exec_mov: { /* char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.ptruint16++;
		vm->cpu.regfile.array[regids & 0xff] = vm->cpu.regfile.array[regids >> 8];
		DISPATCH();
	}
	
	exec_ld1: { /* char: opcode | char: dest reg | char: src reg | i32: offset */
		const uint16_t regids = *pc.ptruint16++;
		const union TaghaPtr mem = {.ptruint8 = vm->cpu.regfile.array[regids >> 8].ptruint8 + *pc.ptrint32++};
		if( mem.ptruint8 < mem_start || mem.ptruint8 > mem_end ) {
			vm->errcode = tagha_err_bad_ptr;
			return -1;
		} else {
			vm->cpu.regfile.array[regids & 0xff].uint64 = (uint64_t) *mem.ptruint8;
			DISPATCH();
		}
	}
	exec_ld2: { /* char: opcode | char: dest reg | char: src reg | i32: offset */
		const uint16_t regids = *pc.ptruint16++;
		const union TaghaPtr mem = {.ptruint8 = vm->cpu.regfile.array[regids >> 8].ptruint8 + *pc.ptrint32++};
		if( mem.ptruint8 < mem_start || mem.ptruint8+1 > mem_end ) {
			vm->errcode = tagha_err_bad_ptr;
			return -1;
		} else {
			vm->cpu.regfile.array[regids & 0xff].uint64 = (uint64_t) *mem.ptruint16;
			DISPATCH();
		}
	}
	exec_ld4: { /* char: opcode | char: dest reg | char: src reg | i32: offset */
		const uint16_t regids = *pc.ptruint16++;
		const union TaghaPtr mem = {.ptruint8 = vm->cpu.regfile.array[regids >> 8].ptruint8 + *pc.ptrint32++};
		if( mem.ptruint8 < mem_start || mem.ptruint8+3 > mem_end ) {
			vm->errcode = tagha_err_bad_ptr;
			return -1;
		} else {
			vm->cpu.regfile.array[regids & 0xff].uint64 = (uint64_t) *mem.ptruint32;
			DISPATCH();
		}
	}
	exec_ld8: { /* char: opcode | char: dest reg | char: src reg | i32: offset */
		const uint16_t regids = *pc.ptruint16++;
		const union TaghaPtr mem = {.ptruint8 = vm->cpu.regfile.array[regids >> 8].ptruint8 + *pc.ptrint32++};
		if( mem.ptruint8 < mem_start || mem.ptruint8+7 > mem_end ) {
			vm->errcode = tagha_err_bad_ptr;
			return -1;
		} else {
			vm->cpu.regfile.array[regids & 0xff].uint64 = *mem.ptruint64;
			DISPATCH();
		}
	}
	
	exec_st1: { /* char: opcode | char: dest reg | char: src reg | i32: offset */
		const uint16_t regids = *pc.ptruint16++;
		const union TaghaPtr mem = {.ptruint8 = vm->cpu.regfile.array[regids & 0xff].ptruint8 + *pc.ptrint32++};
		if( mem.ptruint8 < mem_start || mem.ptruint8 > mem_end ) {
			vm->errcode = tagha_err_bad_ptr;
			return -1;
		} else {
			*mem.ptruint8 = vm->cpu.regfile.array[regids >> 8].uint8;
			DISPATCH();
		}
	}
	exec_st2: { /* char: opcode | char: dest reg | char: src reg | i32: offset */
		const uint16_t regids = *pc.ptruint16++;
		const union TaghaPtr mem = {.ptruint8 = vm->cpu.regfile.array[regids & 0xff].ptruint8 + *pc.ptrint32++};
		if( mem.ptruint8 < mem_start || mem.ptruint8+1 > mem_end ) {
			vm->errcode = tagha_err_bad_ptr;
			return -1;
		} else {
			*mem.ptruint16 = vm->cpu.regfile.array[regids >> 8].uint16;
			DISPATCH();
		}
	}
	exec_st4: { /* char: opcode | char: dest reg | char: src reg | i32: offset */
		const uint16_t regids = *pc.ptruint16++;
		const union TaghaPtr mem = {.ptruint8 = vm->cpu.regfile.array[regids & 0xff].ptruint8 + *pc.ptrint32++};
		if( mem.ptruint8 < mem_start || mem.ptruint8+3 > mem_end ) {
			vm->errcode = tagha_err_bad_ptr;
			return -1;
		} else {
			*mem.ptruint32 = vm->cpu.regfile.array[regids >> 8].uint32;
			DISPATCH();
		}
	}
	exec_st8: { /* char: opcode | char: dest reg | char: src reg | i32: offset */
		const uint16_t regids = *pc.ptruint16++;
		const union TaghaPtr mem = {.ptruint8 = vm->cpu.regfile.array[regids & 0xff].ptruint8 + *pc.ptrint32++};
		if( mem.ptruint8 < mem_start || mem.ptruint8+7 > mem_end ) {
			vm->errcode = tagha_err_bad_ptr;
			return -1;
		} else {
			*mem.ptruint64 = vm->cpu.regfile.array[regids >> 8].uint64;
			DISPATCH();
		}
	}
	
	exec_add: { /* char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.ptruint16++;
		vm->cpu.regfile.array[regids & 0xff].int64 += vm->cpu.regfile.array[regids >> 8].int64;
		DISPATCH();
	}
	exec_sub: { /* char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.ptruint16++;
		vm->cpu.regfile.array[regids & 0xff].int64 -= vm->cpu.regfile.array[regids >> 8].int64;
		DISPATCH();
	}
	exec_mul: { /* char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.ptruint16++;
		vm->cpu.regfile.array[regids & 0xff].int64 *= vm->cpu.regfile.array[regids >> 8].int64;
		DISPATCH();
	}
	exec_divi: { /* char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.ptruint16++;
		vm->cpu.regfile.array[regids & 0xff].uint64 /= vm->cpu.regfile.array[regids >> 8].uint64;
		DISPATCH();
	}
	exec_mod: { /* char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.ptruint16++;
		vm->cpu.regfile.array[regids & 0xff].uint64 %= vm->cpu.regfile.array[regids >> 8].uint64;
		DISPATCH();
	}
	exec_bit_and: { /* char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.ptruint16++;
		vm->cpu.regfile.array[regids & 0xff].uint64 &= vm->cpu.regfile.array[regids >> 8].uint64;
		DISPATCH();
	}
	exec_bit_or: { /* char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.ptruint16++;
		vm->cpu.regfile.array[regids & 0xff].uint64 |= vm->cpu.regfile.array[regids >> 8].uint64;
		DISPATCH();
	}
	exec_bit_xor: { /* char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.ptruint16++;
		vm->cpu.regfile.array[regids & 0xff].uint64 ^= vm->cpu.regfile.array[regids >> 8].uint64;
		DISPATCH();
	}
	exec_shl: { /* char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.ptruint16++;
		vm->cpu.regfile.array[regids & 0xff].uint64 <<= vm->cpu.regfile.array[regids >> 8].uint64;
		DISPATCH();
	}
	exec_shr: { /* char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.ptruint16++;
		vm->cpu.regfile.array[regids & 0xff].uint64 >>= vm->cpu.regfile.array[regids >> 8].uint64;
		DISPATCH();
	}
	exec_bit_not: { /* char: opcode | char: regid */
		const uint8_t regid = *pc.ptruint8++;
		vm->cpu.regfile.array[regid].uint64 = ~vm->cpu.regfile.array[regid].uint64;
		DISPATCH();
	}
	exec_inc: { /* char: opcode | char: regid */
		const uint8_t regid = *pc.ptruint8++;
		++vm->cpu.regfile.array[regid].uint64;
		DISPATCH();
	}
	exec_dec: { /* char: opcode | char: regid */
		const uint8_t regid = *pc.ptruint8++;
		--vm->cpu.regfile.array[regid].uint64;
		DISPATCH();
	}
	exec_neg: { /* char: opcode | char: regid */
		const uint8_t regid = *pc.ptruint8++;
		vm->cpu.regfile.array[regid].int64 = -vm->cpu.regfile.array[regid].int64;
		DISPATCH();
	}
	
	exec_ilt: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.ptruint16++;
		vm->cpu.condflag = vm->cpu.regfile.array[regids & 0xff].int64 < vm->cpu.regfile.array[regids >> 8].int64;
		DISPATCH();
	}
	exec_ile: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.ptruint16++;
		vm->cpu.condflag = vm->cpu.regfile.array[regids & 0xff].int64 <= vm->cpu.regfile.array[regids >> 8].int64;
		DISPATCH();
	}
	
	exec_igt: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.ptruint16++;
		vm->cpu.condflag = vm->cpu.regfile.array[regids & 0xff].int64 > vm->cpu.regfile.array[regids >> 8].int64;
		DISPATCH();
	}
	exec_ige: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.ptruint16++;
		vm->cpu.condflag = vm->cpu.regfile.array[regids & 0xff].int64 >= vm->cpu.regfile.array[regids >> 8].int64;
		DISPATCH();
	}
	
	exec_ult: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.ptruint16++;
		vm->cpu.condflag = vm->cpu.regfile.array[regids & 0xff].uint64 < vm->cpu.regfile.array[regids >> 8].uint64;
		DISPATCH();
	}
	exec_ule: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.ptruint16++;
		vm->cpu.condflag = vm->cpu.regfile.array[regids & 0xff].uint64 <= vm->cpu.regfile.array[regids >> 8].uint64;
		DISPATCH();
	}
	
	exec_ugt: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.ptruint16++;
		vm->cpu.condflag = vm->cpu.regfile.array[regids & 0xff].uint64 > vm->cpu.regfile.array[regids >> 8].uint64;
		DISPATCH();
	}
	exec_uge: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.ptruint16++;
		vm->cpu.condflag = vm->cpu.regfile.array[regids & 0xff].uint64 >= vm->cpu.regfile.array[regids >> 8].uint64;
		DISPATCH();
	}
	
	exec_cmp: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.ptruint16++;
		vm->cpu.condflag = vm->cpu.regfile.array[regids & 0xff].uint64 == vm->cpu.regfile.array[regids >> 8].uint64;
		DISPATCH();
	}
	exec_neq: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.ptruint16++;
		vm->cpu.condflag = vm->cpu.regfile.array[regids & 0xff].uint64 != vm->cpu.regfile.array[regids >> 8].uint64;
		DISPATCH();
	}
	exec_jmp: { /* char: opcode | i64: offset */
		const int64_t offset = *pc.ptrint64++;
		pc.ptruint8 += offset;
		DISPATCH();
	}
	exec_jz: { /* char: opcode | i64: offset */
		const int64_t offset = *pc.ptrint64++;
		if( !vm->cpu.condflag ) {
			pc.ptruint8 += offset;
			DISPATCH();
		} else {
			DISPATCH();
		}
	}
	exec_jnz: { /* char: opcode | i64: offset */
		const int64_t offset = *pc.ptrint64++;
		if( vm->cpu.condflag ) {
			pc.ptruint8 += offset;
			DISPATCH();
		} else {
			DISPATCH();
		}
	}
	exec_call: { /* char: opcode | i64: offset */
		const int64_t index = *pc.ptrint64++;
		if( !index ) {
			vm->errcode = tagha_err_no_func;
			goto *dispatch[halt];
		} else {
			const struct TaghaItem func = vm->funcs.array[(index>0) ? (index - 1) : (-1 - index)];
			if( func.flags >= TAGHA_FLAG_NATIVE ) {
				if( func.flags != TAGHA_FLAG_NATIVE+TAGHA_FLAG_LINKED ) {
					vm->errcode = tagha_err_no_cfunc;
					goto *dispatch[halt];
				} else {
					const uint8_t reg_param_initial = TAGHA_FIRST_PARAM_REG;
					const uint8_t reg_params = TAGHA_REG_PARAMS_MAX;
					const size_t args = vm->cpu.regfile.struc.alaf.size;
					
					/* save stack space by using the registers for passing arguments.
						the other registers can then be used for other data operations. */
					if( args <= reg_params ) {
						vm->cpu.regfile.struc.alaf = (*func.item.cfunc)(vm, args, &vm->cpu.regfile.array[reg_param_initial]);
					} else {
						/* if the native call has more than a certain num of params, get all params from stack. */
						vm->cpu.regfile.struc.alaf = (*func.item.cfunc)(vm, args, vm->cpu.regfile.struc.stkptr.ptrself);
						vm->cpu.regfile.struc.stkptr.ptrself += args;
					}
					
					if( vm->errcode != tagha_err_none ) {
						goto *dispatch[halt];
					} else {
						DISPATCH();
					}
				}
			} else {
				(--vm->cpu.regfile.struc.stkptr.ptrself)->ptrvoid = pc.ptrvoid; /* push rip */
				*--vm->cpu.regfile.struc.stkptr.ptrself = vm->cpu.regfile.struc.baseptr; /* push rbp */
				vm->cpu.regfile.struc.baseptr = vm->cpu.regfile.struc.stkptr; /* mov rbp, rsp */
				pc.ptruint8 = func.item.stream;
				DISPATCH();
			}
		}
	}
	exec_callr: { /* char: opcode | char: regid */
		const uint8_t regid = *pc.ptruint8++;
		const int64_t index = vm->cpu.regfile.array[regid].int64;
		if( !index ) {
			vm->errcode = tagha_err_no_func;
			goto *dispatch[halt];
		} else {
			const struct TaghaItem func = vm->funcs.array[(index>0) ? (index - 1) : (-1 - index)];
			if( func.flags >= TAGHA_FLAG_NATIVE ) {
				if( func.flags != TAGHA_FLAG_NATIVE+TAGHA_FLAG_LINKED ) {
					vm->errcode = tagha_err_no_cfunc;
					goto *dispatch[halt];
				} else {
					const uint8_t reg_param_initial = TAGHA_FIRST_PARAM_REG;
					const uint8_t reg_params = TAGHA_REG_PARAMS_MAX;
					const size_t args = vm->cpu.regfile.struc.alaf.size;
					
					/* save stack space by using the registers for passing arguments.
						the other registers can then be used for other data operations. */
					if( args <= reg_params ) {
						vm->cpu.regfile.struc.alaf = (*func.item.cfunc)(vm, args, &vm->cpu.regfile.array[reg_param_initial]);
					} else {
						/* if the native call has more than a certain num of params, get all params from stack. */
						vm->cpu.regfile.struc.alaf = (*func.item.cfunc)(vm, args, vm->cpu.regfile.struc.stkptr.ptrself);
						vm->cpu.regfile.struc.stkptr.ptrself += args;
					}
					
					if( vm->errcode != tagha_err_none ) {
						goto *dispatch[halt];
					} else {
						DISPATCH();
					}
				}
			} else {
				(--vm->cpu.regfile.struc.stkptr.ptrself)->ptrvoid = pc.ptrvoid; /* push rip */
				*--vm->cpu.regfile.struc.stkptr.ptrself = vm->cpu.regfile.struc.baseptr; /* push rbp */
				vm->cpu.regfile.struc.baseptr = vm->cpu.regfile.struc.stkptr; /* mov rbp, rsp */
				pc.ptruint8 = func.item.stream;
				DISPATCH();
			}
		}
	}
	exec_ret: { /* char: opcode */
		vm->cpu.regfile.struc.stkptr = vm->cpu.regfile.struc.baseptr; /* mov rsp, rbp */
		vm->cpu.regfile.struc.baseptr = *vm->cpu.regfile.struc.stkptr.ptrself++; /* pop rbp */
		pc.ptrvoid = (*vm->cpu.regfile.struc.stkptr.ptrself++).ptrvoid; /* pop rip */
		if( pc.ptrvoid==NULL ) {
			return vm->cpu.regfile.struc.alaf.int32;
		} else {
			DISPATCH();
		}
	}
	
#ifdef TAGHA_USE_FLOATS
	/* treated as nop if float32_t is defined but not the other. */
	exec_flt2dbl: { /* char: opcode | char: reg id */
		const uint8_t regid = *pc.ptruint8++;
#	if defined(TAGHA_FLOAT32_DEFINED) && defined(TAGHA_FLOAT64_DEFINED)
		const float32_t f = vm->cpu.regfile.array[regid].float32;
		vm->cpu.regfile.array[regid].float64 = (float64_t)f;
#	endif
		DISPATCH();
	}
	exec_dbl2flt: { /* char: opcode | char: reg id */
		const uint8_t regid = *pc.ptruint8++;
#	if defined(TAGHA_FLOAT32_DEFINED) && defined(TAGHA_FLOAT64_DEFINED)
		const float64_t d = vm->cpu.regfile.array[regid].float64;
		vm->cpu.regfile.array[regid].int64 = 0;
		vm->cpu.regfile.array[regid].float32 = (float32_t)d;
#	endif
		DISPATCH();
	}
	exec_int2dbl: { /* char: opcode | char: reg id */
		const uint8_t regid = *pc.ptruint8++;
#	ifdef TAGHA_FLOAT64_DEFINED
		const int64_t i = vm->cpu.regfile.array[regid].int64;
		vm->cpu.regfile.array[regid].float64 = (float64_t)i;
#	else
		(void)regid;
#	endif
		DISPATCH();
	}
	exec_int2flt: { /* char: opcode | char: reg id */
		const uint8_t regid = *pc.ptruint8++;
#	ifdef TAGHA_FLOAT32_DEFINED
		const int64_t i = vm->cpu.regfile.array[regid].int64;
		vm->cpu.regfile.array[regid].int64 = 0;
		vm->cpu.regfile.array[regid].float32 = (float32_t)i;
#	else
		(void)regid;
#	endif
		DISPATCH();
	}
	
	exec_addf: { /* char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.ptruint16++;
#	if defined(TAGHA_FLOAT64_DEFINED) /* if float64_t's are defined, regardless whether float32_t is or not */
		vm->cpu.regfile.array[regids & 0xff].float64 += vm->cpu.regfile.array[regids >> 8].float64;
#	elif defined(TAGHA_FLOAT32_DEFINED)
		vm->cpu.regfile.array[regids & 0xff].float32 += vm->cpu.regfile.array[regids >> 8].float32;
#	else
		(void)regids;
#	endif
		DISPATCH();
	}
	exec_subf: { /* char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.ptruint16++;
#	if defined(TAGHA_FLOAT64_DEFINED)
		vm->cpu.regfile.array[regids & 0xff].float64 -= vm->cpu.regfile.array[regids >> 8].float64;
#	elif defined(TAGHA_FLOAT32_DEFINED)
		vm->cpu.regfile.array[regids & 0xff].float32 -= vm->cpu.regfile.array[regids >> 8].float32;
#	else
		(void)regids;
#	endif
		DISPATCH();
	}
	exec_mulf: { /* char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.ptruint16++;
#	if defined(TAGHA_FLOAT64_DEFINED)
		vm->cpu.regfile.array[regids & 0xff].float64 *= vm->cpu.regfile.array[regids >> 8].float64;
#	elif defined(TAGHA_FLOAT32_DEFINED)
		vm->cpu.regfile.array[regids & 0xff].float32 *= vm->cpu.regfile.array[regids >> 8].float32;
#	else
		(void)regids;
#	endif
		DISPATCH();
	}
	exec_divf: { /* char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.ptruint16++;
#	if defined(TAGHA_FLOAT64_DEFINED)
		vm->cpu.regfile.array[regids & 0xff].float64 /= vm->cpu.regfile.array[regids >> 8].float64;
#	elif defined(TAGHA_FLOAT32_DEFINED)
		vm->cpu.regfile.array[regids & 0xff].float32 /= vm->cpu.regfile.array[regids >> 8].float32;
#	else
		(void)regids;
#	endif
		DISPATCH();
	}
	
	exec_ltf: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.ptruint16++;
#	if defined(TAGHA_FLOAT64_DEFINED)
		vm->cpu.condflag = vm->cpu.regfile.array[regids & 0xff].float64 < vm->cpu.regfile.array[regids >> 8].float64;
#	elif defined(TAGHA_FLOAT32_DEFINED)
		vm->cpu.condflag = vm->cpu.regfile.array[regids & 0xff].float32 < vm->cpu.regfile.array[regids >> 8].float32;
#	else
		(void)regids;
#	endif
		DISPATCH();
	}
	exec_lef: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.ptruint16++;
#	if defined(TAGHA_FLOAT64_DEFINED)
		vm->cpu.condflag = vm->cpu.regfile.array[regids & 0xff].float64 <= vm->cpu.regfile.array[regids >> 8].float64;
#	elif defined(TAGHA_FLOAT32_DEFINED)
		vm->cpu.condflag = vm->cpu.regfile.array[regids & 0xff].float32 <= vm->cpu.regfile.array[regids >> 8].float32;
#	else
		(void)regids;
#	endif
		DISPATCH();
	}
	
	exec_gtf: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.ptruint16++;
#	if defined(TAGHA_FLOAT64_DEFINED)
		vm->cpu.condflag = vm->cpu.regfile.array[regids & 0xff].float64 > vm->cpu.regfile.array[regids >> 8].float64;
#	elif defined(TAGHA_FLOAT32_DEFINED)
		vm->cpu.condflag = vm->cpu.regfile.array[regids & 0xff].float32 > vm->cpu.regfile.array[regids >> 8].float32;
#	else
		(void)regids;
#	endif
		DISPATCH();
	}
	exec_gef: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.ptruint16++;
#	if defined(TAGHA_FLOAT64_DEFINED)
		vm->cpu.condflag = vm->cpu.regfile.array[regids & 0xff].float64 >= vm->cpu.regfile.array[regids >> 8].float64;
#	elif defined(TAGHA_FLOAT32_DEFINED)
		vm->cpu.condflag = vm->cpu.regfile.array[regids & 0xff].float32 >= vm->cpu.regfile.array[regids >> 8].float32;
#	else
		(void)regids;
#	endif
		DISPATCH();
	}
	
	exec_cmpf: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.ptruint16++;
#	if defined(TAGHA_FLOAT64_DEFINED)
		vm->cpu.condflag = vm->cpu.regfile.array[regids & 0xff].float64 == vm->cpu.regfile.array[regids >> 8].float64;
#	elif defined(TAGHA_FLOAT32_DEFINED)
		vm->cpu.condflag = vm->cpu.regfile.array[regids & 0xff].float32 == vm->cpu.regfile.array[regids >> 8].float32;
#	else
		(void)regids;
#	endif
		DISPATCH();
	}
	exec_neqf: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.ptruint16++;
#	if defined(TAGHA_FLOAT64_DEFINED)
		vm->cpu.condflag = vm->cpu.regfile.array[regids & 0xff].float64 != vm->cpu.regfile.array[regids >> 8].float64;
#	elif defined(TAGHA_FLOAT32_DEFINED)
		vm->cpu.condflag = vm->cpu.regfile.array[regids & 0xff].float32 != vm->cpu.regfile.array[regids >> 8].float32;
#	else
		(void)regids;
#	endif
		DISPATCH();
	}
	exec_incf: { /* char: opcode | char: regid */
		const uint8_t regid = *pc.ptruint8++;
#	if defined(TAGHA_FLOAT64_DEFINED)
		++vm->cpu.regfile.array[regid].float64;
#	elif defined(TAGHA_FLOAT32_DEFINED)
		++vm->cpu.regfile.array[regid].float32;
#	else
		(void)regid;
#	endif
		DISPATCH();
	}
	exec_decf: { /* char: opcode | char: regid */
		const uint8_t regid = *pc.ptruint8++;
#	if defined(TAGHA_FLOAT64_DEFINED)
		--vm->cpu.regfile.array[regid].float64;
#	elif defined(TAGHA_FLOAT32_DEFINED)
		--vm->cpu.regfile.array[regid].float32;
#	else
		(void)regid;
#	endif
		DISPATCH();
	}
	exec_negf: { /* char: opcode | char: regid */
		const uint8_t regid = *pc.ptruint8++;
#	if defined(TAGHA_FLOAT64_DEFINED)
		vm->cpu.regfile.array[regid].float64 = -vm->cpu.regfile.array[regid].float64;
#	elif defined(TAGHA_FLOAT32_DEFINED)
		vm->cpu.regfile.array[regid].float32 = -vm->cpu.regfile.array[regid].float32;
#	else
		(void)regid;
#	endif
		DISPATCH();
	}
#endif
	exec_halt:
		return vm->cpu.regfile.struc.alaf.int32;
}
