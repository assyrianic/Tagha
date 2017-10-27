
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include "tagha.h"

/*
* This is an example of a host application.
* Meant to test initializing, evaluating, shutting down VM, and testing natives.
*/

/* void print_helloworld(void); */
static void native_print_helloworld(Script_t *restrict script, Param_t params[], const uint32_t argc)
{
	if( !script )
		return;
	
	puts("native_print_helloworld :: hello world from bytecode!\n");
}

/* void test_ptr(struct player *p); */
static void native_test_ptr(Script_t *restrict script, Param_t params[], const uint32_t argc)
{
	if( !script )
		return;
	
	struct Player {
		float		speed;
		uint32_t	health;
		uint32_t	ammo;
	} *player=NULL;
	
	// get first arg which is the virtual address to our data.
	player = (struct Player *)params[0].Pointer;//(uintptr_t)TaghaScript_pop_int64(script);
	if( !player ) {
		puts("native_test_ptr reported an ERROR :: **** param 'player' is NULL ****\n");
		return;
	}
	
	// debug print to see if our data is accurate.
	printf("native_test_ptr :: ammo: %" PRIu32 " | health: %" PRIu32 " | speed: %f\n", player->ammo, player->health, player->speed);
	player=NULL;
}

/* void callfunc( void (*f)(void) ); */
static void native_callfunc(Script_t *restrict script, Param_t params[], const uint32_t argc)
{
	if( !script )
		return;
	
	// addr is the function address.
	uint64_t addr = params[0].UInt64; //TaghaScript_pop_int64(script);
	printf("native_callfunc :: func ptr addr: %" PRIu64 "\n", addr);
	// call our function which should push any return value back for us to pop.
	TaghaScript_call_func_by_addr(script, addr);
	printf("native_callfunc :: invoking.\n");
}

/* void getglobal(void); */
static void native_getglobal(Script_t *restrict script, Param_t params[], const uint32_t argc)
{
	if( !script )
		return;
	
	int *p = TaghaScript_get_global_by_name(script, "i");
	if( !p )
		return;
	printf("native_getglobal :: i == %i\n", *p);
}

/* void callfuncname( const char *func ); */
static void native_callfuncname(Script_t *restrict script, Param_t params[], const uint32_t argc)
{
	if( !script )
		return;
	
	const char *strfunc = params[0].String; //(const char *)(uintptr_t)TaghaScript_pop_int64(script);
	if( !strfunc ) {
		puts("native_callfuncname reported an ERROR :: **** param 'func' is NULL ****\n");
		return;
	}
	
	TaghaScript_call_func_by_name(script, strfunc);
	printf("native_callfuncname :: finished calling script : \'%s\'\n", strfunc);
}


int main(int argc, char **argv)
{
	if( !argv[1] ) {
		puts("[TaghaVM Usage]: './TaghaVM' '.tbc file' \n");
		return 1;
	}
	
	TaghaVM_t vm;
	Tagha_init(&vm);
	
	NativeInfo_t host_natives[] = {
		{"test", native_test_ptr},
		{"printHW", native_print_helloworld},
		{"callfunc", native_callfunc},
		{"getglobal", native_getglobal},
		{"callfuncname", native_callfuncname},
		{NULL, NULL}
	};
	
	Tagha_register_natives(&vm, host_natives);
	Tagha_load_libc_natives(&vm);
	
	uint32_t i;
	for( i=argc-1 ; i ; i-- )
		Tagha_load_script_by_name(&vm, argv[i]);
	
	Tagha_exec(&vm);
	/*
	int32_t x;
	do {
		printf("0 or less to exit.\n");
		scanf("%i", &x);
	}
	while( x>0 );*/
	
	Tagha_free(&vm);
	return 0;
}





