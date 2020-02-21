#include "tagha_libc.h"

/** struct TaghaModule *tagha_module_new_from_file(const char filename[]); */
static union TaghaVal native_tagha_module_new_from_file(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .ptrvoid = tagha_module_new_from_file(params[0].ptrvoid) };
}

/** int32_t tagha_module_call(struct TaghaModule *module, const char funcname[], size_t args, const union TaghaVal params[], union TaghaVal *retval); */
static union TaghaVal native_tagha_module_call(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .int32 = tagha_module_call(params[0].ptrvoid, params[1].string, params[2].uint64, params[3].ptrself, params[4].ptrself) };
}

/** void *tagha_module_get_var(struct TaghaModule *module, const char varname[]); */
static union TaghaVal native_tagha_module_get_var(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .ptrvoid = tagha_module_get_var(params[0].ptrvoid, params[1].string) };
}

/** bool tagha_module_free(struct TaghaModule **modref); */
static union TaghaVal native_tagha_module_free(struct TaghaModule *const restrict module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	struct TaghaModule **restrict modref = params[0].ptrvoid;
	return (union TaghaVal){ .boolean = tagha_module_free(modref) };
}

/** bool tagha_module_clear(struct TaghaModule *module); */
static union TaghaVal native_tagha_module_clear(struct TaghaModule *const restrict module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .boolean = tagha_module_clear(params[0].ptrvoid) };
}


bool tagha_module_load_module_natives(struct TaghaModule *const module)
{
	const struct TaghaNative tagha_module_natives[] = {
		{"tagha_module_new_from_file", native_tagha_module_new_from_file},
		{"tagha_module_call", native_tagha_module_call},
		{"tagha_module_get_var", native_tagha_module_get_var},
		{"tagha_module_free", native_tagha_module_free},
		{"tagha_module_clear", native_tagha_module_clear},
		{NULL, NULL}
	};
	return module ? tagha_module_register_natives(module, tagha_module_natives) : false;
}

