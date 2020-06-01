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

static void _tagha_prep_stackframe(struct TaghaModule *const restrict module, const uintptr_t ret_addr)
{
	module->regs[sp].uintptr -= 2 * sizeof(union TaghaVal);
	union TaghaVal *const restrict rsp = ( union TaghaVal* )module->regs[sp].uintptr;
	rsp[1].uintptr   = ret_addr;                 /// push addr
	rsp[0].uintptr   = module->regs[bp].uintptr; /// push rbp
	module->regs[bp] = module->regs[sp];         /// mov rbp, rsp
}

static NO_NULL struct TaghaItem *_tagha_key_get_item(const struct TaghaSymTable *const restrict syms,
															const char key[restrict static 1])
{
	if( syms->len==0 )
		return NULL;
	else {
		const size_t hash = string_hash(key);
		const size_t bucket_size = (syms->len > TAGHA_SYM_HASH_MAX ? TAGHA_SYM_HASH_MAX : TAGHA_SYM_HASH_MIN);
		const size_t index = hash % bucket_size;
		for( size_t i=syms->buckets[index]; i != SIZE_MAX; i = syms->chain[i] ) {
			if( syms->hashes[i]==hash || !strcmp(syms->keys[i], key) ) {
				return syms->table + i;
			}
		}
		return NULL;
	}
}


static NO_NULL bool _read_module_data(struct TaghaModule *const restrict module, uint8_t filedata[const static 1])
{
	module->script = ( uintptr_t )filedata;
	const struct TaghaHeader *const hdr = ( const struct TaghaHeader* )filedata;
	module->flags = hdr->flags;
	
	union HarbolBinIter iter = { .uint8 = filedata + sizeof *hdr };
	
	/// iterate function table and get bytecode sizes.
	const uint32_t func_table_size = *iter.uint32++;
	for( uint32_t i=0; i<func_table_size; i++ ) {
		const struct TaghaItemEntry *const entry = iter.ptr;
		iter.uint8 += sizeof *entry;
		iter.uint8 += ( !entry->flags ) ? (entry->name_len + entry->data_len) : entry->name_len;
	}
	
	/// iterate global var table
	const uint32_t var_table_size = *iter.uint32++;
	for( uint32_t i=0; i<var_table_size; i++ ) {
		const struct TaghaItemEntry *const entry = iter.ptr;
		iter.uint8 += sizeof *entry;
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
	
	struct TaghaSymTable
		*funcs = harbol_mempool_alloc(&module->heap, sizeof *funcs),
		*vars = harbol_mempool_alloc(&module->heap, sizeof *vars)
	;
	if( vars==NULL || funcs==NULL ) {
		fprintf(stderr, "Tagha Module File Error :: **** unable to allocate symbol tables. ****\n");
		return false;
	}
	funcs->len = func_table_size;
	vars->len  = var_table_size;
	
	iter.uint8 = filedata + sizeof *hdr + sizeof(uint32_t);
	funcs->table  = harbol_mempool_alloc(&module->heap, sizeof *funcs->table  * funcs->len);
	funcs->keys   = harbol_mempool_alloc(&module->heap, sizeof *funcs->keys   * funcs->len);
	funcs->hashes = harbol_mempool_alloc(&module->heap, sizeof *funcs->hashes * funcs->len);
	
	const size_t func_bucket_size = funcs->len > TAGHA_SYM_HASH_MAX ? TAGHA_SYM_HASH_MAX : TAGHA_SYM_HASH_MIN;
	
	funcs->buckets = harbol_mempool_alloc(&module->heap, sizeof *funcs->buckets * func_bucket_size);
	for( size_t i=0; i<func_bucket_size; i++ )
		funcs->buckets[i] = SIZE_MAX;
	
	funcs->chain = harbol_mempool_alloc(&module->heap, sizeof *funcs->chain * funcs->len);
	for( size_t i=0; i<funcs->len; i++ )
		funcs->chain[i] = SIZE_MAX;
	
	for( uint32_t i=0; i<func_table_size; i++ ) {
		const struct TaghaItemEntry *const entry = iter.ptr;
		iter.uint8 += sizeof *entry;
		const uint32_t flag = entry->flags;
		const char *cstr = iter.string;
		const size_t hash = string_hash(cstr);
		iter.uint8 += entry->name_len;
		const struct TaghaItem funcitem = {
			.owner = flag & TAGHA_FLAG_EXTERN ? 0 : ( uintptr_t )module,
			.bytes = entry->data_len,
			.flags = flag,
			.item  = (!flag) ? ( uintptr_t )iter.uint8 : 0
		};
		
		funcs->table[i]  = funcitem;
		funcs->keys[i]   = cstr;
		funcs->hashes[i] = hash;
		
		if( funcs->buckets[hash % func_bucket_size] == SIZE_MAX ) {
			funcs->buckets[hash % func_bucket_size] = i;
		} else {
			size_t n = funcs->buckets[hash % func_bucket_size];
			while( funcs->chain[n] != SIZE_MAX )
				n = funcs->chain[n];
			funcs->chain[n] = i;
		}
		
		if( !flag )
			iter.uint8 += entry->data_len;
	}
	module->end_seg = ( uintptr_t )module->heap.stack.base;
	
	iter.uint32++; /// skip var table size.
	module->start_seg = ( uintptr_t )iter.uint8;
	vars->table  = harbol_mempool_alloc(&module->heap, sizeof *vars->table  * vars->len);
	vars->keys   = harbol_mempool_alloc(&module->heap, sizeof *vars->keys   * vars->len);
	vars->hashes = harbol_mempool_alloc(&module->heap, sizeof *vars->hashes * vars->len);
	
	const size_t var_bucket_size = vars->len > TAGHA_SYM_HASH_MAX ? TAGHA_SYM_HASH_MAX : TAGHA_SYM_HASH_MIN;
	vars->buckets = harbol_mempool_alloc(&module->heap, sizeof *vars->buckets * var_bucket_size);
	
	for( size_t i=0; i<var_bucket_size; i++ )
		vars->buckets[i] = SIZE_MAX;
	
	vars->chain = harbol_mempool_alloc(&module->heap, sizeof *vars->chain * vars->len);
	for( size_t i=0; i<vars->len; i++ )
		vars->chain[i] = SIZE_MAX;
	
	for( uint32_t i=0; i<var_table_size; i++ ) {
		const struct TaghaItemEntry *const entry = iter.ptr;
		iter.uint8 += sizeof *entry;
		const uint32_t flag = entry->flags;
		const char *cstr = iter.string;
		const size_t hash = string_hash(cstr);
		iter.uint8 += entry->name_len;
		const struct TaghaItem varitem = {
			.owner = flag & TAGHA_FLAG_EXTERN ? 0 : ( uintptr_t )module,
			.bytes = entry->data_len,
			.flags = flag,
			.item  = ( uintptr_t )iter.uint8
		};
		
		vars->table[i]  = varitem;
		vars->keys[i]   = cstr;
		vars->hashes[i] = hash;
		
		if( vars->buckets[hash % var_bucket_size] == SIZE_MAX ) {
			vars->buckets[hash % var_bucket_size] = i;
		} else {
			size_t n = vars->buckets[hash % var_bucket_size];
			while( vars->chain[n] != SIZE_MAX )
				n = vars->chain[n];
			vars->chain[n] = i;
		}
		
		iter.uint8 += entry->data_len;
	}
	harbol_mempool_defrag(&module->heap);
	module->funcs = funcs;
	module->vars  = vars;
	
	module->stack = harbol_mempool_alloc(&module->heap, sizeof *module->stack * hdr->stacksize);
	if( module->stack==NULL ) {
		fprintf(stderr, "Tagha Module File Error :: **** couldn't allocate module stack size of %zu bytes, heap remaining: %zu | ('%zu')****\n", hdr->stacksize * sizeof *module->stack, harbol_mempool_mem_remaining(&module->heap), module->heap.stack.base - module->heap.stack.mem);
		return false;
	} else {
		module->regs[sp].uintptr = module->regs[bp].uintptr = ( uintptr_t )(module->stack + hdr->stacksize);
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
	} else if( *( uint32_t* )bytecode != TAGHA_MAGIC_VERIFIER ) {
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
	if( *( uint32_t* )buffer != TAGHA_MAGIC_VERIFIER ) {
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
	if( module->script != 0 ) {
		void *const restrict script = ( void* )module->script;
		free(script);
	}
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
	printf("register instr ptr: '0x%" PRIxPTR "'\n\nPrinting Condition Flag: %u\n=== End Tagha VM State Info ===\n", module->ip, module->condflag);
}

TAGHA_EXPORT const char *tagha_module_get_error(const struct TaghaModule *const restrict module)
{
	switch( module->errcode ) {
		case tagha_err_instr_oob:    return "Out of Bound Instruction";
		case tagha_err_none:         return "None";
		case tagha_err_bad_ptr:      return "Null/Invalid Pointer";
		case tagha_err_no_func:      return "Missing Function";
		case tagha_err_bad_script:   return "Null/Invalid Script";
		case tagha_err_stk_size:     return "Bad Stack Size";
		case tagha_err_no_cfunc:     return "Missing Native";
		case tagha_err_stk_overflow: return "Stack Overflow";
		case tagha_err_bad_extern:   return "Bad External Function";
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
			func->item = ( uintptr_t )n->cfunc;
			func->flags = TAGHA_FLAG_NATIVE | TAGHA_FLAG_LINKED;
		}
	}
	return true;
}

TAGHA_EXPORT bool tagha_module_register_ptr(struct TaghaModule *const module,
											const char name[restrict static 1],
											void *const ptr)
{
	const struct TaghaItem *const restrict var = _tagha_key_get_item(module->vars, name);
	if( var==NULL || var->item==0 ) {
		return false;
	} else {
		void **restrict global_ref = ( void** )var->item;
		*global_ref = ptr;
		return true;
	}
}

TAGHA_EXPORT void *tagha_module_get_var(struct TaghaModule *const restrict module, const char name[restrict static 1])
{
	const struct TaghaItem *const restrict var = _tagha_key_get_item(module->vars, name);
	return( var != NULL ) ? ( void* )var->item : NULL;
}

TAGHA_EXPORT const void *tagha_module_get_func(struct TaghaModule *const restrict module, const char name[restrict static 1])
{
	return _tagha_key_get_item(module->funcs, name);
}

TAGHA_EXPORT uint32_t tagha_module_get_flags(const struct TaghaModule *module)
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
											const void *const restrict f,
											const size_t args,
											const union TaghaVal params[restrict],
											union TaghaVal *const restrict retval)
{
	if( f==NULL ) {
		module->errcode = tagha_err_no_func;
		return -1;
	} else {
		return _tagha_module_start(module, f, args, params, retval);
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
	} else if( func->flags & TAGHA_FLAG_NATIVE ) {
		/// make sure our native function was registered first or else we crash ;)
		if( func->flags & TAGHA_FLAG_LINKED ) {
			TaghaCFunc *const cfunc = ( TaghaCFunc* )func->item;
			memcpy(&module->regs[TAGHA_FIRST_PARAM_REG], &params[0], sizeof(union TaghaVal) * (args));
			const union TaghaVal ret = (*cfunc)(module, args, &module->regs[TAGHA_FIRST_PARAM_REG]);
			if( retval != NULL )
				*retval = ret;
			return( module->errcode==tagha_err_none ) ? 0 : module->errcode;
		} else {
			module->errcode = tagha_err_no_cfunc;
			return -1;
		}
	} else {
		module->ip = func->item;
		if( module->ip==0 ) {
			module->errcode = tagha_err_no_func;
			return -1;
		} else {
			if( params != NULL )
				memcpy(&module->regs[TAGHA_FIRST_PARAM_REG], &params[0], sizeof(union TaghaVal) * (args));
			
			/// prep the stack frame.
			_tagha_prep_stackframe(module, ( uintptr_t )NULL);
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

TAGHA_EXPORT void tagha_module_jit_compile(struct TaghaModule *const restrict module, TaghaCFunc *fn_jit_compile(uintptr_t, size_t, void*), void *const restrict userdata)
{
	if( module->funcs==NULL )
		return;
	else {
		struct TaghaSymTable *const restrict funcs = module->funcs;
		for( uindex_t i=0; i<funcs->len; i++ ) {
			struct TaghaItem *const func = &funcs->table[i];
			/// trying to JIT compile something that's already compiled? lol.
			if( func->flags & TAGHA_FLAG_NATIVE )
				continue;
			/// cannot JIT compile an external function when it's not linked.
			else if( func->flags & TAGHA_FLAG_EXTERN && !(func->flags & TAGHA_FLAG_LINKED) )
				continue;
			else {
				TaghaCFunc *const cfunc = fn_jit_compile(func->item, func->bytes, userdata);
				if( cfunc != NULL ) {
					func->flags = TAGHA_FLAG_NATIVE | TAGHA_FLAG_LINKED;
					func->bytes = 8;
					func->item  = ( uintptr_t )cfunc;
				}
			}
		}
	}
}

TAGHA_EXPORT void tagha_module_resolve_links(struct TaghaModule *const restrict module, const struct TaghaModule *const restrict lib)
{
	if( module->funcs==NULL || lib->funcs==NULL )
		return;
	else {
		struct TaghaSymTable *const funcs = module->funcs;
		for( size_t i=0; i<funcs->len; i++ ) {
			struct TaghaItem *const func = &funcs->table[i];
			if( !(func->flags & TAGHA_FLAG_EXTERN) )
				continue;
			else {
				/// Skip the at-sign so we can get the actual function name.
				const char *at = funcs->keys[i];
				while( *at != 0 && *at != '@' )
					at++;
				
				struct TaghaItem *const extern_func = _tagha_key_get_item(lib->funcs, at+1);
				if( extern_func==NULL
						|| extern_func->flags & TAGHA_FLAG_NATIVE /// don't link natives, could cause potential problems.
						|| extern_func->item==0 ) /// allow getting the extern of an extern but the inner extern must be resolved.
					continue;
				else {
					func->owner = extern_func->owner;
					func->item  = extern_func->item;
					func->flags = TAGHA_FLAG_EXTERN | TAGHA_FLAG_LINKED;
					func->bytes = extern_func->bytes;
				}
			}
		}
	}
}


//#include <unistd.h>    /// sleep() func

static inline uint8_t REG1(const uint16_t r) { return r & 0xff; }
static inline uint8_t REG2(const uint16_t r) { return r  >>  8; }

static int32_t _tagha_module_exec(struct TaghaModule *const vm)
{
	/** pc is restricted and must not access beyond the function table! */
	union TaghaPtr pc = {( const uint64_t* )vm->ip};
	
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
		do { \
			const uint8_t instr = *pc.uint8++; \
			if( instr>=MaxInstrs ) { \
				printf("instr : '0x%x' '%u'\n", instr, instr); \
				vm->errcode = tagha_err_instr_oob; \
				return -1; \
			} else { \
				/*usleep(100);*/ \
				printf("dispatching to '%s'\n", opcode2str[instr]); \
				tagha_module_print_vm_state(vm, false); \
				goto *dispatch[instr]; \
			} \
		} while( 0 )
	
#	define GCC_JMP       goto *dispatch[*pc.uint8++]
	
#	define DISPATCH()    GCC_JMP
	
	/** nop being first will make sure our VM starts with a dispatch! */
	exec_nop: { /** u8: opcode */
		DISPATCH();
	}
	/** push a register's contents. */
	exec_push: { /** u8: opcode | u8: regid */
		vm->regs[sp].uintptr -= sizeof(union TaghaVal);
		*( union TaghaVal* )vm->regs[sp].uintptr = vm->regs[*pc.uint8++];
		DISPATCH();
	}
	/** pops a value from the stack into a register then reduces stack by 8 bytes. */
	exec_pop: { /** u8: opcode | u8: regid */
		vm->regs[*pc.uint8++] = *( union TaghaVal* )vm->regs[sp].uintptr;
		vm->regs[sp].uintptr += sizeof(union TaghaVal);
		DISPATCH();
	}
	exec_ldvar: { /** u8: opcode | u8: regid | u64: index */
		const uint8_t regid = *pc.uint8++;
		vm->regs[regid].uintptr = vm->vars->table[*pc.uint64++].item;
		DISPATCH();
	}
	/** loads a function object. */
	exec_ldfunc: { /** u8: opcode | u8: regid | i64: index */
		const uint8_t regid = *pc.uint8++;
		const int64_t index = *pc.int64++;
		const ssize_t offset = (index > 0LL) ? index - 1LL : -1LL - index;
		vm->regs[regid].uintptr = ( offset < 0 ) ? 0 : ( uintptr_t )&vm->funcs->table[offset];
		DISPATCH();
	}
	exec_ldaddr: { /** u8: opcode | u8: regid1 | u8: regid2 | i32: offset */
		const uint16_t regids = *pc.uint16++;
		vm->regs[REG1(regids)].uintptr = vm->regs[REG2(regids)].uintptr + *pc.int32++;
		DISPATCH();
	}
	
	exec_movi: { /** u8: opcode | u8: regid | i64: imm */
		const uint8_t regid = *pc.uint8++;
		vm->regs[regid] = *pc.val++;
		DISPATCH();
	}
	exec_mov: { /** u8: opcode | u8: dest reg | u8: src reg */
		const uint16_t regids = *pc.uint16++;
		vm->regs[REG1(regids)] = vm->regs[REG2(regids)];
		DISPATCH();
	}
	
	exec_ld1: { /** u8: opcode | u8: dest reg | u8: src reg | i32: offset */
		const uint16_t regids = *pc.uint16++;
		const uintptr_t mem = vm->regs[REG2(regids)].uintptr + *pc.int32++;
		if( mem < vm->start_seg || mem > vm->end_seg ) {
			vm->errcode = tagha_err_bad_ptr;
			return -1;
		} else {
			const uint8_t *const restrict ptr = ( const uint8_t* )mem;
			vm->regs[REG1(regids)].uint64 = *ptr;
			DISPATCH();
		}
	}
	exec_ld2: { /** u8: opcode | u8: dest reg | u8: src reg | i32: offset */
		const uint16_t regids = *pc.uint16++;
		const uintptr_t mem = vm->regs[REG2(regids)].uintptr + *pc.int32++;
		if( mem < vm->start_seg || mem+1 > vm->end_seg ) {
			vm->errcode = tagha_err_bad_ptr;
			return -1;
		} else {
			const uint16_t *const restrict ptr = ( const uint16_t* )mem;
			vm->regs[REG1(regids)].uint64 = *ptr;
			DISPATCH();
		}
	}
	exec_ld4: { /** u8: opcode | u8: dest reg | u8: src reg | i32: offset */
		const uint16_t regids = *pc.uint16++;
		const uintptr_t mem = vm->regs[REG2(regids)].uintptr + *pc.int32++;
		if( mem < vm->start_seg || mem+3 > vm->end_seg ) {
			vm->errcode = tagha_err_bad_ptr;
			return -1;
		} else {
			const uint32_t *const restrict ptr = ( const uint32_t* )mem;
			vm->regs[REG1(regids)].uint64 = *ptr;
			DISPATCH();
		}
	}
	exec_ld8: { /** u8: opcode | u8: dest reg | u8: src reg | i32: offset */
		const uint16_t regids = *pc.uint16++;
		const uintptr_t mem = vm->regs[REG2(regids)].uintptr + *pc.int32++;
		if( mem < vm->start_seg || mem+7 > vm->end_seg ) {
			vm->errcode = tagha_err_bad_ptr;
			return -1;
		} else {
			const uint64_t *const restrict ptr = ( const uint64_t* )mem;
			vm->regs[REG1(regids)].uint64 = *ptr;
			DISPATCH();
		}
	}
	exec_lds1: { /** u8: opcode | u8: dest reg | u8: src reg | i32: offset */
		const uint16_t regids = *pc.uint16++;
		const uintptr_t mem = vm->regs[REG2(regids)].uintptr + *pc.int32++;
		if( mem < vm->start_seg || mem > vm->end_seg ) {
			vm->errcode = tagha_err_bad_ptr;
			return -1;
		} else {
			const int8_t *const restrict ptr = ( const int8_t* )mem;
			vm->regs[REG1(regids)].int64 = *ptr;
			DISPATCH();
		}
	}
	exec_lds2: { /** u8: opcode | u8: dest reg | u8: src reg | i32: offset */
		const uint16_t regids = *pc.uint16++;
		const uintptr_t mem = vm->regs[REG2(regids)].uintptr + *pc.int32++;
		if( mem < vm->start_seg || mem+1 > vm->end_seg ) {
			vm->errcode = tagha_err_bad_ptr;
			return -1;
		} else {
			const int16_t *const restrict ptr = ( const int16_t* )mem;
			vm->regs[REG1(regids)].int64 = *ptr;
			DISPATCH();
		}
	}
	exec_lds4: { /** u8: opcode | u8: dest reg | u8: src reg | i32: offset */
		const uint16_t regids = *pc.uint16++;
		const uintptr_t mem = vm->regs[REG2(regids)].uintptr + *pc.int32++;
		if( mem < vm->start_seg || mem+3 > vm->end_seg ) {
			vm->errcode = tagha_err_bad_ptr;
			return -1;
		} else {
			const int32_t *const restrict ptr = ( const int32_t* )mem;
			vm->regs[REG1(regids)].int64 = *ptr;
			DISPATCH();
		}
	}
	
	exec_st1: { /** u8: opcode | u8: dest reg | u8: src reg | i32: offset */
		const uint16_t regids = *pc.uint16++;
		const uintptr_t mem = vm->regs[REG1(regids)].uintptr + *pc.int32++;
		if( mem < vm->start_seg || mem > vm->end_seg ) {
			vm->errcode = tagha_err_bad_ptr;
			return -1;
		} else {
			uint8_t *const restrict ptr = ( uint8_t* )mem;
			*ptr = vm->regs[REG2(regids)].uint8;
			DISPATCH();
		}
	}
	exec_st2: { /** u8: opcode | u8: dest reg | u8: src reg | i32: offset */
		const uint16_t regids = *pc.uint16++;
		const uintptr_t mem = vm->regs[REG1(regids)].uintptr + *pc.int32++;
		if( mem < vm->start_seg || mem+1 > vm->end_seg ) {
			vm->errcode = tagha_err_bad_ptr;
			return -1;
		} else {
			uint16_t *const restrict ptr = ( uint16_t* )mem;
			*ptr = vm->regs[REG2(regids)].uint16;
			DISPATCH();
		}
	}
	exec_st4: { /** u8: opcode | u8: dest reg | u8: src reg | i32: offset */
		const uint16_t regids = *pc.uint16++;
		const uintptr_t mem = vm->regs[REG1(regids)].uintptr + *pc.int32++;
		if( mem < vm->start_seg || mem+3 > vm->end_seg ) {
			vm->errcode = tagha_err_bad_ptr;
			return -1;
		} else {
			uint32_t *const restrict ptr = ( uint32_t* )mem;
			*ptr = vm->regs[REG2(regids)].uint32;
			DISPATCH();
		}
	}
	exec_st8: { /** u8: opcode | u8: dest reg | u8: src reg | i32: offset */
		const uint16_t regids = *pc.uint16++;
		const uintptr_t mem = vm->regs[REG1(regids)].uintptr + *pc.int32++;
		if( mem < vm->start_seg || mem+7 > vm->end_seg ) {
			vm->errcode = tagha_err_bad_ptr;
			return -1;
		} else {
			uint64_t *const restrict ptr = ( uint64_t* )mem;
			*ptr = vm->regs[REG2(regids)].uint64;
			DISPATCH();
		}
	}
	
	exec_add: { /** u8: opcode | u8: dest reg | u8: src reg */
		const uint16_t regids = *pc.uint16++;
		vm->regs[REG1(regids)].int64 += vm->regs[REG2(regids)].int64;
		DISPATCH();
	}
	exec_sub: { /** u8: opcode | u8: dest reg | u8: src reg */
		const uint16_t regids = *pc.uint16++;
		vm->regs[REG1(regids)].int64 -= vm->regs[REG2(regids)].int64;
		DISPATCH();
	}
	exec_mul: { /** u8: opcode | u8: dest reg | u8: src reg */
		const uint16_t regids = *pc.uint16++;
		vm->regs[REG1(regids)].int64 *= vm->regs[REG2(regids)].int64;
		DISPATCH();
	}
	exec_divi: { /** u8: opcode | u8: dest reg | u8: src reg */
		const uint16_t regids = *pc.uint16++;
		vm->regs[REG1(regids)].uint64 /= vm->regs[REG2(regids)].uint64;
		DISPATCH();
	}
	exec_mod: { /** u8: opcode | u8: dest reg | u8: src reg */
		const uint16_t regids = *pc.uint16++;
		vm->regs[REG1(regids)].uint64 %= vm->regs[REG2(regids)].uint64;
		DISPATCH();
	}
	exec_bit_and: { /** u8: opcode | u8: dest reg | u8: src reg */
		const uint16_t regids = *pc.uint16++;
		vm->regs[REG1(regids)].uint64 &= vm->regs[REG2(regids)].uint64;
		DISPATCH();
	}
	exec_bit_or: { /** u8: opcode | u8: dest reg | u8: src reg */
		const uint16_t regids = *pc.uint16++;
		vm->regs[REG1(regids)].uint64 |= vm->regs[REG2(regids)].uint64;
		DISPATCH();
	}
	exec_bit_xor: { /** u8: opcode | u8: dest reg | u8: src reg */
		const uint16_t regids = *pc.uint16++;
		vm->regs[REG1(regids)].uint64 ^= vm->regs[REG2(regids)].uint64;
		DISPATCH();
	}
	exec_shl: { /** u8: opcode | u8: dest reg | u8: src reg */
		const uint16_t regids = *pc.uint16++;
		vm->regs[REG1(regids)].uint64 <<= vm->regs[REG2(regids)].uint64;
		DISPATCH();
	}
	exec_shr: { /** u8: opcode | u8: dest reg | u8: src reg */
		const uint16_t regids = *pc.uint16++;
		vm->regs[REG1(regids)].uint64 >>= vm->regs[REG2(regids)].uint64;
		DISPATCH();
	}
	exec_shal: { /** u8: opcode | u8: dest reg | u8: src reg */
		const uint16_t regids = *pc.uint16++;
		vm->regs[REG1(regids)].int64 <<= vm->regs[REG2(regids)].uint64;
		DISPATCH();
	}
	exec_shar: { /** u8: opcode | u8: dest reg | u8: src reg */
		const uint16_t regids = *pc.uint16++;
		vm->regs[REG1(regids)].int64 >>= vm->regs[REG2(regids)].uint64;
		DISPATCH();
	}
	exec_bit_not: { /** u8: opcode | u8: regid */
		const uint8_t regid = *pc.uint8++;
		vm->regs[regid].uint64 = ~vm->regs[regid].uint64;
		DISPATCH();
	}
	exec_neg: { /** u8: opcode | u8: regid */
		const uint8_t regid = *pc.uint8++;
		vm->regs[regid].int64 = -vm->regs[regid].int64;
		DISPATCH();
	}
	
	exec_ilt: { /** u8: opcode | u8: reg 1 | u8: reg 2 */
		const uint16_t regids = *pc.uint16++;
		vm->condflag = vm->regs[REG1(regids)].int64 < vm->regs[REG2(regids)].int64;
		DISPATCH();
	}
	exec_ile: { /** u8: opcode | u8: reg 1 | u8: reg 2 */
		const uint16_t regids = *pc.uint16++;
		vm->condflag = vm->regs[REG1(regids)].int64 <= vm->regs[REG2(regids)].int64;
		DISPATCH();
	}
	
	exec_ult: { /** u8: opcode | u8: reg 1 | u8: reg 2 */
		const uint16_t regids = *pc.uint16++;
		vm->condflag = vm->regs[REG1(regids)].uint64 < vm->regs[REG2(regids)].uint64;
		DISPATCH();
	}
	exec_ule: { /** u8: opcode | u8: reg 1 | u8: reg 2 */
		const uint16_t regids = *pc.uint16++;
		vm->condflag = vm->regs[REG1(regids)].uint64 <= vm->regs[REG2(regids)].uint64;
		DISPATCH();
	}
	
	exec_cmp: { /** u8: opcode | u8: reg 1 | u8: reg 2 */
		const uint16_t regids = *pc.uint16++;
		vm->condflag = vm->regs[REG1(regids)].uint64 == vm->regs[REG2(regids)].uint64;
		DISPATCH();
	}
	exec_setc: { /** u8: opcode | u8: reg */
		vm->regs[*pc.uint8++].uint64 = vm->condflag;
		DISPATCH();
	}
	exec_jmp: { /** u8: opcode | i64: offset */
		const int64_t offset = *pc.int64++;
		pc.uint8 += offset;
		DISPATCH();
	}
	exec_jz: { /** u8: opcode | i64: offset */
		const int64_t offset = *pc.int64++;
		if( !vm->condflag ) {
			pc.uint8 += offset;
			DISPATCH();
		} else {
			DISPATCH();
		}
	}
	exec_jnz: { /** u8: opcode | i64: offset */
		const int64_t offset = *pc.int64++;
		if( vm->condflag ) {
			pc.uint8 += offset;
			DISPATCH();
		} else {
			DISPATCH();
		}
	}
	exec_call: { /** u8: opcode | i64: offset */
		const int64_t index = *pc.int64++;
		const ssize_t offset = (index > 0LL) ? index - 1LL : -1LL - index;
		if( offset < 0 ) {
			vm->errcode = tagha_err_no_func;
			return -1;
		} else {
			const uintptr_t item = vm->funcs->table[offset].item;
			if( index < 0 ) {
				if( item==0 ) {
					vm->errcode = tagha_err_no_cfunc;
					return -1;
				} else {
					TaghaCFunc *const cfunc = ( TaghaCFunc* )item;
					const size_t args = vm->regs[alaf].size;
					vm->regs[alaf] = (*cfunc)(vm, args, &vm->regs[TAGHA_FIRST_PARAM_REG]);
					if( vm->errcode != tagha_err_none ) {
						return -1;
					} else {
						DISPATCH();
					}
				}
			} else {
				const uint32_t flags = vm->funcs->table[offset].flags;
				if( flags & TAGHA_FLAG_EXTERN ) {
					struct TaghaModule *const restrict lib = ( struct TaghaModule* )vm->funcs->table[offset].owner;
					if( lib==NULL || item==0 ) {
						vm->errcode = tagha_err_bad_extern;
						return -1;
					} else {
						lib->ip = item;
						memcpy(&lib->regs[TAGHA_FIRST_PARAM_REG], &vm->regs[TAGHA_FIRST_PARAM_REG], sizeof(union TaghaVal) * TAGHA_REG_PARAMS_MAX);
						_tagha_prep_stackframe(lib, ( uintptr_t )NULL);
						_tagha_module_exec(lib);
						vm->regs[alaf] = lib->regs[alaf];
						DISPATCH();
					}
				} else {
					_tagha_prep_stackframe(vm, ( uintptr_t )pc.val);
					pc.uint8 = ( const uint8_t* )item;
					DISPATCH();
				}
			}
		}
	}
	exec_callr: { /** u8: opcode | u8: regid */
		const uint8_t regid = *pc.uint8++;
		const struct TaghaItem *const func = ( const struct TaghaItem* )vm->regs[regid].uintptr;
		if( func==NULL ) {
			vm->errcode = tagha_err_no_func;
			return -1;
		} else {
			const uintptr_t item = func->item;
			if( func->flags & TAGHA_FLAG_NATIVE ) {
				if( item==0 ) {
					vm->errcode = tagha_err_no_cfunc;
					return -1;
				} else {
					TaghaCFunc *const cfunc = ( TaghaCFunc* )item;
					const size_t args = vm->regs[alaf].size;
					vm->regs[alaf] = (*cfunc)(vm, args, &vm->regs[TAGHA_FIRST_PARAM_REG]);
					if( vm->errcode != tagha_err_none ) {
						return -1;
					} else {
						DISPATCH();
					}
				}
			} else {
				/// if not same owner, it's an external function.
				if( func->owner != ( uintptr_t )vm ) {
					struct TaghaModule *const restrict lib = ( struct TaghaModule* )func->owner;
					if( lib==NULL || item==0 ) {
						vm->errcode = tagha_err_bad_extern;
						return -1;
					} else {
						lib->ip = item;
						memcpy(&lib->regs[TAGHA_FIRST_PARAM_REG], &vm->regs[TAGHA_FIRST_PARAM_REG], sizeof(union TaghaVal) * TAGHA_REG_PARAMS_MAX);
						_tagha_prep_stackframe(lib, ( uintptr_t )NULL);
						_tagha_module_exec(lib);
						vm->regs[alaf] = lib->regs[alaf];
						DISPATCH();
					}
				} else {
					_tagha_prep_stackframe(vm, ( uintptr_t )pc.val);
					pc.uint8 = ( const uint8_t* )item;
					DISPATCH();
				}
			}
		}
	}
	
	exec_ret: { /** u8: opcode */
		vm->regs[sp] = vm->regs[bp]; /** mov rsp, rbp */
		const union TaghaVal *const restrict rsp = ( const union TaghaVal* )vm->regs[sp].uintptr;
		vm->regs[bp] = rsp[0];       /** pop rbp */
		const uintptr_t pc_int = rsp[1].uintptr;
		pc.val = ( const union TaghaVal* )pc_int; /** pop pc */
		vm->regs[sp].uintptr += 2 * sizeof(union TaghaVal);
		if( pc.val==NULL ) {
	exec_halt:
			return vm->regs[alaf].int32;
		} else {
			DISPATCH();
		}
	}
	
	/** treated as nop if float32_t is defined but not the other. */
	exec_f32tof64: { /** u8: opcode | u8: reg id */
		const uint8_t regid = *pc.uint8++;
#	if defined(TAGHA_FLOAT32_DEFINED) && defined(TAGHA_FLOAT64_DEFINED)
		const float32_t f = vm->regs[regid].float32;
		vm->regs[regid].float64 = ( float64_t )f;
#	else
		(void)regid;
#	endif
		DISPATCH();
	}
	exec_f64tof32: { /** u8: opcode | u8: reg id */
		const uint8_t regid = *pc.uint8++;
#	if defined(TAGHA_FLOAT32_DEFINED) && defined(TAGHA_FLOAT64_DEFINED)
		const float64_t d = vm->regs[regid].float64;
		vm->regs[regid].int64 = 0;
		vm->regs[regid].float32 = ( float32_t )d;
#	else
		(void)regid;
#	endif
		DISPATCH();
	}
	exec_itof64: { /** u8: opcode | u8: reg id */
		const uint8_t regid = *pc.uint8++;
#	ifdef TAGHA_FLOAT64_DEFINED
		const int64_t i = vm->regs[regid].int64;
		vm->regs[regid].float64 = ( float64_t )i;
#	else
		(void)regid;
#	endif
		DISPATCH();
	}
	exec_itof32: { /** u8: opcode | u8: reg id */
		const uint8_t regid = *pc.uint8++;
#	ifdef TAGHA_FLOAT32_DEFINED
		const int64_t i = vm->regs[regid].int64;
		vm->regs[regid].int64 = 0;
		vm->regs[regid].float32 = ( float32_t )i;
#	else
		(void)regid;
#	endif
		DISPATCH();
	}
	exec_f64toi: { /** u8: opcode | u8: reg id */
		const uint8_t regid = *pc.uint8++;
#	ifdef TAGHA_FLOAT64_DEFINED
		const float64_t i = vm->regs[regid].float64;
		vm->regs[regid].int64 = ( int64_t )i;
#	else
		(void)regid;
#	endif
		DISPATCH();
	}
	exec_f32toi: { /** u8: opcode | u8: reg id */
		const uint8_t regid = *pc.uint8++;
#	ifdef TAGHA_FLOAT32_DEFINED
		const float32_t i = vm->regs[regid].float32;
		vm->regs[regid].int64 = ( int64_t )i;
#	else
		(void)regid;
#	endif
		DISPATCH();
	}
	
	exec_addf: { /** u8: opcode | u8: dest reg | u8: src reg */
		const uint16_t regids = *pc.uint16++;
#	if defined(TAGHA_FLOAT64_DEFINED) /** if float64_t's are defined, regardless whether float32_t is or not */
		vm->regs[REG1(regids)].float64 += vm->regs[REG2(regids)].float64;
#	elif defined(TAGHA_FLOAT32_DEFINED)
		vm->regs[REG1(regids)].float32 += vm->regs[REG2(regids)].float32;
#	else
		(void)regids;
#	endif
		DISPATCH();
	}
	exec_subf: { /** u8: opcode | u8: dest reg | u8: src reg */
		const uint16_t regids = *pc.uint16++;
#	if defined(TAGHA_FLOAT64_DEFINED)
		vm->regs[REG1(regids)].float64 -= vm->regs[REG2(regids)].float64;
#	elif defined(TAGHA_FLOAT32_DEFINED)
		vm->regs[REG1(regids)].float32 -= vm->regs[REG2(regids)].float32;
#	else
		(void)regids;
#	endif
		DISPATCH();
	}
	exec_mulf: { /** u8: opcode | u8: dest reg | u8: src reg */
		const uint16_t regids = *pc.uint16++;
#	if defined(TAGHA_FLOAT64_DEFINED)
		vm->regs[REG1(regids)].float64 *= vm->regs[REG2(regids)].float64;
#	elif defined(TAGHA_FLOAT32_DEFINED)
		vm->regs[REG1(regids)].float32 *= vm->regs[REG2(regids)].float32;
#	else
		(void)regids;
#	endif
		DISPATCH();
	}
	exec_divf: { /** u8: opcode | u8: dest reg | u8: src reg */
		const uint16_t regids = *pc.uint16++;
#	if defined(TAGHA_FLOAT64_DEFINED)
		vm->regs[REG1(regids)].float64 /= vm->regs[REG2(regids)].float64;
#	elif defined(TAGHA_FLOAT32_DEFINED)
		vm->regs[REG1(regids)].float32 /= vm->regs[REG2(regids)].float32;
#	else
		(void)regids;
#	endif
		DISPATCH();
	}
	
	exec_ltf: { /** u8: opcode | u8: reg 1 | u8: reg 2 */
		const uint16_t regids = *pc.uint16++;
#	if defined(TAGHA_FLOAT64_DEFINED)
		vm->condflag = vm->regs[REG1(regids)].float64 < vm->regs[REG2(regids)].float64;
#	elif defined(TAGHA_FLOAT32_DEFINED)
		vm->condflag = vm->regs[REG1(regids)].float32 < vm->regs[REG2(regids)].float32;
#	else
		(void)regids;
#	endif
		DISPATCH();
	}
	exec_lef: { /** u8: opcode | u8: reg 1 | u8: reg 2 */
		const uint16_t regids = *pc.uint16++;
#	if defined(TAGHA_FLOAT64_DEFINED)
		vm->condflag = vm->regs[REG1(regids)].float64 <= vm->regs[REG2(regids)].float64;
#	elif defined(TAGHA_FLOAT32_DEFINED)
		vm->condflag = vm->regs[REG1(regids)].float32 <= vm->regs[REG2(regids)].float32;
#	else
		(void)regids;
#	endif
		DISPATCH();
	}
	exec_negf: { /** u8: opcode | u8: regid */
		const uint8_t regid = *pc.uint8++;
#	if defined(TAGHA_FLOAT64_DEFINED)
		const float64_t f = vm->regs[regid].float64;
		vm->regs[regid].float64 = -f;
#	elif defined(TAGHA_FLOAT32_DEFINED)
		const float32_t f = vm->regs[regid].float32;
		vm->regs[regid].float32 = -f;
#	else
		(void)regid;
#	endif
		DISPATCH();
	}
	return -1;
}
