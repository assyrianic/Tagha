#include "tagha_libc.h"

/** struct TaghaModule *tagha_module_new_from_file(const char filename[]); */
static union TaghaVal native_tagha_module_new_from_file(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .uintptr = ( uintptr_t )tagha_module_new_from_file(( const char* )params[0].uintptr) };
}

/** struct TaghaModule tagha_module_create_from_file(const char filename[]); */
static union TaghaVal native_tagha_module_create_from_file(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	*( struct TaghaModule* )params[0].uintptr = tagha_module_create_from_file(( const char* )params[1].uintptr);
	return (union TaghaVal){ 0 };
}

/** void *tagha_module_get_func(struct TaghaModule *module, const char funcname[]); */
static union TaghaVal native_tagha_module_get_func(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	struct TaghaModule *const p = ( struct TaghaModule* )params[0].uintptr;
	return (union TaghaVal){ .uintptr = ( uintptr_t )tagha_module_get_func(p==NULL ? module : p, ( const char* )params[1].uintptr) };
}

/** void *tagha_module_get_var(struct TaghaModule *module, const char varname[]); */
static union TaghaVal native_tagha_module_get_var(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	struct TaghaModule *const p = ( struct TaghaModule* )params[0].uintptr;
	return (union TaghaVal){ .uintptr = ( uintptr_t )tagha_module_get_var(p==NULL ? module : p, ( const char* )params[1].uintptr) };
}

/** bool tagha_module_free(struct TaghaModule **modref); */
static union TaghaVal native_tagha_module_free(struct TaghaModule *const restrict module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	struct TaghaModule **restrict modref = ( struct TaghaModule** )params[0].uintptr;
	return (union TaghaVal){ .b00l = tagha_module_free(modref) };
}

/** bool tagha_module_clear(struct TaghaModule *module); */
static union TaghaVal native_tagha_module_clear(struct TaghaModule *const restrict module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .b00l = tagha_module_clear(( struct TaghaModule* )params[0].uintptr) };
}

/** void tagha_module_resolve_links(struct TaghaModule *module, struct TaghaModule *lib); */
static union TaghaVal native_tagha_module_resolve_links(struct TaghaModule *const restrict module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	struct TaghaModule       *const restrict caller = ( struct TaghaModule* )params[0].uintptr;
	const struct TaghaModule *const restrict lib    = ( const struct TaghaModule* )params[1].uintptr;
	tagha_module_resolve_links(caller==NULL ? module : caller, lib);
	return (union TaghaVal){ 0 };
}


bool tagha_module_load_module_natives(struct TaghaModule *const module)
{
	const struct TaghaNative tagha_module_natives[] = {
		{"tagha_module_new_from_file", native_tagha_module_new_from_file},
		{"tagha_module_create_from_file", native_tagha_module_create_from_file},
		{"tagha_module_get_func", native_tagha_module_get_func},
		{"tagha_module_get_var", native_tagha_module_get_var},
		{"tagha_module_resolve_links", native_tagha_module_resolve_links},
		{"tagha_module_free", native_tagha_module_free},
		{"tagha_module_clear", native_tagha_module_clear},
		{NULL, NULL}
	};
	return module ? tagha_module_register_natives(module, tagha_module_natives) : false;
}

