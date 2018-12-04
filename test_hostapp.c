
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "tagha.h"

/* int puts(const char *str); */
void Native_puts(struct TaghaModule *const restrict sys, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)sys;
	const char *restrict p = params[0].Ptr;
	retval->Int32 = p != NULL ? puts(p) : -1;
}

/* char *fgets(char *buffer, int num, FILE *stream); */
void Native_fgets(struct TaghaModule *const restrict sys, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)sys;
	char *restrict buf = params[0].Ptr;
	FILE *restrict stream = params[2].Ptr;
	if( !buf || !stream ) {
		return;
	}
	retval->Ptr = fgets(buf, params[1].Int32, stream);
}

/* size_t strlen(const char *s); */
void Native_strlen(struct TaghaModule *const restrict sys, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)sys; (void)args;
	const char *restrict s = params[0].Ptr;
	for( ; *s ; s++ );
	retval->UInt64 = (s - (const char *)params[0].Ptr);
}

/* int add_one(int i); */
void Native_add_one(struct TaghaModule *const sys, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)sys; (void)args;
	retval->Int32 = params[0].Int32 + 1;
}

/* struct TaghaModule *tagha_module_new_from_file(const char filename[]); */
void native_tagha_module_new_from_file(struct TaghaModule *const restrict sys, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)sys; (void)args;
	//puts("called tagha_module_new_from_file native.");
	retval->Ptr = tagha_module_new_from_file(params[0].Ptr);
	//printf("tagha_module_new_from_file return value: %p.\n", retval->Ptr);
}

/* int32_t tagha_module_call(struct TaghaModule *module, const char funcname[], size_t args, union TaghaVal params[], union TaghaVal *return_val); */
void native_tagha_module_call(struct TaghaModule *const restrict sys, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)sys; (void)args;
	//puts("\ncalling tagha_module_call...");
	retval->Int32 = tagha_module_call(params[0].Ptr, params[1].PtrCStr, params[2].UInt64, params[3].PtrSelf, params[4].PtrSelf);
	//printf("tagha_module_call return value: %i.\n", retval->Int32);
}

/* bool tagha_module_free(struct TaghaModule **modref); */
void native_tagha_module_free(struct TaghaModule *const restrict sys, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)sys; (void)args;
	struct TaghaModule **modref = params[0].Ptr;
	//printf("tagha_module_free modref: %p.\n", modref);
	retval->Bool = tagha_module_free(modref);
}


int main(const int argc, char *argv[restrict static argc+1])
{
	if( !argv[1] ) {
		printf("[TaghaVM Usage]: './%s' '.tbc filepath' \n", argv[0]);
		return 1;
	}
	struct TaghaModule *module = tagha_module_new_from_file(argv[1]);
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
		tagha_module_register_ptr(module, "stdin", stdin);
		tagha_module_register_ptr(module, "stdout", stdout);
		tagha_module_register_ptr(module, "stderr", stderr);
		tagha_module_register_ptr(module, "self", module);
		
		struct Player {
			float speed;
			uint32_t health, ammo;
		};
		struct Player player = (struct Player){0};
		// tagha_module_get_globalvar_by_name returns a pointer to the data.
		// if the data itself is a pointer, then you gotta use a pointer-pointer.
		tagha_module_register_ptr(module, "g_pPlayer", &player);
		
		const struct TaghaNative host_natives[] = {
			{"puts", Native_puts},
			{"fgets", Native_fgets},
			{"strlen", Native_strlen},
			{"add_one", Native_add_one},
			{"tagha_module_new_from_file", native_tagha_module_new_from_file},
			{"tagha_module_call", native_tagha_module_call},
			{"tagha_module_free", native_tagha_module_free},
			{NULL, NULL}
		};
		tagha_module_register_natives(module, host_natives);
		const clock_t start = clock();
		const int32_t result = tagha_module_run(module, 0, NULL);
		const clock_t end = clock();
		tagha_module_print_vm_state(module);
		//if( pp )
		printf("player.speed: '%f' | player.health: '%u' | player.ammo: '%u'\n", player.speed, player.health, player.ammo);
		printf("running module... result?: '%i'\nerror?: '%s'\nprofiling time: '%f'\n", result, tagha_module_get_error(module), (end-start)/(double)CLOCKS_PER_SEC);
	}
	printf("freeing module... result?: '%u'\n", tagha_module_free(&module));
}
