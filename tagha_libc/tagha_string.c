#include <uintptr.h>
#include "tagha_libc.h"

/** void *memcpy(void *dest, const void *src, size_t num); */
static union TaghaVal native_memcpy(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .uintptr = memcpy(params[0].uintptr, params[1].uintptr, params[2].uint64) };
}

/** void *memmove(void *dest, const void *src, size_t num); */
static union TaghaVal native_memmove(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .uintptr = memmove(params[0].uintptr, params[1].uintptr, params[2].uint64) };
}

/** char *strcpy(char *dest, const char *src); */
static union TaghaVal native_strcpy(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .uintptr = strcpy(params[0].uintptr, params[1].uintptr) };
}

/** char *strncpy(char *dest, const char *src, size_t num); */
static union TaghaVal native_strncpy(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .uintptr = strncpy(params[0].uintptr, params[1].uintptr, params[2].uint64) };
}

/** char *strcat(char *dest, const char *src); */
static union TaghaVal native_strcat(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .uintptr = strcat(params[0].uintptr, params[1].uintptr) };
}

/** char *strncat(char *dest, const char *src, size_t num); */
static union TaghaVal native_strncat(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .uintptr = strncat(params[0].uintptr, params[1].uintptr, params[2].uint64) };
}

/** int memcmp(const void *ptr1, const void *ptr2, size_t num); */
static union TaghaVal native_memcmp(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .int32 = memcmp(params[0].uintptr, params[1].uintptr, params[2].uint64) };
}

/** int strcmp(const char *str1, const char *str2); */
static union TaghaVal native_strcmp(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .int32 = strcmp(params[0].uintptr, params[1].uintptr) };
}

/** int strcoll(const char *str1, const char *str2); */
static union TaghaVal native_strcoll(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .int32 = strcoll(params[0].uintptr, params[1].uintptr) };
}

/** int strncmp(const char *str1, const char *str2, size_t num); */
static union TaghaVal native_strncmp(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .int32 = strncmp(params[0].uintptr, params[1].uintptr, params[2].uint64) };
}

/** size_t strxfrm(char *dest, const char *src, size_t num); */
static union TaghaVal native_strxfrm(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .uint64 = strxfrm(params[0].uintptr, params[1].uintptr, params[2].uint64) };
}

/** void *memchr(const void *ptr, int value, size_t num); */
static union TaghaVal native_memchr(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .uintptr = memchr(params[0].uintptr, params[1].int32, params[2].uint64) };
}

/** char *strchr(const char *str, int character); */
static union TaghaVal native_strchr(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .uintptr = strchr(params[0].uintptr, params[1].int32) };
}

/** size_t strcspn(const char *str1, const char *str2); */
static union TaghaVal native_strcspn(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .uint64 = strcspn(params[0].uintptr, params[1].uintptr) };
}

/** char *strpbrk(const char *str1, const char *str2); */
static union TaghaVal native_strpbrk(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .uintptr = strpbrk(params[0].uintptr, params[1].uintptr) };
}

/** char *strrchr(const char *str, int character); */
static union TaghaVal native_strrchr(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .uintptr = strrchr(params[0].uintptr, params[1].int32) };
}

/** size_t strspn(const char *str1, const char *str2); */
static union TaghaVal native_strspn(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .uint64 = strspn(params[0].uintptr, params[1].uintptr) };
}

/** char *strstr(const char *str1, const char *str2); */
static union TaghaVal native_strstr(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .uintptr = strstr(params[0].uintptr, params[1].uintptr) };
}

/** char *strtok(char *str, const char *delimiters); */
static union TaghaVal native_strtok(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .uintptr = strtok(params[0].uintptr, params[1].uintptr) };
}

/** void *memset(void *ptr, int value, size_t num); */
static union TaghaVal native_memset(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .uintptr = memset(params[0].uintptr, params[1].int32, params[1].uint64) };
}

/** char *strerror(int errnum); */
static union TaghaVal native_strerror(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .uintptr = strerror(params[0].int32) };
}

/** size_t strlen(const char * str); */
static union TaghaVal native_strlen(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1])
{
	(void)module; (void)args;
	return (union TaghaVal){ .uint64 = strlen(params[0].uintptr) };
}


bool tagha_module_load_string_natives(struct TaghaModule *const module)
{
	const struct TaghaNative libc_string_natives[] = {
		{"memcpy", native_memcpy},
		{"memmove", native_memmove},
		{"strcpy", native_strcpy},
		{"strncpy", native_strncpy},
		{"strcat", native_strcat},
		{"strncat", native_strncat},
		{"memcmp", native_memcmp},
		{"strcmp", native_strcmp},
		{"strcoll", native_strcoll},
		{"strncmp", native_strncmp},
		{"strxfrm", native_strxfrm},
		{"memchr", native_memchr},
		{"strchr", native_strchr},
		{"strcspn", native_strcspn},
		{"strpbrk", native_strpbrk},
		{"strrchr", native_strrchr},
		{"strspn", native_strspn},
		{"strstr", native_strstr},
		{"strtok", native_strtok},
		{"memset", native_memset},
		{"strerror", native_strerror},
		{"strlen", native_strlen},
		{NULL, NULL}
	};
	return module ? tagha_module_register_natives(module, libc_string_natives) : false;
}
