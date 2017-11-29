
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include "tagha.h"

/*
* This is an example of a host application.
* Meant to test initializing, evaluating, shutting down VM, and testing natives.
*/

/* void print_helloworld(void); */
static void native_print_helloworld(Script_t *script, Param_t params[], Param_t *restrict retval, const uint32_t argc, TaghaVM_t *env)
{
	puts("native_print_helloworld :: hello world from bytecode!\n");
}

/* void test(struct Player *p); */
static void native_test(Script_t *script, Param_t params[], Param_t *restrict retval, const uint32_t argc, TaghaVM_t *env)
{
	struct Player {
		float		speed;
		uint32_t	health;
		uint32_t	ammo;
	} *player=NULL;
	
	// get first arg which is the virtual address to our data.
	player = (struct Player *)params[0].Ptr;
	if( !player ) {
		puts("native_test_ptr reported an ERROR :: **** param 'p' is NULL ****\n");
		return;
	}
	
	// debug print to see if our data is accurate.
	printf("native_test_ptr :: ammo: %" PRIu32 " | health: %" PRIu32 " | speed: %f\n", player->ammo, player->health, player->speed);
	player=NULL;
}

/* void getglobal(void); */
static void native_getglobal(Script_t *script, Param_t params[], Param_t *restrict retval, const uint32_t argc, TaghaVM_t *env)
{
	int *p = TaghaScript_get_global_by_name(script, "i");
	if( !p )
		return;
	printf("native_getglobal :: i == %i\n", *p);
}


int main(int argc, char **argv)
{
	if( !argv[1] ) {
		printf("[TaghaVM Usage]: '%s' '.tbc file' \n", argv[0]);
		return 1;
	}
	
	TaghaVM_t vm;
	Tagha_init(&vm);
	
	NativeInfo_t host_natives[] = {
		{"test", native_test},
		{"printHW", native_print_helloworld},
		{"getglobal", native_getglobal},
		{NULL, NULL}
	};
	Tagha_register_natives(&vm, host_natives);
	Tagha_load_libc_natives(&vm);
	Tagha_load_self_natives(&vm);
	
	//uint32_t i;
	//for( i=argc-1 ; i ; i-- )
	Tagha_load_script_by_name(&vm, argv[1]);
	Tagha_exec(&vm, NULL);
	
	/* // tested with test_3d_vecs.tbc
	float vect[3]={ 10.f, 15.f, 20.f };
	TaghaScript_push_value(Tagha_get_script(&vm), (Val_t){ .Ptr=vect });
	Tagha_call_script_func(&vm, "VecInvert");
	printf("vect[3]=={ %f , %f, %f }\n", vect[0], vect[1], vect[2]);
	*/
	
	/* // For testing with "factorial.tbc".
	Script_t *script = Tagha_get_script(&vm);
	//TaghaScript_push_value(script, (Val_t){ .UInt32=6 });	// param b
	TaghaScript_push_value(script, (Val_t){ .UInt32=7 });	// param a
	Tagha_call_script_func(&vm, "factorial");
	Val_t result = TaghaScript_pop_value(script);
	printf("factorial result == %" PRIu32 "\n", result.UInt32);
	*/
	
	/*
	int32_t x;
	do {
		printf("0 or less to exit.\n");
		scanf("%i", &x);
	}
	while( x>0 );
	*/
	
	Tagha_free(&vm);
	return 0;
}





