
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
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

void exec(uint64_t *code)
{
	union {
		uint64_t ll;
		double d;
		char c[8];
	} converter;
	
	uint64_t b, a;
	double da, db;
	
	static const void *dispatch[] = {
		&&exec_nop,
		&&exec_push, &&exec_pop,
		&&exec_add, &&exec_fadd, &&exec_sub, &&exec_fsub,
		&&exec_mul, &&exec_fmul, &&exec_idiv, &&exec_fdiv, &&exec_mod,
		&&exec_jmp, &&exec_lessthan, &&exec_grtrthan, &&exec_cmp,
		&&exec_jnz, &&exec_jz,
		&&exec_inc, &&exec_dec, &&exec_shl, &&exec_shr, //&&exec_and, &&exec_or, &&exec_xor,
		&&exec_cpy, &&exec_swap, &&exec_load, &&exec_store,
		//&&exec_z,
		&&exec_halt
	};
	//printf("current instruction == \'%u\'\n", instr);
	if( code[ip] > halt || code[ip] < nop ) {
		printf("handled instruction exception. instruction == \'%llu\'\n", code[ip]);
		goto *dispatch[halt];
		return;
	}
	#define DISPATCH()	goto *dispatch[ code[ip] ]
	DISPATCH();

exec_nop:; return;
exec_halt:;
	running = false;
	printf("vm done\n");
	return;
exec_cpy:;	// makes a copy of the current value at the top of the stack and places the copy at the top.
	a=stack[sp];
	stack[++sp] = a;
	printf("copied %llu, top of stack: %llu\n", stack[sp-1], stack[sp]);
	return;
exec_swap:;	// swaps two, topmost stack values.
	a = stack[sp--];
	b = stack[sp--];
	stack[sp++] = b;
	stack[sp++] = a;
	printf("swapped: a == %llu | b == %llu\n", stack[sp-2], stack[sp-1]);
	return;
exec_load:;	// stores a register value into the top of the stack.
	a = code[++ip];
	stack[sp] = reg[a];
	printf("loaded %llu from reg[%llu]\n", stack[sp], a);
	return;
exec_store:;	// pops value off the stack into a register.
	a = code[++ip];
	reg[a] = stack[sp--];
	printf("stored %llu to reg[%llu] | reg[%llu] = %llu\n", reg[a], a, a, stack[sp+1]);
	return;

// various jumps
exec_jmp:;	// unconditional jump
	ip = code[++ip];
	printf("jumping to... %u\n", ip);
	DISPATCH();
exec_jnz:;	// Jump if Not Zero = JNZ
	++ip;
	if( stack[sp] ) {
		ip=code[ip];
		printf("jnz'ing to... %u\n", ip);
		DISPATCH();
	}
	return;
exec_jz:;	// Jump if Zero = JZ
	++ip;
	if( !stack[sp] ) {
		ip=code[ip];
		printf("jz'ing to... %u\n", ip);
		DISPATCH();
	}
	return;

// conditional stuff. Conditionals are always done signed I believe.
exec_lessthan:;
	b = stack[sp--];
	a = stack[sp--];
	stack[++sp] = (int64_t)a < (int64_t)b;
	printf("less than result %llu < %llu == %llu\n", a, b, stack[sp]);
	return;
exec_grtrthan:;
	b = stack[sp--];
	a = stack[sp--];
	stack[++sp] = (int64_t)a > (int64_t)b;
	printf("greater than result %llu > %llu == %llu\n", a, b, stack[sp]);
	return;
exec_cmp:;
	b = stack[sp--];
	a = stack[sp--];
	stack[++sp] = (int64_t)a == (int64_t)b;
	printf("compare result %llu == %llu %llu\n", a, b, stack[sp]);
	return;
	
// push and pop
exec_push:;	// put an item on the top of the stack
	sp++;
	if( !sp ) {	// if we increment sp and sp is 0, we ran out of stack memory.
		printf("stack overflow!\n");
		goto *dispatch[halt];
	}
	stack[sp] = code[++ip];
	printf("pushing %llu\n", stack[sp]);
	return;
exec_pop:;	// reduce stack
	if( sp )
		--sp;
	if( sp==255 ) {		// if we decrement sp and sp's bits went all 1, we popped too much!
		printf("stack underflow!\n");
		goto *dispatch[halt];
	}
	printf("popped, stack pointer 0x%x\n", sp);
	return;

// arithmetic maths. order: int math, float math is last.
exec_add:;
	b = stack[sp--];
	a = stack[sp--];
	// we then add the result and push it to the stack
	stack[++sp] = a+b;	// set the value to the top of the stack
	printf("add result %llu\n", stack[sp]);
	return;
exec_sub:;
	b = stack[sp--];
	a = stack[sp--];
	stack[++sp] = b-a;
	// 0x8... is uint64_t's sign bit
	if( stack[sp] & 0x8000000000000000 )
		printf( "sub result %lli\n", (int64_t)stack[sp] );
	else printf( "sub result %llu\n", stack[sp] );
	return;
exec_mul:;
	b = stack[sp--];
	a = stack[sp--];
	stack[++sp] = a*b;
	printf("mul result %llu\n", stack[sp]);
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
	printf("div result %llu\n", stack[sp]);
	return;
exec_mod:;
	b = stack[sp--];
	a = stack[sp--];
	stack[++sp] = b%a;
	printf("mod result %llu\n", stack[sp]);
	return;
exec_inc:;
	stack[sp]++;
	printf("increment result %llu\n", stack[sp]);
	return;
exec_dec:;
	stack[sp]--;
	printf("decrement result %llu\n", stack[sp]);
	return;
exec_shl:;
	b = stack[sp--];
	a = stack[sp--];
	stack[++sp] = b<<a;
	printf( "bit shift left result %llu\n", stack[sp] );
	return;
exec_shr:;
	b = stack[sp--];
	a = stack[sp--];
	stack[++sp] = b>>a;
	printf( "bit shift right result %llu\n", stack[sp] );
	return;

// floating point maths
exec_fadd:;
	// gotta convert long int bits into float/double bits
	converter.ll = stack[sp--];
	db = converter.d;
	converter.ll = stack[sp--];
	da = converter.d;
	//printf("da %f | db %f\n", da, db);
	
	converter.d = da+db;
	stack[++sp] = converter.ll;
	printf("f add result %f\n", converter.d);
	return;
exec_fsub:;
	converter.ll = stack[sp--];
	db = converter.d;
	converter.ll = stack[sp--];
	da = converter.d;
	//printf("da %f | db %f\n", da, db);
	
	converter.d = db-da;
	stack[++sp] = converter.ll;
	printf("f sub result %f\n", converter.d);
	return;
exec_fmul:;
	converter.ll = stack[sp--];
	db = converter.d;
	converter.ll = stack[sp--];
	da = converter.d;
	//printf("da %f | db %f\n", da, db);
	
	converter.d = da*db;
	stack[++sp] = converter.ll;
	printf("f mul result %f\n", converter.d);
	return;
exec_fdiv:;
	converter.ll = stack[sp--];
	db = converter.d;
	converter.ll = stack[sp--];
	da = converter.d;
	printf("da %f | db %f\n", da, db);
	if( da==0 ) {
		printf("fdiv by 0.0 not allowed, restoring stack\n");
		sp += 2;
		return;
	}
	
	converter.d = db/da;
	stack[++sp] = converter.ll;
	printf("f div result %f\n", converter.d);
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
/*
typedef enum {
	// push and pop are always assumed to hold a long int
	nop=0,
	push, pop,
	add, fadd, sub, fsub,
	mul, fmul, idiv, fdiv, mod,
	jmp, lt, gt, cmp, 
	jnz, jz,
	inc, dec, shl, shr, //and, or, xor,
	cpy, swap, load, store,
	halt,
} InstrSet;*/
	// floats are converted to double
	uint64_t program[] = {
		// to deal with floats, we first convert them to an unsigned longs bit value
		push, 0,
		push, 0x4014000000000000,
		fdiv,
		pop,
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
	while( running ) {
		exec( program );
		ip++;
	}
	/*
	fclose(pFile); pFile=NULL;
	free(program); program=NULL;
	*/
	return 0;
}

void printBits(size_t const size_bytes, void const * const ptr)
{
	unsigned char *b = (unsigned char *)ptr;
	unsigned char byte;
	unsigned int i, j;

	for( i=size_bytes-1 ; i>=0 ; i-- ) {
		for( j=7 ; j>=0 ; j-- ) {
			byte = ( b[i] >> j ) & 1;
			printf("%u", byte);
		}
	}
	puts("");
}
