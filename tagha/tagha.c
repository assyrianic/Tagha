#include <stdlib.h>
#include <stdio.h>
#ifdef OS_WINDOWS
#	define TAGHA_LIB
#endif
#include "tagha.h"

static NO_NULL HOT void _tagha_module_exec(struct TaghaModule *module);
static NEVER_NULL(1,2) bool _tagha_module_start(struct TaghaModule *module, TaghaFunc func, size_t args, const union TaghaVal params[], union TaghaVal *retval);


static NO_NULL struct TaghaItem *_tagha_key_get_item(const struct TaghaSymTable *const restrict syms, const char key[restrict static 1])
{
	if( syms->len==0 )
		return NULL;
	else {
		const size_t hash = string_hash(key);
		const size_t index = hash % TAGHA_SYM_BUCKETS;
		for( size_t i=syms->buckets[index]; i != SIZE_MAX; i = syms->chain[i] ) {
			if( syms->hashes[i]==hash || !strcmp(syms->keys[i], key) ) {
				return syms->table + i;
			}
		}
		return NULL;
	}
}


static NO_NULL bool _setup_memory(struct TaghaModule *const module)
{
	const struct TaghaModuleHeader *const hdr = ( const struct TaghaModuleHeader* )module->script;
	module->heap = harbol_mempool_from_buffer(( uint8_t* )(module->script + hdr->mem_offset), hdr->memsize);
	const size_t given_heapsize = harbol_mempool_mem_remaining(&module->heap);
	if( given_heapsize != hdr->memsize ) {
		fprintf(stderr, "Tagha Module File Error :: **** given heapsize (%zu) is not same as required memory size! (%u). ****\n", given_heapsize, hdr->memsize);
		return false;
	} else {
		module->callstack_size = hdr->callstacksize;
		module->opstack_size = hdr->opstacksize;
		
		module->opstack = ( uintptr_t )harbol_mempool_alloc(&module->heap, hdr->opstacksize);
		module->sp = module->opstack + module->opstack_size;
		
		module->high_seg = module->opstack + module->opstack_size + 1;
		module->callstack = ( uintptr_t )harbol_mempool_alloc(&module->heap, hdr->callstacksize);
		module->cp = module->callstack; 
		return true;
	}
}

static NO_NULL bool _setup_func_table(struct TaghaModule *const module)
{
	const struct TaghaModuleHeader *const hdr = ( const struct TaghaModuleHeader* )module->script;
	struct TaghaSymTable *const funcs = harbol_mempool_alloc(&module->heap, sizeof *funcs);
	if( funcs==NULL ) {
		fputs("Tagha Module File Error :: **** Unable to allocate function symbol table. ****\n", stderr);
		return false;
	}
	funcs->len    = hdr->func_count;
	funcs->table  = harbol_mempool_alloc(&module->heap, sizeof *funcs->table  * funcs->len);
	funcs->keys   = harbol_mempool_alloc(&module->heap, sizeof *funcs->keys   * funcs->len);
	funcs->hashes = harbol_mempool_alloc(&module->heap, sizeof *funcs->hashes * funcs->len);
	
	for( uindex_t i=0; i<TAGHA_SYM_BUCKETS; i++ )
		funcs->buckets[i] = SIZE_MAX;
	
	funcs->chain = harbol_mempool_alloc(&module->heap, sizeof *funcs->chain   * funcs->len);
	for( uindex_t i=0; i<funcs->len; i++ )
		funcs->chain[i] = SIZE_MAX;
	
	union HarbolBinIter iter = { .uint8 = ( uint8_t* )(module->script + hdr->funcs_offset) };
	for( uint32_t i=0; i<hdr->func_count; i++ ) {
		const struct TaghaItemEntry *const entry = iter.ptr;
		iter.uint8 += sizeof *entry;
		const uint32_t flag = entry->flags;
		const char *cstr = iter.string;
		const size_t hash = string_hash(cstr);
		iter.uint8 += entry->name_len;
		const struct TaghaItem funcitem = {
			.owner = flag & TAGHA_FLAG_EXTERN ? NIL : ( uintptr_t )module,
			.bytes = entry->data_len,
			.flags = flag,
			.item  = (!flag) ? ( uintptr_t )iter.uint8 : NIL
		};
		
		funcs->table[i]  = funcitem;
		funcs->keys[i]   = cstr;
		funcs->hashes[i] = hash;
		
		if( funcs->buckets[hash % TAGHA_SYM_BUCKETS] == SIZE_MAX ) {
			funcs->buckets[hash % TAGHA_SYM_BUCKETS] = i;
		} else {
			size_t n = funcs->buckets[hash % TAGHA_SYM_BUCKETS];
			while( funcs->chain[n] != SIZE_MAX )
				n = funcs->chain[n];
			funcs->chain[n] = i;
		}
		
		if( !flag )
			iter.uint8 += entry->data_len;
	}
	module->funcs = funcs;
	return true;
}

static NO_NULL bool _setup_var_table(struct TaghaModule *const module)
{
	const struct TaghaModuleHeader *const hdr = ( const struct TaghaModuleHeader* )module->script;
	struct TaghaSymTable *const vars = harbol_mempool_alloc(&module->heap, sizeof *vars);
	if( vars==NULL ) {
		fputs("Tagha Module File Error :: **** Unable to allocate global var symbol table. ****\n", stderr);
		return false;
	}
	vars->len    = hdr->var_count;
	vars->table  = harbol_mempool_alloc(&module->heap, sizeof *vars->table  * vars->len);
	vars->keys   = harbol_mempool_alloc(&module->heap, sizeof *vars->keys   * vars->len);
	vars->hashes = harbol_mempool_alloc(&module->heap, sizeof *vars->hashes * vars->len);
	
	for( uindex_t i=0; i<TAGHA_SYM_BUCKETS; i++ )
		vars->buckets[i] = SIZE_MAX;
	
	vars->chain = harbol_mempool_alloc(&module->heap, sizeof *vars->chain   * vars->len);
	for( uindex_t i=0; i<vars->len; i++ )
		vars->chain[i] = SIZE_MAX;
	
	union HarbolBinIter iter = { .uint8 = ( uint8_t* )(module->script + hdr->vars_offset) };
	module->low_seg = ( uintptr_t )iter.uint8;
	for( uint32_t i=0; i<hdr->var_count; i++ ) {
		const struct TaghaItemEntry *const entry = iter.ptr;
		iter.uint8 += sizeof *entry;
		const uint32_t flag = entry->flags;
		const char *cstr = iter.string;
		const size_t hash = string_hash(cstr);
		iter.uint8 += entry->name_len;
		const struct TaghaItem varitem = {
			.owner = flag & TAGHA_FLAG_EXTERN ? NIL : ( uintptr_t )module,
			.bytes = entry->data_len,
			.flags = flag,
			.item  = ( uintptr_t )iter.uint8
		};
		vars->table[i]  = varitem;
		vars->keys[i]   = cstr;
		vars->hashes[i] = hash;
		
		if( vars->buckets[hash % TAGHA_SYM_BUCKETS] == SIZE_MAX ) {
			vars->buckets[hash % TAGHA_SYM_BUCKETS] = i;
		} else {
			size_t n = vars->buckets[hash % TAGHA_SYM_BUCKETS];
			while( vars->chain[n] != SIZE_MAX )
				n = vars->chain[n];
			vars->chain[n] = i;
		}
		
		iter.uint8 += entry->data_len;
	}
	module->vars = vars;
	return true;
}

static NO_NULL bool _read_module_data(struct TaghaModule *const restrict module, const uintptr_t filedata)
{
	module->script = filedata;
	const struct TaghaModuleHeader *const hdr = ( const struct TaghaModuleHeader* )filedata;
	module->flags = hdr->flags;
	
	const bool res_mem   = _setup_memory(module);
	const bool res_funcs = _setup_func_table(module);
	const bool res_vars  = _setup_var_table(module);
	return res_mem && res_funcs && res_vars;
}


TAGHA_EXPORT struct TaghaModule *tagha_module_new_from_file(const char filename[restrict static 1])
{
	struct TaghaModule *module = calloc(1, sizeof *module);
	if( module==NULL ) {
		fprintf(stderr, "Tagha Module Error :: **** Unable to allocate module for file '%s'. ****\n", filename);
		return NULL;
	} else {
		uint8_t *restrict bytecode = make_buffer_from_binary(filename);
		if( bytecode==NULL ) {
			fprintf(stderr, "Tagha Module Error :: **** Failed to create file data buffer from '%s'. ****\n", filename);
			free(module), module = NULL;
		} else if( *( uint32_t* )bytecode != TAGHA_MAGIC_VERIFIER ) {
			fprintf(stderr, "Tagha Module Error :: **** Invalid Tagha Module: '%s' ****\n", filename);
			free(bytecode);
			free(module), module = NULL;
		} else if( !_read_module_data(module, ( uintptr_t )bytecode) ) {
			fprintf(stderr, "Tagha Module Error :: **** Couldn't allocate tables for file: '%s' ****\n", filename);
			tagha_module_free(&module);
		}
	}
	return module;
}

TAGHA_EXPORT struct TaghaModule *tagha_module_new_from_buffer(uint8_t buffer[restrict static 1])
{
	struct TaghaModule *module = calloc(1, sizeof *module);
	if( module==NULL ) {
		fputs("Tagha Module Error :: **** Unable to allocate module. ****\n", stderr);
		return NULL;
	} else {
		if( *( uint32_t* )buffer != TAGHA_MAGIC_VERIFIER ) {
			fprintf(stderr, "Tagha Module Error :: **** Invalid Tagha Module Buffer '%p' ****\n", buffer);
			free(module), module = NULL;
		} else if( !_read_module_data(module, ( uintptr_t )buffer) ) {
			fputs("Tagha Module Error :: **** Couldn't allocate tables from buffer ****\n", stderr);
			tagha_module_free(&module);
		}
	}
	return module;
}


TAGHA_EXPORT bool tagha_module_clear(struct TaghaModule *const restrict module)
{
	if( module->script != NIL ) {
		uint8_t *const restrict script = ( uint8_t* )module->script;
		free(script);
	}
	*module = (struct TaghaModule){0};
	return true;
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

TAGHA_EXPORT void *tagha_module_get_var(struct TaghaModule *const restrict module, const char name[restrict static 1])
{
	const struct TaghaItem *const restrict var = _tagha_key_get_item(module->vars, name);
	return( var != NULL ) ? ( void* )var->item : NULL;
}

TAGHA_EXPORT TaghaFunc tagha_module_get_func(struct TaghaModule *const restrict module, const char name[restrict static 1])
{
	return _tagha_key_get_item(module->funcs, name);
}

TAGHA_EXPORT uint32_t tagha_module_get_flags(const struct TaghaModule *const module)
{
	return module->flags;
}

TAGHA_EXPORT const char *tagha_module_get_err(const struct TaghaModule *const restrict module)
{
	switch( module->err ) {
		case TaghaErrNone:        return "None";
		case TaghaErrOpcodeOOB:   return "Out of Bound Instruction";
		case TaghaErrBadPtr:      return "Null/Invalid Pointer";
		case TaghaErrBadFunc:     return "Bad Function";
		case TaghaErrBadNative:   return "Missing Native";
		case TaghaErrBadExtern:   return "Bad External Function";
		case TaghaErrOpStackOF:   return "Stack Overflow";
		default:                  return "User-Defined/Unknown Error";
	}
}

TAGHA_EXPORT inline void tagha_module_throw_err(struct TaghaModule *const module, const enum TaghaErrCode err)
{
	module->err = err;
}

TAGHA_EXPORT void tagha_module_link_natives(struct TaghaModule *const module, const struct TaghaNative natives[static 1])
{
	for( size_t i=0; natives[i].name != NULL && natives[i].cfunc != NULL; i++ ) {
		struct TaghaItem *const func = _tagha_key_get_item(module->funcs, natives[i].name);
		if( func==NULL || func->flags != TAGHA_FLAG_NATIVE ) {
			continue;
		} else {
			func->item = ( uintptr_t )natives[i].cfunc;
			func->flags = TAGHA_FLAG_NATIVE | TAGHA_FLAG_LINKED;
		}
	}
}

TAGHA_EXPORT bool tagha_module_link_ptr(struct TaghaModule *const restrict module, const char name[restrict static 1], const uintptr_t ptr)
{
	const struct TaghaItem *const restrict var = _tagha_key_get_item(module->vars, name);
	if( var==NULL || var->item==NIL ) {
		return false;
	} else {
		uintptr_t *const restrict ref = ( uintptr_t* )var->item;
		*ref = ptr;
		return true;
	}
}

TAGHA_EXPORT void tagha_module_link_module(struct TaghaModule *const restrict module, const struct TaghaModule *const restrict lib)
{
	if( module->funcs==NULL || lib->funcs==NULL )
		return;
	else {
		struct TaghaSymTable *const funcs = module->funcs;
		for( size_t i=0; i<funcs->len; i++ ) {
			struct TaghaItem *const func = &funcs->table[i];
			if( func->flags != TAGHA_FLAG_EXTERN ) {
				continue;    /// skip non-extern and already-linked extern funcs.
			} else {
				struct TaghaItem *const extern_func = _tagha_key_get_item(lib->funcs, funcs->keys[i]);
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


TAGHA_EXPORT bool tagha_module_call(struct TaghaModule *const restrict module,
										const char name[restrict static 1],
										const size_t args,
										const union TaghaVal params[const restrict],
										union TaghaVal *const restrict retval)
{
	const restrict TaghaFunc func = _tagha_key_get_item(module->funcs, name);
	if( func==NULL || func->item==NIL ) {
		module->err = TaghaErrBadFunc;
		return false;
	}
	else return _tagha_module_start(module, func, args, params, retval);
}

TAGHA_EXPORT bool tagha_module_invoke(struct TaghaModule *const module,
											const TaghaFunc f,
											const size_t args,
											const union TaghaVal params[const restrict],
											union TaghaVal *const restrict retval)
{
	if( f==NULL || f->item==NIL ) {
		module->err = TaghaErrBadFunc;
		return false;
	} else {
		return _tagha_module_start(module, f, args, params, retval);
	}
}

TAGHA_EXPORT inline bool tagha_module_run(struct TaghaModule *const module, const size_t argc, const union TaghaVal argv[const], int32_t *const retval)
{
	union TaghaVal r = {0};
	if( tagha_module_call(module, "main", argc, argv, &r) ) {
		*retval = r.int32;
		return true;
	}
	else return false;
}

static bool _tagha_module_start(struct TaghaModule *const module, TaghaFunc func, const size_t args, const union TaghaVal params[const restrict], union TaghaVal *const restrict retval)
{
	const size_t bytes = sizeof(union TaghaVal) * args;
	const size_t arg_bytes = sizeof(union TaghaVal) + bytes; /// one more for arg size.
	if( module->sp - arg_bytes < module->opstack ) {
		module->err = TaghaErrOpStackOF;
		return false;
	} else {
		module->sp -= arg_bytes;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )module->sp;
		memcpy(rsp + 1, params, bytes);
		rsp->size = args;
		
		if( func->flags & TAGHA_FLAG_NATIVE ) {
			if( func->flags & TAGHA_FLAG_LINKED ) {
				TaghaCFunc *const cfunc = ( TaghaCFunc* )func->item;
				const union TaghaVal ret = (*cfunc)(module, rsp);
				if( retval != NULL )
					*retval = ret;
				module->sp += arg_bytes;
				return module->err==TaghaErrNone;
			} else {
				module->err = TaghaErrBadNative;
				module->sp += arg_bytes;
				return false;
			}
		} else {
			module->ip = func->item;
			module->lr = NIL;
			_tagha_module_exec(module);
			if( retval != NULL )
				*retval = *( const union TaghaVal* )module->sp;
			module->sp += arg_bytes;
			return module->err==TaghaErrNone;
		}
	}
}


static void _tagha_module_exec(struct TaghaModule *const vm)
{
	/// pc is restricted and must not access beyond the function table!
	union TaghaPtr pc = { ( const uint64_t* )vm->ip };
	const uintptr_t mem_bnds_diff = vm->high_seg - vm->low_seg;
	
#define X(x) #x ,
	/// for debugging purposes.
	//const char *const restrict op_to_cstr[] = { TAGHA_INSTR_SET };
#undef X
	
#define X(x) &&exec_##x ,
	/// our instruction dispatch table.
	static const void *const restrict dispatch[] = { TAGHA_INSTR_SET };
#undef X
#undef TAGHA_INSTR_SET
	
#	define DBG_JMP \
		do { \
			const uint8_t instr = *pc.uint8++; \
			if( instr>=MaxOps ) { \
				printf("instr : '%#x' '%u' | offset: %zu\n", instr, instr, ( uintptr_t )pc.uint8 - vm->script); \
				vm->err = TaghaErrOpcodeOOB; \
				return; \
			} else { \
				/*usleep(100);*/ \
				printf("dispatching to '%s'\n", op_to_cstr[instr]); \
				goto *dispatch[instr]; \
			} \
		} while( 0 )
	
#	define GCC_JMP       goto *dispatch[*pc.uint8++]
	
#	define DISPATCH()    GCC_JMP
	
	/// nop being first will make sure our vm starts with a dispatch!
	exec_nop: { /// u8: opcode
		DISPATCH();
	}
	
	/// make room via opstack pointer.
	exec_alloc: { /// u8: opcode | u8: cells
		const uint32_t cells = *pc.uint8++; /// allocs up to 8kb per stack frame.
		const size_t alloc_size = sizeof(union TaghaVal) * cells;
		if( vm->sp - alloc_size < vm->opstack ) {
			vm->err = TaghaErrOpStackOF;    /// opstack overflow.
			return;
		} else {
			vm->sp -= alloc_size;
			DISPATCH();
		}
	}
	
	/// reduce opstack registers.
	exec_redux: { /// u8: opcode | u8: cells
		const uint32_t cells = *pc.uint8++;
		const size_t dealloc_size = sizeof(union TaghaVal) * cells;
		if( vm->sp + dealloc_size > vm->opstack + vm->opstack_size ) {
			vm->sp = vm->opstack + vm->opstack_size;
			DISPATCH();
		} else {
			vm->sp += dealloc_size;
			DISPATCH();
		}
	}
	exec_movi: { /// u8: opcode | u8: dest reg | u64: imm
		const uint32_t regid = *pc.uint8++;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )vm->sp;
		rsp[regid] = *pc.val++;
		DISPATCH();
	}
	exec_mov: { /// u8: opcode | u8: dest reg | u8: src reg
		const uint32_t instr = *pc.uint16++;
		const uint32_t dst  = instr & 0xff;
		const uint32_t src  = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )vm->sp;
		rsp[dst] = rsp[src];
		DISPATCH();
	}
	exec_lra: { /// u8: opcode | u8: regid | u16: offset
		const uint32_t regid = *pc.uint8++;
		const uint32_t offset = *pc.uint16++;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )vm->sp;
		//rsp[regid].uintptr = vm->sp + offset;
		rsp[regid].uintptr = ( uintptr_t )(rsp + offset);
		DISPATCH();
	}
	exec_lea: { /// u8: opcode | u8: dest | u8: src | i16: offset
		const uint32_t instr = *pc.uint32++;
		const uint32_t dst   = instr & 0xff;
		const uint32_t src   = (instr & 0xffff) >> 8;
		const int32_t offset = ( int32_t )instr >> 16;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )vm->sp;
		rsp[dst].uintptr = rsp[src].uintptr + offset;
		DISPATCH();
	}
	/// load global variable.
	exec_ldvar: { /// u8: opcode | u8: regid | u16: index
		const uint32_t regid = *pc.uint8++;
		const uint32_t index = *pc.uint16++;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )vm->sp;
		rsp[regid].uintptr = vm->vars->table[index].item;
		DISPATCH();
	}
	/// loads a function object.
	exec_ldfn: { /// u8: opcode | u8: regid | u16: index
		const uint32_t regid = *pc.uint8++;
		const uint32_t index = *pc.uint16++;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )vm->sp;
		rsp[regid].uintptr = ( uintptr_t )&vm->funcs->table[index];
		DISPATCH();
	}
	
	exec_ld1: { /// u8: opcode | u8: dest reg | u8: src reg | i16: offset
		const uint32_t instr = *pc.uint32++;
		const uint32_t dst   = instr & 0xff;
		const uint32_t src   = (instr & 0xffff) >> 8;
		const int32_t offset = ( int32_t )instr >> 16;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )vm->sp;
		const uintptr_t mem = rsp[src].uintptr + offset;
		if( (mem - vm->low_seg) > mem_bnds_diff ) {
			vm->err = TaghaErrBadPtr;
			return;
		} else {
			const int8_t *const restrict ptr = ( const int8_t* )mem;
			rsp[dst].int64 = *ptr;
			DISPATCH();
		}
	}
	exec_ld2: { /// u8: opcode | u8: dest reg | u8: src reg | i16: offset
		const uint32_t instr = *pc.uint32++;
		const uint32_t dst   = instr & 0xff;
		const uint32_t src   = (instr & 0xffff) >> 8;
		const int32_t offset = ( int32_t )instr >> 16;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )vm->sp;
		const uintptr_t mem = rsp[src].uintptr + offset;
		if( (mem+1 - vm->low_seg) > mem_bnds_diff ) {
			vm->err = TaghaErrBadPtr;
			return;
		} else {
			const int16_t *const restrict ptr = ( const int16_t* )mem;
			rsp[dst].int64 = *ptr;
			DISPATCH();
		}
	}
	exec_ld4: { /// u8: opcode | u8: dest reg | u8: src reg | i16: offset
		const uint32_t instr = *pc.uint32++;
		const uint32_t dst   = instr & 0xff;
		const uint32_t src   = (instr & 0xffff) >> 8;
		const int32_t offset = ( int32_t )instr >> 16;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )vm->sp;
		const uintptr_t mem = rsp[src].uintptr + offset;
		if( (mem+3 - vm->low_seg) > mem_bnds_diff ) {
			vm->err = TaghaErrBadPtr;
			return;
		} else {
			const int32_t *const restrict ptr = ( const int32_t* )mem;
			rsp[dst].int64 = *ptr;
			DISPATCH();
		}
	}
	exec_ld8: { /// u8: opcode | u8: dest reg | u8: src reg | i16: offset
		const uint32_t instr = *pc.uint32++;
		const uint32_t dst   = instr & 0xff;
		const uint32_t src   = (instr & 0xffff) >> 8;
		const int32_t offset = ( int32_t )instr >> 16;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )vm->sp;
		const uintptr_t mem = rsp[src].uintptr + offset;
		if( (mem+7 - vm->low_seg) > mem_bnds_diff ) {
			vm->err = TaghaErrBadPtr;
			return;
		} else {
			const union TaghaVal *const restrict ptr = ( const union TaghaVal* )mem;
			rsp[dst] = *ptr;
			DISPATCH();
		}
	}
	
	exec_ldu1: { /// u8: opcode | u8: dest reg | u8: src reg | i16: offset
		const uint32_t instr = *pc.uint32++;
		const uint32_t dst   = instr & 0xff;
		const uint32_t src   = (instr & 0xffff) >> 8;
		const int32_t offset = ( int32_t )instr >> 16;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )vm->sp;
		const uintptr_t mem = rsp[src].uintptr + offset;
		if( (mem - vm->low_seg) > mem_bnds_diff ) {
			vm->err = TaghaErrBadPtr;
			return;
		} else {
			const uint8_t *const restrict ptr = ( const uint8_t* )mem;
			rsp[dst].uint64 = *ptr;
			DISPATCH();
		}
	}
	exec_ldu2: { /// u8: opcode | u8: dest reg | u8: src reg | i16: offset
		const uint32_t instr = *pc.uint32++;
		const uint32_t dst   = instr & 0xff;
		const uint32_t src   = (instr & 0xffff) >> 8;
		const int32_t offset = ( int32_t )instr >> 16;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )vm->sp;
		const uintptr_t mem = rsp[src].uintptr + offset;
		if( (mem+1 - vm->low_seg) > mem_bnds_diff ) {
			vm->err = TaghaErrBadPtr;
			return;
		} else {
			const uint16_t *const restrict ptr = ( const uint16_t* )mem;
			rsp[dst].uint64 = *ptr;
			DISPATCH();
		}
	}
	exec_ldu4: { /// u8: opcode | u8: dest reg | u8: src reg | i16: offset
		const uint32_t instr = *pc.uint32++;
		const uint32_t dst   = instr & 0xff;
		const uint32_t src   = (instr & 0xffff) >> 8;
		const int32_t offset = ( int32_t )instr >> 16;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )vm->sp;
		const uintptr_t mem = rsp[src].uintptr + offset;
		if( (mem+3 - vm->low_seg) > mem_bnds_diff ) {
			vm->err = TaghaErrBadPtr;
			return;
		} else {
			const uint32_t *const restrict ptr = ( const uint32_t* )mem;
			rsp[dst].uint64 = *ptr;
			DISPATCH();
		}
	}
	
	exec_st1: { /// u8: opcode | u8: dest reg | u8: src reg | i16: offset
		const uint32_t instr = *pc.uint32++;
		const uint32_t dst   = instr & 0xff;
		const uint32_t src   = (instr & 0xffff) >> 8;
		const int32_t offset = ( int32_t )instr >> 16;
		const union TaghaVal *const restrict rsp = ( const union TaghaVal* )vm->sp;
		const uintptr_t mem = rsp[dst].uintptr + offset;
		if( (mem - vm->low_seg) > mem_bnds_diff ) {
			vm->err = TaghaErrBadPtr;
			return;
		} else {
			int8_t *const restrict ptr = ( int8_t* )mem;
			*ptr = rsp[src].int8;
			DISPATCH();
		}
	}
	exec_st2: { /// u8: opcode | u8: dest reg | u8: src reg | i16: offset
		const uint32_t instr = *pc.uint32++;
		const uint32_t dst   = instr & 0xff;
		const uint32_t src   = (instr & 0xffff) >> 8;
		const int32_t offset = ( int32_t )instr >> 16;
		const union TaghaVal *const restrict rsp = ( const union TaghaVal* )vm->sp;
		const uintptr_t mem = rsp[dst].uintptr + offset;
		if( (mem - vm->low_seg) > mem_bnds_diff ) {
			vm->err = TaghaErrBadPtr;
			return;
		} else {
			uint16_t *const restrict ptr = ( uint16_t* )mem;
			*ptr = rsp[src].uint16;
			DISPATCH();
		}
	}
	exec_st4: { /// u8: opcode | u8: dest reg | u8: src reg | i16: offset
		const uint32_t instr = *pc.uint32++;
		const uint32_t dst   = instr & 0xff;
		const uint32_t src   = (instr & 0xffff) >> 8;
		const int32_t offset = ( int32_t )instr >> 16;
		const union TaghaVal *const restrict rsp = ( const union TaghaVal* )vm->sp;
		const uintptr_t mem = rsp[dst].uintptr + offset;
		if( (mem - vm->low_seg) > mem_bnds_diff ) {
			vm->err = TaghaErrBadPtr;
			return;
		} else {
			uint32_t *const restrict ptr = ( uint32_t* )mem;
			*ptr = rsp[src].uint32;
			DISPATCH();
		}
	}
	exec_st8: { /// u8: opcode | u8: dest reg | u8: src reg | i16: offset
		const uint32_t instr = *pc.uint32++;
		const uint32_t dst   = instr & 0xff;
		const uint32_t src   = (instr & 0xffff) >> 8;
		const int32_t offset = ( int32_t )instr >> 16;
		const union TaghaVal *const restrict rsp = ( const union TaghaVal* )vm->sp;
		const uintptr_t mem = rsp[dst].uintptr + offset;
		if( (mem - vm->low_seg) > mem_bnds_diff ) {
			vm->err = TaghaErrBadPtr;
			return;
		} else {
			union TaghaVal *const restrict ptr = ( union TaghaVal* )mem;
			*ptr = rsp[src];
			DISPATCH();
		}
	}
	
	exec_add: { /// u8: opcode | u8: dest reg | u8: src reg
		const uint32_t instr = *pc.uint16++;
		const uint32_t dst   = instr & 0xff;
		const uint32_t src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )vm->sp;
		rsp[dst].int64 += rsp[src].int64;
		DISPATCH();
	}
	
	exec_sub: { /// u8: opcode | u8: dest reg | u8: src reg
		const uint32_t instr = *pc.uint16++;
		const uint32_t dst   = instr & 0xff;
		const uint32_t src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )vm->sp;
		rsp[dst].int64 -= rsp[src].int64;
		DISPATCH();
	}
	
	exec_mul: { /// u8: opcode | u8: dest reg | u8: src reg
		const uint32_t instr = *pc.uint16++;
		const uint32_t dst   = instr & 0xff;
		const uint32_t src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )vm->sp;
		rsp[dst].int64 *= rsp[src].int64;
		DISPATCH();
	}
	
	exec_idiv: { /// u8: opcode | u8: dest reg | u8: src reg
		const uint32_t instr = *pc.uint16++;
		const uint32_t dst   = instr & 0xff;
		const uint32_t src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )vm->sp;
		rsp[dst].uint64 /= rsp[src].uint64;
		DISPATCH();
	}
	
	exec_mod: { /// u8: opcode | u8: dest reg | u8: src reg
		const uint32_t instr = *pc.uint16++;
		const uint32_t dst   = instr & 0xff;
		const uint32_t src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )vm->sp;
		rsp[dst].uint64 %= rsp[src].uint64;
		DISPATCH();
	}
	
	exec_neg: { /// u8: opcode | u8: regid
		const uint32_t regid = *pc.uint8++;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )vm->sp;
		rsp[regid].int64 = -rsp[regid].int64;
		DISPATCH();
	}
	
	exec_fadd: { /// u8: opcode | u8: dest reg | u8: src reg
		const uint32_t instr = *pc.uint16++;
		const uint32_t dst   = instr & 0xff;
		const uint32_t src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )vm->sp;
		
		/// if float64's are defined, regardless whether float32 is or not
#	if defined(TAGHA_FLOAT64_DEFINED)
		rsp[dst].float64 += rsp[src].float64;
#	elif defined(TAGHA_FLOAT32_DEFINED)
		rsp[dst].float32 += rsp[src].float32;
#	else
		( void )instr; ( void )dst; ( void )src; ( void )rsp;
#	endif
		DISPATCH();
	}
	
	exec_fsub: { /// u8: opcode | u8: dest reg | u8: src reg
		const uint32_t instr = *pc.uint16++;
		const uint32_t dst   = instr & 0xff;
		const uint32_t src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )vm->sp;
#	if defined(TAGHA_FLOAT64_DEFINED)
		rsp[dst].float64 -= rsp[src].float64;
#	elif defined(TAGHA_FLOAT32_DEFINED)
		rsp[dst].float32 -= rsp[src].float32;
#	else
		( void )instr; ( void )dst; ( void )src; ( void )rsp;
#	endif
		DISPATCH();
	}
	
	exec_fmul: { /// u8: opcode | u8: dest reg | u8: src reg
		const uint32_t instr = *pc.uint16++;
		const uint32_t dst   = instr & 0xff;
		const uint32_t src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )vm->sp;
#	if defined(TAGHA_FLOAT64_DEFINED)
		rsp[dst].float64 *= rsp[src].float64;
#	elif defined(TAGHA_FLOAT32_DEFINED)
		rsp[dst].float32 *= rsp[src].float32;
#	else
		( void )instr; ( void )dst; ( void )src; ( void )rsp;
#	endif
		DISPATCH();
	}
	
	exec_fdiv: { /// u8: opcode | u8: dest reg | u8: src reg
		const uint32_t instr = *pc.uint16++;
		const uint32_t dst   = instr & 0xff;
		const uint32_t src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )vm->sp;
#	if defined(TAGHA_FLOAT64_DEFINED)
		rsp[dst].float64 /= rsp[src].float64;
#	elif defined(TAGHA_FLOAT32_DEFINED)
		rsp[dst].float32 /= rsp[src].float32;
#	else
		( void )instr; ( void )dst; ( void )src; ( void )rsp;
#	endif
		DISPATCH();
	}
	
	exec_fneg: { /// u8: opcode | u8: regid
		const uint32_t regid = *pc.uint8++;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )vm->sp;
#	if defined(TAGHA_FLOAT64_DEFINED)
		const float64_t f = rsp[regid].float64;
		rsp[regid].float64 = -f;
#	elif defined(TAGHA_FLOAT32_DEFINED)
		const float32_t f = rsp[regid].float32;
		rsp[regid].float32 = -f;
#	else
		( void )regid; ( void )rsp;
#	endif
		DISPATCH();
	}
	
	exec_bit_and: { /// u8: opcode | u8: dest reg | u8: src reg
		const uint32_t instr = *pc.uint16++;
		const uint32_t dst   = instr & 0xff;
		const uint32_t src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )vm->sp;
		rsp[dst].uint64 &= rsp[src].uint64;
		DISPATCH();
	}
	
	exec_bit_or: { /// u8: opcode | u8: dest reg | u8: src reg
		const uint32_t instr = *pc.uint16++;
		const uint32_t dst   = instr & 0xff;
		const uint32_t src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )vm->sp;
		rsp[dst].uint64 |= rsp[src].uint64;
		DISPATCH();
	}
	
	exec_bit_xor: { /// u8: opcode | u8: dest reg | u8: src reg
		const uint32_t instr = *pc.uint16++;
		const uint32_t dst   = instr & 0xff;
		const uint32_t src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )vm->sp;
		rsp[dst].uint64 ^= rsp[src].uint64;
		DISPATCH();
	}
	
	exec_shl: { /// u8: opcode | u8: dest reg | u8: src reg
		const uint32_t instr = *pc.uint16++;
		const uint32_t dst   = instr & 0xff;
		const uint32_t src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )vm->sp;
		rsp[dst].uint64 <<= rsp[src].uint64;
		DISPATCH();
	}
	
	exec_shr: { /// u8: opcode | u8: dest reg | u8: src reg
		const uint32_t instr = *pc.uint16++;
		const uint32_t dst   = instr & 0xff;
		const uint32_t src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )vm->sp;
		rsp[dst].uint64 >>= rsp[src].uint64;
		DISPATCH();
	}
	
	exec_shar: { /// u8: opcode | u8: dest reg | u8: src reg
		const uint32_t instr = *pc.uint16++;
		const uint32_t dst   = instr & 0xff;
		const uint32_t src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )vm->sp;
		rsp[dst].int64 >>= rsp[src].uint64;
		DISPATCH();
	}
	
	exec_bit_not: { /// u8: opcode | u8: regid
		const uint32_t regid = *pc.uint8++;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )vm->sp;
		rsp[regid].uint64 = ~rsp[regid].uint64;
		DISPATCH();
	}
	
	exec_cmp: { /// u8: opcode | u8: reg 1 | u8: reg 2
		const uint32_t instr = *pc.uint16++;
		const uint32_t dst   = instr & 0xff;
		const uint32_t src   = instr >> 8;
		const union TaghaVal *const restrict rsp = ( const union TaghaVal* )vm->sp;
		vm->cond = rsp[dst].uint64 == rsp[src].uint64;
		DISPATCH();
	}
	
	exec_ilt: { /// u8: opcode | u8: reg 1 | u8: reg 2
		const uint32_t instr = *pc.uint16++;
		const uint32_t dst   = instr & 0xff;
		const uint32_t src   = instr >> 8;
		const union TaghaVal *const restrict rsp = ( const union TaghaVal* )vm->sp;
		vm->cond = rsp[dst].int64 < rsp[src].int64;
		DISPATCH();
	}
	
	exec_ile: { /// u8: opcode | u8: reg 1 | u8: reg 2
		const uint32_t instr = *pc.uint16++;
		const uint32_t dst   = instr & 0xff;
		const uint32_t src   = instr >> 8;
		const union TaghaVal *const restrict rsp = ( const union TaghaVal* )vm->sp;
		vm->cond = rsp[dst].int64 <= rsp[src].int64;
		DISPATCH();
	}
	
	exec_ult: { /// u8: opcode | u8: reg 1 | u8: reg 2
		const uint32_t instr = *pc.uint16++;
		const uint32_t dst   = instr & 0xff;
		const uint32_t src   = instr >> 8;
		const union TaghaVal *const restrict rsp = ( const union TaghaVal* )vm->sp;
		vm->cond = rsp[dst].uint64 < rsp[src].uint64;
		DISPATCH();
	}
	
	exec_ule: { /// u8: opcode | u8: reg 1 | u8: reg 2
		const uint32_t instr = *pc.uint16++;
		const uint32_t dst   = instr & 0xff;
		const uint32_t src   = instr >> 8;
		const union TaghaVal *const restrict rsp = ( const union TaghaVal* )vm->sp;
		vm->cond = rsp[dst].uint64 <= rsp[src].uint64;
		DISPATCH();
	}
	
	exec_flt: { /// u8: opcode | u8: reg 1 | u8: reg 2
		const uint32_t instr = *pc.uint16++;
		const uint32_t dst   = instr & 0xff;
		const uint32_t src   = instr >> 8;
		const union TaghaVal *const restrict rsp = ( const union TaghaVal* )vm->sp;
#	if defined(TAGHA_FLOAT64_DEFINED)
		vm->cond = rsp[dst].float64 < rsp[src].float64;
#	elif defined(TAGHA_FLOAT32_DEFINED)
		vm->cond = rsp[dst].float32 < rsp[src].float32;
#	else
		( void )instr; ( void )dst; ( void )src; ( void )rsp;
#	endif
		DISPATCH();
	}
	
	exec_fle: { /// u8: opcode | u8: reg 1 | u8: reg 2
		const uint32_t regs = *pc.uint16++;
		const uint32_t dst   = regs & 0xff;
		const uint32_t src   = regs >> 8;
		const union TaghaVal *const restrict rsp = ( const union TaghaVal* )vm->sp;
#	if defined(TAGHA_FLOAT64_DEFINED)
		vm->cond = rsp[dst].float64 <= rsp[src].float64;
#	elif defined(TAGHA_FLOAT32_DEFINED)
		vm->cond = rsp[dst].float32 <= rsp[src].float32;
#	else
		( void )regs; ( void )dst; ( void )src; ( void )rsp;
#	endif
		DISPATCH();
	}
	
	exec_setc: { /// u8: opcode | u8: reg id
		const uint32_t regid = *pc.uint8++;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )vm->sp;
		rsp[regid].uint64 = vm->cond;
		DISPATCH();
	}
	exec_jmp: { /// u8: opcode | i32: offset
		const int32_t offset = *pc.int32++;
		pc.uint8 += offset;
		DISPATCH();
	}
	exec_jz: { /// u8: opcode | i32: offset
		const int32_t offset = *pc.int32++;
		if( !vm->cond ) {
			pc.uint8 += offset;
			DISPATCH();
		} else {
			DISPATCH();
		}
	}
	exec_jnz: { /// u8: opcode | i32: offset
		const int32_t offset = *pc.int32++;
		if( vm->cond ) {
			pc.uint8 += offset;
			DISPATCH();
		} else {
			DISPATCH();
		}
	}
	
	exec_pushlr: { /// u8: opcode
		uintptr_t *const restrict call_stack = ( uintptr_t* )vm->cp;
		*call_stack = vm->lr;
		vm->cp += sizeof(uintptr_t);
		DISPATCH();
	}
	exec_poplr: { /// u8: opcode
		vm->cp -= sizeof(uintptr_t);
		const uintptr_t *const restrict call_stack = ( const uintptr_t* )vm->cp;
		vm->lr = *call_stack;
		DISPATCH();
	}
	
	exec_call: { /// u8: opcode | u16: index
		const uint32_t index = *pc.uint16++;
		const TaghaFunc func = vm->funcs->table + (index - 1);
		const uintptr_t item = func->item;
		const uint32_t flags = func->flags;
		if( item==NIL || func->owner==NIL ) {
			vm->err = flags;
			return;
		} else if( flags & TAGHA_FLAG_NATIVE ) {
			TaghaCFunc *const cfunc = ( TaghaCFunc* )item;
			union TaghaVal *const restrict rsp = ( union TaghaVal* )vm->sp;
			*rsp = (*cfunc)(vm, rsp);
			if( vm->err != TaghaErrNone ) {
				return;
			} else {
				DISPATCH();
			}
		} else if( flags & TAGHA_FLAG_EXTERN ) {
			struct TaghaModule *const restrict lib = ( struct TaghaModule* )func->owner;
			/// save old symbol tables.
			uintptr_t
				saved_funcs = ( uintptr_t )vm->funcs,
				saved_vars  = ( uintptr_t )vm->vars
			;
			vm->funcs = lib->funcs;
			vm->vars  = lib->vars;
			
			/// save link register.
			{
				uintptr_t *const restrict call_stack = ( uintptr_t* )vm->cp;
				*call_stack = vm->lr;
				vm->cp += sizeof(uintptr_t);
			}
			vm->ip = item;
			vm->lr = NIL;
			_tagha_module_exec(vm);
			
			/// restore link register.
			{
				vm->cp -= sizeof(uintptr_t);
				const uintptr_t *const restrict call_stack = ( const uintptr_t* )vm->cp;
				vm->lr = *call_stack;
			}
			vm->funcs = ( struct TaghaSymTable* )saved_funcs;
			vm->vars  = ( struct TaghaSymTable* )saved_vars;
			DISPATCH();
		} else {
			vm->lr = ( uintptr_t )pc.uint8;
			pc.uint8 = ( const uint8_t* )item;
			DISPATCH();
		}
	}
	exec_callr: { /// u8: opcode | u8: regid
		union TaghaVal *const restrict rsp = ( union TaghaVal* )vm->sp;
		const uint32_t regid = *pc.uint8++;
		const struct TaghaItem *const func = ( const struct TaghaItem* )rsp[regid].uintptr;
		if( func==NULL ) {
			vm->err = TaghaErrBadFunc;
			return;
		} else {
			const uintptr_t item = func->item;
			if( func->flags & TAGHA_FLAG_NATIVE ) {
				if( item==NIL ) {
					vm->err = TaghaErrBadNative;
					return;
				} else {
					TaghaCFunc *const cfunc = ( TaghaCFunc* )item;
					*rsp = (*cfunc)(vm, rsp);
					if( vm->err != TaghaErrNone ) {
						return;
					} else {
						DISPATCH();
					}
				}
			} else if( func->owner != ( uintptr_t )vm ) {
				/// if not same owner, it's an external function.
				if( func->owner==NIL ) {
					vm->err = TaghaErrBadExtern;
					return;
				} else {
					struct TaghaModule *const restrict lib = ( struct TaghaModule* )func->owner;
					/// save old symbol tables.
					uintptr_t
						saved_funcs = ( uintptr_t )vm->funcs,
						saved_vars  = ( uintptr_t )vm->vars
					;
					vm->funcs = lib->funcs;
					vm->vars  = lib->vars;
					
					/// save link register.
					{
						uintptr_t *const restrict call_stack = ( uintptr_t* )vm->cp;
						*call_stack = vm->lr;
						vm->cp += sizeof(uintptr_t);
					}
					vm->ip = item;
					vm->lr = NIL;
					_tagha_module_exec(vm);
					
					/// restore link register.
					{
						vm->cp -= sizeof(uintptr_t);
						const uintptr_t *const restrict call_stack = ( const uintptr_t* )vm->cp;
						vm->lr = *call_stack;
					}
					vm->funcs = ( struct TaghaSymTable* )saved_funcs;
					vm->vars  = ( struct TaghaSymTable* )saved_vars;
					DISPATCH();
				}
			} else {
				vm->lr = ( uintptr_t )pc.uint8;
				pc.uint8 = ( const uint8_t* )item;
				DISPATCH();
			}
		}
	}
	
	exec_ret: { /// u8: opcode
		pc.uint8 = ( const uint8_t* )vm->lr;
		if( pc.uint8==NULL ) {
	exec_halt:
			return;
		} else {
			DISPATCH();
		}
	}
	
	/// treated as nop if float32_t is defined but not the other.
	exec_f32tof64: { /// u8: opcode | u8: reg id
		const uint32_t regid = *pc.uint8++;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )vm->sp;
#	if defined(TAGHA_FLOAT32_DEFINED) && defined(TAGHA_FLOAT64_DEFINED)
		const float32_t f = rsp[regid].float32;
		rsp[regid].float64 = ( float64_t )f;
#	else
		( void )regid; ( void )rsp;
#	endif
		DISPATCH();
	}
	exec_f64tof32: { /// u8: opcode | u8: reg id
		const uint32_t regid = *pc.uint8++;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )vm->sp;
#	if defined(TAGHA_FLOAT32_DEFINED) && defined(TAGHA_FLOAT64_DEFINED)
		const float64_t d = rsp[regid].float64;
		rsp[regid].int64 = 0;
		rsp[regid].float32 = ( float32_t )d;
#	else
		( void )regid; ( void )rsp;
#	endif
		DISPATCH();
	}
	exec_itof64: { /// u8: opcode | u8: reg id
		const uint32_t regid = *pc.uint8++;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )vm->sp;
#	ifdef TAGHA_FLOAT64_DEFINED
		const int64_t i = rsp[regid].int64;
		rsp[regid].float64 = ( float64_t )i;
#	else
		( void )regid; ( void )rsp;
#	endif
		DISPATCH();
	}
	exec_itof32: { /// u8: opcode | u8: reg id
		const uint32_t regid = *pc.uint8++;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )vm->sp;
#	ifdef TAGHA_FLOAT32_DEFINED
		const int64_t i = rsp[regid].int64;
		rsp[regid].int64 = 0;
		rsp[regid].float32 = ( float32_t )i;
#	else
		( void )regid; ( void )rsp;
#	endif
		DISPATCH();
	}
	exec_f64toi: { /// u8: opcode | u8: reg id
		const uint32_t regid = *pc.uint8++;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )vm->sp;
#	ifdef TAGHA_FLOAT64_DEFINED
		const float64_t i = rsp[regid].float64;
		rsp[regid].int64 = ( int64_t )i;
#	else
		( void )regid; ( void )rsp;
#	endif
		DISPATCH();
	}
	exec_f32toi: { /// u8: opcode | u8: reg id
		const uint32_t regid = *pc.uint8++;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )vm->sp;
#	ifdef TAGHA_FLOAT32_DEFINED
		const float32_t i = rsp[regid].float32;
		rsp[regid].int64 = ( int64_t )i;
#	else
		( void )regid; ( void )rsp;
#	endif
		DISPATCH();
	}
}