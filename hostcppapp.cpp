
#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include "tagha.h"

/*
 * Same as hostapp.c but using C++ code to test portability and compatibility.
*/

/* void print_helloworld(void); */
static void native_print_helloworld(Script_t *script, Param_t params[], Param_t **retval, const uint32_t argc)
{
	puts("native_print_helloworld :: hello world from bytecode!\n");
	*retval = nullptr;
}

/* void test_ptr(struct player *p); */
static void native_test_ptr(Script_t *script, Param_t params[], Param_t **retval, const uint32_t argc)
{
	struct Player {
		float		speed;
		uint32_t	health;
		uint32_t	ammo;
	} *player=nullptr;
	*retval = nullptr;
	
	// get first arg which is the virtual address to our data.
	player = reinterpret_cast< struct Player* >(params[0].Pointer);
	if( !player )
		return;
	
	// debug print to see if our data is accurate.
	printf("native_test_ptr :: ammo: %" PRIu32 " | health: %" PRIu32 " | speed: %f\n", player->ammo, player->health, player->speed);
	player=nullptr;
}

/* void getglobal(void); */
static void native_getglobal(Script_t *script, Param_t params[], Param_t **retval, const uint32_t argc)
{
	*retval = nullptr;
	int *p = (int *)TaghaScript_get_global_by_name(script, "i");
	if( !p )
		return;
	
	printf("native_getglobal :: i == %i\n", *p);
	p=nullptr;
}



int main(int argc, char **argv)
{
	if( !argv[1] ) {
		printf("[TaghaVM Usage]: './TaghaVM' '.tbc file' \n");
		return 1;
	}
	
	struct TaghaVM vm;
	Tagha_init(&vm);
	
	NativeInfo_t host_natives[] = {
		{"test", native_test_ptr},
		{"printHW", native_print_helloworld},
		{"getglobal", native_getglobal},
		{NULL, NULL}
	};
	Tagha_register_natives(&vm, host_natives);
	Tagha_load_libc_natives(&vm);
	
	Tagha_load_script_by_name(&vm, argv[1]);
	
	Tagha_exec(&vm, nullptr);
	Tagha_free(&vm);
	return 0;
}
