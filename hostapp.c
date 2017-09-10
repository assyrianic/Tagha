
#include <stdio.h>
#include <stdlib.h>
#include <iso646.h>
#include <stddef.h>
#include <stdarg.h>
#include "tagha.h"

/*
 * Example Host application
 * shows examples on how host apps can embed this VM.
 */ 
 
 /*
  * void func (int a, ...)
 { 
   // va_start
   char *p = (char *) &a + sizeof a;

   // va_arg
   int i1 = *((int *)p);
   p += sizeof (int);

   // va_arg
   int i2 = *((int *)p);
   p += sizeof (int);

   // va_arg
   long i2 = *((long *)p);
   p += sizeof (long);
 }
  */ 

/* void print_helloworld(void); */
static void native_print_helloworld(Script_t *restrict script, const uint argc, const uint bytes, uchar *arrParams)
{
	if( !script )
		return;
	printf("native_print_helloworld :: hello world from bytecode!\n");
}

/* int puts(const char *s); */
static void native_puts(Script_t *restrict script, const uint argc, const uint bytes, uchar *arrParams)
{
	if( !script )
		return;
	
	Word_t addr = *(Word_t *)arrParams;
	//Word_t addr = TaghaScript_pop_int32(script);

	// using addr for both byte size and address as addr is the size of the string.
	//printf("native_puts :: script->sp == %u\n", script->sp);
	const char *str = (const char *)TaghaScript_addr2ptr(script, addr);
	TaghaScript_push_int32(script, puts(str));
}

/* int printf(const char *fmt, ...); */
static void native_printf(Script_t *restrict script, const uint argc, const uint bytes, uchar *arrParams)
{
	if( !script )
		return;
	
	// get first arg which is the string's memory address.
	// using pop_nbytes because Word_t can be modified into any size.
	Word_t addr = *(Word_t *)arrParams;
	// using addr for both byte size and address as addr is the size of the string.
	const char *str = (const char *)TaghaScript_addr2ptr(script, addr);
	arrParams += 4;
	TaghaScript_push_int32(script, vprintf(str, (char *)arrParams));
}

/* void test_ptr(struct player *p); */
static void native_test_ptr(Script_t *restrict script, const uint argc, const uint bytes, uchar *arrParams)
{
	if( !script )
		return;
	
	struct Player {
		float	speed;
		uint	health;
		uint	ammo;
	} *player=NULL;
	// old code for when we passed the struct data copy by-value instead of by-reference
	//player = *(struct Player *)arrParams;
	
	// get first arg which is the memory address to our data.
	Word_t addr = *(Word_t *)arrParams;
	//Word_t addr; TaghaScript_pop_nbytes(script, &addr, sizeof(Word_t));
	
	/*
	 * Notice the way our struct is formatted.
	 * we pushed the struct data from last to first.
	 * ammo is pushed first, then health, then finally the speed float.
	 * then we get the value from the stack and cast it to our struct!
	*/
	player = (struct Player *)TaghaScript_addr2ptr(script, addr);
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
	
	TaghaVM_t vm;
	Tagha_init(&vm);
	
	NativeInfo_t Hostnatives[] = {
		{"test", native_test_ptr},
		{"printHW", native_print_helloworld},
		{"puts", native_puts},
		{"printf", native_printf},
		{NULL,NULL}
	};
	Tagha_register_natives(&vm, Hostnatives);
	
	uint i;
	for( i=argc-1 ; i ; i-- )
		Tagha_load_script(&vm, argv[i]);
	Tagha_exec(&vm);	//Tagha_free(script);
	/*
	int x;
	do {
		printf("0 or less to exit.\n");
		scanf("%i", &x);
	}
	while( x>0 );
	*/
	Tagha_free(&vm);
	return 0;
}
