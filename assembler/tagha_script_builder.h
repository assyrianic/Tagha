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

static inline NO_NULL void tagha_tbc_gen_write_header(struct TaghaScriptBuilder *const tbc, const uint32_t stack_size, const uint32_t mem_size, const uint32_t flags)
{
	harbol_bytebuffer_insert_int32(&tbc->header, TAGHA_MAGIC_VERIFIER);
	harbol_bytebuffer_insert_int32(&tbc->header, stack_size);
	tbc->memsize = mem_size;
	harbol_bytebuffer_insert_int32(&tbc->header, mem_size);
	harbol_bytebuffer_insert_int32(&tbc->header, flags);
}


static inline NEVER_NULL(1,3) void tagha_tbc_gen_write_func(struct TaghaScriptBuilder *const restrict tbc, const bool is_native, const char name[restrict static 1], const struct HarbolByteBuf *const restrict bytecode)
{
	uint32_t entry_size = sizeof(struct TaghaItemEntry);
	uint32_t name_len = (uint32_t)strlen(name) + 1;
	uint32_t name_len_diff = (uint32_t)harbol_align_size(name_len, 4) - name_len;
	uint32_t data_len = (uint32_t)bytecode->count;
	uint32_t data_len_diff = (uint32_t)harbol_align_size(data_len, 4) - data_len;
	entry_size += name_len + name_len_diff;
	entry_size += ( is_native ) ? 8 : data_len + data_len_diff;
	
	/// entry size
	harbol_bytebuffer_insert_int32(&tbc->functbl, entry_size);
	
	/// write flag
	harbol_bytebuffer_insert_int32(&tbc->functbl, is_native);
	
	/// write strlen
	harbol_bytebuffer_insert_int32(&tbc->functbl, name_len + name_len_diff);
	
	/// write instrlen
	( is_native ) ?
		harbol_bytebuffer_insert_int32(&tbc->functbl, 8) :
		harbol_bytebuffer_insert_int32(&tbc->functbl, data_len + data_len_diff);
	
	/// write string of func.
	harbol_bytebuffer_insert_cstr(&tbc->functbl, name);
	harbol_bytebuffer_insert_zeros(&tbc->functbl, name_len_diff);
	
	/// write bytecode.
	if( !is_native ) {
		harbol_bytebuffer_append(&tbc->functbl, bytecode);
		harbol_bytebuffer_insert_zeros(&tbc->functbl, data_len_diff);
	}
	tbc->funcs++;
}

static inline NEVER_NULL(1,3,4) void tagha_tbc_gen_write_var(struct TaghaScriptBuilder *const restrict tbc, const uint32_t flags, const char name[restrict static 1], const struct HarbolByteBuf *const restrict datum)
{
	uint32_t entry_size = sizeof(struct TaghaItemEntry);
	uint32_t name_len = (uint32_t)strlen(name) + 1;
	uint32_t data_len = (uint32_t)datum->count;
	uint32_t name_len_diff = (uint32_t)harbol_align_size(name_len, 4) - name_len;
	uint32_t data_len_diff = (uint32_t)harbol_align_size(data_len, 4) - data_len;
	entry_size += name_len + data_len + name_len_diff + data_len_diff;
	
	/// entry size
	harbol_bytebuffer_insert_int32(&tbc->datatbl, entry_size);
	
	/// write flag
	harbol_bytebuffer_insert_int32(&tbc->datatbl, flags);
	
	/// write strlen
	harbol_bytebuffer_insert_int32(&tbc->datatbl, name_len + name_len_diff);
	
	/// write var data size
	harbol_bytebuffer_insert_int32(&tbc->datatbl, data_len + data_len_diff);
	
	/// write string of var name.
	harbol_bytebuffer_insert_cstr(&tbc->datatbl, name);
	harbol_bytebuffer_insert_zeros(&tbc->datatbl, name_len_diff);
	
	/// write var data.
	harbol_bytebuffer_append(&tbc->datatbl, datum);
	harbol_bytebuffer_insert_zeros(&tbc->datatbl, data_len_diff);
	
	tbc->vars++;
}


static inline NO_NULL void tagha_tbc_gen_create_file(struct TaghaScriptBuilder *const restrict tbc, const char filename[restrict static 1])
{
	FILE *restrict tbcfile = fopen(filename, "w+");
	if( tbcfile==NULL )
		goto null_file_err;
	
	struct HarbolByteBuf final_tbc = harbol_bytebuffer_create();
	harbol_bytebuffer_append(&final_tbc, &tbc->header);
	
	/// build func table.
	harbol_bytebuffer_insert_int32(&final_tbc, tbc->funcs);
	harbol_bytebuffer_append(&final_tbc, &tbc->functbl);
	
	/// build var table
	harbol_bytebuffer_insert_int32(&final_tbc, tbc->vars);
	harbol_bytebuffer_append(&final_tbc, &tbc->datatbl);
	
	/// build memory region.
	harbol_bytebuffer_insert_zeros(&final_tbc, tbc->memsize);
	
	/// finally generate the tbc file binary and then free data.
	harbol_bytebuffer_to_file(&final_tbc, tbcfile);
	fclose(tbcfile), tbcfile=NULL;
	harbol_bytebuffer_clear(&final_tbc);
	
null_file_err:
	harbol_bytebuffer_clear(&tbc->header);
	harbol_bytebuffer_clear(&tbc->functbl);
	harbol_bytebuffer_clear(&tbc->datatbl);
}
