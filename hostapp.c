
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#include "tagha.h"



/* void print_helloworld(void); */
static void native_print_helloworld(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	if( !script )
		return;
	
	puts("native_print_helloworld :: hello world from bytecode!\n");
}

/* void test_ptr(struct player *p); */
static void native_test_ptr(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	if( !script )
		return;
	
	struct Player {
		float		speed;
		uint32_t	health;
		uint32_t	ammo;
	} *player=NULL;
	
	// get first arg which is the virtual address to our data.
	player = (struct Player *)(uintptr_t)TaghaScript_pop_int64(script);
	if( !player ) {
		puts("native_test_ptr reported an ERROR :: **** param 'p' is NULL ****\n");
		return;
	}
	
	// debug print to see if our data is accurate.
	printf("native_test_ptr :: ammo: %u\n", player->ammo);
	printf("native_test_ptr :: health: %u\n", player->health);
	printf("native_test_ptr :: speed: %f\n", player->speed);
}

/* void callfunc( void (*f)(void) ); */
static void native_callfunc(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	if( !script )
		return;
	
	// addr is the function address.
	uint64_t addr = TaghaScript_pop_int64(script);
	printf("native_callfunc :: func ptr addr: %" PRIu64 "\n", addr);
	// call our function which should push any return value back for us to pop.
	TaghaScript_call_func_by_addr(script, addr);
	printf("native_callfunc :: invoking.\n");
}

/* void getglobal(void); */
static void native_getglobal(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	if( !script )
		return;
	
	int *p = TaghaScript_get_global_by_name(script, "i");
	if( !p )
		return;
	printf("native_getglobal :: i == %i\n", *p);
}

/* void callfuncname( const char *func ); */
static void native_callfuncname(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	if( !script )
		return;
	
	const char *strfunc = (const char *)(uintptr_t)TaghaScript_pop_int64(script);
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
		printf("[TaghaVM Usage]: './TaghaVM' '.tagha file' \n");
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
		Tagha_load_script(&vm, argv[i]);
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
