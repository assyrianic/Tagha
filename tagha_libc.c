
#include "tagha.h"
#include "tagha_libc/tagha_stdio.c"
#include "tagha_libc/tagha_stdlib.c"

/* Script_t *get_self(void); */
static void native_get_script_self(Script_t *restrict script, Param_t params[], union Param **restrict retval, const uint32_t argc)
{
	(*retval)->Pointer = script;
}


void Tagha_load_libc_natives(struct TaghaVM *vm)
{
	if( !vm )
		return;
	
	Tagha_load_stdio_natives(vm);
	Tagha_load_stdlib_natives(vm);
}

void Tagha_load_self_natives(struct TaghaVM *vm)
{
	if( !vm )
		return;
	
	NativeInfo_t libc_self_natives[] = {
		{"get_self", native_get_script_self},
		{NULL, NULL}
	};
	Tagha_register_natives(vm, libc_self_natives);
}










