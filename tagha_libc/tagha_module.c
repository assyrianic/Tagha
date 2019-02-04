#include "tagha_libc.h"

/* struct TaghaModule *tagha_module_new_from_file(const char filename[]); */
static void native_tagha_module_new_from_file(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->Ptr = tagha_module_new_from_file(params[0].Ptr);
}

/* int32_t tagha_module_call(struct TaghaModule *module, const char funcname[], size_t args, union TaghaVal params[], union TaghaVal *return_val); */
static void native_tagha_module_call(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->Int32 = tagha_module_call(params[0].Ptr, params[1].PtrConstChar, params[2].UInt64, params[3].PtrSelf, params[4].PtrSelf);
}

/* void *tagha_module_get_globalvar_by_name(struct TaghaModule *module, const char varname[]); */
static void native_tagha_module_get_globalvar_by_name(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->Ptr = tagha_module_get_globalvar_by_name(params[0].Ptr, params[1].PtrConstChar);
}

/* bool tagha_module_free(struct TaghaModule **modref); */
static void native_tagha_module_free(struct TaghaModule *const restrict module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	struct TaghaModule **restrict modref = params[0].Ptr;
	retval->Bool = tagha_module_free(modref);
}

/* bool tagha_module_from_file(struct TaghaModule *module, const char filename[]); */
static void native_tagha_module_from_file(struct TaghaModule *const restrict module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->Bool = tagha_module_from_file(params[0].Ptr, params[1].PtrConstChar);
}

/* bool tagha_module_del(struct TaghaModule *module); */
static void native_tagha_module_del(struct TaghaModule *const restrict module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->Bool = tagha_module_del(params[0].Ptr);
}


bool tagha_module_load_module_natives(struct TaghaModule *const module)
{
	const struct TaghaNative tagha_module_natives[] = {
		{"tagha_module_new_from_file", native_tagha_module_new_from_file},
		{"tagha_module_call", native_tagha_module_call},
		{"tagha_module_get_globalvar_by_name", native_tagha_module_get_globalvar_by_name},
		{"tagha_module_free", native_tagha_module_free},
		{"tagha_module_from_file", native_tagha_module_from_file},
		{"tagha_module_del", native_tagha_module_del},
		{NULL, NULL}
	};
	return module ? tagha_module_register_natives(module, tagha_module_natives) : false;
}

