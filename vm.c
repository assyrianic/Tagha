
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>

#include <unistd.h>	// sleep()


enum InstrSet {
	// push and pop are always assumed to hold a long int
	nop=0,
	push, pop, pushsp, popsp,		// 1
	add, fadd, sub, fsub,			// 5
	mul, fmul, idiv, fdiv, mod,		// 9
	jmp, lt, gt, cmp, 			// 14
	jnz, jz,				// 18
	inc, dec, shl, shr, and, or, xor, not,	// 20
	cpy, swap,	// 28
	load, store,	// 30
	call, ret,	// 32
	halt,
};

bool running = true;

// index pointers should NEVER go under 0...
uint8_t ip=0, sp=0, callsp=0, callbp=0;

#define STACKSIZE	256
uint64_t	stack[STACKSIZE];
uint64_t	callstack[STACKSIZE >> 2];
uint64_t	memory[STACKSIZE << 2];

// don't forget to update this!
const char *opcode2str[] = {
	"nop","push","pop","pushsp", "popsp",
	"add","fadd","sub","fsub","mul","fmul","idiv","fdiv","mod",
	"jmp","lt","gt","cmp","jnz","jz",
	"inc","dec","shl","shr","and","or","xor","not",
	"cpy","swap","load","store","call","ret",
	"halt"
};

void vm_exec(const uint64_t *code)
{
	uint64_t b, a;
	double da, db;
	
	static const void *dispatch[] = {
		&&exec_nop,
		&&exec_push, &&exec_pop, &&exec_pushsp, &&exec_popsp,
		&&exec_add, &&exec_fadd, &&exec_sub, &&exec_fsub,
		&&exec_mul, &&exec_fmul, &&exec_idiv, &&exec_fdiv, &&exec_mod,
		&&exec_jmp, &&exec_lt, &&exec_gt, &&exec_cmp,
		&&exec_jnz, &&exec_jz,
		&&exec_inc, &&exec_dec, &&exec_shl, &&exec_shr, &&exec_and, &&exec_or, &&exec_xor, &&exec_not,
		&&exec_cpy, &&exec_swap, &&exec_load, &&exec_store,
		&&exec_call, &&exec_ret, 
		//&&exec_z,
		&&exec_halt
	};
	if( code[ip] > halt || code[ip] < nop ) {
		printf("illegal instruction exception! instruction == \'%" PRIu64 "\'\n", code[ip]);
		goto *dispatch[halt];
		return;
	}
	printf( "current instruction == \"%s\"\n", opcode2str[code[ip]] );
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

// procedure instructions
exec_call:;	// calling a procedure
	ip++;	// increment to function address
	printf("calling address: %u\n", ip);
	callstack[++callsp] = ip+1;	// save post address so we can jump back to it after we finish.
	callbp = callsp;	// save stack pointer to frame pointer so we can make a stack frame
	ip = code[ip];	// jump to function address.
	printf("call return addr: %" PRIu64 " | frame ptr == %u\n", callstack[callsp], callbp);
	return;
exec_ret:;
	callsp = callbp;
	printf("callsp set to callbp, callsp == %u\n", callsp);
	ip = callstack[callsp--];
	callbp = callsp;
	printf("returning to address: %u\n", ip);
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
exec_pushsp:;	// pushes value of sp to the top of the stack
	++sp;
	if( !sp ) {	// if we increment sp and sp is 0, we ran out of stack memory.
		printf("stack overflow!\n");
		goto *dispatch[halt];
	}
	stack[sp] = sp-1;
	printf("pushing sp val of %" PRIu64 "\n", stack[sp]);
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
exec_popsp:;	// Pops value off top of stack and sets SP to that value
	if( sp )
		sp = stack[sp];
	printf("popped sp, stack pointer %x\n", sp);
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
	stack[++sp] = a-b;
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
	if( b==0 ) {
		printf("div by 0 not allowed, restoring stack\n");
		sp += 2;
		ip++;
		return;
	}
	stack[++sp] = a/b;
	printf("div result %" PRIu64 "\n", stack[sp]);
	ip++;
	return;
exec_mod:;
	b = stack[sp--];
	a = stack[sp--];
	if( a==0 ) {
		printf("mod by 0 not allowed, restoring stack\n");
		sp += 2;
		ip++;
		return;
	}
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
		call, 5,	// 1
		jmp, 11,
		push, 10,
		push, 15,
		add,
		ret,		// 4
		halt,		// 3
	};
	// test calls within calls and returning.
	uint8_t func1=4, func2=9;
	uint64_t callercalling[] = {
		nop,
		call, func1,
		halt,
	// func1:
		push, 9,	// 4
		call, func2,
		ret,
	// func2:
		push, 5,	// 10
		push, 10,
		mul,
		mul,
		ret	// 15
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
		vm_exec( callercalling );
		sleep(1);
	}
	/*
	fclose(pFile); pFile=NULL;
	free(program); program=NULL;
	*/
	return 0;
}
