
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



#define WORD_SIZE		4
#define STK_SIZE		(512*WORD_SIZE)		// 4096 4Kb
#define CALLSTK_SIZE	256					// 1024 bytes
#define MEM_SIZE		(256*WORD_SIZE)		// 1024 bytes 1Kb of memory

typedef		unsigned char		uchar;
typedef		unsigned char		bytecode[];
typedef		unsigned short		ushort;
typedef		unsigned int		uint;
typedef		unsigned long long	ulong;

typedef struct vm_cpu {
	uchar	bStack[STK_SIZE];			// 4096 bytes
	uint	bCallstack[CALLSTK_SIZE];	// 1024 bytes
	uchar	bMemory[MEM_SIZE];			// 1024 bytes
	uchar	*code;
	uint	ip, sp, callsp, bp;	// 16 bytes
} CVM_t;

union conv_union {
	uint	ui;
	int		i;
	float	f;
	ushort	us;
	short	s;
	uchar	c[WORD_SIZE];
};

// Safe mode enables bounds checking.
// this might slow down the interpreter on a smaller micro level since we're always checking
// if pointers or memory addresses go out of bounds.
#define SAFEMODE	1

void		vm_init(CVM_t *restrict vm, uchar *restrict program);
void		vm_reset(CVM_t *vm);
void		vm_exec(CVM_t *vm);
void		vm_debug_print_ptrs(const CVM_t *vm);
void		vm_debug_print_callstack(const CVM_t *vm);
void		vm_debug_print_stack(const CVM_t *vm);
void		vm_debug_print_memory(const CVM_t *vm);

uint		vm_pop_word(CVM_t *vm);
ushort		vm_pop_short(CVM_t *vm);
uchar		vm_pop_byte(CVM_t *vm);
float		vm_pop_float32(CVM_t *vm);

void		vm_push_word(CVM_t *restrict vm, const uint val);
void		vm_push_short(CVM_t *restrict vm, const ushort val);
void		vm_push_byte(CVM_t *restrict vm, const uchar val);
void		vm_push_float(CVM_t *restrict vm, const float val);

void		vm_write_word(CVM_t *restrict vm, const uint val, const uint address);
void		vm_write_short(CVM_t *restrict vm, const ushort val, const uint address);
void		vm_write_byte(CVM_t *restrict vm, const uchar val, const uint address);
void		vm_write_float(CVM_t *restrict vm, const float val, const uint address);
void		vm_write_bytearray(CVM_t *restrict vm, uchar *restrict val, const uint size, const uint address);

uint		vm_read_word(CVM_t *restrict vm, const uint address);
ushort		vm_read_short(CVM_t *restrict vm, const uint address);
uchar		vm_read_byte(CVM_t *restrict vm, const uint address);
float		vm_read_float(CVM_t *restrict vm, const uint address);
void		vm_read_bytearray(CVM_t *restrict vm, uchar *restrict buffer, const uint size, const uint address);

#ifdef __cplusplus
}
#endif

#endif	// VM_H_INCLUDED
