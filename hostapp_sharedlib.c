
#include <stdio.h>
#include <stdlib.h>
#include <iso646.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <dlfcn.h>
#include "tagha.h"

void	*pLibTagha = NULL;
uchar	*(*taghascript_getptr)(Script_t *, const Word_t);
void	(*taghascript_pushint)(Script_t *, const uint);
void	(*taghascript_pushlong)(Script_t *, const u64);

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
	uchar *stkptr = (*taghascript_getptr)(script, addr);
	if( !stkptr ) {
		(*taghascript_pushint)(script, -1);
		return;
	}
	// cast the physical pointer to char* string.
	const char *str = (const char *)stkptr;
	
	// push back the value of the return val of puts.
	(*taghascript_pushint)(script, puts(str));
}

/* int printf(const char *fmt, ...); */
static void native_printf(Script_t *restrict script, const uint argc, const uint bytes, uchar *arrParams)
{
	if( !script )
		return;
	
	// get first arg which is the string's memory address.
	Word_t addr = *(Word_t *)arrParams;
	
	// using addr to retrieve the physical pointer of the string.
	uchar *stkptr = (*taghascript_getptr)(script, addr);
	if( !stkptr ) {
		(*taghascript_pushint)(script, -1);
		return;
	}
	// cast the physical pointer to char* string.
	const char *str = (const char *)stkptr;
	
	// ptr arithmetic our params straight to our args.
	arrParams += sizeof(Word_t);
	
	// execute (v)printf and push its return value which is a 4-byte int.
	(*taghascript_pushint)(script, vprintf(str, (char *)arrParams));
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
	uchar *stkptr = (*taghascript_getptr)(script, addr);
	if( !stkptr )
		return;
	player = (struct Player *)stkptr;
	
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
	
	uchar *stkptr_filestr = (*taghascript_getptr)(script, filename_addr);
	if( !stkptr_filestr ) {
		(*taghascript_pushint)(script, 0);
		return;
	}
	uchar *stkptr_modes = (*taghascript_getptr)(script, modes_addr);
	if( !stkptr_modes ) {
		(*taghascript_pushint)(script, 0);
		return;
	}
	
	const char *filename = (const char *)stkptr_filestr;
	const char *mode = (const char *)stkptr_modes;
	
	const unsigned ptrsize = sizeof(FILE *);
	FILE *pFile = fopen(filename, mode);
	if( pFile ) {
		printf("native_fopen:: opening file \'%s\' with mode: \'%s\'\n", filename, mode);
		if( ptrsize==4 )
			(*taghascript_pushint)(script, (uintptr_t)pFile);
		else (*taghascript_pushlong)(script, (uintptr_t)pFile);
		(*taghascript_pushint)(script, script->sp);
	}
	else {
		printf("failed to get filename: \'%s\'\n", filename);
		if( ptrsize==4 )
			(*taghascript_pushint)(script, 0);
		else (*taghascript_pushlong)(script, 0L);
		(*taghascript_pushint)(script, 0);
	}
}

/* int fclose(FILE *stream); */
static void native_fclose(Script_t *restrict script, const uint argc, const uint bytes, uchar *arrParams)
{
	if( !script )
		return;
	
	Word_t addr = *(Word_t *)arrParams;
	uchar *stkptr = (*taghascript_getptr)(script, addr);
	if( !stkptr ) {
		(*taghascript_pushint)(script, -1);
		return;
	}
	FILE *pFile = (FILE *) *(uintptr_t *)stkptr;
	if( pFile ) {
		printf("native_fclose:: closing FILE*\n");
		(*taghascript_pushint)(script, fclose(pFile));
		pFile=NULL;
	}
	else printf("native_fclose:: FILE* is NULL\n");
}

/* void *malloc(size_t size); */
static void native_malloc(Script_t *restrict script, const uint argc, const uint bytes, uchar *arrParams)
{
	if( !script )
		return;
	
	const Word_t ptrsize = *(Word_t *)arrParams;
	const unsigned size = sizeof(uintptr_t);
	
	printf("native_malloc:: allocating size: %u\n", ptrsize);
	void *p = malloc(ptrsize);
	if( p ) {
		printf("native_malloc:: pointer is VALID.\n");
		if( size==4 )
			(*taghascript_pushint)(script, (uintptr_t)p);
		else (*taghascript_pushlong)(script, (uintptr_t)p);
		(*taghascript_pushint)(script, script->sp);
	}
	else {
		if( size==4 )
			(*taghascript_pushint)(script, 0);
		else (*taghascript_pushlong)(script, 0L);
		(*taghascript_pushint)(script, 0);
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
	uchar *stkptr = (*taghascript_getptr)(script, addr);
	if( !stkptr )
		return;
	
	void *ptr = (void *) *(uintptr_t *)stkptr;
	if( ptr )
		free(ptr), ptr=NULL;
}

int main(int argc, char **argv)
{
	if( !argv[1] ) {
		printf("[TaghaVM Usage]: './TaghaVM' '.tagha file' \n");
		return 1;
	}
	pLibTagha = dlopen("./libtagha_gcc.so.1.0.0", RTLD_LAZY|RTLD_GLOBAL);
	if( !pLibTagha ) {
		printf("pLibTagha is NULL\n");
		return 1;
	}
	
	taghascript_getptr = dlsym(pLibTagha, "TaghaScript_addr2ptr");
	taghascript_pushint = dlsym(pLibTagha, "TaghaScript_push_int32");
	taghascript_pushlong = dlsym(pLibTagha, "TaghaScript_push_int64");
	
	TaghaVM_t vm;
	void (*taghaInit)(TaghaVM_t *) = dlsym(pLibTagha, "Tagha_init");
	(*taghaInit)(&vm);
	
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
	void (*taghaReg)(TaghaVM_t *, NativeInfo_t *) = dlsym(pLibTagha, "Tagha_register_natives");
	(*taghaReg)(&vm, host_natives);
	
	void (*taghaLoad)(TaghaVM_t *, char *) = dlsym(pLibTagha, "Tagha_load_script");
	uint i;
	for( i=argc-1 ; i ; i-- )
		(*taghaLoad)(&vm, argv[i]);
		
	void (*taghaExec)(TaghaVM_t *) = dlsym(pLibTagha, "Tagha_exec");
	(*taghaExec)(&vm);	//Tagha_free(script);
	/*
	int x;
	do {
		printf("0 or less to exit.\n");
		scanf("%i", &x);
	}
	while( x>0 );
	*/
	void (*taghaFree)(TaghaVM_t *) = dlsym(pLibTagha, "Tagha_free");
	(*taghaFree)(&vm);
	dlclose(pLibTagha), pLibTagha=NULL;
}
