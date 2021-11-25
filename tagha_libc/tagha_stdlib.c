#include <stdlib.h>
#include "../tagha/tagha.h"


/** void *malloc(size_t size); */
static union TaghaVal native_malloc(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	/** size_t is 8 bytes on 64-bit systems */
	//return ( union TaghaVal ){ .uintptr = calloc(1, params[0].uint64) };
	return ( union TaghaVal ){ .uintptr = tagha_module_heap_alloc(module, params[0].uint64) };
}

/** void free(void *ptr); */
static union TaghaVal native_free(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	//free(params[0].uintptr);
	tagha_module_heap_free(module, params[0].uintptr);
	return ( union TaghaVal ){ 0 };
}

/** non-standard addition.
 * bool safe_free(void **ptrref);
 */
static union TaghaVal native_safe_free(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	uintptr_t *const restrict ptrref = ( uintptr_t* )params[0].uintptr;
	if( *ptrref != NULL ) {
		tagha_module_heap_free(module, *ptrref); *ptrref=NIL;
		return ( union TaghaVal ){ .b00l = true };
	}
	return ( union TaghaVal ){ 0 };
}

/** void *calloc(size_t num, size_t size); */
static union TaghaVal native_calloc(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	//return ( union TaghaVal ){ .uintptr = calloc(params[0].uint64, params[1].uint64) };
	return ( union TaghaVal ){ .uintptr = tagha_module_heap_alloc(module, params[0].uint64 * params[1].uint64) };
}

/** void *realloc(void *ptr, size_t size); */
static union TaghaVal native_realloc(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	//return ( union TaghaVal ){ .uintptr = realloc(params[0].uintptr, params[1].uint64) };
	return ( union TaghaVal ){ .uintptr = ( uintptr_t )harbol_mempool_realloc(&module->heap, ( void* )params[0].uintptr, params[1].uint64) };
}

/** void *alloca(size_t size); */
/**
 * WARNING: POTENTIALLY DANGEROUS.
 * If a dev uses `alloca`, compiler could possibly allocate its data that is already in use.
 * Whole point of `alloca` is to have a form of dynamic allocation without resorting to `malloc` + friends and then having to free.
 * This implementation of `alloca` does NOT modify the (operand) stack pointer but can cause issues if `alloca` is called and the compiler then allocates additional registers or if a callee function needs registers itself as well which could corrupt data.
 */
/*
static union TaghaVal native_alloca(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	const size_t len = params[0].size;
	const size_t aligned_len = (len + (sizeof(union TaghaVal)-1)) & -sizeof(union TaghaVal);
	const uintptr_t alloc_space = module->osp - aligned_len;
	return ( union TaghaVal ){ .uintptr = (alloc_space < module->opstack) ? NIL : alloc_space };
}
*/

/** void srand(unsigned int seed); */
static union TaghaVal native_srand(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	srand(params[0].uint32);
	return ( union TaghaVal ){ 0 };
}

/** int rand(void); */
static union TaghaVal native_rand(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module; ( void )params;
	return ( union TaghaVal ){ .int32 = rand() };
}

/** double atof(const char *str); */
static union TaghaVal native_atof(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .float64 = atof(( const char* )params[0].uintptr) };
}

/** int atoi(const char *str); */
static union TaghaVal native_atoi(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .int32 = atoi(( const char* )params[0].uintptr) };
}

/** long int atol(const char *str); */
static union TaghaVal native_atol(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .int64 = atol(( const char* )params[0].uintptr) };
}

/** long long int atoll(const char *str); */
static union TaghaVal native_atoll(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .int64 = atoll(( const char* )params[0].uintptr) };
}

/** double strtod(const char *str, char **endptr); */
static union TaghaVal native_strtod(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	const char *restrict str = ( const char* )params[0].uintptr;
	return ( union TaghaVal ){ .float64 = strtod(str, ( char** )params[1].uintptr) };
}

/** float strtof(const char *str, char **endptr); */
static union TaghaVal native_strtof(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	const char *restrict str = ( const char* )params[0].uintptr;
	return ( union TaghaVal ){ .float32 = strtof(str, ( char** )params[1].uintptr) };
}

/** long int strtol(const char *str, char **endptr, int base); */
static union TaghaVal native_strtol(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	const char *restrict str = ( const char* )params[0].uintptr;
	return ( union TaghaVal ){ .int64 = strtol(str, ( char** )params[1].uintptr, params[2].int32) };
}

/** long long int strtoll(const char *str, char **endptr, int base); */
static union TaghaVal native_strtoll(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	const char *restrict str = ( const char* )params[0].uintptr;
	return ( union TaghaVal ){ .int64 = strtoll(str, ( char** )params[1].uintptr, params[2].int32) };
}

/** unsigned long int strtoul(const char *str, char **endptr, int base); */
static union TaghaVal native_strtoul(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	const char *restrict str = ( const char* )params[0].uintptr;
	return ( union TaghaVal ){ .uint64 = strtoul(str, ( char** )params[1].uintptr, params[2].int32) };
}

/** unsigned long long int strtoull(const char *str, char **endptr, int base); */
static union TaghaVal native_strtoull(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	const char *restrict str = ( const char* )params[0].uintptr;
	return ( union TaghaVal ){ .uint64 = strtoll(str, ( char** )params[1].uintptr, params[2].int32) };
}

/** void abort(void); */
static union TaghaVal native_abort(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module; ( void )params;
	abort();
	return ( union TaghaVal ){ 0 };
}

/** void exit(int status); */
/// TODO: Redo this since it's a no return.
static union TaghaVal native_exit(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module; ( void )params;
	exit(params[0].int32);
	return ( union TaghaVal ){ 0 };
}

/** int system(const char *command); */
static union TaghaVal native_system(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	const char *command = ( const char* )params[0].uintptr;
	return ( union TaghaVal ){ .int32 = ( command==NULL ) ? -1 : system(command) };
}

/** int abs(int n); */
static union TaghaVal native_abs(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .int32 = abs(params[0].int32) };
}

/** long int labs(long int n); */
static union TaghaVal native_labs(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .int64 = labs(params[0].int64) };
}

/** long long int llabs(long long int n); */
static union TaghaVal native_llabs(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .int64 = llabs(params[0].int64) };
}

/*
div_t div(int numer, int denom);
typedef struct {
	int quot;	/// quotient, 1st 32 bits.
	int rem;	/// remainder, 2nd 32 bits.
} div_t;
now how the fuck do we return a damn struct?
the registers are large enough to have an 8-byte struct but what choice will compilers take?
One wrong move could render this native function as undefined behavior.
*/
static union TaghaVal native_div(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	union {
		const div_t divres;
		const union TaghaVal t;
	} conv = { div(params[1].int32, params[2].int32) };
	return conv.t;
}

/*
ldiv_t ldiv(long int numer, long int denom);
typedef struct {
	long int quot;
	long int rem;
} ldiv_t;

static union TaghaVal native_ldiv(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	lldiv_t *res = params[0].uintptr;
	*res = lldiv(params[1].int64, params[2].int64);
}
*/

/*
lldiv_t lldiv(long long int numer, long long int denom);
typedef struct {
	long long int quot;
	long long int rem;
} lldiv_t;
void lldiv(lldiv_t *res, long long int numer, long long int denom);
*/
static union TaghaVal native_lldiv(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	lldiv_t *const res = ( lldiv_t* )params[0].uintptr;
	*res = lldiv(params[1].int64, params[2].int64);
	return ( union TaghaVal ){ 0 };
}

/** int mblen(const char *pmb, size_t max); */
static union TaghaVal native_mblen(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .int32 = mblen(( const char* )params[0].uintptr, params[1].uint64) };
}

/** int mbtowc(wchar_t *pwc, const char *pmb, size_t max); */
static union TaghaVal native_mbtowc(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .int32 = mbtowc(( wchar_t* )params[0].uintptr, ( const char* )params[1].uintptr, params[2].uint64) };
}

/** int wctomb(char *pmb, wchar_t wc); */
static union TaghaVal native_wctomb(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .int32 = wctomb(( char* )params[0].uintptr, sizeof(wchar_t)==2 ? params[1].uint16 : params[1].uint32) };
}

/** size_t mbstowcs(wchar_t *dest, const char *src, size_t max); */
static union TaghaVal native_mbstowcs(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .uint64 = mbstowcs(( wchar_t* )params[0].uintptr, ( const char* )params[1].uintptr, params[2].uint64) };
}

/** size_t wcstombs(char *dest, const wchar_t *src, size_t max); */
static union TaghaVal native_wcstombs(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .uint64 = wcstombs(( char* )params[0].uintptr, ( const wchar_t* )params[1].uintptr, params[2].uint64) };
}


/// TODO:
/** void qsort(void *base, size_t num, size_t size, int (*cmp)(const void *, const void *)); */
static union TaghaVal native_qsort(struct TaghaModule *const restrict module, const union TaghaVal params[const static 1])
{
	( void )module; ( void )params;
	/*
	void *const restrict array_base = ( void* )params[0].uintptr;
	const size_t num_elements = params[1].size;
	const size_t element_bytes = params[2].size;
	const void *func_ptr = ( const void* )params[3].uintptr;
	tagha_module_invoke(module, func_ptr, 2, const union TaghaVal params[], &ret);
	*/
	return ( union TaghaVal ){ 0 };
}

/** int atexit(void (*func)( void )); */
static union TaghaVal native_atexit(struct TaghaModule *const restrict module, const union TaghaVal params[const static 1])
{
	( void )module; ( void )params;
	/*
	const void *func_ptr = ( const void* )params[0].uintptr;
	*/
	return ( union TaghaVal ){ .int32 = -1 };
}

/** void *bsearch(const void *key, const void *base, size_t num, size_t size, int (*cmp)(const void *,const void *)); */
static union TaghaVal native_bsearch(struct TaghaModule *const restrict module, const union TaghaVal params[const static 1])
{
	( void )module; ( void )params;
	/*
	const void *const restrict key = ( const void* )params[0].uintptr;
	const void *const restrict base = ( const void* )params[1].uintptr;
	const size_t num = params[2].size;
	const size_t size = params[3].size;
	const void *const restrict func_ptr = ( const void* )params[4].uintptr;
	union TaghaVal ret = {0};
	tagha_module_invoke(module, func_ptr, 2, const union TaghaVal params[], &ret);
	*/
	return ( union TaghaVal ){ .uintptr = ( uintptr_t )NULL };
}

/** int at_quick_exit(void (*func)( void )); */
static union TaghaVal native_at_quick_exit(struct TaghaModule *const restrict module, const union TaghaVal params[const static 1])
{
	( void )module; ( void )params;
	/*
	const void *func_ptr = ( const void* )params[0].uintptr;
	*/
	return ( union TaghaVal ){ .int32 = -1 };
}

/** char *getenv(const char *name); */
static union TaghaVal native_getenv(struct TaghaModule *const restrict module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .uintptr = ( uintptr_t )getenv(( const char* )params[0].uintptr) };
}


bool tagha_module_load_stdlib_natives(struct TaghaModule *const module)
{
	
	return module ? tagha_module_register_natives(module, ( const struct TaghaNative[] ){
		{"__tagha_malloc",        &native_malloc},
		{"__tagha_free",          &native_free},
		{"__tagha_safe_free",     &native_safe_free},
		{"__tagha_calloc",        &native_calloc},
		{"__tagha_realloc",       &native_realloc},
		{"__tagha_srand",         &native_srand},
		{"__tagha_rand",          &native_rand},
		{"__tagha_atof",          &native_atof},
		{"__tagha_atoi",          &native_atoi},
		{"__tagha_atol",          &native_atol},
		{"__tagha_atoll",         &native_atoll},
		{"__tagha_strtod",        &native_strtod},
		{"__tagha_strtof",        &native_strtof},
		{"__tagha_strtol",        &native_strtol},
		{"__tagha_strtoll",       &native_strtoll},
		{"__tagha_strtoul",       &native_strtoul},
		{"__tagha_strtoull",      &native_strtoull},
		{"__tagha_abort",         &native_abort},
		{"__tagha_exit",          &native_exit},
		{"__tagha_system",        &native_system},
		{"__tagha_abs",           &native_abs},
		{"__tagha_labs",          &native_labs},
		{"__tagha_llabs",         &native_llabs},
		{"__tagha_div",           &native_div},
		{"__tagha_ldiv",          &native_lldiv}, //native_ldiv}, long and long long are both 8 bytes for Tagha...
		{"__tagha_lldiv",         &native_lldiv},
		{"__tagha_mblen",         &native_mblen},
		{"__tagha_mbtowc",        &native_mbtowc},
		{"__tagha_wctomb",        &native_wctomb},
		{"__tagha_mbstowcs",      &native_mbstowcs},
		{"__tagha_wcstombs",      &native_wcstombs},
		{"__tagha_atexit",        &native_atexit},
		{"__tagha_at_quick_exit", &native_at_quick_exit},
		{"__tagha_getenv",        &native_getenv},
		{NULL,                    NULL}
	}) : false;
}