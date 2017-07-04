
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
	X(pushl) X(pushs) X(pushb) \
	X(popl) X(pops) X(popb) \
	X(wrtl) X(wrts) X(wrtb) \
	X(storel) \
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

//#include <unistd.h>	// sleep() func
void vm_exec(const uint8_t *code, struct vm_cpu *const vm)
{
	union {
		uint32_t ui;
		float f;
		uint16_t us;
		uint8_t c[4];
	} conv;
	uint32_t b, a;
	float fa, fb;

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
exec_storel:;	// pops value off stack and into a memory address.
	conv.c[0] = code[++vm->ip];
	conv.c[1] = code[++vm->ip];
	conv.c[2] = code[++vm->ip];
	conv.c[3] = code[++vm->ip];
	a = conv.ui;
	conv.c[3] = vm->bStack[vm->sp--];
	conv.c[2] = vm->bStack[vm->sp--];
	conv.c[1] = vm->bStack[vm->sp--];
	conv.c[0] = vm->bStack[vm->sp--];
	vm->bMemory[a] = conv.c[0];
	vm->bMemory[a+1] = conv.c[1];
	vm->bMemory[a+2] = conv.c[2];
	vm->bMemory[a+3] = conv.c[3];
	printf("stored int data - %u @ address 0x%x\n", conv.ui, a);
	DISPATCH();
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
		// push 5 as int32
		pushl, 0x05, 0, 0, 0,
		//pushs, 0x0D, 0x0C,
		//pops, popl,
		//wrtb, 0, 0, 0, 0, 0xFF,
		//wrts, 1, 0, 0, 0, 0xFF, 0xAD,
		// read
		//wrtl, 4, 0, 0, 0, 0xFF, 0xFF, 0, 0x0,
		storel, 0,0,0,0,
		halt
	};
	struct vm_cpu *p_vm = &(struct vm_cpu){ 0 };
	printf("size == %u\n", sizeof(struct vm_cpu));
	vm_exec( test1, p_vm );
	/*
	fclose(pFile); pFile=NULL;
	free(program); program=NULL;
	*/
	return 0;
}
