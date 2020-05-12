#include <stdlib.h>
#include "tagha_libc.h"


/** void *malloc(size_t size); */
static union TaghaVal native_malloc(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	/** size_t is 8 bytes on 64-bit systems */
	//return (union TaghaVal){ .uintptr = calloc(1, params[0].uint64) };
	return (union TaghaVal){ .uintptr = ( uintptr_t )harbol_mempool_alloc(&module->heap, params[0].uint64) };
}

/** void free(void *ptr); */
static union TaghaVal native_free(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	//free(params[0].uintptr);
	harbol_mempool_free(&module->heap, ( void* )params[0].uintptr);
	return (union TaghaVal){ 0 };
}

/** non-standard addition.
 * bool safe_free(void *ptrref);
 */
static union TaghaVal native_safe_free(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	void **restrict ptrref = ( void** )params[0].uintptr;
	if( *ptrref ) {
		//free(*ptrref), *ptrref=NULL;
		harbol_mempool_free(&module->heap, *ptrref); *ptrref=NULL;
		return (union TaghaVal){ .b00l = true };
	}
	return (union TaghaVal){ 0 };
}

/** void *calloc(size_t num, size_t size); */
static union TaghaVal native_calloc(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	//return (union TaghaVal){ .uintptr = calloc(params[0].uint64, params[1].uint64) };
	return (union TaghaVal){ .uintptr = ( uintptr_t )harbol_mempool_alloc(&module->heap, params[0].uint64 * params[1].uint64) };
}

/** void *realloc(void *ptr, size_t size); */
static union TaghaVal native_realloc(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	//return (union TaghaVal){ .uintptr = realloc(params[0].uintptr, params[1].uint64) };
	return (union TaghaVal){ .uintptr = ( uintptr_t )harbol_mempool_realloc(&module->heap, ( void* )params[0].uintptr, params[1].uint64) };
}

/** void *alloca(size_t size); */
static union TaghaVal native_alloca(struct TaghaModule *const restrict module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)args;
	union TaghaVal ret = {0};
	const size_t cells = harbol_align_size(params[0].uint64, sizeof(union TaghaVal));
	if( module->regs[sp].uintptr - cells < ( uintptr_t )module->stack ) {
		module->errcode = tagha_err_stk_overflow;
		ret.uintptr = ( uintptr_t )NULL;
	} else {
		module->regs[sp].uintptr -= cells;
		ret.uintptr = module->regs[sp].uintptr;
	}
	return ret;
}

/** void srand(unsigned int seed); */
static union TaghaVal native_srand(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	srand(params[0].uint32);
	return (union TaghaVal){ 0 };
}

/** int rand(void); */
static union TaghaVal native_rand(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args; (void)params;
	return (union TaghaVal){ .int32 = rand() };
}

/** float64_t atof(const char *str); */
static union TaghaVal native_atof(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .float64 = atof(( const char* )params[0].uintptr) };
}

/** int atoi(const char *str); */
static union TaghaVal native_atoi(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .int32 = atoi(( const char* )params[0].uintptr) };
}

/** long int atol(const char *str); */
static union TaghaVal native_atol(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .int64 = atol(( const char* )params[0].uintptr) };
}

/** long long int atoll(const char *str); */
static union TaghaVal native_atoll(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .int64 = atoll(( const char* )params[0].uintptr) };
}

/** float64_t strtod(const char *str, char **endptr); */
static union TaghaVal native_strtod(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	const char *restrict str = ( const char* )params[0].uintptr;
	return (union TaghaVal){ .float64 = strtod(str, ( char** )params[1].uintptr) };
}

/** float32_t strtof(const char *str, char **endptr); */
static union TaghaVal native_strtof(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	const char *restrict str = ( const char* )params[0].uintptr;
	return (union TaghaVal){ .float32 = strtof(str, ( char** )params[1].uintptr) };
}

/** long int strtol(const char *str, char **endptr, int base); */
static union TaghaVal native_strtol(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	const char *restrict str = ( const char* )params[0].uintptr;
	return (union TaghaVal){ .int64 = strtol(str, ( char** )params[1].uintptr, params[2].int32) };
}

/** long long int strtoll(const char *str, char **endptr, int base); */
static union TaghaVal native_strtoll(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	const char *restrict str = ( const char* )params[0].uintptr;
	return (union TaghaVal){ .int64 = strtoll(str, ( char** )params[1].uintptr, params[2].int32) };
}

/** unsigned long int strtoul(const char *str, char **endptr, int base); */
static union TaghaVal native_strtoul(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	const char *restrict str = ( const char* )params[0].uintptr;
	return (union TaghaVal){ .uint64 = strtoul(str, ( char** )params[1].uintptr, params[2].int32) };
}

/** unsigned long long int strtoull(const char *str, char **endptr, int base); */
static union TaghaVal native_strtoull(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	const char *restrict str = ( const char* )params[0].uintptr;
	return (union TaghaVal){ .uint64 = strtoll(str, ( char** )params[1].uintptr, params[2].int32) };
}

/** void abort(void); */
static union TaghaVal native_abort(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args; (void)params;
	abort();
	return (union TaghaVal){ 0 };
}

/** void exit(int status); */
/// TODO: Redo this since it's a no return.
static union TaghaVal native_exit(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args; (void)params;
	exit(params[0].int32);
	return (union TaghaVal){ 0 };
}

/** int system(const char *command); */
static union TaghaVal native_system(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	const char *command = ( const char* )params[0].uintptr;
	return (union TaghaVal){ .int32 = ( command==NULL ) ? -1 : system(command) };
}

/** int abs(int n); */
static union TaghaVal native_abs(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .int32 = abs(params[0].int32) };
}

/** long int labs(long int n); */
static union TaghaVal native_labs(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .int64 = labs(params[0].int64) };
}

/** long long int llabs(long long int n); */
static union TaghaVal native_llabs(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .int64 = llabs(params[0].int64) };
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
static union TaghaVal native_div(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
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

static union TaghaVal native_ldiv(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
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
static union TaghaVal native_lldiv(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	lldiv_t *const res = ( lldiv_t* )params[0].uintptr;
	*res = lldiv(params[1].int64, params[2].int64);
	return (union TaghaVal){ 0 };
}

/** int mblen(const char *pmb, size_t max); */
static union TaghaVal native_mblen(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .int32 = mblen(( const char* )params[0].uintptr, params[1].uint64) };
}

/** int mbtowc(wchar_t *pwc, const char *pmb, size_t max); */
static union TaghaVal native_mbtowc(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .int32 = mbtowc(( wchar_t* )params[0].uintptr, ( const char* )params[1].uintptr, params[2].uint64) };
}

/** int wctomb(char *pmb, wchar_t wc); */
static union TaghaVal native_wctomb(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .int32 = wctomb(( char* )params[0].uintptr, sizeof(wchar_t)==2 ? params[1].uint16 : params[1].uint32) };
}

/** size_t mbstowcs(wchar_t *dest, const char *src, size_t max); */
static union TaghaVal native_mbstowcs(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .uint64 = mbstowcs(( wchar_t* )params[0].uintptr, ( const char* )params[1].uintptr, params[2].uint64) };
}

/** size_t wcstombs(char *dest, const wchar_t *src, size_t max); */
static union TaghaVal native_wcstombs(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .uint64 = wcstombs(( char* )params[0].uintptr, ( const wchar_t* )params[1].uintptr, params[2].uint64) };
}


/// TODO:
/** void qsort(void *base, size_t num, size_t size, int (*cmp)(const void *, const void *)); */
static union TaghaVal native_qsort(struct TaghaModule *const restrict module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args; (void)params;
	/*
	void *const restrict array_base = ( void* )params[0].uintptr;
	const size_t num_elements = params[1].size;
	const size_t element_bytes = params[2].size;
	const void *func_ptr = ( const void* )params[3].uintptr;
	union TaghaVal ret = {0};
	tagha_module_invoke(module, func_ptr, 2, const union TaghaVal params[], &ret);
	*/
	return (union TaghaVal){ 0 };
}

/** int atexit(void (*func)(void)); */
static union TaghaVal native_atexit(struct TaghaModule *const restrict module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args; (void)params;
	/*
	const void *func_ptr = ( const void* )params[0].uintptr;
	*/
	return (union TaghaVal){ .int32 = -1 };
}

/** void *bsearch(const void *key, const void *base, size_t num, size_t size, int (*cmp)(const void *,const void *)); */
static union TaghaVal native_bsearch(struct TaghaModule *const restrict module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args; (void)params;
	/*
	const void *const restrict key = ( const void* )params[0].uintptr;
	const void *const restrict base = ( const void* )params[1].uintptr;
	const size_t num = params[2].size;
	const size_t size = params[3].size;
	const void *const restrict func_ptr = ( const void* )params[4].uintptr;
	union TaghaVal ret = {0};
	tagha_module_invoke(module, func_ptr, 2, const union TaghaVal params[], &ret);
	*/
	return (union TaghaVal){ .uintptr = ( uintptr_t )NULL };
}

/** int at_quick_exit(void (*func)(void)); */
static union TaghaVal native_at_quick_exit(struct TaghaModule *const restrict module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args; (void)params;
	/*
	const void *func_ptr = ( const void* )params[0].uintptr;
	*/
	return (union TaghaVal){ .int32 = -1 };
}

/** char *getenv(const char *name); */
static union TaghaVal native_getenv(struct TaghaModule *const restrict module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .uintptr = ( uintptr_t )getenv(( const char* )params[0].uintptr) };
}


bool tagha_module_load_stdlib_natives(struct TaghaModule *const module)
{
	const struct TaghaNative libc_stdlib_natives[] = {
		{"malloc", native_malloc},
		{"free", native_free},
		{"safe_free", native_safe_free},
		{"calloc", native_calloc},
		{"realloc", native_realloc},
		{"srand", native_srand},
		{"rand", native_rand},
		{"atof", native_atof},
		{"atoi", native_atoi},
		{"atol", native_atol},
		{"atoll", native_atoll},
		{"strtod", native_strtod},
		{"strtof", native_strtof},
		{"strtol", native_strtol},
		{"strtoll", native_strtoll},
		{"strtoul", native_strtoul},
		{"strtoull", native_strtoull},
		{"abort", native_abort},
		{"exit", native_exit},
		{"system", native_system},
		{"abs", native_abs},
		{"labs", native_labs},
		{"llabs", native_llabs},
		{"div", native_div},
		{"ldiv", native_lldiv}, //native_ldiv}, long and long long are both 8 bytes for Tagha...
		{"lldiv", native_lldiv},
		{"mblen", native_mblen},
		{"mbtowc", native_mbtowc},
		{"wctomb", native_wctomb},
		{"mbstowcs", native_mbstowcs},
		{"wcstombs", native_wcstombs},
		{"atexit", native_atexit},
		{"at_quick_exit", native_at_quick_exit},
		{"getenv", native_getenv},
		{NULL, NULL}
	};
	return module ? tagha_module_register_natives(module, libc_stdlib_natives) : false;
}
