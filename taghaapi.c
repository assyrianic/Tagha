
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include "tagha.h"


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

void Tagha_init(struct TaghaVM *restrict vm)
{
	if( !vm )
		return;
	
	vm->pvecScripts = malloc(sizeof(Vec_t));
	if( !vm->pvecScripts )
		printf("[Tagha Error]: **** Unable to initialize Script Vector ****\n");
	else vector_init(vm->pvecScripts);
	
	vm->pmapNatives = malloc(sizeof(Map_t));
	if( !vm->pmapNatives )
		printf("[Tagha Error]: **** Unable to initialize Native Map ****\n");
	else map_init(vm->pmapNatives);
	
	vm->pmapExpTypes = malloc(sizeof(Map_t));
	if( !vm->pmapExpTypes )
		printf("[Tagha Error]: **** Unable to initialize Exported Types Map ****\n");
	else map_init(vm->pmapExpTypes);
}

void Tagha_load_script(struct TaghaVM *restrict vm, char *restrict filename)
{
	if( !vm )
		return;
	
	FILE *pFile = fopen(filename, "rb");
	if( !pFile ) {
		printf("[Tagha Error]: File not found: \'%s\'\n", filename);
		return;
	}
	
	struct TaghaScript *script = NULL;
	script = malloc(sizeof(struct TaghaScript));
	if( script ) {
		script->pInstrStream = NULL;
		script->pbMemory = NULL;
		
		//fclose(pFile); pFile=NULL;
		uint64_t filesize = get_file_size(pFile);
		uint32_t bytecount = 0;
		uint16_t verify;
		int32_t ignore_warns;
		ignore_warns = fread(&verify, sizeof(uint16_t), 1, pFile);
		
		// verify that this is executable code.
		if( verify == 0xC0DE ) {
			printf("Tagha_load_script :: verified code!\n");
			bytecount += sizeof(uint16_t);
			
			ignore_warns = fread(&script->uiMemsize, sizeof(uint32_t), 1, pFile);
			printf("Tagha_load_script :: Memory Size: %" PRIu32 "\n", script->uiMemsize);
			if( script->uiMemsize )
				script->pbMemory = calloc(script->uiMemsize, sizeof(uint8_t));
			else script->pbMemory = calloc(32, sizeof(uint8_t));	// have a default size of 32 bytes for memory.
			if( !script->pbMemory ) {
				printf("Tagha_load_script :: Failed to allocate memory for script\n");
				TaghaScript_free(script);
				script = NULL;
				goto error;
			}
			bytecount += sizeof(uint32_t);
			script->sp = script->bp = script->uiMemsize-1;
			
			ignore_warns = fread(&script->uiNatives, sizeof(uint32_t), 1, pFile);
			bytecount += sizeof(uint32_t);
			if( script->uiNatives ) {
				script->pstrNatives = calloc(script->uiNatives, sizeof(char *));
				uint32_t i;
				for( i=0 ; i<script->uiNatives ; i++ ) {
					uint32_t str_size;
					ignore_warns = fread(&str_size, sizeof(uint32_t), 1, pFile);
					bytecount += sizeof(uint32_t);
					script->pstrNatives[i] = calloc(str_size, sizeof(char));
					uint32_t n = 0;
					char c;
					while( n<str_size ) {
						ignore_warns = fread(&c, sizeof(char), 1, pFile);
						script->pstrNatives[i][n++] = c;
						bytecount += sizeof(char);
					}
					printf("Tagha_load_script :: copied native name \'%s\'\n", script->pstrNatives[i]);
				}
			} else script->pstrNatives = NULL;
			
			ignore_warns = fread(&script->uiFuncs, sizeof(uint32_t), 1, pFile);
			bytecount += sizeof(uint32_t);
			if( script->uiFuncs ) {
				//script->pFuncTable = calloc(script->uiFuncs, sizeof(FuncTable_t));
				script->pmapFuncs = malloc(sizeof(Map_t));
				map_init(script->pmapFuncs);
				
				uint32_t i;
				for( i=0 ; i<script->uiFuncs ; i++ ) {
					uint32_t str_size;
					ignore_warns = fread(&str_size, sizeof(uint32_t), 1, pFile);
					bytecount += sizeof(uint32_t);
					//script->pFuncTable[i].pFuncName = calloc(str_size, sizeof(char));
					char *strFunc = calloc(str_size, sizeof(char));
					if( !strFunc ) {
						printf("Tagha_load_script :: Failed to allocate memory for strFunc\n");
						TaghaScript_free(script);
						script = NULL;
						goto error;
					}
					uint32_t n = 0;
					char c;
					while( n<str_size ) {
						ignore_warns = fread(&c, sizeof(char), 1, pFile);
						//script->pFuncTable[i].pFuncName[n++] = c;
						strFunc[n++] = c;
						bytecount += sizeof(char);
					}
					FuncTable_t *pFuncData = malloc(sizeof(FuncTable_t));
					if( !pFuncData ) {
						printf("Tagha_load_script :: Failed to allocate memory for pFuncData\n");
						TaghaScript_free(script);
						script = NULL;
						goto error;
					}
					ignore_warns = fread(&pFuncData->uiParams, sizeof(uint32_t), 1, pFile);
					bytecount += sizeof(uint32_t);
					ignore_warns = fread(&pFuncData->uiEntry, sizeof(uint32_t), 1, pFile);
					bytecount += sizeof(uint32_t);
					printf("Tagha_load_script :: copied function name \'%s\'\n", strFunc);
					printf("Tagha_load_script :: Function Dict Keyvals { \'%s\': %u }\n", strFunc, pFuncData->uiEntry);
					map_insert(script->pmapFuncs, strFunc, pFuncData);
					strFunc = NULL, pFuncData = NULL;
				}
			} else script->pmapFuncs=NULL; //script->pFuncTable = NULL;
			
			ignore_warns = fread(&script->uiGlobals, sizeof(uint32_t), 1, pFile);
			printf("Tagha_load_script :: uiGlobals: \'%u\'\n", script->uiGlobals);
			bytecount += sizeof(uint32_t);
			uint32_t globalbytes = 0;
			if( script->uiGlobals ) {
				//script->pDataTable = calloc(script->uiGlobals, sizeof(DataTable_t));
				script->pmapGlobals = malloc(sizeof(Map_t));
				map_init(script->pmapGlobals);
				
				uint32_t i;
				for( i=0 ; i<script->uiGlobals ; i++ ) {
					uint32_t str_size;
					ignore_warns = fread(&str_size, sizeof(uint32_t), 1, pFile);
					bytecount += sizeof(uint32_t);
					//script->pDataTable[i].pVarName = calloc(str_size, sizeof(char));
					char *strGlobal = calloc(str_size, sizeof(char));
					if( !strGlobal ) {
						printf("Tagha_load_script :: Failed to allocate memory for strGlobal\n");
						TaghaScript_free(script);
						script = NULL;
						goto error;
					}
					
					uint32_t n = 0;
					char c;
					while( n<str_size ) {
						ignore_warns = fread(&c, sizeof(char), 1, pFile);
						//script->pDataTable[i].pVarName[n++] = c;
						strGlobal[n++] = c;
						bytecount += sizeof(char);
					}
					DataTable_t *pGlobalData = malloc(sizeof(DataTable_t));
					if( !pGlobalData ) {
						printf("Tagha_load_script :: Failed to allocate memory for pGlobalData\n");
						TaghaScript_free(script);
						script = NULL;
						goto error;
					}
					
					ignore_warns = fread(&pGlobalData->uiAddress, sizeof(uint32_t), 1, pFile);
					bytecount += sizeof(uint32_t);
					ignore_warns = fread(&pGlobalData->uiBytes, sizeof(uint32_t), 1, pFile);
					bytecount += sizeof(uint32_t);
					
					globalbytes = pGlobalData->uiBytes;
					{
						uint8_t initdata[globalbytes];
						printf("Tagha_load_script :: global var byte size: %u\n", globalbytes);
						ignore_warns = fread(initdata, sizeof(uint8_t), globalbytes, pFile);
						bytecount += globalbytes;
						TaghaScript_push_nbytes(script, initdata, globalbytes);
						//for( n=0 ; n<globalbytes ; n++ )
						//	printf("Tagha_load_script :: buffer[%u] == %u\n", n, initdata[n]);
					}
					printf("Tagha_load_script :: copied global var name \'%s\'\n", strGlobal);
					printf("Tagha_load_script :: Global Dict Keyvals { \'%s\': %u }\n", strGlobal, pGlobalData->uiAddress);
					map_insert(script->pmapGlobals, strGlobal, pGlobalData);
					strGlobal = NULL, pGlobalData = NULL;
				}
			} else script->pmapGlobals = NULL; //script->pDataTable = NULL;
			
			/*
			script->pvecHostData = malloc(sizeof(Vec_t));
			if( !script->pvecHostData ) {
				printf("Tagha_load_script :: Failed to allocate memory for host data Vec_t.\n");
				TaghaScript_free(script);
				script = NULL;
				goto error;
			}
			else vector_init(script->pvecHostData);
			*/
			
			ignore_warns = fread(&script->ip, sizeof(uint64_t), 1, pFile);
			printf("Tagha_load_script :: entry ip starts at %" PRIWord "\n", script->ip);
			bytecount += sizeof(uint64_t);
			
			ignore_warns = fread(&script->bSafeMode, sizeof(bool), 1, pFile);
			printf("Tagha_load_script :: Script Safe Mode: %" PRIu32 "\n", script->bSafeMode);
			bytecount += sizeof(bool);
			ignore_warns = fread(&script->bDebugMode, sizeof(bool), 1, pFile);
			printf("Tagha_load_script :: Script Debug Mode: %" PRIu32 "\n", script->bDebugMode);
			bytecount += sizeof(bool);
			
			printf("Tagha_load_script :: final stack size %" PRIWord "\n", script->sp);
			script->uiMaxInstrs = 0xfffff;	// helps to stop infinite/runaway loops
			
			printf("Tagha_load_script :: header bytecount at %" PRIu32 "\n", bytecount);
			script->uiInstrSize = filesize - bytecount;
			printf("Tagha_load_script :: instr_size at %" PRIu32 "\n", script->uiInstrSize);
			script->pInstrStream = calloc(script->uiInstrSize, sizeof(uint8_t));
			if( !script->pInstrStream ) {
				printf("Tagha_load_script :: ERROR! Could not allocate Instruction Stream!\n");
				TaghaScript_free(script);
				script = NULL;
				goto error;
			}
			
			ignore_warns = fread(script->pInstrStream, sizeof(uint8_t), script->uiInstrSize, pFile);
			
			// transfer script address to the Vec_t.
			vector_add(vm->pvecScripts, script);
			script = NULL;
			printf("\n");
		}
		else {	// invalid script, kill the reference and the script itself.
			printf("Tagha_load_script :: unknown file memory format\n");
			TaghaScript_free(script);
			script = NULL;
		}
	}
	else script = NULL;
error:;
	fclose(pFile), pFile=NULL;
}

void TaghaScript_free(struct TaghaScript *script)
{
	if( !script )
		return;
	
	if( script->pbMemory )
		free(script->pbMemory);
	script->pbMemory = NULL;
	
	if( script->pInstrStream )
		free(script->pInstrStream);
	script->pInstrStream = NULL;
	
	// free our tables
	uint32_t i;
	if( script->pstrNatives ) {
		for( i=0 ; i<script->uiNatives ; i++ ) {
			if( script->pstrNatives[i] )
				free(script->pstrNatives[i]);
			script->pstrNatives[i] = NULL;
		}
		free(script->pstrNatives);
		script->pstrNatives = NULL;
	}
	if( script->pmapFuncs ) {
		kvnode_t
			*kv = NULL,
			*next = NULL
		;
		FuncTable_t *pFuncData;
		char *pStr;
		for( uint32_t i=0 ; i<script->pmapFuncs->size ; i++ ) {
			for( kv = script->pmapFuncs->table[i] ; kv ; kv = next ) {
				next = kv->pNext;
				pFuncData = kv->pData;
				if( pFuncData )
					free(pFuncData), pFuncData=NULL, kv->pData=NULL;
				
				pStr = (char *)kv->strKey;
				if( pStr )
					free(pStr), pStr=NULL, kv->strKey=NULL;
			}
		}
		map_free(script->pmapFuncs);
		free(script->pmapFuncs);
		script->pmapFuncs = NULL;
	}
	if( script->pmapGlobals ) {
		kvnode_t
			*kv = NULL,
			*next = NULL
		;
		DataTable_t *pGlobalData;
		char *pStr;
		for( uint32_t i=0 ; i<script->pmapGlobals->size ; i++ ) {
			for( kv = script->pmapGlobals->table[i] ; kv ; kv = next ) {
				next = kv->pNext;
				pGlobalData = kv->pData;
				if( pGlobalData )
					free(pGlobalData), pGlobalData=NULL, kv->pData=NULL;
				
				pStr = (char *)kv->strKey;
				if( pStr )
					free(pStr), pStr=NULL, kv->strKey=NULL;
			}
		}
		map_free(script->pmapGlobals);
		free(script->pmapGlobals);
		script->pmapGlobals = NULL;
	}
	/*
	if( script->pvecHostData )
		TaghaScript_free_hostdata(script);
	*/
	free(script);
}

void Tagha_free(struct TaghaVM *vm)
{
	if( !vm )
		return;
	if( vm->pvecScripts ) {
		uint32_t n=0;
		uint32_t nScripts = vector_count(vm->pvecScripts);
		struct TaghaScript *script;
		Vec_t *buffer=vm->pvecScripts;
		for( n=0 ; n<nScripts ; n++ ) {
			script = vector_get(buffer, n);
			if( !script )
				continue;
			
			TaghaScript_free(script), script=NULL;
			// after freeing scripts, replace their data with NULL.
			vector_set(buffer, n, NULL);
		}
		vector_free(vm->pvecScripts);
		free(vm->pvecScripts);
		vm->pvecScripts = NULL;
		buffer = NULL;
	}
	if( vm->pmapNatives ) {
		map_free(vm->pmapNatives);
		free(vm->pmapNatives);
		vm->pmapNatives = NULL;
	}
	if( vm->pmapExpTypes ) {
		map_free(vm->pmapExpTypes);
		free(vm->pmapExpTypes);
		vm->pmapExpTypes = NULL;
	}
}


void TaghaScript_reset(struct TaghaScript *script)
{
	if( !script )
		return;
	// better than a for-loop setting everything to 0.
	memset(script->pbMemory, 0, script->uiMemsize);
	script->sp = script->bp = script->uiMemsize-1;
}

bool Tagha_register_natives(struct TaghaVM *restrict vm, NativeInfo_t *restrict arrNatives)
{
	if( !vm or !arrNatives )
		return false;
	else if( !vm->pmapNatives )
		return false;
	
	uint32_t i;
	for( i=0 ; arrNatives[i].pFunc != NULL ; i++ )
		map_insert(vm->pmapNatives, arrNatives[i].strName, (void *)arrNatives[i].pFunc);
	return true;
}



void TaghaScript_push_longfloat(struct TaghaScript *restrict script, const long double val)
{
	if( !script )
		return;
	
	// long doubles are usually 12 or 16 bytes, adjust for both.
	uint32_t size = sizeof(long double);
	if( script->bSafeMode and (script->sp-size) >= script->uiMemsize ) {
		printf("TaghaScript_push_longfloat reported: stack overflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		exit(1);
	}
	script->sp -= size;
	*(long double *)(script->pbMemory + script->sp) = val;
}

long double TaghaScript_pop_longfloat(struct TaghaScript *script)
{
	if( !script )
		return 0;
	
	uint32_t size = sizeof(long double);
	if( script->bSafeMode and (script->sp+size) >= script->uiMemsize ) {
		printf("TaghaScript_pop_longfloat reported: stack underflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		exit(1);
	}
	long double val = *(long double *)(script->pbMemory + script->sp);
	script->sp += size;
	return val;
}

void TaghaScript_push_int64(struct TaghaScript *restrict script, const uint64_t val)
{
	if( !script )
		return;
	uint32_t size = sizeof(uint64_t);
	if( script->bSafeMode and (script->sp-size) >= script->uiMemsize ) {
		printf("TaghaScript_push_int64 reported: stack overflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return;
	}
	script->sp -= size;
	*(uint64_t *)(script->pbMemory + script->sp) = val;
}
uint64_t TaghaScript_pop_int64(struct TaghaScript *script)
{
	if( !script )
		return 0L;
	uint32_t size = sizeof(uint64_t);
	if( script->bSafeMode and (script->sp+size) >= script->uiMemsize ) {
		printf("TaghaScript_pop_int64 reported: stack underflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return 0L;
	}
	uint64_t val = *(uint64_t *)(script->pbMemory + script->sp);
	script->sp += size;
	return val;
}

void TaghaScript_push_float64(struct TaghaScript *restrict script, const double val)
{
	if( !script )
		return;
	uint32_t size = sizeof(double);
	if( script->bSafeMode and (script->sp-size) >= script->uiMemsize ) {
		printf("TaghaScript_push_float64 reported: stack overflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return;
	}
	script->sp -= size;
	*(double *)(script->pbMemory + script->sp) = val;
}
double TaghaScript_pop_float64(struct TaghaScript *script)
{
	if( !script )
		return 0;
	uint32_t size = sizeof(double);
	if( script->bSafeMode and (script->sp+size) >= script->uiMemsize ) {
		printf("TaghaScript_pop_float64 reported: stack underflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return 0;
	}
	double val = *(double *)(script->pbMemory + script->sp);
	script->sp += size;
	return val;
}

void TaghaScript_push_int32(struct TaghaScript *restrict script, const uint32_t val)
{
	if( !script )
		return;
	uint32_t size = sizeof(uint32_t);
	if( script->bSafeMode and (script->sp-size) >= script->uiMemsize ) {
		printf("TaghaScript_push_int32 reported: stack overflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return;
	}
	script->sp -= size;
	*(uint32_t *)(script->pbMemory + script->sp) = val;
}
uint32_t TaghaScript_pop_int32(struct TaghaScript *script)
{
	if( !script )
		return 0;
	uint32_t size = sizeof(uint32_t);
	if( script->bSafeMode and (script->sp+size) >= script->uiMemsize ) {
		printf("TaghaScript_pop_int32 reported: stack underflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return 0;
	}
	uint32_t val = *(uint32_t *)(script->pbMemory + script->sp);
	script->sp += size;
	return val;
}

void TaghaScript_push_float32(struct TaghaScript *restrict script, const float val)
{
	if( !script )
		return;
	uint32_t size = sizeof(float);
	if( script->bSafeMode and (script->sp-size) >= script->uiMemsize ) {
		printf("TaghaScript_push_float32 reported: stack overflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return;
	}
	script->sp -= size;
	*(float *)(script->pbMemory + script->sp) = val;
}
float TaghaScript_pop_float32(struct TaghaScript *script)
{
	if( !script )
		return 0;
	uint32_t size = sizeof(float);
	if( script->bSafeMode and (script->sp+size) >= script->uiMemsize ) {
		printf("TaghaScript_pop_float32 reported: stack underflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return 0;
	}
	float val = *(float *)(script->pbMemory + script->sp);
	script->sp += size;
	return val;
}

void TaghaScript_push_short(struct TaghaScript *restrict script, const uint16_t val)
{
	if( !script )
		return;
	uint32_t size = sizeof(uint16_t);
	if( script->bSafeMode and (script->sp-size) >= script->uiMemsize ) {
		printf("TaghaScript_push_short reported: stack overflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return;
	}
	script->sp -= size;
	*(uint16_t *)(script->pbMemory + script->sp) = val;
}
uint16_t TaghaScript_pop_short(struct TaghaScript *script)
{
	if( !script )
		return 0;
	uint32_t size = sizeof(uint16_t);
	if( script->bSafeMode and (script->sp+size) >= script->uiMemsize ) {
		printf("TaghaScript_pop_short reported: stack underflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return 0;
	}
	uint16_t val = *(uint16_t *)(script->pbMemory + script->sp);
	script->sp += size;
	return val;
}

void TaghaScript_push_byte(struct TaghaScript *restrict script, const uint8_t val)
{
	if( !script )
		return;
	uint32_t size = sizeof(uint8_t);
	if( script->bSafeMode and (script->sp-size) >= script->uiMemsize ) {
		printf("TaghaScript_push_byte reported: stack overflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return;
	}
	script->sp -= size;
	script->pbMemory[script->sp] = val;
}
uint8_t TaghaScript_pop_byte(struct TaghaScript *script)
{
	if( !script )
		return 0;
	uint32_t size = sizeof(uint8_t);
	if( script->bSafeMode and (script->sp+size) >= script->uiMemsize ) {
		printf("TaghaScript_pop_byte reported: stack underflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return 0;
	}
	uint8_t val = script->pbMemory[script->sp];
	script->sp += size;
	return val;
}

void TaghaScript_push_nbytes(struct TaghaScript *restrict script, void *restrict pItem, const uint32_t bytesize)
{
	if( !script )
		return;
	if( script->bSafeMode and (script->sp-bytesize) >= script->uiMemsize ) {
		printf("TaghaScript_push_nbytes reported: stack overflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return;
	}
	script->sp -= bytesize;
	memcpy((script->pbMemory + script->sp), pItem, bytesize);
	/*
	uint64_t i=0;
	for( i=bytesize-1 ; i<bytesize ; i-- )
		script->pbMemory[--script->sp] = ((uint8_t *)pItem)[i];
	*/
}
void TaghaScript_pop_nbytes(struct TaghaScript *restrict script, void *restrict pBuffer, const uint32_t bytesize)
{
	if( !script )
		return;
	if( script->bSafeMode and (script->sp+bytesize) >= script->uiMemsize ) {
		printf("TaghaScript_pop_nbytes reported: stack underflow! Current instruction address: %" PRIWord " | Stack index: %" PRIWord "\n", script->ip, script->sp);
		return;
	}
	memcpy(pBuffer, (script->pbMemory + script->sp), bytesize);
	script->sp += bytesize;
	/*
	uint64_t i=0;
	for( i=0 ; i<bytesize ; i++ )
		((uint8_t *)pBuffer)[i] = script->pbMemory[script->sp++];
	*/
}

uint8_t *TaghaScript_addr2ptr(struct TaghaScript *restrict script, const uint64_t stk_address)
{
	if( !script or !script->pbMemory )
		return NULL;
	else if( stk_address >= script->uiMemsize )
		return NULL;
	return( script->pbMemory + stk_address );
}

void TaghaScript_call_func_by_name(struct TaghaScript *restrict script, const char *restrict strFunc)
{
	if( !script or !strFunc )
		return;
	else if( !script->pmapFuncs )
		return;
	
	FuncTable_t	*pFuncs = map_find(script->pmapFuncs, strFunc);
	if( pFuncs ) {
		bool debugmode = script->bDebugMode;
		uint32_t func_addr = pFuncs->uiEntry;
		
		if( debugmode )
			printf("TaghaScript_call_func_by_name :: calling address: %" PRIu32 "\n", func_addr);
		
		TaghaScript_push_int64(script, script->ip+1);	// save return address.
		if( debugmode )
			printf("TaghaScript_call_func_by_name :: return addr: %" PRIWord "\n", script->ip+1);
		
		script->ip = func_addr;	// jump to instruction
		
		TaghaScript_push_int64(script, script->bp);	// push ebp;
		if( debugmode )
			printf("TaghaScript_call_func_by_name :: pushing bp: %" PRIWord "\n", script->bp);
		script->bp = script->sp;	// mov ebp, esp;
		
		if( debugmode )
			printf("TaghaScript_call_func_by_name :: bp set to sp: %" PRIWord "\n", script->bp);
	}
	pFuncs=NULL;
}
void TaghaScript_call_func_by_addr(struct TaghaScript *script, const uint64_t func_addr)
{
	if( !script )
		return;
	else if( script->uiInstrSize >= func_addr )
		return;
	
	TaghaScript_push_int64(script, script->ip+1);	// save return address.
	script->ip = func_addr;	// jump to instruction
	
	TaghaScript_push_int64(script, script->bp);	// push ebp;
	script->bp = script->sp;	// mov ebp, esp;
}

void *TaghaScript_get_global_by_name(struct TaghaScript *script, const char *strGlobalName)
{
	void *p = NULL;
	if( !script or !script->pmapGlobals )
		return p;
	
	DataTable_t *pGlobals = map_find(script->pmapGlobals, strGlobalName);
	if( pGlobals ) {
		p = script->pbMemory + pGlobals->uiAddress;
		pGlobals=NULL;
	}
	return p;
}
/*
uint32_t TaghaScript_store_hostdata(struct TaghaScript *restrict script, void *restrict data)
{
	if( !script or !script->pvecHostData )
		return 0xFFFFFFFF;
	
	static uint32_t id = 0;
	Handle_t *pHandle = malloc(sizeof(Handle_t));
	pHandle->pHostPtr = data;
	pHandle->uiHNDL = ++id;
	vector_add(script->pvecHostData, pHandle);
	pHandle = NULL;
	return id;
}

void *TaghaScript_get_hostdata(struct TaghaScript *script, const uint32_t id)
{
	if( !script or !script->pvecHostData )
		return NULL;
	
	Vec_t *pVec = script->pvecHostData;
	Handle_t *pIter;
	for( uint32_t i=0 ; i<pVec->count ; i++ ) {
		pIter = pVec->data[i];
		if( !pIter )
			continue;
		else if( pIter->uiHNDL==id )
			return pIter->pHostPtr;
	}
	return NULL;
}

void TaghaScript_del_hostdata(struct TaghaScript *script, const uint32_t id)
{
	if( !script or !script->pvecHostData )
		return;
	
	Vec_t *pVec = script->pvecHostData;
	Handle_t *pIter;
	uint32_t i = 0;
	for( i=0 ; i<pVec->count ; i++ ) {
		pIter = pVec->data[i];
		if( !pIter )
			continue;
		else if( pIter->uiHNDL==id ) {
			pIter->pHostPtr = NULL;
			free(pIter), pIter = NULL;
			pVec->data[i] = NULL;
			break;
		}
	}
	vector_delete(pVec, i);
}

void TaghaScript_free_hostdata(struct TaghaScript *script)
{
	if( !script or !script->pvecHostData )
		return;
	
	Vec_t *pVec = script->pvecHostData;
	Handle_t *pIter;
	for( uint32_t i=0 ; i<pVec->count ; i++ ) {
		pIter = pVec->data[i];
		if( !pIter )
			continue;
		
		pIter->pHostPtr = NULL;
		free(pIter), pIter = NULL;
		pVec->data[i] = NULL;
	}
	vector_free(pVec);
	free(pVec), pVec = script->pvecHostData = NULL;
}
*/

uint32_t TaghaScript_stacksize(const struct TaghaScript *script)
{
	if( !script )
		return 0;
	return script->uiMemsize;
}
uint32_t TaghaScript_instrsize(const struct TaghaScript *script)
{
	if( !script )
		return 0;
	return script->uiInstrSize;
}
uint32_t TaghaScript_maxinstrs(const struct TaghaScript *script)
{
	if( !script )
		return 0;
	return script->uiMaxInstrs;
}
uint32_t TaghaScript_nativecount(const struct TaghaScript *script)
{
	if( !script )
		return 0;
	return script->uiNatives;
}
uint32_t TaghaScript_funcs(const struct TaghaScript *script)
{
	if( !script )
		return 0;
	return script->uiFuncs;
}
uint32_t TaghaScript_globals(const struct TaghaScript *script)
{
	if( !script )
		return 0;
	return script->uiGlobals;
}

bool TaghaScript_safemode_active(const struct TaghaScript *script)
{
	if( !script )
		return false;
	return script->bSafeMode;
}
bool TaghaScript_debug_active(const struct TaghaScript *script)
{
	if( !script )
		return false;
	return script->bDebugMode;
}


void TaghaScript_debug_print_memory(const struct TaghaScript *script)
{
	if( !script )
		return;
	else if( !script->pbMemory )
		return;
	
	printf("DEBUG ...---===---... Printing Memory...\n");
	uint32_t i;
	uint32_t size = script->uiMemsize;
	for( i=0 ; i<size ; i++ )
		if( script->sp == i )
			printf("Memory[%.10" PRIu32 "] == %" PRIu32 " - T.O.S.\n", i, script->pbMemory[i]);
		else printf("Memory[%.10" PRIu32 "] == %" PRIu32 "\n", i, script->pbMemory[i]);
	printf("\n");
}
void TaghaScript_debug_print_ptrs(const struct TaghaScript *script)
{
	if( !script )
		return;
	
	printf("DEBUG ...---===---... Printing Pointers...\n");
	printf("Instruction Pointer: %" PRIWord "\
			\nStack Pointer: %" PRIWord "\
			\nStack Frame Pointer: %" PRIWord "\n", script->ip, script->sp, script->bp);
	printf("\n");
}
void TaghaScript_debug_print_instrs(const struct TaghaScript *script)
{
	if( !script )
		return;
	
	uint32_t i;
	uint32_t size = script->uiInstrSize;
	for( i=0 ; i<size ; i++ )
		printf("Instr[%.10"PRIu32"] == %"PRIu32"\n", i, script->pInstrStream[i]);
	printf("\n");
}
















