
#ifndef TaghaScript_H_INCLUDED
#define TaghaScript_H_INCLUDED

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
#define MEM_SIZE		(4096 * WORD_SIZE)		// 65,536 bytes or 65kb of memory
#define NULL_ADDR		0xFFFFFFFF				// all NULL pointers should have this value

typedef		unsigned char		uchar;
typedef		uchar				bytecode[];
typedef		unsigned short		ushort;
typedef		unsigned int		uint;
typedef		long long int		i64;	// long longs are at minimum 64-bits as defined by C99 standard
typedef		unsigned long long	u64;

typedef		uint				Word_t;	// word size is 4 bytes

// Bytecode header to store important info for our code.
// this will be entirely read as an unsigned char
typedef struct Taghaheader {
	ushort	uiMagic;	// verify bytecode ==> 0xC0DE 'code' - actual bytecode OR 0x0D11 'dll' - for library funcs
	uint	ipstart;	// where does 'main' begin?
	uint	uiDataSize;	// how many variables we got to place directly into memory?
	uint	uiStkSize;	// how many variables we have to place onto the data stack?
	uint	uiInstrCount;	// how many instructions does the code have? This includes the arguments and operands.
	
} TaghaHeader_t;


struct TaghaScript;
struct TaghaVM; 
typedef struct TaghaScript		Script_t;
typedef struct TaghaVM			TaghaVM_t;

//	API to call C/C++ functions from scripts.
//	account for function pointers to natives being executed on script side.
typedef		void (*fnNative)(Script_t *restrict script, const uchar argc, const uint bytes, uchar *arrParams);

typedef struct native_info {
	fnNative			fnpFunc;
	const char			*strFuncName;
	struct native_info	*pNext;
} NativeInfo_t;
/*
typedef struct native_map {
	NativeInfo_t	**arrpNatives;
	uint			uiSize, uiCount;
} NativeMap_t;
*/
//int	Tagha_register_natives(Script_t *restrict script, NativeInfo_t *Natives);


struct TaghaScript {
	uchar	*pbMemory, *pbStack, *pInstrStream;
	fnNative	fnpNative;
	uint	ip, sp, bp;		// 12 bytes
	uint	uiMaxInstrs;
	bool	bSafeMode;
};

struct TaghaVM {
	// vector *Scripts;
	Script_t *pScript;
};

union conv_union {	// converter union. for convenience
	uint	ui;
	int		i;
	float	f;
	ushort	us;
	short	s;
	u64		ull;
	i64		ll;
	double	dbl;
	uchar	c[8];
	
	uint	mmx_ui[2];
	int		mmx_i[2];
	float	mmx_f[2];
	ushort	mmx_us[4];
	short	mmx_s[4];
	char	mmx_c[8];
};

void	Tagha_init(TaghaVM_t *vm);
void	Tagha_load_script(TaghaVM_t *restrict vm, char *restrict filename);
void	Tagha_free(TaghaVM_t *vm);
void	Tagha_exec(TaghaVM_t *vm);
int		Tagha_register_native(TaghaVM_t *restrict vm, fnNative pNative);

void	TaghaScript_debug_print_ptrs(const Script_t *script);
void	TaghaScript_debug_print_stack(const Script_t *script);
void	TaghaScript_debug_print_memory(const Script_t *script);


void	TaghaScript_reset(Script_t *script);
void	TaghaScript_push_longfloat(Script_t *restrict script, const long double val);
long double	TaghaScript_pop_longfloat(Script_t *script);

void	TaghaScript_push_int64(Script_t *restrict script, const u64 val);
u64		TaghaScript_pop_int64(Script_t *script);

void	TaghaScript_push_float64(Script_t *restrict script, const double val);
double	TaghaScript_pop_float64(Script_t *script);

void	TaghaScript_push_int32(Script_t *restrict script, const uint val);
uint	TaghaScript_pop_int32(Script_t *script);

void	TaghaScript_push_float32(Script_t *restrict script, const float val);
float	TaghaScript_pop_float32(Script_t *script);

void	TaghaScript_push_short(Script_t *restrict script, const ushort val);
ushort	TaghaScript_pop_short(Script_t *script);

void	TaghaScript_push_byte(Script_t *restrict script, const uchar val);
uchar	TaghaScript_pop_byte(Script_t *script);

void	TaghaScript_push_nbytes(Script_t *restrict script, void *restrict pItem, const Word_t bytesize);
void	TaghaScript_pop_nbytes(Script_t *restrict script, void *restrict pBuffer, const Word_t bytesize);


long double	TaghaScript_read_longfloat(Script_t *restrict script, const Word_t address);
void	TaghaScript_write_longfloat(Script_t *restrict script, const long double val, const Word_t address);

u64		TaghaScript_read_int64(Script_t *restrict script, const Word_t address);
void	TaghaScript_write_int64(Script_t *restrict script, const u64 val, const Word_t address);

double	TaghaScript_read_float64(Script_t *restrict script, const Word_t address);
void	TaghaScript_write_float64(Script_t *restrict script, const double val, const Word_t address);

uint	TaghaScript_read_int32(Script_t *restrict script, const Word_t address);
void	TaghaScript_write_int32(Script_t *restrict script, const uint val, const Word_t address);

float	TaghaScript_read_float32(Script_t *restrict script, const Word_t address);
void	TaghaScript_write_float32(Script_t *restrict script, const float val, const Word_t address);

ushort	TaghaScript_read_short(Script_t *restrict script, const Word_t address);
void	TaghaScript_write_short(Script_t *restrict script, const ushort val, const Word_t address);

uchar	TaghaScript_read_byte(Script_t *restrict script, const Word_t address);
void	TaghaScript_write_byte(Script_t *restrict script, const uchar val, const Word_t address);

void	TaghaScript_read_nbytes(Script_t *restrict script, void *restrict pBuffer, const Word_t bytesize, const Word_t address);
void	TaghaScript_write_nbytes(Script_t *restrict script, void *restrict pItem, const Word_t bytesize, const Word_t address);


long double	*TaghaScript_addr2ptr_longfloat(Script_t *restrict script, const Word_t address);
u64		*TaghaScript_addr2ptr_int64(Script_t *restrict script, const Word_t address);
double	*TaghaScript_addr2ptr_float64(Script_t *restrict script, const Word_t address);
uint	*TaghaScript_addr2ptr_int32(Script_t *restrict script, const Word_t address);
float	*TaghaScript_addr2ptr_float32(Script_t *restrict script, const Word_t address);
ushort	*TaghaScript_addr2ptr_short(Script_t *restrict script, const Word_t address);
uchar	*TaghaScript_addr2ptr_byte(Script_t *restrict script, const Word_t address);


long double *TaghaScript_stkaddr2ptr_longfloat(Script_t *restrict script, const Word_t address);
u64		*TaghaScript_stkaddr2ptr_int64(Script_t *restrict script, const Word_t address);
double	*TaghaScript_stkaddr2ptr_float64(Script_t *restrict script, const Word_t address);
uint	*TaghaScript_stkaddr2ptr_int32(Script_t *restrict script, const Word_t address);
float	*TaghaScript_stkaddr2ptr_float32(Script_t *restrict script, const Word_t address);
ushort	*TaghaScript_stkaddr2ptr_short(Script_t *restrict script, const Word_t address);
uchar	*TaghaScript_stkaddr2ptr_byte(Script_t *restrict script, const Word_t address);


#ifdef __cplusplus
}
#endif

#endif	// TaghaScript_H_INCLUDED

