#include "../tagha/tagha.h"
#include <ctype.h>

/** int isalnum(int c); */
static union TaghaVal native_isalnum(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .int32 = isalnum(params[0].int32) };
}

/** int isalpha(int c); */
static union TaghaVal native_isalpha(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .int32 = isalpha(params[0].int32) };
}

/** int isblank(int c); */
static union TaghaVal native_isblank(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .int32 = isblank(params[0].int32) };
}

/** int iscntrl(int c); */
static union TaghaVal native_iscntrl(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .int32 = iscntrl(params[0].int32) };
}

/** int isdigit(int c); */
static union TaghaVal native_isdigit(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .int32 = isdigit(params[0].int32) };
}

/** int isgraph(int c); */
static union TaghaVal native_isgraph(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .int32 = isgraph(params[0].int32) };
}

/** int islower(int c); */
static union TaghaVal native_islower(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .int32 = islower(params[0].int32) };
}

/** int isprint(int c); */
static union TaghaVal native_isprint(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .int32 = isprint(params[0].int32) };
}

/** int ispunct(int c); */
static union TaghaVal native_ispunct(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .int32 = ispunct(params[0].int32) };
}

/** int isspace(int c); */
static union TaghaVal native_isspace(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .int32 = isspace(params[0].int32) };
}

/** int isupper(int c); */
static union TaghaVal native_isupper(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .int32 = isupper(params[0].int32) };
}

/** int toupper(int c); */
static union TaghaVal native_toupper(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .int32 = toupper(params[0].int32) };
}

/** int tolower(int c); */
static union TaghaVal native_tolower(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .int32 = tolower(params[0].int32) };
}

/** int isxdigit(int c); */
static union TaghaVal native_isxdigit(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .int32 = isxdigit(params[0].int32) };
}


bool tagha_module_load_ctype_natives(struct TaghaModule *const module)
{
	return module ? tagha_module_register_natives(module, ( const struct TaghaNative[] ){
		{"__tagha_isalnum",  &native_isalnum},
		{"__tagha_isalpha",  &native_isalpha},
		{"__tagha_isblank",  &native_isblank},
		{"__tagha_iscntrl",  &native_iscntrl},
		{"__tagha_isdigit",  &native_isdigit},
		{"__tagha_isgraph",  &native_isgraph},
		{"__tagha_islower",  &native_islower},
		{"__tagha_isprint",  &native_isprint},
		{"__tagha_ispunct",  &native_ispunct},
		{"__tagha_isspace",  &native_isspace},
		{"__tagha_isupper",  &native_isupper},
		{"__tagha_toupper",  &native_toupper},
		{"__tagha_tolower",  &native_tolower},
		{"__tagha_isxdigit", &native_isxdigit},
		{NULL,               NULL}
	}) : false;
}