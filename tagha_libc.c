
#include "tagha.h"
#include "tagha_libc/tagha_stdio.c"
#include "tagha_libc/tagha_stdlib.c"

/* void debug_print_self_memory(void); */
static void native_dbug_print_mem(Script_t *script, Param_t params[], Param_t *restrict retval, const uint32_t argc, TaghaVM_t *env)
{
	TaghaScript_debug_print_memory(script);
}

/* void debug_print_self_ptrs(void); */
static void native_dbug_print_ptrs(Script_t *script, Param_t params[], Param_t *restrict retval, const uint32_t argc, TaghaVM_t *env)
{
	TaghaScript_debug_print_ptrs(script);
}

/* void debug_print_self_instrs(void); */
static void native_dbug_print_instrs(Script_t *script, Param_t params[], Param_t *restrict retval, const uint32_t argc, TaghaVM_t *env)
{
	TaghaScript_debug_print_instrs(script);
}

/* void *script_get_global_by_name(const Script_t *restrict script, const char *restrict str); */
static void native_get_global_by_name(Script_t *script, Param_t params[], Param_t *restrict retval, const uint32_t argc, TaghaVM_t *env)
{
	Script_t *restrict other = params[0].Ptr;
	if( !other ) {
		puts("script_get_global_by_name reported: 'script' is NULL!\n");
		return;
	}
	else if( other==script ) {
		puts("script_get_global_by_name reported: 'script' can't be the same ptr as calling script!\n");
		return;
	}
	else if( !strcmp(other->m_strName, script->m_strName) ) {
		puts("script_get_global_by_name reported: 'script' can't be the same file as calling script!\n");
		return;
	}
	const char *restrict strglobal = params[1].String;
	if( !strglobal ) {
		puts("script_get_global_by_name reported: 'str' is NULL!\n");
		return;
	}
	retval->Ptr = TaghaScript_get_global_by_name(other, strglobal);
}

/* uint32_t script_get_mem_size(const Script_t *restrict script); */
static void native_get_mem_size(Script_t *script, Param_t params[], Param_t *restrict retval, const uint32_t argc, TaghaVM_t *env)
{
	Script_t *restrict other = params[0].Ptr;
	if( !other )
		puts("script_get_mem_size reported: 'script' is NULL!\n");
	retval->UInt32 = TaghaScript_memsize(other);
}

/* uint32_t script_get_instr_size(const Script_t *restrict script); */
static void native_get_instr_size(Script_t *script, Param_t params[], Param_t *restrict retval, const uint32_t argc, TaghaVM_t *env)
{
	Script_t *restrict other = params[0].Ptr;
	if( !other )
		puts("script_get_instr_size reported: 'script' is NULL!\n");
	retval->UInt32 = TaghaScript_instrsize(other);
}

/* Script_t	*get_script_from_file(const char *filename); */
static void native_get_script_from_file(Script_t *script, Param_t params[], Param_t *restrict retval, const uint32_t argc, TaghaVM_t *env)
{
	const char *restrict filename = params[0].String;
	if( !filename )
		puts("get_script_from_file reported: 'filename' is NULL!\n");
	else if( !strcmp(filename, script->m_strName) ) {
		puts("get_script_from_file reported: 'filename' can't be the same file as calling script!\n");
		return;
	}
	retval->Ptr = TaghaScript_from_file(filename);
	filename = NULL;
}

/* void	script_free(Script_t *script); */
static void native_script_free(Script_t *script, Param_t params[], Param_t *restrict retval, const uint32_t argc, TaghaVM_t *env)
{
	Script_t *restrict other = params[0].Ptr;
	if( other==script ) {
		puts("script_free reported: 'script' cannot be the same ptr as calling script!\n");
		return;
	}
	else if( !strcmp(other->m_strName, script->m_strName) ) {
		puts("script_free reported: 'script' can't be the same file as calling script!\n");
		return;
	}
	TaghaScript_free(other);
}

/* int32_t script_callfunc(Script_t *restrict script, const char *restrict strFunc); */
static void native_script_callfunc(Script_t *script, Param_t params[], Param_t *restrict retval, const uint32_t argc, TaghaVM_t *env)
{
	retval->Int32 = -1;
	Script_t *other = params[0].Ptr;
	if( other==script ) {
		puts("script_callfunc reported: 'script' cannot be the same ptr as calling script!\n");
		return;
	}
	else if( !strcmp(other->m_strName, script->m_strName) ) {
		puts("script_callfunc reported: 'script' can't be the same file as calling script!\n");
		return;
	}
	const char *strFunc = params[1].String;
	if( !strFunc ) {
		puts("script_callfunc reported: 'strFunc' is NULL!\n");
		return;
	}
	env->m_pScript = other;
	retval->Int32 = Tagha_call_script_func(env, strFunc);
	env->m_pScript = script;
}

/* void script_push_value(Script_t *script, const CValue_t value); */
static void native_script_push_value(Script_t *script, Param_t params[], Param_t *restrict retval, const uint32_t argc, TaghaVM_t *env)
{
	Script_t *restrict other = params[0].Ptr;
	if( other==script ) {
		puts("script_push_value reported: 'script' cannot be the same ptr as calling script!\n");
		return;
	}
	else if( !strcmp(other->m_strName, script->m_strName) ) {
		puts("script_push_value reported: 'script' can't be the same file as calling script!\n");
		return;
	}
	TaghaScript_push_value(other, params[1]);
}

/* CValue_t script_pop_value(Script_t *script); */
static void native_script_pop_value(Script_t *script, Param_t params[], Param_t *restrict retval, const uint32_t argc, TaghaVM_t *env)
{
	Script_t *restrict other = params[0].Ptr;
	if( other==script ) {
		puts("script_push_value reported: 'script' cannot be the same ptr as calling script!\n");
		return;
	}
	else if( !strcmp(other->m_strName, script->m_strName) ) {
		puts("script_push_value reported: 'script' can't be the same file as calling script!\n");
		return;
	}
	*retval = TaghaScript_pop_value(other);
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
		{"debug_print_self_memory", native_dbug_print_mem},
		{"debug_print_self_ptrs", native_dbug_print_ptrs},
		{"debug_print_self_instrs", native_dbug_print_instrs},
		{"script_get_global_by_name", native_get_global_by_name},
		{"script_get_mem_size", native_get_mem_size},
		{"script_get_instr_size", native_get_instr_size},
		{"get_script_from_file", native_get_script_from_file},
		{"script_free", native_script_free},
		{"script_callfunc", native_script_callfunc},
		{"script_push_value", native_script_push_value},
		{"script_pop_value", native_script_pop_value},
		{NULL, NULL}
	};
	Tagha_register_natives(vm, libc_self_natives);
}










