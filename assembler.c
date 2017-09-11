#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <iso646.h>

#define INSTR_SET	\
	X(halt) \
	X(pushq) X(pushl) X(pushs) X(pushb) X(pushsp) X(puship) X(pushbp) \
	X(pushspadd) X(pushspsub) X(pushbpadd) X(pushbpsub) \
	X(popq) X(popl) X(pops) X(popb) X(popsp) X(popip) X(popbp) \
	X(storespq) X(storespl) X(storesps) X(storespb) \
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
	X(call) X(calls) X(ret) X(retx) X(reset) \
	X(pushnataddr) X(callnat) X(callnats) \
	X(nop) \

#define X(x) x,
enum InstrSet { INSTR_SET };
#undef X
typedef uint8_t		bytecode[];


void wrt_tbc_headers(FILE *f, const unsigned memsize)
{
	if( !f )
		return;
	
	short magic = 0xC0DE;
	fwrite(&magic, sizeof(unsigned short), 1, f);
	fwrite(&memsize, sizeof(unsigned), 1, f);
}

void wrt_natives_to_header(FILE *f, const unsigned numnats, ...)
{
	if( !f )
		return;
	
	fwrite(&numnats, sizeof(unsigned), 1, f);
	if( numnats ) {
		char *buffer;
		va_list varnatives;
		va_start(varnatives, numnats);
		unsigned i;
		for( i=0 ; i<numnats ; i++ ) {
			buffer = va_arg(varnatives, char*);
			unsigned strsize = strlen(buffer)+1;	// copy null terminator too!
			fwrite(&strsize, sizeof(unsigned), 1, f);
			fwrite(buffer, sizeof(char), strsize, f);
		}
	}
}

int main(int argc, char **argv)
{
	FILE *pFile = NULL;
	unsigned short magic = 0xC0DE;
	
	/*
	linkedlist *list = prep_file(argv[1]);
	if( !list ) {
		printf("Tagha ASM: ERROR **** file \'%s\' can't be found. ****\n", argv[1]);
		goto error;
	}
	dict_init(&g_mapOpcodes);
	dict_init(&g_mapSymTable);

#define X(x) #x,
	{
		uint8_t i;
		const char *opcode2str[] = { INSTR_SET };
#undef X
		for( i=0 ; i<nop+1 ; i++ )
			dict_insert(&g_mapOpcodes, opcode2str[i], i);
	}
	strip_comments(list);
	linkedlist *tokens = lex_lines(list);
	list = NULL;
	
	
	listnode *pNode = tokens->pHead;
	uint64_t i, c;
	for( i=0L ; i<tokens->ulCount ; i++ ) {
		char *pstrLine = string_cstr(listnode_getstr(pNode));
		printf("Tagha ASMPRINT: tokens:: \'%s\'\n", pstrLine);
		pNode = pNode->pNext;
		if( !pNode )
			break;
	}
	free(tokens), tokens = NULL;
	*/
	
	//unsigned short magic = 0xC0DE;
	
	bytecode endian_test1 = {
		//0xDE, 0xC0,	// magic
		//4,0,0,0,	// set memory size.
		//5,0,0,0,	// set stack size. count up every stack item and add 1
		//0,0,0,0,	// set amount of natives!
		0,0,0,0,	// set instruction pointer entry point
		nop,
		//pushl, 255, 1, 0, 0x0,
		//pushs, 0xDD, 0xDD,
		//pushb, 0xAA,
		//pushb, 0xFF,
		pushl,	0xA,0xB,0xC,0xD,
		halt
	};
	pFile = fopen("./endian_test1.tbc", "wb");
	if( pFile ) {
		wrt_tbc_headers(pFile, 5);
		wrt_natives_to_header(pFile, 0);
		fwrite(endian_test1, sizeof(uint8_t), sizeof(endian_test1), pFile);
		fclose(pFile);
	}
	
	bytecode float_test = {
		9,0,0,0,	// set stack size.
		0,0,0,0,	// set amount of natives!
		0,0,0,0,	// set instruction pointer entry point
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
	
	bytecode hello_world_char_ptr = {
		0,0,0,0,	// set instruction pointer entry point
		pushb,	0,
		pushb,	10,		// newline char
		pushb,	100,	// d
		pushb,	108,	// l
		pushb,	114,	// r
		pushb,	111,	// o
		pushb,	87,		// W
		pushb,	32,		// space
		pushb,	111,	// o
		pushb,	108,	// l
		pushb,	108,	// l
		pushb,	101,	// e
		pushb,	72,		// H
		call,	0,0,0,32,	// call main
		halt,
		// we pushed 13 bytes worth of chars, let's push as pointer!
		pushl, 0,0,0,18,	// address of first byte is at 0x1
		callnat, 0,0,0,0, 0,0,0,4, 0,0,0,1,	// #1 - get native name, #2 - bytes to push, #3 - number of args
		// realistically, we can push all 13 bytes to the native but this is to demonstrate arrays as pointers.
		ret
	};
	pFile = fopen("./hello_world.tbc", "wb");
	if( pFile ) {
		wrt_tbc_headers(pFile, 32);
		wrt_natives_to_header(pFile, 1, "puts");
		fwrite(hello_world_char_ptr, sizeof(uint8_t), sizeof(hello_world_char_ptr), pFile);
		fclose(pFile);
	}
	
	// example of locally (stack-allocated) made pointers and manipulating them.
	bytecode pointers = {
		16,0,0,0,	// set stack size.
		0,0,0,0,	// set amount of natives!
		0,0,0,0,
		nop,
		// int i = 170;
		// i's address is 11 (tos address, not beginning data address)
		pushl,	0,0,0,34,
		
		// int *p = &i;
		pushl,	0,0,2,0xaf,	// going to overwrite &i with 255.
		pushl,	0,0,0,11,	storespl,	// stack index of i aka &i
		halt
	};
	
	// void func(int a, int b) { a+b; }
	// func declarations are done by cdecl standard.
	bytecode test_func_call = {
		28,0,0,0,	// set stack size.
		0,0,0,0,	// set amount of natives!
		0,0,0,0,
		pushl,	0,0,1,244,	//0-4		push b
		pushl,	0,0,0,2,	//5-9		push a
		call,	0,0,0,18,	//10-14		func(int a, int b) ==> b=500 and a=2
		popl,popl,	//15-16 clean up args a and b from stack
		halt,			// 17
		pushl,	0,0,0,19, loadspl,	// load a to TOS
		pushl,	0,0,0,23, loadspl,	// load b to TOS
		addl,	// a+b;
		ret		// 22
	};
	
	
	bytecode test_call_opcodes = {
		28,0,0,0,	// set stack size.
		0,0,0,0,	// set amount of natives!
		0,0,0,0,
		call,	0,0,0,12,	// 0-4
		pushl,	0,0,0,18,	// 5-9
		calls,	// 10
		
		halt,	//11
		
		pushl,	0xa,0xb,0xc,0xd,	// 12-16
		ret,	//17
		
		pushl,	0,0,0xff,0xff,	//18-22
		ret,	//23
	};
	
	bytecode all_opcodes_test = {
		0,0,0,0,
		// push + pop tests
		pushq,	0xa,0xb,0xc,0xd,0xe,0xf,0xaa,0xbb, popq,
		pushl,	0xa,0xb,0xc,0xd, popl,
		pushs,	0xff,0xaa, pops,
		pushb,	0xfa, popb,
		pushsp,	popsp,
		//puship,	popip,	// it works, trust me lol
		pushbp, popbp,
		
		
		pushl,	0xa,0xb,0xc,0xd,
		pushl,	0,0,0,1,
		loadspl,
		
		pushs,	0xff,0xaa,
		pushl,	0,0,0,5,
		loadsps,
		
		pushb,	0xfa,
		pushl,	0,0,0,7,
		loadspb,
		
		pushl,	0xa,0xb,0xc,0xd,
		pushl,	0,0,0,1,
		storespl,
		
		pushs,	0xff,0xaa,
		pushl,	0,0,0,5,
		storesps,
		
		pushb,	0xfa,
		pushl,	0,0,0,7,
		storespb,
		
		pushl,	0xa,0xb,0xc,0xd,	copyl,
		pushs,	0xff,0xaa,	copys,
		pushb,	0xfa,	copyb,
		
		halt
	};
	
	pFile = fopen("./float_test.tbc", "wb");
	if( pFile ) {
		fwrite(&magic, sizeof(unsigned short), 1, pFile);
		fwrite(float_test, sizeof(uint8_t), sizeof(float_test), pFile);
		fclose(pFile);
	}
	pFile = fopen("./pointers.tbc", "wb");
	if( pFile ) {
		fwrite(&magic, sizeof(unsigned short), 1, pFile);
		fwrite(pointers, sizeof(uint8_t), sizeof(pointers), pFile);
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
		wrt_tbc_headers(pFile, 255);
		wrt_natives_to_header(pFile, 0);
		fwrite(all_opcodes_test, sizeof(uint8_t), sizeof(all_opcodes_test), pFile);
		fclose(pFile);
	}
	
	bytecode test_retx_func = {
		24,0,0,0,	// set stack size.
		0,0,0,0,	// set amount of natives!
		0,0,0,0,
		// func prototype -> int f(int);
		pushl,	0,0,0,9,	//6-10	-push argument 1.
		call,	0,0,0,11,	//11-15	-"f(5);"
		halt,	//16
		
		// int f(int i) {
		pushl,	0,0,0,8,	// [bp+8] retrieve 9 since it's "global" (pushed before main)
		pushbpadd, loadspl,	// load argument i.
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
		255,255,255,0,	// set stack size.
		0,0,0,0,	// set amount of natives!
		0,0,0,0,
		call,	0,0,0,6,	//6-10
		halt,	//11
		call,	0,0,0,6 //12-16
	};
	pFile = fopen("./test_recursion.tbc", "wb");
	if( pFile ) {
		fwrite(&magic, sizeof(unsigned short), 1, pFile);
		fwrite(test_recursion, sizeof(uint8_t), sizeof(test_recursion), pFile);
		fclose(pFile);
	}
	
	//main() {
	//	int n = factorial(7);
	//}
	//
	//int factorial(unsigned int i) {
	//	if( i<=1 )
	//		return 1;
	//	return i * factorial( i-1 );
	//	//	int temp = factorial( i-1 );
	//	//	return i*temp;
	//}
	bytecode test_factorial_recurs = {
		255,0,0,0,	// set stack size.
		0,0,0,0,	// set amount of natives!
		0,0,0,0,
		pushl,	0,0,0,7,	//14-18
		call,	0,0,0,11,	//19-15
		halt,	//16
		
		pushl,	0,0,0,8,	//17-21	// [bp+8] to get i from stack.
		pushbpadd,
		loadspl,	//22-23	// load i to Top of Stack.
		pushl,	0,0,0,1,	//24-28	// push 1
		uleql,	//29	// i <= 1?
		jzl,	0,0,0,39,	//30-34	// if 0, jump passed the first `retx`.
		pushl,	0,0,0,1,	//35-39
		retx,	0,0,0,4,	//40-44
		
		pushl,	0,0,0,8,	//45-49	// [bp+8] to get i from stack.
		pushbpadd,	// 
		loadspl,	// load i to Top of Stack.
		pushl,	0,0,0,1,
		usubl,	// i-1
		call,	0,0,0,11,	// get result of call, with (i-1) as arg.
		// each call makes a new stack frame, regardless of call type opcode.
		
		pushl,	0,0,0,8,	// [bp+8] to get i from stack.
		pushbpadd,
		loadspl,	// load i to Top of Stack.
		umull,
		retx,	0,0,0,4
	};
	pFile = fopen("./test_factorial_recurs.tbc", "wb");
	if( pFile ) {
		fwrite(&magic, sizeof(unsigned short), 1, pFile);
		fwrite(test_factorial_recurs, sizeof(uint8_t), sizeof(test_factorial_recurs), pFile);
		//unsigned funcs = 1;
		//fwrite(&funcs, sizeof(unsigned), 1, pFile);
		fclose(pFile);
	}
	/*
	bytecode test_native = {
		0xDE, 0xC0, 6,0,0,0,	// 0-5
		pushl,	0,0,0,5,
		callnat,	4,1,	// #1 - bytes to push, #2 - number of args
		halt,
	};
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
		32,0,0,0,	// set stack size.
		1,0,0,0,	// set amount of natives!
		5,0,0,0,	't','e','s','t',0,	// string size of 1st native
		0,0,0,0,
		pushl,	0,0,0,50,	// ammo
		pushl,	0,0,0,100,	// health
		pushl,	67,150,0,0,	// speed
		pushl,	0,0,0,19,	// address of our struct data
		callnat, 0,0,0,0, 0,0,0,4, 0,0,0,1,	// #1 - get native name, #2 - bytes to push, #3 - number of args
		halt
	};
	pFile = fopen("./test_native.tbc", "wb");
	if( pFile ) {
		fwrite(&magic, sizeof(unsigned short), 1, pFile);
		fwrite(test_native, sizeof(uint8_t), sizeof(test_native), pFile);
		fclose(pFile);
	}
	
	bytecode test_local_native_funcptr = {
		0xDE, 0xC0,	// magic
		32,0,0,0,	// set stack size.
		1,0,0,0,	// set amount of natives!
		5,0,0,0,	't','e','s','t',0,	// string size of 1st native
		0,0,0,0,	// set entry point, remember to account for natives.
		pushl,	0,0,0,50,	// ammo
		pushl,	0,0,0,100,	// health
		pushl,	67,150,0,0,	// speed
		pushl,	0,0,0,19,	// address of our struct data
		pushnataddr,	0,0,0,0,// push native's function ptr, assume it pushes 8 bytes
		callnats, 0,0,0,4, 0,0,0,1,	// #1 - bytes to push, #2 - number of args
		halt
	};
	pFile = fopen("./test_local_native_funcptr.tbc", "wb");
	if( pFile ) {
		//fwrite(&magic, sizeof(unsigned short), 1, pFile);
		fwrite(test_local_native_funcptr, sizeof(uint8_t), sizeof(test_local_native_funcptr), pFile);
		fclose(pFile);
	}
	
	bytecode test_multiple_natives = {
		//0xDE, 0xC0,	// magic
		//0,0,0,0,	// 6-9 set memory size.
		//16,0,0,0,	// 10-13 set stack size.
		//2,0,0,0,	// 14-17 set amount of natives!
		//5,0,0,0,	// 18-21
		//'t','e','s','t',0,	// 22-26 string size of 1st native
		//8,0,0,0,	// 27-30
		//'p','r','i','n','t','H','W',0,	// 31-38
		0,0,0,0,	// set entry point, remember to account for natives written.
		pushl,	0,0,0,50,	// ammo
		pushl,	0,0,0,100,	// health
		pushl,	67,150,0,0,	// speed
		pushl,	0,0,0,19,	// address of our struct data
		callnat, 0,0,0,1, 0,0,0,0, 0,0,0,0,	// #1 - get native name, #2 - bytes to push, #3 - number of args
		callnat, 0,0,0,0, 0,0,0,4, 0,0,0,1,	// #1 - get native name, #2 - bytes to push, #3 - number of args
		halt
	};
	pFile = fopen("./test_multiple_natives.tbc", "wb");
	if( pFile ) {
		wrt_tbc_headers(pFile, 32);
		wrt_natives_to_header(pFile, 2, "test", "printHW");
		fwrite(test_multiple_natives, sizeof(uint8_t), sizeof(test_multiple_natives), pFile);
		fclose(pFile);
	}
	
	bytecode test_int2chr = {
		0,0,0,0,	// set entry point, remember to account for natives written.
		call,	0,0,0,6,
		halt,
		
		pushl,	0,0,5,42,
		pushl,	0,0,0,4,
		pushbpsub,
		loadspb,
		ret
	};
	pFile = fopen("./test_int2chr.tbc", "wb");
	if( pFile ) {
		wrt_tbc_headers(pFile, 32);
		wrt_natives_to_header(pFile, 0);
		fwrite(test_int2chr, sizeof(uint8_t), sizeof(test_int2chr), pFile);
		fclose(pFile);
	}
	
	bytecode test_printf = {
		0,0,0,0,	// set instruction pointer entry point
		pushb,	0,
		pushb,	'\n',		// newline char
		pushb,	'f',
		pushb,	'%',
		pushb,	' ',
		pushb,	'i',		// W
		pushb,	'%',		// space
		pushb,	'=',	// o
		pushb,	'=',	// o
		pushb,	'm',	// l
		pushb,	'u',	// e
		pushb,	'n',		// H
		pushb,	'\n',		// newline char
		call,	0,0,0,32,	// call main
		halt,
		// we pushed 13 bytes worth of chars, let's push as pointer!
		
		pushq, 64,114,192,0, 0,0,0,0,	// 300.0, printf only accepts 8-byte float constants.
		pushl, 0,0,1,24,
		pushl, 0,0,0,8,
		pushbpadd,
		callnat, 0,0,0,0, 0,0,0,16, 0,0,0,3,	// #1 - get native name, #2 - bytes to push, #3 - number of args
		ret
	};
	pFile = fopen("./test_printf.tbc", "wb");
	if( pFile ) {
		wrt_tbc_headers(pFile, 64);
		wrt_natives_to_header(pFile, 1, "printf");
		fwrite(test_printf, sizeof(uint8_t), sizeof(test_printf), pFile);
		fclose(pFile);
	}
	
	bytecode test_fopen = {
		0,0,0,0,	// set instruction pointer entry point
		pushb,	0,
		pushb,	'c',
		pushb,	'b',
		pushb,	't',
		pushb,	'.',
		pushb,	'1',
		pushb,	't',
		pushb,	's', //15
		pushb,	'e',
		pushb,	't',
		pushb,	'_',
		pushb,	'n',
		pushb,	'a',
		pushb,	'i',
		pushb,	'd',
		pushb,	'n',
		pushb,	'e',
		pushb,	'/',
		pushb,	'.',	// addr 44
		pushb,	0,
		pushb,	'b',
		pushb,	'r',	// addr 41
		call,	0,0,0,50,	// call main
		halt,
		
		pushl, 0,0,0,8,	// gets mode string
		pushbpadd,
		pushl, 0,0,0,11,	// gets filename string.
		pushbpadd,
		callnat, 0,0,0,0, 0,0,0,8, 0,0,0,2,	// #1 - get native name, #2 - bytes to push, #3 - number of args
		ret
	};
	pFile = fopen("./test_fopen.tbc", "wb");
	if( pFile ) {
		wrt_tbc_headers(pFile, 64);
		wrt_natives_to_header(pFile, 1, "fopen");
		fwrite(test_fopen, sizeof(uint8_t), sizeof(test_fopen), pFile);
		fclose(pFile);
	}
	
	bytecode test_malloc = {
		0,0,0,0,	// set instruction pointer entry point
		call,	0,0,0,6,	// call main
		halt,
		pushl,		0,0,0,4,
		callnat,	0,0,0,0, 0,0,0,4, 0,0,0,1,	// #1 - get native name, #2 - bytes to push, #3 - number of args
		pushsp,	// we just pushed the pointer. Let's get it back as a virtual address.
		callnat,	0,0,0,1, 0,0,0,4, 0,0,0,1,	// #1 - get native name, #2 - bytes to push, #3 - number of args
		ret
	};
	pFile = fopen("./test_malloc.tbc", "wb");
	if( pFile ) {
		wrt_tbc_headers(pFile, 32);
		wrt_natives_to_header(pFile, 2, "malloc", "free");
		fwrite(test_malloc, sizeof(uint8_t), sizeof(test_malloc), pFile);
		fclose(pFile);
	}
	
error:;
	pFile = NULL;
	return 0;
}








