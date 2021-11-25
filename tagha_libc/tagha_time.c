#include <time.h>
#include <string.h>
#include "../tagha/tagha.h"

/** clock_t clock(void); */
static union TaghaVal native_clock(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module; ( void )params;
	union {
		const clock_t cl;
		const union TaghaVal v;
	} conv = { clock() };
	return conv.v;
}

/** time_t time(time_t *timer); */
static union TaghaVal native_time(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	union {
		const time_t t;
		const union TaghaVal v;
	} conv = { time(( time_t* )params[0].uintptr) };
	return conv.v;
}

/** float64_t difftime(time_t end, time_t beginning); */
static union TaghaVal native_difftime(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	const union {
		const union TaghaVal v;
		const time_t t;
	} end = { params[0] }, begin = { params[1] };
	return ( union TaghaVal ){ .float64 = difftime(end.t, begin.t) };
}

/** time_t mktime(struct tm *timeptr); */
static union TaghaVal native_mktime(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	union {
		const time_t t;
		const union TaghaVal v;
	} conv = { mktime(( struct tm* )params[0].uintptr) };
	return conv.v;
}

/** char *asctime(const struct tm *timeptr); */
static union TaghaVal native_asctime(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .uintptr = ( uintptr_t )asctime(( const struct tm* )params[0].uintptr) };
}

/** char *ctime(const time_t *timer); */
static union TaghaVal native_ctime(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .uintptr = ( uintptr_t )ctime(( const time_t* )params[0].uintptr)};
}

/** struct tm *gmtime(const time_t *timer); */
static union TaghaVal native_gmtime(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .uintptr = ( uintptr_t )gmtime(( const time_t* )params[0].uintptr) };
}

/** struct tm *localtime(const time_t *timer); */
static union TaghaVal native_localtime(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .uintptr = ( uintptr_t )localtime(( const time_t* )params[0].uintptr) };
}

/** size_t strftime(char *ptr, size_t maxsize, const char *format, const struct tm *timeptr); */
static union TaghaVal native_strftime(struct TaghaModule *const module, const union TaghaVal params[const static 1])
{
	( void )module;
	return ( union TaghaVal ){ .uint64 = strftime(( char* )params[0].uintptr, params[1].uint64, ( const char* )params[2].uintptr, ( const struct tm* )params[3].uintptr) };
}


bool tagha_module_load_time_natives(struct TaghaModule *const module)
{
	return module ? tagha_module_register_natives(module, ( const struct TaghaNative[] ){
		{"__tagha_clock",     &native_clock},
		{"__tagha_time",      &native_time},
		{"__tagha_difftime",  &native_difftime},
		{"__tagha_mktime",    &native_mktime},
		{"__tagha_asctime",   &native_asctime},
		{"__tagha_ctime",     &native_ctime},
		{"__tagha_gmtime",    &native_gmtime},
		{"__tagha_localtime", &native_localtime},
		{"__tagha_strftime",  &native_strftime},
		{NULL,                NULL}
	}) : false;
}