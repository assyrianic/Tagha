
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>


/*	here's the deal ok? make an opcode for each and erry n-bytes!
 * 'l' - int32
 * 's' - int16
 * 'b' - byte | push and pop do not take bytes
 * 'f' - float32
*/

#define INSTR_SET	\
	X(halt) \
	X(pushl) X(pushs) X(pushb) X(pushsp) X(puship)\
	X(popl) X(pops) X(popb) \
	X(wrtl) X(wrts) X(wrtb) \
	X(storel) X(stores) X(storeb) \
	X(loadl) X(loads) X(loadb) \
	X(copyl) X(copys) X(copyb) \
	X(addl) X(uaddl) X(addf) X(subl) X(usubl) X(subf) \
	X(mull) X(umull) X(mulf) X(divl) X(udivl) X(divf) X(modl) X(umodl)\
	X(andl) X(orl) X(xorl) X(notl) X(shl) X(shr) X(incl) X(decl) \
	X(ltl) X(ultl) X(ltf) X(gtl) X(ugtl) X(gtf) X(cmpl) X(ucmpl) X(compf) \
	X(leql) X(uleql) X(leqf) X(geql) X(ugeql) X(geqf) \
	X(jmp) X(jzl) X(jzs) X(jzb) X(jnzl) X(jnzs) X(jnzb) \
	X(call) X(ret) \
	X(nop) \

#define X(x) x,
enum InstrSet{ INSTR_SET };
#undef X

#define X(x) #x ,
const char *opcode2str[] = { INSTR_SET };
#undef X

#define WORD_SIZE		4
#define STK_SIZE		1024*WORD_SIZE	// 4096 4Kb
#define CALLSTK_SIZE	256				// 1024 bytes
#define MEM_SIZE		256*WORD_SIZE	// 1024 bytes

// 'b' for Byte, not bool
struct vm_cpu {
	uint8_t		bStack[STK_SIZE];			// 4096 bytes
	uint32_t	bCallstack[CALLSTK_SIZE];	// 1024 bytes
	uint8_t		bMemory[MEM_SIZE];			// 1024 bytes
	uint32_t	ip, sp, callsp, callbp;		// 16 bytes
};

void debug_print_memory(const struct vm_cpu *restrict vm)
{
	if( !vm )
		return;
	printf("DEBUG ...---===---... Printing Memory...\n");
	uint32_t i;
	for( i=0 ; i<MEM_SIZE ; i++ )
		if( vm->bMemory[i] )
			printf("Memory Index: 0x%x | data: %u\n", i, vm->bMemory[i]);
	printf("\n");
}
void debug_print_stack(const struct vm_cpu *restrict vm)
{
	if( !vm )
		return;
	printf("DEBUG ...---===---... Printing Stack...\n");
	uint32_t i;
	for( i=0 ; i<STK_SIZE ; i++ )
		if( vm->bStack[i] )
			printf("Stack Index: 0x%x | data: %u\n", i, vm->bStack[i]);
	printf("\n");
}
void debug_print_callstack(const struct vm_cpu *restrict vm)
{
	if( !vm )
		return;
	printf("DEBUG ...---===---... Printing Call Stack...\n");
	uint32_t i;
	for( i=0 ; i<CALLSTK_SIZE ; i++ )
		if( vm->bCallstack[i] )
			printf("Call Stack Index: 0x%x | data: %u\n", i, vm->bCallstack[i]);
	printf("\n");
}
void debug_print_ptrs(const struct vm_cpu *restrict vm)
{
	if( !vm )
		return;
	printf("DEBUG ...---===---... Printing Pointers...\n");
	printf("Instruction Pointer: %u\
			\nStack Pointer: %u\
			\nCall Stack Pointer: %u\
			\nCall Stack Frame Pointer: %u\n", vm->ip, vm->sp, vm->callsp, vm->callbp);
	printf("\n");
}

//#include <unistd.h>	// sleep() func
void vm_exec(const uint8_t *code, struct vm_cpu *const vm)
{
	union {
		uint32_t ui;
		int32_t	i;
		float f;
		uint16_t us;
		int16_t	s;
		uint8_t c[4];
	} conv;
	uint32_t b, a;
	float fa, fb;
	uint16_t usa, usb;

#define X(x) &&exec_##x ,
	static const void *dispatch[] = { INSTR_SET };
#undef INSTR_SET

	if( code[vm->ip] > nop) {
		printf("illegal instruction exception! instruction == \'%" PRIu8 "\' @ %u\n", code[vm->ip], vm->ip);
		goto *dispatch[halt];
		return;
	}
	//printf( "current instruction == \"%s\" @ ip == %u\n", opcode2str[code[vm->ip]], vm->ip );
#ifdef _UNISTD_H
	#define DISPATCH()	sleep(1); goto *dispatch[ code[++vm->ip] ]
#else
	#define DISPATCH()	goto *dispatch[ code[++vm->ip] ]
#endif
	goto *dispatch[ code[vm->ip] ];

exec_nop:;
	DISPATCH();
exec_halt:;
	printf("===================== vm done\n");
	return;

// opcodes for longs
exec_pushl:;	// push 4 bytes onto the stack
	conv.c[0] = code[++vm->ip];
	conv.c[1] = code[++vm->ip];
	conv.c[2] = code[++vm->ip];
	conv.c[3] = code[++vm->ip];
	vm->bStack[++vm->sp] = conv.c[0];
	vm->bStack[++vm->sp] = conv.c[1];
	vm->bStack[++vm->sp] = conv.c[2];
	vm->bStack[++vm->sp] = conv.c[3];
	printf("pushl: pushed %u\n", conv.ui);
	DISPATCH();
exec_pushs:;	// push 2 bytes onto the stack
	conv.c[0] = code[++vm->ip];
	conv.c[1] = code[++vm->ip];
	vm->bStack[++vm->sp] = conv.c[0];
	vm->bStack[++vm->sp] = conv.c[1];
	printf("pushs: pushed %u\n", conv.us);
	DISPATCH();
exec_pushb:;	// push a byte onto the stack
	vm->bStack[++vm->sp] = code[++vm->ip];
	printf("pushb: pushed %u\n", vm->bStack[vm->sp]);
	DISPATCH();
exec_pushsp:;	// push sp onto the stack, uses 4 bytes since 'sp' is uint32
	conv.ui = vm->sp;
	vm->bStack[++vm->sp] = conv.c[0];
	vm->bStack[++vm->sp] = conv.c[1];
	vm->bStack[++vm->sp] = conv.c[2];
	vm->bStack[++vm->sp] = conv.c[3];
	printf("pushsp: pushed sp index: %u\n", conv.ui);
	DISPATCH();
exec_puship:;
	conv.ui = vm->ip;
	vm->bStack[++vm->sp] = conv.c[0];
	vm->bStack[++vm->sp] = conv.c[1];
	vm->bStack[++vm->sp] = conv.c[2];
	vm->bStack[++vm->sp] = conv.c[3];
	printf("puship: pushed ip index: %u\n", conv.ui);
	DISPATCH();
exec_popl:;		// pop 4 bytes to eventually be overwritten
	vm->sp -= 4;
	printf("popl\n");
	DISPATCH();
exec_pops:;		// pop 2 bytes
	vm->sp -= 2;
	printf("pops\n");
	DISPATCH();
exec_popb:;		// pop a byte
	--vm->sp;
	printf("popb\n");
	DISPATCH();
exec_wrtl:;	// writes an int to memory, First operand is the memory address as 4 byte number, second is the int of data.
	conv.c[0] = code[++vm->ip];
	conv.c[1] = code[++vm->ip];
	conv.c[2] = code[++vm->ip];
	conv.c[3] = code[++vm->ip];
	a = conv.ui;
	vm->bMemory[a] = code[++vm->ip];
	vm->bMemory[a+1] = code[++vm->ip];
	vm->bMemory[a+2] = code[++vm->ip];
	vm->bMemory[a+3] = code[++vm->ip];
	conv.c[0] = vm->bMemory[a];
	conv.c[1] = vm->bMemory[a+1];
	conv.c[2] = vm->bMemory[a+2];
	conv.c[3] = vm->bMemory[a+3];
	printf("wrote int data - %u @ address 0x%x\n", conv.ui, a);
	DISPATCH();
exec_wrts:;	// writes a short to memory. First operand is the memory address as 4 byte number, second is the short of data.
	conv.c[0] = code[++vm->ip];
	conv.c[1] = code[++vm->ip];
	conv.c[2] = code[++vm->ip];
	conv.c[3] = code[++vm->ip];
	a = conv.ui;
	vm->bMemory[a] = code[++vm->ip];
	vm->bMemory[a+1] = code[++vm->ip];
	conv.c[0] = vm->bMemory[a];
	conv.c[1] = vm->bMemory[a+1];
	printf("wrote short data - %u @ address 0x%x\n", conv.us, a);
	DISPATCH();
exec_wrtb:;	// writes a byte to memory. First operand is the memory address as 32-bit number, second is the byte of data.
	conv.c[0] = code[++vm->ip];
	conv.c[1] = code[++vm->ip];
	conv.c[2] = code[++vm->ip];
	conv.c[3] = code[++vm->ip];
	vm->bMemory[conv.ui] = code[++vm->ip];
	printf("wrote byte data - %u @ address 0x%x\n", vm->bMemory[conv.ui], conv.ui);
	DISPATCH();
exec_storel:;	// pops 4-byte value off stack and into a memory address.
	conv.c[0] = code[++vm->ip];
	conv.c[1] = code[++vm->ip];
	conv.c[2] = code[++vm->ip];
	conv.c[3] = code[++vm->ip];
	a = conv.ui;
	vm->bMemory[a+3] = vm->bStack[vm->sp--];
	vm->bMemory[a+2] = vm->bStack[vm->sp--];
	vm->bMemory[a+1] = vm->bStack[vm->sp--];
	vm->bMemory[a] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bMemory[a];
	conv.c[1] = vm->bMemory[a+1];
	conv.c[2] = vm->bMemory[a+2];
	conv.c[3] = vm->bMemory[a+3];
	printf("stored int data - %u @ address 0x%x\n", conv.ui, a);
	DISPATCH();
exec_stores:;	// pops 2-byte value off stack and into a memory address.
	conv.c[0] = code[++vm->ip];
	conv.c[1] = code[++vm->ip];
	conv.c[2] = code[++vm->ip];
	conv.c[3] = code[++vm->ip];
	a = conv.ui;
	vm->bMemory[a+1] = vm->bStack[vm->sp--];
	vm->bMemory[a] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bMemory[a];
	conv.c[1] = vm->bMemory[a+1];
	printf("stored short data - %u @ address 0x%x\n", conv.us, a);
	DISPATCH();
exec_storeb:;	// pops byte value off stack and into a memory address.
	conv.c[0] = code[++vm->ip];
	conv.c[1] = code[++vm->ip];
	conv.c[2] = code[++vm->ip];
	conv.c[3] = code[++vm->ip];
	a = conv.ui;
	vm->bMemory[a] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bMemory[a];
	printf("stored byte data - %u @ address 0x%x\n", conv.c[0], a);
	DISPATCH();
exec_loadl:;
	conv.c[0] = code[++vm->ip];
	conv.c[1] = code[++vm->ip];
	conv.c[2] = code[++vm->ip];
	conv.c[3] = code[++vm->ip];
	a = conv.ui;
	conv.c[0] = vm->bStack[++vm->sp] = vm->bMemory[a];
	conv.c[1] = vm->bStack[++vm->sp] = vm->bMemory[a+1];
	conv.c[2] = vm->bStack[++vm->sp] = vm->bMemory[a+2];
	conv.c[3] = vm->bStack[++vm->sp] = vm->bMemory[a+3];
	printf("loaded int data to T.O.S. - %u from address 0x%x\n", conv.ui, a);
	DISPATCH();
exec_loads:;
	conv.c[0] = code[++vm->ip];
	conv.c[1] = code[++vm->ip];
	conv.c[2] = code[++vm->ip];
	conv.c[3] = code[++vm->ip];
	a = conv.ui;
	conv.c[1] = vm->bStack[++vm->sp] = vm->bMemory[a];
	conv.c[0] = vm->bStack[++vm->sp] = vm->bMemory[a+1];
	printf("loaded short data to T.O.S. - %u from address 0x%x\n", conv.us, a);
	DISPATCH();
exec_loadb:;
	conv.c[0] = code[++vm->ip];
	conv.c[1] = code[++vm->ip];
	conv.c[2] = code[++vm->ip];
	conv.c[3] = code[++vm->ip];
	a = conv.ui;
	vm->bStack[++vm->sp] = vm->bMemory[a];
	printf("loaded byte data to T.O.S. - %u from address 0x%x\n", vm->bStack[vm->sp], a);
	DISPATCH();
exec_copyl:;	// copy 4 bytes of top of stack and put as new top of stack.
	conv.c[0] = vm->bStack[vm->sp-3];
	conv.c[1] = vm->bStack[vm->sp-2];
	conv.c[2] = vm->bStack[vm->sp-1];
	conv.c[3] = vm->bStack[vm->sp];
	printf("copied int data from T.O.S. - %u\n", conv.ui);
	vm->bStack[++vm->sp] = conv.c[0];
	vm->bStack[++vm->sp] = conv.c[1];
	vm->bStack[++vm->sp] = conv.c[2];
	vm->bStack[++vm->sp] = conv.c[3];
	DISPATCH();
exec_copys:;
	conv.c[0] = vm->bStack[vm->sp-1];
	conv.c[1] = vm->bStack[vm->sp];
	vm->bStack[++vm->sp] = conv.c[0];
	vm->bStack[++vm->sp] = conv.c[1];
	printf("copied short data from T.O.S. - %u\n", conv.us);
	DISPATCH();
exec_copyb:;
	conv.c[0] = vm->bStack[vm->sp];
	vm->bStack[++vm->sp] = conv.c[0];
	printf("copied byte data from T.O.S. - %u\n", conv.c[0]);
	DISPATCH();
exec_addl:;		// pop 8 bytes, signed addition, and push 4 byte result to top of stack
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	b=conv.ui;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	a=conv.ui;
	conv.i = (int32_t)a + (int32_t)b;
	printf("signed 4 byte addition result: %i == %i + %i\n", conv.ui, a,b);
	vm->bStack[++vm->sp] = conv.c[0];
	vm->bStack[++vm->sp] = conv.c[1];
	vm->bStack[++vm->sp] = conv.c[2];
	vm->bStack[++vm->sp] = conv.c[3];
	DISPATCH();
exec_uaddl:;	// In C, all integers in an expression are promoted to int32, if number is bigger then unsigned int32 or int64
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	b=conv.ui;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	a=conv.ui;
	conv.ui = a+b;
	printf("unsigned 4 byte addition result: %u == %u + %u\n", conv.ui, a,b);
	vm->bStack[++vm->sp] = conv.c[0];
	vm->bStack[++vm->sp] = conv.c[1];
	vm->bStack[++vm->sp] = conv.c[2];
	vm->bStack[++vm->sp] = conv.c[3];
	DISPATCH();
exec_addf:;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	fb=conv.f;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	fa=conv.f;
	conv.f = fa+fb;
	printf("float addition result: %f == %f + %f\n", conv.f, fa,fb);
	vm->bStack[++vm->sp] = conv.c[0];
	vm->bStack[++vm->sp] = conv.c[1];
	vm->bStack[++vm->sp] = conv.c[2];
	vm->bStack[++vm->sp] = conv.c[3];
	DISPATCH();
exec_subl:;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	b=conv.ui;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	a=conv.ui;
	conv.i = (int32_t)a - (int32_t)b;
	printf("signed 4 byte subtraction result: %i == %i - %i\n", conv.ui, a,b);
	vm->bStack[++vm->sp] = conv.c[0];
	vm->bStack[++vm->sp] = conv.c[1];
	vm->bStack[++vm->sp] = conv.c[2];
	vm->bStack[++vm->sp] = conv.c[3];
	DISPATCH();
exec_usubl:;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	b=conv.ui;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	a=conv.ui;
	conv.ui = a-b;
	printf("unsigned 4 byte subtraction result: %u == %u - %u\n", conv.ui, a,b);
	vm->bStack[++vm->sp] = conv.c[0];
	vm->bStack[++vm->sp] = conv.c[1];
	vm->bStack[++vm->sp] = conv.c[2];
	vm->bStack[++vm->sp] = conv.c[3];
	DISPATCH();
exec_subf:;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	fb=conv.f;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	fa=conv.f;
	conv.f = fa-fb;
	printf("float subtraction result: %f == %f - %f\n", conv.f, fa,fb);
	vm->bStack[++vm->sp] = conv.c[0];
	vm->bStack[++vm->sp] = conv.c[1];
	vm->bStack[++vm->sp] = conv.c[2];
	vm->bStack[++vm->sp] = conv.c[3];
	DISPATCH();
exec_mull:;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	b=conv.ui;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	a=conv.ui;
	conv.i = (int32_t)a * (int32_t)b;
	printf("signed 4 byte mult result: %i == %i * %i\n", conv.i, a,b);
	vm->bStack[++vm->sp] = conv.c[0];
	vm->bStack[++vm->sp] = conv.c[1];
	vm->bStack[++vm->sp] = conv.c[2];
	vm->bStack[++vm->sp] = conv.c[3];
	DISPATCH();
exec_umull:;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	b=conv.ui;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	a=conv.ui;
	conv.ui = a*b;
	printf("unsigned 4 byte mult result: %u == %u * %u\n", conv.ui, a,b);
	vm->bStack[++vm->sp] = conv.c[0];
	vm->bStack[++vm->sp] = conv.c[1];
	vm->bStack[++vm->sp] = conv.c[2];
	vm->bStack[++vm->sp] = conv.c[3];
	DISPATCH();
exec_mulf:;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	fb=conv.f;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	fa=conv.f;
	conv.f = fa*fb;
	printf("float mul result: %f == %f * %f\n", conv.f, fa,fb);
	vm->bStack[++vm->sp] = conv.c[0];
	vm->bStack[++vm->sp] = conv.c[1];
	vm->bStack[++vm->sp] = conv.c[2];
	vm->bStack[++vm->sp] = conv.c[3];
	DISPATCH();
exec_divl:;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	b=conv.ui;
	if( !b ) {
		printf("divl: divide by 0 error.\n");
		goto *dispatch[halt];
	}
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	a=conv.ui;
	conv.i = (int32_t)a / (int32_t)b;
	printf("signed 4 byte division result: %i == %i / %i\n", conv.i, a,b);
	vm->bStack[++vm->sp] = conv.c[0];
	vm->bStack[++vm->sp] = conv.c[1];
	vm->bStack[++vm->sp] = conv.c[2];
	vm->bStack[++vm->sp] = conv.c[3];
	DISPATCH();
exec_udivl:;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	b=conv.ui;
	if( !b ) {
		printf("udivl: divide by 0 error.\n");
		goto *dispatch[halt];
	}
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	a=conv.ui;
	conv.ui = a/b;
	printf("unsigned 4 byte division result: %u == %u / %u\n", conv.ui, a,b);
	vm->bStack[++vm->sp] = conv.c[0];
	vm->bStack[++vm->sp] = conv.c[1];
	vm->bStack[++vm->sp] = conv.c[2];
	vm->bStack[++vm->sp] = conv.c[3];
	DISPATCH();
exec_divf:;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	fb=conv.f;
	if( !fb ) {
		printf("divf: divide by 0.0 error.\n");
		goto *dispatch[halt];
	}
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	fa=conv.f;
	conv.f = fa/fb;
	printf("float division result: %f == %f / %f\n", conv.f, fa,fb);
	vm->bStack[++vm->sp] = conv.c[0];
	vm->bStack[++vm->sp] = conv.c[1];
	vm->bStack[++vm->sp] = conv.c[2];
	vm->bStack[++vm->sp] = conv.c[3];
	DISPATCH();
exec_modl:;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	b=conv.ui;
	if( !b ) {
		printf("modl: divide by 0 error.\n");
		goto *dispatch[halt];
	}
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	a=conv.ui;
	conv.i = (int32_t)a % (int32_t)b;
	printf("signed 4 byte modulo result: %i == %i %% %i\n", conv.i, a,b);
	vm->bStack[++vm->sp] = conv.c[0];
	vm->bStack[++vm->sp] = conv.c[1];
	vm->bStack[++vm->sp] = conv.c[2];
	vm->bStack[++vm->sp] = conv.c[3];
	DISPATCH();
exec_umodl:;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	b=conv.ui;
	if( !b ) {
		printf("umodl: divide by 0 error.\n");
		goto *dispatch[halt];
	}
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	a=conv.ui;
	conv.ui = a%b;
	printf("unsigned 4 byte modulo result: %u == %u %% %u\n", conv.ui, a,b);
	vm->bStack[++vm->sp] = conv.c[0];
	vm->bStack[++vm->sp] = conv.c[1];
	vm->bStack[++vm->sp] = conv.c[2];
	vm->bStack[++vm->sp] = conv.c[3];
	DISPATCH();
exec_andl:;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	b=conv.ui;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	a=conv.ui;
	conv.ui = a & b;
	printf("4 byte AND result: %u == %i & %i\n", conv.ui, a,b);
	vm->bStack[++vm->sp] = conv.c[0];
	vm->bStack[++vm->sp] = conv.c[1];
	vm->bStack[++vm->sp] = conv.c[2];
	vm->bStack[++vm->sp] = conv.c[3];
	DISPATCH();
exec_orl:;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	b=conv.ui;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	a=conv.ui;
	conv.ui = a | b;
	printf("4 byte OR result: %u == %i | %i\n", conv.ui, a,b);
	vm->bStack[++vm->sp] = conv.c[0];
	vm->bStack[++vm->sp] = conv.c[1];
	vm->bStack[++vm->sp] = conv.c[2];
	vm->bStack[++vm->sp] = conv.c[3];
	DISPATCH();
exec_xorl:;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	b=conv.ui;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	a=conv.ui;
	conv.ui = a ^ b;
	printf("4 byte XOR result: %u == %i ^ %i\n", conv.ui, a,b);
	vm->bStack[++vm->sp] = conv.c[0];
	vm->bStack[++vm->sp] = conv.c[1];
	vm->bStack[++vm->sp] = conv.c[2];
	vm->bStack[++vm->sp] = conv.c[3];
	DISPATCH();
exec_notl:;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	a=conv.ui;
	conv.ui = ~a;
	printf("4 byte NOT result: %u\n", conv.ui);
	vm->bStack[++vm->sp] = conv.c[0];
	vm->bStack[++vm->sp] = conv.c[1];
	vm->bStack[++vm->sp] = conv.c[2];
	vm->bStack[++vm->sp] = conv.c[3];
	DISPATCH();
exec_shl:;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	b=conv.ui;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	a=conv.ui;
	conv.ui = a << b;
	printf("4 byte Shift Left result: %u == %i >> %i\n", conv.ui, a,b);
	vm->bStack[++vm->sp] = conv.c[0];
	vm->bStack[++vm->sp] = conv.c[1];
	vm->bStack[++vm->sp] = conv.c[2];
	vm->bStack[++vm->sp] = conv.c[3];
	DISPATCH();
exec_shr:;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	b=conv.ui;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	a=conv.ui;
	conv.ui = a >> b;
	printf("4 byte Shift Right result: %u == %i >> %i\n", conv.ui, a,b);
	vm->bStack[++vm->sp] = conv.c[0];
	vm->bStack[++vm->sp] = conv.c[1];
	vm->bStack[++vm->sp] = conv.c[2];
	vm->bStack[++vm->sp] = conv.c[3];
	DISPATCH();
exec_incl:;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	a=conv.ui;
	conv.ui = ++a;
	printf("4 byte Increment result: %u\n", conv.ui);
	vm->bStack[++vm->sp] = conv.c[0];
	vm->bStack[++vm->sp] = conv.c[1];
	vm->bStack[++vm->sp] = conv.c[2];
	vm->bStack[++vm->sp] = conv.c[3];
	DISPATCH();
exec_decl:;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	a=conv.ui;
	conv.ui = --a;
	printf("4 byte Decrement result: %u\n", conv.ui);
	vm->bStack[++vm->sp] = conv.c[0];
	vm->bStack[++vm->sp] = conv.c[1];
	vm->bStack[++vm->sp] = conv.c[2];
	vm->bStack[++vm->sp] = conv.c[3];
	DISPATCH();
exec_ltl:;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	b=conv.ui;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	a=conv.ui;
	conv.ui = (int32_t)a < (int32_t)b;
	printf("4 byte Signed Less Than result: %u == %i < %i\n", conv.ui, a,b);
	vm->bStack[++vm->sp] = conv.c[0];
	vm->bStack[++vm->sp] = conv.c[1];
	vm->bStack[++vm->sp] = conv.c[2];
	vm->bStack[++vm->sp] = conv.c[3];
	DISPATCH();
exec_ultl:;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	b=conv.ui;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	a=conv.ui;
	conv.ui = a < b;
	printf("4 byte Unsigned Less Than result: %u == %u < %u\n", conv.ui, a,b);
	vm->bStack[++vm->sp] = conv.c[0];
	vm->bStack[++vm->sp] = conv.c[1];
	vm->bStack[++vm->sp] = conv.c[2];
	vm->bStack[++vm->sp] = conv.c[3];
	DISPATCH();
exec_ltf:;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	fb=conv.f;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	fa=conv.f;
	conv.ui = fa < fb;
	printf("4 byte Less Than Float result: %u == %f < %f\n", conv.ui, fa,fb);
	vm->bStack[++vm->sp] = conv.c[0];
	vm->bStack[++vm->sp] = conv.c[1];
	vm->bStack[++vm->sp] = conv.c[2];
	vm->bStack[++vm->sp] = conv.c[3];
	DISPATCH();
exec_gtl:;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	b=conv.ui;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	a=conv.ui;
	conv.ui = (int32_t)a > (int32_t)b;
	printf("4 byte Signed Greater Than result: %u == %i > %i\n", conv.ui, a,b);
	vm->bStack[++vm->sp] = conv.c[0];
	vm->bStack[++vm->sp] = conv.c[1];
	vm->bStack[++vm->sp] = conv.c[2];
	vm->bStack[++vm->sp] = conv.c[3];
	DISPATCH();
exec_ugtl:;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	b=conv.ui;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	a=conv.ui;
	conv.ui = a > b;
	printf("4 byte Signed Greater Than result: %u == %u > %u\n", conv.ui, a,b);
	vm->bStack[++vm->sp] = conv.c[0];
	vm->bStack[++vm->sp] = conv.c[1];
	vm->bStack[++vm->sp] = conv.c[2];
	vm->bStack[++vm->sp] = conv.c[3];
	DISPATCH();
exec_gtf:;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	fb=conv.f;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	fa=conv.f;
	conv.ui = fa > fb;
	printf("4 byte Greater Than Float result: %u == %f > %f\n", conv.ui, fa,fb);
	vm->bStack[++vm->sp] = conv.c[0];
	vm->bStack[++vm->sp] = conv.c[1];
	vm->bStack[++vm->sp] = conv.c[2];
	vm->bStack[++vm->sp] = conv.c[3];
	DISPATCH();
exec_cmpl:;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	b=conv.ui;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	a=conv.ui;
	conv.ui = (int32_t)a == (int32_t)b;
	printf("4 byte Signed Compare result: %u == %i == %i\n", conv.ui, a,b);
	vm->bStack[++vm->sp] = conv.c[0];
	vm->bStack[++vm->sp] = conv.c[1];
	vm->bStack[++vm->sp] = conv.c[2];
	vm->bStack[++vm->sp] = conv.c[3];
	DISPATCH();
exec_ucmpl:;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	b=conv.ui;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	a=conv.ui;
	conv.ui = a == b;
	printf("4 byte Unsigned Compare result: %u == %u == %u\n", conv.ui, a,b);
	vm->bStack[++vm->sp] = conv.c[0];
	vm->bStack[++vm->sp] = conv.c[1];
	vm->bStack[++vm->sp] = conv.c[2];
	vm->bStack[++vm->sp] = conv.c[3];
	DISPATCH();
exec_compf:;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	fb=conv.f;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	fa=conv.f;
	conv.ui = fa == fb;
	printf("4 byte Compare Float result: %u == %f == %f\n", conv.ui, fa,fb);
	vm->bStack[++vm->sp] = conv.c[0];
	vm->bStack[++vm->sp] = conv.c[1];
	vm->bStack[++vm->sp] = conv.c[2];
	vm->bStack[++vm->sp] = conv.c[3];
	DISPATCH();
exec_leql:;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	b=conv.ui;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	a=conv.ui;
	conv.ui = (int32_t)a <= (int32_t)b;
	printf("4 byte Signed Less Equal result: %u == %i <= %i\n", conv.ui, a,b);
	vm->bStack[++vm->sp] = conv.c[0];
	vm->bStack[++vm->sp] = conv.c[1];
	vm->bStack[++vm->sp] = conv.c[2];
	vm->bStack[++vm->sp] = conv.c[3];
	DISPATCH();
exec_uleql:;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	b=conv.ui;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	a=conv.ui;
	conv.ui = a <= b;
	printf("4 byte Unsigned Less Equal result: %u == %u <= %u\n", conv.ui, a,b);
	vm->bStack[++vm->sp] = conv.c[0];
	vm->bStack[++vm->sp] = conv.c[1];
	vm->bStack[++vm->sp] = conv.c[2];
	vm->bStack[++vm->sp] = conv.c[3];
	DISPATCH();
exec_leqf:;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	fb=conv.f;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	fa=conv.f;
	conv.ui = fa <= fb;
	printf("4 byte Less Equal Float result: %u == %f <= %f\n", conv.ui, fa, fb);
	vm->bStack[++vm->sp] = conv.c[0];
	vm->bStack[++vm->sp] = conv.c[1];
	vm->bStack[++vm->sp] = conv.c[2];
	vm->bStack[++vm->sp] = conv.c[3];
	DISPATCH();
exec_geql:;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	b=conv.ui;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	a=conv.ui;
	conv.ui = (int32_t)a >= (int32_t)b;
	printf("4 byte Signed Greater Equal result: %u == %i >= %i\n", conv.ui, a,b);
	vm->bStack[++vm->sp] = conv.c[0];
	vm->bStack[++vm->sp] = conv.c[1];
	vm->bStack[++vm->sp] = conv.c[2];
	vm->bStack[++vm->sp] = conv.c[3];
	DISPATCH();
exec_ugeql:;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	b=conv.ui;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	a=conv.ui;
	conv.ui = a >= b;
	printf("4 byte Unsigned Greater Equal result: %u == %u >= %u\n", conv.ui, a,b);
	vm->bStack[++vm->sp] = conv.c[0];
	vm->bStack[++vm->sp] = conv.c[1];
	vm->bStack[++vm->sp] = conv.c[2];
	vm->bStack[++vm->sp] = conv.c[3];
	DISPATCH();
exec_geqf:;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	fb=conv.f;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	fa=conv.f;
	conv.ui = fa >= fb;
	printf("4 byte Greater Equal Float result: %u == %f >= %f\n", conv.ui, fa, fb);
	vm->bStack[++vm->sp] = conv.c[0];
	vm->bStack[++vm->sp] = conv.c[1];
	vm->bStack[++vm->sp] = conv.c[2];
	vm->bStack[++vm->sp] = conv.c[3];
	DISPATCH();
exec_jmp:;	// addresses are 4 bytes.
	conv.c[0] = code[++vm->ip];
	conv.c[1] = code[++vm->ip];
	conv.c[2] = code[++vm->ip];
	conv.c[3] = code[++vm->ip];
	vm->ip = conv.ui;
	printf("jmping to instruction address: %u\n", vm->ip);
	goto *dispatch[ code[vm->ip] ];
exec_jzl:;	// check if the first 4 bytes on stack are zero, if yes then jump it.
	conv.c[3] = vm->bStack[vm->sp];
	conv.c[2] = vm->bStack[vm->sp-1];
	conv.c[1] = vm->bStack[vm->sp-2];
	conv.c[0] = vm->bStack[vm->sp-3];
	a = conv.ui;
	conv.c[0] = code[++vm->ip];
	conv.c[1] = code[++vm->ip];
	conv.c[2] = code[++vm->ip];
	conv.c[3] = code[++vm->ip];
	vm->ip = (!a) ? conv.ui : vm->ip+1 ;
	printf("jzl'ing to instruction address: %u\n", vm->ip);
	goto *dispatch[ code[vm->ip] ];
exec_jzs:;
	conv.c[1] = vm->bStack[vm->sp];
	conv.c[0] = vm->bStack[vm->sp-1];
	a = conv.ui;
	conv.c[0] = code[++vm->ip];
	conv.c[1] = code[++vm->ip];
	conv.c[2] = code[++vm->ip];
	conv.c[3] = code[++vm->ip];
	vm->ip = (!a) ? conv.ui : vm->ip+1 ;
	printf("jzs'ing to instruction address: %u\n", vm->ip);
	goto *dispatch[ code[vm->ip] ];
exec_jzb:;
	conv.c[0] = vm->bStack[vm->sp];
	a = conv.ui;
	conv.c[0] = code[++vm->ip];
	conv.c[1] = code[++vm->ip];
	conv.c[2] = code[++vm->ip];
	conv.c[3] = code[++vm->ip];
	vm->ip = (!a) ? conv.ui : vm->ip+1 ;
	printf("jzb'ing to instruction address: %u\n", vm->ip);
	goto *dispatch[ code[vm->ip] ];
exec_jnzl:;
	conv.c[3] = vm->bStack[vm->sp];
	conv.c[2] = vm->bStack[vm->sp-1];
	conv.c[1] = vm->bStack[vm->sp-2];
	conv.c[0] = vm->bStack[vm->sp-3];
	a = conv.ui;
	conv.c[0] = code[++vm->ip];
	conv.c[1] = code[++vm->ip];
	conv.c[2] = code[++vm->ip];
	conv.c[3] = code[++vm->ip];
	vm->ip = (a) ? conv.ui : vm->ip+1 ;
	printf("jnzl'ing to instruction address: %u\n", vm->ip);
	goto *dispatch[ code[vm->ip] ];
exec_jnzs:;
	conv.c[1] = vm->bStack[vm->sp];
	conv.c[0] = vm->bStack[vm->sp-1];
	a = conv.ui;
	conv.c[0] = code[++vm->ip];
	conv.c[1] = code[++vm->ip];
	conv.c[2] = code[++vm->ip];
	conv.c[3] = code[++vm->ip];
	vm->ip = (a) ? conv.ui : vm->ip+1 ;
	printf("jnzs'ing to instruction address: %u\n", vm->ip);
	goto *dispatch[ code[vm->ip] ];
exec_jnzb:;
	conv.c[0] = vm->bStack[vm->sp];
	a = conv.ui;
	conv.c[0] = code[++vm->ip];
	conv.c[1] = code[++vm->ip];
	conv.c[2] = code[++vm->ip];
	conv.c[3] = code[++vm->ip];
	vm->ip = (a) ? conv.ui : vm->ip+1 ;
	printf("jnzb'ing to instruction address: %u\n", vm->ip);
	goto *dispatch[ code[vm->ip] ];
exec_call:;
	vm->ip++;
	printf("calling address: %u\n", vm->ip);
	vm->bCallstack[++vm->callsp] = vm->ip + 1;
	vm->callbp = vm->callsp;
	vm->ip = code[vm->ip];
	printf("call return addr: %u | frame ptr == %u\n", vm->bCallstack[vm->callsp], vm->callbp);
	goto *dispatch[ code[vm->ip] ];
exec_ret:;
	vm->callsp = vm->callbp;
	printf("callsp set to callbp, callsp == %u\n", vm->callsp);
	vm->ip = vm->bCallstack[vm->callsp--];
	vm->callbp = vm->callsp;
	printf("returning to address: %u\n", vm->ip);
	goto *dispatch[ code[vm->ip] ];
}

uint64_t get_file_size(FILE *pFile)
{
	uint64_t size = 0;
	if( !pFile )
		return size;
	
	if( !fseek(pFile, 0, SEEK_END) ) {
		size = (uint64_t)ftell(pFile);
		rewind(pFile);
	}
	return size;
}

int main(void)
{
	typedef uint8_t		bytecode[] ;
	
	/*
	FILE *pFile = fopen("./myfile.cbyte", "rb");
	if( !pFile )
		return 0;
	
	uint64_t size = get_file_size(pFile);
	uint8_t *program = malloc(sizeof(uint8_t)*size);
	fread(program, sizeof(uint8_t), size, pFile);
	*/
	const bytecode test1 = {
		nop,
		//pushl, 255, 1, 0, 0x0,
		//pushs, 0xDD, 0xDD,
		pushb, 0xAA,
		//pushs, 0x0D, 0x0C,
		//pops, popl,
		//wrtl, 4,0,0,0, 0xFF,0xFF,0,0,
		//storeb, 0,0,0,0,
		//stores, 1,0,0,0,
		//storel, 4,0,0,0,
		//loadb, 4,0,0,0,
		//storel, 0,0,0,0,
		//loadl, 0,0,0,0,
		//pushsp, puship, copyl, copys, copyb,
		halt
	};
	const bytecode test2 = {
		nop,
		//jmp, 17,0,0,0,
		// -16776961
		//pushl, 255,0,0,255,
		//pushl, 1,0,0,0,
		// -855637761
		//pushl, 255,0,0,205,
		//pushl, 255,255,0,0,
		//uaddl,
		// 10.f in 4 bytes form
		pushl, 0,0,32,65,
		jzb, 17,0,0,0,
		// 2.f in 4 bytes form
		pushl, 0,0,0,64,
		addf,	// 12.f
		//leqf,
		halt
	};
	const bytecode test3 = {
		nop,
		call, 7,0,0,0,	// 1
		halt,			// 6
	// func1:
		pushl, 9,0,0,0,	// 7
		call, 18,0,0,0,	// 12
		ret,	// 17
	// func2:
		pushl, 5,0,0,0,	// 18
		pushl, 10,0,0,0,	// 23
		mull,	// 28
		mull,
		call, 36,0,0,0,	// 30
		ret,	
	// func3:
		pushl, 40,0,0,0,	// 36
		divl,	// 41
		ret,
	};
	struct vm_cpu *p_vm = malloc( sizeof(struct vm_cpu) ); //&(struct vm_cpu){ 0 };
	if( p_vm ) { //printf("size == %u\n", sizeof(struct vm_cpu));
		vm_exec( test3, p_vm );
		debug_print_memory(p_vm);
		debug_print_stack(p_vm);
		//debug_print_callstack(p_vm);
		//debug_print_ptrs(p_vm);
		free(p_vm);
	}
	p_vm=NULL;
	/*
	fclose(pFile); pFile=NULL;
	free(program); program=NULL;
	*/
	return 0;
}
