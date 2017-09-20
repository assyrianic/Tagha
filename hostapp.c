
#include <stdio.h>
#include <stdlib.h>
#include <iso646.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include "tagha.h"



/* void print_helloworld(void); */
static void native_print_helloworld(Script_t *restrict script, const uint32_t argc, const uint32_t bytes)
{
	if( !script )
		return;
	
	puts("native_print_helloworld :: hello world from bytecode!\n");
}

/* int puts(const char *s); */
static void native_puts(Script_t *restrict script, const uint32_t argc, const uint32_t bytes)
{
	if( !script )
		return;
	
	// get our first parameter which is a virtual address to our string.
	Word_t addr = TaghaScript_pop_int32(script);
	
	// use the virtual address to get the physical pointer of the string.
	uint8_t *stkptr = TaghaScript_addr2ptr(script, addr);
	if( !stkptr ) {
		TaghaScript_push_int32(script, -1);
		puts("native_puts reported an ERROR :: **** param 's' is NULL ****\n");
		return;
	}
	// cast the physical pointer to char*.
	const char *str = (const char *)stkptr;
	
	// push back the value of the return val of puts.
	TaghaScript_push_int32(script, puts(str));
	stkptr=NULL;
}

/* int printf(const char *fmt, ...); */
static void native_printf(Script_t *restrict script, const uint32_t argc, const uint32_t bytes)
{
	if( !script )
		return;
	
	// get our first parameter which is a virtual address to our string.
	Word_t addr = TaghaScript_pop_int32(script);
	
	// use the virtual address to get the physical pointer of the string.
	uint8_t *stkptr = TaghaScript_addr2ptr(script, addr);
	if( !stkptr ) {
		TaghaScript_push_int32(script, -1);
		puts("native_printf reported an ERROR :: **** param 'fmt' is NULL ****\n");
		return;
	}
	// cast the physical pointer to char*.
	const char *str = (const char *)stkptr;
	
	char *iter=(char *)str;
	int32_t chrs=0;
	
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
					chrs += sprintf(data_buffer, "%f", TaghaScript_pop_float64(script));
					printf(data_buffer);
					break;
				
				case 'e':
				case 'E':
					chrs += sprintf(data_buffer, "%e", TaghaScript_pop_float64(script));
					printf(data_buffer);
					break;
					
				case 'a':
				case 'A':
					chrs += sprintf(data_buffer, "%a", TaghaScript_pop_float64(script));
					printf(data_buffer);
					break;
					
				case 'i':
				case 'd':
					chrs += sprintf(data_buffer, "%i", (int32_t)TaghaScript_pop_int32(script));
					printf(data_buffer);
					break;
					
				case 'u':
					chrs += sprintf(data_buffer, "%u", TaghaScript_pop_int32(script));
					printf(data_buffer);
					break;
					
				case 'x':
				case 'X':
					chrs += sprintf(data_buffer, "%x", TaghaScript_pop_int32(script));
					printf(data_buffer);
					break;
				
				case 'o':
					chrs += sprintf(data_buffer, "%o", TaghaScript_pop_int32(script));
					printf(data_buffer);
					break;
				
				case 'c':
					chrs += sprintf(data_buffer, "%c", TaghaScript_pop_byte(script));
					printf(data_buffer);
					break;
				
				case 'p':
					chrs += sprintf(data_buffer, "%p", (void *) TaghaScript_pop_int32(script));
					printf(data_buffer);
					break;
				
				default:
					printf("invalid format\n");
					TaghaScript_push_int32(script, -1);
					return;
			} /* switch( *iter ) */
		} /* if( *iter=='%' ) */
		else putchar(*iter);
		chrs++, iter++;
	} /* while( *iter ) */
	iter = NULL;
	TaghaScript_push_int32(script, (uint32_t)chrs);
}

/* void test_ptr(struct player *p); */
static void native_test_ptr(Script_t *restrict script, const uint32_t argc, const uint32_t bytes)
{
	if( !script )
		return;
	
	struct Player {
		float		speed;
		uint32_t	health;
		uint32_t	ammo;
	} *player=NULL;
	
	// get first arg which is the virtual address to our data.
	Word_t addr = TaghaScript_pop_int32(script);
	
	/*
	 * Notice the order of the struct's data.
	 * we pushed the struct data from last to first.
	 * ammo is pushed first, then health, then finally the speed float.
	 * then we get the value from the stack and cast it to our struct!
	*/
	uint8_t *stkptr = TaghaScript_addr2ptr(script, addr);
	if( !stkptr ) {
		puts("native_test_ptr reported an ERROR :: **** param 'p' is NULL ****\n");
		return;
	}
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
	
	Word_t filename_addr = TaghaScript_pop_int32(script);
	Word_t modes_addr = TaghaScript_pop_int32(script);
	
	uint8_t *stkptr_filestr = TaghaScript_addr2ptr(script, filename_addr);
	if( !stkptr_filestr ) {
		puts("native_fopen reported an ERROR :: **** param 'filename' is NULL ****\n");
		TaghaScript_push_int32(script, 0);
		return;
	}
	uint8_t *stkptr_modes = TaghaScript_addr2ptr(script, modes_addr);
	if( !stkptr_modes ) {
		puts("native_fopen reported an ERROR :: **** param 'modes' is NULL ****\n");
		TaghaScript_push_int32(script, 0);
		return;
	}
	
	const char *filename = (const char *)stkptr_filestr;
	const char *mode = (const char *)stkptr_modes;
	
	const unsigned ptrsize = sizeof(FILE *);
	FILE *pFile = fopen(filename, mode);
	if( pFile ) {
		printf("native_fopen:: opening file \'%s\' with mode: \'%s\'\n", filename, mode);
		if( ptrsize==4 )
			TaghaScript_push_int32(script, (uintptr_t)pFile);
		else TaghaScript_push_int64(script, (uintptr_t)pFile);
		TaghaScript_push_int32(script, script->sp);
	}
	else {
		printf("failed to get filename: \'%s\'\n", filename);
		if( ptrsize==4 )
			TaghaScript_push_int32(script, 0);
		else TaghaScript_push_int64(script, 0L);
		TaghaScript_push_int32(script, 0);
	}
}

/* int fclose(FILE *stream); */
static void native_fclose(Script_t *restrict script, const uint32_t argc, const uint32_t bytes)
{
	if( !script )
		return;
	
	Word_t addr = TaghaScript_pop_int32(script);
	uint8_t *stkptr = TaghaScript_addr2ptr(script, addr);
	if( !stkptr ) {
		puts("native_fclose reported an ERROR :: **** param 'stream' is NULL ****\n");
		TaghaScript_push_int32(script, -1);
		return;
	}
	FILE *pFile = (FILE *) *(uintptr_t *)stkptr;
	if( pFile ) {
		printf("native_fclose:: closing FILE*\n");
		TaghaScript_push_int32(script, fclose(pFile));
		pFile=NULL;
	}
	else printf("native_fclose:: FILE* is NULL\n");
}

/* void *malloc(size_t size); */
static void native_malloc(Script_t *restrict script, const uint32_t argc, const uint32_t bytes)
{
	if( !script )
		return;
	
	const Word_t ptrsize = TaghaScript_pop_int32(script);
	const unsigned size = sizeof(uintptr_t);
	
	printf("native_malloc:: allocating size: %u\n", ptrsize);
	void *p = malloc(ptrsize);
	if( p ) {
		printf("native_malloc:: pointer is VALID.\n");
		if( size==4 )
			TaghaScript_push_int32(script, (uintptr_t)p);
		else TaghaScript_push_int64(script, (uintptr_t)p);
		TaghaScript_push_int32(script, script->sp);
	}
	else {
		if( size==4 )
			TaghaScript_push_int32(script, 0);
		else TaghaScript_push_int64(script, 0L);
		TaghaScript_push_int32(script, 0);
	}
}

/* void free(void *ptr); */
static void native_free(Script_t *restrict script, const uint32_t argc, const uint32_t bytes)
{
	if( !script )
		return;
	
	// get physical ptr then cast to an int that's big enough to hold a pointer
	// then cast to void pointer.
	Word_t addr = TaghaScript_pop_int32(script);
	uint8_t *stkptr = TaghaScript_addr2ptr(script, addr);
	if( !stkptr ) {
		puts("native_free reported an ERROR :: **** param 'ptr' is NULL ****\n");
		return;
	}
	
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
	Word_t addr = TaghaScript_pop_int32(script);
	printf("native_callfunc :: func ptr addr: %u\n", addr);
	// call our function which should push any return value back for us to pop.
	TaghaScript_call_func_by_addr(script, addr);
	printf("native_callfunc :: invoking.\n");
	//TaghaScript_call_func_by_name(script, "f");
}

/* void getglobal(void); */
static void native_getglobal(Script_t *restrict script, const uint32_t argc, const uint32_t bytes)
{
	if( !script )
		return;
	
	uint8_t *p = TaghaScript_get_global_by_name(script, "i");
	if( !p )
		return;
	printf("native_getglobal :: i == %i\n", *(int *)p);
}

/* void callfuncname( const char *func ); */
static void native_callfuncname(Script_t *restrict script, const uint32_t argc, const uint32_t bytes)
{
	if( !script )
		return;
	
	// addr is the function address.
	Word_t addr = TaghaScript_pop_int32(script);
	printf("native_callfuncname :: func ptr addr: %u\n", addr);
	uint8_t *stkptr = TaghaScript_addr2ptr(script, addr);
	if( !stkptr ) {
		puts("native_callfuncname reported an ERROR :: **** param 'func' is NULL ****\n");
		return;
	}
	TaghaScript_call_func_by_name(script, (const char *)stkptr);
	printf("native_callfuncname :: finished calling script : \'%s\'\n", (const char *)stkptr);
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
		{"callfunc", native_callfunc},
		{"getglobal", native_getglobal},
		{"callfuncname", native_callfuncname},
		{NULL, NULL}
	};
	Tagha_register_natives(&vm, host_natives);
	
	uint32_t i;
	for( i=argc-1 ; i ; i-- )
		Tagha_load_script(&vm, argv[i]);
	Tagha_exec(&vm);
	/*
	int32_t x;
	do {
		printf("0 or less to exit.\n");
		scanf("%i", &x);
	}
	while( x>0 );*/
	
	Tagha_free(&vm);
	return 0;
}
