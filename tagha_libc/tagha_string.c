#include <string.h>
#include "../tagha/tagha.h"

/** void *memcpy(void *dest, const void *src, size_t num); */
static union TaghaVal native_memcpy(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .uintptr = ( uintptr_t )memcpy(( void* )params[0].uintptr, ( const void* )params[1].uintptr, params[2].uint64) };
}

/** void *memmove(void *dest, const void *src, size_t num); */
static union TaghaVal native_memmove(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .uintptr = ( uintptr_t )memmove(( void* )params[0].uintptr, ( const void* )params[1].uintptr, params[2].uint64) };
}

/** char *strcpy(char *dest, const char *src); */
static union TaghaVal native_strcpy(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .uintptr = ( uintptr_t )strcpy(( char* )params[0].uintptr, ( const char* )params[1].uintptr) };
}

/** char *strncpy(char *dest, const char *src, size_t num); */
static union TaghaVal native_strncpy(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .uintptr = ( uintptr_t )strncpy(( char* )params[0].uintptr, ( const char* )params[1].uintptr, params[2].uint64) };
}

/** char *strcat(char *dest, const char *src); */
static union TaghaVal native_strcat(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .uintptr = ( uintptr_t )strcat(( char* )params[0].uintptr, ( const char* )params[1].uintptr) };
}

/** char *strncat(char *dest, const char *src, size_t num); */
static union TaghaVal native_strncat(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .uintptr = ( uintptr_t )strncat(( char* )params[0].uintptr, ( const char* )params[1].uintptr, params[2].uint64) };
}

/** int memcmp(const void *ptr1, const void *ptr2, size_t num); */
static union TaghaVal native_memcmp(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .int32 = memcmp(( const void* )params[0].uintptr, ( const void* )params[1].uintptr, params[2].uint64) };
}

/** int strcmp(const char *str1, const char *str2); */
static union TaghaVal native_strcmp(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .int32 = strcmp(( const char* )params[0].uintptr, ( const char* )params[1].uintptr) };
}

/** int strcoll(const char *str1, const char *str2); */
static union TaghaVal native_strcoll(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .int32 = strcoll(( const char* )params[0].uintptr, ( const char* )params[1].uintptr) };
}

/** int strncmp(const char *str1, const char *str2, size_t num); */
static union TaghaVal native_strncmp(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .int32 = strncmp(( const char* )params[0].uintptr, ( const char* )params[1].uintptr, params[2].uint64) };
}

/** size_t strxfrm(char *dest, const char *src, size_t num); */
static union TaghaVal native_strxfrm(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .uint64 = strxfrm(( char* )params[0].uintptr, ( const char* )params[1].uintptr, params[2].uint64) };
}

/** void *memchr(const void *ptr, int value, size_t num); */
static union TaghaVal native_memchr(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .uintptr = ( uintptr_t )memchr(( const void* )params[0].uintptr, params[1].int32, params[2].uint64) };
}

/** char *strchr(const char *str, int character); */
static union TaghaVal native_strchr(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .uintptr = ( uintptr_t )strchr(( const char* )params[0].uintptr, params[1].int32) };
}

/** size_t strcspn(const char *str1, const char *str2); */
static union TaghaVal native_strcspn(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .uint64 = strcspn(( const char* )params[0].uintptr, ( const char* )params[1].uintptr) };
}

/** char *strpbrk(const char *str1, const char *str2); */
static union TaghaVal native_strpbrk(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .uintptr = ( uintptr_t )strpbrk(( const char* )params[0].uintptr, ( const char* )params[1].uintptr) };
}

/** char *strrchr(const char *str, int character); */
static union TaghaVal native_strrchr(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .uintptr = ( uintptr_t )strrchr(( const char* )params[0].uintptr, params[1].int32) };
}

/** size_t strspn(const char *str1, const char *str2); */
static union TaghaVal native_strspn(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .uint64 = strspn(( const char* )params[0].uintptr, ( const char* )params[1].uintptr) };
}

/** char *strstr(const char *str1, const char *str2); */
static union TaghaVal native_strstr(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .uintptr = ( uintptr_t )strstr(( const char* )params[0].uintptr, ( const char* )params[1].uintptr) };
}

/** char *strtok(char *str, const char *delimiters); */
static union TaghaVal native_strtok(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .uintptr = ( uintptr_t )strtok(( char* )params[0].uintptr, ( const char* )params[1].uintptr) };
}

/** void *memset(void *ptr, int value, size_t num); */
static union TaghaVal native_memset(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .uintptr = ( uintptr_t )memset(( void* )params[0].uintptr, params[1].int32, params[1].uint64) };
}

/** char *strerror(int errnum); */
static union TaghaVal native_strerror(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .uintptr = ( uintptr_t )strerror(params[0].int32) };
}

/** size_t strlen(const char *str); */
static union TaghaVal native_strlen(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .uint64 = strlen(( const char* )params[0].uintptr) };
}


bool tagha_module_load_string_natives(struct TaghaModule *const module)
{
	return module ? tagha_module_register_natives(module, ( const struct TaghaNative[] ){
		{"__tagha_memcpy",   &native_memcpy},
		{"__tagha_memmove",  &native_memmove},
		{"__tagha_strcpy",   &native_strcpy},
		{"__tagha_strncpy",  &native_strncpy},
		{"__tagha_strcat",   &native_strcat},
		{"__tagha_strncat",  &native_strncat},
		{"__tagha_memcmp",   &native_memcmp},
		{"__tagha_strcmp",   &native_strcmp},
		{"__tagha_strcoll",  &native_strcoll},
		{"__tagha_strncmp",  &native_strncmp},
		{"__tagha_strxfrm",  &native_strxfrm},
		{"__tagha_memchr",   &native_memchr},
		{"__tagha_strchr",   &native_strchr},
		{"__tagha_strcspn",  &native_strcspn},
		{"__tagha_strpbrk",  &native_strpbrk},
		{"__tagha_strrchr",  &native_strrchr},
		{"__tagha_strspn",   &native_strspn},
		{"__tagha_strstr",   &native_strstr},
		{"__tagha_strtok",   &native_strtok},
		{"__tagha_memset",   &native_memset},
		{"__tagha_strerror", &native_strerror},
		{"__tagha_strlen",   &native_strlen},
		{NULL,               NULL}
	}) : false;
}