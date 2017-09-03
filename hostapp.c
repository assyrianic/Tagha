
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
	// using addr for both byte size and address as addr is the size of the string.
	char *s = (char *)TaghaScript_addr2ptr(script, addr, addr);
	printf(s);
}

/* void test_ptr(struct player *p) */
static void native_test_ptr(Script_t *restrict script, const uint argc, const uint bytes, uchar *arrParams)
{
	if( !script )
		return;
	
	struct Player {
		uint	ammo;
		uint	health;
		float	speed;
	} *player=NULL;
	// old code for when we passed the struct data copy by-value instead of by-reference
	//player = *(struct Player *)arrParams;
	Word_t addr = *(Word_t *)arrParams;
	
	// Since the stack addresses are viewed from top of stack perspective, to get
	// the real address of our struct data, get the top most rather than bottom most address.
	// then we subtract it with the size of the data - 1. the Player struct's size is 12 bytes
	// so the address would be subtracted by 11. If the struct data is the first 12 bytes pushed
	// to the stack, then the final pointer address would be 12 - 11 aka 0x1.
	// after casting our final pointer to a struct Player*, we can deref-> our data correctly.
	player = (struct Player *)TaghaScript_addr2ptr(script, sizeof(struct Player), addr);
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
