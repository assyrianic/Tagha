
#include <stdlib.h>

/* void *malloc(size_t size); */
static void native_malloc(Script_t *script, Param_t params[], Param_t **restrict retval, const uint32_t argc, TaghaVM_t *env)
{
	// size_t is 8 bytes on 64-bit systems
	const uint64_t ptrsize = params[0].UInt64;
	
	printf("native_malloc:: allocating size: %" PRIu64 "\n", ptrsize);
	void *p = malloc(ptrsize);
	if( p )
		puts("native_malloc:: pointer is VALID.\n");
	else puts("native_malloc:: returned \'p\' is NULL\n");
	(*retval)->Ptr = p;
}

/* void free(void *ptr); */
static void native_free(Script_t *script, Param_t params[], Param_t **restrict retval, const uint32_t argc, TaghaVM_t *env)
{
	void *ptr = params[0].Ptr;
	if( ptr ) {
		puts("native_free :: ptr is VALID, freeing...\n");
		free(ptr), ptr=NULL;
	}
	*retval = NULL;
}

/* void *calloc(size_t num, size_t size); */
static void native_calloc(Script_t *script, Param_t params[], Param_t **restrict retval, const uint32_t argc, TaghaVM_t *env)
{
	const uint64_t num = params[0].UInt64;
	const uint64_t size = params[1].UInt64;
	
	(*retval)->Ptr = calloc(num, size);
}

/* void *realloc(void *ptr, size_t size); */
static void native_realloc(Script_t *script, Param_t params[], Param_t **restrict retval, const uint32_t argc, TaghaVM_t *env)
{
	void *ptr = params[0].Ptr;
	const uint64_t size = params[1].UInt64;
	
	(*retval)->Ptr = realloc(ptr, size);
}

/* void srand(unsigned int seed); */
static void native_srand(Script_t *script, Param_t params[], Param_t **restrict retval, const uint32_t argc, TaghaVM_t *env)
{
	*retval = NULL;
	srand(params[0].UInt32);
}

/* int rand(void); */
static void native_rand(Script_t *script, Param_t params[], Param_t **restrict retval, const uint32_t argc, TaghaVM_t *env)
{
	(*retval)->Int32 = rand();
}

/* double strtod(const char *str, char **endptr); */
static void native_strtod(Script_t *script, Param_t params[], Param_t **restrict retval, const uint32_t argc, TaghaVM_t *env)
{
	const char *str = params[0].String;
	if( !str ) {
		puts("strtod reported :: **** param 'str' is NULL ****\n");
		(*retval)->Double = 0.0;
		return;
	}
	(*retval)->Double = strtod(str, params[1].Ptr);
}

/* float strtof(const char *str, char **endptr); */
static void native_strtof(Script_t *script, Param_t params[], Param_t **restrict retval, const uint32_t argc, TaghaVM_t *env)
{
	const char *str = params[0].String;
	if( !str ) {
		puts("strtof reported :: **** param 'str' is NULL ****\n");
		(*retval)->Float = 0.f;
		return;
	}
	(*retval)->Float = strtof(str, params[1].Ptr);
}

/* long int strtol(const char *str, char **endptr, int base); */
static void native_strtol(Script_t *script, Param_t params[], Param_t **restrict retval, const uint32_t argc, TaghaVM_t *env)
{
	const char *str = params[0].String;
	if( !str ) {
		puts("strtol reported :: **** param 'str' is NULL ****\n");
		(*retval)->Int32 = 0;
		return;
	}
	(*retval)->Int32 = strtol(str, params[1].Ptr, params[2].Int32);
}

/* long long int strtoll(const char *str, char **endptr, int base); */
static void native_strtoll(Script_t *script, Param_t params[], Param_t **restrict retval, const uint32_t argc, TaghaVM_t *env)
{
	const char *str = params[0].String;
	if( !str ) {
		puts("strtoll reported :: **** param 'str' is NULL ****\n");
		(*retval)->Int64 = 0L;
		return;
	}
	(*retval)->Int64 = strtoll(str, params[1].Ptr, params[2].Int32);
}

/* unsigned long int strtoul(const char *str, char **endptr, int base); */
static void native_strtoul(Script_t *script, Param_t params[], Param_t **restrict retval, const uint32_t argc, TaghaVM_t *env)
{
	const char *str = params[0].String;
	if( !str ) {
		puts("strtoul reported :: **** param 'str' is NULL ****\n");
		(*retval)->UInt32 = 0;
		return;
	}
	(*retval)->UInt32 = strtoul(str, params[1].Ptr, params[2].Int32);
}

/* unsigned long long int strtoull(const char *str, char **endptr, int base); */
static void native_strtoull(Script_t *script, Param_t params[], Param_t **restrict retval, const uint32_t argc, TaghaVM_t *env)
{
	const char *str = params[0].String;
	if( !str ) {
		puts("strtoull reported :: **** param 'str' is NULL ****\n");
		(*retval)->UInt64 = 0L;
		return;
	}
	(*retval)->UInt64 = strtoll(str, params[1].Ptr, params[2].Int32);
}

/* void abort(void); */
static void native_abort(Script_t *script, Param_t params[], Param_t **restrict retval, const uint32_t argc, TaghaVM_t *env)
{
	*retval = NULL;
	*script->m_Regs[rip].UCharPtr = halt;
}

/* void exit(int status); */
static void native_exit(Script_t *script, Param_t params[], Param_t **restrict retval, const uint32_t argc, TaghaVM_t *env)
{
	*retval = NULL;
	*script->m_Regs[rip].UCharPtr = halt;
	printf("Exiting with status: %i\n", params[0].Int32);
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
		{"srand", native_srand},
		{"rand", native_rand},
		{"strtod", native_strtod},
		{"strtof", native_strtof},
		{"strtol", native_strtol},
		{"strtoll", native_strtoll},
		{"strtoul", native_strtoul},
		{"strtoull", native_strtoull},
		{"abort", native_abort},
		{"exit", native_exit},
		{NULL, NULL}
	};
	Tagha_register_natives(vm, libc_stdlib_natives);
}











