#include <stdlib.h>
#include "tagha_libc.h"

/* void *malloc(size_t size); */
static void native_malloc(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	(void)module; (void)argc;
	/* size_t is 8 bytes on 64-bit systems */
	retval->Ptr = calloc(1, params[0].UInt64);
}

/* void free(void *ptr); */
static void native_free(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	(void)module; (void)argc; (void)retval;
	free(params[0].Ptr);
}

/* non-standard addition.
	bool safe_free(void *ptrref);
*/
static void native_safe_free(struct TaghaModule *const restrict module, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	(void)module; (void)argc;
	void **restrict ptrref = params[0].Ptr;
	if( *ptrref ) {
		free(*ptrref), *ptrref=NULL;
		retval->Bool = true;
	}
}

/* void *calloc(size_t num, size_t size); */
static void native_calloc(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	(void)module; (void)argc;
	retval->Ptr = calloc(params[0].UInt64, params[1].UInt64);
}

/* void *realloc(void *ptr, size_t size); */
static void native_realloc(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	(void)module; (void)argc;
	retval->Ptr = realloc(params[0].Ptr, params[1].UInt64);
}

/* void srand(unsigned int seed); */
static void native_srand(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	(void)module; (void)argc; (void)retval;
	srand(params[0].UInt32);
}

/* int rand(void); */
static void native_rand(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	(void)module; (void)argc; (void)params;
	retval->Int32 = rand();
}

/* double atof(const char *str); */
static void native_atof(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	(void)module; (void)argc;
	retval->Double = atof(params[0].Ptr);
}

/* int atoi(const char *str); */
static void native_atoi(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	(void)module; (void)argc;
	retval->Int32 = atoi(params[0].Ptr);
}

/* long int atol(const char *str); */
static void native_atol(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	(void)module; (void)argc;
	retval->Int64 = atol(params[0].Ptr);
}

/* long long int atoll(const char *str); */
static void native_atoll(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	(void)module; (void)argc;
	retval->Int64 = atoll(params[0].Ptr);
}

/* double strtod(const char *str, char **endptr); */
static void native_strtod(struct TaghaModule *const restrict module, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	(void)module; (void)argc;
	const char *restrict str = params[0].Ptr;
	if( !str )
		return;
	retval->Double = strtod(str, params[1].Ptr);
}

/* float strtof(const char *str, char **endptr); */
static void native_strtof(struct TaghaModule *const restrict module, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	(void)module; (void)argc;
	const char *restrict str = params[0].Ptr;
	if( !str )
		return;
	retval->Float = strtof(str, params[1].Ptr);
}

/* long int strtol(const char *str, char **endptr, int base); */
static void native_strtol(struct TaghaModule *const restrict module, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	(void)module; (void)argc;
	const char *restrict str = params[0].Ptr;
	if( !str )
		return;
	retval->Int32 = strtol(str, params[1].Ptr, params[2].Int32);
}

/* long long int strtoll(const char *str, char **endptr, int base); */
static void native_strtoll(struct TaghaModule *const restrict module, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	(void)module; (void)argc;
	const char *restrict str = params[0].Ptr;
	if( !str )
		return;
	retval->Int64 = strtoll(str, params[1].Ptr, params[2].Int32);
}

/* unsigned long int strtoul(const char *str, char **endptr, int base); */
static void native_strtoul(struct TaghaModule *const restrict module, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	(void)module; (void)argc;
	const char *restrict str = params[0].Ptr;
	if( !str )
		return;
	retval->UInt64 = strtoul(str, params[1].Ptr, params[2].Int32);
}

/* unsigned long long int strtoull(const char *str, char **endptr, int base); */
static void native_strtoull(struct TaghaModule *const restrict module, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	(void)module; (void)argc;
	const char *restrict str = params[0].Ptr;
	if( !str )
		return;
	retval->UInt64 = strtoll(str, params[1].Ptr, params[2].Int32);
}

/* void abort(void); */
static void native_abort(struct TaghaModule *const restrict module, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	(void)module; (void)argc; (void)params; (void)retval;
	abort();
}

/* void exit(int status); */
static void native_exit(struct TaghaModule *const restrict module, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	(void)module; (void)argc; (void)params; (void)retval;
	exit(params[0].Int32);
}

/* int system(const char *command); */
static void native_system(struct TaghaModule *const restrict module, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	(void)module; (void)argc;
	const char *restrict command = params[0].Ptr;
	if( !command ) {
		retval->Int32 = -1;
		return;
	}
	retval->Int32 = system(command);
}

/* int abs(int n); */
static void native_abs(struct TaghaModule *const restrict module, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	(void)module; (void)argc;
	retval->Int32 = abs(params[0].Int32);
}

/* long int labs(long int n); */
static void native_labs(struct TaghaModule *const restrict module, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	(void)module; (void)argc;
	retval->Int64 = labs(params[0].Int64);
}

/* long long int llabs(long long int n); */
static void native_llabs(struct TaghaModule *const restrict module, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	(void)module; (void)argc;
	retval->Int64 = llabs(params[0].Int64);
}

/*
div_t div(int numer, int denom);
typedef struct {
	int quot;	// quotient, 1st 32 bits.
	int rem;	// remainder, 2nd 32 bits.
} div_t;
now how the fuck do we return a damn struct?
the registers are large enough to have an 8-byte struct but what choice will compilers take?
One wrong move could render this native function as undefined behavior.
*/
static void native_div(struct TaghaModule *const restrict module, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	(void)module; (void)argc;
	const div_t divres = div(params[1].Int32, params[2].Int32);
	memcpy(retval, &divres, sizeof divres);
}

/*
ldiv_t ldiv(long int numer, long int denom);
typedef struct {
	long int quot;
	long int rem;
} ldiv_t;

static void native_ldiv(struct TaghaModule *const restrict module, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	lldiv_t *res = params[0].Ptr;
	*res = lldiv(params[1].Int64, params[2].Int64);
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
static void native_lldiv(struct TaghaModule *const restrict module, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	(void)module; (void)argc; (void)retval;
	lldiv_t *const res = params[0].Ptr;
	*res = lldiv(params[1].Int64, params[2].Int64);
}

/* int mblen(const char *pmb, size_t max); */
static void native_mblen(struct TaghaModule *const restrict module, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	(void)module; (void)argc;
	retval->Int32 = mblen(params[0].Ptr, params[1].UInt64);
}

/* int mbtowc(wchar_t *pwc, const char *pmb, size_t max); */
static void native_mbtowc(struct TaghaModule *const restrict module, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	(void)module; (void)argc;
	retval->Int32 = mbtowc(params[0].Ptr, params[1].Ptr, params[2].UInt64);
}

/* int wctomb(char *pmb, wchar_t wc); */
static void native_wctomb(struct TaghaModule *const restrict module, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	(void)module; (void)argc;
	retval->Int32 = wctomb(params[0].Ptr, sizeof(wchar_t)==2 ? params[1].UInt16 : params[1].UInt32);
}

/* size_t mbstowcs(wchar_t *dest, const char *src, size_t max); */
static void native_mbstowcs(struct TaghaModule *const restrict module, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	(void)module; (void)argc;
	retval->UInt64 = mbstowcs(params[0].Ptr, params[1].Ptr, params[2].UInt64);
}

/* size_t wcstombs(char *dest, const wchar_t *src, size_t max); */
static void native_wcstombs(struct TaghaModule *const restrict module, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	(void)module; (void)argc;
	retval->UInt64 = wcstombs(params[0].Ptr, params[1].Ptr, params[2].UInt64);
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
		{NULL, NULL}
	};
	return module ? tagha_module_register_natives(module, libc_stdlib_natives) : false;
}
