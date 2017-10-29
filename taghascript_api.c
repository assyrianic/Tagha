
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "tagha.h"


void TaghaScript_free(struct TaghaScript *script)
{
	if( !script )
		return;
	
	// kill memory
	gfree((void **)&script->m_pMemory);
	
	// kill instruction stream
	gfree((void **)&script->m_pText);
	
	// free our native table
	uint32_t i, Size;
	void *pFree;
	if( script->m_pstrNatives ) {
		for( i=0 ; i<script->m_uiNatives ; i++ ) {
			gfree((void **)&script->m_pstrNatives[i]);
		}
		memset(script->m_pstrNatives, 0, script->m_uiNatives);
		gfree((void **)&script->m_pstrNatives);
	}
	// free our function hashmap and all the tables in it.
	if( script->m_pmapFuncs ) {
		kvnode_t
			*kv = NULL,
			*next = NULL
		;
		Size = script->m_pmapFuncs->size;
		for( i=0 ; i<Size ; i++ ) {
			for( kv = script->m_pmapFuncs->table[i] ; kv ; kv = next ) {
				next = kv->pNext;
				pFree = (void *)(uintptr_t)kv->pData;
				if( pFree )
					free(pFree), pFree=NULL, kv->pData=0;
				
				pFree = (char *)kv->strKey;
				if( pFree )
					free(pFree), pFree=NULL, kv->strKey=NULL;
			}
		}
		memset(script->m_pmapFuncs->table, 0, Size);
		map_free(script->m_pmapFuncs);
		gfree((void **)&script->m_pmapFuncs);
	}
	// free our global var hashmap and all the tables in it.
	if( script->m_pmapGlobals ) {
		kvnode_t
			*kv = NULL,
			*next = NULL
		;
		Size = script->m_pmapGlobals->size;
		for( i=0 ; i<Size ; i++ ) {
			for( kv = script->m_pmapGlobals->table[i] ; kv ; kv = next ) {
				next = kv->pNext;
				pFree = (void *)(uintptr_t)kv->pData;
				if( pFree )
					free(pFree), pFree=NULL, kv->pData=0;
				
				pFree = (char *)kv->strKey;
				if( pFree )
					free(pFree), pFree=NULL, kv->strKey=NULL;
			}
		}
		memset(script->m_pmapGlobals->table, 0, Size);
		map_free(script->m_pmapGlobals);
		gfree((void **)&script->m_pmapGlobals);
	}
	// set our stack pointer pointers to NULL
	// and release the script itself.
	script->m_pIP = script->m_pSP = script->m_pBP = NULL;
	free(script);
}


void TaghaScript_reset(struct TaghaScript *script)
{
	if( !script )
		return;
	// resets the script without, hopefully, crashing Tagha and the host.
	// better than a for-loop setting everything to 0.
	memset(script->m_pMemory, 0, script->m_uiMemsize);
	script->m_pSP = script->m_pBP = script->m_pMemory + (script->m_uiMemsize-1);
	// TODO: reset global variables here as well?
	
}

void *TaghaScript_get_global_by_name(struct TaghaScript *restrict script, const char *restrict strGlobalName)
{
	void *p = NULL;
	if( !script or !script->m_pmapGlobals )
		return p;
	
	struct DataTable *pGlobals = (struct DataTable *)(uintptr_t)map_find(script->m_pmapGlobals, strGlobalName);
	if( pGlobals ) {
		p = script->m_pMemory + pGlobals->m_uiOffset;
		pGlobals=NULL;
	}
	return p;
}


uint32_t TaghaScript_stacksize(const struct TaghaScript *script)
{
	if( !script )
		return 0;
	return script->m_uiMemsize;
}
uint32_t TaghaScript_instrsize(const struct TaghaScript *script)
{
	if( !script )
		return 0;
	return script->m_uiInstrSize;
}
uint32_t TaghaScript_maxinstrs(const struct TaghaScript *script)
{
	if( !script )
		return 0;
	return script->m_uiMaxInstrs;
}
uint32_t TaghaScript_nativecount(const struct TaghaScript *script)
{
	if( !script )
		return 0;
	return script->m_uiNatives;
}
uint32_t TaghaScript_funcs(const struct TaghaScript *script)
{
	if( !script )
		return 0;
	return script->m_uiFuncs;
}
uint32_t TaghaScript_globals(const struct TaghaScript *script)
{
	if( !script )
		return 0;
	return script->m_uiGlobals;
}

bool TaghaScript_safemode_active(const struct TaghaScript *script)
{
	if( !script )
		return false;
	return script->m_bSafeMode;
}
bool TaghaScript_debug_active(const struct TaghaScript *script)
{
	if( !script )
		return false;
	return script->m_bDebugMode;
}



void TaghaScript_debug_print_memory(const struct TaghaScript *script)
{
	if( !script or !script->m_pMemory )
		return;
	
	printf("DEBUG ...---===---... Printing Memory...\n");
	
	uint32_t size = script->m_uiMemsize;
	for( uint32_t i=0 ; i<size ; i++ )
		if( script->m_pSP == script->m_pMemory + i )
			printf("Memory[%.10" PRIu32 "] == %" PRIu32 " - T.O.S.\n", i, script->m_pMemory[i]);
		else printf("Memory[%.10" PRIu32 "] == %" PRIu32 "\n", i, script->m_pMemory[i]);
	printf("\n");
}
void TaghaScript_debug_print_ptrs(const struct TaghaScript *script)
{
	if( !script )
		return;
	
	printf("DEBUG ...---===---... Printing Pointers...\n");
	printf("Instruction Pointer: %p\
			\nStack Pointer: %p\
			\nStack Frame Pointer: %p\n", script->m_pIP, script->m_pSP, script->m_pBP);
	printf("\n");
}
void TaghaScript_debug_print_instrs(const struct TaghaScript *script)
{
	if( !script )
		return;
	
	uint32_t size = script->m_uiInstrSize;
	for( uint32_t i=0 ; i<size ; i++ )
		printf("Instr[%.10" PRIu32 "] == %" PRIu32 "\n", i, script->m_pText[i]);
	printf("\n");
}

void TaghaScript_PrintErr(struct TaghaScript *restrict script, const char *restrict funcname, const char *restrict err, ...)
{
	if( !script or !err )
		return;
	
	va_list args;
	va_start(args, err);
	printf("[%sTagha Error%s]: **** %s reported: \'", KRED, KNRM, funcname);
	vprintf(err, args);
	va_end(args);
	printf("\' ****\nCurrent Instr Addr: %s%p | offset: %" PRIuPTR "%s\nCurrent Stack Addr: %s%p | offset: %" PRIuPTR "%s\n",
		KGRN, script->m_pIP, script->m_pIP-script->m_pText, RESET, KGRN, script->m_pSP, script->m_pSP-script->m_pMemory, RESET);
}










