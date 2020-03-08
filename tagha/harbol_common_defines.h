#ifndef HARBOL_COMMON_DEFINES_INCLUDED
#	define HARBOL_COMMON_DEFINES_INCLUDED

/* Check if Windows */
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
#	ifndef OS_WINDOWS
#		define OS_WINDOWS 1
#	endif

/* Check if Linux/UNIX & FreeBSD */
#elif defined(unix) || defined(__unix) || defined(__unix__) || defined(__linux__) || defined(linux) || defined(__linux) || defined(__freeBSD__)
#	ifndef OS_LINUX_UNIX
#		define OS_LINUX_UNIX 1
#	endif

/* Check if Android */
#elif defined(__ANDROID__)
#	ifndef OS_ANDROID
#		define OS_ANDROID 1
#	endif

/* Check if Solaris/SunOS */
#elif defined(sun) || defined(__sun)
#	if defined(__SVR4) || defined(__svr4__)
#		ifndef OS_SOLARIS
#			define OS_SOLARIS 1
#		endif
#	else
#		ifndef OS_SUNOS
#			define OS_SUNOS 1
#		endif
#	endif

/* Check if Macintosh/MacOS */
#elif defined(macintosh) || defined(Macintosh) || defined(__APPLE__)
#	ifndef OS_MAC
#		define OS_MAC 1
#	endif

#endif /* end OS checks */

/* check what compiler we got */
#if defined(__clang__)
#	ifndef COMPILER_CLANG
#		define COMPILER_CLANG
#	endif
#elif defined(__GNUC__) || defined(__GNUG__)
#	ifndef COMPILER_GCC
#		define COMPILER_GCC
#	endif
#elif defined(_MSC_VER)
#	ifndef COMPILER_MSVC
#		define COMPILER_MSVC
#	endif
#elif defined(__INTEL_COMPILER)
#	ifndef COMPILER_INTEL
#		define COMPILER_INTEL
#	endif
#endif /* end compiler check macros */

/* set up the C standard macros! */
#ifdef __STDC__
#	ifndef C89
#		define C89
#	endif
#	ifdef __STDC_VERSION__
#		ifndef C90
#			define C90
#		endif
#		if (__STDC_VERSION__ >= 199409L)
#			ifndef C94
#				define C94
#			endif
#		endif
#		if (__STDC_VERSION__ >= 199901L)
#			ifndef C99
#				define C99
#			endif
#		endif
#		if (__STDC_VERSION__ >= 201112L)
#			ifndef C11
#				define C11
#			endif
#		endif
#		if (__STDC_VERSION__ >= 201710L)
#			ifndef C18
#				define C18
#			endif
#		endif
#	endif
#endif


/* setup RAII destructor macro if possibru to mark functions as cleaner-uppers. */
#ifndef RAII_DTOR
#	if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#		define RAII_DTOR(func) __attribute__ ((cleanup((func))))
#	else
#		define RAII_DTOR(func)
#	endif
#endif

/* setup macro to mark a param as "cannot or can never be NULL". */
#ifndef NEVER_NULL
#	if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#		define NEVER_NULL(...) __attribute__( (nonnull(__VA_ARGS__)) )
#	else
#		define NEVER_NULL(...)
#	endif
#endif

/* setup macro that declares all params to never be NULL. */
#ifndef NO_NULL
#	if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#		define NO_NULL __attribute__((nonnull))
#	else
#		define NO_NULL
#	endif
#endif

/* setup macro to mark a function as never returning a null pointer. */
#ifndef NONNULL_RET
#	if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#		define NONNULL_RET __attribute__((returns_nonnull))
#	else
#		define NONNULL_RET
#	endif
#endif

/* setup macro that marks a function as deprecated. */
#ifndef DEPRECATED
#	if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#		define DEPRECATED(func) func __attribute__ ((deprecated))
#	elif defined(COMPILER_MSVC)
#		define DEPRECATED(func) __declspec(deprecated) func
#	else
#		define DEPRECATED(func)
#	endif
#endif

/* setup macro that defines a var that it may be aliased by other data. */
#ifndef PTR_ALIAS
#	if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#		define PTR_ALIAS __attribute__ ((__may_alias__))
#	else
#		define PTR_ALIAS
#	endif
#endif

/* setup macro that marks a function as having hidden visibility. */
#ifndef VIS_HIDDEN
#	if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#		define VIS_HIDDEN __attribute__ ((visibility ("hidden")))
#	else
#		define VIS_HIDDEN
#	endif
#endif

/* setup macro that marks a function as having internal visibility. */
#ifndef VIS_INTERNAL
#	if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#		define VIS_INTERNAL __attribute__ ((visibility ("internal")))
#	else
#		define VIS_INTERNAL
#	endif
#endif

/* setup macro that marks a function as having protected visibility. */
#ifndef VIS_PROTECTED
#	if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#		define VIS_PROTECTED __attribute__ ((visibility ("protected")))
#	else
#		define VIS_PROTECTED
#	endif
#endif

/* setup macro that gives a warning if a function's result is unused. */
#ifndef WARN_UNUSED_RESULT
#	if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#		define WARN_UNUSED_RESULT __attribute__ ((warn_unused_result))
#	else
#		define WARN_UNUSED_RESULT
#	endif
#endif

/* setup macro to mark a function or variable as unused and ignore unused warnings. */
#ifndef UNUSED
#	if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#		define UNUSED __attribute__ ((unused))
#	else
#		define UNUSED
#	endif
#endif

/* setup macro to mark a function as a hot spot, thus requiring aggressive optimizations. */
#ifndef HOT
#	if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#		define HOT __attribute__ ((hot))
#	else
#		define HOT
#	endif
#endif

/* setup macro to make vector types. Argument must be power of 2.
 * Example:
	typedef __attribute__ ((vector_size (32))) int int_vec32_t; which makes int_vec32_t as 32-bytes.
 * 
 * All the basic integer types can be used as base types, both as signed and as unsigned: char, short, int, long, long long. In addition, float and double can be used to build floating-point vector types. 
 * +, -, *, /, unary minus, ^, |, &, ~, %. are the only operators allowed.
 * 
 * Bigger Example:
	typedef int v4si __attribute__ ((vector_size (16)));
	v4si a = {1,2,3,4};
	v4si b = {3,2,1,4};
	v4si c;
	
	c = a >  b;     // The result would be {0, 0,-1, 0}
	c = a == b;     // The result would be {0,-1, 0,-1}
 */
#ifndef SIMD_VECTOR
#	if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#		define SIMD_VECTOR(bytes) __attribute__ ((vector_size ((bytes))))
#	else
#		define SIMD_VECTOR(bytes)
#	endif
#endif

/* setup macro to define the data type mode.
 * available modes:
 * -- __byte__ -> type is one byte integer.
 * -- __word__ -> type is word-sized (native integer width) integer.
 * -- __pointer__ -> type is size of pointer integer.
 * 
 * -- __QI__ -> type is 1 byte integer.
 * -- __HI__ -> type is 2 bytes integer.
 * -- __SI__ -> type is 4 bytes integer.
 * -- __DI__ -> type is 8 bytes integer.
 * -- __TI__ -> type is 16 bytes integer.
 * -- __OI__ -> type is 32 bytes integer.
 * -- __XI__ -> type is 64 bytes integer.
 * 
 * -- __QF__ -> type is 1 byte float.
 * -- __HF__ -> type is 2 bytes float.
 * -- __SF__ -> type is 4 bytes float.
 * -- __DF__ -> type is 8 bytes float.
 * -- __XF__ -> type is 10 or 12 or 16 bytes float.
 * -- __TF__ -> type is 16 bytes float.
 */
#ifndef TYPE_MODE
#	if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#		define TYPE_MODE(mode) __attribute__ ((__mode__ (mode)))
#	else
#		define TYPE_MODE(mode)
#	endif
#endif

/* DLL crap to deal with Windows asinine poppycock DLL construction. */
#ifdef HARBOL_DLL
#	ifndef HARBOL_LIB 
#		define HARBOL_EXPORT __declspec(dllimport)
#	else
#		define HARBOL_EXPORT __declspec(dllexport)
#	endif
#else
#	define HARBOL_EXPORT
#endif

#ifdef __cplusplus
#	ifdef OS_WINDOWS
#		ifndef restrict
#			define restrict __restrict
#		endif
#	else
#		ifndef restrict
#			define restrict __restrict__
#		endif
#	endif
#endif


#ifdef COMPILER_MSVC
#	ifndef inline
#		define inline    __inline
#	endif
#elif !defined __STDC_VERSION__ || __STDC_VERSION__ < 199901L
#	ifndef inline
#		define inline    __inline__
#	endif
#endif


#ifdef C11
#	ifndef harbol_is_type
#		define harbol_is_type(n, T)    _Generic((n), T:true, default:false)
#	endif
#endif

#endif /* HARBOL_COMMON_DEFINES_INCLUDED */
