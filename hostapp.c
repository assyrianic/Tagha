
#include <stdio.h>
#include <stdlib.h>
#include <iso646.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include "tagha.h"



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
	
	// get our first argument which is a pointer to char.
	Word_t addr = *(Word_t *)arrParams;
	
	// using addr to retrieve the physical pointer of the string.
	const char *str = (const char *)TaghaScript_addr2ptr(script, addr);
	
	// push back the value of the return val of puts.
	TaghaScript_push_int32(script, puts(str));
}

/* int printf(const char *fmt, ...); */
static void native_printf(Script_t *restrict script, const uint argc, const uint bytes, uchar *arrParams)
{
	if( !script )
		return;
	
	// get first arg which is the string's memory address.
	Word_t addr = *(Word_t *)arrParams;
	
	// using addr to retrieve the physical pointer of the string.
	const char *str = (const char *)TaghaScript_addr2ptr(script, addr);
	
	// ptr arithmetic our params straight to our args.
	arrParams += sizeof(Word_t);
	
	// execute (v)printf and push its return value which is a 4-byte int.
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
	
	// get first arg which is the memory address to our data.
	Word_t addr = *(Word_t *)arrParams;
	
	/*
	 * Notice the way our struct is formatted.
	 * we pushed the struct data from last to first.
	 * ammo is pushed first, then health, then finally the speed float.
	 * then we get the value from the stack and cast it to our struct!
	*/
	player = (struct Player *)TaghaScript_addr2ptr(script, addr);
	
	// debug print to see if our data is accurate.
	printf("native_test_ptr :: ammo: %u\n", player->ammo);
	printf("native_test_ptr :: health: %u\n", player->health);
	printf("native_test_ptr :: speed: %f\n", player->speed);
}

/* FILE *fopen(const char *filename, const char *modes); */
static void native_fopen(Script_t *restrict script, const uint argc, const uint bytes, uchar *arrParams)
{
	if( !script )
		return;
	
	Word_t filename_addr = *(Word_t *)arrParams;
	arrParams += sizeof(Word_t);
	Word_t modes_addr = *(Word_t *)arrParams;
	
	
	const char *filename = (const char *)TaghaScript_addr2ptr(script, filename_addr);
	const char *mode = (const char *)TaghaScript_addr2ptr(script, modes_addr);
	
	const unsigned ptrsize = sizeof(FILE *);
	FILE *pFile = fopen(filename, mode);
	if( pFile ) {
		if( ptrsize==4 )
			TaghaScript_push_int32(script, (uintptr_t)pFile);
		else TaghaScript_push_int64(script, (uintptr_t)pFile);
		fclose(pFile), pFile=NULL;
	}
	else {
		printf("failed to get filename: %s\n", filename);
		if( ptrsize==4 )
			TaghaScript_push_int32(script, 0);
		else TaghaScript_push_int64(script, 0L);
	}
}

/* int fclose(FILE *stream); */
static void native_fclose(Script_t *restrict script, const uint argc, const uint bytes, uchar *arrParams)
{
	if( !script )
		return;
	
	Word_t addr = *(Word_t *)arrParams;
	FILE *pf = (FILE *) *(uintptr_t *)TaghaScript_addr2ptr(script, addr);
	if( pf ) {
		TaghaScript_push_int32(script, fclose(pf));
		pf=NULL;
	}
}

/* void *malloc(size_t size); */
static void native_malloc(Script_t *restrict script, const uint argc, const uint bytes, uchar *arrParams)
{
	if( !script )
		return;
	
	const Word_t ptrsize = *(Word_t *)arrParams;
	const unsigned size = sizeof(void *);
	
	void *p = malloc(ptrsize);
	if( p ) {
		if( size==4 )
			TaghaScript_push_int32(script, (uintptr_t)p);
		else TaghaScript_push_int64(script, (uintptr_t)p);
	}
	else {
		if( size==4 )
			TaghaScript_push_int32(script, 0);
		else TaghaScript_push_int64(script, 0L);
	}
}

/* void free(void *ptr); */
static void native_free(Script_t *restrict script, const uint argc, const uint bytes, uchar *arrParams)
{
	if( !script )
		return;
	
	// arrParams holds the virtual address as usual.
	// get physical ptr then cast to an int that's big enough to hold a pointer
	// then cast to void pointer.
	Word_t addr = *(Word_t *)arrParams;
	void *ptr = (void *) *(uintptr_t *)TaghaScript_addr2ptr(script, addr);
	if( ptr )
		free(ptr), ptr=NULL;
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
		{"puts", native_puts},
		{"printf", native_printf},
		{"fopen", native_fopen},
		{"fclose", native_fclose},
		{"malloc", native_malloc},
		{"free", native_free},
		{NULL, NULL}
	};
	Tagha_register_natives(&vm, host_natives);
	
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
