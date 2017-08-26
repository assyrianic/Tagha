/* fwrite example : write buffer */
#include <stdio.h>
#include <stdint.h>

#define INSTR_SET	\
	X(halt) \
	X(pushq) X(pushl) X(pushs) X(pushb) X(pushsp) X(puship) X(pushbp) \
	X(pushspadd) X(pushspsub) X(pushbpadd) X(pushbpsub) \
	X(popq) X(popl) X(pops) X(popb) X(popsp) X(popip) X(popbp) \
	X(wrtq) X(wrtl) X(wrts) X(wrtb) \
	X(storeq) X(storel) X(stores) X(storeb) \
	X(storeqa) X(storela) X(storesa) X(storeba) \
	X(storespq) X(storespl) X(storesps) X(storespb) \
	X(loadq) X(loadl) X(loads) X(loadb) \
	X(loadqa) X(loadla) X(loadsa) X(loadba) \
	X(loadspq) X(loadspl) X(loadsps) X(loadspb) \
	X(copyq) X(copyl) X(copys) X(copyb) \
	X(addq) X(uaddq) X(addl) X(uaddl) X(addf) \
	X(subq) X(usubq) X(subl) X(usubl) X(subf) \
	X(mulq) X(umulq) X(mull) X(umull) X(mulf) \
	X(divq) X(udivq) X(divl) X(udivl) X(divf) \
	X(modq) X(umodq) X(modl) X(umodl) \
	X(addf64) X(subf64) X(mulf64) X(divf64) \
	X(andl) X(orl) X(xorl) X(notl) X(shll) X(shrl) \
	X(andq) X(orq) X(xorq) X(notq) X(shlq) X(shrq) \
	X(incq) X(incl) X(incf) X(decq) X(decl) X(decf) X(negq) X(negl) X(negf) \
	X(incf64) X(decf64) X(negf64) \
	X(ltq) X(ltl) X(ultq) X(ultl) X(ltf) \
	X(gtq) X(gtl) X(ugtq) X(ugtl) X(gtf) \
	X(cmpq) X(cmpl) X(ucmpq) X(ucmpl) X(compf) \
	X(leqq) X(uleqq) X(leql) X(uleql) X(leqf) \
	X(geqq) X(ugeqq) X(geql) X(ugeql) X(geqf) \
	X(ltf64) X(gtf64) X(cmpf64) X(leqf64) X(geqf64) \
	X(neqq) X(uneqq) X(neql) X(uneql) X(neqf) X(neqf64) \
	X(jmp) X(jzq) X(jnzq) X(jzl) X(jnzl) \
	X(call) X(calls) X(calla) X(ret) X(retx) X(reset) \
	X(wrtnataddr) X(pushnataddr) X(callnat) X(callnats) X(callnata) \
	X(mmxaddl) X(mmxuaddl) X(mmxaddf) X(mmxadds) X(mmxuadds) X(mmxaddb) X(mmxuaddb) \
	X(mmxsubl) X(mmxusubl) X(mmxsubf) X(mmxsubs) X(mmxusubs) X(mmxsubb) X(mmxusubb) \
	X(mmxmull) X(mmxumull) X(mmxmulf) X(mmxmuls) X(mmxumuls) X(mmxmulb) X(mmxumulb) \
	X(mmxdivl) X(mmxudivl) X(mmxdivf) X(mmxdivs) X(mmxudivs) X(mmxdivb) X(mmxudivb) \
	X(mmxmodl) X(mmxumodl) X(mmxmods) X(mmxumods) X(mmxmodb) X(mmxumodb) \
	X(mmxandl) X(mmxands) X(mmxandb) X(mmxorl) X(mmxors) X(mmxorb) \
	X(mmxxorl) X(mmxxors) X(mmxxorb) X(mmxnotl) X(mmxnots) X(mmxnotb) \
	X(mmxshll) X(mmxshls) X(mmxshlb) X(mmxshrl) X(mmxshrs) X(mmxshrb) \
	X(mmxincl) X(mmxincf) X(mmxincs) X(mmxincb) X(mmxdecl) X(mmxdecf) X(mmxdecs) X(mmxdecb) \
	X(mmxnegl) X(mmxnegf) X(mmxnegs) X(mmxnegb) \
	X(nop) \

#define X(x) x,
enum InstrSet { INSTR_SET };
#undef X
typedef uint8_t		bytecode[];

int main ()
{
	FILE *pFile = NULL;
	unsigned short magic = 0xC0DE;
	
	bytecode endian_test1 = {
		0xDE, 0xC0,	// magic
		18,0,0,0,	// set instruction pointer entry point
		4,0,0,0,	// set memory size.
		5,0,0,0,	// set stack size. count up every stack item and add 1
		0,0,0,0,	// set amount of natives!
		nop,
		//pushl, 255, 1, 0, 0x0,
		//pushs, 0xDD, 0xDD,
		//pushb, 0xAA,
		//pushb, 0xFF,
		wrtl,	0,0,0,0,	0xA,0xB,0xC,0xD,
		pushl,	0xA,0xB,0xC,0xD,
		halt
	};
	bytecode float_test = {
		18,0,0,0,
		0,0,0,0,	// set memory size.
		9,0,0,0,	// set stack size.
		0,0,0,0,	// set amount of natives!
		//jmp, 17,0,0,0,
		// -16776961
		//pushl, 255,0,0,255,
		//pushl, 1,0,0,0,
		// -855637761
		// 10.f in 4 bytes form
		pushl,	65,32,0,0,
		// 2.f in 4 bytes form
		pushl,	64,0,0,0,
		addf,	// 12.f
		//leqf,
		halt
	};
	
	// Fibonnaci sequence to test performance!
	/*	int fib(int n)
		{
			int a=0, b=1;
			while (n-- > 1) {
				int t = a;
				a = b;
				b += t;
			}
			return b;
		}
	*/
	bytecode fibonacci = {
		18,0,0,0,
		20,0,0,0,	// set memory size.
		60,0,0,0,	// set stack size.
		0,0,0,0,	// set amount of natives!
		nop, // calc fibonnaci number
		wrtl,	0,0,0,0,	0,0,0,7,	// write n to address 0, remember that memory is little endian!
		call,	0,0,0,34,
		halt,
		// a = 0;
		wrtl,	0,0,0,4,	0,0,0,0,		// 16
		// b = 1;
		wrtl,	0,0,0,8,	0,0,0,1,		// 25
		// while( n-- > 1 ) {
		loadl,	0,0,0,0,		// load param n		// 34
		pushl,	0,0,0,1,		// push 1
		gtl,
		loadl,	0,0,0,0,		// load param n
		decl,				// decrement address 0
		storel,	0,0,0,0,	// store decrement result to memory address
		jzl,	0,0,0,121,		// jmp to storing b and returning.
		popl,
		// int t = a;
		loadl,	0,0,0,4,		// load a's value.
		storel,	0,0,0,12,	// store a's value into another address as 't'
		// a = b;
		loadl,	0,0,0,8,		// load b.
		storel,	0,0,0,4,	// store b's value into a's address.
		// b += t;
		loadl,	0,0,0,12,	// load t.
		loadl,	0,0,0,8,		// load b.
		uaddl,				// add b and t
		storel,	0,0,0,8,	// store addition value to b's address.
		jmp,	0,0,0,52,		// jmp back to start of loop.	// 98
		// }
		ret		// b has been fully 'mathemized' and is stored into memory for reuse.
	};
	
	bytecode hello_world = {
		18,0,0,0,
		12,0,0,0,	// set memory size.
		0,0,0,0,	// set stack size.
		0,0,0,0,	// set amount of natives!
		nop,
		wrtb,	0,0,0,0,	100,	// d
		wrtb,	0,0,0,1,	108,	// l
		wrtb,	0,0,0,2,	114,	// r
		wrtb,	0,0,0,3,	111,	// o
		wrtb,	0,0,0,4,	87,		// W
		wrtb,	0,0,0,5,	32,		// space
		wrtb,	0,0,0,6,	111,	// o
		wrtb,	0,0,0,7,	108,	// l
		wrtb,	0,0,0,8,	108,	// l
		wrtb,	0,0,0,9,	101,	// e
		wrtb,	0,0,0,10,	72,		// H
		halt
	};
	
	bytecode global_pointers = {
		18,0,0,0,
		255,1,0,0,	// set memory size.
		16,0,0,0,	// set stack size.
		0,0,0,0,	// set amount of natives!
		nop,
		// The way you wuold store to a pointer would be something like...
		// pushl <value to store>
		// loadl <ptr address>
		// storela
		
		// push 170 to tos.
		pushl,	0,0,0,0xAA,
		// store 170 to address 255 as variable 'i'
		// int i = 170;
		storel,	0,0,0,0xff,
		
		// the way you would load from a pointer would be something like:
		// loadl <ptr address>
		// loadla
		
		// load address of 'i'
		// int *p = &i;
		// *p = 26;		// i == 26;
		pushl,	0,0,0,0x1a,
		pushl, 0,0,0,0xff,
		storela, // pops 4 byte address, then pops 4 byte data into memory address.
		halt
	};
	
	// example of locally (stack-allocated) made pointers and manipulating them.
	bytecode local_pointers = {
		18,0,0,0,
		1,0,0,0,	// set memory size.
		16,0,0,0,	// set stack size.
		0,0,0,0,	// set amount of natives!
		nop,
		// int i = 170;
		// i's address is 4 (tos address, not beginning data address)
		pushl,	0,0,0,0xaa,
		
		// int *p = &i;
		pushl,	0,0,0,0xff,	// going to overwrite &i with 255.
		pushl,	0,0,0,4, storespl,	// stack index of i aka &i
		halt
	};
	
	// void func(int a, int b) { a+b; }
	// func declarations are done by cdecl standard.
	bytecode test_func_call = {
		18,0,0,0,
		1,0,0,0,	// set memory size.
		28,0,0,0,	// set stack size.
		0,0,0,0,	// set amount of natives!
		pushl,	0,0,1,244,	//6-10		push b
		pushl,	0,0,0,2,	//11-15		push a
		call,	0,0,0,36,	//16-20		func(int a, int b) ==> b=500 and a=2
		popl,popl,	//21-22 clean up args a and b from stack
		halt,			// 23
		pushl,	0,0,0,8, loadspl,	// load a to TOS
		pushl,	0,0,0,4, loadspl,	// load b to TOS
		addl,	// a+b;
		ret		// 22
	};
	
	
	bytecode test_call_opcodes = {
		18,0,0,0,
		5,0,0,0,	// set memory size.
		28,0,0,0,	// set stack size.
		0,0,0,0,	// set amount of natives!
		call,	0,0,0,45,
		pushl,	0,0,0,51,	//11
		calls,	//16
		
		wrtl,	0,0,0,0,	0,0,0,57,	//17
		pushl,	0,0,0,0,	//26
		calla,	//31
		halt,	//32
		
		pushl,	0xa,0xb,0xc,0xd,	// 33
		ret,	//38
		
		pushl,	0,0,0xff,0xff,	//39
		ret,	//44
		
		pushl,	0,0,0xac,0xca,	//45
		ret	//50
	};
	
	bytecode all_opcodes_test = {
		18,0,0,0,
		255,0,0,0,	// set memory size.
		255,0,0,0,	// set stack size.
		0,0,0,0,	// set amount of natives!
		
		// push + pop tests
		pushq,	0xa,0xb,0xc,0xd,0xe,0xf,0xaa,0xbb, popq,
		pushl,	0xa,0xb,0xc,0xd, popl,
		pushs,	0xff,0xaa, pops,
		pushb,	0xfa, popb,
		pushsp,	popsp,
		//puship,	popip,	// it works, trust me lol
		pushbp, popbp,
		
		// write to memory tests
		wrtl,	0,0,0,0,	0xa,0xb,0xc,0xd,	// direct write to address 0x0
		wrts,	0,0,0,4,	0xff,0xaa,			// direct write to address 0x4
		wrtb,	0,0,0,6,	0xfa,				// direct write to address 0x6
		wrtq,	0,0,0,10,	0xaa,0xab,0xac,0xad, 0xae,0xaf,0xba,0xbb,
		
		// store + load tests
		pushl,	0xa,0xb,0xc,0xd,
		storel,	0,0,0,0,
		loadl,	0,0,0,0,
		
		pushs,	0xff,0xaa,
		stores,	0,0,0,0,
		loads,	0,0,0,0,
		
		pushb,	0xfa,
		storeb,	0,0,0,0,
		loadb,	0,0,0,0,
		
		wrtl,	0,0,0,0,	0xa,0xb,0xc,0xd,
		pushl,	0xa,0xb,0xc,0xd,
		pushl,	0,0,0,0,
		storela,
		
		wrts,	0,0,0,0,	0xff,0xaa,
		pushs,	0xff,0xaa,
		pushl,	0,0,0,0,
		storesa,
		
		wrtb,	0,0,0,0,	0xfa,
		pushb,	0xfa,
		pushl,	0,0,0,0,
		storeba,
		
		wrtl,	0,0,0,0,	0xa,0xb,0xc,0xd,
		pushl,	0,0,0,0,
		loadla,
		
		wrts,	0,0,0,0,	0xff,0xaa,
		pushl,	0,0,0,0,
		loadsa,
		
		wrtb,	0,0,0,0,	0xfa,
		pushl,	0,0,0,0,
		loadba,
		
		pushl,	0xa,0xb,0xc,0xd,
		pushl,	0,0,0,4,
		loadspl,
		
		pushs,	0xff,0xaa,
		pushl,	0,0,0,2,
		loadsps,
		
		pushb,	0xfa,
		pushl,	0,0,0,1,
		loadspb,
		
		pushl,	0xa,0xb,0xc,0xd,
		pushl,	0,0,0,4,
		storespl,
		
		pushs,	0xff,0xaa,
		pushl,	0,0,0,2,
		storesps,
		
		pushb,	0xfa,
		pushl,	0,0,0,1,
		storespb,
		
		pushl,	0xa,0xb,0xc,0xd,	copyl,
		pushs,	0xff,0xaa,	copys,
		pushb,	0xfa,	copyb,
		
		halt
	};
	
	pFile = fopen("./endian_test1.tbc", "wb");
	if( pFile ) {
		fwrite(endian_test1, sizeof(uint8_t), sizeof(endian_test1), pFile);
		fclose(pFile);
	}
	pFile = fopen("./float_test.tbc", "wb");
	if( pFile ) {
		fwrite(&magic, sizeof(unsigned short), 1, pFile);
		fwrite(float_test, sizeof(uint8_t), sizeof(float_test), pFile);
		fclose(pFile);
	}
	pFile = fopen("./fibonacci.tbc", "wb");
	if( pFile ) {
		fwrite(&magic, sizeof(unsigned short), 1, pFile);
		fwrite(fibonacci, sizeof(uint8_t), sizeof(fibonacci), pFile);
		fclose(pFile);
	}
	pFile = fopen("./global_pointers.tbc", "wb");
	if( pFile ) {
		fwrite(&magic, sizeof(unsigned short), 1, pFile);
		fwrite(global_pointers, sizeof(uint8_t), sizeof(global_pointers), pFile);
		fclose(pFile);
	}
	pFile = fopen("./local_pointers.tbc", "wb");
	if( pFile ) {
		fwrite(&magic, sizeof(unsigned short), 1, pFile);
		fwrite(local_pointers, sizeof(uint8_t), sizeof(local_pointers), pFile);
		fclose(pFile);
	}
	pFile = fopen("./test_func_call.tbc", "wb");
	if( pFile ) {
		fwrite(&magic, sizeof(unsigned short), 1, pFile);
		fwrite(test_func_call, sizeof(uint8_t), sizeof(test_func_call), pFile);
		fclose(pFile);
	}
	pFile = fopen("./test_call_opcodes.tbc", "wb");
	if( pFile ) {
		fwrite(&magic, sizeof(unsigned short), 1, pFile);
		fwrite(test_call_opcodes, sizeof(uint8_t), sizeof(test_call_opcodes), pFile);
		fclose(pFile);
	}
	pFile = fopen("./all_opcodes_test.tbc", "wb");
	if( pFile ) {
		fwrite(&magic, sizeof(unsigned short), 1, pFile);
		fwrite(all_opcodes_test, sizeof(uint8_t), sizeof(all_opcodes_test), pFile);
		fclose(pFile);
	}
	
	bytecode test_retx_func = {
		18,0,0,0,
		0,0,0,0,	// set memory size.
		24,0,0,0,	// set stack size.
		0,0,0,0,	// set amount of natives!
		// func prototype -> int f(int);
		pushl,	0,0,0,9,	//6-10	-push argument 1.
		call,	0,0,0,29,	//11-15	-"f(5);"
		halt,	//16
		
		// int f(int i) {
		pushl,	0,0,0,8,	// [ebp+8] since x86 Stack grows "downward" from high to low address.
		pushbpsub, loadspl,	// load argument i.
		pushl,	0,0,0,6,	//17-21	-int b = 6;
		addl,	//22
		retx,	0,0,0,4	//23-27	-return a+b;
		// }
	};
	pFile = fopen("./test_retx_func.tbc", "wb");
	if( pFile ) {
		fwrite(&magic, sizeof(unsigned short), 1, pFile);
		fwrite(test_retx_func, sizeof(uint8_t), sizeof(test_retx_func), pFile);
		fclose(pFile);
	}
	
	bytecode test_recursion = {
		18,0,0,0,	// 2-5
		0,0,0,0,	// set memory size.
		255,0,0,0,	// set stack size.
		0,0,0,0,	// set amount of natives!
		call,	0,0,0,24,	//6-10
		halt,	//11
		call,	0,0,0,24 //12-16
	};
	pFile = fopen("./test_recursion.tbc", "wb");
	if( pFile ) {
		fwrite(&magic, sizeof(unsigned short), 1, pFile);
		fwrite(test_recursion, sizeof(uint8_t), sizeof(test_recursion), pFile);
		fclose(pFile);
	}
	
	/*
	main() {
		int n = factorial(15);
	}
	
	int factorial(unsigned int i) {
		if( i<=1 )
			return 1;
		return i * factorial( i-1 );
		//	int temp = factorial( i-1 );
		//	return i*temp;
	}
	*/
	bytecode test_factorial_recurs = {
		18,0,0,0,	// 2-5
		0,0,0,0,	// set memory size.
		255,0,0,0,	// set stack size.
		0,0,0,0,	// set amount of natives!
		pushl,	0,0,0,7,	//14-18
		call,	0,0,0,29,	//19-15
		halt,	//16
		
		pushl,	0,0,0,8,	//17-21	// [ebp+8] to get i from stack.
		pushbpsub,
		loadspl,	//22-23	// load i to Top of Stack.
		pushl,	0,0,0,1,	//24-28	// push 1
		uleql,	//29	// i <= 1?
		jzl,	0,0,0,57,	//30-34	// if 0, jump passed the first `retx`.
		pushl,	0,0,0,1,	//35-39
		retx,	0,0,0,4,	//40-44
		
		pushl,	0,0,0,8,	//45-49	// [ebp+8] to get i from stack.
		pushbpsub,
		loadspl,	// load i to Top of Stack.
		pushl,	0,0,0,1,
		usubl,	// i-1
		call,	0,0,0,29,	// get result of call, with (i-1) as arg.
		// each call makes a new stack frame, regardless of call type opcode.
		
		pushl,	0,0,0,8,	// [ebp+8] to get i from stack.
		pushbpsub,
		loadspl,	// load i to Top of Stack.
		umull,
		retx,	0,0,0,4
	};
	pFile = fopen("./test_factorial_recurs.tbc", "wb");
	if( pFile ) {
		fwrite(&magic, sizeof(unsigned short), 1, pFile);
		fwrite(test_factorial_recurs, sizeof(uint8_t), sizeof(test_factorial_recurs), pFile);
		unsigned funcs = 1;
		fwrite(&funcs, sizeof(unsigned), 1, pFile);
		fclose(pFile);
	}
	/*
	bytecode test_native = {
		0xDE, 0xC0, 6,0,0,0,	// 0-5
		pushl,	0,0,0,5,
		callnat,	4,1,	// #1 - bytes to push, #2 - number of args
		halt,
	};
	*/
	/*
	bytecode test_native = {
		0xDE, 0xC0, 6,0,0,0,	// 0-5
		pushl,	64,160,0,0,
		pushb,	127,
		pushs,	0xff,0xff,
		pushl,	0,0,0,200,
		callnat, 4, halt,	//6-7
	};
	*/
	bytecode test_native = {
		27,0,0,0,	// 2-5
		0,0,0,0,	// set memory size.
		16,0,0,0,	// set stack size.
		1,0,0,0,	// set amount of natives!
		5,0,0,0,	't','e','s','t',0,	// string size of 1st native
		pushl,	0,0,0,50,	// ammo
		pushl,	0,0,0,100,	// health
		pushl,	67,150,0,0,	// speed
		callnat, 0,0,0,0, 0,0,0,12, 0,0,0,1,	// #1 - get native name, #2 - bytes to push, #3 - number of args
		halt
	};
	pFile = fopen("./test_native.tbc", "wb");
	if( pFile ) {
		fwrite(&magic, sizeof(unsigned short), 1, pFile);
		fwrite(test_native, sizeof(uint8_t), sizeof(test_native), pFile);
		fclose(pFile);
	}
	
	bytecode mmx_test={
		18,0,0,0,	// 2-5
		0,0,0,0,	// set memory size.
		20,0,0,0,	// set stack size.
		0,0,0,0,	// set amount of natives!
		pushq,	0,0,0,255, 0,0,0,1,
		pushq,	0,0,0,5, 0,0,0,2,
		//mmxaddl, // treats the 64-bit values as 4 ints, added top down (2 on bottom is added to 1 at top, 5 is added to 255.)
		mmxmuls, //mmxnegb,
		halt
	};
	pFile = fopen("./mmx_test.tbc", "wb");
	if( pFile ) {
		fwrite(&magic, sizeof(unsigned short), 1, pFile);
		fwrite(mmx_test, sizeof(uint8_t), sizeof(mmx_test), pFile);
		fclose(pFile);
	}
	
	bytecode test_local_native_funcptr = {
		0xDE, 0xC0,	// magic
		27,0,0,0,	// set entry point, remember to account for natives.
		1,0,0,0,	// set memory size.
		24,0,0,0,	// set stack size.
		1,0,0,0,	// set amount of natives!
		5,0,0,0,	't','e','s','t',0,	// string size of 1st native
		pushnataddr,	0,0,0,0,// push native's function ptr, assume it pushes 8 bytes
		pushl,	0,0,0,50,	// ammo
		pushl,	0,0,0,100,	// health
		pushl,	67,150,0,0,	// speed
		callnats, 0,0,0,12, 0,0,0,1,	// #1 - bytes to push, #2 - number of args
		halt
	};
	pFile = fopen("./test_local_native_funcptr.tbc", "wb");
	if( pFile ) {
		//fwrite(&magic, sizeof(unsigned short), 1, pFile);
		fwrite(test_local_native_funcptr, sizeof(uint8_t), sizeof(test_local_native_funcptr), pFile);
		fclose(pFile);
	}
	bytecode test_global_native_funcptr = {
		0xDE, 0xC0,	// magic
		27,0,0,0,	// set entry point, remember to account for natives.
		1,0,0,0,	// set memory size.
		24,0,0,0,	// set stack size.
		1,0,0,0,	// set amount of natives!
		5,0,0,0,	't','e','s','t',0,	// string size of 1st native
		wrtnataddr,	0,0,0,0,	0,0,0,0,	// #1 - native name index, #2 - memory address to write to.
		pushl,	0,0,0,50,	// ammo
		pushl,	0,0,0,100,	// health
		pushl,	67,150,0,0,	// speed "300.f"
		callnata, 0,0,0,0, 0,0,0,12, 0,0,0,1,	// #1 - mem address, #2 - bytes to push, #3 - number of args
		halt
	};
	pFile = fopen("./test_global_native_funcptr.tbc", "wb");
	if( pFile ) {
		fwrite(test_global_native_funcptr, sizeof(uint8_t), sizeof(test_global_native_funcptr), pFile);
		fclose(pFile);
	}
	bytecode test_multiple_natives = {
		0xDE, 0xC0,	// magic
		39,0,0,0,	// 2-5
		0,0,0,0,	// 6-9 set memory size.
		16,0,0,0,	// 10-13 set stack size.
		2,0,0,0,	// 14-17 set amount of natives!
		5,0,0,0,	// 18-21
		't','e','s','t',0,	// 22-26 string size of 1st native
		8,0,0,0,	// 27-30
		'p','r','i','n','t','H','W',0,	// 31-38
		pushl,	0,0,0,50,	// ammo
		pushl,	0,0,0,100,	// health
		pushl,	67,150,0,0,	// speed
		callnat, 0,0,0,1, 0,0,0,0, 0,0,0,0,	// #1 - get native name, #2 - bytes to push, #3 - number of args
		callnat, 0,0,0,0, 0,0,0,12, 0,0,0,1,	// #1 - get native name, #2 - bytes to push, #3 - number of args
		halt
	};
	pFile = fopen("./test_multiple_natives.tbc", "wb");
	if( pFile ) {
		fwrite(test_multiple_natives, sizeof(uint8_t), sizeof(test_multiple_natives), pFile);
		fclose(pFile);
	}
	pFile = NULL;
	return 0;
}








