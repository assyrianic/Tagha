
#include "tagha.h"
#include "tagha_libc/tagha_stdio.c"
#include "tagha_libc/tagha_stdlib.c"

/* void debug_print_self_memory(void); */
static void native_dbug_print_mem(struct TaghaScript *script, union CValue params[], union CValue *restrict retval, const uint32_t argc, struct TaghaVM *env)
{
	TaghaScript_PrintMem(script);
}

/* void debug_print_self_ptrs(void); */
static void native_dbug_print_ptrs(struct TaghaScript *script, union CValue params[], union CValue *restrict retval, const uint32_t argc, struct TaghaVM *env)
{
	TaghaScript_PrintPtrs(script);
}

/* void debug_print_self_instrs(void); */
static void native_dbug_print_instrs(struct TaghaScript *script, union CValue params[], union CValue *restrict retval, const uint32_t argc, struct TaghaVM *env)
{
	TaghaScript_PrintInstrs(script);
}

/* void *script_get_global_by_name(const TaghaScript *restrict script, const char *restrict str); */
static void native_get_global_by_name(struct TaghaScript *script, union CValue params[], union CValue *restrict retval, const uint32_t argc, struct TaghaVM *env)
{
	TaghaScript *restrict other = params[0].Ptr;
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
	retval->Ptr = TaghaScript_GetGlobalByName(other, strglobal);
}

/* uint32_t script_get_mem_size(const TaghaScript *restrict script); */
static void native_get_mem_size(struct TaghaScript *script, union CValue params[], union CValue *restrict retval, const uint32_t argc, struct TaghaVM *env)
{
	TaghaScript *restrict other = params[0].Ptr;
	if( !other )
		puts("script_get_mem_size reported: 'script' is NULL!\n");
	retval->UInt32 = TaghaScript_GetMemSize(other);
}

/* uint32_t script_get_instr_size(const TaghaScript *restrict script); */
static void native_get_instr_size(struct TaghaScript *script, union CValue params[], union CValue *restrict retval, const uint32_t argc, struct TaghaVM *env)
{
	TaghaScript *restrict other = params[0].Ptr;
	if( !other )
		puts("script_get_instr_size reported: 'script' is NULL!\n");
	retval->UInt32 = TaghaScript_GetInstrSize(other);
}

/* TaghaScript	*get_script_from_file(const char *filename); */
static void native_get_script_from_file(struct TaghaScript *script, union CValue params[], union CValue *restrict retval, const uint32_t argc, struct TaghaVM *env)
{
	const char *restrict filename = params[0].String;
	if( !filename )
		puts("get_script_from_file reported: 'filename' is NULL!\n");
	else if( !strcmp(filename, script->m_strName) ) {
		puts("get_script_from_file reported: 'filename' can't be the same file as calling script!\n");
		return;
	}
	retval->Ptr = TaghaScript_FromFile(filename);
	filename = NULL;
}

/* void	script_free(TaghaScript *script); */
static void native_script_free(struct TaghaScript *script, union CValue params[], union CValue *restrict retval, const uint32_t argc, struct TaghaVM *env)
{
	TaghaScript *restrict other = params[0].Ptr;
	if( other==script ) {
		puts("script_free reported: 'script' cannot be the same ptr as calling script!\n");
		return;
	}
	else if( !strcmp(other->m_strName, script->m_strName) ) {
		puts("script_free reported: 'script' can't be the same file as calling script!\n");
		return;
	}
	TaghaScript_Free(other);
}

/* int32_t script_callfunc(TaghaScript *restrict script, const char *restrict strFunc); */
static void native_script_callfunc(struct TaghaScript *script, union CValue params[], union CValue *restrict retval, const uint32_t argc, struct TaghaVM *env)
{
	retval->Int32 = -1;
	TaghaScript *other = params[0].Ptr;
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
	retval->Int32 = Tagha_CallScriptFunc(env, strFunc);
	env->m_pScript = script;
}

/* void script_push_value(TaghaScript *script, const CValue value); */
static void native_script_push_value(struct TaghaScript *script, union CValue params[], union CValue *restrict retval, const uint32_t argc, struct TaghaVM *env)
{
	TaghaScript *restrict other = params[0].Ptr;
	if( other==script ) {
		puts("script_push_value reported: 'script' cannot be the same ptr as calling script!\n");
		return;
	}
	else if( !strcmp(other->m_strName, script->m_strName) ) {
		puts("script_push_value reported: 'script' can't be the same file as calling script!\n");
		return;
	}
	TaghaScript_PushValue(other, params[1]);
}

/* CValue script_pop_value(TaghaScript *script); */
static void native_script_pop_value(struct TaghaScript *script, union CValue params[], union CValue *restrict retval, const uint32_t argc, struct TaghaVM *env)
{
	TaghaScript *restrict other = params[0].Ptr;
	if( other==script ) {
		puts("script_push_value reported: 'script' cannot be the same ptr as calling script!\n");
		return;
	}
	else if( !strcmp(other->m_strName, script->m_strName) ) {
		puts("script_push_value reported: 'script' can't be the same file as calling script!\n");
		return;
	}
	*retval = TaghaScript_PopValue(other);
}



void Tagha_LoadLibCNatives(struct TaghaVM *vm)
{
	if( !vm )
		return;
	
	Tagha_load_stdio_natives(vm);
	Tagha_load_stdlib_natives(vm);
}

void Tagha_LoadSelfNatives(struct TaghaVM *vm)
{
	if( !vm )
		return;
	
	NativeInfo libc_self_natives[] = {
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
	Tagha_RegisterNatives(vm, libc_self_natives);
}










