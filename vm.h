
#ifndef VM_H_INCLUDED
#define VM_H_INCLUDED

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define typename(x) _Generic((x),        /* Get the name of a type */             \
        _Bool: "_Bool",                  unsigned char: "unsigned char",          \
         char: "char",                     signed char: "signed char",            \
    short int: "short int",         unsigned short int: "unsigned short int",     \
          int: "int",                     unsigned int: "unsigned int",           \
     long int: "long int",           unsigned long int: "unsigned long int",      \
long long int: "long long int", unsigned long long int: "unsigned long long int", \
        float: "float",                         double: "double",                 \
  long double: "long double",                   char *: "pointer to char",        \
       void *: "pointer to void",                int *: "pointer to int",         \
      default: "other")


/*	here's the deal ok? make an opcode for each and erry n-bytes!
 * 'q' = int64
 * 'l' - int32
 * 's' - int16
 * 'b' - byte | push and pop do not take bytes
 * 'f' - float32
 * 'df' - float64
*/

#define INSTR_SET	\
	X(halt) \
	X(pushl) X(pushs) X(pushb) X(pushsp) X(puship) \
	X(popl) X(pops) X(popb) \
	X(wrtl) X(wrts) X(wrtb) \
	X(storel) X(stores) X(storeb) \
	X(loadl) X(loads) X(loadb) \
	X(copyl) X(copys) X(copyb) \
	X(addl) X(uaddl) X(addf) X(subl) X(usubl) X(subf) \
	X(mull) X(umull) X(mulf) X(divl) X(udivl) X(divf) X(modl) X(umodl) \
	X(andl) X(orl) X(xorl) X(notl) X(shl) X(shr) X(incl) X(decl) \
	X(ltl) X(ultl) X(ltf) X(gtl) X(ugtl) X(gtf) X(cmpl) X(ucmpl) X(compf) \
	X(leql) X(uleql) X(leqf) X(geql) X(ugeql) X(geqf) \
	X(jmp) X(jzl) X(jzs) X(jzb) X(jnzl) X(jnzs) X(jnzb) \
	X(call) X(ret) X(reset) \
	X(nop) \

#define X(x) x,
enum InstrSet{ INSTR_SET };
#undef X

#define X(x) #x ,
const char *opcode2str[] = { INSTR_SET };
#undef X

#define WORD_SIZE		4
#define STK_SIZE		(1024*WORD_SIZE)	// 4096 4Kb
#define CALLSTK_SIZE	256				// 1024 bytes
#define MEM_SIZE		(256*WORD_SIZE)	// 1024 bytes

struct vm_cpu {
	unsigned char		bStack[STK_SIZE];			// 4096 bytes
	unsigned int	bCallstack[CALLSTK_SIZE];	// 1024 bytes
	unsigned char		bMemory[MEM_SIZE];			// 1024 bytes
	unsigned int	ip, sp, callsp; //callbp;		// 16 bytes
};

union conv_union {
	unsigned int ui;
	int	i;
	float f;
	unsigned short us;
	short	s;
	unsigned char c[4];
};
/*
vm_t*		vm_create(vm_t *restrict pVM, char *restrict rawcode);
void		vm_init(vm_t *pVM);
void		vm_free(vm_t *pVM, const bool mallocd);
void		vm_exec(vm_t *pVM);
float		vm_pop_float(vm_t *pVM);
unsigned int	vm_pop_uint(vm_t *pVM);
unsigned short	vm_pop_ushort(vm_t *pVM);
unsigned char		vm_pop_byte(vm_t *pVM);
void		vm_push_float(vm_t *pVM, float val);
void		vm_push_uint(vm_t *pVM, unsigned int val);
void		vm_push_ushort(vm_t *pVM, unsigned short val);
void		vm_push_byte(vm_t *pVM, unsigned char val);
*/

// Safe mode enables bounds checking.
// this might slow down the interpreter on a smaller micro level since we're always checking
// if pointers or memory addresses go out of bounds.
#define SAFEMODE	1

void			vm_reset(struct vm_cpu *vm);
void			vm_exec(const unsigned char *restrict code, struct vm_cpu *restrict vm);
void			vm_debug_print_ptrs(const struct vm_cpu *vm);
void			vm_debug_print_callstack(const struct vm_cpu *vm);
void			vm_debug_print_stack(const struct vm_cpu *vm);
void			vm_debug_print_memory(const struct vm_cpu *vm);

unsigned int	vm_get_imm4(struct vm_cpu *restrict vm, const unsigned char *restrict code);
unsigned short	vm_get_imm2(struct vm_cpu *restrict vm, const unsigned char *restrict code);
unsigned char	vm_get_imm1(struct vm_cpu *restrict vm, const unsigned char *restrict code);

unsigned int	vm_pop_word(struct vm_cpu *vm);
unsigned short	vm_pop_short(struct vm_cpu *vm);
unsigned char	vm_pop_byte(struct vm_cpu *vm);
float			vm_pop_float32(struct vm_cpu *vm);

void			vm_push_word(struct vm_cpu *restrict vm, const unsigned int val);
void			vm_push_short(struct vm_cpu *restrict vm, const unsigned short val);
void			vm_push_byte(struct vm_cpu *restrict vm, const unsigned char val);
void			vm_push_float(struct vm_cpu *restrict vm, const float val);

void			vm_write_word(struct vm_cpu *restrict vm, const unsigned int val, const unsigned int address);
void			vm_write_short(struct vm_cpu *restrict vm, const unsigned short val, const unsigned int address);
void			vm_write_byte(struct vm_cpu *restrict vm, const unsigned char val, const unsigned int address);
void			vm_write_float(struct vm_cpu *restrict vm, const float val, const unsigned int address);

unsigned int	vm_read_word(struct vm_cpu *restrict vm, const unsigned int address);
unsigned short	vm_read_short(struct vm_cpu *restrict vm, const unsigned int address);
unsigned char	vm_read_byte(struct vm_cpu *restrict vm, const unsigned int address);
float			vm_read_float(struct vm_cpu *restrict vm, const unsigned int address);

#ifdef __cplusplus
}
#endif

#endif	// VM_H_INCLUDED
