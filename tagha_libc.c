
#include "tagha.h"
#include "tagha_libc/tagha_stdio.c"
#include "tagha_libc/tagha_stdlib.c"

/* Script_t *script_get_self(void); */
static void native_get_script_self(Script_t *restrict script, Param_t params[], Param_t **restrict retval, const uint32_t argc)
{
	(*retval)->Pointer = script;
}

/* void debug_print_self_memory(void); */
static void native_dbug_print_mem(Script_t *restrict script, Param_t params[], Param_t **restrict retval, const uint32_t argc)
{
	*retval = NULL;
	TaghaScript_debug_print_memory(script);
}

/* void debug_print_self_ptrs(void); */
static void native_dbug_print_ptrs(Script_t *restrict script, Param_t params[], Param_t **restrict retval, const uint32_t argc)
{
	*retval = NULL;
	TaghaScript_debug_print_ptrs(script);
}

/* void debug_print_self_instrs(void); */
static void native_dbug_print_instrs(Script_t *restrict script, Param_t params[], Param_t **restrict retval, const uint32_t argc)
{
	*retval = NULL;
	TaghaScript_debug_print_instrs(script);
}

/* void *script_get_global_by_name(const Script_t *restrict script, const char *restrict str); */
static void native_get_global_by_name(Script_t *restrict script, Param_t params[], Param_t **restrict retval, const uint32_t argc)
{
	Script_t *applet = params[0].Pointer;
	if( !applet )
		puts("script_get_global_by_name reported: 'applet' is NULL!\n");
	const char *restrict strglobal = params[1].String;
	if( !strglobal )
		puts("script_get_global_by_name reported: 'strglobal' is NULL!\n");
	(*retval)->Pointer = TaghaScript_get_global_by_name(applet, strglobal);
}

/* uint32_t script_get_stk_size(const Script_t *restrict script); */
static void native_get_stk_size(Script_t *restrict script, Param_t params[], Param_t **restrict retval, const uint32_t argc)
{
	Script_t *applet = params[0].Pointer;
	if( !applet )
		puts("script_get_stk_size reported: 'applet' is NULL!\n");
	(*retval)->UInt32 = TaghaScript_stacksize(applet);
}

/* uint32_t script_get_instr_size(const Script_t *restrict script); */
static void native_get_instr_size(Script_t *restrict script, Param_t params[], Param_t **restrict retval, const uint32_t argc)
{
	Script_t *applet = params[0].Pointer;
	if( !applet )
		puts("script_get_instr_size reported: 'applet' is NULL!\n");
	(*retval)->UInt32 = TaghaScript_instrsize(applet);
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
		{"script_get_self", native_get_script_self},
		{"debug_print_self_memory", native_dbug_print_mem},
		{"debug_print_self_ptrs", native_dbug_print_ptrs},
		{"debug_print_self_instrs", native_dbug_print_instrs},
		{"script_get_global_by_name", native_get_global_by_name},
		{"script_get_stk_size", native_get_stk_size},
		{"script_get_instr_size", native_get_instr_size},
		{NULL, NULL}
	};
	Tagha_register_natives(vm, libc_self_natives);
}










