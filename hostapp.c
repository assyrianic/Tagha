
#include <stdio.h>
#include <stdlib.h>
#include <iso646.h>
#include "vm.h"

/*
 * Example Host application
 * shows examples on how host apps can embed this VM.
 */ 

/* void PrintHelloWorld(void); */
static void NativePrintHelloWorld(TaghaVM_t *restrict vm, const uchar argc, const uint bytes, uchar *arrParams)
{
	if( !vm )
		return;
	
	printf("hello world from bytecode!\n");
}

/* void TestArgs(int, short, char, float); */
static void NativeTestArgs(TaghaVM_t *restrict vm, const uchar argc, const uint bytes, uchar *arrParams)
{
	if( !vm )
		return;
	
	int iInt = tagha_pop_int32(vm);
	printf("NativeTestArgs Int: %i\n", iInt); 
	ushort sShort = tagha_pop_short(vm);
	printf("NativeTestArgs uShort: %u\n", sShort); 
	char cChar = tagha_pop_byte(vm);
	printf("NativeTestArgs Char: %i\n", cChar); 
	float fFloat = tagha_pop_float32(vm);
	printf("NativeTestArgs Float: %f\n", fFloat); 
}

/* float TestArgs(void); */
static void NativeTestRet(TaghaVM_t *restrict vm, const uchar argc, const uint bytes, uchar *arrParams)
{
	if( !vm )
		return;
	
	float f = 100.f;
	printf("NativeTestRet: returning %f\n", f);
	tagha_push_float32(vm, f);
}

/* void Test(void); */
static void NativeTestArray(TaghaVM_t *restrict vm, const uchar argc, const uint bytes, uchar *arrParams)
{
	if( !vm )
		return;
	
	//int array[]={ 1,2,3,4,5,6,7,8,9,10 };
	//tagha_push_nbytes(vm, array, sizeof(int)*10);
	
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
	tagha_push_nbytes(vm, array2, sizeof(struct tester)*5);
	
	struct tester buffer[5];
	tagha_pop_nbytes(vm, buffer, sizeof(struct tester)*5);
	
	printf("NativeTestArray: buffer[0].str == %s\n", buffer[0].str);
}

/* void Test(struct player plyr) */
static void NativeTestPtr(TaghaVM_t *restrict vm, const uchar argc, const uint bytes, uchar *arrParams)
{
	if( !vm )
		return;
	
	// Code below all works example the last commented printf function.
	/*
	char *pstr = "kek";
	printf("NativeTestPtr: pushing ptr\n");
	tagha_push_nbytes(vm, pstr, 4);
	
	char *retstr = &(char[4]){0};
	tagha_pop_nbytes(vm, retstr, 4);
	printf("NativeTestPtr: char ptr %s\n", retstr);
	
	int i = 1;
	int *p = &i;
	tagha_push_nbytes(vm, &p, sizeof(int **));
	
	int **l = &(int *){ &(int){0} };
	tagha_pop_nbytes(vm, l, sizeof(int **));
	printf("NativeTestPtr: int ptr ptr %i\n", **l);
	
	//printf("%i\n", *(int *)arrParams);
	*/
	
	struct Player {
		uint	ammo;
		uint	health;
		float	speed;
	} player = *(struct Player *)arrParams;
	//tagha_pop_nbytes(vm, &player, sizeof(struct Player));
	
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
	
	TaghaVM_t *vm = &(TaghaVM_t){ 0 };
	tagha_init(vm);
	tagha_load_code(vm, argv[1]);
	//tagha_register_func(vm, NativePrintHelloWorld);
	//tagha_register_func(vm, NativeTestArgs);
	//tagha_register_func(vm, NativeTestRet);
	//tagha_register_func(vm, NativeTestArray);
	tagha_register_func(vm, NativeTestPtr);
	tagha_exec(vm);	//tagha_free(vm);
	
	/*
	// Hello World is approximately 12 chars if u count NULL-term
	
	char buffer[12];
	tagha_write_nbytes(vm, "Hello World", 12, 0x0);
	tagha_read_nbytes(vm, buffer, 12, 0x0);
	printf("read/write array test == %s\n", buffer);
	*/
	//tagha_push_int32(vm, 256);
	
	/*
	struct kek {
		long long int lli;
		short shrt;
		char ic;
	};
	struct kek *test = &(struct kek){500, 6436, 127};
	
	tagha_push_nbytes(vm, test, sizeof(struct kek));
	struct kek test2;
	tagha_pop_nbytes(vm, &test2, sizeof(struct kek));
	printf("test2 data: lli:%lli , shrt: %i , ic:%i\n", test2.lli, test2.shrt, test2.ic);
	*/
	tagha_debug_print_memory(vm);
	tagha_debug_print_stack(vm);
	tagha_free(vm);
	//tagha_debug_print_ptrs(p_vm);
	//free(program); program=NULL;
	return 0;
}
