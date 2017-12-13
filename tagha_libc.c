
#include "tagha.h"
#include "tagha_libc/tagha_stdio.c"
#include "tagha_libc/tagha_stdlib.c"

/* void Debug_PrintSelfMemory(void); */
static void native_dbug_print_mem(struct TaghaScript *pScript, union CValue params[], union CValue *restrict pRetval, const uint32_t argc, struct TaghaVM *pEnv)
{
	TaghaScript_PrintStack(pScript);
}

/* void debug_print_self_ptrs(void); */
static void native_dbug_print_ptrs(struct TaghaScript *pScript, union CValue params[], union CValue *restrict pRetval, const uint32_t argc, struct TaghaVM *pEnv)
{
	TaghaScript_PrintPtrs(pScript);
}

/* void debug_print_self_instrs(void); */
static void native_dbug_print_instrs(struct TaghaScript *pScript, union CValue params[], union CValue *restrict pRetval, const uint32_t argc, struct TaghaVM *pEnv)
{
	TaghaScript_PrintInstrs(pScript);
}

/* void *TaghaScript_GetGlobalByName(const TaghaScript *restrict pScript, const char *restrict str); */
static void native_get_global_by_name(struct TaghaScript *pScript, union CValue params[], union CValue *restrict pRetval, const uint32_t argc, struct TaghaVM *pEnv)
{
	TaghaScript *restrict other = params[0].Ptr;
	if( !other ) {
		puts("pScript_get_global_by_name reported: 'pScript' is NULL!\n");
		return;
	}
	else if( other==pScript ) {
		puts("pScript_get_global_by_name reported: 'pScript' can't be the same ptr as calling pScript!\n");
		return;
	}
	else if( !strcmp(other->m_strName, pScript->m_strName) ) {
		puts("pScript_get_global_by_name reported: 'pScript' can't be the same file as calling pScript!\n");
		return;
	}
	const char *restrict strglobal = params[1].String;
	if( !strglobal ) {
		puts("pScript_get_global_by_name reported: 'str' is NULL!\n");
		return;
	}
	pRetval->Ptr = TaghaScript_GetGlobalByName(other, strglobal);
}

/* uint32_t pScript_get_mem_size(const TaghaScript *restrict pScript); */
static void native_get_mem_size(struct TaghaScript *pScript, union CValue params[], union CValue *restrict pRetval, const uint32_t argc, struct TaghaVM *pEnv)
{
	TaghaScript *restrict other = params[0].Ptr;
	if( !other )
		puts("pScript_get_mem_size reported: 'pScript' is NULL!\n");
	pRetval->UInt32 = TaghaScript_GetMemSize(other);
}

/* uint32_t pScript_get_instr_size(const TaghaScript *restrict pScript); */
static void native_get_instr_size(struct TaghaScript *pScript, union CValue params[], union CValue *restrict pRetval, const uint32_t argc, struct TaghaVM *pEnv)
{
	TaghaScript *restrict other = params[0].Ptr;
	if( !other )
		puts("pScript_get_instr_size reported: 'pScript' is NULL!\n");
	pRetval->UInt32 = TaghaScript_GetInstrSize(other);
}

/* TaghaScript	*get_pScript_from_file(const char *filename); */
static void native_get_pScript_from_file(struct TaghaScript *pScript, union CValue params[], union CValue *restrict pRetval, const uint32_t argc, struct TaghaVM *pEnv)
{
	const char *restrict filename = params[0].String;
	if( !filename )
		puts("get_pScript_from_file reported: 'filename' is NULL!\n");
	else if( !strcmp(filename, pScript->m_strName) ) {
		puts("get_pScript_from_file reported: 'filename' can't be the same file as calling pScript!\n");
		return;
	}
	pRetval->Ptr = TaghaScript_BuildFromFile(filename);
	filename = NULL;
}

/* void	pScript_free(struct TaghaScript *pScript); */
static void native_pScript_free(struct TaghaScript *pScript, union CValue params[], union CValue *restrict pRetval, const uint32_t argc, struct TaghaVM *pEnv)
{
	struct TaghaScript *restrict other = params[0].Ptr;
	if( other==pScript ) {
		puts("pScript_free reported: 'pScript' cannot be the same ptr as calling pScript!\n");
		return;
	}
	else if( !strcmp(other->m_strName, pScript->m_strName) ) {
		puts("pScript_free reported: 'pScript' can't be the same file as calling pScript!\n");
		return;
	}
	TaghaScript_Free(other);
}

/* int32_t pScript_callfunc(TaghaScript *restrict pScript, const char *restrict strFunc); */
static void native_pScript_callfunc(struct TaghaScript *pScript, union CValue params[], union CValue *restrict pRetval, const uint32_t argc, struct TaghaVM *pEnv)
{
	pRetval->Int32 = -1;
	TaghaScript *other = params[0].Ptr;
	if( other==pScript ) {
		puts("pScript_callfunc reported: 'pScript' cannot be the same ptr as calling pScript!\n");
		return;
	}
	else if( !strcmp(other->m_strName, pScript->m_strName) ) {
		puts("pScript_callfunc reported: 'pScript' can't be the same file as calling pScript!\n");
		return;
	}
	const char *strFunc = params[1].String;
	if( !strFunc ) {
		puts("pScript_callfunc reported: 'strFunc' is NULL!\n");
		return;
	}
	pEnv->m_pScript = other;
	pRetval->Int32 = Tagha_CallScriptFunc(pEnv, strFunc);
	pEnv->m_pScript = pScript;
}

/* void pScript_push_value(TaghaScript *pScript, const CValue value); */
static void native_pScript_push_value(struct TaghaScript *pScript, union CValue params[], union CValue *restrict pRetval, const uint32_t argc, struct TaghaVM *pEnv)
{
	TaghaScript *restrict other = params[0].Ptr;
	if( other==pScript ) {
		puts("pScript_push_value reported: 'pScript' cannot be the same ptr as calling pScript!\n");
		return;
	}
	else if( !strcmp(other->m_strName, pScript->m_strName) ) {
		puts("pScript_push_value reported: 'pScript' can't be the same file as calling pScript!\n");
		return;
	}
	TaghaScript_PushValue(other, params[1]);
}

/* CValue pScript_pop_value(TaghaScript *pScript); */
static void native_pScript_pop_value(struct TaghaScript *pScript, union CValue params[], union CValue *restrict pRetval, const uint32_t argc, struct TaghaVM *pEnv)
{
	TaghaScript *restrict other = params[0].Ptr;
	if( other==pScript ) {
		puts("pScript_push_value reported: 'pScript' cannot be the same ptr as calling pScript!\n");
		return;
	}
	else if( !strcmp(other->m_strName, pScript->m_strName) ) {
		puts("pScript_push_value reported: 'pScript' can't be the same file as calling pScript!\n");
		return;
	}
	*pRetval = TaghaScript_PopValue(other);
}



void Tagha_LoadLibCNatives(struct TaghaVM *pVM)
{
	if( !pVM )
		return;
	
	Tagha_load_stdio_natives(pVM);
	Tagha_load_stdlib_natives(pVM);
}

void Tagha_LoadSelfNatives(struct TaghaVM *pVM)
{
	if( !pVM )
		return;
	
	NativeInfo libc_self_natives[] = {
		{"Debug_PrintSelfMemory", native_dbug_print_mem},
		{"debug_print_self_ptrs", native_dbug_print_ptrs},
		{"debug_print_self_instrs", native_dbug_print_instrs},
		{"TaghaScript_GetGlobalByName", native_get_global_by_name},
		{"Script_get_mem_size", native_get_mem_size},
		{"Script_get_instr_size", native_get_instr_size},
		{"get_Script_from_file", native_get_pScript_from_file},
		{"Script_free", native_pScript_free},
		{"Script_callfunc", native_pScript_callfunc},
		{"Script_push_value", native_pScript_push_value},
		{"Script_pop_value", native_pScript_pop_value},
		{NULL, NULL}
	};
	Tagha_RegisterNatives(pVM, libc_self_natives);
}










