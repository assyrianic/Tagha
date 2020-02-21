#include <stdlib.h>
#include <ctype.h>
#include "tagha_libc.h"

/** int isalnum(int c); */
static union TaghaVal native_isalnum(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .int32 = isalnum(params[0].int32) };
}

/** int isalpha(int c); */
static union TaghaVal native_isalpha(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .int32 = isalpha(params[0].int32) };
}

/** int isblank(int c); */
static union TaghaVal native_isblank(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .int32 = isblank(params[0].int32) };
}

/** int iscntrl(int c); */
static union TaghaVal native_iscntrl(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .int32 = iscntrl(params[0].int32) };
}

/** int isdigit(int c); */
static union TaghaVal native_isdigit(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .int32 = isdigit(params[0].int32) };
}

/** int isgraph(int c); */
static union TaghaVal native_isgraph(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .int32 = isgraph(params[0].int32) };
}

/** int islower(int c); */
static union TaghaVal native_islower(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .int32 = islower(params[0].int32) };
}

/** int isprint(int c); */
static union TaghaVal native_isprint(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .int32 = isprint(params[0].int32) };
}

/** int ispunct(int c); */
static union TaghaVal native_ispunct(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .int32 = ispunct(params[0].int32) };
}

/** int isspace(int c); */
static union TaghaVal native_isspace(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .int32 = isspace(params[0].int32) };
}

/** int isupper(int c); */
static union TaghaVal native_isupper(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .int32 = isupper(params[0].int32) };
}

/** int toupper(int c); */
static union TaghaVal native_toupper(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .int32 = toupper(params[0].int32) };
}

/** int tolower(int c); */
static union TaghaVal native_tolower(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .int32 = tolower(params[0].int32) };
}

/** int isxdigit(int c); */
static union TaghaVal native_isxdigit(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .int32 = isxdigit(params[0].int32) };
}


bool tagha_module_load_ctype_natives(struct TaghaModule *const module)
{
	const struct TaghaNative libc_ctype_natives[] = {
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
	return module ? tagha_module_register_natives(module, libc_ctype_natives) : false;
}
