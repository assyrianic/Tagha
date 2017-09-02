
#include <stdio.h>
#include <stdlib.h>
#include <iso646.h>
#include "tagha.h"

/*
 * Example Host application
 * shows examples on how host apps can embed this VM.
 */ 

/* void print_helloworld(void); */
static void native_print_helloworld(Script_t *restrict script, const uint argc, const uint bytes, uchar *arrParams)
{
	if( !script )
		return;
	printf("hello world from bytecode!\n");
}

/* void puts(const char *); */
static void native_puts(Script_t *restrict script, const uint argc, const uint bytes, uchar *arrParams)
{
	if( !script )
		return;
	
	Word_t addr = *(Word_t *)arrParams;
	char *s = (char *)TaghaScript_addr2ptr(script, addr);
	printf(s);
}

/* void test_ptr(struct player *plyr) */
static void native_test_ptr(Script_t *restrict script, const uint argc, const uint bytes, uchar *arrParams)
{
	if( !script )
		return;
	
	// passing the struct data by pointer, get the address as a 4-
	Word_t addr = *(Word_t *)arrParams;
	struct Player {
		uint	ammo;
		uint	health;
		float	speed;
	}
	// old code for when we passed the struct data copy by-value instead of by-reference
	//player = *(struct Player *)arrParams;
	
	// compiler should have aligned the structure correctly so we can perform this.
	*player = (struct Player *)TaghaScript_addr2ptr(script, addr);
	//TaghaScript_pop_nbytes(script, &player, sizeof(struct Player));
	
	printf("native_test_ptr :: ammo: %u\n", player->ammo);
	printf("native_test_ptr :: health: %u\n", player->health);
	printf("native_test_ptr :: speed: %f\n", player->speed);
}


int main(int argc, char **argv)
{
	if( !argv[1] ) {
		printf("[TaghaVM Usage]: './TaghaVM' '.tagha file' \n");
		return 1;
	}
	
	TaghaVM_t vm = (TaghaVM_t){ 0 };
	//vm->pScript = &(Script_t){ 0 };
	//Script_t *script = &(Script_t){ 0 };
	Tagha_init(&vm);
	Tagha_load_script(&vm, argv[1]);
	//Tagha_register_native(script, native_print_helloworld);
	//Tagha_register_native(script, NativeTestArgs);
	//Tagha_register_native(script, NativeTestRet);
	//Tagha_register_native(script, NativeTestArray);
	
	NativeInfo_t Hostnatives[] = {
		{"test", native_test_ptr},
		{"printHW", native_print_helloworld},
		{"puts", native_puts},
		{NULL,NULL}
	};
	Tagha_register_natives(&vm, Hostnatives);
	Tagha_exec(&vm);	//Tagha_free(script);
	
	TaghaScript_debug_print_memory(vm.pScript);
	/*
	int x;
	do {
		printf("0 or less to exit.\n");
		scanf("%i", &x);
	}
	while( x>0 );
	*/
	Tagha_free(&vm);
	//TaghaScript_debug_print_ptrs(p_script);
	//free(program); program=NULL;
	return 0;
}
