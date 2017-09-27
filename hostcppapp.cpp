
#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <cstdint>
#include <cinttypes>
#include "tagha.h"


/* void print_helloworld(void); */
static void native_print_helloworld(struct TaghaScript *script, const uint32_t argc, const uint32_t bytes)
{
	if( !script )
		return;
	
	puts("native_print_helloworld :: hello world from bytecode!\n");
}

/* int puts(const char *s); */
static void native_puts(struct TaghaScript *script, const uint32_t argc, const uint32_t bytes)
{
	if( !script )
		return;
	
	// get our first parameter which is a virtual address to our string.
	uint64_t addr = TaghaScript_pop_int64(script);
	
	// use the virtual address to get the physical pointer of the string.
	uint8_t *stkptr = TaghaScript_addr2ptr(script, addr);
	if( !stkptr ) {
		TaghaScript_push_int32(script, -1);
		return;
	}
	// cast the physical pointer to char*.
	const char *str = reinterpret_cast< const char* >(stkptr);
	
	// push back the value of the return val of puts.
	TaghaScript_push_int32(script, puts(str));
}

/* int printf(const char *fmt, ...); */
static void native_printf(struct TaghaScript *script, const uint32_t argc, const uint32_t bytes)
{
	if( !script )
		return;
	
	// get our first parameter which is a virtual address to our string.
	uint64_t addr = TaghaScript_pop_int64(script);
	
	// use the virtual address to get the physical pointer of the string.
	uint8_t *stkptr = TaghaScript_addr2ptr(script, addr);
	if( !stkptr ) {
		TaghaScript_push_int32(script, -1);
		return;
	}
	// cast the physical pointer to char*.
	const char *str = reinterpret_cast< const char* >(stkptr);
	
	char *iter = const_cast< char* >(str);
	int32_t chrs=0;
	
	while( *iter ) {
		if( *iter=='%' ) {
			iter++;
			
			// make sure we zero out the buffer 
			char data_buffer[1024] = {0};
			
			switch( *iter ) {
				case '%':
					printf("%%");
					chrs++;
					break;
				
				case 'f':
				case 'F':
					chrs += sprintf(data_buffer, "%f", TaghaScript_pop_float64(script));
					printf("%s", data_buffer);
					break;
				
				case 'e':
				case 'E':
					chrs += sprintf(data_buffer, "%e", TaghaScript_pop_float64(script));
					printf("%s", data_buffer);
					break;
					
				case 'a':
				case 'A':
					chrs += sprintf(data_buffer, "%a", TaghaScript_pop_float64(script));
					printf("%s", data_buffer);
					break;
					
				case 'i':
				case 'd':
					chrs += sprintf(data_buffer, "%i", (int32_t)TaghaScript_pop_int32(script));
					printf("%s", data_buffer);
					break;
					
				case 'u':
					chrs += sprintf(data_buffer, "%u", TaghaScript_pop_int32(script));
					printf("%s", data_buffer);
					break;
					
				case 'x':
				case 'X':
					chrs += sprintf(data_buffer, "%x", TaghaScript_pop_int32(script));
					printf("%s", data_buffer);
					break;
				
				case 'o':
					chrs += sprintf(data_buffer, "%o", TaghaScript_pop_int32(script));
					printf("%s", data_buffer);
					break;
				
				case 'c':
					chrs += sprintf(data_buffer, "%c", TaghaScript_pop_byte(script));
					printf("%s", data_buffer);
					break;
				
				case 'p':
					chrs += sprintf(data_buffer, "%p", reinterpret_cast< void* >(TaghaScript_pop_int64(script)));
					printf("%s", data_buffer);
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
	iter = nullptr;
	TaghaScript_push_int32(script, (uint32_t)chrs);
}

/* void test_ptr(struct player *p); */
static void native_test_ptr(struct TaghaScript *script, const uint32_t argc, const uint32_t bytes)
{
	if( !script )
		return;
	
	struct Player {
		float	speed;
		uint32_t	health;
		uint32_t	ammo;
	} *player=nullptr;
	
	// get first arg which is the virtual address to our data.
	uint64_t addr = TaghaScript_pop_int64(script);
	
	/*
	 * Notice the order of the struct's data.
	 * we pushed the struct data from last to first.
	 * ammo is pushed first, then health, then finally the speed float.
	 * then we get the value from the stack and cast it to our struct!
	*/
	uint8_t *stkptr = TaghaScript_addr2ptr(script, addr);
	if( !stkptr )
		return;
	player = reinterpret_cast< struct Player* >(stkptr);
	
	// debug print to see if our data is accurate.
	printf("native_test_ptr :: ammo: %u\n", player->ammo);
	printf("native_test_ptr :: health: %u\n", player->health);
	printf("native_test_ptr :: speed: %f\n", player->speed);
}

/* FILE *fopen(const char *filename, const char *modes); */
static void native_fopen(struct TaghaScript *script, const uint32_t argc, const uint32_t bytes)
{
	if( !script )
		return;
	
	uint64_t filename_addr = TaghaScript_pop_int64(script);
	uint64_t modes_addr = TaghaScript_pop_int64(script);
	
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
	
	FILE *pFile = fopen(filename, mode);
	if( pFile ) {
		printf("native_fopen:: opening file \'%s\' with mode: \'%s\'\n", filename, mode);
		TaghaScript_push_int64(script, (uintptr_t)pFile);
	}
	else {
		printf("failed to get filename: \'%s\'\n", filename);
		TaghaScript_push_int64(script, 0);
	}
}

/* int fclose(FILE *stream); */
static void native_fclose(struct TaghaScript *script, const uint32_t argc, const uint32_t bytes)
{
	if( !script )
		return;
	
	FILE *pFile = reinterpret_cast< FILE* >(TaghaScript_pop_int64(script));
	if( pFile ) {
		printf("native_fclose:: closing FILE*\n");
		TaghaScript_push_int32(script, fclose(pFile));
		pFile=nullptr;
	}
	else {
		printf("native_fclose:: FILE* is NULL\n");
		TaghaScript_push_int32(script, -1);
	}
}

/* void *malloc(size_t size); */
static void native_malloc(struct TaghaScript *script, const uint32_t argc, const uint32_t bytes)
{
	if( !script )
		return;
	
	const uint64_t ptrsize = TaghaScript_pop_int64(script);
	
	printf("native_malloc:: allocating size: %" PRIWord "\n", ptrsize);
	void *p = malloc(ptrsize);
	if( p ) {
		printf("native_malloc:: pointer is VALID.\n");
		TaghaScript_push_int64(script, (uintptr_t)p);
	}
	else {
		printf("native_malloc:: returned\'p\' is NULL\n");
		TaghaScript_push_int64(script, 0);
	}
}

/* void free(void *ptr); */
static void native_free(struct TaghaScript *script, const uint32_t argc, const uint32_t bytes)
{
	if( !script )
		return;
	
	void *ptr = (void *)TaghaScript_pop_int64(script);
	if( ptr ) {
		printf("native_free :: ptr is VALID, freeing...\n");
		free(ptr), ptr=NULL;
	}
}

/* void callfunc( void (*f)(void) ); */
static void native_callfunc(struct TaghaScript *script, const uint32_t argc, const uint32_t bytes)
{
	if( !script )
		return;
	
	// addr is the function address.
	uint64_t addr = TaghaScript_pop_int64(script);
	printf("native_callfunc :: func ptr addr: %" PRIWord "\n", addr);
	// call our function which should push any return value back for us to pop.
	TaghaScript_call_func_by_addr(script, addr);
	printf("native_callfunc :: invoking.\n");
}

/* void getglobal(void); */
static void native_getglobal(struct TaghaScript *script, const uint32_t argc, const uint32_t bytes)
{
	if( !script )
		return;
	
	void *p = TaghaScript_get_global_by_name(script, "i");
	if( p==nullptr )
		return;
	printf("native_getglobal :: i == %i\n", *(int *)p);
}

/* void callfuncname( const char *func ); */
static void native_callfuncname(struct TaghaScript *script, const uint32_t argc, const uint32_t bytes)
{
	if( !script )
		return;
	
	uint64_t addr = TaghaScript_pop_int64(script);
	printf("native_callfuncname :: func ptr addr: %" PRIWord "\n", addr);
	
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
	
	struct TaghaVM vm;
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
	Tagha_free(&vm);
	return 0;
}
