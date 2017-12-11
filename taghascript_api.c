
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "tagha.h"


/* Tagha File Structure (Dec 10, 2017)
 * 
 * 2 bytes: magic verifier ==> 0xC0DE
 * 4 bytes: stack segment size (aligned by 8 bytes)
 * 4 bytes: data segment size (aligned by 8 bytes)
 * 4 bytes: text segment size (never align or else jumps will be incorrect)
 * 
 * 4 bytes: amount of natives
 * n bytes: native table
 * 		4 bytes: string size + '\0' of native string
 * 		n bytes: native string.
 * 
 * 4 bytes: amount of functions
 * n bytes: functions table
 * 		4 bytes: string size + '\0' of func string
 * 		n bytes: function string
 * 		4 bytes: offset
 * 
 * 4 bytes: amount of global vars
 * n bytes: global vars table
 * 		4 bytes: string size + '\0' of global var string
 * 		n bytes: global var string
 * 		4 bytes: offset
 * 		4 bytes: size in bytes
 * 1 byte: safemode and debugmode flags
 * n bytes: instruction stream
 */

static uint64_t get_file_size(FILE *pFile);
static uint32_t scripthdr_read_natives_table(TaghaScript **script, FILE **pFile);
static uint32_t scripthdr_read_func_table(TaghaScript **script, FILE **pFile);
static uint32_t scripthdr_read_global_table(TaghaScript **script, FILE **pFile);


// need this to determine the text segment size.
static uint64_t get_file_size(FILE *pFile)
{
	uint64_t size = 0L;
	if( !pFile )
		return size;
	
	if( !fseek(pFile, 0, SEEK_END) ) {
		size = (uint64_t)ftell(pFile);
		rewind(pFile);
	}
	return size;
}

static uint32_t scripthdr_read_natives_table(TaghaScript **script, FILE **pFile)
{
	if( !*script or !*pFile )
		return 0;
	
	TaghaScript	*pScript = *script;
	uint32_t	bytesread = 0;
	int32_t		ignores = 0;
	
	// see if the script is using any natives.
	ignores = fread(&pScript->m_uiNatives, sizeof(uint32_t), 1, *pFile);
	bytesread += sizeof(uint32_t);
	if( !pScript->m_uiNatives )
		return bytesread;
	
	// script has natives? Copy their names so we can use them on VM natives hashmap later.
	pScript->m_pstrNativeCalls = calloc(pScript->m_uiNatives, sizeof(char *));
	if( !pScript->m_pstrNativeCalls ) {
		printf("[%sTagha Load Script Error%s]: **** %sFailed to allocate memory for Native Table%s ****\n", KRED, RESET, KGRN, RESET);
		TaghaScript_Free(pScript), *script = NULL;
		fclose(*pFile), *pFile=NULL;
		return 0;
	}
	
	for( uint32_t i=0 ; i<pScript->m_uiNatives ; i++ ) {
		uint32_t str_size;
		ignores = fread(&str_size, sizeof(uint32_t), 1, *pFile);
		bytesread += sizeof(uint32_t);
		
		// allocate memory to hold the native's name.
		pScript->m_pstrNativeCalls[i] = calloc(str_size, sizeof(char));
		if( !pScript->m_pstrNativeCalls[i] ) {
			printf("[%sTagha Load Script Error%s]: **** %sFailed to allocate memory for Native String%s ****\n", KRED, RESET, KGRN, RESET);
			TaghaScript_Free(pScript), *script = NULL;
			fclose(*pFile), *pFile=NULL;
			return 0;
		}
		
		// read in the native's name.
		ignores = fread(pScript->m_pstrNativeCalls[i], sizeof(char), str_size, *pFile);
		bytesread += str_size;
		printf("[Tagha Load Script] :: copied native name \'%s\'\n", pScript->m_pstrNativeCalls[i]);
	}
	pScript = NULL;
	return bytesread;
}

static uint32_t scripthdr_read_func_table(TaghaScript **script, FILE **pFile)
{
	if( !*script or !*pFile )
		return 0;
	
	TaghaScript	*pScript = *script;
	uint32_t	bytecount = 0;
	int32_t		ignore_warns = 0;
	
	// see if the script has its own functions.
	// This table is so host or other scripts can call these functions by name or address.
	ignore_warns = fread(&pScript->m_uiFuncs, sizeof(uint32_t), 1, *pFile);
	bytecount += sizeof(uint32_t);
	pScript->m_pmapFuncs = NULL;
	if( !pScript->m_uiFuncs )
		return bytecount;
	
	// allocate our function hashmap so we can call functions by name.
	pScript->m_pmapFuncs = calloc(1, sizeof(struct hashmap));
	if( !pScript->m_pmapFuncs ) {
		printf("[%sTagha Load Script Error%s]: **** %sFailed to allocate memory for Func Hashmap%s ****\n", KRED, RESET, KGRN, RESET);
		TaghaScript_Free(pScript), *script = NULL;
		fclose(*pFile), *pFile=NULL;
		return 0;
	}
	map_init(pScript->m_pmapFuncs);
	
	// copy the function data from the header.
	for( uint32_t i=0 ; i<pScript->m_uiFuncs ; i++ ) {
		uint32_t str_size;
		ignore_warns = fread(&str_size, sizeof(uint32_t), 1, *pFile);
		bytecount += sizeof(uint32_t);
		
		// allocate the hashmap function key.
		char *strFunc = calloc(str_size, sizeof(char));
		if( !strFunc ) {
			printf("[%sTagha Load Script Error%s]: **** %sFailed to allocate memory for Func Table String%s ****\n", KRED, RESET, KGRN, RESET);
			TaghaScript_Free(pScript), *script = NULL;
			fclose(*pFile), *pFile=NULL;
			return 0;
		}
		ignore_warns = fread(strFunc, sizeof(char), str_size, *pFile);
		bytecount += str_size;
		
		// allocate function's data to a table.
		struct FuncTable *pFuncData = calloc(1, sizeof(struct FuncTable));
		if( !pFuncData ) {
			printf("[%sTagha Load Script Error%s]: **** %sFailed to allocate memory for Func Table Data%s ****\n", KRED, RESET, KGRN, RESET);
			TaghaScript_Free(pScript), *script = NULL;
			fclose(*pFile), *pFile=NULL;
			return 0;
		}
		// copy func's header data to our table
		// then store the table to our function hashmap with the key
		// we allocated earlier.
		ignore_warns = fread(&pFuncData->m_uiEntry, sizeof(uint32_t), 1, *pFile);
		bytecount += sizeof(uint32_t);
		printf("[Tagha Load Script] :: Copied Function name \'%s\' | offset: %" PRIu32 "\n", strFunc, pFuncData->m_uiEntry);
		map_insert(pScript->m_pmapFuncs, strFunc, (uintptr_t)pFuncData);
		strFunc = NULL, pFuncData = NULL;
	} /* for( uint32_t i=0 ; i<pScript->m_uiFuncs ; i++ ) */
	pScript = NULL;
	return bytecount;
}

static uint32_t scripthdr_read_global_table(TaghaScript **script, FILE **pFile)
{
	if( !*script or !*pFile )
		return 0;
	
	TaghaScript	*pScript = *script;
	uint32_t	bytecount = 0;
	int32_t		ignore_warns = 0;
	
	// check if the script has global variables.
	ignore_warns = fread(&pScript->m_uiGlobals, sizeof(uint32_t), 1, *pFile);
	printf("[Tagha Load Script] :: Amount of Global Vars: %" PRIu32 "\n", pScript->m_uiGlobals);
	bytecount += sizeof(uint32_t);
	pScript->m_pmapGlobals = NULL;
	uint32_t globalbytes = 0;
	if( !pScript->m_uiGlobals )
		return bytecount;
	
	// script has globals, allocate our global var hashmap.
	pScript->m_pmapGlobals = calloc(1, sizeof(struct hashmap));
	if( !pScript->m_pmapGlobals ) {
		printf("[%sTagha Load Script Error%s]: **** %sFailed to allocate memory for Global Var Hashmap%s ****\n", KRED, RESET, KGRN, RESET);
		TaghaScript_Free(pScript), *script = NULL;
		fclose(*pFile), *pFile=NULL;
		return 0;
	}
	map_init(pScript->m_pmapGlobals);
	
	for( uint32_t i=0 ; i<pScript->m_uiGlobals ; i++ ) {
		uint32_t str_size;
		ignore_warns = fread(&str_size, sizeof(uint32_t), 1, *pFile);
		bytecount += sizeof(uint32_t);
		// allocate string to use as a key for our global var.
		char *strGlobal = calloc(str_size, sizeof(char));
		if( !strGlobal ) {
			printf("[%sTagha Load Script Error%s]: **** %sFailed to allocate memory for Global Var String%s ****\n", KRED, RESET, KGRN, RESET);
			TaghaScript_Free(pScript), *script = NULL;
			fclose(*pFile), *pFile=NULL;
			return 0;
		}
		ignore_warns = fread(strGlobal, sizeof(char), str_size, *pFile);
		bytecount += str_size;
		
		// allocate table for our global var's data.
		struct DataTable *pGlobalData = calloc(1, sizeof(struct DataTable));
		if( !pGlobalData ) {
			printf("[%sTagha Load Script Error%s]: **** %sFailed to allocate memory for Global Var Table Data%s ****\n", KRED, RESET, KGRN, RESET);
			TaghaScript_Free(pScript), *script = NULL;
			fclose(*pFile), *pFile=NULL;
			return 0;
		}
		
		// read the global var's data from the header and add it to our hashmap.
		// same procedure as our function hashmap.
		ignore_warns = fread(&pGlobalData->m_uiOffset, sizeof(uint32_t), 1, *pFile);
		bytecount += sizeof(uint32_t);
		ignore_warns = fread(&pGlobalData->m_uiBytes, sizeof(uint32_t), 1, *pFile);
		bytecount += sizeof(uint32_t);
		
		printf("[Tagha Load Script] :: copied global var's name: \'%s\' | offset: %" PRIu32 "\n", strGlobal, pGlobalData->m_uiOffset);
		
		// insert the global var's table to our hashmap.
		map_insert(pScript->m_pmapGlobals, strGlobal, (uintptr_t)pGlobalData);
		strGlobal = NULL, pGlobalData = NULL;
	} /* for( uint32_t i=0 ; i<script->m_uiGlobals ; i++ ) */
	pTemp = NULL, pScript = NULL;
	return bytecount;
}

struct TaghaScript *TaghaScript_BuildFromFile(const char *restrict filename)
{
	if( !filename )
		return NULL;
	
	// open up our script in read-only binary mode.
	FILE *pFile = fopen(filename, "rb");
	if( !pFile ) {
		printf("[%sTagha Load Script Error%s]: **** %sFile not found: \'%s\'%s ****\n", KRED, RESET, KGRN, filename, RESET);
		return NULL;
	}
	
	struct TaghaScript *script = calloc(1, sizeof(struct TaghaScript));
	if( !script )
		return NULL;
	
	strncpy(script->m_strName, filename, 64);
	
	uint64_t filesize = get_file_size(pFile);
	uint32_t bytecount = 0;	// bytecount is for separating the header data from the actual instruction stream.
	uint16_t verify;
	int32_t ignore_warns;
	ignore_warns = fread(&verify, sizeof(uint16_t), 1, pFile);
	bytecount += sizeof(uint16_t);
	
	if( verify != 0xC0DE ) {
		printf("[%sTagha Load Script Error%s]: **** %sUnknown or Invalid script file format%s ****\n", KRED, RESET, KGRN, RESET);
		goto error;
	}
	printf("[Tagha Load Script] :: Verified script!\n");
	
	
	// get stack memory size.
	uint32_t stacksize, datasize;
	ignore_warns = fread(&stacksize, sizeof(uint32_t), 1, pFile);
	bytecount += sizeof(uint32_t);
	stacksize = stacksize + 7 & -8;	// align by 8 bytes
	printf("[Tagha Load Script] :: Stack Size: %" PRIu32 "\n", stacksize);
	
	// get static data (global vars, string literals) memory size.
	ignore_warns = fread(&datasize, sizeof(uint32_t), 1, pFile);
	bytecount += sizeof(uint32_t);
	datasize = datasize + 7 & -8;	// align by 8 bytes
	printf("[Tagha Load Script] :: Data Size: %" PRIu32 "\n", datasize);
	
	bytecount += scripthdr_read_natives_table(&script, &pFile);
	bytecount += scripthdr_read_func_table(&script, &pFile);
	if( !pFile )
		return NULL;
	bytecount += scripthdr_read_global_table(&script, &pFile);
	if( !pFile )
		return NULL;
	
	
	// check if the script is either in safemode or debug mode.
	char modeflags;
	ignore_warns = fread(&modeflags, sizeof(bool), 1, pFile);
	bytecount += sizeof(bool);
	script->m_bSafeMode = (modeflags & 1) >= 1;
	printf("[Tagha Load Script] :: Script Safe Mode: %" PRIu32 "\n", script->m_bSafeMode);
	
	script->m_bDebugMode = (modeflags & 2) >= 1;
	printf("[Tagha Load Script] :: Script Debug Mode: %" PRIu32 "\n", script->m_bDebugMode);
	
	script->m_uiMaxInstrs = 0xffff;	// helps to stop infinite/runaway loops
	
	// header data is finished, subtract the filesize with the bytecount
	// to get the size of our instruction stream.
	// If the instruction stream is invalid, we can't load script.
	printf("[Tagha Load Script] :: Header Byte Count: %" PRIu32 "\n", bytecount);
	script->m_uiInstrSize = textsize = filesize - bytecount;
	
	script->m_uiMemsize = stacksize + datasize + script->m_uiInstrSize;
	script->m_pMemory = calloc(script->m_uiMemsize, sizeof(uint8_t));
	
	// scripts NEED memory, if memory is invalid then we can't use the script.
	if( !script->m_pMemory ) {
		printf("[%sTagha Load Script Error%s]: **** %sFailed to allocate memory for script%s ****\n", KRED, RESET, KGRN, RESET);
		TaghaScript_Free(script), script = NULL;
		goto error;
	}
	
	// set up the stack ptrs to the top of the stack.
	script->m_Regs[rsp].UCharPtr = script->m_Regs[rbp].UCharPtr = script->m_pMemory + (script->m_uiMemsize-1);
	
	// set up our stack segment by taking the top of the stack and subtracting it with the size.
	script->m_pStackSegment = script->m_Regs[rsp].UCharPtr - stacksize;
	
	// set up the data segment by taking the stack segment - 1
	script->m_pDataSegment = script->m_pStackSegment - 1;
	
	// set up the text segment similar to how the stack ptrs are set up
	script->m_pTextSegment = script->m_pMemory + (script->m_uiInstrSize-1);
	
	// instruction stream is valid, copy every last instruction and data to the last byte.
	ignore_warns = fread(script->m_pMemory, sizeof(uint8_t), script->m_uiInstrSize, pFile);
	
	// set our entry point to 'main', IF it exists.
	struct FuncTable *pFuncTable = (struct FuncTable *)(uintptr_t) map_find(script->m_pmapFuncs, "main");
	script->m_Regs[rip].UCharPtr = (pFuncTable != NULL) ? script->m_pMemory + pFuncTable->m_uiEntry : NULL;
	
	// we're done with the file, close it up.
	fclose(pFile), pFile=NULL;
	printf("\n");
	return script;
error:;
	fclose(pFile), pFile=NULL;
	return NULL;
}

void TaghaScript_Free(struct TaghaScript *script)
{
	if( !script )
		return;
	
	// kill memory
	gfree((void **)&script->m_pMemory);
	
	// free our native table
	uint32_t i, Size;
	if( script->m_pstrNativeCalls ) {
		for( i=0 ; i<script->m_uiNatives ; i++ ) {
			gfree((void **)&script->m_pstrNativeCalls[i]);
		}
		memset(script->m_pstrNativeCalls, 0, script->m_uiNatives);
		gfree((void **)&script->m_pstrNativeCalls);
	}
	// free our function hashmap and all the tables in it.
	if( script->m_pmapFuncs ) {
		struct kvnode
			*kv = NULL,
			*next = NULL
		;
		Size = script->m_pmapFuncs->size;
		struct FuncTable *pfreeData;
		for( i=0 ; i<Size ; i++ ) {
			for( kv = script->m_pmapFuncs->table[i] ; kv ; kv = next ) {
				next = kv->pNext;
				pfreeData = (struct FuncTable *)(uintptr_t)kv->pData;
				if( pfreeData )
					free(pfreeData), kv->pData=0;
				
				if( kv->strKey )
					free((char *)kv->strKey), kv->strKey=NULL;
			}
		}
		pfreeData = NULL;
		map_free(script->m_pmapFuncs);
		gfree((void **)&script->m_pmapFuncs);
	}
	// free our global var hashmap and all the tables in it.
	if( script->m_pmapGlobals ) {
		struct kvnode
			*kv = NULL,
			*next = NULL
		;
		struct DataTable *pfreeData;
		Size = script->m_pmapGlobals->size;
		for( i=0 ; i<Size ; i++ ) {
			for( kv = script->m_pmapGlobals->table[i] ; kv ; kv = next ) {
				next = kv->pNext;
				pfreeData = (struct DataTable *)(uintptr_t)kv->pData;
				if( pfreeData )
					free(pfreeData), kv->pData=0;
				
				if( kv->strKey )
					free((char *)kv->strKey), kv->strKey=NULL;
			}
		}
		pfreeData = NULL;
		map_free(script->m_pmapGlobals);
		gfree((void **)&script->m_pmapGlobals);
	}
	// set our stack pointer pointers to NULL
	// and release the script itself.
	script->m_Regs[rip].UCharPtr = script->m_Regs[rsp].UCharPtr = script->m_Regs[rbp].UCharPtr = NULL;
	free(script);
}


void TaghaScript_Reset(struct TaghaScript *script)
{
	if( !script )
		return;
	// resets the script without, hopefully, crashing Tagha and the host.
	// better than a for-loop setting everything to 0.
	memset(script->m_pMemory, 0, script->m_uiMemsize);
	script->m_Regs[rsp].UCharPtr = script->m_Regs[rbp].UCharPtr = script->m_pMemory + (script->m_uiMemsize-1);
	
	memset(script->m_Regs, 0, sizeof(union CValue) * rsp);
	// TODO: reset global variables here as well?
}


void *TaghaScript_GetGlobalByName(struct TaghaScript *restrict script, const char *restrict strGlobalName)
{
	void *p = NULL;
	if( !script or !script->m_pmapGlobals )
		return p;
	
	struct DataTable *pGlobals = (struct DataTable *)(uintptr_t) map_find(script->m_pmapGlobals, strGlobalName);
	if( pGlobals ) {
		p = script->m_pMemory + pGlobals->m_uiOffset;
		pGlobals=NULL;
	}
	return p;
}
bool TaghaScript_BindGlobalPtr(struct TaghaScript *restrict script, const char *restrict strGlobalName, void *restrict pVar)
{
	if( !script or !script->m_pmapGlobals or !strGlobalName )
		return false;
	
	struct DataTable *pGlobals = (struct DataTable *)(uintptr_t) map_find(script->m_pmapGlobals, strGlobalName);
	if( pGlobals ) {
		*(uint64_t *)(script->m_pMemory + pGlobals->m_uiOffset) = (uintptr_t)pVar;
		if( script->m_bDebugMode )
			printf("set offset: %u to %s: %p\n", pGlobals->m_uiOffset, strGlobalName, pVar);
		return true;
	}
	return false;
}


void TaghaScript_PushValue(struct TaghaScript *script, const union CValue value)
{
	if( !script or !script->m_pMemory )
		return;
	if( script->m_bSafeMode and ((script->m_Regs[rsp].UCharPtr-script->m_pMemory)-8) >= script->m_uiMemsize ) {
		TaghaScript_PrintErr(script, __func__, "stack overflow!");
		return;
	}
	(--script->m_Regs[rsp].SelfPtr)->UInt64 = 0;
	*script->m_Regs[rsp].SelfPtr = value;
}

union CValue TaghaScript_PopValue(struct TaghaScript *script)
{
	CValue val={ .UInt64=0L };
	if( !script or !script->m_pMemory ) {
		printf("[%sTaghaScript Pop%s]: **** %sScript is NULL%s ****\n", KRED, RESET, KGRN, RESET);
		return val;
	}
	return script->m_Regs[ras];
}

uint32_t TaghaScript_GetMemSize(const struct TaghaScript *script)
{
	return !script ? 0 : script->m_uiMemsize;
}
uint32_t TaghaScript_GetInstrSize(const struct TaghaScript *script)
{
	return !script ? 0 : script->m_uiInstrSize;
}
uint32_t TaghaScript_GetMaxInstrs(const struct TaghaScript *script)
{
	return !script ? 0 : script->m_uiMaxInstrs;
}
uint32_t TaghaScript_GetNativeCount(const struct TaghaScript *script)
{
	return !script ? 0 : script->m_uiNatives;
}
uint32_t TaghaScript_GetFuncCount(const struct TaghaScript *script)
{
	return !script ? 0 : script->m_uiFuncs;
}
uint32_t TaghaScript_GetGlobalsCount(const struct TaghaScript *script)
{
	return !script ? 0 : script->m_uiGlobals;
}
bool TaghaScript_IsSafemodeActive(const struct TaghaScript *script)
{
	return !script ? 0 : script->m_bSafeMode;
}
bool TaghaScript_IsDebugActive(const struct TaghaScript *script)
{
	return !script ? 0 : script->m_bDebugMode;
}



void TaghaScript_PrintMem(const struct TaghaScript *script)
{
	if( !script or !script->m_pMemory )
		return;
	
	printf("DEBUG ...---===---... Printing Memory...\n");
	
	uint32_t size = script->m_uiMemsize;
	for( uint32_t i=0 ; i<size ; i++ )
		if( script->m_Regs[rsp].UCharPtr == script->m_pMemory + i )
			printf("Memory[%.10" PRIu32 "] == %" PRIu32 " - T.O.S.\n", i, script->m_pMemory[i]);
		else printf("Memory[%.10" PRIu32 "] == %" PRIu32 "\n", i, script->m_pMemory[i]);
	printf("\n");
}
void TaghaScript_PrintPtrs(const struct TaghaScript *script)
{
	if( !script )
		return;
	
	printf("DEBUG ...---===---... Printing Pointers...\n");
	printf("Instruction Ptr: %p\
			\nStack Ptr: %p\
			\nStack Frame Ptr: %p\n", script->m_Regs[rip].UCharPtr, script->m_Regs[rsp].UCharPtr, script->m_Regs[rbp].UCharPtr);
	printf("\n");
}
void TaghaScript_PrintInstrs(const struct TaghaScript *script)
{
	if( !script or )
		return;
	
	uint32_t size = script->m_uiInstrSize;
	size_t limit = script->m_pTextSegment-script->m_pMemory;
	for( uint32_t i=0 ; i<limit ; i++ )
		printf("Instr[%.10" PRIu32 "] == %" PRIu32 "\n", i, script->m_pMemory[i]);
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
		KGRN, script->m_Regs[rip].UCharPtr, script->m_Regs[rip].UCharPtr-script->m_pMemory, RESET, KGRN, script->m_Regs[rsp].UCharPtr, script->m_Regs[rsp].UCharPtr-script->m_pMemory, RESET);
}










