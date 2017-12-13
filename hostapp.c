
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include "tagha.h"

/*
* This is an example of a host application.
* Meant to test initializing, evaluating, shutting down VM, and testing natives.
*/

/* void print_helloworld(void); */
static void native_print_helloworld(struct TaghaScript *pScript, union CValue params[], union CValue *restrict pRetval, const uint32_t argc, struct TaghaVM *pEnv)
{
	puts("native_print_helloworld :: hello world from bytecode!\n");
}

/* void test(struct Player *p); */
static void native_test(struct TaghaScript *pScript, union CValue params[], union CValue *restrict pRetval, const uint32_t argc, struct TaghaVM *pEnv)
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
static void native_getglobal(struct TaghaScript *pScript, union CValue params[], union CValue *restrict pRetval, const uint32_t argc, struct TaghaVM *pEnv)
{
	int *p = TaghaScript_GetGlobalByName(pScript, "i");
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
	
	TaghaVM vm;
	Tagha_Init(&vm);
	
	NativeInfo host_natives[] = {
		{"test", native_test},
		{"printHW", native_print_helloworld},
		{"getglobal", native_getglobal},
		{NULL, NULL}
	};
	Tagha_RegisterNatives(&vm, host_natives);
	Tagha_LoadLibCNatives(&vm);
	Tagha_LoadSelfNatives(&vm);
	
	Tagha_LoadScriptByName(&vm, argv[1]);
	
	// keep compatible with 32-bit and 64-bit systems by
	// using CValue data instead of using a ptr to char ptr.
	int argcount = 3;
	CValue args[argcount];
	args[0].Str = argv[1],
	args[1].Str = "kektus",
	args[2].Str = NULL;
	int result = Tagha_Exec(&vm, argcount, args);
	printf("[Tagha] :: result: %" PRIi32 "\n", result);
	
	/* // tested with test_3d_vecs.tbc
	float vect[3]={ 10.f, 15.f, 20.f };
	TaghaScript_PushValue(Tagha_GetpScript(&vm), (CValue){ .Ptr=vect });
	Tagha_CallpScriptFunc(&vm, "VecInvert");
	printf("vect[3]=={ %f , %f, %f }\n", vect[0], vect[1], vect[2]);
	*/
	
	/* // For testing with "factorial.tbc".
	TaghaScript *pScript = Tagha_GetpScript(&vm);
	//TaghaScript_PushValue(pScript, (CValue){ .UInt32=6 });	// param b
	TaghaScript_PushValue(pScript, (CValue){ .UInt32=7 });	// param a
	Tagha_CallpScriptFunc(&vm, "factorial");
	printf("factorial result == %" PRIu32 "\n", TaghaScript_PopValue(pScript).UInt32);
	*/
	
	/*
	int32_t x;
	do {
		printf("0 or less to exit.\n");
		scanf("%i", &x);
	}
	while( x>0 );
	*/
	
	Tagha_Free(&vm);
	return 0;
}





