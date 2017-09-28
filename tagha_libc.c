
#include "tagha.h"
#include "tagha_libc/tagha_stdio.c"
#include "tagha_libc/tagha_stdlib.c"


void Tagha_load_libc_natives(struct TaghaVM *vm)
{
	if( !vm )
		return;
	
	Tagha_load_stdio_natives(vm);
	Tagha_load_stdlib_natives(vm);
}











