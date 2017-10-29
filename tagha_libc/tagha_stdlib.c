
#include <stdlib.h>

/* void *malloc(size_t size); */
static void native_malloc(Script_t *restrict script, Param_t params[], Param_t **restrict retval, const uint32_t argc)
{
	// size_t is 8 bytes on 64-bit systems
	const uint64_t ptrsize = params[0].UInt64;
	
	printf("native_malloc:: allocating size: %" PRIu64 "\n", ptrsize);
	void *p = malloc(ptrsize);
	if( p )
		puts("native_malloc:: pointer is VALID.\n");
	else puts("native_malloc:: returned \'p\' is NULL\n");
	(*retval)->Pointer = p;
}

/* void free(void *ptr); */
static void native_free(Script_t *restrict script, Param_t params[], Param_t **restrict retval, const uint32_t argc)
{
	void *ptr = params[0].Pointer;
	if( ptr ) {
		puts("native_free :: ptr is VALID, freeing...\n");
		free(ptr), ptr=NULL;
	}
	*retval = NULL;
}

/* void *calloc(size_t num, size_t size); */
static void native_calloc(Script_t *restrict script, Param_t params[], Param_t **restrict retval, const uint32_t argc)
{
	const uint64_t num = params[0].UInt64;
	const uint64_t size = params[1].UInt64;
	
	(*retval)->Pointer = calloc(num, size);
}

/* void *realloc(void *ptr, size_t size); */
static void native_realloc(Script_t *restrict script, Param_t params[], Param_t **restrict retval, const uint32_t argc)
{
	void *ptr = params[0].Pointer;
	const uint64_t size = params[1].UInt64;
	
	(*retval)->Pointer = realloc(ptr, size);
}


void Tagha_load_stdlib_natives(struct TaghaVM *vm)
{
	if( !vm )
		return;
	
	NativeInfo_t libc_stdlib_natives[] = {
		{"malloc", native_malloc},
		{"free", native_free},
		{"calloc", native_calloc},
		{"realloc", native_realloc},
		{NULL, NULL}
	};
	Tagha_register_natives(vm, libc_stdlib_natives);
}











