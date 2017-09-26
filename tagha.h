
#ifndef TAGHA_H_INCLUDED
#define TAGHA_H_INCLUDED


#ifdef __cplusplus
extern "C" {
#endif

#include "ds.h"
#include <stdint.h>
#include <iso646.h>

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

#define TAGHA_VERSION_STR		"0.0.11a"
//typedef		uint32_t			uint64_t;
//#define		PRIWord				PRIu32
#define		PRIWord				PRIu64


struct TaghaScript;
struct TaghaVM;
typedef struct TaghaScript		Script_t;
typedef struct TaghaVM			TaghaVM_t;

//	API to call C/C++ functions from scripts.
//	account for function pointers to natives being executed on script side.
typedef		void (*fnNative_t)(struct TaghaScript *script, const uint32_t argc, const uint32_t bytes/*, uint8_t *arrParams*/);

typedef struct nativeinfo {
	const char	*strName;	// use as string literals
	fnNative_t	pFunc;
} NativeInfo_t;

bool		Tagha_register_natives(struct TaghaVM *vm, NativeInfo_t *arrNatives);


bool		Tagha_register_enum(struct TaghaVM *vm, const char *strEnum);
bool		Tagha_register_enum_val(struct TaghaVM *vm, const char *strEnumval, const uint64_t ulEnumVal);

bool		Tagha_register_struct(struct TaghaVM *vm, const char *strStruct);
bool		Tagha_register_struct_field(struct TaghaVM *vm, const char *strField, const uint32_t ulBytes);

bool		Tagha_register_union(struct TaghaVM *vm, const char *strUnion, const uint32_t ulBytes);
bool		Tagha_register_union_field(struct TaghaVM *vm, const char *strField);

/* Script File Structure.
 * magic verifier
 * Stack Vector
 * Native Table Vector
 * Func Table Dictionary
 * Data Table Dictionary
 */

typedef struct func_tbl {
	uint32_t	uiParams;
	uint32_t	uiEntry;	// TODO: make this into uint64?
} FuncTable_t;

typedef struct data_tbl {
	uint32_t	uiBytes;
	uint32_t	uiAddress;	// TODO: make this into uint64?
} DataTable_t;


/*
typedef struct handle {
	void		*pHostPtr;
	uint32_t	uiHNDL;
} Handle_t;
*/

struct TaghaScript {
	uint8_t		*pbMemory, *pInstrStream;
	char		**pstrNatives;	// natives table as stored strings.
	Map_t		*pmapFuncs;		// stores the functions compiled to script.
	Map_t		*pmapGlobals;	// stores global vars.
	//Vec_t		*pvecHostData;
	uint64_t		ip, sp, bp;
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
	Vec_t	*pvecScripts;	// all loaded scripts in here.
	Map_t	*pmapNatives;	// Natives are mapped here.
	Map_t	*pmapExpTypes;	// Exported Types from Host to Scripts.
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
	
	uint64_t	mmx_ull[2];
	int64_t		mmx_ll[2];
	uint32_t	mmx_ui[4];
	int32_t		mmx_i[4];
	float		mmx_f[4];
	uint16_t	mmx_us[8];
	int16_t		mmx_s[8];
	int8_t		mmx_c[16];
	uint8_t		mmx_uc[16];
};

void		Tagha_init(struct TaghaVM *vm);
void		Tagha_load_script(struct TaghaVM *vm, char *filename);
void		Tagha_free(struct TaghaVM *vm);
void		Tagha_exec(struct TaghaVM *vm);

void		TaghaScript_debug_print_ptrs(const struct TaghaScript *script);
void		TaghaScript_debug_print_memory(const struct TaghaScript *script);
void		TaghaScript_debug_print_instrs(const struct TaghaScript *script);
void		TaghaScript_reset(struct TaghaScript *script);
void		TaghaScript_free(struct TaghaScript *script);

uint32_t	TaghaScript_stacksize(const struct TaghaScript *script);
uint32_t	TaghaScript_instrsize(const struct TaghaScript *script);
uint32_t	TaghaScript_maxinstrs(const struct TaghaScript *script);
uint32_t	TaghaScript_nativecount(const struct TaghaScript *script);
uint32_t	TaghaScript_funcs(const struct TaghaScript *script);
uint32_t	TaghaScript_globals(const struct TaghaScript *script);
bool		TaghaScript_safemode_active(const struct TaghaScript *script);
bool		TaghaScript_debug_active(const struct TaghaScript *script);


void		TaghaScript_push_longfloat(struct TaghaScript *script, const long double val);
long double	TaghaScript_pop_longfloat(struct TaghaScript *script);

void		TaghaScript_push_int64(struct TaghaScript *script, const uint64_t val);
uint64_t	TaghaScript_pop_int64(struct TaghaScript *script);

void		TaghaScript_push_float64(struct TaghaScript *script, const double val);
double		TaghaScript_pop_float64(struct TaghaScript *script);

void		TaghaScript_push_int32(struct TaghaScript *script, const uint32_t val);
uint32_t	TaghaScript_pop_int32(struct TaghaScript *script);

void		TaghaScript_push_float32(struct TaghaScript *script, const float val);
float		TaghaScript_pop_float32(struct TaghaScript *script);

void		TaghaScript_push_short(struct TaghaScript *script, const uint16_t val);
uint16_t	TaghaScript_pop_short(struct TaghaScript *script);

void		TaghaScript_push_byte(struct TaghaScript *script, const uint8_t val);
uint8_t		TaghaScript_pop_byte(struct TaghaScript *script);

void		TaghaScript_push_nbytes(struct TaghaScript *script, void *pItem, const uint32_t bytesize);
void		TaghaScript_pop_nbytes(struct TaghaScript *script, void *pBuffer, const uint32_t bytesize);

/*
 * IDEA : have pointer dereferencing by load/store sp
 * actually dereference the address instead of using an address
 * as a relative offset array index.
 */
uint8_t		*TaghaScript_addr2ptr(struct TaghaScript *script, const uint64_t stk_address);

void		TaghaScript_call_func_by_name(struct TaghaScript *script, const char *strFunc);
void		TaghaScript_call_func_by_addr(struct TaghaScript *script, const uint64_t func_addr);

void		*TaghaScript_get_global_by_name(struct TaghaScript *script, const char *strGlobalName);
/*
uint32_t	TaghaScript_store_hostdata(struct TaghaScript *script, void *data);
void		*TaghaScript_get_hostdata(struct TaghaScript *script, const uint32_t id);
void		TaghaScript_del_hostdata(struct TaghaScript *script, const uint32_t id);
void		TaghaScript_free_hostdata(struct TaghaScript *script);
*/
#ifdef __cplusplus
}
#endif

#endif	// TAGHA_H_INCLUDED

