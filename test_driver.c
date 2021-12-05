#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "tagha/tagha.h"
#include "tagha_toolchain/module_info.h"

/// struct TaghaModule *tagha_module_new_from_file(const char filename[]);
static NO_NULL union TaghaVal native_tagha_module_new_from_file(struct TaghaModule *const restrict module, const union TaghaVal params[const static 1])
{
	( void )(module);
	const char *const filename = ( const char* )(params[0].uintptr);
	return ( union TaghaVal ){ .uintptr = ( uintptr_t )(tagha_module_new_from_file(filename)) };
}

/// bool tagha_module_free(struct TaghaModule **modref);
static NO_NULL union TaghaVal native_tagha_module_free(struct TaghaModule *const restrict module, const union TaghaVal params[const static 1])
{
	( void )(module);
	struct TaghaModule **const restrict modref = ( struct TaghaModule** )(params[0].uintptr);
	return ( union TaghaVal ){ .b00l = tagha_module_free(modref) };
}

/// TaghaFunc tagha_module_get_func(struct TaghaModule *module, const char name[]);
static NO_NULL union TaghaVal native_tagha_module_get_func(struct TaghaModule *const module, const union TaghaVal params[const static 2])
{
	const struct TaghaModule *const p = ( const struct TaghaModule* )(params[0].uintptr);
	const char *const name = ( const char* )(params[1].uintptr);
	return ( union TaghaVal ){ .uintptr = ( uintptr_t )(tagha_module_get_func((p==NULL)? module : p, name)) };
}

/// int puts(const char *str);
static NO_NULL union TaghaVal native_puts(struct TaghaModule *const restrict module, const union TaghaVal params[const static 1])
{
	( void )(module);
	const char *const cstr = ( const char* )(params[0].uintptr);
	return ( union TaghaVal ){ .int32 = puts(cstr) };
}

/// char *fgets(char *str, int num, FILE *stream);
static NO_NULL union TaghaVal native_fgets(struct TaghaModule *const module, const union TaghaVal params[const static 3])
{
	( void )(module);
	char *const restrict buffer = ( char* )(params[0].uintptr);
	FILE *const restrict stream = ( FILE* )(params[2].uintptr);
	return ( union TaghaVal ){ .uintptr = ( uintptr_t )(fgets(buffer, params[1].int32, stream)) };
}

/// int add_one(const int n);
static NO_NULL union TaghaVal native_add_one(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )(module);
	return ( union TaghaVal ){ .int32 = params[0].int32 + 1 };
}

/*
/// int strcpy(char *str1, const char *str2);
static NO_NULL union TaghaVal native_strcpy(struct TaghaModule *const restrict module, const union TaghaVal params[const restrict static 2])
{
	( void )(module);
	char *restrict str1 = ( char* )(params[0].uintptr);
	const char *str2 = ( const char* )(params[1].uintptr);
	
	int i=0;
	while( (str1[i] = str2[i]) != 0 ) {
		i++;
	}
	return ( union TaghaVal ){ .int32 = i };
}
*/

/// void tagha_module_link_module(struct TaghaModule *module, struct TaghaModule *lib);
static NO_NULL union TaghaVal native_tagha_module_link_module(struct TaghaModule *const restrict module, const union TaghaVal params[const static 2])
{
	struct TaghaModule *const restrict caller = ( struct TaghaModule* )(params[0].uintptr);
	const struct TaghaModule *const lib = ( const struct TaghaModule* )(params[1].uintptr);
	tagha_module_link_module(((caller==NULL)? module : caller), lib);
	return ( union TaghaVal ){ 0 };
}

/**
 * POTENTIALLY DANGEROUS though not sure how.
 * If a dev uses `alloca`, compiler could possibly allocate its data that is already in use.
 * Whole point of `alloca` is to have a form of dynamic allocation without resorting to `malloc` + friends and then having to free.
 * This implementation of `alloca` does NOT modify the (operand) stack pointer but can cause issues if `alloca` is called and the compiler then allocates additional registers or if a callee function needs registers itself as well which could corrupt data.
 */
/// void *alloca(size_t len);
static NO_NULL union TaghaVal native_alloca(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	const size_t len = params[0].size;
	const size_t aligned_len = (len + (sizeof(union TaghaVal)-1)) & -sizeof(union TaghaVal);
	const uintptr_t alloc_space = module->osp - aligned_len;
	return ( union TaghaVal ){ .uintptr = (alloc_space < module->opstack)? NIL : alloc_space };
}


NO_NULL int main(const int argc, char *argv[restrict static 1]) {
	( void )(argc);
	if( argv[1]==NULL ) {
		printf("[TaghaVM (v%s) Test Host App Usage]: '%s' '.tbc filepath' \n", TAGHA_VERSION_STRING, argv[0]);
		//remove("tagha_test_res.txt");
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
				//{"strcpy",                     &native_strcpy},
				{"add_one",                    &native_add_one},
				{NULL, NULL}
			});
			
			tagha_module_link_ptr(module, "stdin",  ( uintptr_t )(stdin));
			tagha_module_link_ptr(module, "stderr", ( uintptr_t )(stderr));
			tagha_module_link_ptr(module, "stdout", ( uintptr_t )(stdout));
			tagha_module_link_ptr(module, "self",   ( uintptr_t )(module));
			
			const clock_t start     = clock();
			const int     r         = tagha_module_run(module, 0, NULL);
			const clock_t end       = clock();
			const float64_t elapsed = ( float64_t )(end - start) / ( float64_t )(CLOCKS_PER_SEC);
			FILE *res_file = fopen("tagha_test_res.txt", "a+");
			if( res_file==NULL )
				return -1;
			
			fprintf(res_file, "result => %i | err? '%s' | elapsed => %" PRIf64 "ms\n", r, tagha_module_get_err(module), elapsed * 1000.);
			fclose(res_file);
			//tagha_module_print_opstack(module, res_file);
			//tagha_module_print_callstack(module, res_file);
			
			tagha_module_free(&module);
		}
	}
}