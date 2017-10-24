
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include "tagha.h"


void Tagha_init(struct TaghaVM *restrict vm)
{
	if( !vm )
		return;
	
	// initialize our Script vector and Natives hashmap.
	vm->m_pvecScripts = malloc(sizeof(Vec_t));
	if( !vm->m_pvecScripts )
		printf("[Tagha Error]: **** Unable to initialize Script Vector ****\n");
	else vector_init(vm->m_pvecScripts);
	
	vm->m_pmapNatives = malloc(sizeof(Map_t));
	if( !vm->m_pmapNatives )
		printf("[Tagha Error]: **** Unable to initialize Native Map ****\n");
	else map_init(vm->m_pmapNatives);
}


static uint32_t scripthdr_read_natives_table(Script_t **script, FILE **pFile)
{
	if( !*script or !*pFile )
		return 0;
	
	uint32_t	bytesread = 0;
	int32_t		ignores = 0;
	
	// see if the script is using any natives.
	ignores = fread(&(*script)->m_uiNatives, sizeof(uint32_t), 1, *pFile);
	bytesread += sizeof(uint32_t);
	if( (*script)->m_uiNatives ) {
		// script has natives? Copy their names so we can late bind them to host later.
		(*script)->m_pstrNatives = calloc((*script)->m_uiNatives, sizeof(char *));
		for( uint32_t i=0 ; i<(*script)->m_uiNatives ; i++ ) {
			uint32_t str_size;
			ignores = fread(&str_size, sizeof(uint32_t), 1, *pFile);
			bytesread += sizeof(uint32_t);
			
			// allocate memory to hold the native's name.
			(*script)->m_pstrNatives[i] = calloc(str_size, sizeof(char));
			
			// read in the native's name.
			ignores = fread((*script)->m_pstrNatives[i], sizeof(char), str_size, *pFile);
			bytesread += str_size;
			printf("Tagha_load_script_by_name :: copied native name \'%s\'\n", (*script)->m_pstrNatives[i]);
		}
	} /* if( script->m_uiNatives ) */
	else (*script)->m_pstrNatives = NULL;
	return bytesread;
}

static uint32_t scripthdr_read_func_table(Script_t **script, FILE **pFile)
{
	if( !*script or !*pFile )
		return 0;
	
	uint32_t	bytecount = 0;
	int32_t		ignore_warns = 0;
	// see if the script has its own functions.
	// This table is so host or other scripts can call these functions by name or address.
	ignore_warns = fread(&(*script)->m_uiFuncs, sizeof(uint32_t), 1, *pFile);
	bytecount += sizeof(uint32_t);
	if( (*script)->m_uiFuncs ) {
		// allocate our function hashmap so we can call functions by name.
		(*script)->m_pmapFuncs = malloc(sizeof(Map_t));
		map_init((*script)->m_pmapFuncs);
		
		// copy the function data from the header.
		for( uint32_t i=0 ; i<(*script)->m_uiFuncs ; i++ ) {
			uint32_t str_size;
			ignore_warns = fread(&str_size, sizeof(uint32_t), 1, *pFile);
			bytecount += sizeof(uint32_t);
			
			// allocate the hashmap function key.
			char *strFunc = calloc(str_size, sizeof(char));
			if( !strFunc ) {
				printf("Tagha_load_script_by_name :: Failed to allocate memory for strFunc\n");
				TaghaScript_free((*script));
				(*script) = NULL;
				fclose(*pFile), *pFile=NULL;
				return 0;
			}
			ignore_warns = fread(strFunc, sizeof(char), str_size, *pFile);
			bytecount += str_size;
			
			// allocate function's data to a table.
			struct FuncTable *pFuncData = malloc(sizeof(struct FuncTable));
			if( !pFuncData ) {
				printf("Tagha_load_script_by_name :: Failed to allocate memory for pFuncData\n");
				TaghaScript_free((*script));
				(*script) = NULL;
				fclose(*pFile), *pFile=NULL;
				return 0;
			}
			// copy func's header data to our table
			// then store the table to our function hashmap with the key
			// we allocated earlier.
			ignore_warns = fread(&pFuncData->m_uiParams, sizeof(uint32_t), 1, *pFile);
			bytecount += sizeof(uint32_t);
			ignore_warns = fread(&pFuncData->m_uiEntry, sizeof(uint32_t), 1, *pFile);
			bytecount += sizeof(uint32_t);
			printf("Tagha_load_script_by_name :: copied function name \'%s\'\n", strFunc);
			printf("Tagha_load_script_by_name :: Function Dict Keyvals { \'%s\': %u }\n", strFunc, pFuncData->m_uiEntry);
			map_insert((*script)->m_pmapFuncs, strFunc, (uintptr_t)pFuncData);
			strFunc = NULL, pFuncData = NULL;
		} /* for( uint32_t i=0 ; i<(*script)->m_uiFuncs ; i++ ) */
	} /* if( script->m_uiFuncs ) */
	else (*script)->m_pmapFuncs=NULL; //(*script)->pFuncTable = NULL;
	return bytecount;
}

static uint32_t scripthdr_read_global_table(Script_t **script, FILE **pFile)
{
	if( !*script or !*pFile )
		return 0;
	
	uint32_t	bytecount = 0;
	int32_t		ignore_warns = 0;
	
	// check if the script has global variables.
	ignore_warns = fread(&(*script)->m_uiGlobals, sizeof(uint32_t), 1, *pFile);
	printf("Tagha_load_script_by_name :: m_uiGlobals: \'%u\'\n", (*script)->m_uiGlobals);
	bytecount += sizeof(uint32_t);
	uint32_t globalbytes = 0;
	if( (*script)->m_uiGlobals ) {
		// script has globals, allocate our global var hashmap.
		(*script)->m_pmapGlobals = malloc(sizeof(Map_t));
		map_init((*script)->m_pmapGlobals);
		
		uint8_t *pTemp = (*script)->m_pMemory;
		for( uint32_t i=0 ; i<(*script)->m_uiGlobals ; i++ ) {
			uint32_t str_size;
			ignore_warns = fread(&str_size, sizeof(uint32_t), 1, *pFile);
			bytecount += sizeof(uint32_t);
			// allocate string to use as a key for our global var.
			char *strGlobal = calloc(str_size, sizeof(char));
			if( !strGlobal ) {
				printf("Tagha_load_script_by_name :: Failed to allocate memory for strGlobal\n");
				TaghaScript_free(*script), *script = NULL;
				fclose(*pFile), *pFile=NULL;
				return 0;
			}
			ignore_warns = fread(strGlobal, sizeof(char), str_size, *pFile);
			bytecount += str_size;
			
			// allocate table for our global var's data.
			struct DataTable *pGlobalData = malloc(sizeof(struct DataTable));
			if( !pGlobalData ) {
				printf("Tagha_load_script_by_name :: Failed to allocate memory for pGlobalData\n");
				TaghaScript_free(*script), *script = NULL;
				fclose(*pFile), *pFile=NULL;
				return 0;
			}
			
			// read the global var's data from the header and add it to our hashmap.
			// same procedure as our function hashmap.
			ignore_warns = fread(&pGlobalData->m_uiOffset, sizeof(uint32_t), 1, *pFile);
			bytecount += sizeof(uint32_t);
			ignore_warns = fread(&pGlobalData->m_uiBytes, sizeof(uint32_t), 1, *pFile);
			bytecount += sizeof(uint32_t);
			
			// global var always has intialized data. Copy that data to our stack.
			// copy it by byte.
			globalbytes = pGlobalData->m_uiBytes;
			{
				uint8_t initdata[globalbytes];
				printf("Tagha_load_script_by_name :: global var byte size: %u\n", globalbytes);
				ignore_warns = fread(initdata, sizeof(uint8_t), globalbytes, *pFile);
				bytecount += globalbytes;
				memcpy(pTemp, initdata, globalbytes);
				pTemp += globalbytes;
				//for( n=0 ; n<globalbytes ; n++ )
				//	printf("Tagha_load_script_by_name :: initdata[%u] == %u\n", n, initdata[n]);
			}
			printf("Tagha_load_script_by_name :: copied global var name \'%s\'\n", strGlobal);
			printf("Tagha_load_script_by_name :: Global Dict Keyvals { \'%s\': %u }\n", strGlobal, pGlobalData->m_uiOffset);
			
			// insert the global var's table to our hashmap.
			map_insert((*script)->m_pmapGlobals, strGlobal, (uintptr_t)pGlobalData);
			strGlobal = NULL, pGlobalData = NULL;
		} /* for( uint32_t i=0 ; i<script->m_uiGlobals ; i++ ) */
		pTemp = NULL;
	} /* if( script->m_uiGlobals ) */
	else (*script)->m_pmapGlobals = NULL; //script->pDataTable = NULL;
	return bytecount;
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
	return( (p[-3]=='t' and p[-2]=='b' and p[-1]=='c') or (p[-3]=='T' and p[-2]=='B' and p[-1]=='C') );
}

// need this to determine m_pText's final size.
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

void Tagha_load_script_by_name(struct TaghaVM *restrict vm, char *restrict filename)
{
	if( !vm )
		return;
	
	// open up our script in binary-mode.
	FILE *pFile = fopen(filename, "rb");
	if( !pFile ) {
		printf("[Tagha Load Script Error]: File not found: \'%s\'\n", filename);
		return;
	}
	
	// allocate our script.
	struct TaghaScript *script = malloc(sizeof(struct TaghaScript));
	if( script ) {
		script->m_pText = NULL;
		script->m_pMemory = NULL;
		
		uint64_t filesize = get_file_size(pFile);
		uint32_t bytecount = 0;	// bytecount is for separating the header data from the actual instruction stream.
		uint16_t verify;
		int32_t ignore_warns;
		ignore_warns = fread(&verify, sizeof(uint16_t), 1, pFile);
		
		// verify that this is executable code.
		if( verify == 0xC0DE ) {
			printf("Tagha_load_script_by_name :: verified code!\n");
			bytecount += sizeof(uint16_t);
			
			// get the needed script memory size which will include both global and local vars.
			ignore_warns = fread(&script->m_uiMemsize, sizeof(uint32_t), 1, pFile);
			printf("Tagha_load_script_by_name :: Memory Size: %" PRIu32 "\n", script->m_uiMemsize);
			
			// have a default size of 64 bytes for memory.
			if( !script->m_uiMemsize )
				script->m_uiMemsize = 64;
			script->m_pMemory = calloc(script->m_uiMemsize, sizeof(uint8_t));
			
			// scripts NEED memory, if memory is invalid then we can't use the script.
			if( !script->m_pMemory ) {
				printf("Tagha_load_script_by_name :: Failed to allocate memory for script\n");
				TaghaScript_free(script), script = NULL;
				goto error;
			}
			bytecount += sizeof(uint32_t);
			
			// set both the integer and pointer stack pointers to point at the top of the stack.
			script->m_pSP = script->m_pBP = script->m_pMemory + (script->m_uiMemsize-1);
			
			bytecount += scripthdr_read_natives_table(&script, &pFile);
			bytecount += scripthdr_read_func_table(&script, &pFile);
			if( !pFile )
				return;
			bytecount += scripthdr_read_global_table(&script, &pFile);
			if( !pFile )
				return;
			
			// read in our entry point where our code should begin executing.
			uint64_t entry;
			ignore_warns = fread(&entry, sizeof(uint64_t), 1, pFile);
			printf("Tagha_load_script_by_name :: entry m_pIP starts at %" PRIu64 "\n", entry);
			bytecount += sizeof(uint64_t);
			
			// check if the script is either in safemode or debug mode.
			char boolean;
			ignore_warns = fread(&boolean, sizeof(bool), 1, pFile);
			script->m_bSafeMode = (boolean & 1) >= 1;
			printf("Tagha_load_script_by_name :: Script Safe Mode: %" PRIu32 "\n", script->m_bSafeMode);
			
			script->m_bDebugMode = (boolean & 2) >= 1;
			printf("Tagha_load_script_by_name :: Script Debug Mode: %" PRIu32 "\n", script->m_bDebugMode);
			bytecount += sizeof(bool);
			
			printf("Tagha_load_script_by_name :: final stack size %p\n", script->m_pSP);
			script->m_uiMaxInstrs = 0xfffff;	// helps to stop infinite/runaway loops
			
			// header data is finished, subtract the filesize with the bytecount
			// to get the size of our instruction stream.
			// If the instruction stream is invalid, we can't load script.
			printf("Tagha_load_script_by_name :: header bytecount at %" PRIu32 "\n", bytecount);
			script->m_uiInstrSize = filesize - bytecount;
			printf("Tagha_load_script_by_name :: instr_size at %" PRIu32 "\n", script->m_uiInstrSize);
			script->m_pText = calloc(script->m_uiInstrSize, sizeof(uint8_t));
			if( !script->m_pText ) {
				printf("Tagha_load_script_by_name :: ERROR! Could not allocate Instruction Stream!\n");
				TaghaScript_free(script), script = NULL;
				goto error;
			}
			
			// instruction stream is valid, copy every last instruction and data to the last byte.
			ignore_warns = fread(script->m_pText, sizeof(uint8_t), script->m_uiInstrSize, pFile);
			script->m_pIP = script->m_pText + entry;
			
			// script data has been verified and loaded to memory.
			// copy script's memory address to VM's script vector.
			vector_add(vm->m_pvecScripts, script);
			script = NULL;
			printf("\n");
		}
		else {	// invalid script, kill the reference and whatever memory the script has allocated.
			printf("Tagha_load_script_by_name :: unknown file memory format\n");
			TaghaScript_free(script), script = NULL;
		}
	}
	else script = NULL;
error:;
	fclose(pFile), pFile=NULL;
}


void Tagha_free(struct TaghaVM *vm)
{
	if( !vm )
		return;
	
	// iterate through our script vector and free
	// each script individually. Then set each vector index
	// to NULL so we don't leave a trace of it.
	if( vm->m_pvecScripts ) {
		Vec_t *vecbuffer=vm->m_pvecScripts;
		uint32_t nScripts = vector_count(vecbuffer);
		struct TaghaScript *script;
		for( uint32_t i=0 ; i<nScripts ; i++ ) {
			script = vector_get(vecbuffer, i);
			if( !script )
				continue;
			
			TaghaScript_free(script), script=NULL;
			// after freeing scripts, replace their data with NULL.
			vector_set(vecbuffer, i, NULL);
		}
		
		// after we've cleaned up the scripts,
		// free the vector's remaining data then free the vector itself.
		vector_free(vm->m_pvecScripts);
		gfree((void **)&vm->m_pvecScripts);
		vecbuffer = NULL;
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

void gfree(void **ptr)
{
	if( *ptr )
		free(*ptr);
	*ptr = NULL;
}











