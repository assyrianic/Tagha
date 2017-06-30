
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>

bool running = true;

// index pointers should NEVER go under 0...
uint8_t ip=0, sp=0, callbp=0, callsp=0;


enum InstrSet {
	// push and pop are always assumed to hold a long int
	nop=0,
	push, pop, cpush, cpop,			// 1
	add, fadd, sub, fsub,			// 5
	mul, fmul, idiv, fdiv, mod,		// 9
	jmp, lt, gt, cmp, 			// 14
	jnz, jz,				// 18
	inc, dec, shl, shr, and, or, xor, not,	// 20
	cpy, swap,	// 28
	load, store,	// 30
	prol, epil, call, ret,	// 32
	halt,
};

#define STACKSIZE	256
uint64_t	stack[STACKSIZE];
uint64_t	callstack[STACKSIZE >> 2];
uint64_t	memory[STACKSIZE << 2];

// don't forget to update this!
const char *opcode2str[] = {
	"nop","push","pop","cpush","cpop",
	"add","fadd","sub","fsub","mul","fmul","idiv","fdiv","mod",
	"jmp","lt","gt","cmp","jnz","jz",
	"inc","dec","shl","shr","and","or","xor","not",
	"cpy","swap","load","store","prol","epil","call","ret",
	"halt"
};

void vm_exec(const uint64_t *code)
{
	uint64_t b, a;
	double da, db;
	
	static const void *dispatch[] = {
		&&exec_nop,
		&&exec_push, &&exec_pop, &&exec_cpush, &&exec_cpop,
		&&exec_add, &&exec_fadd, &&exec_sub, &&exec_fsub,
		&&exec_mul, &&exec_fmul, &&exec_idiv, &&exec_fdiv, &&exec_mod,
		&&exec_jmp, &&exec_lt, &&exec_gt, &&exec_cmp,
		&&exec_jnz, &&exec_jz,
		&&exec_inc, &&exec_dec, &&exec_shl, &&exec_shr, &&exec_and, &&exec_or, &&exec_xor, &&exec_not,
		&&exec_cpy, &&exec_swap, &&exec_load, &&exec_store,
		&&exec_prol, &&exec_epil, &&exec_call, &&exec_ret, 
		//&&exec_z,
		&&exec_halt
	};
	//printf( "current instruction == \"%s\"\n", opcode2str[code[ip]] );
	if( code[ip] > halt || code[ip] < nop ) {
		printf("illegal instruction exception! instruction == \'%" PRIu64 "\'\n", code[ip]);
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
exec_load:;	// stores a memory value into the top of the stack. pretty much push from memory.
	a = code[++ip];
	stack[++sp] = memory[a];
	printf("loaded %" PRIu64 " from memory[%" PRIu64 "]\n", stack[sp], a);
	ip++;
	return;
exec_store:;	// pops value off stack into memory.
	a = code[++ip];
	memory[a] = stack[sp--];
	printf("stored %" PRIu64 " to memory[%" PRIu64 "] | memory[%" PRIu64 "] = %" PRIu64 "\n", memory[a], a, a, stack[sp+1]);
	ip++;
	return;

/* C - x86 calling convention.
	code:
		cpush lastarg,
		cpush, 2nd-to-last-arg,
		...
		cpush, 1st-arg
		call procedure
		cpop all args
	procedure:
		prol,
		// callstack[callbp-1] == return address;
		1st_arg = callstack[callbp+1];
		// code
		epil,
		ret
*/
// procedure instructions | call and ret act more like 
exec_prol:;	// function prologue or beginning to start new stack frame.
	callstack[++callsp] = callbp;	// push	ebp
	callbp = callsp;		// mov	ebp, esp
	printf("prolog: pushed frame pointer %u to call stack %" PRIu64 "\n", callbp, callstack[callsp]);
	ip++;
	return;
exec_epil:;	// function epilogue or ending to restore previous stack frame.
	callsp = callbp;		// mov	esp, ebp
	callbp = callstack[callsp--];	// pop	ebp
	printf("epilog: popped frame pointer %u from call stack %" PRIu64 "\n", callbp, callstack[callsp+1]);
	ip++;
	return;
exec_call:;	// calling a procedure
	ip++;	// increment to function address
	callstack[++callsp] = ip+1;	// save post address so we can jump back to it after we finish.
	ip = code[ip];	// jump to function address.
	printf("calling @ %" PRIu64 " @ address: %u\n", callstack[callsp], ip);
	return;
exec_ret:;
	ip = callstack[callsp--];
	printf("returning to address: %u from %" PRIu64 "\n", ip, callstack[callsp]);
	return;
exec_cpush:;	// have call stack put from memory or from data stack? I pick data stack
	++callsp;
	if( !callsp ) {
		printf("call stack overflow!\n");
		goto *dispatch[halt];
	}
	callstack[callsp] = stack[sp--];
	printf("pushed to call stack: %" PRIu64 "\n", callstack[callsp]);
	ip++;
	return;
exec_cpop:;
	if( callsp )
		stack[++sp] = callstack[callsp--];
	if( callsp==255 ) {
		printf("call stack underflow!\n");
		goto *dispatch[halt];
	}
	printf("cpopped, call stack pointer %u | data from call stack : %" PRIu64 "\n", callsp, stack[sp]);
	ip++;
	return;

// various jumps
exec_jmp:;	// unconditional jump
	ip = code[ip+1];
	printf("jumping to... %u\n", ip);
	return;
exec_jnz:;	// Jump if Not Zero = JNZ
	++ip;
	//if( stack[sp] ) {
	//	ip=code[ip];
	//	printf("jnz'ing to... %u\n", ip);
	//}
	//else ++ip;
	ip = (stack[sp]) ? code[ip] : ip+1;
	printf("jnz'ing to... %u\n", ip);
	return;
exec_jz:;	// Jump if Zero = JZ
	++ip;
	//if( !stack[sp] ) {
	//	ip=code[ip];
	//	printf("jz'ing to... %u\n", ip);
	//}
	//else ++ip;
	ip = (!stack[sp]) ? code[ip] : ip+1;
	printf("jz'ing to... %u\n", ip);
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
	
// pushes and pops
exec_push:;	// put an item on the top of the stack
	++sp;
	if( !sp ) {	// if we increment sp and sp is 0, we ran out of stack memory.
		printf("stack overflow!\n");
		goto *dispatch[halt];
	}
	stack[sp] = code[++ip];
	printf("pushing %" PRIu64 "\n", stack[sp]);
	ip++;
	return;
exec_pop:;	// reduce stack
	if( sp )	// make sure that there's something in the stack before popping.
		sp--;
	if( sp==255 ) {		// if we decrement sp and sp's bits went all 1, we popped too much!
		printf("stack underflow!\n");
		goto *dispatch[halt];
	}
	printf("popped, stack pointer %x\n", sp);
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

// bit wise maths
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
exec_and:;
	b = stack[sp--];
	a = stack[sp--];
	stack[++sp] = b & a;
	printf( "bitwise and result %" PRIu64 "\n", stack[sp] );
	ip++;
	return;
exec_or:;
	b = stack[sp--];
	a = stack[sp--];
	stack[++sp] = b | a;
	printf( "bitwise or result %" PRIu64 "\n", stack[sp] );
	ip++;
	return;
exec_xor:;
	b = stack[sp--];
	a = stack[sp--];
	stack[++sp] = b ^ a;
	printf( "bitwise xor result %" PRIu64 "\n", stack[sp] );
	ip++;
	return;
exec_not:;
	a = stack[sp--];
	stack[++sp] = ~a;
	printf( "bitwise not result %" PRIu64 "\n", stack[sp] );
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
	uint64_t size = 0;
	if( !pFile )
		return size;
	
	if( !fseek(pFile, 0, SEEK_END) ) {
		size = ( uint64_t )ftell(pFile);
		rewind(pFile);
	}
	return size;
}

int main(void)
{
	/*
		uint i = 10;
		uint n = 0;
		while( n<i )
			++n;
	*/
	uint64_t loop[] = {
		push, 10,	// push 10
		store, 0,	// store 10 to memory address 0
		push, 0,	// push 0
		store, 1,	// store 0 to address #1
		load, 1,	// push 0 from address #1
		load, 0,	// push 10 from address #0
		lt,		// 0 < 10?
		//jz, 24,	// jump to halt if 0.
		jz, 22,
		load, 1,	// push 0 from memory
		//push, 1,	// push 1,
		//add,		// increment by 1, possibly change to inc?
		inc,		// increment by 1
		store, 1,	// store result to mem address #1.
		jmp, 8,		// jump to loading 0x01 into stack.
		halt
	};
	/*
		uint a = 10;
		if( a )
			a = 15;
	*/
	uint64_t ifcond[] = {
		push, 10,
		jz, 8,
		push, 15,
		store, 0x0,
		halt
	};
	// test call and ret opcodes
	uint64_t func[] = {
		nop,
		call, 4,	// 1
		halt,		// 3
		ret,		// 4
	};
	// test calls within calls and returning.
	uint64_t multicall[] = {
		nop,
		call, 6,
		call, 9,
		halt,
		push, 9,
		ret,
		push, 5,
		push, 10,
		mul,
		mul,
		ret,
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
		vm_exec( multicall );
	/*
	fclose(pFile); pFile=NULL;
	free(program); program=NULL;
	*/
	return 0;
}
