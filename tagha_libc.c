
#include "tagha.h"
#include "tagha_libc/tagha_stdio.c"
#include "tagha_libc/tagha_stdlib.c"

/* void ScriptDebug_PrintSelfStack(void); */
static void native_dbug_print_stk(struct Tagha *pSys, union CValue params[], union CValue *restrict pRetval, const uint32_t argc)
{
	Tagha_PrintStack(pSys);
}

/* void ScriptDebug_PrintSelfPtrs(void); */
static void native_dbug_print_ptrs(struct Tagha *pSys, union CValue params[], union CValue *restrict pRetval, const uint32_t argc)
{
	Tagha_PrintPtrs(pSys);
}

/* void ScriptDebug_PrintSelfInstrs(void); */
static void native_dbug_print_instrs(struct Tagha *pSys, union CValue params[], union CValue *restrict pRetval, const uint32_t argc)
{
	Tagha_PrintInstrs(pSys);
}

/* struct Tagha *Script_New(void); */
static void native_script_new(struct Tagha *pSys, union CValue params[], union CValue *restrict pRetval, const uint32_t argc)
{
	pRetval->Ptr = Tagha_New();
}

/* void *Script_GetGlobalByName(const Tagha *restrict pScript, const char *restrict str); */
static void native_get_global_by_name(struct Tagha *pSys, union CValue params[], union CValue *restrict pRetval, const uint32_t argc)
{
	struct Tagha *restrict other = params[0].Ptr;
	if( !other ) {
		puts("Script_GetGlobalByName reported: 'pScript' is NULL!\n");
		return;
	}
	else if( other==pSys ) {
		puts("Script_GetGlobalByName reported: 'pScript' can't be the same ptr as calling script!\n");
		return;
	}
	const char *restrict strglobal = params[1].String;
	if( !strglobal ) {
		puts("Script_GetGlobalByName reported: 'str' is NULL!\n");
		return;
	}
	pRetval->Ptr = Tagha_GetGlobalByName(other, strglobal);
}

/* uint32_t Script_GetMemSize(const Tagha *restrict pScript); */
static void native_get_mem_size(struct Tagha *pSys, union CValue params[], union CValue *restrict pRetval, const uint32_t argc)
{
	struct Tagha *restrict other = params[0].Ptr;
	if( !other )
		puts("Script_GetMemSize reported: 'pScript' is NULL!\n");
	pRetval->UInt32 = Tagha_GetMemSize(other);
}

/* uint32_t Script_GetInstrSize(const Tagha *restrict pScript); */
static void native_get_instr_size(struct Tagha *pSys, union CValue params[], union CValue *restrict pRetval, const uint32_t argc)
{
	struct Tagha *restrict other = params[0].Ptr;
	if( !other )
		puts("Script_GetInstrSize reported: 'pScript' is NULL!\n");
	pRetval->UInt32 = Tagha_GetInstrSize(other);
}

/* void Script_LoadScriptByName(struct Tagha *restrict pScript, char *restrict filename); */
static void native_get_script_from_file(struct Tagha *pSys, union CValue params[], union CValue *restrict pRetval, const uint32_t argc)
{
	struct Tagha *restrict pScript = params[0].Ptr;
	if( !pScript )
		puts("Script_LoadScriptByName reported: 'pScript' is NULL!\n");
	
	char *restrict filename = params[1].Str;
	if( !filename )
		puts("Script_LoadScriptByName reported: 'filename' is NULL!\n");
	Tagha_LoadScriptByName(pScript, filename);
	filename = NULL, pScript = NULL;
}

/* void	Script_Free(struct Tagha *pScript); */
static void native_script_free(struct Tagha *pSys, union CValue params[], union CValue *restrict pRetval, const uint32_t argc)
{
	struct Tagha *restrict other = params[0].Ptr;
	if( other==pSys ) {
		puts("Script_Free reported: 'pScript' cannot be the same ptr as calling script!\n");
		return;
	}
	Tagha_Free(other);
}

/* int32_t Script_CallFunc(struct Tagha *restrict pScript, const char *restrict strFunc); */
static void native_script_callfunc(struct Tagha *pSys, union CValue params[], union CValue *restrict pRetval, const uint32_t argc)
{
	pRetval->Int32 = -1;
	struct Tagha *other = params[0].Ptr;
	if( other==pSys ) {
		puts("Script_CallFunc reported: 'pScript' cannot be the same ptr as calling script!\n");
		return;
	}
	const char *strFunc = params[1].String;
	if( !strFunc ) {
		puts("Script_CallFunc reported: 'strFunc' is NULL!\n");
		return;
	}
	pRetval->Int32 = Tagha_CallFunc(other, strFunc);
}

/* void Script_PushValues(struct Tagha *pScript, const uint32_t uiArgs, union CValue values[]); */
static void native_script_push_values(struct Tagha *pSys, union CValue params[], union CValue *restrict pRetval, const uint32_t argc)
{
	struct Tagha *restrict other = params[0].Ptr;
	if( other==pSys ) {
		puts("Script_PushValue reported: 'pScript' cannot be the same ptr as calling script!\n");
		return;
	}
	Tagha_PushValues(other, params[1].UInt32, params[2].SelfPtr);
}

/* union CValue Script_PopValue(struct Tagha *pScript); */
static void native_script_pop_value(struct Tagha *pSys, union CValue params[], union CValue *restrict pRetval, const uint32_t argc)
{
	struct Tagha *restrict other = params[0].Ptr;
	if( other==pSys ) {
		puts("Script_PopValue reported: 'pScript' cannot be the same ptr as calling script!\n");
		return;
	}
	*pRetval = Tagha_PopValue(other);
}



bool Tagha_LibC_Init(struct Tagha *pSys)
{
	bool res=false;
	if( !pSys )
		return res;
	
	res=Tagha_Load_stdioNatives(pSys);
	if( !res )
		return res;
	
	res=Tagha_Load_stdlibNatives(pSys);
	return res;
}

bool Tagha_SelfNativesInit(struct Tagha *pSys)
{
	if( !pSys )
		return false;
	
	struct NativeInfo libc_self_natives[] = {
		{"ScriptDebug_PrintSelfStack", native_dbug_print_stk},
		{"ScriptDebug_PrintSelfPtrs", native_dbug_print_ptrs},
		{"ScriptDebug_PrintSelfInstrs", native_dbug_print_instrs},
		{"Script_New", native_script_new},
		{"Script_GetGlobalByName", native_get_global_by_name},
		{"Script_GetMemSize", native_get_mem_size},
		{"Script_GetInstrSize", native_get_instr_size},
		{"Script_LoadScriptByName", native_get_script_from_file},
		{"Script_Free", native_script_free},
		{"Script_CallFunc", native_script_callfunc},
		{"Script_PushValues", native_script_push_values},
		{"Script_PopValue", native_script_pop_value},
		{NULL, NULL}
	};
	return Tagha_RegisterNatives(pSys, libc_self_natives);
}










