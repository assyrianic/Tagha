
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include "tagha.h"


struct Player {
	float		speed;
	uint32_t	health;
	uint32_t	ammo;
};

/*
* This is an example of a host application.
* Meant to test initializing, evaluating, shutting down VM, and testing natives.
*/

/* void print_helloworld(void); */
static void native_print_helloworld(struct Tagha *pSys, union CValue params[], union CValue *restrict pRetval, const uint32_t argc)
{
	puts("native_print_helloworld :: hello world from bytecode!\n");
}

/* void test(struct Player *p); */
static void native_test(struct Tagha *pSys, union CValue params[], union CValue *restrict pRetval, const uint32_t argc)
{
	struct Player *player=NULL;
	
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
static void native_getglobal(struct Tagha *pSys, union CValue params[], union CValue *restrict pRetval, const uint32_t argc)
{
	int *p = Tagha_GetGlobalByName(pSys, "i");
	if( !p )
		return;
	printf("native_getglobal :: i == %i\n", *p);
}


/* int puts(const char *__s); */
static void native_puts(struct Tagha *pSys, union CValue params[], union CValue *restrict pRetval, const uint32_t argc)
{
	const char *restrict __s = params[0].String;
	if( !__s ) {
		puts("native_puts :: reported \'__s\' is NULL.\n");
		pRetval->Int32 = -1;
		return;
	}
	pRetval->Int32 = puts(__s);
}

/* char *fgets(char *str, int num, FILE *stream); */
static void native_fgets(struct Tagha *pSys, union CValue params[], union CValue *restrict pRetval, const uint32_t argc)
{
	pRetval->Ptr = fgets(params[0].Str, params[1].Int32, params[2].Ptr);
}


int main(int argc, char *argv[])
{
	if( !argv[1] ) {
		printf("[Tagha Usage]: '%s' '.tbc file' \n", argv[0]);
		return 1;
	}
	
	struct Tagha *vm = Tagha_New();
	if( !vm ) {
		puts("Tagha :: Tagha_New returned NULL\n");
		return 1;
	}
	struct NativeInfo tagha_host_natives[] = {
		{"test", native_test},
		{"printHW", native_print_helloworld},
		{"getglobal", native_getglobal},
		{"puts", native_puts},
		{"fgets", native_fgets},
		{NULL, NULL}
	};
	Tagha_RegisterNatives(vm, tagha_host_natives);
	Tagha_LoadScriptByName(vm, argv[1]);
	
	/* // for testing with 'test_exported_host_var.tbc'
	struct Player player = (struct Player){0};
	struct Player **ppPlayer = Tagha_GetGlobalByName(vm, "g_pPlayer");
	if( ppPlayer )
		*ppPlayer=&player;
	*/
	char *args[] = {
		argv[1],
		"kektus",
		NULL
	};
	Tagha_SetCmdArgs(vm, args);
	printf("[Tagha] :: result: %" PRIi32 "\n", Tagha_RunScript(vm));
	
	//printf("[Tagha] :: player.speed: %f\nplayer.health: %u\nplayer.ammo: %u\n", player.speed, player.health, player.ammo);
	
	/* // tested with 'test_3d_vecs.tbc'.
	float vect[3]={ 10.f, 15.f, 20.f };
	Tagha_PushValues(vm, 1, &(CValue){ .Ptr=vect });
	Tagha_CallFunc(vm, "VecInvert");
	printf("vect[3]=={ %f , %f, %f }\n", vect[0], vect[1], vect[2]);
	*/
	
	/* // For testing with 'test_factorial.tbc'.
	//Tagha_PushValues(vm, 1, &(CValue){ .UInt32=6 });	// param b
	Tagha_PushValues(vm, 1, &(CValue){ .UInt32=7 });	// param a
	Tagha_CallFunc(vm, "factorial");
	printf("factorial result == %" PRIu32 "\n", Tagha_PopValue(vm).UInt32);
	*/
	
	/*
	int32_t x;
	do {
		puts("0 or less to exit.\n");
		scanf("%i", &x);
	}
	while( x>0 );
	*/
	
	Tagha_Free(vm);
	FREE_MEM(vm);
	return 0;
}





