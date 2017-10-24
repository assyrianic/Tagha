
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



void TaghaScript_push_longfloat(struct TaghaScript *restrict script, const long double val)
{
	if( !script )
		return;
	
	// long doubles are usually 12 or 16 bytes, adjust for both.
	uint32_t size = sizeof(long double);
	if( script->m_bSafeMode and ((script->m_pSP-script->m_pMemory)-size) >= script->m_uiMemsize ) {
		TaghaScript_PrintErr(script, __func__, "stack overflow!");
		return;
	}
	script->m_pSP -= size;
	*(long double *)script->m_pSP = val;
}

long double TaghaScript_pop_longfloat(struct TaghaScript *script)
{
	if( !script )
		return 0;
	
	uint32_t size = sizeof(long double);
	if( script->m_bSafeMode and ((script->m_pSP-script->m_pMemory)+size) >= script->m_uiMemsize ) {
		TaghaScript_PrintErr(script, __func__, "stack underflow!");
		return 0;
	}
	long double val = *(long double *)script->m_pSP;
	script->m_pSP += size;
	return val;
}

void TaghaScript_push_int64(struct TaghaScript *restrict script, const uint64_t val)
{
	if( !script )
		return;
	uint32_t size = sizeof(uint64_t);
	if( script->m_bSafeMode and ((script->m_pSP-script->m_pMemory)-size) >= script->m_uiMemsize ) {
		TaghaScript_PrintErr(script, __func__, "stack overflow!");
		return;
	}
	script->m_pSP -= size;
	*(uint64_t *)script->m_pSP = val;
}
uint64_t TaghaScript_pop_int64(struct TaghaScript *script)
{
	if( !script )
		return 0L;
	uint32_t size = sizeof(uint64_t);
	if( script->m_bSafeMode and ((script->m_pSP-script->m_pMemory)+size) >= script->m_uiMemsize ) {
		TaghaScript_PrintErr(script, __func__, "stack underflow!");
		return 0L;
	}
	uint64_t val = *(uint64_t *)script->m_pSP;
	script->m_pSP += size;
	return val;
}

void TaghaScript_push_double(struct TaghaScript *restrict script, const double val)
{
	if( !script )
		return;
	uint32_t size = sizeof(double);
	if( script->m_bSafeMode and ((script->m_pSP-script->m_pMemory)-size) >= script->m_uiMemsize ) {
		TaghaScript_PrintErr(script, __func__, "stack overflow!");
		return;
	}
	script->m_pSP -= size;
	*(double *)script->m_pSP = val;
}
double TaghaScript_pop_double(struct TaghaScript *script)
{
	if( !script )
		return 0;
	uint32_t size = sizeof(double);
	if( script->m_bSafeMode and ((script->m_pSP-script->m_pMemory)+size) >= script->m_uiMemsize ) {
		TaghaScript_PrintErr(script, __func__, "stack underflow!");
		return 0;
	}
	double val = *(double *)script->m_pSP;
	script->m_pSP += size;
	return val;
}

void TaghaScript_push_int32(struct TaghaScript *restrict script, const uint32_t val)
{
	if( !script )
		return;
	uint32_t size = sizeof(uint32_t);
	if( script->m_bSafeMode and ((script->m_pSP-script->m_pMemory)-size) >= script->m_uiMemsize ) {
		TaghaScript_PrintErr(script, __func__, "stack overflow!");
		return;
	}
	script->m_pSP -= size;
	*(uint32_t *)script->m_pSP = val;
}
uint32_t TaghaScript_pop_int32(struct TaghaScript *script)
{
	if( !script )
		return 0;
	uint32_t size = sizeof(uint32_t);
	if( script->m_bSafeMode and ((script->m_pSP-script->m_pMemory)+size) >= script->m_uiMemsize ) {
		TaghaScript_PrintErr(script, __func__, "stack underflow!");
		return 0;
	}
	uint32_t val = *(uint32_t *)script->m_pSP;
	script->m_pSP += size;
	return val;
}

void TaghaScript_push_float(struct TaghaScript *restrict script, const float val)
{
	if( !script )
		return;
	uint32_t size = sizeof(float);
	if( script->m_bSafeMode and ((script->m_pSP-script->m_pMemory)-size) >= script->m_uiMemsize ) {
		TaghaScript_PrintErr(script, __func__, "stack overflow!");
		return;
	}
	script->m_pSP -= size;
	*(float *)script->m_pSP = val;
}
float TaghaScript_pop_float(struct TaghaScript *script)
{
	if( !script )
		return 0;
	uint32_t size = sizeof(float);
	if( script->m_bSafeMode and ((script->m_pSP-script->m_pMemory)+size) >= script->m_uiMemsize ) {
		TaghaScript_PrintErr(script, __func__, "stack underflow!");
		return 0;
	}
	float val = *(float *)script->m_pSP;
	script->m_pSP += size;
	return val;
}

void TaghaScript_push_short(struct TaghaScript *restrict script, const uint16_t val)
{
	if( !script )
		return;
	uint32_t size = sizeof(uint16_t);
	if( script->m_bSafeMode and ((script->m_pSP-script->m_pMemory)-size) >= script->m_uiMemsize ) {
		TaghaScript_PrintErr(script, __func__, "stack overflow!");
		return;
	}
	script->m_pSP -= size;
	*(uint16_t *)script->m_pSP = val;
}
uint16_t TaghaScript_pop_short(struct TaghaScript *script)
{
	if( !script )
		return 0;
	uint32_t size = sizeof(uint16_t);
	if( script->m_bSafeMode and ((script->m_pSP-script->m_pMemory)+size) >= script->m_uiMemsize ) {
		TaghaScript_PrintErr(script, __func__, "stack underflow!");
		return 0;
	}
	uint16_t val = *(uint16_t *)script->m_pSP;
	script->m_pSP += size;
	return val;
}

void TaghaScript_push_byte(struct TaghaScript *restrict script, const uint8_t val)
{
	if( !script )
		return;
	uint32_t size = sizeof(uint8_t);
	if( script->m_bSafeMode and ((script->m_pSP-script->m_pMemory)-size) >= script->m_uiMemsize ) {
		TaghaScript_PrintErr(script, __func__, "stack overflow!");
		return;
	}
	script->m_pSP -= size;
	*(uint8_t *)script->m_pSP = val;
}
uint8_t TaghaScript_pop_byte(struct TaghaScript *script)
{
	if( !script )
		return 0;
	uint32_t size = sizeof(uint8_t);
	if( script->m_bSafeMode and ((script->m_pSP-script->m_pMemory)+size) >= script->m_uiMemsize ) {
		TaghaScript_PrintErr(script, __func__, "stack underflow!");
		return 0;
	}
	uint8_t val = *(uint8_t *)script->m_pSP;
	script->m_pSP += size;
	return val;
}

void TaghaScript_push_nbytes(struct TaghaScript *restrict script, void *restrict pItem, const uint32_t bytesize)
{
	if( !script )
		return;
	if( script->m_bSafeMode and ((script->m_pSP-script->m_pMemory)-bytesize) >= script->m_uiMemsize ) {
		TaghaScript_PrintErr(script, __func__, "stack overflow!");
		return;
	}
	script->m_pSP -= bytesize;
	memcpy(script->m_pSP, pItem, bytesize);
	/*
	for( uint32_t i=bytesize-1 ; i<bytesize ; i-- )
		script->m_pMemory[--script->m_pSP] = ((uint8_t *)pItem)[i];
	*/
}
void TaghaScript_pop_nbytes(struct TaghaScript *restrict script, void *restrict pBuffer, const uint32_t bytesize)
{
	if( !script )
		return;
	if( script->m_bSafeMode and ((script->m_pSP-script->m_pMemory)+bytesize) >= script->m_uiMemsize ) {
		TaghaScript_PrintErr(script, __func__, "stack underflow!");
		return;
	}
	memcpy(pBuffer, script->m_pSP, bytesize);
	script->m_pSP += bytesize;
	/*
	for( uint32_t i=0 ; i<bytesize ; i++ )
		((uint8_t *)pBuffer)[i] = script->m_pMemory[script->m_pSP++];
	*/
}
/*
uint8_t *TaghaScript_addr2ptr(struct TaghaScript *restrict script, const uint64_t stk_address)
{
	if( !script or !script->m_pMemory )
		return NULL;
	else if( stk_address >= script->m_uiMemsize )
		return NULL;
	return( script->m_pMemory + stk_address );
}
*/
void TaghaScript_call_func_by_name(struct TaghaScript *restrict script, const char *restrict strFunc)
{
	if( !script or !strFunc )
		return;
	else if( !script->m_pmapFuncs )
		return;
	
	struct FuncTable *pFuncs = (struct FuncTable *)(uintptr_t)map_find(script->m_pmapFuncs, strFunc);
	if( pFuncs ) {
		bool debugmode = script->m_bDebugMode;
		uint32_t func_addr = pFuncs->m_uiEntry;
		
		if( debugmode )
			printf("TaghaScript_call_func_by_name :: calling address offset: %" PRIu32 "\n", func_addr);
		
		TaghaScript_push_int64(script, (uintptr_t)script->m_pIP+1);	// save return address.
		if( debugmode )
			printf("TaghaScript_call_func_by_name :: return addr: %p\n", script->m_pIP+1);
		
		script->m_pIP = script->m_pText + func_addr;	// jump to instruction
		
		TaghaScript_push_int64(script, (uintptr_t)script->m_pBP);	// push ebp;
		if( debugmode )
			printf("TaghaScript_call_func_by_name :: pushing bp: %p\n", script->m_pBP);
		script->m_pBP = script->m_pSP;	// mov ebp, esp;
		
		if( debugmode )
			printf("TaghaScript_call_func_by_name :: bp set to sp: %p\n", script->m_pBP);
	}
	pFuncs=NULL;
}
void TaghaScript_call_func_by_addr(struct TaghaScript *script, const uint64_t func_addr)
{
	if( !script )
		return;
	else if( script->m_uiInstrSize >= func_addr )
		return;
	
	TaghaScript_push_int64(script, (uintptr_t)script->m_pIP+1);	// save return address.
	script->m_pIP = script->m_pText + func_addr;	// jump to instruction
	
	TaghaScript_push_int64(script, (uintptr_t)script->m_pBP);	// push ebp;
	script->m_pBP = script->m_pSP;	// mov ebp, esp;
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
	if( !script )
		return;
	else if( !script->m_pMemory )
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
	printf("\' ****\nCurrent Instr Addr: %s%p%s\nCurrent Stack Addr: %s%p%s\n",
		KGRN, script->m_pIP, RESET, KGRN, script->m_pSP, RESET);
}










