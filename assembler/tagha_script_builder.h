#pragma once

#include "libharbol/harbol.h"
#include "../tagha/tagha.h"

typedef struct TaghaScriptBuilder {
	struct HarbolByteBuf header, datatbl, functbl;
	uint32_t funcs, vars, memsize;
} STaghaScriptBuilder;

static inline struct TaghaScriptBuilder tagha_tbc_gen_create(void)
{
	struct TaghaScriptBuilder tbc = { harbol_bytebuffer_create(), harbol_bytebuffer_create(), harbol_bytebuffer_create(), 0,0,0 };
	return tbc;
}

static inline NO_NULL void tagha_tbc_gen_write_header(struct TaghaScriptBuilder *const tbc, const uint32_t stack_size, const uint32_t mem_size, const uint8_t flags)
{
	harbol_bytebuffer_insert_int16(&tbc->header, TAGHA_MAGIC_VERIFIER);
	harbol_bytebuffer_insert_int32(&tbc->header, stack_size);
	tbc->memsize = mem_size;
	harbol_bytebuffer_insert_int32(&tbc->header, mem_size);
	harbol_bytebuffer_insert_byte(&tbc->header, flags);
}


static inline NEVER_NULL(1,3) void tagha_tbc_gen_write_func(struct TaghaScriptBuilder *const restrict tbc, const bool is_native, const char name[restrict static 1], const struct HarbolByteBuf *const restrict bytecode)
{
	// write flag
	harbol_bytebuffer_insert_byte(&tbc->functbl, is_native);
	
	// write strlen
	harbol_bytebuffer_insert_int32(&tbc->functbl, (uint32_t)strlen(name) + 1);
	
	// write instrlen
	( is_native ) ?
		harbol_bytebuffer_insert_int32(&tbc->functbl, 8) :
		harbol_bytebuffer_insert_int32(&tbc->functbl, (uint32_t)bytecode->count);
	
	// write string of func.
	harbol_bytebuffer_insert_cstr(&tbc->functbl, name);
	
	// write bytecode.
	if( !is_native )
		harbol_bytebuffer_append(&tbc->functbl, bytecode);
	
	tbc->funcs++;
}

static inline NEVER_NULL(1,3,4) void tagha_tbc_gen_write_var(struct TaghaScriptBuilder *const restrict tbc, const uint8_t flags, const char name[restrict static 1], const struct HarbolByteBuf *const restrict datum)
{
	// write flag
	harbol_bytebuffer_insert_byte(&tbc->datatbl, flags);
	
	// write strlen
	harbol_bytebuffer_insert_int32(&tbc->datatbl, (uint32_t)strlen(name) + 1);
	
	// write var data size
	harbol_bytebuffer_insert_int32(&tbc->datatbl, (uint32_t)datum->count);
	
	// write string of var name.
	harbol_bytebuffer_insert_cstr(&tbc->datatbl, name);
	
	// write var data.
	harbol_bytebuffer_append(&tbc->datatbl, datum);
	
	tbc->vars++;
}


static inline NO_NULL void tagha_tbc_gen_create_file(struct TaghaScriptBuilder *const restrict tbc, const char filename[restrict static 1])
{
	FILE *restrict tbcfile = fopen(filename, "w+");
	if( tbcfile==NULL )
		goto null_file_err;
	
	struct HarbolByteBuf final_tbc = harbol_bytebuffer_create();
	harbol_bytebuffer_append(&final_tbc, &tbc->header);
	
	// build func table.
	harbol_bytebuffer_insert_int32(&final_tbc, tbc->funcs);
	harbol_bytebuffer_append(&final_tbc, &tbc->functbl);
	
	// build var table
	harbol_bytebuffer_insert_int32(&final_tbc, tbc->vars);
	harbol_bytebuffer_append(&final_tbc, &tbc->datatbl);
	
	// build memory region.
	harbol_bytebuffer_insert_zeros(&final_tbc, tbc->memsize);
	
	// finally generate the tbc file binary and then free data.
	harbol_bytebuffer_to_file(&final_tbc, tbcfile);
	fclose(tbcfile), tbcfile=NULL;
	harbol_bytebuffer_clear(&final_tbc);
	
null_file_err:
	harbol_bytebuffer_clear(&tbc->header);
	harbol_bytebuffer_clear(&tbc->functbl);
	harbol_bytebuffer_clear(&tbc->datatbl);
}
