
#ifndef tagha_H_INCLUDED
#define tagha_H_INCLUDED

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

typedef		uint				Word_t;	// word size is 32-bits

// Bytecode header to store important info for our code.
// this will be entirely read as an unsigned char
typedef struct taghaheader {
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

struct Taghavm;
typedef struct Taghavm		TaghaVM_t;

//	API to call C/C++ functions from scripts.
typedef		void (*fnNative)(TaghaVM_t *restrict vm, void *retVal, void **arrParams, const uint argc);

typedef struct native_info {
	fnNative			fnpFunc;
	const char			*strFuncName;
	struct native_info	*pNext;
} NativeInfo_t;

typedef struct native_map {
	NativeInfo_t	**arrpNatives;
	uint			uiSize, uiCount;
} NativeMap_t;

int		tagha_register_funcs(TaghaVM_t *restrict vm, NativeInfo_t **Natives);

struct Taghavm {
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

void	tagha_init(TaghaVM_t *vm);
void	tagha_load_code(TaghaVM_t *restrict vm, char *restrict filename);
void	tagha_reset(TaghaVM_t *vm);
void	tagha_free(TaghaVM_t *vm);
void	tagha_exec(TaghaVM_t *vm);

void	tagha_debug_print_ptrs(const TaghaVM_t *vm);
void	tagha_debug_print_stack(const TaghaVM_t *vm);
void	tagha_debug_print_memory(const TaghaVM_t *vm);

void	tagha_push_long(TaghaVM_t *restrict vm, const uint val);
uint	tagha_pop_long(TaghaVM_t *vm);

void	tagha_push_float32(TaghaVM_t *restrict vm, const float val);
float	tagha_pop_float32(TaghaVM_t *vm);

void	tagha_push_short(TaghaVM_t *restrict vm, const ushort val);
ushort	tagha_pop_short(TaghaVM_t *vm);

void	tagha_push_byte(TaghaVM_t *restrict vm, const uchar val);
uchar	tagha_pop_byte(TaghaVM_t *vm);

void	tagha_push_nbytes(TaghaVM_t *restrict vm, void *restrict pItem, const uint bytesize);
void	tagha_pop_nbytes(TaghaVM_t *restrict vm, void *restrict pBuffer, const uint bytesize);


uint	tagha_read_long(TaghaVM_t *restrict vm, const Word_t address);
void	tagha_write_long(TaghaVM_t *restrict vm, const uint val, const Word_t address);

ushort	tagha_read_short(TaghaVM_t *restrict vm, const Word_t address);
void	tagha_write_short(TaghaVM_t *restrict vm, const ushort val, const Word_t address);

uchar	tagha_read_byte(TaghaVM_t *restrict vm, const Word_t address);
void	tagha_write_byte(TaghaVM_t *restrict vm, const uchar val, const Word_t address);

float	tagha_read_float32(TaghaVM_t *restrict vm, const Word_t address);
void	tagha_write_float32(TaghaVM_t *restrict vm, const float val, const Word_t address);

void	tagha_read_nbytes(TaghaVM_t *restrict vm, void *restrict pBuffer, const uint bytesize, const Word_t address);
void	tagha_write_nbytes(TaghaVM_t *restrict vm, void *restrict pItem, const uint bytesize, const Word_t address);




#ifdef __cplusplus
}
#endif

#endif	// tagha_H_INCLUDED

