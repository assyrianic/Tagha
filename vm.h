
#ifndef CROWNVM_H_INCLUDED
#define CROWNVM_H_INCLUDED

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

//#define SIZE(x)		sizeof((x)) / sizeof((x)[0]);

//#define typename(x) _Generic((x),        /* Get the name of a type */           \
        _Bool: "_Bool",                    unsigned char: "unsigned char",        \
        char: "char",                      short: "short",                        \
        unsigned short: "unsigned short",  int: "int",                            \
        unsigned int: "unsigned int",      unsigned long long: "unsigned int64",  \
        long long: "long long", \
        float: "float",                    double: "double",                      \
        _Bool *: "_Bool *",                unsigned char *:"uchar *",             \
        char *: "char *",                  short *: "short *",                    \
        unsigned short *: "unsigned short *",  int *: "int *",                    \
        unsigned int *: "unsigned int *",      unsigned long long *: "unsigned long2 *",  \
        long long *: "long long *", \
        float *: "float *",                    double *: "double *",              \
        default: "other")


#define WORD_SIZE		8
#define STK_SIZE		(256 * WORD_SIZE)		// 2,048 bytes or 2kb
#define MEM_SIZE		(8192 * WORD_SIZE)		// 65,536 bytes or 65kb of memory

typedef		unsigned char		uchar;
typedef		uchar				bytecode[];
typedef		unsigned short		ushort;
typedef		unsigned int		uint;
typedef		long long int		llong;	// long longs are at minimum 64-bits as defined by C99 standard
typedef		unsigned long long	ullong;


// Bytecode header to store important info for our code.
// this will be entirely read as an unsigned char of course.
// actual Header format:
// magic, memsize, stksize, ipstart, instruction count

// how many beginning bytes the header takes up.
#define HEADER_BYTES	34

// Script structure
// Vector of Instruction Stream
// Vector of Memory
// Vector of Stack
// entry address/offset

typedef struct __script {
	uchar	*pMemory, *pStack, *pInstrStream;
	ullong	ip, sp, bp;		// 24 bytes
	
	// vector *pFuncTable;	// 
	ullong	uiMemSize;		// how much memory does script need?
	ullong	uiStkSize;		// how large of a stack does script need?
	ullong	ipstart;		// where does 'main' begin?
	ullong	uiInstrCount;	// how many instructions does the code have? This includes the arguments and operands.
	ushort	magic;			// verify bytecode ==> 0xC0DE 'code' - actual bytecode.
} Script_t;


/*	normally a program's memory layout is...
+------------------+
|    stack   |     |      high address
|    ...     v     |
|                  |
|                  |
|                  |
|                  |
|    ...     ^     |
|    heap    |     |
+------------------+
| bss  segment     |
+------------------+
| data segment     |
+------------------+
| text segment     |      low address
+------------------+
* 
* but we're gonna do something different here since the stack, callstack, and memory are separate.
* Plugins will have a similar layout but heap is replaced with callstack.
*/

// TODO: replace with vector so we can have multiple scripts.
// TODO: even better, upgrade vector into a hashmap so we can look up plugins by name.

typedef struct CrownVM {
	Script_t	*pScript;
} CrownVM_t;

union conv_union {
	uint	ui;
	int		i;
	float	f;
	ushort	us;
	short	s;
	ullong	ull;
	llong	ll;
	double	dbl;
	uchar	c[WORD_SIZE];
};

/*
typedef void *(*ExportFunc_t)(CrownVM_t *, void *userdata);

typedef struct export {
	const char		*funcName;
	ExportFunc_t	exprtFunc;
} Export_t;
*/

// Safe mode enables bounds checking.
// this might slow down the interpreter on a smaller level since we're always checking
// if pointers or memory addresses go out of bounds but it does help.
#define SAFEMODE	1

void	crown_init(CrownVM_t *vm);
void	crown_load_script(CrownVM_t *restrict vm, uchar *restrict program);
void	crown_free_script(CrownVM_t *restrict vm);
void	script_reset(Script_t *pScript);
//void	crown_free(CrownVM_t *vm);
void	crown_exec(CrownVM_t *vm);
void	scripts_debug_print_ptrs(const Script_t *pScript);
void	scripts_debug_print_stack(const Script_t *pScript);
void	scripts_debug_print_memory(const Script_t *pScript);

ullong	script_pop_quad(Script_t *pScript);
uint	script_pop_long(Script_t *pScript);
ushort	script_pop_short(Script_t *pScript);
uchar	script_pop_byte(Script_t *pScript);
float	script_pop_float32(Script_t *pScript);
double	script_pop_float64(Script_t *pScript);

void	script_push_quad(Script_t *restrict pScript, const ullong val);
void	script_push_long(Script_t *restrict pScript, const uint val);
void	script_push_short(Script_t *restrict pScript, const ushort val);
void	script_push_byte(Script_t *restrict pScript, const uchar val);
void	script_push_float32(Script_t *restrict pScript, const float val);
void	script_push_float64(Script_t *restrict pScript, const double val);

void	script_write_quad(Script_t *restrict pScript, const ullong val, const ullong address);
void	script_write_long(Script_t *restrict pScript, const uint val, const ullong address);
void	script_write_short(Script_t *restrict pScript, const ushort val, const ullong address);
void	script_write_byte(Script_t *restrict pScript, const uchar val, const ullong address);
void	script_write_float32(Script_t *restrict pScript, const float val, const ullong address);
void	script_write_float64(Script_t *restrict pScript, const double val, const ullong address);
void	script_write_bytearray(Script_t *restrict pScript, uchar *restrict val, const uint size, const ullong address);

ullong	script_read_quad(Script_t *restrict pScript, const ullong address);
uint	script_read_long(Script_t *restrict pScript, const ullong address);
ushort	script_read_short(Script_t *restrict pScript, const ullong address);
uchar	script_read_byte(Script_t *restrict pScript, const ullong address);
float	script_read_float32(Script_t *restrict pScript, const ullong address);
double	script_read_float64(Script_t *restrict pScript, const ullong address);
void	script_read_bytearray(Script_t *restrict pScript, uchar *restrict buffer, const uint size, const ullong address);

#ifdef __cplusplus
}
#endif

#endif	// CROWNVM_H_INCLUDED
