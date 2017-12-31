
#include <stdlib.h>

/* void *malloc(size_t size); */
static void native_malloc(struct Tagha *pSys, union CValue params[], union CValue *restrict const pRetval, const uint32_t argc)
{
	// size_t is 8 bytes on 64-bit systems
	pRetval->Ptr = calloc(1, params[0].UInt64);
}

/* void free(void *ptr); */
static void native_free(struct Tagha *pSys, union CValue params[], union CValue *restrict const pRetval, const uint32_t argc)
{
	free(params[0].Ptr);
}

/* non-standard addition. */
/* void safe_free(void **pptr); */
static void native_safe_free(struct Tagha *pSys, union CValue params[], union CValue *restrict const pRetval, const uint32_t argc)
{
	void **pptr = params[0].PtrPtr;
	if( *pptr ) {
		puts("safe_free :: *pptr is VALID, freeing.\n");
		free(*pptr), *pptr=NULL;
	}
}

/* void *calloc(size_t num, size_t size); */
static void native_calloc(struct Tagha *pSys, union CValue params[], union CValue *restrict const pRetval, const uint32_t argc)
{
	pRetval->Ptr = calloc(params[0].UInt64, params[1].UInt64);
}

/* void *realloc(void *ptr, size_t size); */
static void native_realloc(struct Tagha *pSys, union CValue params[], union CValue *restrict const pRetval, const uint32_t argc)
{
	pRetval->Ptr = realloc(params[0].Ptr, params[1].UInt64);
}

/* void srand(unsigned int seed); */
static void native_srand(struct Tagha *pSys, union CValue params[], union CValue *restrict const pRetval, const uint32_t argc)
{
	srand(params[0].UInt32);
}

/* int rand(void); */
static void native_rand(struct Tagha *pSys, union CValue params[], union CValue *restrict const pRetval, const uint32_t argc)
{
	pRetval->Int32 = rand();
}

/* double atof(const char *str); */
static void native_atof(struct Tagha *pSys, union CValue params[], union CValue *restrict const pRetval, const uint32_t argc)
{
	pRetval->Double = atof(params[0].String);
}

/* int atoi(const char *str); */
static void native_atoi(struct Tagha *pSys, union CValue params[], union CValue *restrict const pRetval, const uint32_t argc)
{
	pRetval->Int32 = atoi(params[0].String);
}

/* long int atol(const char *str); */
static void native_atol(struct Tagha *pSys, union CValue params[], union CValue *restrict const pRetval, const uint32_t argc)
{
	pRetval->Int64 = atol(params[0].String);
}

/* long long int atoll(const char *str); */
static void native_atoll(struct Tagha *pSys, union CValue params[], union CValue *restrict const pRetval, const uint32_t argc)
{
	pRetval->Int64 = atoll(params[0].String);
}

/* double strtod(const char *str, char **endptr); */
static void native_strtod(struct Tagha *pSys, union CValue params[], union CValue *restrict const pRetval, const uint32_t argc)
{
	const char *str = params[0].String;
	if( !str ) {
		puts("strtod reported :: **** param 'str' is NULL ****\n");
		return;
	}
	pRetval->Double = strtod(str, params[1].StrPtr);
}

/* float strtof(const char *str, char **endptr); */
static void native_strtof(struct Tagha *pSys, union CValue params[], union CValue *restrict const pRetval, const uint32_t argc)
{
	const char *str = params[0].String;
	if( !str ) {
		puts("strtof reported :: **** param 'str' is NULL ****\n");
		return;
	}
	pRetval->Float = strtof(str, params[1].StrPtr);
}

/* long int strtol(const char *str, char **endptr, int base); */
static void native_strtol(struct Tagha *pSys, union CValue params[], union CValue *restrict const pRetval, const uint32_t argc)
{
	const char *str = params[0].String;
	if( !str ) {
		puts("strtol reported :: **** param 'str' is NULL ****\n");
		return;
	}
	pRetval->Int32 = strtol(str, params[1].StrPtr, params[2].Int32);
}

/* long long int strtoll(const char *str, char **endptr, int base); */
static void native_strtoll(struct Tagha *pSys, union CValue params[], union CValue *restrict const pRetval, const uint32_t argc)
{
	const char *str = params[0].String;
	if( !str ) {
		puts("strtoll reported :: **** param 'str' is NULL ****\n");
		return;
	}
	pRetval->Int64 = strtoll(str, params[1].StrPtr, params[2].Int32);
}

/* unsigned long int strtoul(const char *str, char **endptr, int base); */
static void native_strtoul(struct Tagha *pSys, union CValue params[], union CValue *restrict const pRetval, const uint32_t argc)
{
	const char *str = params[0].String;
	if( !str ) {
		puts("strtoul reported :: **** param 'str' is NULL ****\n");
		return;
	}
	pRetval->UInt32 = strtoul(str, params[1].StrPtr, params[2].Int32);
}

/* unsigned long long int strtoull(const char *str, char **endptr, int base); */
static void native_strtoull(struct Tagha *pSys, union CValue params[], union CValue *restrict const pRetval, const uint32_t argc)
{
	const char *str = params[0].String;
	if( !str ) {
		puts("strtoull reported :: **** param 'str' is NULL ****\n");
		return;
	}
	pRetval->UInt64 = strtoll(str, params[1].StrPtr, params[2].Int32);
}

/* void abort(void); */
static void native_abort(struct Tagha *pSys, union CValue params[], union CValue *restrict const pRetval, const uint32_t argc)
{
	*pSys->m_Regs[rip].UCharPtr = halt;
}

/* void exit(int status); */
static void native_exit(struct Tagha *pSys, union CValue params[], union CValue *restrict const pRetval, const uint32_t argc)
{
	*pSys->m_Regs[rip].UCharPtr = halt;
	printf("Tagha: Exiting with status: %i\n", params[0].Int32);
}

/* int system(const char *command); */
static void native_system(struct Tagha *pSys, union CValue params[], union CValue *restrict const pRetval, const uint32_t argc)
{
	const char *command = params[0].String;
	if( !command ) {
		puts("system reported :: **** param 'command' is NULL ****\n");
		pRetval->Int32 = -1;
		return;
	}
	pRetval->Int32 = system(command);
}

/* int abs(int n); */
static void native_abs(struct Tagha *pSys, union CValue params[], union CValue *restrict const pRetval, const uint32_t argc)
{
	pRetval->Int32 = abs(params[0].Int32);
}

/* long int labs(long int n); */
static void native_labs(struct Tagha *pSys, union CValue params[], union CValue *restrict const pRetval, const uint32_t argc)
{
	pRetval->Int64 = (int64_t)labs(params[0].Int64);
}

/* long long int llabs(long long int n); */
static void native_llabs(struct Tagha *pSys, union CValue params[], union CValue *restrict const pRetval, const uint32_t argc)
{
	pRetval->Int64 = llabs(params[0].Int64);
}

/*
div_t div(int numer, int denom);
typedef struct {
	int quot;	// quotient, 1st 32 bits.
	int rem;	// remainder, 2nd 32 bits.
} div_t;
now how the fuck do we return a damn struct?
registers are large enough to have an 8-byte struct though. Nope.
void div(div_t *res, int numer, int denom);
*/
static void native_div(struct Tagha *pSys, union CValue params[], union CValue *restrict const pRetval, const uint32_t argc)
{
	div_t *res = params[0].Ptr;
	*res = div(params[1].Int32, params[2].Int32);
}

/*
ldiv_t ldiv(long int numer, long int denom);
typedef struct {
	long int quot;
	long int rem;
} ldiv_t;
now how in hell are going to return something 16 bytes?
We can use two registers but that ain't feasible here.
Why isn't it feasible? Because we need to maintain data between
Tagha's env and the host machine env...
So in the end, for ldiv, you need to pass a ptr to ldiv_t struct for now.
void ldiv(ldiv_t *res, long int numer, long int denom);
*/
static void native_ldiv(struct Tagha *pSys, union CValue params[], union CValue *restrict const pRetval, const uint32_t argc)
{
	ldiv_t *res = params[0].Ptr;
	*res = ldiv(params[1].Int64, params[2].Int64);
}
/*
lldiv_t lldiv(long long int numer, long long int denom);
typedef struct {
	long long int quot;
	long long int rem;
} lldiv_t;
void ldiv(lldiv_t *res, long long int numer, long long int denom);
*/
static void native_lldiv(struct Tagha *pSys, union CValue params[], union CValue *restrict const pRetval, const uint32_t argc)
{
	lldiv_t *res = params[0].Ptr;
	*res = lldiv(params[1].Int64, params[2].Int64);
}

/* int mblen(const char *pmb, size_t max); */
static void native_mblen(struct Tagha *pSys, union CValue params[], union CValue *restrict const pRetval, const uint32_t argc)
{
	pRetval->Int32 = mblen(params[0].String, params[1].UInt64);
}

/* int mbtowc(wchar_t *pwc, const char *pmb, size_t max); */
static void native_mbtowc(struct Tagha *pSys, union CValue params[], union CValue *restrict const pRetval, const uint32_t argc)
{
	pRetval->Int32 = mbtowc(params[0].Ptr, params[1].String, params[2].UInt64);
}

/* int wctomb(char *pmb, wchar_t wc); */
static void native_wctomb(struct Tagha *pSys, union CValue params[], union CValue *restrict const pRetval, const uint32_t argc)
{
	pRetval->Int32 = wctomb(params[0].Str, sizeof(wchar_t)==2 ? params[1].UShort : params[1].UInt32);
}

/* size_t mbstowcs(wchar_t *dest, const char *src, size_t max); */
static void native_mbstowcs(struct Tagha *pSys, union CValue params[], union CValue *restrict const pRetval, const uint32_t argc)
{
	pRetval->UInt64 = mbstowcs((wchar_t *)params[0].Ptr, params[1].String, params[2].UInt64);
}

/* size_t wcstombs(char *dest, const wchar_t *src, size_t max); */
static void native_wcstombs(struct Tagha *pSys, union CValue params[], union CValue *restrict const pRetval, const uint32_t argc)
{
	pRetval->UInt64 = wcstombs(params[0].Str, (wchar_t *)params[1].Ptr, params[2].UInt64);
}


bool Tagha_Load_stdlibNatives(struct Tagha *pSys)
{
	if( !pSys )
		return false;
	
	struct NativeInfo libc_stdlib_natives[] = {
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
		{"ldiv", native_ldiv},
		{"lldiv", native_lldiv},
		{"mblen", native_mblen},
		{"mbtowc", native_mbtowc},
		{"wctomb", native_wctomb},
		{"mbstowcs", native_mbstowcs},
		{"wcstombs", native_wcstombs},
		{NULL, NULL}
	};
	return Tagha_RegisterNatives(pSys, libc_stdlib_natives);
}











