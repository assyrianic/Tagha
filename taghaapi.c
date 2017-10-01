
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include "tagha.h"

// need this to determine pInstrStream's final size.
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
	
	// initialize our Script vector and Natives hashmap.
	
	vm->pvecScripts = malloc(sizeof(Vec_t));
	if( !vm->pvecScripts )
		printf("[Tagha Error]: **** Unable to initialize Script Vector ****\n");
	else vector_init(vm->pvecScripts);
	
	vm->pmapNatives = malloc(sizeof(Map_t));
	if( !vm->pmapNatives )
		printf("[Tagha Error]: **** Unable to initialize Native Map ****\n");
	else map_init(vm->pmapNatives);
	
	/*
	vm->pmapExpTypes = malloc(sizeof(Map_t));
	if( !vm->pmapExpTypes )
		printf("[Tagha Error]: **** Unable to initialize Exported Types Map ****\n");
	else map_init(vm->pmapExpTypes);
	*/
}

void Tagha_load_script(struct TaghaVM *restrict vm, char *restrict filename)
{
	if( !vm )
		return;
	
	// open up our script in binary-mode.
	FILE *pFile = fopen(filename, "rb");
	if( !pFile ) {
		printf("[Tagha Error]: File not found: \'%s\'\n", filename);
		return;
	}
	
	// allocate our script.
	struct TaghaScript *script = NULL;
	script = malloc(sizeof(struct TaghaScript));
	if( script ) {
		script->pInstrStream = NULL;
		script->pbMemory = NULL;
		
		uint64_t filesize = get_file_size(pFile);
		uint32_t bytecount = 0;	// bytecount is for separating the header data from the actual instruction stream.
		uint16_t verify;
		int32_t ignore_warns;
		ignore_warns = fread(&verify, sizeof(uint16_t), 1, pFile);
		
		// verify that this is executable code.
		if( verify == 0xC0DE ) {
			printf("Tagha_load_script :: verified code!\n");
			bytecount += sizeof(uint16_t);
			
			// get the needed script memory size which will include both global and local vars.
			ignore_warns = fread(&script->uiMemsize, sizeof(uint32_t), 1, pFile);
			printf("Tagha_load_script :: Memory Size: %" PRIu32 "\n", script->uiMemsize);
			if( script->uiMemsize )
				script->pbMemory = calloc(script->uiMemsize, sizeof(uint8_t));
			else script->pbMemory = calloc(32, sizeof(uint8_t));	// have a default size of 32 bytes for memory.
			
			// scripts NEED memory, if memory is invalid then we can't use the script.
			if( !script->pbMemory ) {
				printf("Tagha_load_script :: Failed to allocate memory for script\n");
				TaghaScript_free(script);
				script = NULL;
				goto error;
			}
			bytecount += sizeof(uint32_t);
			
			// set both the integer and pointer stack pointers to point at the top of the stack.
			script->sp = script->bp = script->uiMemsize-1;
			script->SP = script->BP = (script->pbMemory + script->sp);
			
			// see if the script is using any natives.
			ignore_warns = fread(&script->uiNatives, sizeof(uint32_t), 1, pFile);
			bytecount += sizeof(uint32_t);
			if( script->uiNatives ) {
				// script has natives? Copy their names so we can match them later.
				script->pstrNatives = calloc(script->uiNatives, sizeof(char *));
				for( uint32_t i=0 ; i<script->uiNatives ; i++ ) {
					uint32_t str_size;
					ignore_warns = fread(&str_size, sizeof(uint32_t), 1, pFile);
					bytecount += sizeof(uint32_t);
					
					// allocate memory to hold the native's name.
					script->pstrNatives[i] = calloc(str_size, sizeof(char));
					
					// read in the native's name.
					ignore_warns = fread(script->pstrNatives[i], sizeof(char), str_size, pFile);
					bytecount += str_size;
					printf("Tagha_load_script :: copied native name \'%s\'\n", script->pstrNatives[i]);
				}
			} else script->pstrNatives = NULL;
			
			// see if the script has its own functions.
			// This table is so host or other scripts can call these functions by name or address.
			ignore_warns = fread(&script->uiFuncs, sizeof(uint32_t), 1, pFile);
			bytecount += sizeof(uint32_t);
			if( script->uiFuncs ) {
				// allocate our function hashmap so we can call functions by name.
				script->pmapFuncs = malloc(sizeof(Map_t));
				map_init(script->pmapFuncs);
				
				// copy the function data from the header.
				for( uint32_t i=0 ; i<script->uiFuncs ; i++ ) {
					uint32_t str_size;
					ignore_warns = fread(&str_size, sizeof(uint32_t), 1, pFile);
					bytecount += sizeof(uint32_t);
					
					// allocate the hashmap function key.
					char *strFunc = calloc(str_size, sizeof(char));
					if( !strFunc ) {
						printf("Tagha_load_script :: Failed to allocate memory for strFunc\n");
						TaghaScript_free(script);
						script = NULL;
						goto error;
					}
					ignore_warns = fread(strFunc, sizeof(char), str_size, pFile);
					bytecount += str_size;
					
					// allocate function's data to a table.
					struct FuncTable *pFuncData = malloc(sizeof(struct FuncTable));
					if( !pFuncData ) {
						printf("Tagha_load_script :: Failed to allocate memory for pFuncData\n");
						TaghaScript_free(script);
						script = NULL;
						goto error;
					}
					// copy func's header data to our table
					// then store the table to our function hashmap with the key
					// we allocated earlier.
					ignore_warns = fread(&pFuncData->uiParams, sizeof(uint32_t), 1, pFile);
					bytecount += sizeof(uint32_t);
					ignore_warns = fread(&pFuncData->uiEntry, sizeof(uint32_t), 1, pFile);
					bytecount += sizeof(uint32_t);
					printf("Tagha_load_script :: copied function name \'%s\'\n", strFunc);
					printf("Tagha_load_script :: Function Dict Keyvals { \'%s\': %u }\n", strFunc, pFuncData->uiEntry);
					map_insert(script->pmapFuncs, strFunc, (uintptr_t)pFuncData);
					strFunc = NULL, pFuncData = NULL;
				}
			} else script->pmapFuncs=NULL; //script->pFuncTable = NULL;
			
			// check if the script has global variables.
			ignore_warns = fread(&script->uiGlobals, sizeof(uint32_t), 1, pFile);
			printf("Tagha_load_script :: uiGlobals: \'%u\'\n", script->uiGlobals);
			bytecount += sizeof(uint32_t);
			uint32_t globalbytes = 0;
			if( script->uiGlobals ) {
				// script has globals, allocate our global var hashmap.
				script->pmapGlobals = malloc(sizeof(Map_t));
				map_init(script->pmapGlobals);
				
				for( uint32_t i=0 ; i<script->uiGlobals ; i++ ) {
					uint32_t str_size;
					ignore_warns = fread(&str_size, sizeof(uint32_t), 1, pFile);
					bytecount += sizeof(uint32_t);
					// allocate string to use as a key for our global var.
					char *strGlobal = calloc(str_size, sizeof(char));
					if( !strGlobal ) {
						printf("Tagha_load_script :: Failed to allocate memory for strGlobal\n");
						TaghaScript_free(script);
						script = NULL;
						goto error;
					}
					ignore_warns = fread(strGlobal, sizeof(char), str_size, pFile);
					bytecount += str_size;
					
					// allocate table for our global var's data.
					struct DataTable *pGlobalData = malloc(sizeof(struct DataTable));
					if( !pGlobalData ) {
						printf("Tagha_load_script :: Failed to allocate memory for pGlobalData\n");
						TaghaScript_free(script);
						script = NULL;
						goto error;
					}
					
					// read the global var's data from the header and add it to our hashmap.
					// same procedure as our function hashmap.
					ignore_warns = fread(&pGlobalData->uiAddress, sizeof(uint32_t), 1, pFile);
					bytecount += sizeof(uint32_t);
					ignore_warns = fread(&pGlobalData->uiBytes, sizeof(uint32_t), 1, pFile);
					bytecount += sizeof(uint32_t);
					
					// global var always has intialized data. Copy that data to our stack.
					// copy it by byte.
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
					
					// insert the global var's table to our hashmap.
					map_insert(script->pmapGlobals, strGlobal, (uintptr_t)pGlobalData);
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
			
			// read in our entry point where our code should begin executing.
			ignore_warns = fread(&script->ip, sizeof(uint64_t), 1, pFile);
			printf("Tagha_load_script :: entry ip starts at %" PRIWord "\n", script->ip);
			bytecount += sizeof(uint64_t);
			
			// check if the script is either in safemode or debug mode.
			char boolean;
			ignore_warns = fread(&boolean, sizeof(bool), 1, pFile);
			script->bSafeMode = (boolean & 1) >= 1;
			printf("Tagha_load_script :: Script Safe Mode: %" PRIu32 "\n", script->bSafeMode);
			
			script->bDebugMode = (boolean & 2) >= 1;
			printf("Tagha_load_script :: Script Debug Mode: %" PRIu32 "\n", script->bDebugMode);
			bytecount += sizeof(bool);
			
			printf("Tagha_load_script :: final stack size %" PRIWord "\n", script->sp);
			script->uiMaxInstrs = 0xfffff;	// helps to stop infinite/runaway loops
			
			// header data is finished, subtract the filesize with the bytecount
			// to get the size of our instruction stream.
			// If the instruction stream is invalid, we can't load script.
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
			
			// instruction stream is valid, copy every last instruction and data to the last byte.
			ignore_warns = fread(script->pInstrStream, sizeof(uint8_t), script->uiInstrSize, pFile);
			
			// script data has been verified and loaded to memory.
			// copy script's memory address to VM's script vector.
			vector_add(vm->pvecScripts, script);
			script = NULL;
			printf("\n");
		}
		else {	// invalid script, kill the reference and whatever memory the script has allocated.
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
	
	// kill memory
	if( script->pbMemory )
		free(script->pbMemory);
	script->pbMemory = NULL;
	
	// kill instruction stream
	if( script->pInstrStream )
		free(script->pInstrStream);
	script->pInstrStream = NULL;
	
	// free our native table
	uint32_t i, Size;
	void *pFree;
	if( script->pstrNatives ) {
		for( i=0 ; i<script->uiNatives ; i++ ) {
			if( script->pstrNatives[i] )
				free(script->pstrNatives[i]);
			script->pstrNatives[i] = NULL;
		}
		free(script->pstrNatives);
		script->pstrNatives = NULL;
	}
	// free our function hashmap and all the tables in it.
	if( script->pmapFuncs ) {
		kvnode_t
			*kv = NULL,
			*next = NULL
		;
		Size = script->pmapFuncs->size;
		for( i=0 ; i<Size ; i++ ) {
			for( kv = script->pmapFuncs->table[i] ; kv ; kv = next ) {
				next = kv->pNext;
				pFree = (void *)(uintptr_t)kv->pData;
				if( pFree )
					free(pFree), pFree=NULL, kv->pData=0;
				
				pFree = (char *)kv->strKey;
				if( pFree )
					free(pFree), pFree=NULL, kv->strKey=NULL;
			}
		}
		map_free(script->pmapFuncs);
		free(script->pmapFuncs);
		script->pmapFuncs = NULL;
	}
	// free our global var hashmap and all the tables in it.
	if( script->pmapGlobals ) {
		kvnode_t
			*kv = NULL,
			*next = NULL
		;
		Size = script->pmapGlobals->size;
		for( i=0 ; i<Size ; i++ ) {
			for( kv = script->pmapGlobals->table[i] ; kv ; kv = next ) {
				next = kv->pNext;
				pFree = (void *)(uintptr_t)kv->pData;
				if( pFree )
					free(pFree), pFree=NULL, kv->pData=0;
				
				pFree = (char *)kv->strKey;
				if( pFree )
					free(pFree), pFree=NULL, kv->strKey=NULL;
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
	// set our stack pointer pointers to NULL
	// and release the script itself.
	script->BP = script->SP = NULL;
	free(script);
}

void Tagha_free(struct TaghaVM *vm)
{
	if( !vm )
		return;
	
	// iterate through our script vector and free
	// each script individually. Then set each vector index
	// to NULL so we don't leave a trace of it.
	if( vm->pvecScripts ) {
		uint32_t nScripts = vector_count(vm->pvecScripts);
		struct TaghaScript *script;
		Vec_t *buffer=vm->pvecScripts;
		for( uint32_t n=0 ; n<nScripts ; n++ ) {
			script = vector_get(buffer, n);
			if( !script )
				continue;
			
			TaghaScript_free(script), script=NULL;
			// after freeing scripts, replace their data with NULL.
			vector_set(buffer, n, NULL);
		}
		
		// after we've cleaned up the scripts,
		// free the vector's remaining data then free the vector itself.
		vector_free(vm->pvecScripts);
		free(vm->pvecScripts);
		vm->pvecScripts = NULL;
		buffer = NULL;
	}
	// since the VMs native hashmap has nothing allocated,
	// we just free the hashmap's internal data and then the hashmap itself.
	if( vm->pmapNatives ) {
		map_free(vm->pmapNatives);
		free(vm->pmapNatives);
		vm->pmapNatives = NULL;
	}
	/*
	if( vm->pmapExpTypes ) {
		kvnode_t
			*kv = NULL,
			*next = NULL
		;
		Map_t *pType = NULL;
		uint64_t mapsize = vm->pmapExpTypes->size;
		for( uint32_t i=0 ; i<mapsize ; i++ ) {
			for( kv = vm->pmapExpTypes->table[i] ; kv ; kv = next ) {
				next = kv->pNext;
				pType = (Map_t *)(uintptr_t)kv->pData;
				if( pType )
					map_free(pType), free(pType), pType=NULL;
				kv->pData=0;
			}
		}
		map_free(vm->pmapExpTypes);
		free(vm->pmapExpTypes);
		vm->pmapExpTypes = NULL;
	}
	*/
}


void TaghaScript_reset(struct TaghaScript *script)
{
	if( !script )
		return;
	// resets the script without, hopefully, crashing Tagha and the host.
	// better than a for-loop setting everything to 0.
	memset(script->pbMemory, 0, script->uiMemsize);
	script->sp = script->bp = script->uiMemsize-1;
	script->SP = script->BP = (script->pbMemory + script->sp);
}

bool Tagha_register_natives(struct TaghaVM *restrict vm, struct NativeInfo *restrict arrNatives)
{
	if( !vm or !arrNatives )
		return false;
	else if( !vm->pmapNatives )
		return false;
	
	for( uint32_t i=0 ; arrNatives[i].pFunc != NULL ; i++ )
		map_insert(vm->pmapNatives, arrNatives[i].strName, (uintptr_t)arrNatives[i].pFunc);
	return true;
}

/*
bool Tagha_register_type(struct TaghaVM *restrict vm, const char *restrict strType, struct TypeInfo *arrVals)
{
	if( !vm or !vm->pmapExpTypes )
		return false;
	
	if( !arrVals ) {	// data hiding
		map_insert(vm->pmapExpTypes, strType, (uintptr_t)NULL);
		return true;
	}
	
	Map_t *pmapType = malloc(sizeof(Map_t));
	if( !pmapType )
		return false;
	else map_init(pmapType);
	
	for( uint32_t i=0 ; arrVals[i].strType != NULL ; i++ )
		map_insert(pmapType, arrVals[i].strType, (uint64_t)arrVals[i].ulVal);
	
	map_insert(vm->pmapExpTypes, strType, (uintptr_t)pmapType);
	return true;
}
*/


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
	script->SP -= size;
	*(long double *)(script->SP) = val;
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
	long double val = *(long double *)(script->SP);
	script->sp += size;
	script->SP += size;
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
	script->SP -= size;
	*(uint64_t *)(script->SP) = val;
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
	script->SP += size;
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
	script->SP -= size;
	*(double *)(script->SP) = val;
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
	double val = *(double *)(script->SP);
	script->sp += size;
	script->SP += size;
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
	script->SP -= size;
	*(uint32_t *)(script->SP) = val;
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
	uint32_t val = *(uint32_t *)(script->SP);
	script->sp += size;
	script->SP += size;
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
	script->SP -= size;
	*(float *)(script->SP) = val;
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
	float val = *(float *)(script->SP);
	script->sp += size;
	script->SP += size;
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
	script->SP -= size;
	*(uint16_t *)(script->SP) = val;
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
	uint16_t val = *(uint16_t *)(script->SP);
	script->sp += size;
	script->SP += size;
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
	script->SP -= size;
	*(script->SP) = val;
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
	uint8_t val = *(script->SP);
	script->sp += size;
	script->SP += size;
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
	script->SP -= bytesize;
	memcpy(script->SP, pItem, bytesize);
	/*
	for( uint32_t i=bytesize-1 ; i<bytesize ; i-- )
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
	memcpy(pBuffer, script->SP, bytesize);
	script->sp += bytesize;
	script->SP += bytesize;
	/*
	for( uint32_t i=0 ; i<bytesize ; i++ )
		((uint8_t *)pBuffer)[i] = script->pbMemory[script->sp++];
	*/
}
/*
uint8_t *TaghaScript_addr2ptr(struct TaghaScript *restrict script, const uint64_t stk_address)
{
	if( !script or !script->pbMemory )
		return NULL;
	else if( stk_address >= script->uiMemsize )
		return NULL;
	return( script->pbMemory + stk_address );
}
*/
void TaghaScript_call_func_by_name(struct TaghaScript *restrict script, const char *restrict strFunc)
{
	if( !script or !strFunc )
		return;
	else if( !script->pmapFuncs )
		return;
	
	struct FuncTable *pFuncs = (struct FuncTable *)(uintptr_t)map_find(script->pmapFuncs, strFunc);
	if( pFuncs ) {
		bool debugmode = script->bDebugMode;
		uint32_t func_addr = pFuncs->uiEntry;
		
		if( debugmode )
			printf("TaghaScript_call_func_by_name :: calling address: %" PRIu32 "\n", func_addr);
		
		TaghaScript_push_int64(script, script->ip+1);	// save return address.
		if( debugmode )
			printf("TaghaScript_call_func_by_name :: return addr: %" PRIWord "\n", script->ip+1);
		
		script->ip = func_addr;	// jump to instruction
		
		TaghaScript_push_int64(script, (uintptr_t)script->BP);	// push ebp;
		if( debugmode )
			printf("TaghaScript_call_func_by_name :: pushing bp: %" PRIWord "\n", script->bp);
		script->bp = script->sp;	// mov ebp, esp;
		script->BP = script->SP;
		
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
	
	TaghaScript_push_int64(script, (uintptr_t)script->BP);	// push ebp;
	script->bp = script->sp;	// mov ebp, esp;
	script->BP = script->SP;
}

void *TaghaScript_get_global_by_name(struct TaghaScript *script, const char *strGlobalName)
{
	void *p = NULL;
	if( !script or !script->pmapGlobals )
		return p;
	
	struct DataTable *pGlobals = (struct DataTable *)(uintptr_t)map_find(script->pmapGlobals, strGlobalName);
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
	for( uint32_t i=0 ; i<pVec->count ; i++ ) {
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
	
	uint32_t size = script->uiMemsize;
	for( uint32_t i=0 ; i<size ; i++ )
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
	
	uint32_t size = script->uiInstrSize;
	for( uint32_t i=0 ; i<size ; i++ )
		printf("Instr[%.10"PRIu32"] == %"PRIu32"\n", i, script->pInstrStream[i]);
	printf("\n");
}

void gfree(void **ptr)
{
	if( *ptr )
		free(*ptr);
	*ptr = NULL;
}











