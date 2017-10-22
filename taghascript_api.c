
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
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
	free(script);
}


void TaghaScript_reset(struct TaghaScript *script)
{
	if( !script )
		return;
	// resets the script without, hopefully, crashing Tagha and the host.
	// better than a for-loop setting everything to 0.
	memset(script->m_pMemory, 0, script->m_uiMemsize);
	script->sp = script->bp = script->m_uiMemsize-1;
	// TODO: reset global variables here as well!
	
}



void TaghaScript_push_longfloat(struct TaghaScript *restrict script, const long double val)
{
	if( !script )
		return;
	
	// long doubles are usually 12 or 16 bytes, adjust for both.
	uint32_t size = sizeof(long double);
	if( script->m_bSafeMode and (script->sp-size) >= script->m_uiMemsize ) {
		printf("TaghaScript_push_longfloat reported: stack overflow! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\n", script->ip, script->sp);
		return;
	}
	script->sp -= size;
	*(long double *)(script->m_pMemory + script->sp) = val;
}

long double TaghaScript_pop_longfloat(struct TaghaScript *script)
{
	if( !script )
		return 0;
	
	uint32_t size = sizeof(long double);
	if( script->m_bSafeMode and (script->sp+size) >= script->m_uiMemsize ) {
		printf("TaghaScript_pop_longfloat reported: stack underflow! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\n", script->ip, script->sp);
		return 0;
	}
	long double val = *(long double *)(script->m_pMemory + script->sp);
	script->sp += size;
	return val;
}

void TaghaScript_push_int64(struct TaghaScript *restrict script, const uint64_t val)
{
	if( !script )
		return;
	uint32_t size = sizeof(uint64_t);
	if( script->m_bSafeMode and (script->sp-size) >= script->m_uiMemsize ) {
		printf("TaghaScript_push_int64 reported: stack overflow! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\n", script->ip, script->sp);
		return;
	}
	script->sp -= size;
	*(uint64_t *)(script->m_pMemory + script->sp) = val;
}
uint64_t TaghaScript_pop_int64(struct TaghaScript *script)
{
	if( !script )
		return 0L;
	uint32_t size = sizeof(uint64_t);
	if( script->m_bSafeMode and (script->sp+size) >= script->m_uiMemsize ) {
		printf("TaghaScript_pop_int64 reported: stack underflow! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\n", script->ip, script->sp);
		return 0L;
	}
	uint64_t val = *(uint64_t *)(script->m_pMemory + script->sp);
	script->sp += size;
	return val;
}

void TaghaScript_push_double(struct TaghaScript *restrict script, const double val)
{
	if( !script )
		return;
	uint32_t size = sizeof(double);
	if( script->m_bSafeMode and (script->sp-size) >= script->m_uiMemsize ) {
		printf("TaghaScript_push_double reported: stack overflow! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\n", script->ip, script->sp);
		return;
	}
	script->sp -= size;
	*(double *)(script->m_pMemory + script->sp) = val;
}
double TaghaScript_pop_double(struct TaghaScript *script)
{
	if( !script )
		return 0;
	uint32_t size = sizeof(double);
	if( script->m_bSafeMode and (script->sp+size) >= script->m_uiMemsize ) {
		printf("TaghaScript_pop_double reported: stack underflow! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\n", script->ip, script->sp);
		return 0;
	}
	double val = *(double *)(script->m_pMemory + script->sp);
	script->sp += size;
	return val;
}

void TaghaScript_push_int32(struct TaghaScript *restrict script, const uint32_t val)
{
	if( !script )
		return;
	uint32_t size = sizeof(uint32_t);
	if( script->m_bSafeMode and (script->sp-size) >= script->m_uiMemsize ) {
		printf("TaghaScript_push_int32 reported: stack overflow! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\n", script->ip, script->sp);
		return;
	}
	script->sp -= size;
	*(uint32_t *)(script->m_pMemory + script->sp) = val;
}
uint32_t TaghaScript_pop_int32(struct TaghaScript *script)
{
	if( !script )
		return 0;
	uint32_t size = sizeof(uint32_t);
	if( script->m_bSafeMode and (script->sp+size) >= script->m_uiMemsize ) {
		printf("TaghaScript_pop_int32 reported: stack underflow! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\n", script->ip, script->sp);
		return 0;
	}
	uint32_t val = *(uint32_t *)(script->m_pMemory + script->sp);
	script->sp += size;
	return val;
}

void TaghaScript_push_float(struct TaghaScript *restrict script, const float val)
{
	if( !script )
		return;
	uint32_t size = sizeof(float);
	if( script->m_bSafeMode and (script->sp-size) >= script->m_uiMemsize ) {
		printf("TaghaScript_push_float reported: stack overflow! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\n", script->ip, script->sp);
		return;
	}
	script->sp -= size;
	*(float *)(script->m_pMemory + script->sp) = val;
}
float TaghaScript_pop_float(struct TaghaScript *script)
{
	if( !script )
		return 0;
	uint32_t size = sizeof(float);
	if( script->m_bSafeMode and (script->sp+size) >= script->m_uiMemsize ) {
		printf("TaghaScript_pop_float reported: stack underflow! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\n", script->ip, script->sp);
		return 0;
	}
	float val = *(float *)(script->m_pMemory + script->sp);
	script->sp += size;
	return val;
}

void TaghaScript_push_short(struct TaghaScript *restrict script, const uint16_t val)
{
	if( !script )
		return;
	uint32_t size = sizeof(uint16_t);
	if( script->m_bSafeMode and (script->sp-size) >= script->m_uiMemsize ) {
		printf("TaghaScript_push_short reported: stack overflow! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\n", script->ip, script->sp);
		return;
	}
	script->sp -= size;
	*(uint16_t *)(script->m_pMemory + script->sp) = val;
}
uint16_t TaghaScript_pop_short(struct TaghaScript *script)
{
	if( !script )
		return 0;
	uint32_t size = sizeof(uint16_t);
	if( script->m_bSafeMode and (script->sp+size) >= script->m_uiMemsize ) {
		printf("TaghaScript_pop_short reported: stack underflow! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\n", script->ip, script->sp);
		return 0;
	}
	uint16_t val = *(uint16_t *)(script->m_pMemory + script->sp);
	script->sp += size;
	return val;
}

void TaghaScript_push_byte(struct TaghaScript *restrict script, const uint8_t val)
{
	if( !script )
		return;
	uint32_t size = sizeof(uint8_t);
	if( script->m_bSafeMode and (script->sp-size) >= script->m_uiMemsize ) {
		printf("TaghaScript_push_byte reported: stack overflow! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\n", script->ip, script->sp);
		return;
	}
	script->sp -= size;
	*(uint8_t *)(script->m_pMemory + script->sp) = val;
}
uint8_t TaghaScript_pop_byte(struct TaghaScript *script)
{
	if( !script )
		return 0;
	uint32_t size = sizeof(uint8_t);
	if( script->m_bSafeMode and (script->sp+size) >= script->m_uiMemsize ) {
		printf("TaghaScript_pop_byte reported: stack underflow! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\n", script->ip, script->sp);
		return 0;
	}
	uint8_t val = *(uint8_t *)(script->m_pMemory + script->sp);
	script->sp += size;
	return val;
}

void TaghaScript_push_nbytes(struct TaghaScript *restrict script, void *restrict pItem, const uint32_t bytesize)
{
	if( !script )
		return;
	if( script->m_bSafeMode and (script->sp-bytesize) >= script->m_uiMemsize ) {
		printf("TaghaScript_push_nbytes reported: stack overflow! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\n", script->ip, script->sp);
		return;
	}
	script->sp -= bytesize;
	memcpy(script->m_pMemory + script->sp, pItem, bytesize);
	/*
	for( uint32_t i=bytesize-1 ; i<bytesize ; i-- )
		script->m_pMemory[--script->sp] = ((uint8_t *)pItem)[i];
	*/
}
void TaghaScript_pop_nbytes(struct TaghaScript *restrict script, void *restrict pBuffer, const uint32_t bytesize)
{
	if( !script )
		return;
	if( script->m_bSafeMode and (script->sp+bytesize) >= script->m_uiMemsize ) {
		printf("TaghaScript_pop_nbytes reported: stack underflow! Current instruction address: %" PRIu64 " | Stack index: %" PRIu64 "\n", script->ip, script->sp);
		return;
	}
	memcpy(pBuffer, script->m_pMemory + script->sp, bytesize);
	script->sp += bytesize;
	/*
	for( uint32_t i=0 ; i<bytesize ; i++ )
		((uint8_t *)pBuffer)[i] = script->m_pMemory[script->sp++];
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
			printf("TaghaScript_call_func_by_name :: calling address: %" PRIu32 "\n", func_addr);
		
		TaghaScript_push_int64(script, script->ip+1);	// save return address.
		if( debugmode )
			printf("TaghaScript_call_func_by_name :: return addr: %" PRIu64 "\n", script->ip+1);
		
		script->ip = func_addr;	// jump to instruction
		
		TaghaScript_push_int64(script, script->bp);	// push ebp;
		if( debugmode )
			printf("TaghaScript_call_func_by_name :: pushing bp: %" PRIu64 "\n", script->bp);
		script->bp = script->sp;	// mov ebp, esp;
		
		if( debugmode )
			printf("TaghaScript_call_func_by_name :: bp set to sp: %" PRIu64 "\n", script->bp);
	}
	pFuncs=NULL;
}
void TaghaScript_call_func_by_addr(struct TaghaScript *script, const uint64_t func_addr)
{
	if( !script )
		return;
	else if( script->m_uiInstrSize >= func_addr )
		return;
	
	TaghaScript_push_int64(script, script->ip+1);	// save return address.
	script->ip = func_addr;	// jump to instruction
	
	TaghaScript_push_int64(script, script->bp);	// push ebp;
	script->bp = script->sp;	// mov ebp, esp;
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
		if( script->sp==i )
			printf("Memory[%.10" PRIu32 "] == %" PRIu32 " - T.O.S.\n", i, script->m_pMemory[i]);
		else printf("Memory[%.10" PRIu32 "] == %" PRIu32 "\n", i, script->m_pMemory[i]);
	printf("\n");
}
void TaghaScript_debug_print_ptrs(const struct TaghaScript *script)
{
	if( !script )
		return;
	
	printf("DEBUG ...---===---... Printing Pointers...\n");
	printf("Instruction Pointer: %" PRIu64 "\
			\nStack Pointer: %" PRIu64 "\
			\nStack Frame Pointer: %" PRIu64 "\n", script->ip, script->sp, script->bp);
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










