
#ifndef Tagha_H_INCLUDED
#define Tagha_H_INCLUDED

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


#define WORD_SIZE		4
#define STK_SIZE		(512 * WORD_SIZE)		// 2,048 bytes or 2kb
#define CALLSTK_SIZE	256						// 1,024 bytes
#define MEM_SIZE		(16384 * WORD_SIZE)		// 65,536 bytes or 65kb of memory
#define NULL_ADDRESS	0xFFFFFFFF				// all NULL pointers should have this value

typedef		unsigned char		uchar;
typedef		uchar				bytecode[];
typedef		unsigned short		ushort;
typedef		unsigned int		uint;
typedef		long long int		i64;	// long longs are at minimum 64-bits as defined by C99 standard
typedef		unsigned long long	u64;

// Bytecode header to store important info for our code.
// this will be entirely read as an unsigned char
typedef struct Tagha_header {
	ushort	uiMagic;	// verify bytecode ==> 0xC0DE 'code' - actual bytecode OR 0x0D11 'dll' - for library funcs
	uint	ipstart;	// where does 'main' begin?
	uint	uiDataSize;	// how many variables we got to place directly into memory?
	uint	uiStkSize;	// how many variables we have to place onto the data stack?
	uint	uiInstrCount;	// how many instructions does the code have? This includes the arguments and operands.
	
} TaghaHeader_t;

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

struct Tagha_vm;
typedef struct Tagha_vm		TaghaVM_t;

//	API to call C/C++ functions from scripts.
typedef		void (*fnNative)(TaghaVM_t *restrict vm, ...);

typedef struct native_info {
	fnNative			fnpFunc;
	const char			*strFuncName;
	struct native_info	*pNext;
	uchar				ucArgCount, ucArgBytes;
} NativeInfo_t;

typedef struct native_map {
	NativeInfo_t	**arrpNatives;
	uint			uiSize, uiCount;
} NativeMap_t;

int		Tagha_register_funcs(TaghaVM_t *restrict vm, NativeInfo_t **Natives);

struct Tagha_vm {
	uchar	*pbMemory, *pbStack, *pInstrStream;
	NativeMap_t	**arrpNativeTable;
	uint	ip, sp, bp;		// 12 bytes
	uint	uiMaxInstrs;
	bool	bSafeMode;
};
 
union conv_union {	// converter union.
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

void		Tagha_init(TaghaVM_t *vm);
void		Tagha_load_code(TaghaVM_t *restrict vm, uchar *restrict program);
void		Tagha_reset(TaghaVM_t *vm);
void		Tagha_free(TaghaVM_t *vm);
void		Tagha_exec(TaghaVM_t *vm);

void		Tagha_debug_print_ptrs(const TaghaVM_t *vm);
void		Tagha_debug_print_stack(const TaghaVM_t *vm);
void		Tagha_debug_print_memory(const TaghaVM_t *vm);

uint		Tagha_pop_long(TaghaVM_t *vm);
ushort		Tagha_pop_short(TaghaVM_t *vm);
uchar		Tagha_pop_byte(TaghaVM_t *vm);
float		Tagha_pop_float32(TaghaVM_t *vm);

void		Tagha_push_long(TaghaVM_t *restrict vm, const uint val);
void		Tagha_push_short(TaghaVM_t *restrict vm, const ushort val);
void		Tagha_push_byte(TaghaVM_t *restrict vm, const uchar val);
void		Tagha_push_float32(TaghaVM_t *restrict vm, const float val);

void		Tagha_write_long(TaghaVM_t *restrict vm, const uint val, const uint address);
void		Tagha_write_short(TaghaVM_t *restrict vm, const ushort val, const uint address);
void		Tagha_write_byte(TaghaVM_t *restrict vm, const uchar val, const uint address);
void		Tagha_write_float32(TaghaVM_t *restrict vm, const float val, const uint address);
void		Tagha_write_bytearray(TaghaVM_t *restrict vm, uchar *restrict val, const uint size, const uint address);

uint		Tagha_read_long(TaghaVM_t *restrict vm, const uint address);
ushort		Tagha_read_short(TaghaVM_t *restrict vm, const uint address);
uchar		Tagha_read_byte(TaghaVM_t *restrict vm, const uint address);
float		Tagha_read_float32(TaghaVM_t *restrict vm, const uint address);
void		Tagha_read_bytearray(TaghaVM_t *restrict vm, uchar *restrict buffer, const uint size, const uint address);


#ifdef __cplusplus
}
#endif

#endif	// Tagha_H_INCLUDED

