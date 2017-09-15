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

void wrt_funcs_to_header(FILE *f, const unsigned num_funcs, ...)
{
	if( !f )
		return;
	
	fwrite(&num_funcs, sizeof(unsigned), 1, f);
	if( num_funcs ) {
		char *buffer;
		va_list varfuncs;
		va_start(varfuncs, num_funcs);
		unsigned i;
		for( i=0 ; i<num_funcs ; i++ ) {
			buffer = va_arg(varfuncs, char*);
			unsigned strsize = strlen(buffer)+1;	// copy null terminator too!
			fwrite(&strsize, sizeof(unsigned), 1, f);
			fwrite(buffer, sizeof(char), strsize, f);
			
			unsigned params = va_arg(varfuncs, unsigned);
			fwrite(&params, sizeof(unsigned), 1, f);
			
			unsigned entry = va_arg(varfuncs, unsigned);
			fwrite(&entry, sizeof(unsigned), 1, f);
		}
	}
}

int main(int argc, char **argv)
{
	FILE *pFile = NULL;
	
	
	
	bytecode test_malloc = {
		0,0,0,0,	// set instruction pointer entry point
		call,	0,0,0,6,	// call main
		halt,
		pushl,		0,0,0,4,
		callnat,	0,0,0,0, 0,0,0,4, 0,0,0,1,	// #1 - get native name, #2 - bytes to push, #3 - number of args
		callnat,	0,0,0,1, 0,0,0,4, 0,0,0,1,	// #1 - get native name, #2 - bytes to push, #3 - number of args
		ret
	};
	pFile = fopen("./test_malloc.tbc", "wb");
	if( pFile ) {
		wrt_tbc_headers(pFile, 32);
		wrt_natives_to_header(pFile, 2, "malloc", "free");
		wrt_funcs_to_header(pFile, 1, "main", 0, 6);
		fwrite(test_malloc, sizeof(uint8_t), sizeof(test_malloc), pFile);
		fclose(pFile);
	}
	
error:;
	pFile = NULL;
	return 0;
}








