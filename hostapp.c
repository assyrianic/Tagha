
#include <stdio.h>
#include <stdlib.h>
#include <iso646.h>
#include "vm.h"

/*
 * Example Host application
 * shows examples on how host apps can embed this VM.
 */ 

/* void PrintHelloWorld(void); */
static void NativePrintHelloWorld(Script_t *restrict script, const uchar argc, const uint bytes, uchar *arrParams)
{
	if( !script )
		return;
	
	printf("hello world from bytecode!\n");
}

/* void TestArgs(int, short, char, float); */
static void NativeTestArgs(Script_t *restrict script, const uchar argc, const uint bytes, uchar *arrParams)
{
	if( !script )
		return;
	
	int iInt = TaghaScript_pop_int32(script);
	printf("NativeTestArgs Int: %i\n", iInt); 
	ushort sShort = TaghaScript_pop_short(script);
	printf("NativeTestArgs uShort: %u\n", sShort); 
	char cChar = TaghaScript_pop_byte(script);
	printf("NativeTestArgs Char: %i\n", cChar); 
	float fFloat = TaghaScript_pop_float32(script);
	printf("NativeTestArgs Float: %f\n", fFloat); 
}

/* float TestArgs(void); */
static void NativeTestRet(Script_t *restrict script, const uchar argc, const uint bytes, uchar *arrParams)
{
	if( !script )
		return;
	
	float f = 100.f;
	printf("NativeTestRet: returning %f\n", f);
	TaghaScript_push_float32(script, f);
}

/* void Test(void); */
static void NativeTestArray(Script_t *restrict script, const uchar argc, const uint bytes, uchar *arrParams)
{
	if( !script )
		return;
	
	//int array[]={ 1,2,3,4,5,6,7,8,9,10 };
	//TaghaScript_push_nbytes(script, array, sizeof(int)*10);
	
	struct tester {
		float	fl;
		char	*str;
	} array2[] = {
		{1.f, "1.f"},
		{2.f, "2.f"},
		{3.f, "3.f"},
		{4.f, "4.f"},
		{5.f, "5.f"}
	};
	printf("NativeTestArray: pushing array\n");
	TaghaScript_push_nbytes(script, array2, sizeof(struct tester)*5);
	
	struct tester buffer[5];
	TaghaScript_pop_nbytes(script, buffer, sizeof(struct tester)*5);
	
	printf("NativeTestArray: buffer[0].str == %s\n", buffer[0].str);
}

/* void Test(struct player plyr) */
static void NativeTestPtr(Script_t *restrict script, const uchar argc, const uint bytes, uchar *arrParams)
{
	if( !script )
		return;
	
	// Code below all works example the last commented printf function.
	/*
	char *pstr = "kek";
	printf("NativeTestPtr: pushing ptr\n");
	TaghaScript_push_nbytes(script, pstr, 4);
	
	char *retstr = &(char[4]){0};
	TaghaScript_pop_nbytes(script, retstr, 4);
	printf("NativeTestPtr: char ptr %s\n", retstr);
	
	int i = 1;
	int *p = &i;
	TaghaScript_push_nbytes(script, &p, sizeof(int **));
	
	int **l = &(int *){ &(int){0} };
	TaghaScript_pop_nbytes(script, l, sizeof(int **));
	printf("NativeTestPtr: int ptr ptr %i\n", **l);
	
	//printf("%i\n", *(int *)arrParams);
	*/
	
	struct Player {
		uint	ammo;
		uint	health;
		float	speed;
	} player = *(struct Player *)arrParams;
	//TaghaScript_pop_nbytes(script, &player, sizeof(struct Player));
	
	printf("NativeTestPtr :: ammo: %u\n", player.ammo);
	printf("NativeTestPtr :: health: %u\n", player.health);
	printf("NativeTestPtr :: speed: %f\n", player.speed);
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
	//Tagha_register_native(script, NativePrintHelloWorld);
	//Tagha_register_native(script, NativeTestArgs);
	//Tagha_register_native(script, NativeTestRet);
	//Tagha_register_native(script, NativeTestArray);
	Tagha_register_native(&vm, NativeTestPtr);
	Tagha_exec(&vm);	//Tagha_free(script);
	
	/*
	// Hello World is approximately 12 chars if u count NULL-term
	
	char buffer[12];
	TaghaScript_write_nbytes(script, "Hello World", 12, 0x0);
	TaghaScript_read_nbytes(script, buffer, 12, 0x0);
	printf("read/write array test == %s\n", buffer);
	*/
	//TaghaScript_push_int32(script, 256);
	
	/*
	struct kek {
		long long int lli;
		short shrt;
		char ic;
	};
	struct kek *test = &(struct kek){500, 6436, 127};
	
	TaghaScript_push_nbytes(script, test, sizeof(struct kek));
	struct kek test2;
	TaghaScript_pop_nbytes(script, &test2, sizeof(struct kek));
	printf("test2 data: lli:%lli , shrt: %i , ic:%i\n", test2.lli, test2.shrt, test2.ic);
	*/
	TaghaScript_debug_print_memory(vm.pScript);
	TaghaScript_debug_print_stack(vm.pScript);
	Tagha_free(&vm);
	//TaghaScript_debug_print_ptrs(p_script);
	//free(program); program=NULL;
	return 0;
}
