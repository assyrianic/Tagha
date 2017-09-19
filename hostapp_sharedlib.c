
#include <stdio.h>
#include <stdlib.h>
#include <iso646.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <dlfcn.h>
#include "tagha.h"

void *pLibTagha;
uint8_t		*(*taghascript_getptr)(Script_t *, const Word_t);
void		(*taghascript_pushint)(Script_t *, const uint32_t);
void		(*taghascript_pushlong)(Script_t *, const uint64_t);
uint32_t	(*taghascript_popint)(Script_t *);
uint64_t	(*taghascript_poplong)(Script_t *);

/* void print_helloworld(void); */
static void native_print_helloworld(Script_t *restrict script, const uint32_t argc, const uint32_t bytes)
{
	if( !script )
		return;
	
	printf("native_print_helloworld :: hello world from bytecode!\n");
}

/* int puts(const char *s); */
static void native_puts(Script_t *restrict script, const uint32_t argc, const uint32_t bytes)
{
	if( !script )
		return;
	
	// get our first argument which is a pointer to char.
	Word_t addr = (*taghascript_popint)(script);
	
	// using addr to retrieve the physical pointer of the string.
	uint8_t *stkptr = (*taghascript_getptr)(script, addr);
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
static void native_printf(Script_t *restrict script, const uint32_t argc, const uint32_t bytes)
{
	if( !script )
		return;
	
	// get first arg which is the string's memory address.
	Word_t addr = (*taghascript_popint)(script);
	
	// using addr to retrieve the physical pointer of the string.
	uint8_t *stkptr = (*taghascript_getptr)(script, addr);
	if( !stkptr ) {
		(*taghascript_pushint)(script, -1);
		return;
	}
	// cast the physical pointer to char* string.
	const char *str = (const char *)stkptr;
	char *iter=(char *)str;
	int32_t chrs=0;
	
	if( !pLibTagha )
		return;
	
	double (*popdbl)(Script_t *) = dlsym(pLibTagha, "TaghaScript_pop_float64");
	uint8_t (*popbyte)(Script_t *) = dlsym(pLibTagha, "TaghaScript_pop_byte");
	
	while( *iter ) {
		if( *iter=='%' ) {
			iter++;
			
			char data_buffer[1024];
			switch( *iter ) {
				case '%':
					printf("%%");
					chrs++;
					break;
				
				case 'f':
				case 'F':
					chrs += sprintf(data_buffer, "%f", (*popdbl)(script));
					printf(data_buffer);
					break;
				
				case 'e':
				case 'E':
					chrs += sprintf(data_buffer, "%e", (*popdbl)(script));
					printf(data_buffer);
					break;
					
				case 'a':
				case 'A':
					chrs += sprintf(data_buffer, "%a", (*popdbl)(script));
					printf(data_buffer);
					break;
					
				case 'i':
				case 'd':
					chrs += sprintf(data_buffer, "%i", (int32_t)(*taghascript_popint)(script));
					printf(data_buffer);
					break;
					
				case 'u':
					chrs += sprintf(data_buffer, "%u", (*taghascript_popint)(script));
					printf(data_buffer);
					break;
					
				case 'x':
				case 'X':
					chrs += sprintf(data_buffer, "%x", (*taghascript_popint)(script));
					printf(data_buffer);
					break;
				
				case 'o':
					chrs += sprintf(data_buffer, "%o", (*taghascript_popint)(script));
					printf(data_buffer);
					break;
				
				case 'c':
					chrs += sprintf(data_buffer, "%c", (*popbyte)(script));
					printf(data_buffer);
					break;
				
				case 'p':
					chrs += sprintf(data_buffer, "%p", (void *) (*taghascript_popint)(script));
					printf(data_buffer);
					break;
				
				default:
					printf("invalid format\n");
					(*taghascript_pushint)(script, -1);
					return;
			}
		}
		else putchar(*iter);
		chrs++, iter++;
	}
	iter = NULL;
	(*taghascript_pushint)(script, (uint32_t)chrs);
}

/* void test_ptr(struct player *p); */
static void native_test_ptr(Script_t *restrict script, const uint32_t argc, const uint32_t bytes)
{
	if( !script )
		return;
	
	struct Player {
		float	speed;
		uint32_t	health;
		uint32_t	ammo;
	} *player=NULL;
	
	// get first arg which is the memory address to our data.
	Word_t addr = (*taghascript_popint)(script);
	
	/*
	 * Notice the way our struct is formatted.
	 * we pushed the struct data from last to first.
	 * ammo is pushed first, then health, then finally the speed float.
	 * then we get the value from the stack and cast it to our struct!
	*/
	uint8_t *stkptr = (*taghascript_getptr)(script, addr);
	if( !stkptr )
		return;
	player = (struct Player *)stkptr;
	
	// debug print to see if our data is accurate.
	printf("native_test_ptr :: ammo: %u\n", player->ammo);
	printf("native_test_ptr :: health: %u\n", player->health);
	printf("native_test_ptr :: speed: %f\n", player->speed);
}

/* FILE *fopen(const char *filename, const char *modes); */
static void native_fopen(Script_t *restrict script, const uint32_t argc, const uint32_t bytes)
{
	if( !script )
		return;
	
	Word_t filename_addr = (*taghascript_popint)(script);
	Word_t modes_addr = (*taghascript_popint)(script);
	
	uint8_t *stkptr_filestr = (*taghascript_getptr)(script, filename_addr);
	if( !stkptr_filestr ) {
		(*taghascript_pushint)(script, 0);
		return;
	}
	uint8_t *stkptr_modes = (*taghascript_getptr)(script, modes_addr);
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
static void native_fclose(Script_t *restrict script, const uint32_t argc, const uint32_t bytes)
{
	if( !script )
		return;
	
	Word_t addr = (*taghascript_popint)(script);
	uint8_t *stkptr = (*taghascript_getptr)(script, addr);
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
static void native_malloc(Script_t *restrict script, const uint32_t argc, const uint32_t bytes)
{
	if( !script )
		return;
	
	const Word_t ptrsize = (*taghascript_popint)(script);
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
static void native_free(Script_t *restrict script, const uint32_t argc, const uint32_t bytes)
{
	if( !script )
		return;
	
	// pop the virtual address as usual.
	// get physical ptr then cast to an int that's big enough to hold a pointer
	// then cast to void pointer.
	Word_t addr = (*taghascript_popint)(script);
	uint8_t *stkptr = (*taghascript_getptr)(script, addr);
	if( !stkptr )
		return;
	
	void *ptr = (void *) *(uintptr_t *)stkptr;
	if( ptr )
		printf("native_free :: ptr is VALID, freeing...\n"), free(ptr), ptr=NULL;
}

/* void callfunc( void (*f)(void) ); */
static void native_callfunc(Script_t *restrict script, const uint32_t argc, const uint32_t bytes)
{
	if( !script )
		return;
	
	// addr is the function address.
	Word_t addr = (*taghascript_popint)(script);
	printf("native_callfunc :: func ptr addr: %u\n", addr);
	// call our function which should push any return value back for us to pop.
	void (*call_func_by_addr)(Script_t *, const uint32_t) = dlsym(pLibTagha, "TaghaScript_call_func_by_addr");
	(*call_func_by_addr)(script, addr);
	printf("native_callfunc :: invoking.\n");
	//TaghaScript_call_func_by_name(script, "f");
}

/* void getglobal(void); */
static void native_getglobal(Script_t *restrict script, const uint32_t argc, const uint32_t bytes)
{
	if( !script )
		return;
	
	void (*get_global_by_name)(Script_t *, const char *) = dlsym(pLibTagha, "TaghaScript_get_global_by_name");
	uint8_t *p = (*get_global_by_name)(script, "i");
	if( !p )
		return;
	
	printf("native_getglobal :: i == %i\n", *(int *)p);
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
	taghascript_popint = dlsym(pLibTagha, "TaghaScript_push_int64");
	taghascript_poplong = dlsym(pLibTagha, "TaghaScript_pop_int32");
	
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
	uint32_t i;
	for( i=argc-1 ; i ; i-- )
		(*taghaLoad)(&vm, argv[i]);
		
	void (*taghaExec)(TaghaVM_t *) = dlsym(pLibTagha, "Tagha_exec");
	(*taghaExec)(&vm);
	/*
	int32_t x;
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
