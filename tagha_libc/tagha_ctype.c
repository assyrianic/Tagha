
#include <stdlib.h>

/* int isalnum(int c); */
static void native_isalnum(struct Tagha *const restrict sys, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	retval->Int32 = isalnum(params[0].Int32);
}

/* int isalpha(int c); */
static void native_isalpha(struct Tagha *const restrict sys, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	retval->Int32 = isalpha(params[0].Int32);
}

/* int isblank(int c); */
static void native_isblank(struct Tagha *const restrict sys, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	retval->Int32 = isblank(params[0].Int32);
}

/* int iscntrl(int c); */
static void native_iscntrl(struct Tagha *const restrict sys, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	retval->Int32 = iscntrl(params[0].Int32);
}

/* int isdigit(int c); */
static void native_isdigit(struct Tagha *const restrict sys, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	retval->Int32 = isdigit(params[0].Int32);
}

/* int isgraph(int c); */
static void native_isgraph(struct Tagha *const restrict sys, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	retval->Int32 = isgraph(params[0].Int32);
}

/* int islower(int c); */
static void native_islower(struct Tagha *const restrict sys, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	retval->Int32 = islower(params[0].Int32);
}

/* int isprint(int c); */
static void native_isprint(struct Tagha *const restrict sys, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	retval->Int32 = isprint(params[0].Int32);
}

/* int ispunct(int c); */
static void native_ispunct(struct Tagha *const restrict sys, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	retval->Int32 = ispunct(params[0].Int32);
}

/* int isspace(int c); */
static void native_isspace(struct Tagha *const restrict sys, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	retval->Int32 = isspace(params[0].Int32);
}

/* int isupper(int c); */
static void native_isupper(struct Tagha *const restrict sys, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	retval->Int32 = isupper(params[0].Int32);
}

/* int toupper(int c); */
static void native_toupper(struct Tagha *const restrict sys, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	retval->Int32 = toupper(params[0].Int32);
}

/* int tolower(int c); */
static void native_tolower(struct Tagha *const restrict sys, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	retval->Int32 = tolower(params[0].Int32);
}

/* int isxdigit(int c); */
static void native_isxdigit(struct Tagha *const restrict sys, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	retval->Int32 = isxdigit(params[0].Int32);
}


bool Tagha_LoadctypeNatives(struct Tagha *const restrict sys)
{
	if( !sys )
		return false;
	
	const struct NativeInfo libc_ctype_natives[] = {
		{"isalnum", native_isalnum},
		{"isalpha", native_isalpha},
		{"isblank", native_isblank},
		{"iscntrl", native_iscntrl},
		{"isdigit", native_isdigit},
		{"isgraph", native_isgraph},
		{"islower", native_islower},
		{"isprint", native_isprint},
		{"ispunct", native_ispunct},
		{"isspace", native_isspace},
		{"isupper", native_isupper},
		{"toupper", native_toupper},
		{"tolower", native_tolower},
		{"isxdigit", native_isxdigit},
		{NULL, NULL}
	};
	return Tagha_RegisterNatives(sys, libc_ctype_natives);
}
