
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "tagha.h"
#include "tagha_libc/tagha_libc.h"



/* int add_one(int i); */
void native_add_one(struct TaghaModule *const mod, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)mod; (void)args;
	retval->Int32 = params[0].Int32 + 1;
}


int main(const int argc, char *argv[restrict static argc+1])
{
	if( !argv[1] ) {
		printf("[TaghaVM Usage]: './%s' '.tbc filepath' \n", argv[0]);
		return 1;
	}
	struct TaghaModule *restrict module = tagha_module_new_from_file(argv[1]);
	puts(module ? "module is Valid." : "module is NULL.");
	if( module ) {
		/*
		// print func stuff
		{
			const union HarbolValue *const end = harbol_linkmap_get_iter_end_count(&module->FuncMap);
			for( const union HarbolValue *iter = harbol_linkmap_get_iter(&module->FuncMap) ; iter && iter<end ; iter++ ) {
				struct TaghaItem *const item = iter->KvPairPtr->Data.Ptr;
				printf("func key: '%s' | func bytes: '%zu'\n", iter->KvPairPtr->KeyName.CStr, item->Bytes);
				if( !item->Data )
					continue;
				for( size_t n=0 ; n<item->Bytes ; n++ ) {
					printf("Data[%zu] = '%X'\n", n, item->Data[n]);
				}
			}
		}
		// print var stuff
		{
			const union HarbolValue *const end = harbol_linkmap_get_iter_end_count(&module->VarMap);
			for( const union HarbolValue *iter = harbol_linkmap_get_iter(&module->VarMap) ; iter && iter<end ; iter++ ) {
				struct TaghaItem *const item = iter->KvPairPtr->Data.Ptr;
				printf("var key: '%s' | var bytes: '%zu'\n", iter->KvPairPtr->KeyName.CStr, item->Bytes);
				for( size_t n=0 ; n<item->Bytes ; n++ ) {
					printf("Data[%zu] = '%c'\n", n, item->Data[n]);
				}
			}
		}
		*/
		/* make our global pointers available, if the module has them defined and uses them. */
		tagha_module_register_ptr(module, "stdin", stdin);
		tagha_module_register_ptr(module, "stdout", stdout);
		tagha_module_register_ptr(module, "stderr", stderr);
		tagha_module_register_ptr(module, "self", module);
		
		/* load tagha library natives! */
		tagha_module_load_stdio_natives(module);
		tagha_module_load_string_natives(module);
		tagha_module_load_module_natives(module);
		
		struct Player {
			float speed;
			uint32_t health, ammo;
		} player = (struct Player){0};
		// tagha_module_get_globalvar_by_name returns a pointer to the data.
		// if the data from the script itself is a pointer, then it'll return a pointer-pointer.
		tagha_module_register_ptr(module, "g_pPlayer", &player);
		
		const struct TaghaNative host_natives[] = {
			{"add_one", native_add_one},
			{NULL, NULL}
		};
		tagha_module_register_natives(module, host_natives);
		
		const clock_t start = clock();
		const int32_t result = tagha_module_run(module, 2, (char *[]){ (char[]){"example1"}, (char[]){"example2"}, NULL });
		const clock_t end = clock();
		tagha_module_print_vm_state(module);
		
		printf("player.speed: '%f' | player.health: '%u' | player.ammo: '%u'\nrunning module... result?: '%i'\nerror?: '%s'\nprofiling time: '%f'\n", player.speed, player.health, player.ammo, result, tagha_module_get_error(module), (end-start)/(double)CLOCKS_PER_SEC);
	}
	printf("freeing module... result?: '%u'\n", tagha_module_del(module));
	free(module), module=NULL;
}
