
#include <stdlib.h>

/* void *malloc(size_t size); */
static void native_malloc(struct Tagha *const restrict sys, union TaghaVal *const restrict RetVal, const size_t argc, union TaghaVal params[restrict static argc])
{
	// size_t is 8 bytes on 64-bit systems
	RetVal->Ptr = calloc(1, params[0].UInt64);
}

/* void free(void *ptr); */
static void native_free(struct Tagha *const restrict sys, union TaghaVal *const restrict RetVal, const size_t argc, union TaghaVal params[restrict static argc])
{
	free(params[0].Ptr);
}

/* non-standard addition. */
/* void safe_free(void **pptr); */
static void native_safe_free(struct Tagha *const restrict sys, union TaghaVal *const restrict RetVal, const size_t argc, union TaghaVal params[restrict static argc])
{
	void **restrict pptr = params[0].Ptr;
	if( *pptr ) {
		free(*pptr), *pptr=NULL;
	}
}

/* void *calloc(size_t num, size_t size); */
static void native_calloc(struct Tagha *const restrict sys, union TaghaVal *const restrict RetVal, const size_t argc, union TaghaVal params[restrict static argc])
{
	RetVal->Ptr = calloc(params[0].UInt64, params[1].UInt64);
}

/* void *realloc(void *ptr, size_t size); */
static void native_realloc(struct Tagha *const restrict sys, union TaghaVal *const restrict RetVal, const size_t argc, union TaghaVal params[restrict static argc])
{
	RetVal->Ptr = realloc(params[0].Ptr, params[1].UInt64);
}

/* void srand(unsigned int seed); */
static void native_srand(struct Tagha *const restrict sys, union TaghaVal *const restrict RetVal, const size_t argc, union TaghaVal params[restrict static argc])
{
	srand(params[0].UInt32);
}

/* int rand(void); */
static void native_rand(struct Tagha *const restrict sys, union TaghaVal *const restrict RetVal, const size_t argc, union TaghaVal params[restrict static argc])
{
	RetVal->Int32 = rand();
}

/* double atof(const char *str); */
static void native_atof(struct Tagha *const restrict sys, union TaghaVal *const restrict RetVal, const size_t argc, union TaghaVal params[restrict static argc])
{
	RetVal->Double = atof(params[0].Ptr);
}

/* int atoi(const char *str); */
static void native_atoi(struct Tagha *const restrict sys, union TaghaVal *const restrict RetVal, const size_t argc, union TaghaVal params[restrict static argc])
{
	RetVal->Int32 = atoi(params[0].Ptr);
}

/* long int atol(const char *str); */
static void native_atol(struct Tagha *const restrict sys, union TaghaVal *const restrict RetVal, const size_t argc, union TaghaVal params[restrict static argc])
{
	RetVal->Int64 = atol(params[0].Ptr);
}

/* long long int atoll(const char *str); */
static void native_atoll(struct Tagha *const restrict sys, union TaghaVal *const restrict RetVal, const size_t argc, union TaghaVal params[restrict static argc])
{
	RetVal->Int64 = atoll(params[0].Ptr);
}

/* double strtod(const char *str, char **endptr); */
static void native_strtod(struct Tagha *const restrict sys, union TaghaVal *const restrict RetVal, const size_t argc, union TaghaVal params[restrict static argc])
{
	const char *restrict str = params[0].Ptr;
	if( !str )
		return;
	RetVal->Double = strtod(str, (char **)params[1].PtrPtr);
}

/* float strtof(const char *str, char **endptr); */
static void native_strtof(struct Tagha *const restrict sys, union TaghaVal *const restrict RetVal, const size_t argc, union TaghaVal params[restrict static argc])
{
	const char *restrict str = params[0].Ptr;
	if( !str )
		return;
	RetVal->Float = strtof(str, (char **)params[1].PtrPtr);
}

/* long int strtol(const char *str, char **endptr, int base); */
static void native_strtol(struct Tagha *const restrict sys, union TaghaVal *const restrict RetVal, const size_t argc, union TaghaVal params[restrict static argc])
{
	const char *restrict str = params[0].Ptr;
	if( !str )
		return;
	RetVal->Int32 = strtol(str, (char **)params[1].PtrPtr, params[2].Int32);
}

/* long long int strtoll(const char *str, char **endptr, int base); */
static void native_strtoll(struct Tagha *const restrict sys, union TaghaVal *const restrict RetVal, const size_t argc, union TaghaVal params[restrict static argc])
{
	const char *restrict str = params[0].Ptr;
	if( !str )
		return;
	RetVal->Int64 = strtoll(str, (char **)params[1].PtrPtr, params[2].Int32);
}

/* unsigned long int strtoul(const char *str, char **endptr, int base); */
static void native_strtoul(struct Tagha *const restrict sys, union TaghaVal *const restrict RetVal, const size_t argc, union TaghaVal params[restrict static argc])
{
	const char *restrict str = params[0].Ptr;
	if( !str )
		return;
	RetVal->UInt64 = strtoul(str, (char **)params[1].PtrPtr, params[2].Int32);
}

/* unsigned long long int strtoull(const char *str, char **endptr, int base); */
static void native_strtoull(struct Tagha *const restrict sys, union TaghaVal *const restrict RetVal, const size_t argc, union TaghaVal params[restrict static argc])
{
	const char *restrict str = params[0].Ptr;
	if( !str )
		return;
	RetVal->UInt64 = strtoll(str, (char **)params[1].PtrPtr, params[2].Int32);
}

/* void abort(void); */
static void native_abort(struct Tagha *const restrict sys, union TaghaVal *const restrict RetVal, const size_t argc, union TaghaVal params[restrict static argc])
{
	abort();
}

/* void exit(int status); */
static void native_exit(struct Tagha *const restrict sys, union TaghaVal *const restrict RetVal, const size_t argc, union TaghaVal params[restrict static argc])
{
	exit(params[0].Int32);
}

/* int system(const char *command); */
static void native_system(struct Tagha *const restrict sys, union TaghaVal *const restrict RetVal, const size_t argc, union TaghaVal params[restrict static argc])
{
	const char *restrict command = params[0].Ptr;
	if( !command ) {
		RetVal->Int32 = -1;
		return;
	}
	RetVal->Int32 = system(command);
}

/* int abs(int n); */
static void native_abs(struct Tagha *const restrict sys, union TaghaVal *const restrict RetVal, const size_t argc, union TaghaVal params[restrict static argc])
{
	RetVal->Int32 = abs(params[0].Int32);
}

/* long int labs(long int n); */
static void native_labs(struct Tagha *const restrict sys, union TaghaVal *const restrict RetVal, const size_t argc, union TaghaVal params[restrict static argc])
{
	RetVal->Int64 = labs(params[0].Int64);
}

/* long long int llabs(long long int n); */
static void native_llabs(struct Tagha *const restrict sys, union TaghaVal *const restrict RetVal, const size_t argc, union TaghaVal params[restrict static argc])
{
	RetVal->Int64 = llabs(params[0].Int64);
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
static void native_div(struct Tagha *const restrict sys, union TaghaVal *const restrict RetVal, const size_t argc, union TaghaVal params[restrict static argc])
{
	const div_t divres = div(params[1].Int32, params[2].Int32);
	memcpy(RetVal, &divres, sizeof divres);
}

/*
ldiv_t ldiv(long int numer, long int denom);
typedef struct {
	long int quot;
	long int rem;
} ldiv_t;

static void native_ldiv(struct Tagha *const restrict sys, union TaghaVal *const restrict RetVal, const size_t argc, union TaghaVal params[restrict static argc])
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
static void native_lldiv(struct Tagha *const restrict sys, union TaghaVal *const restrict RetVal, const size_t argc, union TaghaVal params[restrict static argc])
{
	lldiv_t *res = params[0].Ptr;
	*res = lldiv(params[1].Int64, params[2].Int64);
}

/* int mblen(const char *pmb, size_t max); */
static void native_mblen(struct Tagha *const restrict sys, union TaghaVal *const restrict RetVal, const size_t argc, union TaghaVal params[restrict static argc])
{
	RetVal->Int32 = mblen(params[0].Ptr, params[1].UInt64);
}

/* int mbtowc(wchar_t *pwc, const char *pmb, size_t max); */
static void native_mbtowc(struct Tagha *const restrict sys, union TaghaVal *const restrict RetVal, const size_t argc, union TaghaVal params[restrict static argc])
{
	RetVal->Int32 = mbtowc(params[0].Ptr, params[1].Ptr, params[2].UInt64);
}

/* int wctomb(char *pmb, wchar_t wc); */
static void native_wctomb(struct Tagha *const restrict sys, union TaghaVal *const restrict RetVal, const size_t argc, union TaghaVal params[restrict static argc])
{
	RetVal->Int32 = wctomb(params[0].Ptr, sizeof(wchar_t)==2 ? params[1].UInt16 : params[1].UInt32);
}

/* size_t mbstowcs(wchar_t *dest, const char *src, size_t max); */
static void native_mbstowcs(struct Tagha *const restrict sys, union TaghaVal *const restrict RetVal, const size_t argc, union TaghaVal params[restrict static argc])
{
	RetVal->UInt64 = mbstowcs(params[0].Ptr, params[1].Ptr, params[2].UInt64);
}

/* size_t wcstombs(char *dest, const wchar_t *src, size_t max); */
static void native_wcstombs(struct Tagha *const restrict sys, union TaghaVal *const restrict RetVal, const size_t argc, union TaghaVal params[restrict static argc])
{
	RetVal->UInt64 = wcstombs(params[0].Ptr, params[1].Ptr, params[2].UInt64);
}


bool Tagha_LoadstdlibNatives(struct Tagha *const restrict sys)
{
	const struct NativeInfo libc_stdlib_natives[] = {
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
	return sys ? Tagha_RegisterNatives(sys, libc_stdlib_natives) : false;
}
