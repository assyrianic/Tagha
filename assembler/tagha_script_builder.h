#pragma once

#include "libharbol/harbol.h"
#include "../tagha/tagha.h"

typedef struct TaghaScriptBuilder {
	struct TaghaHeader hdr;
	struct HarbolByteBuf var_table, func_table;
} STaghaScriptBuilder;


static inline struct TaghaScriptBuilder tagha_tbc_gen_create(void)
{
	struct TaghaScriptBuilder tbc = { {0}, harbol_bytebuffer_create(), harbol_bytebuffer_create() };
	tbc.hdr.magic = TAGHA_MAGIC_VERIFIER;
	return tbc;
}

static inline NO_NULL void tagha_tbc_gen_write_header(struct TaghaScriptBuilder *const tbc, const uint32_t stack_size, const uint32_t heap_size, const uint32_t mem_size, const uint32_t flags)
{
	tbc->hdr.memsize = mem_size;
	tbc->hdr.stacksize = stack_size;
	tbc->hdr.heapsize = heap_size;
	tbc->hdr.flags = flags;
}


static inline NEVER_NULL(1,3) void tagha_tbc_gen_write_func(struct TaghaScriptBuilder *const restrict tbc, const uint32_t flags, const char *restrict name, const struct HarbolByteBuf *const restrict bytecode)
{
	uint32_t entry_size = sizeof(struct TaghaItemEntry);
	const uint32_t name_len = ( uint32_t )strlen(name) + 1;
	const uint32_t name_len_diff = ( uint32_t )harbol_align_size(name_len, 4) - name_len;
	const uint32_t data_len = ( uint32_t )bytecode->count;
	const uint32_t data_len_diff = ( uint32_t )harbol_align_size(data_len, 4) - data_len;
	entry_size += name_len + name_len_diff;
	entry_size += ( flags != 0 ) ? 8 : data_len + data_len_diff;
	
	/// entry size
	harbol_bytebuffer_insert_int32(&tbc->func_table, entry_size);
	
	/// write flag
	harbol_bytebuffer_insert_int32(&tbc->func_table, flags);
	
	/// write strlen
	harbol_bytebuffer_insert_int32(&tbc->func_table, name_len + name_len_diff);
	
	/// write instrlen
	( flags != 0 ) ?
		harbol_bytebuffer_insert_int32(&tbc->func_table, 8) :
		harbol_bytebuffer_insert_int32(&tbc->func_table, data_len + data_len_diff);
	
	/// write string of func.
	harbol_bytebuffer_insert_cstr(&tbc->func_table, name);
	harbol_bytebuffer_insert_zeros(&tbc->func_table, name_len_diff);
	
	/// write bytecode.
	if( !flags ) {
		harbol_bytebuffer_append(&tbc->func_table, bytecode);
		harbol_bytebuffer_insert_zeros(&tbc->func_table, data_len_diff);
	}
	tbc->hdr.func_count++;
}

static inline NEVER_NULL(1,3,4) void tagha_tbc_gen_write_var(struct TaghaScriptBuilder *const restrict tbc, const uint32_t flags, const char *restrict name, const struct HarbolByteBuf *const restrict datum)
{
	uint32_t entry_size = sizeof(struct TaghaItemEntry);
	const uint32_t name_len = ( uint32_t )strlen(name) + 1;
	const uint32_t data_len = ( uint32_t )datum->count;
	const uint32_t name_len_diff = ( uint32_t )harbol_align_size(name_len, 4) - name_len;
	const uint32_t data_len_diff = ( uint32_t )harbol_align_size(data_len, 4) - data_len;
	entry_size += name_len + data_len + name_len_diff + data_len_diff;
	
	/// entry size
	harbol_bytebuffer_insert_int32(&tbc->var_table, entry_size);
	
	/// write flag
	harbol_bytebuffer_insert_int32(&tbc->var_table, flags);
	
	/// write strlen
	harbol_bytebuffer_insert_int32(&tbc->var_table, name_len + name_len_diff);
	
	/// write var data size
	harbol_bytebuffer_insert_int32(&tbc->var_table, data_len + data_len_diff);
	
	/// write string of var name.
	harbol_bytebuffer_insert_cstr(&tbc->var_table, name);
	harbol_bytebuffer_insert_zeros(&tbc->var_table, name_len_diff);
	
	/// write var data.
	harbol_bytebuffer_append(&tbc->var_table, datum);
	harbol_bytebuffer_insert_zeros(&tbc->var_table, data_len_diff);
	
	tbc->hdr.var_count++;
}

static inline NO_NULL struct HarbolByteBuf tagha_tbc_gen_to_buffer(struct TaghaScriptBuilder *const tbc)
{
	struct HarbolByteBuf final_tbc = harbol_bytebuffer_create();
	tbc->hdr.funcs_offset = sizeof tbc->hdr;
	tbc->hdr.vars_offset  = tbc->hdr.funcs_offset + tbc->func_table.count;
	tbc->hdr.mem_offset   = tbc->hdr.vars_offset  + tbc->var_table.count;
	
	harbol_bytebuffer_insert_obj(&final_tbc, &tbc->hdr, sizeof tbc->hdr);
	
	/// build func table & var table.
	harbol_bytebuffer_append(&final_tbc, &tbc->func_table);
	harbol_bytebuffer_append(&final_tbc, &tbc->var_table);
	
	/// build memory region.
	harbol_bytebuffer_insert_zeros(&final_tbc, tbc->hdr.memsize);
	
	harbol_bytebuffer_clear(&tbc->func_table);
	harbol_bytebuffer_clear(&tbc->var_table);
	return final_tbc;
}

static inline NO_NULL bool tagha_tbc_gen_create_file(struct TaghaScriptBuilder *const restrict tbc, const char *restrict filename)
{
	bool res = false;
	FILE *restrict tbcfile = fopen(filename, "w+");
	if( tbcfile==NULL )
		goto null_file_err;
	
	struct HarbolByteBuf final_tbc = tagha_tbc_gen_to_buffer(tbc);
	
	/// finally generate the tbc file binary and then free data.
	res = harbol_bytebuffer_to_file(&final_tbc, tbcfile);
	fclose(tbcfile), tbcfile=NULL;
	harbol_bytebuffer_clear(&final_tbc);
	
null_file_err:
	return res;
}
