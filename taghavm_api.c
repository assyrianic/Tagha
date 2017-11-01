
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "tagha.h"

static bool is_c_file(const char *filename);
static bool is_tbc_file(const char *filename);
static uint64_t get_file_size(FILE *pFile);
static uint32_t scripthdr_read_natives_table(Script_t **script, FILE **pFile);
static uint32_t scripthdr_read_func_table(Script_t **script, FILE **pFile);
static uint32_t scripthdr_read_global_table(Script_t **script, FILE **pFile);


void Tagha_init(struct TaghaVM *restrict vm)
{
	if( !vm )
		return;
	
	vm->m_pScript = NULL;
	
	vm->m_pmapNatives = malloc(sizeof(Map_t));
	if( !vm->m_pmapNatives )
		printf("[%sTagha Init Error%s]: **** %sUnable to initialize Native Map%s ****\n", KRED, RESET, KGRN, RESET);
	else map_init(vm->m_pmapNatives);
}

static bool is_c_file(const char *filename)
{
	if( !filename )
		return false;
	
	const char *p = filename;
	// iterate to end of string and then check backwards.
	while( *p )
		p++;
	return( (*(p-1)=='c' or *(p-1)=='C') and (*(p-2)=='.') );
}
static bool is_tbc_file(const char *filename)
{
	if( !filename )
		return false;
	
	const char *p = filename;
	// iterate to end of string and then check backwards.
	while( *p )
		p++;
	return( (*(p-3)=='t' and *(p-2)=='b' and *(p-1)=='c') or (*(p-3)=='T' and *(p-2)=='B' and *(p-1)=='C') and *(p-4)=='.' );
}


void Tagha_load_script_by_name(struct TaghaVM *restrict vm, char *restrict filename)
{
	if( !vm )
		return;
	
	// allocate our script.
	vm->m_pScript = TaghaScript_from_file(filename);
}


void Tagha_free(struct TaghaVM *vm)
{
	if( !vm )
		return;
	
	if( vm->m_pScript ) {
		TaghaScript_free(vm->m_pScript), vm->m_pScript=NULL;
	}
	
	// since the VMs native hashmap has nothing allocated,
	// we just free the hashmap's internal data and then the hashmap itself.
	if( vm->m_pmapNatives ) {
		map_free(vm->m_pmapNatives);
		gfree((void **)&vm->m_pmapNatives);
	}
}


bool Tagha_register_natives(struct TaghaVM *restrict vm, struct NativeInfo arrNatives[])
{
	if( !vm or !arrNatives )
		return false;
	else if( !vm->m_pmapNatives )
		return false;
	
	for( uint32_t i=0 ; arrNatives[i].pFunc != NULL ; i++ )
		map_insert(vm->m_pmapNatives, arrNatives[i].strName, (uintptr_t)arrNatives[i].pFunc);
	return true;
}

void Tagha_call_script_func(struct TaghaVM *restrict vm, const char *restrict strFunc)
{
	// We need the VM system in order to call scripts, why?
	// Because the exec function needs the VM system in order to check for native functions.
	if( !vm or !vm->m_pScript or !strFunc )
		return;
	
	struct TaghaScript *script = vm->m_pScript;
	if( !script->m_pmapFuncs ) {
		TaghaScript_PrintErr(script, __func__, "Cannot call functions from host using a NULL function table!");
		return;
	}
	else if( ((script->m_pSP-script->m_pMemory)-16) >= script->m_uiMemsize ) {
		TaghaScript_PrintErr(script, __func__, "stack overflow!");
		return;
	}
	
	struct FuncTable *pFuncTable = (struct FuncTable *)(uintptr_t) map_find(script->m_pmapFuncs, strFunc);
	if( !pFuncTable ) {
		TaghaScript_PrintErr(script, __func__, "Function \'%s\' doesn't exist!", strFunc);
		return;
	}
	
	// save return address.
	script->m_pSP -= 8;
	*(uint64_t *)script->m_pSP = (uintptr_t)script->m_pIP+1;
	
	// jump to the function entry address.
	script->m_pIP = script->m_pText + pFuncTable->m_uiEntry;
	pFuncTable = NULL;
	
	/* Save bp in a separate variable so that we can remember what stack frame we began with.
	 * When the stack frame reverts back to saved bp variable, that means the function ended in the frame it first began.
	 * This is so the VM can appropriately call recursive functions or else
	 * the first 'ret' opcode would kill the entire call process.
	 * Not a big deal for non-recursive function but script function calling should accomodate for all types of functions.
	*/
	uint8_t *oldBP = script->m_pBP;
	
	// push bp and copy sp to bp.
	script->m_pSP -= 8;
	*(uint64_t *)script->m_pSP = (uintptr_t)script->m_pBP;
	script->m_pBP = script->m_pSP;
	
	Tagha_exec(vm, oldBP);
	oldBP = NULL;
	//TaghaScript_debug_print_instrs(script);
}

Script_t *Tagha_get_script(const struct TaghaVM *vm)
{
	return !vm ? NULL : vm->m_pScript;
}

void Tagha_set_script(struct TaghaVM *vm, struct TaghaScript *script)
{
	if( !vm )
		return;
	vm->m_pScript = script;
}

void gfree(void **ptr)
{
	if( *ptr )
		free(*ptr);
	*ptr = NULL;
}










