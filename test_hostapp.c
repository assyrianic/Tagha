#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "tagha/tagha.h"


/* int add_one(int i); */
static NO_NULL UTaghaVal
native_add_one(STaghaModule *const mod, const size_t args, const UTaghaVal params[const static 1])
{
	(void)mod; (void)args;
	return (UTaghaVal){ .int32 = params[0].int32 + 1 };
}

/* size_t strlen(const char *str); */
static NO_NULL UTaghaVal
native_strlen(STaghaModule *const mod, const size_t args, const UTaghaVal params[const static 1])
{
	(void)mod; (void)args;
	const char *p = params[0].string;
	while( *p != 0 )
		p++;
	return (UTaghaVal){ .uint64 = p - params[0].string };
}

/* char *fgets(char *str, int num, FILE *stream); */
static NO_NULL UTaghaVal
native_fgets(STaghaModule *const module, const size_t args, const UTaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (UTaghaVal){ .string = fgets(params[0].string, params[1].int32, params[2].ptrvoid) };
}

/* int puts(const char *str); */
static NO_NULL UTaghaVal
native_puts(STaghaModule *const module, const size_t args, const UTaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (UTaghaVal){ .int32 = puts(params[0].string) };
}

/* int32_t tagha_module_call(struct TaghaModule *module, const char funcname[], size_t args, union TaghaVal params[], union TaghaVal *retval); */
static UTaghaVal native_tagha_module_call(STaghaModule *const module, const size_t args, const UTaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (UTaghaVal){ .int32 = tagha_module_call(params[0].ptrvoid, params[1].string, params[2].uint64, params[3].ptrself, params[4].ptrself) };
}

/* struct TaghaModule tagha_module_create_from_file(const char filename[]);
 * Returning a struct that's larger than 8 bytes.
 * Function is optimized into taking a struct pointer as first param.
 */
static UTaghaVal native_tagha_module_create_from_file(STaghaModule *const restrict module, const size_t args, const UTaghaVal params[const static 1])
{
	(void)module; (void)args;
	STaghaModule *const restrict loading_module = params[0].ptrvoid;
	*loading_module = tagha_module_create_from_file(params[1].string);
	return (UTaghaVal){0};
}

/* struct TaghaModule *tagha_module_new_from_file(const char filename[]); */
static UTaghaVal native_tagha_module_new_from_file(STaghaModule *const module, const size_t args, const UTaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (UTaghaVal){ .ptrvoid = tagha_module_new_from_file(params[0].ptrvoid) };
}

/* bool tagha_module_free(struct TaghaModule **modref); */
static UTaghaVal native_tagha_module_free(STaghaModule *const restrict module, const size_t args, const UTaghaVal params[const static 1])
{
	(void)module; (void)args;
	STaghaModule **restrict modref = params[0].ptrvoid;
	return (UTaghaVal){ .boolean = tagha_module_free(modref) };
}


struct Player {
	float32_t speed;
	uint32_t health, ammo;
};

NO_NULL int main(const int argc, char *argv[const static 1])
{
	(void)argc;
	if( !argv[1] ) {
		printf("[TaghaVM (v%s) Test Host App Usage]: './%s' '.tbc filepath' \n", TAGHA_VERSION_STRING, argv[0]);
		return 1;
	}
	struct TaghaModule *module = tagha_module_new_from_file(argv[1]);
	puts(module != NULL ? "module is valid." : "module is NULL.");
	if( module != NULL ) {
		/* make our global pointers available, if the module has them defined and uses them. */
		tagha_module_register_ptr(module, "stdin", stdin);
		tagha_module_register_ptr(module, "stdout", stdout);
		tagha_module_register_ptr(module, "stderr", stderr);
		tagha_module_register_ptr(module, "self", module);
		
		struct Player player = { -1.f, 1, 1 };
		
		const struct TaghaNative host_natives[] = {
			{"add_one", native_add_one},
			{"strlen", native_strlen},
			{"fgets", native_fgets},
			{"puts", native_puts},
			{"tagha_module_call", native_tagha_module_call},
			{"tagha_module_create_from_file", native_tagha_module_create_from_file},
			{"tagha_module_new_from_file", native_tagha_module_new_from_file},
			{"tagha_module_free", native_tagha_module_free},
			{NULL, NULL}
		};
		tagha_module_register_natives(module, host_natives);
		
		/* TODO: make work around so we can pass custom arguments to script's `main`.
		union TaghaVal script_main_args[] = {
			(union TaghaVal){ .int32 = 3 },
			// Have to make an array because 32-bit OS and 64-bit hardware.
			// can't directly pass a char*[] array or we break the 64-bit VM system.
			(union TaghaVal){ .ptrself = (union TaghaVal[]){
					{ .string = (char[]){ "example1" } },
					{ .string = (char[]){ "example2" } },
					{ .string = (char[]){ "example3" } },
					{ .string = NULL },
				} 
			}
		};
		*/
		
		const int32_t result = tagha_module_run(module, 0, NULL);
		tagha_module_print_vm_state(module);
		
		{
			// tagha_module_get_var returns a pointer to the data.
			// if the data from the script itself is a pointer,
			// then tagha_module_get_var will return a pointer to pointer.
			struct Player *p = tagha_module_get_var(module, "g_player");
			if( p != NULL )
				player = *p;
		}
		printf("player.speed: '%f' | player.health: '%u' | player.ammo: '%u'\nmodule result?: '%i'\nerror?: '%s'\n", player.speed, player.health, player.ammo, result, tagha_module_get_error(module));
		tagha_module_clear(module);
		free(module), module=NULL;
	}
}
