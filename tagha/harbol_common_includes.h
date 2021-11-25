#ifndef HARBOL_COMMON_INCLUDES_INCLUDED
#	define HARBOL_COMMON_INCLUDES_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>
#include <float.h>


/** placing this here so we can get this after including inttypes.h */
#if defined(INTPTR_MAX)
#	if defined(INT32_MAX) && INTPTR_MAX==INT32_MAX
#		ifndef HARBOL32
#			define HARBOL32
#		endif
#	endif
#	if defined(INT64_MAX) && INTPTR_MAX==INT64_MAX
#		ifndef HARBOL64
#			define HARBOL64
#		endif
#	endif
#endif


#ifndef NIL
#	define NIL    ( uintptr_t )NULL
#endif


/** types as defined by Harbol. */
#ifndef __ssize_t_defined
typedef long    ssize_t;
#	define __ssize_t_defined
#endif


/** According to C99 standards.
 * there are three floating point types: float, double, and long double.
 * 
 * The type double provides at least as much precision as float, and the type long double provides at least as much precision as double.
 * 
 * so in summary: float <= double <= long double
 */
#ifndef __float32_t_defined
#	if FLT_MANT_DIG==24
#		define __float32_t_defined
#		define PRIf32    "f"
#		define SCNf32    "f"
#		define SCNxf32   "a"
#		define strtof32  strtof
		typedef float float32_t;
#	elif DBL_MANT_DIG==24
#		define __float32_t_defined
#		define PRIf32    "f"
#		define SCNf32    "lf"
#		define SCNxf32   "la"
#		define strtof32  strtod
		typedef double float32_t;
#	else
#		error "no appropriate float32_t implementation"
#	endif
#endif

#ifdef C11
	_Static_assert(sizeof(float32_t) * CHAR_BIT == 32, "Unexpected `float32_t` size");
#endif


#ifndef __float64_t_defined
#	if DBL_MANT_DIG==53
#		define __float64_t_defined
#		define PRIf64    "f"
#		define SCNf64    "lf"
#		define SCNxf64   "la"
#		define strtof64  strtod
		typedef double float64_t;
#	elif LDBL_MANT_DIG==53
#		define __float64_t_defined
#		define PRIf64    "Lf"
#		define SCNf64    "Lf"
#		define SCNxf64   "La"
#		define strtof64  strtold
		typedef long double float64_t;
/// This is unlikely but check just in case.
#	elif FLT_MANT_DIG==53
#		define __float64_t_defined
#		define PRIf64    "f"
#		define SCNf64    "f"
#		define SCNxf64   "a"
#		define strtof64  strtof
		typedef float float64_t;
#	else
#		error "no appropriate float64_t implementation"
#	endif
#endif

#ifdef C11
	_Static_assert(sizeof(float64_t) * CHAR_BIT == 64, "Unexpected `float64_t` size");
#endif


#ifndef __floatptr_t_defined
#	if defined(HARBOL64)
#		define __floatptr_t_defined
#		define PRIfPTR    PRIf64
#		define strtofptr  strtof64
#		define SCNfPTR    SCNf64
#		define SCNxfPTR   SCNxf64
		typedef float64_t floatptr_t;
#	elif defined(HARBOL32)
#		define __floatptr_t_defined
#		define PRIfPTR    PRIf32
#		define strtofptr  strtof32
#		define SCNfPTR    SCNf32
#		define SCNxfPTR   SCNxf32
		typedef float32_t floatptr_t;
#	else
#		error "no appropriate floatptr_t implementation"
#	endif
#endif

#ifdef C11
	_Static_assert(sizeof(floatptr_t)==sizeof(intptr_t), "Unexpected `floatptr_t` size");
#endif


#ifndef __floatmax_t_defined
#	if LDBL_MANT_DIG > DBL_MANT_DIG
#		define __floatmax_t_defined
#		define PRIfMAX    "Lf"
#		define SCNfMAX    "Lf"
#		define SCNxfMAX   "La"
#		define strtofmax  strtold
		typedef long double floatmax_t;
#	elif DBL_MANT_DIG==LDBL_MANT_DIG && DBL_MANT_DIG > FLT_MANT_DIG
#		define __floatmax_t_defined
#		define PRIfMAX    "f"
#		define SCNfMAX    "lf"
#		define SCNxfMAX   "la"
#		define strtofmax  strtod
		typedef double floatmax_t;
#	elif DBL_MANT_DIG==FLT_MANT_DIG
#		define __floatmax_t_defined
#		define PRIfMAX    "f"
#		define SCNfMAX    "f"
#		define SCNxfMAX   "a"
#		define strtofmax  strtof
		typedef float floatmax_t;
#	else
#		error "no appropriate floatmax_t implementation"
#	endif
#endif


static inline void *harbol_recalloc(void *const arr, const size_t new_size, const size_t element_size, const size_t old_size) {
	if( arr==NULL || old_size==0 )
		return calloc(new_size, element_size);
	
#ifdef __cplusplus
	uint8_t *const restrict new_block = reinterpret_cast< decltype(new_block) >(realloc(arr, new_size * element_size));
#else
	uint8_t *const restrict new_block = realloc(arr, new_size * element_size);
#endif
	if( new_block==NULL )
		return NULL;
	
	if( old_size < new_size )
		memset(&new_block[old_size * element_size], 0, (new_size - old_size) * element_size);
	
	return new_block;
}

static inline void harbol_cleanup(void *const ptr_ref) {
#ifdef __cplusplus
	void **const p = reinterpret_cast< decltype(p) >(ptr_ref);
#else
	void **const p = ptr_ref;
#endif
	free(*p); *p = NULL;
}

static inline NO_NULL void *harbol_mempcpy(void *const dest, const void *const src, const size_t bytes) {
#ifdef __cplusplus
	uint8_t *const r = reinterpret_cast< decltype(r) >(memcpy(dest, src, bytes));
	return r + bytes;
#else
	return (( uint8_t* )memcpy(dest, src, bytes) + bytes);
#endif
}

static inline NO_NULL void *harbol_memccpy(void *const restrict dest, const void *const src, const int c, const size_t bytes) {
#ifdef __cplusplus
	const uint8_t *const p = reinterpret_cast< decltype(p) >(memchr(src, c, bytes));
	if( p != nullptr ) {
		const uint8_t *const s = reinterpret_cast< decltype(s) >(src);
		return harbol_mempcpy(dest, src, (p - s + 1));
	}
	memcpy(dest, src, bytes);
	return nullptr;
#else
	const uint8_t *const p = memchr(src, c, bytes);
	if( p != NULL ) {
		return harbol_mempcpy(dest, src, (p - ( const uint8_t* )src + 1));
	}
	memcpy(dest, src, bytes);
	return NULL;
#endif
}


static inline size_t harbol_align_size(const size_t size, const size_t align) {
	return (size + (align - 1)) & ~(align - 1);
}

static inline size_t harbol_pad_size(const size_t size, const size_t align) {
	return (align - (size & (align - 1))) & (align - 1);
}


/// these are NOT cryptographic hashes.
/// use ONLY FOR HASH TABLE IMPLEMENTATIONS.
#ifdef __cplusplus
static inline NO_NULL size_t string_hash(const char *const key)
#else
static inline size_t string_hash(const char key[static 1])
#endif
{
	size_t h = 0;
	for( size_t i=0; key[i] != 0; i++ ) {
		h = ( size_t )key[i] + (h << 6) + (h << 16) - h;
	}
	return h;
}


#ifdef __cplusplus
static inline NO_NULL size_t array_hash(const uint8_t *const key, const size_t len)
#else
static inline size_t array_hash(const uint8_t key[static 1], const size_t len)
#endif
{
	size_t h = 0;
	for( size_t i=0; i<len; i++ ) {
		h = ( size_t )key[i] + (h << 6) + (h << 16) - h;
	}
	return h;
}

static inline size_t int_hash(const size_t i) {
	size_t h = 0;
	for( size_t n=0; n<sizeof(size_t) * CHAR_BIT; n += 8 ) {
		h = ((i >> n) & 0xFF) + (h << 6) + (h << 16) - h;
	}
	return h;
}

static inline size_t float_hash(const floatptr_t a) {
	union {
		const floatptr_t f;
		const size_t     s;
	} c = {a};
	return int_hash(c.s);
}

static inline NO_NULL size_t ptr_hash(const void *const p) {
	union {
		const void *const p;
		const size_t      y;
	} c = {p};
	return (c.y >> 4u) | (c.y << (8u * sizeof(void*) - 4u));
}

#ifdef C11
#	define harbol_hash(h)   _Generic((h)+0, \
				int         : int_hash,     \
				size_t      : int_hash,     \
				int64_t     : int_hash,     \
				uint64_t    : int_hash,     \
				float32_t   : float_hash,   \
				float64_t   : float_hash,   \
				floatptr_t  : float_hash,   \
				char*       : string_hash,  \
				const char* : string_hash,  \
				default     : ptr_hash)     \
							((h))
#endif


static inline NO_NULL ssize_t get_file_size(FILE *const file) {
	fseek(file, 0, SEEK_END);
	const ssize_t filesize = ftell(file);
	rewind(file);
	return filesize;
}

/// Harbol Iterator.
/// for "struct/union" types, cast from the 'ptr' alias.
union HarbolIter {
	const bool       *b00l;
	const uint8_t    *uint8;   const int8_t   *int8;
	const uint16_t   *uint16;  const int16_t  *int16;
	const uint32_t   *uint32;  const int32_t  *int32;
	const uint64_t   *uint64;  const int64_t  *int64;
	const size_t     *size;    const ssize_t  *ssize;
	const uintptr_t  *uintptr; const intptr_t *intptr;
	
	const float32_t           *float32;
	const float64_t           *float64;
	const floatptr_t          *floatptr;
	const floatmax_t          *floatmax;
	
	const char                *string;
	const void                *ptr;
	const union HarbolIter    *self;
};

#ifdef __cplusplus
static inline NO_NULL uint8_t *make_buffer_from_binary(const char *const file_name, size_t *const restrict bytes)
#else
static inline NO_NULL uint8_t *make_buffer_from_binary(const char file_name[static 1], size_t *const restrict bytes)
#endif
{
	FILE *restrict file = fopen(file_name, "rb");
	if( file==NULL ) {
		return NULL;
	}
	
	const ssize_t filesize = get_file_size(file);
	if( filesize<=0 ) {
		fclose(file);
		return NULL;
	}
#ifdef __cplusplus
	uint8_t *restrict stream = reinterpret_cast< decltype(stream) >(calloc(filesize, sizeof *stream));
#else
	uint8_t *restrict stream = calloc(filesize, sizeof *stream);
#endif
	*bytes = fread(stream, sizeof *stream, filesize, file);
	fclose(file); file = NULL;
	return stream;
}

#ifdef __cplusplus
static inline NO_NULL char *make_buffer_from_text(const char *const file_name, size_t *const restrict len)
#else
static inline NO_NULL char *make_buffer_from_text(const char file_name[static 1], size_t *const restrict len)
#endif
{
	FILE *restrict file = fopen(file_name, "r");
	if( file==NULL ) {
		return NULL;
	}
	
	const ssize_t filesize = get_file_size(file);
	if( filesize<=0 ) {
		fclose(file);
		return NULL;
	}
	
#ifdef __cplusplus
	char *restrict stream = reinterpret_cast< decltype(stream) >(calloc(filesize + 1, sizeof *stream));
#else
	char *restrict stream = calloc(filesize + 1, sizeof *stream);
#endif
	*len = fread(stream, sizeof *stream, filesize, file);
	fclose(file); file = NULL;
	return stream;
}

static inline bool is_ptr_aligned(const void *const ptr, const size_t bytes) {
	return (( uintptr_t )ptr & (bytes-1))==0;
}

static inline NO_NULL void *dup_data(const void *const data, const size_t bytes)
{
#ifdef __cplusplus
	uint8_t *restrict cpy = reinterpret_cast< decltype(cpy) >(calloc(bytes, sizeof *cpy));
#else
	uint8_t *restrict cpy = calloc(bytes, sizeof *cpy);
#endif
	return( cpy==NULL ) ? NULL : memcpy(cpy, data, bytes);
}

#ifdef __cplusplus
static inline NO_NULL char *dup_str(const char *cstr)
#else
static inline char *dup_str(const char cstr[static 1])
#endif
{
	const size_t len = strlen(cstr);
#ifdef __cplusplus
	char *restrict cpy = reinterpret_cast< decltype(cpy) >(calloc(len + 1, sizeof *cpy));
#else
	char *restrict cpy = calloc(len + 1, sizeof *cpy);
#endif
	return( cpy==NULL ) ? NULL : strcpy(cpy, cstr);
}


#ifdef __cplusplus
static inline NO_NULL char *sprintf_alloc(const char *restrict fmt, ...)
#else
static inline char *sprintf_alloc(const char fmt[static 1], ...)
#endif
{
	va_list ap, st;
	va_start(ap, fmt);
	va_copy(st, ap);
	
	char c = 0;
	const int32_t size = vsnprintf(&c, 1, fmt, ap);
	va_end(ap);
	
#ifdef __cplusplus
	char *restrict text = reinterpret_cast< decltype(cpy) >(calloc(size + 2, sizeof *text));
#else
	char *restrict text = calloc(size + 2, sizeof *text);
#endif
	if( text != NULL ) {
		vsnprintf(text, size + 1, fmt, st);
	}
	va_end(st);
	return text;
}

static inline NO_NULL void harbol_print_tree_tabs(const size_t tabs, FILE *const f) {
	const size_t amount = tabs * 2;
	char str_branches[256] = {0};
	if( amount > 0 ) {
		char *end = &str_branches[0] + sizeof str_branches;
		char *p = harbol_memccpy(&str_branches[0], " ", 0, sizeof str_branches);
		for( size_t i=1; i<amount && p != NULL; i++ ) {
			p = harbol_memccpy(p - 1, " ", 0, end - p);
		}
	}
	fprintf(f, "%s", str_branches);
}

#endif /** HARBOL_COMMON_INCLUDES_INCLUDED */