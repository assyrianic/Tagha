
#ifndef VM_H_INCLUDED
#define VM_H_INCLUDED

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

//#define IS_64BIT

#define WORD_SIZE		4
#define STK_SIZE		(512 * WORD_SIZE)		// 2,048 bytes or 2kb
#define CALLSTK_SIZE	256						// 1,024 bytes
#define MEM_SIZE		(16384 * WORD_SIZE)		// 65,536 bytes or 65kb of memory

typedef		unsigned char		uchar;
typedef		uchar				bytecode[];
typedef		unsigned short		ushort;
typedef		unsigned int		uint;
typedef		long long int		i64;	// long longs are at minimum 64-bits as defined by C99 standard
typedef		unsigned long long	u64;

// Bytecode header to store important info for our code.
// this will be entirely read as an unsigned char
typedef struct crown_header {
	ushort	uiMagic;	// verify bytecode ==> 0xC0DE 'code' - actual bytecode OR 0x0D11 'dll' - for library funcs
#ifdef IS_64BIT
	u64		ipstart;	// where does 'main' begin?
#else
	uint	ipstart;	// where does 'main' begin?
#endif
#ifdef IS_64BIT
	u64		ulDataSize;	// how many variables we got to place directly into memory?
#else
	uint	uiDataSize;	// how many variables we got to place directly into memory?
#endif
#ifdef IS_64BIT
	u64		ulStkSize;	// how many variables we have to place onto the data stack?
#else
	uint	uiStkSize;	// how many variables we have to place onto the data stack?
#endif
#ifdef IS_64BIT
	u64		uiInstrCount;	// how many instructions does the code have? This includes the arguments and operands.
#else
	uint	uiInstrCount;	// how many instructions does the code have? This includes the arguments and operands.
#endif
} CrownHeader_t;

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

typedef struct crown_vm {
	uchar	*pbMemory, *pbStack, *pInstrStream;
#ifdef IS_64BIT
	u64		ip, sp, bp;		// 16 bytes;
#else
	uint	ip, sp, bp;		// 12 bytes
#endif
	uint	uiMaxInstrs;
} CrownVM_t;

union conv_union {
	uint	ui;
	int		i;
	float	f;
	ushort	us;
	short	s;
	u64		ull;
	i64		ll;
	double	dbl;
	uchar	c[8];
};

// Safe mode enables bounds checking.
// this might slow down the interpreter on a smaller level since we're always checking
// if pointers or memory addresses go out of bounds but it does help.
#define SAFEMODE	1

void		vm_init(CrownVM_t *vm);
void		vm_load_code(CrownVM_t *restrict vm, uchar *restrict program);
void		vm_reset(CrownVM_t *vm);
void		vm_free(CrownVM_t *vm);
void		vm_exec(CrownVM_t *vm);

void		vm_debug_print_ptrs(const CrownVM_t *vm);
void		vm_debug_print_stack(const CrownVM_t *vm);
void		vm_debug_print_memory(const CrownVM_t *vm);

uint		vm_pop_long(CrownVM_t *vm);
ushort		vm_pop_short(CrownVM_t *vm);
uchar		vm_pop_byte(CrownVM_t *vm);
float		vm_pop_float32(CrownVM_t *vm);

void		vm_push_long(CrownVM_t *restrict vm, const uint val);
void		vm_push_short(CrownVM_t *restrict vm, const ushort val);
void		vm_push_byte(CrownVM_t *restrict vm, const uchar val);
void		vm_push_float32(CrownVM_t *restrict vm, const float val);

void		vm_write_long(CrownVM_t *restrict vm, const uint val, const uint address);
void		vm_write_short(CrownVM_t *restrict vm, const ushort val, const uint address);
void		vm_write_byte(CrownVM_t *restrict vm, const uchar val, const uint address);
void		vm_write_float32(CrownVM_t *restrict vm, const float val, const uint address);
void		vm_write_bytearray(CrownVM_t *restrict vm, uchar *restrict val, const uint size, const uint address);

uint		vm_read_long(CrownVM_t *restrict vm, const uint address);
ushort		vm_read_short(CrownVM_t *restrict vm, const uint address);
uchar		vm_read_byte(CrownVM_t *restrict vm, const uint address);
float		vm_read_float32(CrownVM_t *restrict vm, const uint address);
void		vm_read_bytearray(CrownVM_t *restrict vm, uchar *restrict buffer, const uint size, const uint address);

/*	NOT READY YET...
//	API to call C/C++ functions from scripts. Supports up to 5 params
//	Realistically, if you require more than 5 arguments, you could just group everything into a struct and pass its pointer.
typedef		void (*fnNative0)(CrownVM_t *restrict vm);
typedef		void (*fnNative1)(CrownVM_t *restrict vm, void *restrict param1);
typedef		void (*fnNative2)(CrownVM_t *restrict vm, void *restrict param1, void *restrict param2);
typedef		void (*fnNative3)(CrownVM_t *restrict vm, void *restrict param1, void *restrict param2, void *restrict param3);
typedef		void (*fnNative4)(CrownVM_t *restrict vm, void *restrict param1, void *restrict param2, void *restrict param3, void *restrict param4);
typedef		void (*fnNative5)(CrownVM_t *restrict vm, void *restrict param1, void *restrict param2, void *restrict param3, void *restrict param4, void *restrict param5);

typedef struct {
	union {
		fnNative0	fnNoArgs;
		fnNative1	fnOneArg;
		fnNative2	fnTwoArgs;
		fnNative3	fnTreArgs;
		fnNative4	fnFourArgs;
		fnNative5	fnPentaArgs;
	};
	uchar ucArgs;
	const char	*strName;
} NativeInfo;

int		vm_register_funcs(CrownVM_t *restrict vm, NativeInfo *arrNatives);
*/

#ifdef __cplusplus
}
#endif

#endif	// VM_H_INCLUDED

