#ifndef TAGHA_MODULE_GEN
#	define TAGHA_MODULE_GEN

#ifdef __cplusplus
extern "C" {
#endif

#include "../tagha/tagha.h"
#include "libharbol/harbol.h"


struct TaghaModGen {
	struct TaghaModuleHeader hdr;
	struct HarbolByteBuf var_data, func_data;
};


static inline struct TaghaModGen tagha_mod_gen_create(void)
{
	struct TaghaModGen module = {0};
	module.func_data = harbol_bytebuffer_create();
	module.var_data  = harbol_bytebuffer_create();
	module.hdr.magic = TAGHA_MAGIC_VERIFIER;
	return module;
}

static inline NO_NULL void tagha_mod_gen_write_header(struct TaghaModGen *const mod, const uint32_t opstacksize, const uint32_t callstacksize, const uint32_t heapsize, const uint32_t flags)
{
	mod->hdr.opstacksize = opstacksize;
	mod->hdr.callstacksize = callstacksize;
	mod->hdr.stacksize = opstacksize + callstacksize;
	mod->hdr.heapsize = heapsize;
	mod->hdr.memsize = opstacksize + callstacksize + heapsize;
	mod->hdr.flags = flags;
}

static inline NEVER_NULL(1,3) void tagha_mod_gen_write_func(struct TaghaModGen *const restrict mod, const uint32_t flags, const char name[restrict static 1], const struct HarbolByteBuf *const restrict bytecode)
{
	uint32_t entry_size = sizeof(struct TaghaItemEntry);
	const uint32_t name_len = ( uint32_t )strlen(name) + 1;
	const uint32_t name_len_diff = ( uint32_t )harbol_align_size(name_len, 4) - name_len;
	const uint32_t data_len = ( uint32_t )bytecode->count;
	const uint32_t data_len_diff = ( uint32_t )harbol_align_size(data_len, 4) - data_len;
	entry_size += name_len + name_len_diff;
	entry_size += ( flags != 0 ) ? 8 : data_len + data_len_diff;
	
	/// entry size
	harbol_bytebuffer_insert_int32(&mod->func_data, entry_size);
	
	/// write flag
	harbol_bytebuffer_insert_int32(&mod->func_data, flags);
	
	/// write strlen
	harbol_bytebuffer_insert_int32(&mod->func_data, name_len + name_len_diff);
	
	/// write instrlen
	( flags != 0 ) ?
		harbol_bytebuffer_insert_int32(&mod->func_data, 8) :
		harbol_bytebuffer_insert_int32(&mod->func_data, data_len + data_len_diff);
	
	/// write string of func.
	harbol_bytebuffer_insert_cstr(&mod->func_data, name);
	harbol_bytebuffer_insert_zeros(&mod->func_data, name_len_diff);
	
	/// write bytecode.
	if( !flags ) {
		harbol_bytebuffer_append(&mod->func_data, bytecode);
		harbol_bytebuffer_insert_zeros(&mod->func_data, data_len_diff);
	}
	mod->hdr.func_count++;
}

static inline NEVER_NULL(1,3,4) void tagha_mod_gen_write_var(struct TaghaModGen *const restrict mod, const uint32_t flags, const char name[restrict static 1], const struct HarbolByteBuf *const restrict datum)
{
	uint32_t entry_size = sizeof(struct TaghaItemEntry);
	const uint32_t name_len = ( uint32_t )strlen(name) + 1;
	const uint32_t data_len = ( uint32_t )datum->count;
	const uint32_t name_len_diff = ( uint32_t )harbol_align_size(name_len, 4) - name_len;
	const uint32_t data_len_diff = ( uint32_t )harbol_align_size(data_len, 4) - data_len;
	entry_size += name_len + data_len + name_len_diff + data_len_diff;
	
	/// entry size
	harbol_bytebuffer_insert_int32(&mod->var_data, entry_size);
	
	/// write flag
	harbol_bytebuffer_insert_int32(&mod->var_data, flags);
	
	/// write strlen
	harbol_bytebuffer_insert_int32(&mod->var_data, name_len + name_len_diff);
	
	/// write var data size
	harbol_bytebuffer_insert_int32(&mod->var_data, data_len + data_len_diff);
	
	/// write string of var name.
	harbol_bytebuffer_insert_cstr(&mod->var_data, name);
	harbol_bytebuffer_insert_zeros(&mod->var_data, name_len_diff);
	
	/// write var data.
	harbol_bytebuffer_append(&mod->var_data, datum);
	harbol_bytebuffer_insert_zeros(&mod->var_data, data_len_diff);
	
	mod->hdr.var_count++;
}

static inline NO_NULL struct HarbolByteBuf __tagha_mod_gen_finalize(struct TaghaModGen *const mod)
{
	struct HarbolByteBuf final_tbc = harbol_bytebuffer_create();
	
	/// calculate offsets.
	mod->hdr.funcs_offset = sizeof mod->hdr;
	mod->hdr.vars_offset  = mod->hdr.funcs_offset + mod->func_data.count;
	mod->hdr.mem_offset   = mod->hdr.vars_offset  + mod->var_data.count;
	
	/// add the header.
	harbol_bytebuffer_insert_obj(&final_tbc, &mod->hdr, sizeof mod->hdr);
	
	/// build func table & var table.
	harbol_bytebuffer_append(&final_tbc, &mod->func_data);
	harbol_bytebuffer_append(&final_tbc, &mod->var_data);
	
	/// build memory region.
	harbol_bytebuffer_insert_zeros(&final_tbc, mod->hdr.memsize);
	
	/// free data.
	harbol_bytebuffer_clear(&mod->func_data);
	harbol_bytebuffer_clear(&mod->var_data);
	
	return final_tbc;
}

static inline NO_NULL bool tagha_mod_gen_create_file(struct TaghaModGen *const restrict mod, const char filename[restrict static 1])
{
	bool result = false;
	FILE *restrict tbcfile = fopen(filename, "w");
	if( tbcfile==NULL )
		goto null_file_err;
	
	struct HarbolByteBuf final_tbc = __tagha_mod_gen_finalize(mod);
	result = harbol_bytebuffer_to_file(&final_tbc, tbcfile);
	fclose(tbcfile), tbcfile=NULL;
	harbol_bytebuffer_clear(&final_tbc);
	
null_file_err:
	return result;
}

static inline NO_NULL struct HarbolByteBuf tagha_mod_gen_buffer(struct TaghaModGen *const mod)
{
	struct HarbolByteBuf final_tbc = __tagha_mod_gen_finalize(mod);
	return final_tbc;
}

static inline NO_NULL uint8_t *tagha_mod_gen_raw(struct TaghaModGen *const mod)
{
	return tagha_mod_gen_buffer(mod).table;
}

#ifdef __cplusplus
} /// extern "C"
#endif

#endif /** TAGHA_MODULE_GEN */
