#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "tagha/tagha.h"
#include "tagha_toolchain/module_info.h"

/// struct TaghaModule *tagha_module_new_from_file(const char filename[]);
static NO_NULL union TaghaVal native_tagha_module_new_from_file(struct TaghaModule *const restrict module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .uintptr = ( uintptr_t )tagha_module_new_from_file(( const char* )params[0].uintptr) };
}

/// bool tagha_module_free(struct TaghaModule **modref);
static NO_NULL union TaghaVal native_tagha_module_free(struct TaghaModule *const restrict module, const union TaghaVal params[const static 1])
{
	( void )module;
	struct TaghaModule **const restrict modref = ( struct TaghaModule** )params[0].uintptr;
	return ( union TaghaVal ){ .b00l = tagha_module_free(modref) };
}

/// TaghaFunc tagha_module_get_func(struct TaghaModule *module, const char name[]);
static NO_NULL union TaghaVal native_tagha_module_get_func(struct TaghaModule *const module, const union TaghaVal params[const static 2])
{
	struct TaghaModule *const p = ( struct TaghaModule* )params[0].uintptr;
	return ( union TaghaVal ){ .uintptr = ( uintptr_t )tagha_module_get_func(p==NULL ? module : p, ( const char* )params[1].uintptr) };
}

/// int puts(const char *str);
static NO_NULL union TaghaVal native_puts(struct TaghaModule *const restrict module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .int32 = puts(( const char* )params[0].uintptr) };
}

/// char *fgets(char *str, int num, FILE *stream);
static NO_NULL union TaghaVal native_fgets(struct TaghaModule *const module, const union TaghaVal params[const static 3])
{
	( void )module;
	return ( union TaghaVal ){ .uintptr = ( uintptr_t )fgets(( char* )params[0].uintptr, params[1].int32, ( FILE* )params[2].uintptr) };
}

/// int add_one(const int n);
static NO_NULL union TaghaVal native_add_one(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .int32 = params[0].int32 + 1 };
}

/// void tagha_module_link_module(struct TaghaModule *module, struct TaghaModule *lib);
static NO_NULL union TaghaVal native_tagha_module_link_module(struct TaghaModule *const restrict module, const union TaghaVal params[const static 2])
{
	( void )module;
	struct TaghaModule       *const restrict caller = ( struct TaghaModule* )params[0].uintptr;
	const struct TaghaModule *const restrict lib    = ( const struct TaghaModule* )params[1].uintptr;
	tagha_module_link_module(caller==NULL ? module : caller, lib);
	return ( union TaghaVal ){ 0 };
}


NO_NULL int main(const int argc, char *argv[const restrict static 1])
{
	( void )argc;
	if( argv[1]==NULL ) {
		printf("[TaghaVM (v%s) Test Host App Usage]: './%s' '.tbc filepath' \n", TAGHA_VERSION_STRING, argv[0]);
		return 1;
	} else {
		struct TaghaModule *module = tagha_module_new_from_file(argv[1]);
		if( module != NULL ) {
			tagha_module_link_natives(module, ( const struct TaghaNative[] ){
				{"tagha_module_new_from_file", &native_tagha_module_new_from_file},
				{"tagha_module_free",          &native_tagha_module_free},
				{"tagha_module_get_func",      &native_tagha_module_get_func},
				{"tagha_module_link_module",   &native_tagha_module_link_module},
				{"puts",                       &native_puts},
				{"fgets",                      &native_fgets},
				{"add_one",                    &native_add_one},
				{NULL, NULL}
			});
			
			tagha_module_link_ptr(module, "stdin",  ( uintptr_t )stdin);
			tagha_module_link_ptr(module, "stderr", ( uintptr_t )stderr);
			tagha_module_link_ptr(module, "stdout", ( uintptr_t )stdout);
			tagha_module_link_ptr(module, "self",   ( uintptr_t )module);
			
			int32_t r = 0;
			tagha_module_run(module, 0, NULL, &r);
			printf("result => %i | err? '%s'\n", r, tagha_module_get_err(module));
			tagha_module_print_opstack(module);
			tagha_module_print_callstack(module);
			
			tagha_module_free(&module);
		}
	}
}
