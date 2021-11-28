#include <stdlib.h>
#include <stdio.h>

#ifdef OS_WINDOWS
#	define TAGHA_LIB
#endif

#include "tagha.h"

static NO_NULL HOT void _tagha_module_exec(struct TaghaModule *module, uintptr_t ip);
static NEVER_NULL(1,2) bool _tagha_module_start(struct TaghaModule *module, const struct TaghaItem *func, size_t args, const union TaghaVal params[], union TaghaVal *retval);


static NO_NULL struct TaghaItem *_tagha_key_get_item(const struct TaghaSymTable *const syms, const char key[static 1])
{
	if( syms->len==0 ) {
		return NULL;
	} else {
		const size_t hash  = string_hash(key);
		const size_t index = hash % TAGHA_SYM_BUCKETS;
		for( size_t i=syms->buckets[index]; i != SIZE_MAX; i = syms->chain[i] ) {
			if( syms->hashes[i]==hash || !strcmp(syms->keys[i], key) ) {
				return &syms->table[i];
			}
		}
		return NULL;
	}
}

static NO_NULL size_t _tagha_item_index(const struct TaghaSymTable *const syms, const struct TaghaItem *const item)
{
	return ( size_t )(item - syms->table);
}


static NO_NULL bool _setup_memory(struct TaghaModule *const module)
{
	const struct TaghaModuleHeader *const hdr = ( const struct TaghaModuleHeader* )(module->script);
	bool res = false;
	module->heap = harbol_mempool_make_from_buffer(( uint8_t* )(module->script + hdr->mem_offset), hdr->memsize, &res);
	if( !res ) {
		fprintf(stderr, "Tagha Module File Error :: **** Failed to marshall script heap. ****\n");
		return false;
	}
	
	const size_t given_heapsize = harbol_mempool_mem_remaining(&module->heap);
	if( given_heapsize != hdr->memsize ) {
		fprintf(stderr, "Tagha Module File Error :: **** given heapsize (%zu) is not same as required memory size! (%u). ****\n", given_heapsize, hdr->memsize);
		return false;
	} else {
		module->callstack_size = hdr->callstacksize;
		module->opstack_size   = hdr->opstacksize;
		
		module->opstack        = tagha_module_heap_alloc(module, hdr->opstacksize);
		module->osp            = module->opstack + module->opstack_size;
		
		module->high_seg       = module->opstack + module->opstack_size + 1;
		module->callstack      = tagha_module_heap_alloc(module, hdr->callstacksize);
		module->csp            = module->callstack; 
		return true;
	}
}

static NO_NULL bool _setup_func_table(struct TaghaModule *const module)
{
	const struct TaghaModuleHeader *const hdr = ( const struct TaghaModuleHeader* )(module->script);
	struct TaghaSymTable *const funcs = harbol_mempool_alloc(&module->heap, sizeof *funcs);
	if( funcs==NULL ) {
		fputs("Tagha Module File Error :: **** Unable to allocate function symbol table. ****\n", stderr);
		return false;
	}
	funcs->len    = hdr->func_count;
	
	funcs->table  = harbol_mempool_alloc(&module->heap, sizeof *funcs->table  * funcs->len);
	funcs->bytes  = harbol_mempool_alloc(&module->heap, sizeof *funcs->bytes  * funcs->len);
	funcs->keys   = harbol_mempool_alloc(&module->heap, sizeof *funcs->keys   * funcs->len);
	funcs->hashes = harbol_mempool_alloc(&module->heap, sizeof *funcs->hashes * funcs->len);
	
	for( size_t i=0; i<TAGHA_SYM_BUCKETS; i++ ) {
		funcs->buckets[i] = SIZE_MAX;
	}
	
	funcs->chain = harbol_mempool_alloc(&module->heap, sizeof *funcs->chain   * funcs->len);
	for( size_t i=0; i<funcs->len; i++ ) {
		funcs->chain[i] = SIZE_MAX;
	}
	
	union HarbolIter iter = { .uint8 = ( const uint8_t* )(module->script + hdr->funcs_offset) };
	for( uint32_t i=0; i<hdr->func_count; i++ ) {
		const struct TaghaItemEntry *const entry = iter.ptr;
		iter.uint8 += sizeof *entry;
		const uint32_t flag = entry->flags;
		const char    *cstr = iter.string;
		const size_t   hash = string_hash(cstr);
		iter.uint8 += entry->name_len;
		
		struct TaghaItem *const fn = (funcs->table + i);
		fn->item  = (!flag)? ( uintptr_t )(iter.uint8) : NIL;
		fn->owner = (flag & TAGHA_FLAG_EXTERN)? NIL : ( uintptr_t )(module);
		fn->flags = flag;
		
		funcs->bytes[i]  = entry->data_len;
		funcs->keys[i]   = cstr;
		funcs->hashes[i] = hash;
		
		if( funcs->buckets[hash % TAGHA_SYM_BUCKETS] == SIZE_MAX ) {
			funcs->buckets[hash % TAGHA_SYM_BUCKETS] = i;
		} else {
			size_t n = funcs->buckets[hash % TAGHA_SYM_BUCKETS];
			while( funcs->chain[n] != SIZE_MAX ) {
				n = funcs->chain[n];
			}
			funcs->chain[n] = i;
		}
		
		if( !flag ) {
			iter.uint8 += entry->data_len;
		}
	}
	module->funcs = funcs;
	return true;
}

static NO_NULL bool _setup_var_table(struct TaghaModule *const module)
{
	const struct TaghaModuleHeader *const hdr = ( const struct TaghaModuleHeader* )(module->script);
	struct TaghaSymTable *const vars = harbol_mempool_alloc(&module->heap, sizeof *vars);
	if( vars==NULL ) {
		fputs("Tagha Module File Error :: **** Unable to allocate global var symbol table. ****\n", stderr);
		return false;
	}
	vars->len    = hdr->var_count;
	
	vars->table  = harbol_mempool_alloc(&module->heap, sizeof *vars->table  * vars->len);
	vars->bytes  = harbol_mempool_alloc(&module->heap, sizeof *vars->bytes  * vars->len);
	vars->keys   = harbol_mempool_alloc(&module->heap, sizeof *vars->keys   * vars->len);
	vars->hashes = harbol_mempool_alloc(&module->heap, sizeof *vars->hashes * vars->len);
	
	for( size_t i=0; i<TAGHA_SYM_BUCKETS; i++ ) {
		vars->buckets[i] = SIZE_MAX;
	}
	
	vars->chain = harbol_mempool_alloc(&module->heap, sizeof *vars->chain   * vars->len);
	for( size_t i=0; i<vars->len; i++ ) {
		vars->chain[i] = SIZE_MAX;
	}
	
	union HarbolIter iter = { .uint8 = ( uint8_t* )(module->script + hdr->vars_offset) };
	module->low_seg = ( uintptr_t )(iter.uint8);
	for( uint32_t i=0; i<hdr->var_count; i++ ) {
		const struct TaghaItemEntry *const entry = iter.ptr;
		iter.uint8 += sizeof *entry;
		const uint32_t flag = entry->flags;
		const char    *cstr = iter.string;
		const size_t   hash = string_hash(cstr);
		iter.uint8 += entry->name_len;
		
		struct TaghaItem *const v = (vars->table + i);
		v->item  = ( uintptr_t )(iter.uint8);
		v->owner = (flag & TAGHA_FLAG_EXTERN)? NIL : ( uintptr_t )(module);
		v->flags = flag;
		
		vars->bytes[i]  = entry->data_len;
		vars->keys[i]   = cstr;
		vars->hashes[i] = hash;
		
		if( vars->buckets[hash % TAGHA_SYM_BUCKETS] == SIZE_MAX ) {
			vars->buckets[hash % TAGHA_SYM_BUCKETS] = i;
		} else {
			size_t n = vars->buckets[hash % TAGHA_SYM_BUCKETS];
			while( vars->chain[n] != SIZE_MAX ) {
				n = vars->chain[n];
			}
			vars->chain[n] = i;
		}
		
		iter.uint8 += entry->data_len;
	}
	module->vars = vars;
	return true;
}

static NO_NULL bool _read_module_data(struct TaghaModule *const module, const uintptr_t filedata)
{
	module->script = filedata;
	const struct TaghaModuleHeader *const hdr = ( const struct TaghaModuleHeader* )(filedata);
	module->flags = hdr->flags;
	
	const bool res_mem   = _setup_memory(module);
	const bool res_funcs = _setup_func_table(module);
	const bool res_vars  = _setup_var_table(module);
	return res_mem && res_funcs && res_vars;
}


TAGHA_EXPORT struct TaghaModule *tagha_module_new_from_file(const char filename[static 1])
{
	struct TaghaModule *module = calloc(1, sizeof *module);
	if( module==NULL ) {
		fprintf(stderr, "Tagha Module Error :: **** Unable to allocate module for file '%s'. ****\n", filename);
		return NULL;
	} else {
		size_t buf_len = 0;
		uint8_t *restrict bytecode = make_buffer_from_binary(filename, &buf_len);
		if( bytecode==NULL ) {
			fprintf(stderr, "Tagha Module Error :: **** Failed to create file data buffer from '%s'. ****\n", filename);
			free(module); module = NULL;
		} else if( buf_len < sizeof(struct TaghaModuleHeader) || *( const uint32_t* )(bytecode) != TAGHA_MAGIC_VERIFIER ) {
			fprintf(stderr, "Tagha Module Error :: **** Invalid Tagha Module: '%s' ****\n", filename);
			free(bytecode);
			free(module); module = NULL;
		} else if( !_read_module_data(module, ( uintptr_t )(bytecode)) ) {
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
		if( *( const uint32_t* )(buffer) != TAGHA_MAGIC_VERIFIER ) {
			fprintf(stderr, "Tagha Module Error :: **** Invalid Tagha Module Buffer '%p' ****\n", buffer);
			tagha_module_free(&module);
		} else if( !_read_module_data(module, ( uintptr_t )(buffer)) ) {
			fputs("Tagha Module Error :: **** Couldn't allocate tables from buffer ****\n", stderr);
			tagha_module_free(&module);
		}
	}
	return module;
}


TAGHA_EXPORT bool tagha_module_clear(struct TaghaModule *const restrict module)
{
	if( module->script != NIL ) {
		uint8_t *const restrict script = ( uint8_t* )(module->script);
		free(script);
	}
	*module = ( struct TaghaModule ){0};
	return true;
}

TAGHA_EXPORT bool tagha_module_free(struct TaghaModule **const modref)
{
	if( *modref==NULL ) {
		return false;
	} else {
		tagha_module_clear(*modref);
		free(*modref); *modref=NULL;
		return true;
	}
}

TAGHA_EXPORT void *tagha_module_get_var(const struct TaghaModule *const module, const char name[static 1])
{
	const struct TaghaItem *const var = _tagha_key_get_item(module->vars, name);
	return( var != NULL )? ( void* )(var->item) : NULL;
}

TAGHA_EXPORT const struct TaghaItem *tagha_module_get_func(const struct TaghaModule *const module, const char name[static 1])
{
	return _tagha_key_get_item(module->funcs, name);
}

TAGHA_EXPORT uint32_t tagha_module_get_flags(const struct TaghaModule *const module)
{
	return module->flags;
}

TAGHA_EXPORT uintptr_t tagha_module_heap_alloc(struct TaghaModule *const module, const size_t size)
{
	return ( uintptr_t )(harbol_mempool_alloc(&module->heap, size));
}

TAGHA_EXPORT bool tagha_module_heap_free(struct TaghaModule *const module, const uintptr_t ptr)
{
	return harbol_mempool_free(&module->heap, ( void* )(ptr));
}


TAGHA_EXPORT const char *tagha_module_get_err(const struct TaghaModule *const module)
{
	switch( module->err ) {
		case TaghaErrNone:      return "None";
		case TaghaErrOpcodeOOB: return "Out of Bound Instruction";
		case TaghaErrBadPtr:    return "Null/Invalid Pointer";
		case TaghaErrBadFunc:   return "Bad Function";
		case TaghaErrBadNative: return "Missing Native";
		case TaghaErrBadExtern: return "Bad External Function";
		case TaghaErrOpStackOF: return "Stack Overflow";
		default:                return "User-Defined/Unknown Error";
	}
}

TAGHA_EXPORT void tagha_module_throw_err(struct TaghaModule *const module, const enum TaghaErrCode err)
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
			func->item  = ( uintptr_t )(natives[i].cfunc);
			func->flags = TAGHA_FLAG_NATIVE | TAGHA_FLAG_LINKED;
		}
	}
}

TAGHA_EXPORT bool tagha_module_link_ptr(struct TaghaModule *const restrict module, const char name[static 1], const uintptr_t ptr)
{
	const struct TaghaItem *const var = _tagha_key_get_item(module->vars, name);
	if( var==NULL || var->item==NIL ) {
		return false;
	} else {
		uintptr_t *const restrict ref = ( uintptr_t* )(var->item);
		*ref = ptr;
		return true;
	}
}

TAGHA_EXPORT void tagha_module_link_module(struct TaghaModule *const restrict module, const struct TaghaModule *const lib)
{
	if( module->funcs==NULL || lib->funcs==NULL ) {
		return;
	} else {
		const struct TaghaSymTable *const funcs = module->funcs;
		for( size_t i=0; i<funcs->len; i++ ) {
			struct TaghaItem *const restrict func = &funcs->table[i];
			if( func->flags != TAGHA_FLAG_EXTERN ) {
				continue;    /// skip non-extern and already-linked extern funcs.
			} else {
				const struct TaghaSymTable *const libfuncs = lib->funcs;
				const struct TaghaItem *const extern_func = _tagha_key_get_item(libfuncs, funcs->keys[i]);
				if( extern_func==NULL
						|| extern_func->flags & TAGHA_FLAG_NATIVE /// don't link natives, could cause potential problems.
						|| extern_func->item==NIL /// allow getting the extern of an extern but the inner extern must be resolved.
				  ) {
					continue;
				} else {
					func->owner = extern_func->owner;
					func->item  = extern_func->item;
					func->flags = TAGHA_FLAG_EXTERN | TAGHA_FLAG_LINKED;
					
					const size_t index = _tagha_item_index(libfuncs, extern_func);
					funcs->bytes[i] = libfuncs->bytes[index];
				}
			}
		}
	}
}


TAGHA_EXPORT bool tagha_module_call(struct TaghaModule *const restrict module,
										const char name[static 1],
										const size_t args,
										const union TaghaVal params[const],
										union TaghaVal *const restrict retval
)
{
	const struct TaghaItem *const func = _tagha_key_get_item(module->funcs, name);
	if( func==NULL || func->item==NIL ) {
		module->err = TaghaErrBadFunc;
		return false;
	} else {
		return _tagha_module_start(module, func, args, params, retval);
	}
}

TAGHA_EXPORT bool tagha_module_invoke(struct TaghaModule *const module,
											const struct TaghaItem *const f,
											const size_t args,
											const union TaghaVal params[const],
											union TaghaVal *const restrict retval
)
{
	if( f->item==NIL ) {
		module->err = TaghaErrBadFunc;
		return false;
	} else {
		return _tagha_module_start(module, f, args, params, retval);
	}
}

TAGHA_EXPORT int tagha_module_run(struct TaghaModule *const module, const size_t argc, const union TaghaVal argv[const])
{
	union TaghaVal res = {1};
	tagha_module_call(module, "main", argc, argv, &res);
	return res.int32;
}

static bool _tagha_module_start(struct TaghaModule *const module, const struct TaghaItem *const func, const size_t args, const union TaghaVal params[const], union TaghaVal *const restrict retval)
{
	if( func->flags & TAGHA_FLAG_NATIVE ) {
		if( func->flags & TAGHA_FLAG_LINKED ) {
			TaghaCFunc *const cfunc = ( TaghaCFunc* )(func->item);
			const union TaghaVal ret = (*cfunc)(module, params);
			if( retval != NULL ) {
				*retval = ret;
			}
			return module->err==TaghaErrNone;
		} else {
			module->err = TaghaErrBadNative;
			return false;
		}
	} else {
		const size_t bytes = sizeof(union TaghaVal) * (args + 1); /// one more for ret value.
		if( module->osp - bytes < module->opstack ) {
			module->err = TaghaErrOpStackOF;
			return false;
		} else {
			module->osp -= bytes;
			union TaghaVal *const restrict rsp = ( union TaghaVal* )(module->osp);
			memcpy(rsp + 1, params, bytes - sizeof(union TaghaVal));
			module->lr = NIL;
			_tagha_module_exec(module, func->item);
			if( retval != NULL ) {
				*retval = *( const union TaghaVal* )(module->osp);
			}
			module->osp += bytes;
			return module->err==TaghaErrNone;
		}
	}
}


static void _tagha_push_lr(struct TaghaModule *const vm)
{
	uintptr_t *const call_stack = ( uintptr_t* )(vm->csp);
	*call_stack = vm->lr;
	vm->csp += sizeof(uintptr_t);
}

static void _tagha_pop_lr(struct TaghaModule *const vm)
{
	vm->csp -= sizeof(uintptr_t);
	const uintptr_t *const call_stack = ( const uintptr_t* )(vm->csp);
	vm->lr = *call_stack;
}

static union TaghaVal read_opstack(const uintptr_t ptr, const int_fast8_t regid) {
	const union TaghaVal *const p = ( const union TaghaVal* )(ptr);
	return ( regid==0 )? ( union TaghaVal ){0} : p[regid];
}

static void write_opstack(const uintptr_t ptr, const int_fast8_t regid, const union TaghaVal val) {
	union TaghaVal *const restrict p = ( union TaghaVal* )(ptr);
	if( regid==0 ) {
		p[0] = p[0];
	} else {
		p[regid] = val;
	}
}


static void _tagha_module_exec(struct TaghaModule *const restrict vm, const uintptr_t ip)
{
	/// pc is restricted and must not access beyond the function table!
	union TaghaPtr pc = { ( const uint64_t* )(ip) };
	const uintptr_t mem_bnds_diff = vm->high_seg - vm->low_seg;
	
#define X(x) #x ,
	/// for debugging purposes.
	//const char *const op_to_cstr[] = { TAGHA_INSTR_SET };
#undef X
	
#define X(x) &&exec_##x ,
	/// our instruction dispatch table.
	static const void *const dispatch[] = { TAGHA_INSTR_SET };
#undef X
#undef TAGHA_INSTR_SET
	
#	define DBG_JMP \
		do { \
			const uint_fast8_t instr = *pc.uint8++; \
			if( instr>=MaxOps ) { \
				printf("instr : '%#x' '%u' | offset: %zu\n", instr, instr, ( uintptr_t )(pc.uint8) - vm->script); \
				vm->err = TaghaErrOpcodeOOB; \
				return; \
			} else { \
				/*sleep(1);*/ \
				printf("dispatching to '%s' | vm: %p\n", op_to_cstr[instr], vm); \
				goto *dispatch[instr]; \
			} \
		} while( 0 )
	
#	define GCC_JMP       goto *dispatch[*pc.uint8++]
	
#	define DISPATCH()    GCC_JMP
	
	/// nop being first will make sure our vm starts with a dispatch!
	exec_nop: { /// u8: opcode
		DISPATCH();
	}
	exec_enter: { /// u8: opcode | u8: cells
		_tagha_push_lr(vm);
		goto exec_alloc;
	}
	
	exec_pushlr: { /// u8: opcode
		_tagha_push_lr(vm);
		DISPATCH();
	}
	exec_poplr: { /// u8: opcode
		_tagha_pop_lr(vm);
		DISPATCH();
	}
	exec_restore: { /// u8: opcode
		_tagha_pop_lr(vm);
		goto exec_ret;
	}
	exec_leave: { /// u8: opcode | u8: cells
		_tagha_pop_lr(vm);
		goto exec_remit;
	}
	exec_ret: { /// u8: opcode
		pc.uint8 = ( const uint8_t* )(vm->lr);
		if( pc.uint8==NULL ) {
	exec_halt:
			return;
		} else {
			DISPATCH();
		}
	}
	exec_movi: { /// u8: opcode | u8: dest reg | u64: imm
		const uint_fast8_t regid = *pc.uint8++;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		rsp[regid] = *pc.val++;
		DISPATCH();
	}
	exec_mov: { /// u8: opcode | u8: dest reg | u8: src reg
		const uint_fast16_t instr = *pc.uint16++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		rsp[dst] = rsp[src];
		DISPATCH();
	}
	
	/// make room via opstack pointer.
	exec_alloc: { /// u8: opcode | u8: cells
		const uint_fast8_t cells = *pc.uint8++; /// allocs up to 8kb per stack frame.
		const size_t alloc_size  = sizeof(union TaghaVal) * cells;
		if( vm->osp - alloc_size < vm->opstack ) {
			vm->err = TaghaErrOpStackOF;    /// opstack overflow.
			return;
		} else {
			vm->osp -= alloc_size;
			DISPATCH();
		}
	}
	
	/// reduce opstack registers.
	exec_redux: { /// u8: opcode | u8: cells
		const uint_fast8_t cells  = *pc.uint8++;
		const size_t dealloc_size = sizeof(union TaghaVal) * cells;
		if( vm->osp + dealloc_size > vm->opstack + vm->opstack_size ) {
			vm->osp = vm->opstack + vm->opstack_size;
			DISPATCH();
		} else {
			vm->osp += dealloc_size;
			DISPATCH();
		}
	}
	
	exec_remit: { /// u8: opcode | u8: cells
		const uint_fast8_t cells  = *pc.uint8++;
		const size_t dealloc_size = sizeof(union TaghaVal) * cells;
		if( vm->osp + dealloc_size > vm->opstack + vm->opstack_size ) {
			vm->osp = vm->opstack + vm->opstack_size;
			goto exec_ret;
		} else {
			vm->osp += dealloc_size;
			goto exec_ret;
		}
	}
	
	exec_lra: { /// u8: opcode | u8: regid | u16: offset
		const uint_fast8_t  regid  = *pc.uint8++;
		const uint_fast16_t offset = *pc.uint16++;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		rsp[regid].uintptr = ( uintptr_t )(rsp + offset);
		DISPATCH();
	}
	exec_lea: { /// u8: opcode | u8: dest | u8: src | i16: offset
		const uint32_t instr = *pc.uint32++;
		const uint_fast8_t dst   = instr & 0xff;
		const uint_fast8_t src   = (instr & 0xffff) >> 8;
		const int32_t offset = ( int32_t )(instr) >> 16;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		rsp[dst].uintptr = rsp[src].uintptr + offset;
		DISPATCH();
	}
	
	/// load global variable.
	exec_ldvar: { /// u8: opcode | u8: regid | u16: index
		const uint_fast8_t  regid = *pc.uint8++;
		const uint_fast16_t index = *pc.uint16++;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		rsp[regid].uintptr = vm->vars->table[index].item;
		DISPATCH();
	}
	/// loads a function object.
	exec_ldfn: { /// u8: opcode | u8: regid | u16: index
		const uint_fast8_t  regid = *pc.uint8++;
		const uint_fast16_t index = *pc.uint16++;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		rsp[regid].uintptr = ( uintptr_t )(&vm->funcs->table[index]);
		DISPATCH();
	}
	
	exec_ld1: { /// u8: opcode | u8: dest reg | u8: src reg | i16: offset
		const uint_fast32_t instr = *pc.uint32++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = (instr & 0xffff) >> 8;
		const int32_t offset = ( int32_t )(instr) >> 16;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		const uintptr_t mem = rsp[src].uintptr + offset;
		if( (mem - vm->low_seg) > mem_bnds_diff ) {
			vm->err = TaghaErrBadPtr;
			return;
		} else {
			const int8_t *const ptr = ( const int8_t* )(mem);
			rsp[dst].int64 = *ptr;
			DISPATCH();
		}
	}
	exec_ld2: { /// u8: opcode | u8: dest reg | u8: src reg | i16: offset
		const uint_fast32_t instr = *pc.uint32++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = (instr & 0xffff) >> 8;
		const int32_t offset = ( int32_t )(instr) >> 16;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		const uintptr_t mem = rsp[src].uintptr + offset;
		if( (mem+1 - vm->low_seg) > mem_bnds_diff ) {
			vm->err = TaghaErrBadPtr;
			return;
		} else {
			const int16_t *const ptr = ( const int16_t* )(mem);
			rsp[dst].int64 = *ptr;
			DISPATCH();
		}
	}
	exec_ld4: { /// u8: opcode | u8: dest reg | u8: src reg | i16: offset
		const uint_fast32_t instr = *pc.uint32++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = (instr & 0xffff) >> 8;
		const int32_t offset = ( int32_t )(instr) >> 16;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		const uintptr_t mem = rsp[src].uintptr + offset;
		if( (mem+3 - vm->low_seg) > mem_bnds_diff ) {
			vm->err = TaghaErrBadPtr;
			return;
		} else {
			const int32_t *const ptr = ( const int32_t* )(mem);
			rsp[dst].int64 = *ptr;
			DISPATCH();
		}
	}
	exec_ld8: { /// u8: opcode | u8: dest reg | u8: src reg | i16: offset
		const uint_fast32_t instr = *pc.uint32++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = (instr & 0xffff) >> 8;
		const int32_t offset = ( int32_t )(instr) >> 16;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		const uintptr_t mem = rsp[src].uintptr + offset;
		if( (mem+7 - vm->low_seg) > mem_bnds_diff ) {
			vm->err = TaghaErrBadPtr;
			return;
		} else {
			const union TaghaVal *const ptr = ( const union TaghaVal* )(mem);
			rsp[dst] = *ptr;
			DISPATCH();
		}
	}
	
	exec_ldu1: { /// u8: opcode | u8: dest reg | u8: src reg | i16: offset
		const uint_fast32_t instr = *pc.uint32++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = (instr & 0xffff) >> 8;
		const int32_t offset = ( int32_t )(instr) >> 16;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		const uintptr_t mem = rsp[src].uintptr + offset;
		if( (mem - vm->low_seg) > mem_bnds_diff ) {
			vm->err = TaghaErrBadPtr;
			return;
		} else {
			const uint8_t *const ptr = ( const uint8_t* )(mem);
			rsp[dst].uint64 = *ptr;
			DISPATCH();
		}
	}
	exec_ldu2: { /// u8: opcode | u8: dest reg | u8: src reg | i16: offset
		const uint_fast32_t instr = *pc.uint32++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = (instr & 0xffff) >> 8;
		const int32_t offset = ( int32_t )(instr) >> 16;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		const uintptr_t mem = rsp[src].uintptr + offset;
		if( (mem+1 - vm->low_seg) > mem_bnds_diff ) {
			vm->err = TaghaErrBadPtr;
			return;
		} else {
			const uint16_t *const ptr = ( const uint16_t* )(mem);
			rsp[dst].uint64 = *ptr;
			DISPATCH();
		}
	}
	exec_ldu4: { /// u8: opcode | u8: dest reg | u8: src reg | i16: offset
		const uint_fast32_t instr = *pc.uint32++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = (instr & 0xffff) >> 8;
		const int32_t offset = ( int32_t )(instr) >> 16;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		const uintptr_t mem = rsp[src].uintptr + offset;
		if( (mem+3 - vm->low_seg) > mem_bnds_diff ) {
			vm->err = TaghaErrBadPtr;
			return;
		} else {
			const uint32_t *const ptr = ( const uint32_t* )(mem);
			rsp[dst].uint64 = *ptr;
			DISPATCH();
		}
	}
	
	exec_st1: { /// u8: opcode | u8: dest reg | u8: src reg | i16: offset
		const uint_fast32_t instr = *pc.uint32++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = (instr & 0xffff) >> 8;
		const int32_t offset = ( int32_t )(instr) >> 16;
		const union TaghaVal *const rsp = ( const union TaghaVal* )(vm->osp);
		const uintptr_t mem = rsp[dst].uintptr + offset;
		if( (mem - vm->low_seg) > mem_bnds_diff ) {
			vm->err = TaghaErrBadPtr;
			return;
		} else {
			uint8_t *const restrict ptr = ( uint8_t* )(mem);
			*ptr = rsp[src].uint8;
			DISPATCH();
		}
	}
	exec_st2: { /// u8: opcode | u8: dest reg | u8: src reg | i16: offset
		const uint_fast32_t instr = *pc.uint32++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = (instr & 0xffff) >> 8;
		const int32_t offset = ( int32_t )(instr) >> 16;
		const union TaghaVal *const rsp = ( const union TaghaVal* )(vm->osp);
		const uintptr_t mem = rsp[dst].uintptr + offset;
		if( (mem+1 - vm->low_seg) > mem_bnds_diff ) {
			vm->err = TaghaErrBadPtr;
			return;
		} else {
			uint16_t *const restrict ptr = ( uint16_t* )(mem);
			*ptr = rsp[src].uint16;
			DISPATCH();
		}
	}
	exec_st4: { /// u8: opcode | u8: dest reg | u8: src reg | i16: offset
		const uint_fast32_t instr = *pc.uint32++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = (instr & 0xffff) >> 8;
		const int32_t offset = ( int32_t )(instr) >> 16;
		const union TaghaVal *const rsp = ( const union TaghaVal* )(vm->osp);
		const uintptr_t mem = rsp[dst].uintptr + offset;
		if( (mem+3 - vm->low_seg) > mem_bnds_diff ) {
			vm->err = TaghaErrBadPtr;
			return;
		} else {
			uint32_t *const restrict ptr = ( uint32_t* )(mem);
			*ptr = rsp[src].uint32;
			DISPATCH();
		}
	}
	exec_st8: { /// u8: opcode | u8: dest reg | u8: src reg | i16: offset
		const uint_fast32_t instr = *pc.uint32++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = (instr & 0xffff) >> 8;
		const int32_t offset = ( int32_t )(instr) >> 16;
		const union TaghaVal *const rsp = ( const union TaghaVal* )(vm->osp);
		const uintptr_t mem = rsp[dst].uintptr + offset;
		if( (mem+7 - vm->low_seg) > mem_bnds_diff ) {
			vm->err = TaghaErrBadPtr;
			return;
		} else {
			union TaghaVal *const restrict ptr = ( union TaghaVal* )(mem);
			*ptr = rsp[src];
			DISPATCH();
		}
	}
	
	exec_add: { /// u8: opcode | u8: dest reg | u8: src reg
		const uint_fast16_t instr = *pc.uint16++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		rsp[dst].int64 += rsp[src].int64;
		DISPATCH();
	}
	
	exec_sub: { /// u8: opcode | u8: dest reg | u8: src reg
		const uint_fast16_t instr = *pc.uint16++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		rsp[dst].int64 -= rsp[src].int64;
		DISPATCH();
	}
	
	exec_mul: { /// u8: opcode | u8: dest reg | u8: src reg
		const uint_fast16_t instr = *pc.uint16++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		rsp[dst].int64 *= rsp[src].int64;
		DISPATCH();
	}
	
	exec_idiv: { /// u8: opcode | u8: dest reg | u8: src reg
		const uint_fast16_t instr = *pc.uint16++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		rsp[dst].uint64 /= rsp[src].uint64;
		DISPATCH();
	}
	
	exec_mod: { /// u8: opcode | u8: dest reg | u8: src reg
		const uint_fast16_t instr = *pc.uint16++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		rsp[dst].uint64 %= rsp[src].uint64;
		DISPATCH();
	}
	
	exec_neg: { /// u8: opcode | u8: regid
		const uint_fast8_t regid = *pc.uint8++;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		rsp[regid].int64 = -rsp[regid].int64;
		DISPATCH();
	}
	
	exec_fadd: { /// u8: opcode | u8: dest reg | u8: src reg
		const uint_fast16_t instr = *pc.uint16++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		
		/// if float64's are defined, regardless whether float32 is or not
#	if defined(TAGHA_FLOAT64_DEFINED)
		rsp[dst].float64 += rsp[src].float64;
#	elif defined(TAGHA_FLOAT32_DEFINED)
		rsp[dst].float32 += rsp[src].float32;
#	else
		( void )(instr); ( void )(dst); ( void )(src); ( void )(rsp);
#	endif
		DISPATCH();
	}
	
	exec_fsub: { /// u8: opcode | u8: dest reg | u8: src reg
		const uint_fast16_t instr = *pc.uint16++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
#	if defined(TAGHA_FLOAT64_DEFINED)
		rsp[dst].float64 -= rsp[src].float64;
#	elif defined(TAGHA_FLOAT32_DEFINED)
		rsp[dst].float32 -= rsp[src].float32;
#	else
		( void )(instr); ( void )(dst); ( void )(src); ( void )(rsp);
#	endif
		DISPATCH();
	}
	
	exec_fmul: { /// u8: opcode | u8: dest reg | u8: src reg
		const uint_fast16_t instr = *pc.uint16++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
#	if defined(TAGHA_FLOAT64_DEFINED)
		rsp[dst].float64 *= rsp[src].float64;
#	elif defined(TAGHA_FLOAT32_DEFINED)
		rsp[dst].float32 *= rsp[src].float32;
#	else
		( void )(instr); ( void )(dst); ( void )(src); ( void )(rsp);
#	endif
		DISPATCH();
	}
	
	exec_fdiv: { /// u8: opcode | u8: dest reg | u8: src reg
		const uint_fast16_t instr = *pc.uint16++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
#	if defined(TAGHA_FLOAT64_DEFINED)
		rsp[dst].float64 /= rsp[src].float64;
#	elif defined(TAGHA_FLOAT32_DEFINED)
		rsp[dst].float32 /= rsp[src].float32;
#	else
		( void )(instr); ( void )(dst); ( void )(src); ( void )(rsp);
#	endif
		DISPATCH();
	}
	
	exec_fneg: { /// u8: opcode | u8: regid
		const uint_fast8_t regid = *pc.uint8++;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
#	if defined(TAGHA_FLOAT64_DEFINED)
		const float64_t f = rsp[regid].float64;
		rsp[regid].float64 = -f;
#	elif defined(TAGHA_FLOAT32_DEFINED)
		const float32_t f = rsp[regid].float32;
		rsp[regid].float32 = -f;
#	else
		( void )(regid); ( void )(rsp);
#	endif
		DISPATCH();
	}
	
	exec__and: { /// u8: opcode | u8: dest reg | u8: src reg
		const uint_fast16_t instr = *pc.uint16++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		rsp[dst].uint64 &= rsp[src].uint64;
		DISPATCH();
	}
	
	exec__or: { /// u8: opcode | u8: dest reg | u8: src reg
		const uint_fast16_t instr = *pc.uint16++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		rsp[dst].uint64 |= rsp[src].uint64;
		DISPATCH();
	}
	
	exec__xor: { /// u8: opcode | u8: dest reg | u8: src reg
		const uint_fast16_t instr = *pc.uint16++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		rsp[dst].uint64 ^= rsp[src].uint64;
		DISPATCH();
	}
	
	exec_sll: { /// u8: opcode | u8: dest reg | u8: src reg
		const uint_fast16_t instr = *pc.uint16++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		rsp[dst].uint64 <<= rsp[src].uint64;
		DISPATCH();
	}
	
	exec_srl: { /// u8: opcode | u8: dest reg | u8: src reg
		const uint_fast16_t instr = *pc.uint16++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		rsp[dst].uint64 >>= rsp[src].uint64;
		DISPATCH();
	}
	
	exec_sra: { /// u8: opcode | u8: dest reg | u8: src reg
		const uint_fast16_t instr = *pc.uint16++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		rsp[dst].int64 >>= rsp[src].uint64;
		DISPATCH();
	}
	
	exec__not: { /// u8: opcode | u8: regid
		const uint_fast8_t regid = *pc.uint8++;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		rsp[regid].uint64 = ~rsp[regid].uint64;
		DISPATCH();
	}
	
	exec_cmp: { /// u8: opcode | u8: reg 1 | u8: reg 2
		const uint_fast16_t instr = *pc.uint16++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = instr >> 8;
		const union TaghaVal *const rsp = ( const union TaghaVal* )(vm->osp);
		vm->cond = rsp[dst].uint64 == rsp[src].uint64;
		DISPATCH();
	}
	
	exec_ilt: { /// u8: opcode | u8: reg 1 | u8: reg 2
		const uint_fast16_t instr = *pc.uint16++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = instr >> 8;
		const union TaghaVal *const rsp = ( const union TaghaVal* )(vm->osp);
		vm->cond = rsp[dst].int64 < rsp[src].int64;
		DISPATCH();
	}
	
	exec_ile: { /// u8: opcode | u8: reg 1 | u8: reg 2
		const uint_fast16_t instr = *pc.uint16++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = instr >> 8;
		const union TaghaVal *const rsp = ( const union TaghaVal* )(vm->osp);
		vm->cond = rsp[dst].int64 <= rsp[src].int64;
		DISPATCH();
	}
	
	exec_ult: { /// u8: opcode | u8: reg 1 | u8: reg 2
		const uint_fast16_t instr = *pc.uint16++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = instr >> 8;
		const union TaghaVal *const rsp = ( const union TaghaVal* )(vm->osp);
		vm->cond = rsp[dst].uint64 < rsp[src].uint64;
		DISPATCH();
	}
	
	exec_ule: { /// u8: opcode | u8: reg 1 | u8: reg 2
		const uint_fast16_t instr = *pc.uint16++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = instr >> 8;
		const union TaghaVal *const rsp = ( const union TaghaVal* )(vm->osp);
		vm->cond = rsp[dst].uint64 <= rsp[src].uint64;
		DISPATCH();
	}
	
	exec_flt: { /// u8: opcode | u8: reg 1 | u8: reg 2
		const uint_fast16_t instr = *pc.uint16++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = instr >> 8;
		const union TaghaVal *const rsp = ( const union TaghaVal* )(vm->osp);
#	if defined(TAGHA_FLOAT64_DEFINED)
		vm->cond = rsp[dst].float64 < rsp[src].float64;
#	elif defined(TAGHA_FLOAT32_DEFINED)
		vm->cond = rsp[dst].float32 < rsp[src].float32;
#	else
		( void )(instr); ( void )(dst); ( void )(src); ( void )(rsp);
#	endif
		DISPATCH();
	}
	
	exec_fle: { /// u8: opcode | u8: reg 1 | u8: reg 2
		const uint_fast16_t instr = *pc.uint16++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = instr >> 8;
		const union TaghaVal *const rsp = ( const union TaghaVal* )(vm->osp);
#	if defined(TAGHA_FLOAT64_DEFINED)
		vm->cond = rsp[dst].float64 <= rsp[src].float64;
#	elif defined(TAGHA_FLOAT32_DEFINED)
		vm->cond = rsp[dst].float32 <= rsp[src].float32;
#	else
		( void )(instr); ( void )(dst); ( void )(src); ( void )(rsp);
#	endif
		DISPATCH();
	}
	
	exec_setc: { /// u8: opcode | u8: reg id
		const uint_fast8_t regid = *pc.uint8++;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		rsp[regid].uint64 = vm->cond;
		DISPATCH();
	}
	
	exec_jmp: { /// u8: opcode | i32: offset
		const int_fast32_t offset = *pc.int32++;
		pc.uint8 += offset;
		DISPATCH();
	}
	exec_jz: { /// u8: opcode | i32: offset
		const int_fast32_t offset = *pc.int32++;
		if( !vm->cond ) {
			pc.uint8 += offset;
			DISPATCH();
		} else {
			DISPATCH();
		}
	}
	exec_jnz: { /// u8: opcode | i32: offset
		const int_fast32_t offset = *pc.int32++;
		if( vm->cond ) {
			pc.uint8 += offset;
			DISPATCH();
		} else {
			DISPATCH();
		}
	}
	
	exec_call: { /// u8: opcode | u16: index
		const uint_fast16_t index = *pc.uint16++;
		const struct TaghaItem *const func = &vm->funcs->table[index - 1];
		const uintptr_t     item  = func->item;
		const uintptr_t     owner = func->owner;
		const uint_fast32_t flags = func->flags;
		if( item==NIL || owner==NIL ) {
			vm->err = flags;
			return;
		} else if( flags & TAGHA_FLAG_NATIVE ) {
			TaghaCFunc *const cfunc = ( TaghaCFunc* )(item);
			union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
			*rsp = (*cfunc)(vm, rsp + 1);
			if( vm->err != TaghaErrNone ) {
				return;
			} else {
				DISPATCH();
			}
		} else if( flags & TAGHA_FLAG_EXTERN ) {
			const struct TaghaModule *const lib = ( const struct TaghaModule* )(owner);
			/// save old symbol tables.
			const uintptr_t
				saved_funcs = ( uintptr_t )(vm->funcs),
				saved_vars  = ( uintptr_t )(vm->vars)
			;
			vm->funcs = lib->funcs;
			vm->vars  = lib->vars;
			
			_tagha_push_lr(vm);
			vm->lr = NIL;
			_tagha_module_exec(vm, item);
			_tagha_pop_lr(vm);
			
			vm->funcs = ( const struct TaghaSymTable* )(saved_funcs);
			vm->vars  = ( const struct TaghaSymTable* )(saved_vars);
			DISPATCH();
		} else {
			vm->lr   = ( uintptr_t )(pc.uint8);
			pc.uint8 = ( const uint8_t* )(item);
			DISPATCH();
		}
	}
	exec_callr: { /// u8: opcode | u8: regid
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		const uint_fast8_t regid = *pc.uint8++;
		const struct TaghaItem *const func = ( const struct TaghaItem* )(rsp[regid].uintptr);
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
					TaghaCFunc *const cfunc = ( TaghaCFunc* )(item);
					*rsp = (*cfunc)(vm, rsp + 1);
					if( vm->err != TaghaErrNone ) {
						return;
					} else {
						DISPATCH();
					}
				}
			} else if( func->owner != ( uintptr_t )(vm) ) {
				/// if not same owner, it's an external function.
				if( func->owner==NIL ) {
					vm->err = TaghaErrBadExtern;
					return;
				} else {
					const struct TaghaModule *const lib = ( const struct TaghaModule* )(func->owner);
					/// save old symbol tables.
					const uintptr_t
						saved_funcs = ( uintptr_t )(vm->funcs),
						saved_vars  = ( uintptr_t )(vm->vars)
					;
					vm->funcs = lib->funcs;
					vm->vars  = lib->vars;
					
					_tagha_push_lr(vm);
					vm->lr = NIL;
					_tagha_module_exec(vm, item);
					_tagha_pop_lr(vm);
					
					vm->funcs = ( const struct TaghaSymTable* )(saved_funcs);
					vm->vars  = ( const struct TaghaSymTable* )(saved_vars);
					DISPATCH();
				}
			} else {
				vm->lr   = ( uintptr_t )(pc.uint8);
				pc.uint8 = ( const uint8_t* )(item);
				DISPATCH();
			}
		}
	}
	
	/// treated as nop if float32_t is defined but not the other.
	exec_f32tof64: { /// u8: opcode | u8: reg id
		const uint_fast8_t regid = *pc.uint8++;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
#	if defined(TAGHA_FLOAT32_DEFINED) && defined(TAGHA_FLOAT64_DEFINED)
		const float32_t f = rsp[regid].float32;
		rsp[regid].float64 = ( float64_t )(f);
#	else
		( void )(regid); ( void )(rsp);
#	endif
		DISPATCH();
	}
	exec_f64tof32: { /// u8: opcode | u8: reg id
		const uint_fast8_t regid = *pc.uint8++;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
#	if defined(TAGHA_FLOAT32_DEFINED) && defined(TAGHA_FLOAT64_DEFINED)
		const float64_t d  = rsp[regid].float64;
		rsp[regid].int64   = 0;
		rsp[regid].float32 = ( float32_t )(d);
#	else
		( void )(regid); ( void )(rsp);
#	endif
		DISPATCH();
	}
	exec_itof64: { /// u8: opcode | u8: reg id
		const uint_fast8_t regid = *pc.uint8++;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
#	ifdef TAGHA_FLOAT64_DEFINED
		const int64_t i    = rsp[regid].int64;
		rsp[regid].float64 = ( float64_t )(i);
#	else
		( void )(regid); ( void )(rsp);
#	endif
		DISPATCH();
	}
	exec_itof32: { /// u8: opcode | u8: reg id
		const uint_fast8_t regid = *pc.uint8++;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
#	ifdef TAGHA_FLOAT32_DEFINED
		const int64_t i    = rsp[regid].int64;
		rsp[regid].int64   = 0;
		rsp[regid].float32 = ( float32_t )(i);
#	else
		( void )(regid); ( void )(rsp);
#	endif
		DISPATCH();
	}
	exec_f64toi: { /// u8: opcode | u8: reg id
		const uint_fast8_t regid = *pc.uint8++;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
#	ifdef TAGHA_FLOAT64_DEFINED
		const float64_t i = rsp[regid].float64;
		rsp[regid].int64  = ( int64_t )(i);
#	else
		( void )(regid); ( void )(rsp);
#	endif
		DISPATCH();
	}
	exec_f32toi: { /// u8: opcode | u8: reg id
		const uint_fast8_t regid = *pc.uint8++;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
#	ifdef TAGHA_FLOAT32_DEFINED
		const float32_t i = rsp[regid].float32;
		rsp[regid].int64  = ( int64_t )(i);
#	else
		( void )(regid); ( void )(rsp);
#	endif
		DISPATCH();
	}
	
	/** Vector Extension */
	exec_setvlen: { /// u8: opcode | u16: vector width
		vm->vec_len = *pc.uint16++;
		DISPATCH();
	}
	exec_setelen: { /// u8: opcode | u8: element width
		vm->elem_len = *pc.uint8++;
		DISPATCH();
	}
	exec_vmov: { /// u8: opcode | u8: reg 1 | u8: reg 2
		const uint_fast16_t instr = *pc.uint16++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		memcpy(&rsp[dst], &rsp[src], vm->vec_len * vm->elem_len);
		DISPATCH();
	}
	exec_vadd: { /// u8: opcode | u8: reg 1 | u8: reg 2
		const uint_fast16_t instr = *pc.uint16++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		for( size_t i=0; i<vm->vec_len; i++ ) {
			if( vm->elem_len==sizeof(int8_t) ) {
				int8_t *const restrict r1 = ( int8_t* )(rsp + dst);
				const int8_t *const r2 = ( const int8_t* )(rsp + src);
				r1[i] += r2[i];
			} else if( vm->elem_len==sizeof(int16_t) ) {
				int16_t *const restrict r1 = ( int16_t* )(rsp + dst);
				const int16_t *const r2 = ( const int16_t* )(rsp + src);
				r1[i] += r2[i];
			} else if( vm->elem_len==sizeof(int32_t) ) {
				int32_t *const restrict r1 = ( int32_t* )(rsp + dst);
				const int32_t *const r2 = ( const int32_t* )(rsp + src);
				r1[i] += r2[i];
			} else {
				int64_t *const restrict r1 = ( int64_t* )(rsp + dst);
				const int64_t *const r2 = ( const int64_t* )(rsp + src);
				r1[i] += r2[i];
			}
		}
		DISPATCH();
	}
	exec_vsub: { /// u8: opcode | u8: reg 1 | u8: reg 2
		const uint_fast16_t instr = *pc.uint16++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		for( size_t i=0; i<vm->vec_len; i++ ) {
			if( vm->elem_len==sizeof(int8_t) ) {
				int8_t *const restrict r1 = ( int8_t* )(rsp + dst);
				const int8_t *const r2 = ( const int8_t* )(rsp + src);
				r1[i] -= r2[i];
			} else if( vm->elem_len==sizeof(int16_t) ) {
				int16_t *const restrict r1 = ( int16_t* )(rsp + dst);
				const int16_t *const r2 = ( const int16_t* )(rsp + src);
				r1[i] -= r2[i];
			} else if( vm->elem_len==sizeof(int32_t) ) {
				int32_t *const restrict r1 = ( int32_t* )(rsp + dst);
				const int32_t *const r2 = ( const int32_t* )(rsp + src);
				r1[i] -= r2[i];
			} else {
				int64_t *const restrict r1 = ( int64_t* )(rsp + dst);
				const int64_t *const r2 = ( const int64_t* )(rsp + src);
				r1[i] -= r2[i];
			}
		}
		DISPATCH();
	}
	exec_vmul: { /// u8: opcode | u8: reg 1 | u8: reg 2
		const uint_fast16_t instr = *pc.uint16++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		for( size_t i=0; i<vm->vec_len; i++ ) {
			if( vm->elem_len==sizeof(int8_t) ) {
				int8_t *const restrict r1 = ( int8_t* )(rsp + dst);
				const int8_t *const r2 = ( const int8_t* )(rsp + src);
				r1[i] *= r2[i];
			} else if( vm->elem_len==sizeof(int16_t) ) {
				int16_t *const restrict r1 = ( int16_t* )(rsp + dst);
				const int16_t *const r2 = ( const int16_t* )(rsp + src);
				r1[i] *= r2[i];
			} else if( vm->elem_len==sizeof(int32_t) ) {
				int32_t *const restrict r1 = ( int32_t* )(rsp + dst);
				const int32_t *const r2 = ( const int32_t* )(rsp + src);
				r1[i] *= r2[i];
			} else {
				int64_t *const restrict r1 = ( int64_t* )(rsp + dst);
				const int64_t *const r2 = ( const int64_t* )(rsp + src);
				r1[i] *= r2[i];
			}
		}
		DISPATCH();
	}
	exec_vdiv: { /// u8: opcode | u8: reg 1 | u8: reg 2
		const uint_fast16_t instr = *pc.uint16++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		for( size_t i=0; i<vm->vec_len; i++ ) {
			if( vm->elem_len==sizeof(uint8_t) ) {
				uint8_t *const restrict r1 = ( uint8_t* )(rsp + dst);
				const uint8_t *const r2 = ( const uint8_t* )(rsp + src);
				r1[i] /= r2[i];
			} else if( vm->elem_len==sizeof(uint16_t) ) {
				uint16_t *const restrict r1 = ( uint16_t* )(rsp + dst);
				const uint16_t *const r2 = ( const uint16_t* )(rsp + src);
				r1[i] /= r2[i];
			} else if( vm->elem_len==sizeof(uint32_t) ) {
				uint32_t *const restrict r1 = ( uint32_t* )(rsp + dst);
				const uint32_t *const r2 = ( const uint32_t* )(rsp + src);
				r1[i] /= r2[i];
			} else {
				uint64_t *const restrict r1 = ( uint64_t* )(rsp + dst);
				const uint64_t *const r2 = ( const uint64_t* )(rsp + src);
				r1[i] /= r2[i];
			}
		}
		DISPATCH();
	}
	exec_vmod: { /// u8: opcode | u8: reg 1 | u8: reg 2
		const uint_fast16_t instr = *pc.uint16++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		for( size_t i=0; i<vm->vec_len; i++ ) {
			if( vm->elem_len==sizeof(uint8_t) ) {
				uint8_t *const restrict r1 = ( uint8_t* )(rsp + dst);
				const uint8_t *const r2 = ( const uint8_t* )(rsp + src);
				r1[i] %= r2[i];
			} else if( vm->elem_len==sizeof(uint16_t) ) {
				uint16_t *const restrict r1 = ( uint16_t* )(rsp + dst);
				const uint16_t *const r2 = ( const uint16_t* )(rsp + src);
				r1[i] %= r2[i];
			} else if( vm->elem_len==sizeof(uint32_t) ) {
				uint32_t *const restrict r1 = ( uint32_t* )(rsp + dst);
				const uint32_t *const r2 = ( const uint32_t* )(rsp + src);
				r1[i] %= r2[i];
			} else {
				uint64_t *const restrict r1 = ( uint64_t* )(rsp + dst);
				const uint64_t *const r2 = ( const uint64_t* )(rsp + src);
				r1[i] %= r2[i];
			}
		}
		DISPATCH();
	}
	exec_vneg: { /// u8: opcode | u8: regid
		const uint_fast8_t regid = *pc.uint8++;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		for( size_t i=0; i<vm->vec_len; i++ ) {
			if( vm->elem_len==sizeof(uint8_t) ) {
				int8_t *const restrict r = ( int8_t* )(rsp + regid);
				r[i] = -r[i];
			} else if( vm->elem_len==sizeof(uint16_t) ) {
				int16_t *const restrict r = ( int16_t* )(rsp + regid);
				r[i] = -r[i];
			} else if( vm->elem_len==sizeof(uint32_t) ) {
				int32_t *const restrict r = ( int32_t* )(rsp + regid);
				r[i] = -r[i];
			} else {
				int64_t *const restrict r = ( int64_t* )(rsp + regid);
				r[i] = -r[i];
			}
		}
		DISPATCH();
	}
	exec_vfadd: { /// u8: opcode | u8: reg 1 | u8: reg 2
		const uint_fast16_t instr = *pc.uint16++;
#	if defined(TAGHA_FLOAT32_DEFINED) && defined(TAGHA_FLOAT64_DEFINED)
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		for( size_t i=0; i<vm->vec_len; i++ ) {
			if( vm->elem_len==sizeof(float32_t) ) {
				float32_t *const restrict r1 = ( float32_t* )(rsp + dst);
				const float32_t *const r2 = ( const float32_t* )(rsp + src);
				r1[i] += r2[i];
			} else {
				float64_t *const restrict r1 = ( float64_t* )(rsp + dst);
				const float64_t *const r2 = ( const float64_t* )(rsp + src);
				r1[i] += r2[i];
			}
		}
#else
		( void )(instr);
#endif
		DISPATCH();
	}
	exec_vfsub: { /// u8: opcode | u8: reg 1 | u8: reg 2
		const uint_fast16_t instr = *pc.uint16++;
#	if defined(TAGHA_FLOAT32_DEFINED) && defined(TAGHA_FLOAT64_DEFINED)
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		for( size_t i=0; i<vm->vec_len; i++ ) {
			if( vm->elem_len==sizeof(float32_t) ) {
				float32_t *const restrict r1 = ( float32_t* )(rsp + dst);
				const float32_t *const r2 = ( const float32_t* )(rsp + src);
				r1[i] -= r2[i];
			} else {
				float64_t *const restrict r1 = ( float64_t* )(rsp + dst);
				const float64_t *const r2 = ( const float64_t* )(rsp + src);
				r1[i] -= r2[i];
			}
		}
#else
		( void )(instr);
#endif
		DISPATCH();
	}
	exec_vfmul: { /// u8: opcode | u8: reg 1 | u8: reg 2
		const uint_fast16_t instr = *pc.uint16++;
#	if defined(TAGHA_FLOAT32_DEFINED) && defined(TAGHA_FLOAT64_DEFINED)
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		for( size_t i=0; i<vm->vec_len; i++ ) {
			if( vm->elem_len==sizeof(float32_t) ) {
				float32_t *const restrict r1 = ( float32_t* )(rsp + dst);
				const float32_t *const r2 = ( const float32_t* )(rsp + src);
				r1[i] *= r2[i];
			} else {
				float64_t *const restrict r1 = ( float64_t* )(rsp + dst);
				const float64_t *const r2 = ( const float64_t* )(rsp + src);
				r1[i] *= r2[i];
			}
		}
#else
		( void )(instr);
#endif
		DISPATCH();
	}
	exec_vfdiv: { /// u8: opcode | u8: reg 1 | u8: reg 2
		const uint_fast16_t instr = *pc.uint16++;
#	if defined(TAGHA_FLOAT32_DEFINED) && defined(TAGHA_FLOAT64_DEFINED)
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		for( size_t i=0; i<vm->vec_len; i++ ) {
			if( vm->elem_len==sizeof(float32_t) ) {
				float32_t *const restrict r1 = ( float32_t* )(rsp + dst);
				const float32_t *const r2 = ( const float32_t* )(rsp + src);
				r1[i] /= r2[i];
			} else {
				float64_t *const restrict r1 = ( float64_t* )(rsp + dst);
				const float64_t *const r2 = ( const float64_t* )(rsp + src);
				r1[i] /= r2[i];
			}
		}
#else
		( void )(instr);
#endif
		DISPATCH();
	}
	exec_vfneg: { /// u8: opcode | u8: regid
		const uint_fast8_t regid = *pc.uint8++;
#	if defined(TAGHA_FLOAT32_DEFINED) && defined(TAGHA_FLOAT64_DEFINED)
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		for( size_t i=0; i<vm->vec_len; i++ ) {
			if( vm->elem_len==sizeof(float32_t) ) {
				float32_t *const restrict r = ( float32_t* )(rsp + regid);
				r[i] = -r[i];
			} else {
				float64_t *const restrict r = ( float64_t* )(rsp + regid);
				r[i] = -r[i];
			}
		}
#else
		( void )(regid);
#endif
		DISPATCH();
	}
	exec_vand: { /// u8: opcode | u8: reg 1 | u8: reg 2
		const uint_fast16_t instr = *pc.uint16++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		for( size_t i=0; i<vm->vec_len; i++ ) {
			if( vm->elem_len==sizeof(uint8_t) ) {
				uint8_t *const restrict r1 = ( uint8_t* )(rsp + dst);
				const uint8_t *const r2 = ( const uint8_t* )(rsp + src);
				r1[i] &= r2[i];
			} else if( vm->elem_len==sizeof(uint16_t) ) {
				uint16_t *const restrict r1 = ( uint16_t* )(rsp + dst);
				const uint16_t *const r2 = ( const uint16_t* )(rsp + src);
				r1[i] &= r2[i];
			} else if( vm->elem_len==sizeof(uint32_t) ) {
				uint32_t *const restrict r1 = ( uint32_t* )(rsp + dst);
				const uint32_t *const r2 = ( const uint32_t* )(rsp + src);
				r1[i] &= r2[i];
			} else {
				uint64_t *const restrict r1 = ( uint64_t* )(rsp + dst);
				const uint64_t *const r2 = ( const uint64_t* )(rsp + src);
				r1[i] &= r2[i];
			}
		}
		DISPATCH();
	}
	exec_vor: { /// u8: opcode | u8: reg 1 | u8: reg 2
		const uint_fast16_t instr = *pc.uint16++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		for( size_t i=0; i<vm->vec_len; i++ ) {
			if( vm->elem_len==sizeof(uint8_t) ) {
				uint8_t *const restrict r1 = ( uint8_t* )(rsp + dst);
				const uint8_t *const r2 = ( const uint8_t* )(rsp + src);
				r1[i] |= r2[i];
			} else if( vm->elem_len==sizeof(uint16_t) ) {
				uint16_t *const restrict r1 = ( uint16_t* )(rsp + dst);
				const uint16_t *const r2 = ( const uint16_t* )(rsp + src);
				r1[i] |= r2[i];
			} else if( vm->elem_len==sizeof(uint32_t) ) {
				uint32_t *const restrict r1 = ( uint32_t* )(rsp + dst);
				const uint32_t *const r2 = ( const uint32_t* )(rsp + src);
				r1[i] |= r2[i];
			} else {
				uint64_t *const restrict r1 = ( uint64_t* )(rsp + dst);
				const uint64_t *const r2 = ( const uint64_t* )(rsp + src);
				r1[i] |= r2[i];
			}
		}
		DISPATCH();
	}
	exec_vxor: { /// u8: opcode | u8: reg 1 | u8: reg 2
		const uint_fast16_t instr = *pc.uint16++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		for( size_t i=0; i<vm->vec_len; i++ ) {
			if( vm->elem_len==sizeof(uint8_t) ) {
				uint8_t *const restrict r1 = ( uint8_t* )(rsp + dst);
				const uint8_t *const r2 = ( const uint8_t* )(rsp + src);
				r1[i] ^= r2[i];
			} else if( vm->elem_len==sizeof(uint16_t) ) {
				uint16_t *const restrict r1 = ( uint16_t* )(rsp + dst);
				const uint16_t *const r2 = ( const uint16_t* )(rsp + src);
				r1[i] ^= r2[i];
			} else if( vm->elem_len==sizeof(uint32_t) ) {
				uint32_t *const restrict r1 = ( uint32_t* )(rsp + dst);
				const uint32_t *const r2 = ( const uint32_t* )(rsp + src);
				r1[i] ^= r2[i];
			} else {
				uint64_t *const restrict r1 = ( uint64_t* )(rsp + dst);
				const uint64_t *const r2 = ( const uint64_t* )(rsp + src);
				r1[i] ^= r2[i];
			}
		}
		DISPATCH();
	}
	exec_vsll: { /// u8: opcode | u8: reg 1 | u8: reg 2
		const uint_fast16_t instr = *pc.uint16++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		for( size_t i=0; i<vm->vec_len; i++ ) {
			if( vm->elem_len==sizeof(uint8_t) ) {
				uint8_t *const restrict r1 = ( uint8_t* )(rsp + dst);
				const uint8_t *const r2 = ( const uint8_t* )(rsp + src);
				r1[i] <<= r2[i];
			} else if( vm->elem_len==sizeof(uint16_t) ) {
				uint16_t *const restrict r1 = ( uint16_t* )(rsp + dst);
				const uint16_t *const r2 = ( const uint16_t* )(rsp + src);
				r1[i] <<= r2[i];
			} else if( vm->elem_len==sizeof(uint32_t) ) {
				uint32_t *const restrict r1 = ( uint32_t* )(rsp + dst);
				const uint32_t *const r2 = ( const uint32_t* )(rsp + src);
				r1[i] <<= r2[i];
			} else {
				uint64_t *const restrict r1 = ( uint64_t* )(rsp + dst);
				const uint64_t *const r2 = ( const uint64_t* )(rsp + src);
				r1[i] <<= r2[i];
			}
		}
		DISPATCH();
	}
	exec_vsrl: { /// u8: opcode | u8: reg 1 | u8: reg 2
		const uint_fast16_t instr = *pc.uint16++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		for( size_t i=0; i<vm->vec_len; i++ ) {
			if( vm->elem_len==sizeof(uint8_t) ) {
				uint8_t *const restrict r1 = ( uint8_t* )(rsp + dst);
				const uint8_t *const r2 = ( const uint8_t* )(rsp + src);
				r1[i] >>= r2[i];
			} else if( vm->elem_len==sizeof(uint16_t) ) {
				uint16_t *const restrict r1 = ( uint16_t* )(rsp + dst);
				const uint16_t *const r2 = ( const uint16_t* )(rsp + src);
				r1[i] >>= r2[i];
			} else if( vm->elem_len==sizeof(uint32_t) ) {
				uint32_t *const restrict r1 = ( uint32_t* )(rsp + dst);
				const uint32_t *const r2 = ( const uint32_t* )(rsp + src);
				r1[i] >>= r2[i];
			} else {
				uint64_t *const restrict r1 = ( uint64_t* )(rsp + dst);
				const uint64_t *const r2 = ( const uint64_t* )(rsp + src);
				r1[i] >>= r2[i];
			}
		}
		DISPATCH();
	}
	exec_vsra: { /// u8: opcode | u8: reg 1 | u8: reg 2
		const uint_fast16_t instr = *pc.uint16++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = instr >> 8;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		for( size_t i=0; i<vm->vec_len; i++ ) {
			if( vm->elem_len==sizeof(uint8_t) ) {
				int8_t *const restrict r1 = ( int8_t* )(rsp + dst);
				const int8_t *const r2 = ( const int8_t* )(rsp + src);
				r1[i] >>= r2[i];
			} else if( vm->elem_len==sizeof(uint16_t) ) {
				int16_t *const restrict r1 = ( int16_t* )(rsp + dst);
				const int16_t *const r2 = ( const int16_t* )(rsp + src);
				r1[i] >>= r2[i];
			} else if( vm->elem_len==sizeof(uint32_t) ) {
				int32_t *const restrict r1 = ( int32_t* )(rsp + dst);
				const int32_t *const r2 = ( const int32_t* )(rsp + src);
				r1[i] >>= r2[i];
			} else {
				int64_t *const restrict r1 = ( int64_t* )(rsp + dst);
				const int64_t *const r2 = ( const int64_t* )(rsp + src);
				r1[i] >>= r2[i];
			}
		}
		DISPATCH();
	}
	exec_vnot: { /// u8: opcode | u8: regid
		const uint_fast8_t regid = *pc.uint8++;
		union TaghaVal *const restrict rsp = ( union TaghaVal* )(vm->osp);
		for( size_t i=0; i<vm->vec_len; i++ ) {
			if( vm->elem_len==sizeof(uint8_t) ) {
				uint8_t *const restrict r = ( uint8_t* )(rsp + regid);
				r[i] = ~r[i];
			} else if( vm->elem_len==sizeof(uint16_t) ) {
				uint16_t *const restrict r = ( uint16_t* )(rsp + regid);
				r[i] = ~r[i];
			} else if( vm->elem_len==sizeof(uint32_t) ) {
				uint32_t *const restrict r = ( uint32_t* )(rsp + regid);
				r[i] = ~r[i];
			} else {
				uint64_t *const restrict r = ( uint64_t* )(rsp + regid);
				r[i] = ~r[i];
			}
		}
		DISPATCH();
	}
	exec_vcmp: { /// u8: opcode | u8: reg 1 | u8: reg 2
		const uint_fast16_t instr = *pc.uint16++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = instr >> 8;
		const union TaghaVal *const rsp = ( const union TaghaVal* )(vm->osp);
		vm->cond = !memcmp(&rsp[dst], &rsp[src], vm->vec_len * vm->elem_len);
		DISPATCH();
	}
	exec_vilt: { /// u8: opcode | u8: reg 1 | u8: reg 2
		const uint_fast16_t instr = *pc.uint16++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = instr >> 8;
		const union TaghaVal *const rsp = ( const union TaghaVal* )(vm->osp);
		uint_fast16_t c = 0;
		for( size_t i=0; i<vm->vec_len; i++ ) {
			if( vm->elem_len==sizeof(uint8_t) ) {
				const int8_t *const r1 = ( const int8_t* )(rsp + dst);
				const int8_t *const r2 = ( const int8_t* )(rsp + src);
				c += r1[i] < r2[i];
			} else if( vm->elem_len==sizeof(uint16_t) ) {
				const int16_t *const r1 = ( const int16_t* )(rsp + dst);
				const int16_t *const r2 = ( const int16_t* )(rsp + src);
				c += r1[i] < r2[i];
			} else if( vm->elem_len==sizeof(uint32_t) ) {
				const int32_t *const r1 = ( const int32_t* )(rsp + dst);
				const int32_t *const r2 = ( const int32_t* )(rsp + src);
				c += r1[i] < r2[i];
			} else {
				const int64_t *const r1 = ( const int64_t* )(rsp + dst);
				const int64_t *const r2 = ( const int64_t* )(rsp + src);
				c += r1[i] < r2[i];
			}
		}
		vm->cond = c==vm->vec_len;
		DISPATCH();
	}
	exec_vile: { /// u8: opcode | u8: reg 1 | u8: reg 2
		const uint_fast16_t instr = *pc.uint16++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = instr >> 8;
		const union TaghaVal *const rsp = ( const union TaghaVal* )(vm->osp);
		uint_fast16_t c = 0;
		for( size_t i=0; i<vm->vec_len; i++ ) {
			if( vm->elem_len==sizeof(uint8_t) ) {
				const int8_t *const r1 = ( const int8_t* )(rsp + dst);
				const int8_t *const r2 = ( const int8_t* )(rsp + src);
				c += r1[i] <= r2[i];
			} else if( vm->elem_len==sizeof(uint16_t) ) {
				const int16_t *const r1 = ( const int16_t* )(rsp + dst);
				const int16_t *const r2 = ( const int16_t* )(rsp + src);
				c += r1[i] <= r2[i];
			} else if( vm->elem_len==sizeof(uint32_t) ) {
				const int32_t *const r1 = ( const int32_t* )(rsp + dst);
				const int32_t *const r2 = ( const int32_t* )(rsp + src);
				c += r1[i] <= r2[i];
			} else {
				const int64_t *const r1 = ( const int64_t* )(rsp + dst);
				const int64_t *const r2 = ( const int64_t* )(rsp + src);
				c += r1[i] <= r2[i];
			}
		}
		vm->cond = c==vm->vec_len;
		DISPATCH();
	}
	exec_vult: { /// u8: opcode | u8: reg 1 | u8: reg 2
		const uint_fast16_t instr = *pc.uint16++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = instr >> 8;
		const union TaghaVal *const rsp = ( const union TaghaVal* )(vm->osp);
		uint_fast16_t c = 0;
		for( size_t i=0; i<vm->vec_len; i++ ) {
			if( vm->elem_len==sizeof(uint8_t) ) {
				const uint8_t *const r1 = ( const uint8_t* )(rsp + dst);
				const uint8_t *const r2 = ( const uint8_t* )(rsp + src);
				c += r1[i] < r2[i];
			} else if( vm->elem_len==sizeof(uint16_t) ) {
				const uint16_t *const r1 = ( const uint16_t* )(rsp + dst);
				const uint16_t *const r2 = ( const uint16_t* )(rsp + src);
				c += r1[i] < r2[i];
			} else if( vm->elem_len==sizeof(uint32_t) ) {
				const uint32_t *const r1 = ( const uint32_t* )(rsp + dst);
				const uint32_t *const r2 = ( const uint32_t* )(rsp + src);
				c += r1[i] < r2[i];
			} else {
				const uint64_t *const r1 = ( const uint64_t* )(rsp + dst);
				const uint64_t *const r2 = ( const uint64_t* )(rsp + src);
				c += r1[i] < r2[i];
			}
		}
		vm->cond = c==vm->vec_len;
		DISPATCH();
	}
	exec_vule: { /// u8: opcode | u8: reg 1 | u8: reg 2
		const uint_fast16_t instr = *pc.uint16++;
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = instr >> 8;
		const union TaghaVal *const rsp = ( const union TaghaVal* )(vm->osp);
		uint_fast16_t c = 0;
		for( size_t i=0; i<vm->vec_len; i++ ) {
			if( vm->elem_len==sizeof(uint8_t) ) {
				const uint8_t *const r1 = ( const uint8_t* )(rsp + dst);
				const uint8_t *const r2 = ( const uint8_t* )(rsp + src);
				c += r1[i] <= r2[i];
			} else if( vm->elem_len==sizeof(uint16_t) ) {
				const uint16_t *const r1 = ( const uint16_t* )(rsp + dst);
				const uint16_t *const r2 = ( const uint16_t* )(rsp + src);
				c += r1[i] <= r2[i];
			} else if( vm->elem_len==sizeof(uint32_t) ) {
				const uint32_t *const r1 = ( const uint32_t* )(rsp + dst);
				const uint32_t *const r2 = ( const uint32_t* )(rsp + src);
				c += r1[i] <= r2[i];
			} else {
				const uint64_t *const r1 = ( const uint64_t* )(rsp + dst);
				const uint64_t *const r2 = ( const uint64_t* )(rsp + src);
				c += r1[i] <= r2[i];
			}
		}
		vm->cond = c==vm->vec_len;
		DISPATCH();
	}
	exec_vflt: { /// u8: opcode | u8: reg 1 | u8: reg 2
		const uint_fast16_t instr = *pc.uint16++;
#	if defined(TAGHA_FLOAT32_DEFINED) && defined(TAGHA_FLOAT64_DEFINED)
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = instr >> 8;
		const union TaghaVal *const rsp = ( const union TaghaVal* )(vm->osp);
		uint_fast16_t c = 0;
		for( size_t i=0; i<vm->vec_len; i++ ) {
			if( vm->elem_len==sizeof(float32_t) ) {
				const float32_t *const r1 = ( const float32_t* )(rsp + dst);
				const float32_t *const r2 = ( const float32_t* )(rsp + src);
				c += r1[i] < r2[i];
			} else {
				const float64_t *const r1 = ( const float64_t* )(rsp + dst);
				const float64_t *const r2 = ( const float64_t* )(rsp + src);
				c += r1[i] < r2[i];
			}
		}
		vm->cond = c==vm->vec_len;
#else
		( void )(instr);
#endif
		DISPATCH();
	}
	exec_vfle: { /// u8: opcode | u8: reg 1 | u8: reg 2
		const uint_fast16_t instr = *pc.uint16++;
#	if defined(TAGHA_FLOAT32_DEFINED) && defined(TAGHA_FLOAT64_DEFINED)
		const uint_fast8_t  dst   = instr & 0xff;
		const uint_fast8_t  src   = instr >> 8;
		const union TaghaVal *const rsp = ( const union TaghaVal* )(vm->osp);
		uint_fast16_t c = 0;
		for( size_t i=0; i<vm->vec_len; i++ ) {
			if( vm->elem_len==sizeof(float32_t) ) {
				const float32_t *const r1 = ( const float32_t* )(rsp + dst);
				const float32_t *const r2 = ( const float32_t* )(rsp + src);
				c += r1[i] <= r2[i];
			} else {
				const float64_t *const r1 = ( const float64_t* )(rsp + dst);
				const float64_t *const r2 = ( const float64_t* )(rsp + src);
				c += r1[i] <= r2[i];
			}
		}
		vm->cond = c==vm->vec_len;
#else
		( void )(instr);
#endif
		DISPATCH();
	}
}