
#include <stdlib.h>

/* void *malloc(size_t size); */
static void native_malloc(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	// size_t is 8 bytes on 64-bit systems
	const uint64_t ptrsize = TaghaScript_pop_int64(script);
	
	printf("native_malloc:: allocating size: %llu\n", ptrsize);
	void *p = malloc(ptrsize);
	if( p ) {
		printf("native_malloc:: pointer is VALID.\n");
		//uint32_t addr = TaghaScript_store_hostdata(script, p);
		//TaghaScript_push_int32(script, addr);
		TaghaScript_push_int64(script, (uintptr_t)p);
	}
	else {
		printf("native_malloc:: returned \'p\' is NULL\n");
		TaghaScript_push_int64(script, 0);
	}
}

/* void free(void *ptr); */
static void native_free(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	void *ptr = (void *)(uintptr_t) TaghaScript_pop_int64(script);
	if( ptr ) {
		printf("native_free :: ptr is VALID, freeing...\n");
		free(ptr), ptr=NULL;
	}
}

/* void *calloc(size_t num, size_t size); */
static void native_calloc(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	const uint64_t num = TaghaScript_pop_int64(script);
	const uint64_t size = TaghaScript_pop_int64(script);
	
	void *p = calloc(num, size);
	TaghaScript_push_int64(script, (uintptr_t)p);
}

/* void *realloc(void *ptr, size_t size); */
static void native_realloc(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	void *ptr = (void *)(uintptr_t)TaghaScript_pop_int64(script);
	const uint64_t size = TaghaScript_pop_int64(script);
	
	TaghaScript_push_int64(script, (uintptr_t)realloc(ptr, size));
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











