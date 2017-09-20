
#ifndef TAGHA_H_INCLUDED
#define TAGHA_H_INCLUDED


#ifdef __cplusplus
extern "C" {
#endif

#include "hashmap.h"
#include "vector.h"
#include <stdint.h>

//#define SIZE(x)		sizeof((x)) / sizeof((x)[0]);

//#define typename(x) _Generic((x),        /* Get the name of a type */           \
        _Bool: "_Bool",                    unsigned char: "unsigned char",        \
        char: "char",                      short: "short",                        \
        unsigned short: "unsigned short",  int: "int",                            \
        unsigned int: "unsigned int",      unsigned long long: "unsigned int64",  \
        long long: "long long", \
        float: "float",                    double: "double",                      \
        _Bool *: "_Bool *",                unsigned char *:"uint8_t *",             \
        char *: "char *",                  short *: "short *",                    \
        unsigned short *: "unsigned short *",  int *: "int *",                    \
        unsigned int *: "unsigned int *",      unsigned long long *: "unsigned long2 *",  \
        long long *: "long long *", \
        float *: "float *",                    double *: "double *",              \
        default: "other")

#define TAGHA_VERSION_STR		"0.0.9a"
typedef		uint32_t			Word_t;	// word size is 4 bytes


struct TaghaScript;
struct TaghaVM;
typedef struct TaghaScript		Script_t;
typedef struct TaghaVM			TaghaVM_t;

//	API to call C/C++ functions from scripts.
//	account for function pointers to natives being executed on script side.
typedef		void (*fnNative_t)(Script_t *script, const uint32_t argc, const uint32_t bytes/*, uint8_t *arrParams*/);

typedef struct nativeinfo {
	const char	*strName;	// use as string literals
	fnNative_t	pFunc;
} NativeInfo_t;

bool		Tagha_register_natives(TaghaVM_t *vm, NativeInfo_t *arrNatives);


typedef struct typeinfo {
	const char	*strType;
	void		*pAddr;
	uint32_t	uiSize;
} TypeInfo_t;

bool		Tagha_register_type(TaghaVM_t *vm, TypeInfo_t *arrTypes);


/* Script File Structure.
 * magic verifier
 * Stack Vector
 * Native Table Vector
 * Func Table Vector
 * Data Table Vector
 */

typedef struct func_tbl {
	uint32_t	uiParams;
	uint32_t	uiEntry;
} FuncTable_t;

typedef struct data_tbl {
	uint32_t	uiBytes;
	uint32_t	uiAddress;
} DataTable_t;

struct TaghaScript {
	uint8_t		*pbMemory, *pInstrStream;
	char		**ppstrNatives;		// natives table as stored strings.
	dict		*pmapFuncs;		// stores the functions compiled to script.
	dict		*pmapGlobals;	// stores global vars.
	Word_t		ip, sp, bp;
	uint32_t
		uiMemsize,
		uiInstrSize,
		uiMaxInstrs,
		uiNatives,
		uiFuncs,
		uiGlobals
	;
	bool	bSafeMode, bDebugMode;
};

struct TaghaVM {
	vector	*pvecScripts;
	dict	*pmapNatives;
};

union conv_union {	// converter union. for convenience
	uint32_t	ui;
	int32_t		i;
	float		f;
	uint16_t	us;
	int16_t		s;
	uint64_t	ull;
	int64_t		ll;
	double		dbl;
	uint8_t		c[8];
	
	uint32_t	mmx_ui[4];
	int32_t		mmx_i[4];
	float		mmx_f[4];
	uint16_t	mmx_us[8];
	int16_t		mmx_s[8];
	int8_t		mmx_c[16];
	uint8_t		mmx_uc[16];
};

void		Tagha_init(TaghaVM_t *vm);
void		Tagha_load_script(TaghaVM_t *vm, char *filename);
void		Tagha_free(TaghaVM_t *vm);
void		Tagha_exec(TaghaVM_t *vm);

void		TaghaScript_debug_print_ptrs(const Script_t *script);
void		TaghaScript_debug_print_memory(const Script_t *script);
void		TaghaScript_debug_print_instrs(const Script_t *script);
void		TaghaScript_reset(Script_t *script);
void		TaghaScript_free(Script_t *script);

uint32_t	TaghaScript_stacksize(const Script_t *script);
uint32_t	TaghaScript_instrsize(const Script_t *script);
uint32_t	TaghaScript_maxinstrs(const Script_t *script);
uint32_t	TaghaScript_nativecount(const Script_t *script);
uint32_t	TaghaScript_funcs(const Script_t *script);
uint32_t	TaghaScript_globals(const Script_t *script);
bool		TaghaScript_safemode_active(const Script_t *script);
bool		TaghaScript_debug_active(const Script_t *script);



void		TaghaScript_push_longfloat(Script_t *script, const long double val);
long double	TaghaScript_pop_longfloat(Script_t *script);

void		TaghaScript_push_int64(Script_t *script, const uint64_t val);
uint64_t	TaghaScript_pop_int64(Script_t *script);

void		TaghaScript_push_float64(Script_t *script, const double val);
double		TaghaScript_pop_float64(Script_t *script);

void		TaghaScript_push_int32(Script_t *script, const uint32_t val);
uint32_t	TaghaScript_pop_int32(Script_t *script);

void		TaghaScript_push_float32(Script_t *script, const float val);
float		TaghaScript_pop_float32(Script_t *script);

void		TaghaScript_push_short(Script_t *script, const uint16_t val);
uint16_t	TaghaScript_pop_short(Script_t *script);

void		TaghaScript_push_byte(Script_t *script, const uint8_t val);
uint8_t		TaghaScript_pop_byte(Script_t *script);

void		TaghaScript_push_nbytes(Script_t *script, void *pItem, const Word_t bytesize);
void		TaghaScript_pop_nbytes(Script_t *script, void *pBuffer, const Word_t bytesize);

uint8_t		*TaghaScript_addr2ptr(Script_t *script, const Word_t stk_address);

void		TaghaScript_call_func_by_name(Script_t *script, const char *strFunc);
void		TaghaScript_call_func_by_addr(Script_t *script, const Word_t func_addr);

uint8_t		*TaghaScript_get_global_by_name(Script_t *script, const char *strGlobalName);

#ifdef __cplusplus
}
#endif

#endif	// TAGHA_H_INCLUDED

