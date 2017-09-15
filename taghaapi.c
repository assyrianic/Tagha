
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <iso646.h>
#include <inttypes.h>
#include <assert.h>
#include <string.h>
#include "tagha.h"


static u64 get_file_size(FILE *pFile)
{
	u64 size = 0L;
	if( !pFile )
		return size;
	
	if( !fseek(pFile, 0, SEEK_END) ) {
		size = (u64)ftell(pFile);
		rewind(pFile);
	}
	return size;
}

void Tagha_init(TaghaVM_t *restrict vm)
{
	if( !vm )
		return;
	
	vm->pvecScripts = malloc(sizeof(vector));
	if( !vm->pvecScripts )
		printf("[Tagha Error]: **** Unable to initialize Script Vector ****\n");
	else vector_init(vm->pvecScripts);
	
	vm->pmapNatives = malloc(sizeof(dict));
	if( !vm->pmapNatives )
		printf("[Tagha Error]: **** Unable to initialize Native Map ****\n");
	else dict_init(vm->pmapNatives);
}

void Tagha_load_script(TaghaVM_t *restrict vm, char *restrict filename)
{
	if( !vm )
		return;
	
	FILE *pFile = fopen(filename, "rb");
	if( !pFile ) {
		printf("[Tagha Error]: File not found: \'%s\'\n", filename);
		return;
	}
	
	Script_t *script = malloc(sizeof(Script_t));
	if( script ) {
		script->pInstrStream = NULL;
		script->pbMemory = NULL;
		script->ppstrNatives = NULL;
		script->pFuncTable = NULL;
		
		//fclose(pFile); pFile=NULL;
		u64 filesize = get_file_size(pFile);
		uint bytecount = 0;
		ushort verify;
		fread(&verify, sizeof(ushort), 1, pFile);
		
		// verify that this is executable code.
		if( verify == 0xC0DE ) {
			printf("Tagha_load_script :: verified code!\n");
			bytecount += 2;
			
			fread(&script->uiMemsize, sizeof(uint), 1, pFile);
			printf("Tagha_load_script :: Memory Size: %" PRIu32 "\n", script->uiMemsize);
			if( script->uiMemsize )
				script->pbMemory = calloc(script->uiMemsize, sizeof(uchar));
			else script->pbMemory = calloc(32, sizeof(uchar));	// have a default size of 32 bytes for memory.
			if( !script->pbMemory ) {
				printf("Tagha_load_script :: Failed to allocate memory for script\n");
				TaghaScript_free(script);
				script = NULL;
				goto error;
			}
			bytecount += 4;
			
			fread(&script->uiNatives, sizeof(uint), 1, pFile);
			bytecount += 4;
			if( script->uiNatives ) {
				script->ppstrNatives = calloc(script->uiNatives, sizeof(char *));
				uint i;
				for( i=0 ; i<script->uiNatives ; i++ ) {
					uint str_size;
					fread(&str_size, sizeof(uint), 1, pFile);
					bytecount += 4;
					script->ppstrNatives[i] = calloc(str_size, sizeof(char));
					uint n = 0;
					char c;
					while( n<str_size ) {
						fread(&c, sizeof(char), 1, pFile);
						script->ppstrNatives[i][n++] = c;
						++bytecount;
					}
					printf("Tagha_load_script :: copied native name \'%s\'\n", script->ppstrNatives[i]);
				}
			} else script->ppstrNatives = NULL;
			
			fread(&script->uiFuncs, sizeof(uint), 1, pFile);
			bytecount += 4;
			if( script->uiFuncs ) {
				script->pFuncTable = calloc(script->uiFuncs, sizeof(FuncTable_t));
				uint i;
				for( i=0 ; i<script->uiFuncs ; i++ ) {
					uint str_size;
					fread(&str_size, sizeof(uint), 1, pFile);
					bytecount += 4;
					script->pFuncTable[i].pFuncName = calloc(str_size, sizeof(char));
					uint n = 0;
					char c;
					while( n<str_size ) {
						fread(&c, sizeof(char), 1, pFile);
						script->pFuncTable[i].pFuncName[n++] = c;
						++bytecount;
					}
					
					fread(&script->pFuncTable[i].uiParams, sizeof(uint), 1, pFile);
					bytecount += 4;
					fread(&script->pFuncTable[i].uiEntry, sizeof(uint), 1, pFile);
					bytecount += 4;
					printf("Tagha_load_script :: copied function name \'%s\'\n", script->pFuncTable[i].pFuncName);
				}
			} else script->pFuncTable = NULL;
			
			fread(&script->ip, sizeof(uint), 1, pFile);
			printf("Tagha_load_script :: ip starts at %" PRIu32 "\n", script->ip);
			bytecount += 4;
			
			script->bSafeMode = true;
			script->bDebugMode = true;
			script->sp = script->bp = script->uiMemsize-1;
			script->uiMaxInstrs = 0xfffff;	// helps to stop infinite/runaway loops
			
			printf("Tagha_load_script :: header bytecount at %" PRIu32 "\n", bytecount);
			script->uiInstrSize = filesize - bytecount;
			printf("Tagha_load_script :: instr_size at %" PRIu32 "\n", script->uiInstrSize);
			script->pInstrStream = calloc(script->uiInstrSize, sizeof(uchar));
			if( !script->pInstrStream ) {
				printf("Tagha_load_script :: ERROR! Could not allocate Instruction Stream!\n");
				TaghaScript_free(script);
				script = NULL;
				goto error;
			}
			
			fread(script->pInstrStream, sizeof(uchar), script->uiInstrSize, pFile);
			// transfer script address to the vector.
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

void TaghaScript_free(Script_t *script)
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
	uint i;
	if( script->ppstrNatives ) {
		for( i=0 ; i<script->uiNatives ; i++ ) {
			if( script->ppstrNatives[i] )
				free(script->ppstrNatives[i]);
			script->ppstrNatives[i] = NULL;
		}
		free(script->ppstrNatives);
		script->ppstrNatives = NULL;
	}
	if( script->pFuncTable ) {
		for( i=0 ; i<script->uiFuncs ; i++ ) {
			if( script->pFuncTable[i].pFuncName )
				free(script->pFuncTable[i].pFuncName);
			script->pFuncTable[i].pFuncName = NULL;
		}
		free(script->pFuncTable);
		script->pFuncTable = NULL;
	}
	free(script);
}

void Tagha_free(TaghaVM_t *vm)
{
	if( !vm )
		return;
	if( vm->pvecScripts ) {
		uint n=0;
		uint nScripts = vector_count(vm->pvecScripts);
		Script_t *script;
		vector *buffer=vm->pvecScripts;
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
		dict_free(vm->pmapNatives, false);
		free(vm->pmapNatives);
		vm->pmapNatives = NULL;
	}
}


void TaghaScript_reset(Script_t *script)
{
	if( !script )
		return;
	
	uint i;
	for( i=0 ; i<script->uiMemsize ; i++ )
		script->pbMemory[i] = 0;
	
	script->sp = script->bp = script->uiMemsize - 1;
}

bool Tagha_register_natives(TaghaVM_t *restrict vm, NativeInfo_t *restrict arrNatives)
{
	if( !vm or !arrNatives )
		return false;
	else if( !vm->pmapNatives )
		return false;
	
	Word_t i;
	for( i=0 ; arrNatives[i].pFunc != NULL ; i++ )
		dict_insert(vm->pmapNatives, arrNatives[i].strName, (void *)arrNatives[i].pFunc);
	return true;
}



void TaghaScript_push_longfloat(Script_t *restrict script, const long double val)
{
	if( !script )
		return;
	
	// long doubles are usually 12 or 16 bytes, adjust for both.
	uint ldsize = sizeof(long double);
	if( script->bSafeMode and (script->sp-ldsize) >= script->uiMemsize ) {
		printf("TaghaScript_push_longfloat reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp-ldsize);
		exit(1);
	}
	script->sp -= ldsize;
	*(long double *)(script->pbMemory + script->sp) = val;
}

long double TaghaScript_pop_longfloat(Script_t *script)
{
	if( !script )
		return 0;
	
	uint ldsize = sizeof(long double);
	if( script->bSafeMode and (script->sp+ldsize) >= script->uiMemsize ) {
		printf("TaghaScript_pop_longfloat reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+ldsize);
		exit(1);
	}
	long double val = *(long double *)(script->pbMemory + script->sp);
	script->sp += ldsize;
	return val;
}

void TaghaScript_push_int64(Script_t *restrict script, const u64 val)
{
	if( !script )
		return;
	if( script->bSafeMode and (script->sp-8) >= script->uiMemsize ) {
		printf("TaghaScript_push_int64 reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp-8);
		return;
	}
	script->sp -= 8;
	*(u64 *)(script->pbMemory + script->sp) = val;
}
u64 TaghaScript_pop_int64(Script_t *script)
{
	if( !script )
		return 0L;
	if( script->bSafeMode and (script->sp+8) >= script->uiMemsize ) {
		printf("TaghaScript_pop_int64 reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+8);
		return 0L;
	}
	u64 val = *(u64 *)(script->pbMemory + script->sp);
	script->sp += 8;
	return val;
}

void TaghaScript_push_float64(Script_t *restrict script, const double val)
{
	if( !script )
		return;
	if( script->bSafeMode and (script->sp-8) >= script->uiMemsize ) {
		printf("TaghaScript_push_float64 reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp-8);
		return;
	}
	script->sp -= 8;
	*(double *)(script->pbMemory + script->sp) = val;
}
double TaghaScript_pop_float64(Script_t *script)
{
	if( !script )
		return 0;
	if( script->bSafeMode and (script->sp+8) >= script->uiMemsize ) {
		printf("TaghaScript_pop_float64 reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+8);
		return 0;
	}
	double val = *(double *)(script->pbMemory + script->sp);
	script->sp += 8;
	return val;
}

void TaghaScript_push_int32(Script_t *restrict script, const uint val)
{
	if( !script )
		return;
	if( script->bSafeMode and (script->sp-4) >= script->uiMemsize ) {
		printf("TaghaScript_push_int32 reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp-4);
		return;
	}
	script->sp -= 4;
	*(uint *)(script->pbMemory + script->sp) = val;
}
uint TaghaScript_pop_int32(Script_t *script)
{
	if( !script )
		return 0;
	if( script->bSafeMode and (script->sp+4) >= script->uiMemsize ) {	// we're subtracting, did we integer underflow?
		printf("TaghaScript_pop_int32 reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+4);
		return 0;
	}
	uint val = *(uint *)(script->pbMemory + script->sp);
	script->sp += 4;
	return val;
}

void TaghaScript_push_float32(Script_t *restrict script, const float val)
{
	if( !script )
		return;
	if( script->bSafeMode and (script->sp-4) >= script->uiMemsize ) {
		printf("TaghaScript_push_float32 reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp-4);
		return;
	}
	script->sp -= 4;
	*(float *)(script->pbMemory + script->sp) = val;
}
float TaghaScript_pop_float32(Script_t *script)
{
	if( !script )
		return 0;
	if( script->bSafeMode and (script->sp+4) >= script->uiMemsize ) {
		printf("TaghaScript_pop_float32 reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+4);
		return 0;
	}
	float val = *(float *)(script->pbMemory + script->sp);
	script->sp += 4;
	return val;
}

void TaghaScript_push_short(Script_t *restrict script, const ushort val)
{
	if( !script )
		return;
	if( script->bSafeMode and (script->sp-2) >= script->uiMemsize ) {
		printf("TaghaScript_push_short reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp-2);
		return;
	}
	script->sp -= 2;
	*(ushort *)(script->pbMemory + script->sp) = val;
}
ushort TaghaScript_pop_short(Script_t *script)
{
	if( !script )
		return 0;
	if( script->bSafeMode and (script->sp+2) >= script->uiMemsize ) {
		printf("TaghaScript_pop_short reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+2);
		return 0;
	}
	ushort val = *(ushort *)(script->pbMemory + script->sp);
	script->sp += 2;
	return val;
}

void TaghaScript_push_byte(Script_t *restrict script, const uchar val)
{
	if( !script )
		return;
	if( script->bSafeMode and (script->sp-1) >= script->uiMemsize ) {
		printf("TaghaScript_push_byte reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp-1);
		return;
	}
	script->sp--;
	script->pbMemory[script->sp] = val;
}
uchar TaghaScript_pop_byte(Script_t *script)
{
	if( !script )
		return 0;
	if( script->bSafeMode and (script->sp+1) >= script->uiMemsize ) {
		printf("TaghaScript_pop_byte reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+1);
		return 0;
	}
	uchar val = script->pbMemory[script->sp];
	script->sp++;
	return val;
}

void TaghaScript_push_nbytes(Script_t *restrict script, void *restrict pItem, const Word_t bytesize)
{
	if( !script )
		return;
	if( script->bSafeMode and (script->sp-bytesize) >= script->uiMemsize ) {
		printf("TaghaScript_push_nbytes reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp-bytesize);
		return;
	}
	Word_t i=0;
	//for( i=0 ; i<bytesize ; i++ )
	for( i=bytesize-1 ; i<bytesize ; i-- )
		script->pbMemory[--script->sp] = ((uchar *)pItem)[i];
}
void TaghaScript_pop_nbytes(Script_t *restrict script, void *restrict pBuffer, const Word_t bytesize)
{
	if( !script )
		return;
	if( script->bSafeMode and (script->sp+bytesize) >= script->uiMemsize ) {
		printf("TaghaScript_pop_nbytes reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+bytesize);
		return;
	}
	Word_t i=0;
	// should stop when the integer underflows
	//for( i=bytesize-1 ; i<bytesize ; i-- )
	for( i=0 ; i<bytesize ; i++ )
		((uchar *)pBuffer)[i] = script->pbMemory[script->sp++];
}

uchar *TaghaScript_addr2ptr(Script_t *restrict script, const Word_t stk_address)
{
	if( !script )
		return NULL;
	else if( !script->pbMemory )
		return NULL;
	else if( stk_address >= script->uiMemsize )
		return NULL;
	return( script->pbMemory + stk_address );
}

void TaghaScript_call_func(Script_t *restrict script, const char *restrict funcname)
{
	if( !script or !funcname )
		return;
	else if( !script->pFuncTable )
		return;
	
	FuncTable_t	*pFuncs = script->pFuncTable;
	uint i;
	bool bfound=false;
	for( i=0 ; i<script->uiFuncs ; i++ ) {
		if( !strcmp(funcname, pFuncs[i].pFuncName) ) {
			bfound=true;
			break;
		}
	}
	if( bfound ) {
	}
	pFuncs=NULL;
}


void TaghaScript_debug_print_memory(const Script_t *script)
{
	if( !script )
		return;
	else if( !script->pbMemory )
		return;
	
	printf("DEBUG ...---===---... Printing Memory...\n");
	uint i;
	uint size = script->uiMemsize;
	for( i=0 ; i<size ; i++ )
		if( script->sp == i )
			printf("T.O.S. : Memory[%" PRIu32 "] == %" PRIu32 "\n", i, script->pbMemory[i]);
		else printf("Memory[%" PRIu32 "] == %" PRIu32 "\n", i, script->pbMemory[i]);
	printf("\n");
}
void TaghaScript_debug_print_ptrs(const Script_t *script)
{
	if( !script )
		return;
	
	printf("DEBUG ...---===---... Printing Pointers...\n");
	printf("Instruction Pointer: %" PRIu32 "\
			\nStack Pointer: %" PRIu32 "\
			\nStack Frame Pointer: %" PRIu32 "\n", script->ip, script->sp, script->bp);
	printf("\n");
}
void TaghaScript_debug_print_instrs(const Script_t *script)
{
	if( !script )
		return;
	
	uint i;
	for( i=0 ; i<script->uiInstrSize ; i++ )
		printf("Instr[%"PRIu32"] == %"PRIu32"\n", i, script->pInstrStream[i]);
	printf("\n");
}
















