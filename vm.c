
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>

bool running = true;

// index pointers should NEVER go under 0...
uint8_t ip=0, sp=0, callbp=0, callsp=0;

// we need this register stuff to save necessary values!
// technically this is still a stack machine since almost everything we do fiddles with the stack.
// saving and loading from registers still requires the stack.
enum {
	r1=0,
	r2, r3
};
uint64_t reg[3];

typedef enum {
	// push and pop are always assumed to hold a long int
	nop=0,
	push, pop,						// 1
	add, fadd, sub, fsub,			// 3
	mul, fmul, idiv, fdiv, mod,		// 7
	jmp, lt, gt, cmp /* cmp does == */, 
	jnz, jz,
	inc, dec, shl, shr, //and, or, xor,
	// cpy copies the top of the stack and pushes that to the top.
	cpy, swap,
	load,	// put register value to top of stack.
	store,	// stores top of stack to register.
	halt,
} InstrSet;

#define STACKSIZE	256
uint64_t	stack[STACKSIZE];

void exec(const uint64_t *code)
{
	uint64_t b, a;
	double da, db;
	
	static const void *dispatch[] = {
		&&exec_nop,
		&&exec_push, &&exec_pop,
		&&exec_add, &&exec_fadd, &&exec_sub, &&exec_fsub,
		&&exec_mul, &&exec_fmul, &&exec_idiv, &&exec_fdiv, &&exec_mod,
		&&exec_jmp, &&exec_lt, &&exec_gt, &&exec_cmp,
		&&exec_jnz, &&exec_jz,
		&&exec_inc, &&exec_dec, &&exec_shl, &&exec_shr, //&&exec_and, &&exec_or, &&exec_xor,
		&&exec_cpy, &&exec_swap, &&exec_load, &&exec_store,
		//&&exec_z,
		&&exec_halt
	};
	//printf("current instruction == \'%u\'\n", instr);
	if( code[ip] > halt || code[ip] < nop ) {
		printf("handled instruction exception. instruction == \'%" PRIu64 "\'\n", code[ip]);
		goto *dispatch[halt];
		return;
	}
	goto *dispatch[ code[ip] ];

exec_nop:;
	ip++;
	return;
exec_halt:;
	running = false;
	printf("vm done\n");
	return;
exec_cpy:;	// makes a copy of the current value at the top of the stack and places the copy at the top.
	a=stack[sp];
	stack[++sp] = a;
	printf("copied %" PRIu64 ", top of stack: %" PRIu64 "\n", stack[sp-1], stack[sp]);
	ip++;
	return;
exec_swap:;	// swaps two, topmost stack values.
	a = stack[sp--];
	b = stack[sp--];
	stack[sp++] = b;
	stack[sp++] = a;
	printf("swapped: a == %" PRIu64 " | b == %" PRIu64 "\n", stack[sp-2], stack[sp-1]);
	ip++;
	return;
exec_load:;	// stores a register value into the top of the stack.
	a = code[++ip];
	stack[sp] = reg[a];
	printf("loaded %" PRIu64 " from reg[%" PRIu64 "]\n", stack[sp], a);
	return;
exec_store:;	// pops value off the stack into a register.
	a = code[++ip];
	reg[a] = stack[sp--];
	printf("stored %" PRIu64 " to reg[%" PRIu64 "] | reg[%" PRIu64 "] = %" PRIu64 "\n", reg[a], a, a, stack[sp+1]);
	return;

// various jumps
exec_jmp:;	// unconditional jump
	ip = code[ip+1];
	printf("jumping to... %u\n", ip);
	return;
exec_jnz:;	// Jump if Not Zero = JNZ
	if( stack[sp] ) {
		ip=code[ip+1];
		printf("jnz'ing to... %u\n", ip);
	}
	return;
exec_jz:;	// Jump if Zero = JZ
	++ip;
	if( !stack[sp] ) {
		ip=code[ip];
		printf("jz'ing to... %u\n", ip);
	}
	return;

// conditional stuff. Conditionals are always done signed I believe.
exec_lt:;
	b = stack[sp--];
	a = stack[sp--];
	stack[++sp] = (int64_t)a < (int64_t)b;
	printf("less than result %" PRIu64 " < %" PRIu64 " == %" PRIu64 "\n", a, b, stack[sp]);
	ip++;
	return;
exec_gt:;
	b = stack[sp--];
	a = stack[sp--];
	stack[++sp] = (int64_t)a > (int64_t)b;
	printf("greater than result %" PRIu64 " > %" PRIu64 " == %" PRIu64 "\n", a, b, stack[sp]);
	ip++;
	return;
exec_cmp:;
	b = stack[sp--];
	a = stack[sp--];
	stack[++sp] = (int64_t)a == (int64_t)b;
	printf("compare result %" PRIu64 " == %" PRIu64 " %" PRIu64 "\n", a, b, stack[sp]);
	ip++;
	return;
	
// push and pop
exec_push:;	// put an item on the top of the stack
	sp++;
	if( !sp ) {	// if we increment sp and sp is 0, we ran out of stack memory.
		printf("stack overflow!\n");
		goto *dispatch[halt];
	}
	stack[sp] = code[++ip];
	printf("pushing %" PRIu64 "\n", stack[sp]);
	ip++;
	return;
exec_pop:;	// reduce stack
	if( sp )
		--sp;
	if( sp==255 ) {		// if we decrement sp and sp's bits went all 1, we popped too much!
		printf("stack underflow!\n");
		goto *dispatch[halt];
	}
	printf("popped, stack pointer 0x%x\n", sp);
	ip++;
	return;

// arithmetic maths. order: int math, float math is last.
exec_add:;
	b = stack[sp--];
	a = stack[sp--];
	// we then add the result and push it to the stack
	stack[++sp] = a+b;	// set the value to the top of the stack
	printf("add result %" PRIu64 "\n", stack[sp]);
	ip++;
	return;
exec_sub:;
	b = stack[sp--];
	a = stack[sp--];
	stack[++sp] = b-a;
	// 0x8... is uint64_t's sign bit
	if( stack[sp] & 0x8000000000000000 )
		printf( "sub result %lli\n", (int64_t)stack[sp] );
	else printf( "sub result %" PRIu64 "\n", stack[sp] );
	ip++;
	return;
exec_mul:;
	b = stack[sp--];
	a = stack[sp--];
	stack[++sp] = a*b;
	printf("mul result %" PRIu64 "\n", stack[sp]);
	ip++;
	return;
exec_idiv:;
	b = stack[sp--];
	a = stack[sp--];
	if( a==0 ) {
		printf("div by 0 not allowed, restoring stack\n");
		sp += 2;
		return;
	}
	stack[++sp] = b/a;
	printf("div result %" PRIu64 "\n", stack[sp]);
	ip++;
	return;
exec_mod:;
	b = stack[sp--];
	a = stack[sp--];
	stack[++sp] = b%a;
	printf("mod result %" PRIu64 "\n", stack[sp]);
	ip++;
	return;
exec_inc:;
	stack[sp]++;
	printf("increment result %" PRIu64 "\n", stack[sp]);
	ip++;
	return;
exec_dec:;
	stack[sp]--;
	printf("decrement result %" PRIu64 "\n", stack[sp]);
	ip++;
	return;
exec_shl:;
	b = stack[sp--];
	a = stack[sp--];
	stack[++sp] = b<<a;
	printf( "bit shift left result %" PRIu64 "\n", stack[sp] );
	ip++;
	return;
exec_shr:;
	b = stack[sp--];
	a = stack[sp--];
	stack[++sp] = b>>a;
	printf( "bit shift right result %" PRIu64 "\n", stack[sp] );
	ip++;
	return;

// floating point maths
exec_fadd:;
	db = *(double *)(&stack[sp--]);
	da = *(double *)(&stack[sp--]);
	printf("da %f | db %f\n", da, db);
	db += da;
	stack[++sp] = *(uint64_t *)(&db);
	printf("f add result %f\n", db);
	ip++;
	return;
exec_fsub:;
	db = *(double *)(&stack[sp--]);
	da = *(double *)(&stack[sp--]);
	//printf("da %f | db %f\n", da, db);
	db -= da;
	stack[++sp] = *(uint64_t *)(&db);
	printf("f sub result %f\n", db);
	ip++;
	return;
exec_fmul:;
	db = *(double *)(&stack[sp--]);
	da = *(double *)(&stack[sp--]);
	//printf("da %f | db %f\n", da, db);
	db *= da;
	stack[++sp] = *(uint64_t *)(&db);
	printf("f mul result %f\n", db);
	ip++;
	return;
exec_fdiv:;
	db = *(double *)(&stack[sp--]);
	da = *(double *)(&stack[sp--]);
	printf("da %f | db %f\n", da, db);
	if( !db ) {
		printf("fdiv by 0 not allowed, restoring stack\n");
		sp += 2;
		return;
	}
	db /= da;
	stack[++sp] = *(uint64_t *)(&db);
	printf("f div result %f\n", db);
	ip++;
	return;
}

uint64_t get_file_size(FILE *pFile)
{
	if( !pFile )
		return 0;
	
	fseek(pFile, 0, SEEK_END);
	uint64_t size = ftell(pFile);
	rewind(pFile);
	return size;
}

int main(void)
{
	uint64_t program[] = {
		// to deal with floats, we first convert them to an unsigned longs bit value
		push, 0x4014000000000000,
		push, 0x4014000000000000,
		fadd,
		jz, 2,
		halt
	};
	/*
	FILE *pFile = fopen("./myfile.casm", "rb");
	if( !pFile )
		return 0;
	
	uint64_t size = get_file_size(pFile);
	
	uint64_t *program = malloc(sizeof(uint64_t)*size);
	fread(program, sizeof(uint64_t), size, pFile);
	*/
	while( running )
		exec( program );
	/*
	fclose(pFile); pFile=NULL;
	free(program); program=NULL;
	*/
	return 0;
}

void printBits(const size_t size_bytes, void const * const ptr)
{
	uint8_t *b = (unsigned char *)ptr;
	uint8_t byte;
	uint32_t i, j;

	for( i=size_bytes-1 ; i>=0 ; i-- ) {
		for( j=7 ; j>=0 ; j-- ) {
			byte = ( b[i] >> j ) & 1;
			printf("%u", byte);
		}
	}
	puts("");
}
