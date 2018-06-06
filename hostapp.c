
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include "tagha.h"


/* lldiv_t lldiv(long long int numer, long long int denom); */
void Native_lldiv(struct Tagha *sys, union Value *retval, const size_t args, union Value params[static args])
{
	(void)sys;
	(void)retval; // makes the compiler stop bitching.
	lldiv_t *val = params[0].Ptr;
	*val = lldiv(params[1].Int64, params[2].Int64);
}

int main(int argc, char *argv[static argc])
{
	(void)argc;
	(void)argv;
	/*
	if( !argv[1] ) {
		printf("[TaghaVM Usage]: '%s' '.tbc file' \n", argv[0]);
		return 1;
	}
	*/
	/*
	struct NativeInfo host_natives[] = {
		{"test", native_test_ptr},
		{"printHW", native_print_helloworld},
		{"getglobal", native_getglobal},
		{NULL, NULL}
	};
	Tagha_RegisterNatives(&vm, host_natives);
	*/
	uint8_t program[1024] = {0};
	union Value prog = (union Value){ .UCharPtr=program };
	*prog.UShortPtr = 0xC0DE; prog.UShortPtr++;
	*prog.UInt32Ptr = 256; prog.UInt32Ptr++;
	*prog.UCharPtr = 0; prog.UCharPtr++;
	*prog.UInt32Ptr = 1; prog.UInt32Ptr++;
	
	// create main function and add it to table!
	*prog.UCharPtr = 0; prog.UCharPtr++;
	*prog.UInt32Ptr = sizeof "main"; prog.UInt32Ptr++;
	*prog.UInt32Ptr = 43; prog.UInt32Ptr++;
	*prog.UCharPtr = 'm'; prog.UCharPtr++;
	*prog.UCharPtr = 'a'; prog.UCharPtr++;
	*prog.UCharPtr = 'i'; prog.UCharPtr++;
	*prog.UCharPtr = 'n'; prog.UCharPtr++;
	*prog.UCharPtr = 0; prog.UCharPtr++;
	
	// mov regAlaf, 50
	*prog.UCharPtr = mov; prog.UCharPtr++;
	*prog.UCharPtr = Register; prog.UCharPtr++;
	*prog.UCharPtr = Immediate; prog.UCharPtr++;
	*prog.UCharPtr = regAlaf; prog.UCharPtr++;
	*prog.UInt64Ptr = 50; prog.UInt64Ptr++;
	
	// mov regBeth, 2
	*prog.UCharPtr = mov; prog.UCharPtr++;
	*prog.UCharPtr = Register; prog.UCharPtr++;
	*prog.UCharPtr = Immediate; prog.UCharPtr++;
	*prog.UCharPtr = regBeth; prog.UCharPtr++;
	*prog.UInt64Ptr = 2; prog.UInt64Ptr++;
	
	// mul regAlaf, regBeth
	*prog.UCharPtr = mul; prog.UCharPtr++;
	*prog.UCharPtr = Register; prog.UCharPtr++;
	*prog.UCharPtr = Register; prog.UCharPtr++;
	*prog.UCharPtr = regBeth; prog.UCharPtr++;
	*prog.UCharPtr = regAlaf; prog.UCharPtr++;
	
	// mov regAlaf, 0
	*prog.UCharPtr = mov; prog.UCharPtr++;
	*prog.UCharPtr = Register; prog.UCharPtr++;
	*prog.UCharPtr = Immediate; prog.UCharPtr++;
	*prog.UCharPtr = regAlaf; prog.UCharPtr++;
	*prog.UInt64Ptr = 0; prog.UInt64Ptr++;
	
	// halt immediately ;)
	*prog.UCharPtr = halt; prog.UCharPtr++;
	*prog.UCharPtr = Immediate; prog.UCharPtr++;
	
	struct Tagha vm = (struct Tagha){0};
	Tagha_Init(&vm, program);
	int32_t result = Tagha_RunScript(&vm);
	printf("result?: '%u'\n", result);
	TaghaDebug_PrintRegisters(&vm);
	
	Tagha_Del(&vm);
	return 0;
}





