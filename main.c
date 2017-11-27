#include <stdio.h>
#include <string.h>
#include "tagha.h"


/* void print_helloworld(void); */
static void native_print_helloworld(Script_t *script, Param_t params[], Param_t **restrict retval, const uint32_t argc, TaghaVM_t *env)
{
	*retval = NULL;
	puts("native_print_helloworld :: hello world from bytecode!\n");
}

int main(int argc, char **argv)
{
	char main = 11;
	char fact = 59;
	uint8_t code[] = {
		call, Immediate,		// 0-1
			main,0,0,0,0,0,0,0,	// 2-9
		halt, // 10
	// main:
		// sub rsp, 16
		usubr, Immediate,	//11-12
			rsp,0,0,0,0,0,0,0, //13-20
			16,0,0,0,0,0,0,0, //21-28
			
		// mov rcs, 7
		movr, Immediate, //29-30
			rcs,0,0,0,0,0,0,0, //31-38
			7,0,0,0,0,0,0,0, //39-46
		
		// call factorial
		call, Immediate, //47-48
			fact,0,0,0,0,0,0,0, //49-56
		
		ret, 0, //57-58
		
	// factorial:
		// sub rsp, 16
		usubr, Immediate,	//59-60
			rsp,0,0,0,0,0,0,0, //61-68
			16,0,0,0,0,0,0,0, //69-76

		// mov (uint32_t *)[rbp-4], rcs
		movm, RegIndirect|Register|FourBytes,	//77-78
			rbp,0,0,0,0,0,0,0, //79-86
			rcs,0,0,0,0,0,0,0, //87-94
				252,255,255,255, //95-98	// rbp + -4
		
		// cmp (uint32_t *)[rbp-4], 1
		ucmpm, RegIndirect|Immediate|FourBytes, //99-100
			rbp,0,0,0,0,0,0,0, //101-108
			1,0,0,0,0,0,0,0, //109-116
				252,255,255,255, //117-120	// rbp + -4
		// jump if zero
		jz, Immediate, //121-122
			151,0,0,0,0,0,0,0, //123-130
		
		// return 1;
		movr, Immediate, //131-132
			ras,0,0,0,0,0,0,0, //133-140
			1,0,0,0,0,0,0,0, //141-148
		ret, 0, //149-150
		
		// mov ras, (uint32_t *)[rbp-4]
		movr, RegIndirect|FourBytes,
			ras,0,0,0,0,0,0,0,
			rbp,0,0,0,0,0,0,0,
				252,255,255,255, // rbp + -4
		
		// sub ras, 1
		usubr, Immediate,
			ras,0,0,0,0,0,0,0,
			1,0,0,0,0,0,0,0,
		
		// mov rcs, ras
		movr, Register,
			rcs,0,0,0,0,0,0,0,
			ras,0,0,0,0,0,0,0,
		
		// call factorial
		call, Immediate,
			fact,0,0,0,0,0,0,0,
		
		// mul eax, (uint32_t *)[rbp-4]
		umulr, RegIndirect|FourBytes,
			ras,0,0,0,0,0,0,0, //53-60
			rbp,0,0,0,0,0,0,0, //61-68
				252,255,255,255, //69-72	// rbp + -4
		
		ret, 0
	};
	printf("this VM has == %" PRIu32 " opcodes\n", nop+1);
	return vm_exec(code);
}
