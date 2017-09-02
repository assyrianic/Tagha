
#ifndef TAGHA_H_INCLUDED
#define TAGHA_H_INCLUDED

#include "hashmap.h"

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

#define TAGHA_VERSION_STR		"0.0.6a"
#define WORD_SIZE		4
#define NULL_ADDR		0				// all NULL pointers should have this value

typedef		unsigned char		uchar;
typedef		uchar				bytecode[];
typedef		unsigned short		ushort;
typedef		unsigned int		uint;
typedef		long long int		i64;	// long longs are at minimum 64-bits as defined by C99 standard
typedef		unsigned long long	u64;

typedef		uint				Word_t;	// word size is 4 bytes


struct TaghaScript;
struct TaghaVM; 
typedef struct TaghaScript		Script_t;
typedef struct TaghaVM			TaghaVM_t;

//	API to call C/C++ functions from scripts.
//	account for function pointers to natives being executed on script side.
typedef		void (*fnNative_t)(Script_t *restrict script, const uint argc, const uint bytes, uchar *arrParams);

typedef struct nativeinfo {
	const char	*strName;
	fnNative_t	pFunc;
} NativeInfo_t;

int	Tagha_register_natives(TaghaVM_t *restrict vm, NativeInfo_t *arrNatives);

/* Script File Structure.
 * magic verifier
 * Stack Vector
 * NativeTable Vector
 */
 
struct TaghaScript {
	uchar	*pbMemory, *pInstrStream;
	char	**ppstrNatives;	// natives table as stored strings.
	Word_t	ip, sp, bp;
	uint	uiMemsize;
	uint	uiInstrSize;
	uint	uiMaxInstrs;
	uint	uiNatives;		// count how many natives script uses.
	bool	bSafeMode;
};

struct TaghaVM {
	// vector *Scripts;
	Script_t *pScript;
	// Natives should be GLOBALLY available to all scripts on a per-header-basis...
	// meaning that a script can only use a native if the header has it.
	dict	*pmapNatives;
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

void	TaghaScript_debug_print_ptrs(const Script_t *script);
void	TaghaScript_debug_print_memory(const Script_t *script);
void	TaghaScript_debug_print_instrs(const Script_t *script);
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

uchar	*TaghaScript_addr2ptr(Script_t *restrict script, const Word_t stk_address);

#ifdef __cplusplus
}
#endif

#endif	// TAGHA_H_INCLUDED

