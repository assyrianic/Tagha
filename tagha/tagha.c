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
	const struct TaghaHeader *const hdr = ( const struct TaghaHeader* )filedata;
	
	size_t largest_funcs_hash=0, largest_vars_hash=0;
	module->flags = hdr->flags;
	
	union HarbolBinIter iter = { .uint8 = filedata + sizeof *hdr };
	
	/// iterate function table and get bytecode sizes.
	const uint32_t func_table_size = *iter.uint32++;
	module->funcs.arrlen = func_table_size;
	for( uint32_t i=0; i<func_table_size; i++ ) {
		const struct TaghaItemEntry *const entry = iter.ptr;
		iter.uint8 += sizeof *entry;
		const char *cstr = iter.string;
		const size_t hash = string_hash(cstr) % module->funcs.arrlen;
		if( hash > largest_funcs_hash )
			largest_funcs_hash = hash;
		
		module->funcs.hashlen = largest_funcs_hash + 1;
		iter.uint8 += ( !entry->flags ) ? (entry->name_len + entry->data_len) : entry->name_len;
	}
	
	/// iterate global var table
	const uint32_t var_table_size = *iter.uint32++;
	module->vars.arrlen = var_table_size;
	for( uint32_t i=0; i<var_table_size; i++ ) {
		const struct TaghaItemEntry *const entry = iter.ptr;
		iter.uint8 += sizeof *entry;
		const char *cstr = iter.string;
		const size_t hash = string_hash(cstr) % module->vars.arrlen;
		if( hash > largest_vars_hash )
			largest_vars_hash = hash;
		
		module->vars.hashlen = largest_vars_hash + 1;
		iter.uint8 += (entry->name_len + entry->data_len);
	}
	
	module->heap = harbol_mempool_from_buffer(iter.uint8, hdr->memsize);
	module->heap.freelist.auto_defrag = true;
	module->heap.freelist.max_nodes = 0;
	const size_t given_heapsize = harbol_mempool_mem_remaining(&module->heap);
	if( !given_heapsize || given_heapsize != hdr->memsize ) {
		fprintf(stderr, "Tagha Module File Error :: **** given heapsize (%zu) is not same as required memory size! (%u). ****\n", given_heapsize, hdr->memsize);
		return false;
	}
	
	iter.uint8 = filedata + sizeof *hdr + sizeof(uint32_t);
	module->funcs.buckets = harbol_mempool_alloc(&module->heap, sizeof *module->funcs.buckets * module->funcs.hashlen);
	module->funcs.array = harbol_mempool_alloc(&module->heap, sizeof *module->funcs.array * module->funcs.arrlen);
	for( uint32_t i=0; i<func_table_size; i++ ) {
		const struct TaghaItemEntry *const entry = iter.ptr;
		iter.uint8 += sizeof *entry;
		const uint32_t flag = entry->flags;
		const char *cstr = iter.string;
		const size_t hash = string_hash(cstr) % module->funcs.hashlen;
		iter.uint8 += entry->name_len;
		const struct TaghaItem funcitem = { .bytes=entry->data_len, .flags=flag, .item=(!flag) ? iter.uint8 : NULL };
		
		module->funcs.array[i] = funcitem;
		module->funcs.buckets[hash].table = harbol_mempool_realloc(&module->heap, module->funcs.buckets[hash].table, sizeof *module->funcs.buckets[hash].table * (module->funcs.buckets[hash].len + 1));
		if( module->funcs.buckets[hash].table==NULL ) {
			fprintf(stderr, "Tagha Module File Error :: **** unable to allocate enough space for function table: %zu bytes ****\n", sizeof *module->vars.buckets[hash].table * (module->vars.buckets[hash].len + 1));
			return false;
		}
		module->funcs.buckets[hash].table[module->funcs.buckets[hash].len].key = cstr;
		module->funcs.buckets[hash].table[module->funcs.buckets[hash].len].keylen = entry->name_len - 1;
		module->funcs.buckets[hash].table[module->funcs.buckets[hash].len].val = &module->funcs.array[i];
		module->funcs.buckets[hash].len++;
		
		if( !flag )
			iter.uint8 += entry->data_len;
	}
	module->end_seg = ( uintptr_t )module->heap.stack.base;
	
	iter.uint32++; /// skip var table size.
	module->start_seg = ( uintptr_t )iter.uint8;
	module->vars.buckets = harbol_mempool_alloc(&module->heap, sizeof *module->vars.buckets * module->vars.hashlen);
	module->vars.array = harbol_mempool_alloc(&module->heap, sizeof *module->vars.array * module->vars.arrlen);
	for( uint32_t i=0; i<var_table_size; i++ ) {
		const struct TaghaItemEntry *const entry = iter.ptr;
		iter.uint8 += sizeof *entry;
		const uint32_t flag = entry->flags;
		const char *cstr = iter.string;
		const size_t hash = string_hash(cstr) % module->vars.hashlen;
		iter.uint8 += entry->name_len;
		const struct TaghaItem varitem = { .bytes=entry->data_len, .flags=flag, .item=iter.uint8 };
		
		module->vars.array[i] = varitem;
		module->vars.buckets[hash].table = harbol_mempool_realloc(&module->heap, module->vars.buckets[hash].table, sizeof *module->vars.buckets[hash].table * (module->vars.buckets[hash].len + 1));
		if( module->vars.buckets[hash].table==NULL ) {
			fprintf(stderr, "Tagha Module File Error :: **** unable to allocate enough space for var table: %zu bytes ****\n", sizeof *module->vars.buckets[hash].table * (module->vars.buckets[hash].len + 1));
			return false;
		}
		module->vars.buckets[hash].table[module->vars.buckets[hash].len].key = cstr;
		module->vars.buckets[hash].table[module->vars.buckets[hash].len].keylen = entry->name_len - 1;
		module->vars.buckets[hash].table[module->vars.buckets[hash].len].val = &module->vars.array[i];
		module->vars.buckets[hash].len++;
		
		iter.uint8 += entry->data_len;
	}
	harbol_mempool_defrag(&module->heap);
	
	module->stack.size = hdr->stacksize;
	module->stack.start = harbol_mempool_alloc(&module->heap, sizeof *module->stack.start * hdr->stacksize);
	if( module->stack.start==NULL ) {
		fprintf(stderr, "Tagha Module File Error :: **** couldn't allocate module stack size of %zu bytes, heap remaining: %zu | ('%zu')****\n", hdr->stacksize * sizeof *module->stack.start, harbol_mempool_mem_remaining(&module->heap), module->heap.stack.base - module->heap.stack.mem);
		return false;
	} else {
		module->regs[sp].uintptr = module->regs[bp].uintptr = ( uintptr_t )(module->stack.start + hdr->stacksize);
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
		/// check if module is completely empty.
		if( !memcmp(module, &(struct TaghaModule){0}, sizeof *module) )
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
		if( !memcmp(module, &(struct TaghaModule){0}, sizeof *module) )
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
	struct TaghaModule module = {0};
	uint8_t *restrict bytecode = make_buffer_from_binary(filename);
	if( bytecode==NULL ) {
		fprintf(stderr, "Tagha Module Error :: **** failed to create file data buffer from '%s'. ****\n", filename);
		return module;
	} else if( *(uint32_t *)bytecode != TAGHA_MAGIC_VERIFIER ) {
		fprintf(stderr, "Tagha Module Error :: **** invalid tagha module: '%s' ****\n", filename);
		free(bytecode);
		return module;
	}
	
	const bool read_result = _read_module_data(&module, bytecode);
	if( !read_result ) {
		fprintf(stderr, "Tagha Module Error :: **** couldn't allocate tables for file: '%s' ****\n", filename);
		tagha_module_clear(&module);
		return (struct TaghaModule){0};
	}
	else return module;
}

TAGHA_EXPORT struct TaghaModule tagha_module_create_from_buffer(uint8_t buffer[restrict static 1])
{
	struct TaghaModule module = {0};
	if( *(uint32_t *)buffer != TAGHA_MAGIC_VERIFIER ) {
		fprintf(stderr, "Tagha Module Error :: **** invalid module buffer '%p' ****\n", buffer);
		return module;
	}
	
	const bool read_result = _read_module_data(&module, buffer);
	if( !read_result ) {
		fprintf(stderr, "Tagha Module Error :: **** couldn't allocate tables from buffer ****\n");
		tagha_module_clear(&module);
		return (struct TaghaModule){0};
	}
	else return module;
}

TAGHA_EXPORT bool tagha_module_clear(struct TaghaModule *const module)
{
	if( module->script != NULL )
		free(module->script);
	*module = (struct TaghaModule){0};
	return true;
}


TAGHA_EXPORT void tagha_module_print_vm_state(const struct TaghaModule *const module, const bool hex)
{
	printf("=== Tagha VM State Info ===\n\nPrinting Registers:\n");
	for( size_t i=0; i<MaxRegisters; i++ ) {
		switch( i ) {
			case sp: printf("register stack ptr: '0x%" PRIxPTR "'\n", module->regs[i].uintptr); break;
			case bp: printf("register base  ptr: '0x%" PRIxPTR "'\n", module->regs[i].uintptr); break;
			default:
				if( hex )
					printf("register %.3zu:       '%" PRIx64 "'\n", i, module->regs[i].uint64);
				else printf("register %.3zu:       '%" PRIu64 "'\n", i, module->regs[i].uint64);
				break;
		}
	}
	printf("register instr ptr: '%p'\n\nPrinting Condition Flag: %u\n=== End Tagha VM State Info ===\n", module->ip, module->condflag);
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
			func->item = n->cfunc;
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
	return( var != NULL ) ? var->item : NULL;
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
	if( args >= TAGHA_REG_PARAMS_MAX ) {
		module->errcode = tagha_err_stk_overflow;
		return -1;
	} else if( func->flags >= TAGHA_FLAG_NATIVE ) {
		/// make sure our native function was registered first or else we crash ;)
		if( func->flags >= TAGHA_FLAG_LINKED ) {
			TaghaCFunc *const cfunc = func->item;
			memcpy(&module->regs[TAGHA_FIRST_PARAM_REG], &params[0], sizeof(union TaghaVal) * (args));
			const union TaghaVal ret = (*cfunc)(module, args, &module->regs[TAGHA_FIRST_PARAM_REG]);
			if( retval != NULL )
				*retval = ret;
			return( module->errcode == tagha_err_none ) ? 0 : module->errcode;
		} else {
			module->errcode = tagha_err_no_cfunc;
			return -1;
		}
	} else {
		module->ip = func->item;
		if( module->ip==NULL ) {
			module->errcode = tagha_err_no_func;
			return -1;
		} else {
			/// make sure we have the register space to accommodate the amount of args.
			memcpy(&module->regs[TAGHA_FIRST_PARAM_REG], &params[0], sizeof(union TaghaVal) * (args));
			
			/// prep the stack frame.
			module->regs[sp].uintptr -= 2 * sizeof(union TaghaVal);
			union TaghaVal *const restrict base = ( union TaghaVal* )module->regs[sp].uintptr;
			base[1].uintptr = ( uintptr_t )NULL;           /** push NULL */
			base[0].uintptr = module->regs[bp].uintptr;    /** push rbp */
			module->regs[bp] = module->regs[sp];           /** mov rbp, rsp */
			
			const int32_t result = _tagha_module_exec(module);
			if( retval != NULL )
				*retval = module->regs[alaf];
			return result;
		}
	}
}

TAGHA_EXPORT void tagha_module_throw_error(struct TaghaModule *const module, const int32_t err)
{
	if( !err )
		return;
	else module->errcode = err;
}

TAGHA_EXPORT void tagha_module_jit_compile(struct TaghaModule *const restrict module, TaghaCFunc *fn_jit_compile(const uint8_t*, size_t, void *), void *const restrict userdata)
{
	for( uindex_t i=0; i<module->funcs.arrlen; i++ ) {
		/// trying to JIT compile something that's already compiled? lol.
		if( module->funcs.array[i].flags & TAGHA_FLAG_NATIVE )
			continue;
		else {
			TaghaCFunc *const func = fn_jit_compile(module->funcs.array[i].item, module->funcs.array[i].bytes, userdata);
			if( func != NULL ) {
				module->funcs.array[i].flags = TAGHA_FLAG_NATIVE + TAGHA_FLAG_LINKED;
				module->funcs.array[i].bytes = 8;
				module->funcs.array[i].item  = func;
			}
		}
	}
}


//#include <unistd.h>    /// sleep() func

static int32_t _tagha_module_exec(struct TaghaModule *const vm)
{
	/** pc is restricted and must not access beyond the function table! */
	union TaghaPtr pc = {.ptruint8 = vm->ip};
	
#define X(x) #x ,
	/** for debugging purposes. */
	//const char *const restrict opcode2str[] = { TAGHA_INSTR_SET };
#undef X
	
#define X(x) &&exec_##x ,
	/** our instruction dispatch table. */
	static const void *const restrict dispatch[] = { TAGHA_INSTR_SET };
#undef X
#undef TAGHA_INSTR_SET
	
#	define DBG_JMP \
		const uint8_t instr = *pc.ptruint8++; \
		\
		if( instr>gef ) { \
			printf("instr : '0x%x' '%u'\n", instr, instr); \
			vm->errcode = tagha_err_instr_oob; \
			return -1; \
		} \
		\
		/*usleep(100);*/ \
		printf("dispatching to '%s'\n", opcode2str[instr]); \
		/*tagha_module_print_vm_state(vm, false);*/ \
		goto *dispatch[instr]

#	define GCC_JMP       goto *dispatch[*pc.ptruint8++]

#	define DISPATCH()    GCC_JMP
	
	/** nop being first will make sure our VM starts with a dispatch! */
	exec_nop: {
		DISPATCH();
	}
	/** push a register's contents. */
	exec_push: { /** char: opcode | char: regid */
		vm->regs[sp].uintptr -= sizeof(union TaghaVal);
		*( union TaghaVal* )vm->regs[sp].uintptr = vm->regs[*pc.ptruint8++];
		DISPATCH();
	}
	/** pops a value from the stack into a register then reduces stack by 8 bytes. */
	exec_pop: { /** char: opcode | char: regid */
		vm->regs[*pc.ptruint8++] = *( union TaghaVal* )vm->regs[sp].uintptr;
		vm->regs[sp].uintptr += sizeof(union TaghaVal);
		DISPATCH();
	}
	exec_ldvar: { /** char: opcode | char: regid | u64: index */
		const uint8_t regid = *pc.ptruint8++;
		vm->regs[regid].uintptr = ( uintptr_t )vm->vars.array[ *pc.ptruint64++ ].item;
		DISPATCH();
	}
	/** loads a function index which could be a native */
	exec_ldfunc: { /** char: opcode | char: regid | i64: index */
		const uint8_t regid = *pc.ptruint8++;
		vm->regs[regid] = *pc.ptrval++;
		DISPATCH();
	}
	exec_ldaddr: { /** char: opcode | char: regid1 | char: regid2 | i32: offset */
		const uint16_t regids = *pc.ptruint16++;
		vm->regs[regids & 0xff].uintptr = vm->regs[regids >> 8].uintptr + *pc.ptrint32++;
		DISPATCH();
	}
	
	exec_movi: { /** char: opcode | char: regid | i64: imm */
		const uint8_t regid = *pc.ptruint8++;
		vm->regs[regid] = *pc.ptrval++;
		DISPATCH();
	}
	exec_mov: { /** char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.ptruint16++;
		vm->regs[regids & 0xff] = vm->regs[regids >> 8];
		DISPATCH();
	}
	
	exec_ld1: { /** char: opcode | char: dest reg | char: src reg | i32: offset */
		const uint16_t regids = *pc.ptruint16++;
		const uintptr_t mem = vm->regs[regids >> 8].uintptr + *pc.ptrint32++;
		if( mem < vm->start_seg || mem > vm->end_seg ) {
			vm->errcode = tagha_err_bad_ptr;
			return -1;
		} else {
			const uint8_t *const restrict ptr = ( uint8_t* )mem;
			vm->regs[regids & 0xff].uint64 = *ptr;
			DISPATCH();
		}
	}
	exec_ld2: { /** char: opcode | char: dest reg | char: src reg | i32: offset */
		const uint16_t regids = *pc.ptruint16++;
		const uintptr_t mem = vm->regs[regids >> 8].uintptr + *pc.ptrint32++;
		if( mem < vm->start_seg || mem+1 > vm->end_seg ) {
			vm->errcode = tagha_err_bad_ptr;
			return -1;
		} else {
			const uint16_t *const restrict ptr = ( uint16_t* )mem;
			vm->regs[regids & 0xff].uint64 = *ptr;
			DISPATCH();
		}
	}
	exec_ld4: { /** char: opcode | char: dest reg | char: src reg | i32: offset */
		const uint16_t regids = *pc.ptruint16++;
		const uintptr_t mem = vm->regs[regids >> 8].uintptr + *pc.ptrint32++;
		if( mem < vm->start_seg || mem+3 > vm->end_seg ) {
			vm->errcode = tagha_err_bad_ptr;
			return -1;
		} else {
			const uint32_t *const restrict ptr = ( uint32_t* )mem;
			vm->regs[regids & 0xff].uint64 = *ptr;
			DISPATCH();
		}
	}
	exec_ld8: { /** char: opcode | char: dest reg | char: src reg | i32: offset */
		const uint16_t regids = *pc.ptruint16++;
		const uintptr_t mem = vm->regs[regids >> 8].uintptr + *pc.ptrint32++;
		if( mem < vm->start_seg || mem+7 > vm->end_seg ) {
			vm->errcode = tagha_err_bad_ptr;
			return -1;
		} else {
			const uint64_t *const restrict ptr = ( uint64_t* )mem;
			vm->regs[regids & 0xff].uint64 = *ptr;
			DISPATCH();
		}
	}
	
	exec_st1: { /** char: opcode | char: dest reg | char: src reg | i32: offset */
		const uint16_t regids = *pc.ptruint16++;
		const uintptr_t mem = vm->regs[regids & 0xff].uintptr + *pc.ptrint32++;
		if( mem < vm->start_seg || mem > vm->end_seg ) {
			vm->errcode = tagha_err_bad_ptr;
			return -1;
		} else {
			uint8_t *const restrict ptr = ( uint8_t* )mem;
			*ptr = vm->regs[regids >> 8].uint8;
			DISPATCH();
		}
	}
	exec_st2: { /** char: opcode | char: dest reg | char: src reg | i32: offset */
		const uint16_t regids = *pc.ptruint16++;
		const uintptr_t mem = vm->regs[regids & 0xff].uintptr + *pc.ptrint32++;
		if( mem < vm->start_seg || mem+1 > vm->end_seg ) {
			vm->errcode = tagha_err_bad_ptr;
			return -1;
		} else {
			uint16_t *const restrict ptr = ( uint16_t* )mem;
			*ptr = vm->regs[regids >> 8].uint16;
			DISPATCH();
		}
	}
	exec_st4: { /** char: opcode | char: dest reg | char: src reg | i32: offset */
		const uint16_t regids = *pc.ptruint16++;
		const uintptr_t mem = vm->regs[regids & 0xff].uintptr + *pc.ptrint32++;
		if( mem < vm->start_seg || mem+3 > vm->end_seg ) {
			vm->errcode = tagha_err_bad_ptr;
			return -1;
		} else {
			uint32_t *const restrict ptr = ( uint32_t* )mem;
			*ptr = vm->regs[regids >> 8].uint32;
			DISPATCH();
		}
	}
	exec_st8: { /** char: opcode | char: dest reg | char: src reg | i32: offset */
		const uint16_t regids = *pc.ptruint16++;
		const uintptr_t mem = vm->regs[regids & 0xff].uintptr + *pc.ptrint32++;
		if( mem < vm->start_seg || mem+7 > vm->end_seg ) {
			vm->errcode = tagha_err_bad_ptr;
			return -1;
		} else {
			uint64_t *const restrict ptr = ( uint64_t* )mem;
			*ptr = vm->regs[regids >> 8].uint64;
			DISPATCH();
		}
	}
	
	exec_add: { /** char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.ptruint16++;
		vm->regs[regids & 0xff].int64 += vm->regs[regids >> 8].int64;
		DISPATCH();
	}
	exec_sub: { /** char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.ptruint16++;
		vm->regs[regids & 0xff].int64 -= vm->regs[regids >> 8].int64;
		DISPATCH();
	}
	exec_mul: { /** char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.ptruint16++;
		vm->regs[regids & 0xff].int64 *= vm->regs[regids >> 8].int64;
		DISPATCH();
	}
	exec_divi: { /** char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.ptruint16++;
		vm->regs[regids & 0xff].uint64 /= vm->regs[regids >> 8].uint64;
		DISPATCH();
	}
	exec_mod: { /** char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.ptruint16++;
		vm->regs[regids & 0xff].uint64 %= vm->regs[regids >> 8].uint64;
		DISPATCH();
	}
	exec_bit_and: { /** char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.ptruint16++;
		vm->regs[regids & 0xff].uint64 &= vm->regs[regids >> 8].uint64;
		DISPATCH();
	}
	exec_bit_or: { /** char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.ptruint16++;
		vm->regs[regids & 0xff].uint64 |= vm->regs[regids >> 8].uint64;
		DISPATCH();
	}
	exec_bit_xor: { /** char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.ptruint16++;
		vm->regs[regids & 0xff].uint64 ^= vm->regs[regids >> 8].uint64;
		DISPATCH();
	}
	exec_shl: { /** char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.ptruint16++;
		vm->regs[regids & 0xff].uint64 <<= vm->regs[regids >> 8].uint64;
		DISPATCH();
	}
	exec_shr: { /** char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.ptruint16++;
		vm->regs[regids & 0xff].uint64 >>= vm->regs[regids >> 8].uint64;
		DISPATCH();
	}
	exec_bit_not: { /** char: opcode | char: regid */
		const uint8_t regid = *pc.ptruint8++;
		vm->regs[regid].uint64 = ~vm->regs[regid].uint64;
		DISPATCH();
	}
	exec_neg: { /** char: opcode | char: regid */
		const uint8_t regid = *pc.ptruint8++;
		vm->regs[regid].int64 = -vm->regs[regid].int64;
		DISPATCH();
	}
	
	exec_ilt: { /** char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.ptruint16++;
		vm->condflag = vm->regs[regids & 0xff].int64 < vm->regs[regids >> 8].int64;
		DISPATCH();
	}
	exec_ile: { /** char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.ptruint16++;
		vm->condflag = vm->regs[regids & 0xff].int64 <= vm->regs[regids >> 8].int64;
		DISPATCH();
	}
	
	exec_igt: { /** char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.ptruint16++;
		vm->condflag = vm->regs[regids & 0xff].int64 > vm->regs[regids >> 8].int64;
		DISPATCH();
	}
	exec_ige: { /** char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.ptruint16++;
		vm->condflag = vm->regs[regids & 0xff].int64 >= vm->regs[regids >> 8].int64;
		DISPATCH();
	}
	
	exec_ult: { /** char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.ptruint16++;
		vm->condflag = vm->regs[regids & 0xff].uint64 < vm->regs[regids >> 8].uint64;
		DISPATCH();
	}
	exec_ule: { /** char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.ptruint16++;
		vm->condflag = vm->regs[regids & 0xff].uint64 <= vm->regs[regids >> 8].uint64;
		DISPATCH();
	}
	
	exec_ugt: { /** char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.ptruint16++;
		vm->condflag = vm->regs[regids & 0xff].uint64 > vm->regs[regids >> 8].uint64;
		DISPATCH();
	}
	exec_uge: { /** char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.ptruint16++;
		vm->condflag = vm->regs[regids & 0xff].uint64 >= vm->regs[regids >> 8].uint64;
		DISPATCH();
	}
	
	exec_cmp: { /** char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.ptruint16++;
		vm->condflag = vm->regs[regids & 0xff].uint64 == vm->regs[regids >> 8].uint64;
		DISPATCH();
	}
	exec_jmp: { /** char: opcode | i64: offset */
		const int64_t offset = *pc.ptrint64++;
		pc.ptruint8 += offset;
		DISPATCH();
	}
	exec_jz: { /** char: opcode | i64: offset */
		const int64_t offset = *pc.ptrint64++;
		if( !vm->condflag ) {
			pc.ptruint8 += offset;
			DISPATCH();
		} else {
			DISPATCH();
		}
	}
	exec_jnz: { /** char: opcode | i64: offset */
		const int64_t offset = *pc.ptrint64++;
		if( vm->condflag ) {
			pc.ptruint8 += offset;
			DISPATCH();
		} else {
			DISPATCH();
		}
	}
	exec_call: { /** char: opcode | i64: offset */
		const int64_t index = *pc.ptrint64++;
		const size_t offset = (index>0LL) ? (index - 1LL) : (-1LL - index);
		if( offset==SIZE_MAX ) {
			vm->errcode = tagha_err_no_func;
			return -1;
		} else {
			const int flags = vm->funcs.array[offset].flags;
			void *const item = vm->funcs.array[offset].item;
			if( flags >= TAGHA_FLAG_NATIVE ) {
				if( flags != TAGHA_FLAG_NATIVE+TAGHA_FLAG_LINKED ) {
					vm->errcode = tagha_err_no_cfunc;
					return -1;
				} else {
					TaghaCFunc *const cfunc = item;
					const size_t args = vm->regs[alaf].size;
					vm->regs[alaf] = (*cfunc)(vm, args, &vm->regs[TAGHA_FIRST_PARAM_REG]);
					if( vm->errcode != tagha_err_none ) {
						return -1;
					} else {
						DISPATCH();
					}
				}
			} else {
				vm->regs[sp].uintptr -= 2 * sizeof(union TaghaVal);
				union TaghaVal *const restrict base = ( union TaghaVal* )vm->regs[sp].uintptr;
				base[1].uintptr = ( uintptr_t )pc.ptrvoid; /** push pc */
				base[0] = vm->regs[bp];                    /** push rbp */
				vm->regs[bp] = vm->regs[sp];               /** mov rbp, rsp */
				pc.ptruint8 = item;
				DISPATCH();
			}
			DISPATCH();
		}
		DISPATCH();
	}
	exec_callr: { /** char: opcode | char: regid */
		const uint8_t regid = *pc.ptruint8++;
		const int64_t index = vm->regs[regid].int64;
		const size_t offset = (index>0LL) ? (index - 1LL) : (-1LL - index);
		if( offset==SIZE_MAX ) {
			vm->errcode = tagha_err_no_func;
			return -1;
		} else {
			const int flags = vm->funcs.array[offset].flags;
			void *const item = vm->funcs.array[offset].item;
			if( flags >= TAGHA_FLAG_NATIVE ) {
				if( flags != TAGHA_FLAG_NATIVE+TAGHA_FLAG_LINKED ) {
					vm->errcode = tagha_err_no_cfunc;
					return -1;
				} else {
					TaghaCFunc *const cfunc = item;
					const size_t args = vm->regs[alaf].size;
					vm->regs[alaf] = (*cfunc)(vm, args, &vm->regs[TAGHA_FIRST_PARAM_REG]);
					if( vm->errcode != tagha_err_none ) {
						return -1;
					} else {
						DISPATCH();
					}
				}
			} else {
				vm->regs[sp].uintptr -= 2 * sizeof(union TaghaVal);
				union TaghaVal *const restrict base = ( union TaghaVal* )vm->regs[sp].uintptr;
				base[1].uintptr = ( uintptr_t )pc.ptrvoid; /** push pc */
				base[0] = vm->regs[bp];                    /** push rbp */
				vm->regs[bp] = vm->regs[sp];               /** mov rbp, rsp */
				pc.ptruint8 = item;
				DISPATCH();
			}
			DISPATCH();
		}
		DISPATCH();
	}
	exec_ret: { /** char: opcode */
		vm->regs[sp] = vm->regs[bp];       /** mov rsp, rbp */
		const union TaghaVal *const restrict base = ( const union TaghaVal* )vm->regs[sp].uintptr;
		vm->regs[bp] = base[0];            /** pop rbp */
		pc.ptrvoid = ( const union TaghaVal* )base[1].uintptr;      /** pop pc */
		vm->regs[sp].uintptr += 2 * sizeof(union TaghaVal);
		if( pc.ptrvoid==NULL ) {
	exec_halt:
			return vm->regs[alaf].int32;
		} else {
			DISPATCH();
		}
	}
	
	/** treated as nop if float32_t is defined but not the other. */
	exec_f32tof64: { /** char: opcode | char: reg id */
		const uint8_t regid = *pc.ptruint8++;
#	if defined(TAGHA_FLOAT32_DEFINED) && defined(TAGHA_FLOAT64_DEFINED)
		const float32_t f = vm->regs[regid].float32;
		vm->regs[regid].float64 = ( float64_t )f;
#	else
		(void)regid;
#	endif
		DISPATCH();
	}
	exec_f64tof32: { /** char: opcode | char: reg id */
		const uint8_t regid = *pc.ptruint8++;
#	if defined(TAGHA_FLOAT32_DEFINED) && defined(TAGHA_FLOAT64_DEFINED)
		const float64_t d = vm->regs[regid].float64;
		vm->regs[regid].int64 = 0;
		vm->regs[regid].float32 = ( float32_t )d;
#	else
		(void)regid;
#	endif
		DISPATCH();
	}
	exec_itof64: { /** char: opcode | char: reg id */
		const uint8_t regid = *pc.ptruint8++;
#	ifdef TAGHA_FLOAT64_DEFINED
		const int64_t i = vm->regs[regid].int64;
		vm->regs[regid].float64 = ( float64_t )i;
#	else
		(void)regid;
#	endif
		DISPATCH();
	}
	exec_itof32: { /** char: opcode | char: reg id */
		const uint8_t regid = *pc.ptruint8++;
#	ifdef TAGHA_FLOAT32_DEFINED
		const int64_t i = vm->regs[regid].int64;
		vm->regs[regid].int64 = 0;
		vm->regs[regid].float32 = ( float32_t )i;
#	else
		(void)regid;
#	endif
		DISPATCH();
	}
	exec_f64toi: { /** char: opcode | char: reg id */
		const uint8_t regid = *pc.ptruint8++;
#	ifdef TAGHA_FLOAT64_DEFINED
		const float64_t i = vm->regs[regid].float64;
		vm->regs[regid].int64 = ( int64_t )i;
#	else
		(void)regid;
#	endif
		DISPATCH();
	}
	exec_f32toi: { /** char: opcode | char: reg id */
		const uint8_t regid = *pc.ptruint8++;
#	ifdef TAGHA_FLOAT32_DEFINED
		const float32_t i = vm->regs[regid].float32;
		vm->regs[regid].int64 = ( int64_t )i;
#	else
		(void)regid;
#	endif
		DISPATCH();
	}
	
	exec_addf: { /** char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.ptruint16++;
#	if defined(TAGHA_FLOAT64_DEFINED) /** if float64_t's are defined, regardless whether float32_t is or not */
		vm->regs[regids & 0xff].float64 += vm->regs[regids >> 8].float64;
#	elif defined(TAGHA_FLOAT32_DEFINED)
		vm->regs[regids & 0xff].float32 += vm->regs[regids >> 8].float32;
#	else
		(void)regids;
#	endif
		DISPATCH();
	}
	exec_subf: { /** char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.ptruint16++;
#	if defined(TAGHA_FLOAT64_DEFINED)
		vm->regs[regids & 0xff].float64 -= vm->regs[regids >> 8].float64;
#	elif defined(TAGHA_FLOAT32_DEFINED)
		vm->regs[regids & 0xff].float32 -= vm->regs[regids >> 8].float32;
#	else
		(void)regids;
#	endif
		DISPATCH();
	}
	exec_mulf: { /** char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.ptruint16++;
#	if defined(TAGHA_FLOAT64_DEFINED)
		vm->regs[regids & 0xff].float64 *= vm->regs[regids >> 8].float64;
#	elif defined(TAGHA_FLOAT32_DEFINED)
		vm->regs[regids & 0xff].float32 *= vm->regs[regids >> 8].float32;
#	else
		(void)regids;
#	endif
		DISPATCH();
	}
	exec_divf: { /** char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.ptruint16++;
#	if defined(TAGHA_FLOAT64_DEFINED)
		vm->regs[regids & 0xff].float64 /= vm->regs[regids >> 8].float64;
#	elif defined(TAGHA_FLOAT32_DEFINED)
		vm->regs[regids & 0xff].float32 /= vm->regs[regids >> 8].float32;
#	else
		(void)regids;
#	endif
		DISPATCH();
	}
	
	exec_ltf: { /** char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.ptruint16++;
#	if defined(TAGHA_FLOAT64_DEFINED)
		vm->condflag = vm->regs[regids & 0xff].float64 < vm->regs[regids >> 8].float64;
#	elif defined(TAGHA_FLOAT32_DEFINED)
		vm->condflag = vm->regs[regids & 0xff].float32 < vm->regs[regids >> 8].float32;
#	else
		(void)regids;
#	endif
		DISPATCH();
	}
	exec_lef: { /** char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.ptruint16++;
#	if defined(TAGHA_FLOAT64_DEFINED)
		vm->condflag = vm->regs[regids & 0xff].float64 <= vm->regs[regids >> 8].float64;
#	elif defined(TAGHA_FLOAT32_DEFINED)
		vm->condflag = vm->regs[regids & 0xff].float32 <= vm->regs[regids >> 8].float32;
#	else
		(void)regids;
#	endif
		DISPATCH();
	}
	
	exec_gtf: { /** char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.ptruint16++;
#	if defined(TAGHA_FLOAT64_DEFINED)
		vm->condflag = vm->regs[regids & 0xff].float64 > vm->regs[regids >> 8].float64;
#	elif defined(TAGHA_FLOAT32_DEFINED)
		vm->condflag = vm->regs[regids & 0xff].float32 > vm->regs[regids >> 8].float32;
#	else
		(void)regids;
#	endif
		DISPATCH();
	}
	exec_gef: { /** char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.ptruint16++;
#	if defined(TAGHA_FLOAT64_DEFINED)
		vm->condflag = vm->regs[regids & 0xff].float64 >= vm->regs[regids >> 8].float64;
#	elif defined(TAGHA_FLOAT32_DEFINED)
		vm->condflag = vm->regs[regids & 0xff].float32 >= vm->regs[regids >> 8].float32;
#	else
		(void)regids;
#	endif
		DISPATCH();
	}
	exec_negf: { /** char: opcode | char: regid */
		const uint8_t regid = *pc.ptruint8++;
#	if defined(TAGHA_FLOAT64_DEFINED)
		const float64_t f = -vm->regs[regid].float64;
		vm->regs[regid].float64 = f;
#	elif defined(TAGHA_FLOAT32_DEFINED)
		const float32_t f = -vm->regs[regid].float32;
		vm->regs[regid].float32 = f;
#	else
		(void)regid;
#	endif
		DISPATCH();
	}
	return -1;
}
