
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <iso646.h>
#include <inttypes.h>
#include <assert.h>
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
	
	vm->pScript = NULL;
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
	
	vm->pScript = malloc(sizeof(Script_t));
	if( vm->pScript ) {
		Script_t *script = vm->pScript;
		script->pInstrStream = NULL;
		script->pbMemory = NULL;
		script->ppstrNatives = NULL;
		
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
				printf("Tagha_load_script :: Failed to allocate global memory for script\n");
				Tagha_free(vm);
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
					printf("Tagha_load_script :: copied native name %s\n", script->ppstrNatives[i]);
				}
			} else script->ppstrNatives = NULL;
			
			fread(&script->ip, sizeof(uint), 1, pFile);
			printf("Tagha_load_script :: ip starts at %" PRIu32 "\n", script->ip);
			bytecount += 4;
			
			script->bSafeMode = true;
			script->sp = script->bp = 0;
			script->uiMaxInstrs = 0xfffff;	// helps to stop infinite/runaway loops
			
			printf("Tagha_load_script :: header bytecount at %" PRIu32 "\n", bytecount);
			script->uiInstrSize = filesize - bytecount;
			printf("Tagha_load_script :: instr_size at %" PRIu32 "\n", script->uiInstrSize);
			script->pInstrStream = calloc(script->uiInstrSize, sizeof(uchar));
			if( !script->pInstrStream ) {
				printf("Tagha_load_script :: ERROR! Could not allocate Instruction Stream!\n");
				Tagha_free(vm);
				script = NULL;
				goto error;
			}
			fread(script->pInstrStream, sizeof(uchar), script->uiInstrSize, pFile);
			script = NULL;
		}
		else {	// invalid script, kill the reference and the script itself.
			printf("Tagha_load_script :: unknown file memory format\n");
			Tagha_free(vm);
			script = NULL;
		}
	}
	else vm->pScript = NULL;
error:;
	fclose(pFile); pFile=NULL;
}

void Tagha_free(TaghaVM_t *vm)
{
	if( !vm )
		return;
	if( vm->pScript ) {
		Script_t *script = vm->pScript;
		
		if( script->pbMemory )
			free(script->pbMemory);
		script->pbMemory = NULL;
		
		if( script->pInstrStream )
			free(script->pInstrStream);
		script->pInstrStream = NULL;
		
		if( script->ppstrNatives ) {
			uint i;
			for( i=0 ; i<script->uiNatives ; i++ ) {
				if( script->ppstrNatives[i] )
					free(script->ppstrNatives[i]);
				script->ppstrNatives[i] = NULL;
			}
			free(script->ppstrNatives);
			script->ppstrNatives = NULL;
		}
		free(vm->pScript);
		vm->pScript = NULL;
		script = NULL;
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
	
	script->sp = script->bp = 0;
}


int Tagha_register_natives(TaghaVM_t *restrict vm, NativeInfo_t *arrNatives)
{
	if( !vm or !arrNatives )
		return 0;
	else if( !vm->pmapNatives )
		return 0;
	
	Word_t i;
	for( i=0 ; arrNatives[i].pFunc != NULL ; i++ )
		dict_insert(vm->pmapNatives, arrNatives[i].strName, (void *)arrNatives[i].pFunc);
	return 1;
}



void TaghaScript_push_longfloat(Script_t *restrict script, const long double val)
{
	if( !script )
		return;
	
	// long doubles are usually 12 or 16 bytes, adjust for both.
	uint ldsize = sizeof(long double);
	if( script->bSafeMode and (script->sp+ldsize) >= script->uiMemsize ) {
		printf("TaghaScript_push_longfloat reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+ldsize);
		exit(1);
	}
	long double longdbl = val;
	uint i = 0;
	while( i<ldsize )
		script->pbMemory[++script->sp] = ((uchar *)&longdbl)[i++];
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
	long double longdbl;
	uint i = ldsize-1;
	while( i<ldsize )
		((uchar *)&longdbl)[i--] = script->pbMemory[script->sp--];
	return longdbl;
}

void TaghaScript_push_int64(Script_t *restrict script, const u64 val)
{
	if( !script )
		return;
	if( script->bSafeMode and (script->sp+8) >= script->uiMemsize ) {
		printf("TaghaScript_push_int64 reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+8);
		exit(1);
	}
	union conv_union conv;
	conv.ull = val;
	script->pbMemory[++script->sp] = conv.c[0];
	script->pbMemory[++script->sp] = conv.c[1];
	script->pbMemory[++script->sp] = conv.c[2];
	script->pbMemory[++script->sp] = conv.c[3];
	script->pbMemory[++script->sp] = conv.c[4];
	script->pbMemory[++script->sp] = conv.c[5];
	script->pbMemory[++script->sp] = conv.c[6];
	script->pbMemory[++script->sp] = conv.c[7];
}
u64 TaghaScript_pop_int64(Script_t *script)
{
	if( !script )
		return 0L;
	if( script->bSafeMode and (script->sp-8) >= script->uiMemsize ) {
		printf("TaghaScript_pop_int64 reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp-8);
		exit(1);
	}
	union conv_union conv;
	conv.c[7] = script->pbMemory[script->sp--];
	conv.c[6] = script->pbMemory[script->sp--];
	conv.c[5] = script->pbMemory[script->sp--];
	conv.c[4] = script->pbMemory[script->sp--];
	conv.c[3] = script->pbMemory[script->sp--];
	conv.c[2] = script->pbMemory[script->sp--];
	conv.c[1] = script->pbMemory[script->sp--];
	conv.c[0] = script->pbMemory[script->sp--];
	return conv.ull;
}

void TaghaScript_push_float64(Script_t *restrict script, const double val)
{
	if( !script )
		return;
	if( script->bSafeMode and (script->sp+8) >= script->uiMemsize ) {
		printf("TaghaScript_push_float64 reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+8);
		exit(1);
	}
	union conv_union conv;
	conv.dbl = val;
	script->pbMemory[++script->sp] = conv.c[0];
	script->pbMemory[++script->sp] = conv.c[1];
	script->pbMemory[++script->sp] = conv.c[2];
	script->pbMemory[++script->sp] = conv.c[3];
	script->pbMemory[++script->sp] = conv.c[4];
	script->pbMemory[++script->sp] = conv.c[5];
	script->pbMemory[++script->sp] = conv.c[6];
	script->pbMemory[++script->sp] = conv.c[7];
}
double TaghaScript_pop_float64(Script_t *script)
{
	if( !script )
		return 0;
	if( script->bSafeMode and (script->sp-8) >= script->uiMemsize ) {
		printf("TaghaScript_pop_float64 reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp-8);
		exit(1);
	}
	union conv_union conv;
	conv.c[7] = script->pbMemory[script->sp--];
	conv.c[6] = script->pbMemory[script->sp--];
	conv.c[5] = script->pbMemory[script->sp--];
	conv.c[4] = script->pbMemory[script->sp--];
	conv.c[3] = script->pbMemory[script->sp--];
	conv.c[2] = script->pbMemory[script->sp--];
	conv.c[1] = script->pbMemory[script->sp--];
	conv.c[0] = script->pbMemory[script->sp--];
	return conv.dbl;
}

void TaghaScript_push_int32(Script_t *restrict script, const uint val)
{
	if( !script )
		return;
	if( script->bSafeMode and (script->sp+4) >= script->uiMemsize ) {
		printf("TaghaScript_push_int32 reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+4);
		exit(1);
	}
	union conv_union conv;
	conv.ui = val;
	script->pbMemory[++script->sp] = conv.c[0];
	script->pbMemory[++script->sp] = conv.c[1];
	script->pbMemory[++script->sp] = conv.c[2];
	script->pbMemory[++script->sp] = conv.c[3];
}
uint TaghaScript_pop_int32(Script_t *script)
{
	if( !script )
		return 0;
	if( script->bSafeMode and (script->sp-4) >= script->uiMemsize ) {	// we're subtracting, did we integer underflow?
		printf("TaghaScript_pop_int32 reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp-4);
		exit(1);
	}
	union conv_union conv;
	conv.c[3] = script->pbMemory[script->sp--];
	conv.c[2] = script->pbMemory[script->sp--];
	conv.c[1] = script->pbMemory[script->sp--];
	conv.c[0] = script->pbMemory[script->sp--];
	return conv.ui;
}

void TaghaScript_push_float32(Script_t *restrict script, const float val)
{
	if( !script )
		return;
	if( script->bSafeMode and (script->sp+4) >= script->uiMemsize ) {
		printf("TaghaScript_push_float32 reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+4);
		exit(1);
	}
	union conv_union conv;
	conv.f = val;
	script->pbMemory[++script->sp] = conv.c[0];
	script->pbMemory[++script->sp] = conv.c[1];
	script->pbMemory[++script->sp] = conv.c[2];
	script->pbMemory[++script->sp] = conv.c[3];
}
float TaghaScript_pop_float32(Script_t *script)
{
	if( !script )
		return 0;
	if( script->bSafeMode and (script->sp-4) >= script->uiMemsize ) {
		printf("TaghaScript_pop_float32 reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp-4);
		exit(1);
	}
	union conv_union conv;
	conv.c[3] = script->pbMemory[script->sp--];
	conv.c[2] = script->pbMemory[script->sp--];
	conv.c[1] = script->pbMemory[script->sp--];
	conv.c[0] = script->pbMemory[script->sp--];
	return conv.f;
}

void TaghaScript_push_short(Script_t *restrict script, const ushort val)
{
	if( !script )
		return;
	if( script->bSafeMode and (script->sp+2) >= script->uiMemsize ) {
		printf("TaghaScript_push_short reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+2);
		exit(1);
	}
	union conv_union conv;
	conv.us = val;
	script->pbMemory[++script->sp] = conv.c[0];
	script->pbMemory[++script->sp] = conv.c[1];
}
ushort TaghaScript_pop_short(Script_t *script)
{
	if( !script )
		return 0;
	if( script->bSafeMode and (script->sp-2) >= script->uiMemsize ) {
		printf("TaghaScript_pop_short reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp-2);
		exit(1);
	}
	union conv_union conv;
	conv.c[1] = script->pbMemory[script->sp--];
	conv.c[0] = script->pbMemory[script->sp--];
	return conv.us;
}

void TaghaScript_push_byte(Script_t *restrict script, const uchar val)
{
	if( !script )
		return;
	if( script->bSafeMode and (script->sp+1) >= script->uiMemsize ) {
		printf("TaghaScript_push_byte reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+1);
		exit(1);
	}
	script->pbMemory[++script->sp] = val;
}
uchar TaghaScript_pop_byte(Script_t *script)
{
	if( !script )
		return 0;
	if( script->bSafeMode and (script->sp-1) >= script->uiMemsize ) {
		printf("TaghaScript_pop_byte reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp-1);
		exit(1);
	}
	return script->pbMemory[script->sp--];
}

void TaghaScript_push_nbytes(Script_t *restrict script, void *restrict pItem, const Word_t bytesize)
{
	if( !script )
		return;
	if( script->bSafeMode and (script->sp+bytesize) >= script->uiMemsize ) {
		printf("TaghaScript_push_nbytes reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+1);
		exit(1);
	}
	Word_t i=0;
	for( i=0 ; i<bytesize ; i++ )
		script->pbMemory[++script->sp] = ((uchar *)pItem)[i];
}
void TaghaScript_pop_nbytes(Script_t *restrict script, void *restrict pBuffer, const Word_t bytesize)
{
	if( !script )
		return;
	if( script->bSafeMode and (script->sp-bytesize) >= script->uiMemsize ) {
		printf("TaghaScript_pop_nbytes reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+1);
		exit(1);
	}
	Word_t i=0;
	// should stop when the integer underflows
	for( i=bytesize-1 ; i<bytesize ; i-- )
		((uchar *)pBuffer)[i] = script->pbMemory[script->sp--];
}

uchar *TaghaScript_addr2ptr(Script_t *restrict script, const Word_t byteoffset, const Word_t stk_address)
{
	if( !script )
		return NULL;
	else if( !script->pbMemory )
		return NULL;
	
	return( script->pbMemory + (stk_address-(byteoffset-1)) );
}


void TaghaScript_debug_print_memory(const Script_t *script)
{
	if( !script )
		return;
	else if( !script->pbMemory )
		return;
	
	printf("DEBUG ...---===---... Printing Memory...\n");
	uint i;
	for( i=0 ; i<script->uiMemsize ; i++ )
		if( script->sp == i )
			printf("T.O.S. : Memory[0x%x] == %" PRIu32 "\n", i, script->pbMemory[i]);
		else printf("Memory[0x%x] == %" PRIu32 "\n", i, script->pbMemory[i]);
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
















