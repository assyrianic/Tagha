
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <iso646.h>
#include <inttypes.h>
#include <assert.h>
#include "vm.h"


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
		// TODO:
		// improve this code so that we only retrieve the instruction stream and not header data.
		// Instruction stream should be exclusively that, the instruction stream, not instructions + header data.
		// 
		Script_t *script = vm->pScript;
		u64 size = get_file_size(pFile);
		script->pInstrStream = calloc(size, sizeof(uchar));
		if( !script->pInstrStream ) {
			printf("Tagha_load_script :: ERROR! Could not allocate Instruction Stream!\n");
			Tagha_free(vm);
			script = NULL;
			return;
		}
		fread(script->pInstrStream, sizeof(uchar), size, pFile);
		fclose(pFile); pFile=NULL;
		
		uchar *verify = script->pInstrStream;
		// verify that this is executable code.
		if( *(ushort *)verify == 0xC0DE ) {
			printf("Tagha_load_script :: verified code!\n");
			verify += 2;
			script->ip = *(uint *)verify; verify += 4;
			printf("Tagha_load_script :: ip starts at %" PRIu32 "\n", script->ip);
			
			script->uiMemsize = *(uint *)verify; verify += 4;
			printf("Tagha_load_script :: Memory Size: %" PRIu32 "\n", script->uiMemsize);
			if( script->uiMemsize )
				script->pbMemory = calloc(script->uiMemsize, sizeof(uchar));
			else script->pbMemory = calloc(32, sizeof(uchar));	// have a default size of 32 bytes for memory.
			assert( script->pbMemory );
			
			script->uiStksize = *(uint *)verify; verify += 4;
			printf("Tagha_load_script :: Stack Size: %" PRIu32 "\n", script->uiStksize);
			if( script->uiStksize )
				script->pbStack = calloc(script->uiStksize, sizeof(uchar));
			else script->pbStack = calloc(32, sizeof(uchar));	// have a default size of 32 bytes for stack.
			assert( script->pbStack );
			
			script->uiNatives = *(uint *)verify; verify += 4;
			if( script->uiNatives ) {
				script->ppstrNatives = calloc(script->uiNatives, sizeof(char *));
				uint i;
				for( i=0 ; i<script->uiNatives ; i++ ) {
					uint size = *(uint *)verify; verify += 4;
					script->ppstrNatives[i] = calloc(size, sizeof(char));
					uint n = 0;
					while( *(char *)verify != 0 )
						script->ppstrNatives[i][n++] = *(char *)verify++;
					script->ppstrNatives[i][size-1] = *(char *)verify++;
				}
			} else script->ppstrNatives = NULL;
			
			script->bSafeMode = true;
			script->sp = script->bp = 0;
			script->uiMaxInstrs = 200;	// helps to stop infinite/runaway loops
			script = NULL;
		}
		else {	// invalid script, kill the reference and the script itself.
			printf("Tagha_load_script :: unknown file memory format\n");
			Tagha_free(vm);
			script = NULL;
		}
	}
	else vm->pScript = NULL;
}

void Tagha_free(TaghaVM_t *vm)
{
	if( !vm )
		return;
	if( vm->pScript ) {
		Script_t *script = vm->pScript;
	
		if( script->pbStack )
			free(script->pbStack);
		script->pbStack = NULL;
		
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
	for( i=0 ; i<script->uiStksize ; i++ )
		script->pbStack[i] = 0;
	
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
	if( script->bSafeMode and (script->sp+ldsize) >= script->uiStksize ) {
		printf("TaghaScript_push_longfloat reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+ldsize);
		exit(1);
	}
	long double longdbl = val;
	uint i = 0;
	while( i<ldsize )
		script->pbStack[++script->sp] = ((uchar *)&longdbl)[i++];
}

long double TaghaScript_pop_longfloat(Script_t *script)
{
	if( !script )
		return 0;
	uint ldsize = sizeof(long double);
	if( script->bSafeMode and (script->sp+ldsize) >= script->uiStksize ) {
		printf("TaghaScript_pop_longfloat reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+ldsize);
		exit(1);
	}
	long double longdbl;
	uint i = ldsize-1;
	while( i<ldsize )
		((uchar *)&longdbl)[i--] = script->pbStack[script->sp--];
	return longdbl;
}

void TaghaScript_push_int64(Script_t *restrict script, const u64 val)
{
	if( !script )
		return;
	if( script->bSafeMode and (script->sp+8) >= script->uiStksize ) {
		printf("TaghaScript_push_int64 reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+8);
		exit(1);
	}
	union conv_union conv;
	conv.ull = val;
	script->pbStack[++script->sp] = conv.c[0];
	script->pbStack[++script->sp] = conv.c[1];
	script->pbStack[++script->sp] = conv.c[2];
	script->pbStack[++script->sp] = conv.c[3];
	script->pbStack[++script->sp] = conv.c[4];
	script->pbStack[++script->sp] = conv.c[5];
	script->pbStack[++script->sp] = conv.c[6];
	script->pbStack[++script->sp] = conv.c[7];
}
u64 TaghaScript_pop_int64(Script_t *script)
{
	if( !script )
		return 0L;
	if( script->bSafeMode and (script->sp-8) >= script->uiStksize ) {
		printf("TaghaScript_pop_int64 reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp-8);
		exit(1);
	}
	union conv_union conv;
	conv.c[7] = script->pbStack[script->sp--];
	conv.c[6] = script->pbStack[script->sp--];
	conv.c[5] = script->pbStack[script->sp--];
	conv.c[4] = script->pbStack[script->sp--];
	conv.c[3] = script->pbStack[script->sp--];
	conv.c[2] = script->pbStack[script->sp--];
	conv.c[1] = script->pbStack[script->sp--];
	conv.c[0] = script->pbStack[script->sp--];
	return conv.ull;
}

void TaghaScript_push_float64(Script_t *restrict script, const double val)
{
	if( !script )
		return;
	if( script->bSafeMode and (script->sp+8) >= script->uiStksize ) {
		printf("TaghaScript_push_float64 reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+8);
		exit(1);
	}
	union conv_union conv;
	conv.dbl = val;
	script->pbStack[++script->sp] = conv.c[0];
	script->pbStack[++script->sp] = conv.c[1];
	script->pbStack[++script->sp] = conv.c[2];
	script->pbStack[++script->sp] = conv.c[3];
	script->pbStack[++script->sp] = conv.c[4];
	script->pbStack[++script->sp] = conv.c[5];
	script->pbStack[++script->sp] = conv.c[6];
	script->pbStack[++script->sp] = conv.c[7];
}
double TaghaScript_pop_float64(Script_t *script)
{
	if( !script )
		return 0;
	if( script->bSafeMode and (script->sp-8) >= script->uiStksize ) {
		printf("TaghaScript_pop_float64 reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp-8);
		exit(1);
	}
	union conv_union conv;
	conv.c[7] = script->pbStack[script->sp--];
	conv.c[6] = script->pbStack[script->sp--];
	conv.c[5] = script->pbStack[script->sp--];
	conv.c[4] = script->pbStack[script->sp--];
	conv.c[3] = script->pbStack[script->sp--];
	conv.c[2] = script->pbStack[script->sp--];
	conv.c[1] = script->pbStack[script->sp--];
	conv.c[0] = script->pbStack[script->sp--];
	return conv.dbl;
}

void TaghaScript_push_int32(Script_t *restrict script, const uint val)
{
	if( !script )
		return;
	if( script->bSafeMode and (script->sp+4) >= script->uiStksize ) {
		printf("TaghaScript_push_int32 reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+4);
		exit(1);
	}
	union conv_union conv;
	conv.ui = val;
	script->pbStack[++script->sp] = conv.c[0];
	script->pbStack[++script->sp] = conv.c[1];
	script->pbStack[++script->sp] = conv.c[2];
	script->pbStack[++script->sp] = conv.c[3];
}
uint TaghaScript_pop_int32(Script_t *script)
{
	if( !script )
		return 0;
	if( script->bSafeMode and (script->sp-4) >= script->uiStksize ) {	// we're subtracting, did we integer underflow?
		printf("TaghaScript_pop_int32 reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp-4);
		exit(1);
	}
	union conv_union conv;
	conv.c[3] = script->pbStack[script->sp--];
	conv.c[2] = script->pbStack[script->sp--];
	conv.c[1] = script->pbStack[script->sp--];
	conv.c[0] = script->pbStack[script->sp--];
	return conv.ui;
}

void TaghaScript_push_float32(Script_t *restrict script, const float val)
{
	if( !script )
		return;
	if( script->bSafeMode and (script->sp+4) >= script->uiStksize ) {
		printf("TaghaScript_push_float32 reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+4);
		exit(1);
	}
	union conv_union conv;
	conv.f = val;
	script->pbStack[++script->sp] = conv.c[0];
	script->pbStack[++script->sp] = conv.c[1];
	script->pbStack[++script->sp] = conv.c[2];
	script->pbStack[++script->sp] = conv.c[3];
}
float TaghaScript_pop_float32(Script_t *script)
{
	if( !script )
		return 0;
	if( script->bSafeMode and (script->sp-4) >= script->uiStksize ) {
		printf("TaghaScript_pop_float32 reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp-4);
		exit(1);
	}
	union conv_union conv;
	conv.c[3] = script->pbStack[script->sp--];
	conv.c[2] = script->pbStack[script->sp--];
	conv.c[1] = script->pbStack[script->sp--];
	conv.c[0] = script->pbStack[script->sp--];
	return conv.f;
}

void TaghaScript_push_short(Script_t *restrict script, const ushort val)
{
	if( !script )
		return;
	if( script->bSafeMode and (script->sp+2) >= script->uiStksize ) {
		printf("TaghaScript_push_short reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+2);
		exit(1);
	}
	union conv_union conv;
	conv.us = val;
	script->pbStack[++script->sp] = conv.c[0];
	script->pbStack[++script->sp] = conv.c[1];
}
ushort TaghaScript_pop_short(Script_t *script)
{
	if( !script )
		return 0;
	if( script->bSafeMode and (script->sp-2) >= script->uiStksize ) {
		printf("TaghaScript_pop_short reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp-2);
		exit(1);
	}
	union conv_union conv;
	conv.c[1] = script->pbStack[script->sp--];
	conv.c[0] = script->pbStack[script->sp--];
	return conv.us;
}

void TaghaScript_push_byte(Script_t *restrict script, const uchar val)
{
	if( !script )
		return;
	if( script->bSafeMode and (script->sp+1) >= script->uiStksize ) {
		printf("TaghaScript_push_byte reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+1);
		exit(1);
	}
	script->pbStack[++script->sp] = val;
}
uchar TaghaScript_pop_byte(Script_t *script)
{
	if( !script )
		return 0;
	if( script->bSafeMode and (script->sp-1) >= script->uiStksize ) {
		printf("TaghaScript_pop_byte reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp-1);
		exit(1);
	}
	
	return script->pbStack[script->sp--];
}

void TaghaScript_push_nbytes(Script_t *restrict script, void *restrict pItem, const Word_t bytesize)
{
	if( !script )
		return;
	if( script->bSafeMode and (script->sp+bytesize) >= script->uiStksize ) {
		printf("TaghaScript_push_nbytes reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+1);
		exit(1);
	}
	Word_t i=0;
	for( i=0 ; i<bytesize ; i++ )
		script->pbStack[++script->sp] = ((uchar *)pItem)[i];
}
void TaghaScript_pop_nbytes(Script_t *restrict script, void *restrict pBuffer, const Word_t bytesize)
{
	if( !script )
		return;
	if( script->bSafeMode and (script->sp-bytesize) >= script->uiStksize ) {
		printf("TaghaScript_pop_nbytes reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, script->sp+1);
		exit(1);
	}
	Word_t i=0;
	// should stop when the integer underflows
	for( i=bytesize-1 ; i<bytesize ; i-- )
		((uchar *)pBuffer)[i] = script->pbStack[script->sp--];
}


long double TaghaScript_read_longfloat(Script_t *restrict script, const Word_t address)
{
	if( !script )
		return 0;
	
	uint ldsize = sizeof(long double);
	if( script->bSafeMode and address > script->uiMemsize-ldsize ) {
		printf("TaghaScript_read_longfloat reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", script->ip, script->sp, address);
		exit(1);
	}
	return( *(long double *)(script->pbMemory + address) );
}
void TaghaScript_write_longfloat(Script_t *restrict script, const long double val, const Word_t address)
{
	if( !script )
		return;
		
	uint ldsize = sizeof(long double);
	if( script->bSafeMode and address > script->uiMemsize-ldsize ) {
		printf("TaghaScript_write_longfloat reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", script->ip, script->sp, address);
		exit(1);
	}
	*(long double *)(script->pbMemory + address) = val;
}

u64 TaghaScript_read_int64(Script_t *restrict script, const Word_t address)
{
	if( !script )
		return 0;
	if( script->bSafeMode and address > script->uiMemsize-8 ) {
		printf("TaghaScript_read_int64 reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", script->ip, script->sp, address);
		exit(1);
	}
	return( *(u64 *)(script->pbMemory + address) );
}
void TaghaScript_write_int64(Script_t *restrict script, const u64 val, const Word_t address)
{
	if( !script )
		return;
	if( script->bSafeMode and address > script->uiMemsize-8 ) {
		printf("TaghaScript_write_int64 reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", script->ip, script->sp, address);
		exit(1);
	}
	*(u64 *)(script->pbMemory + address) = val;
}

double TaghaScript_read_float64(Script_t *restrict script, const Word_t address)
{
	if( !script )
		return 0;
	if( script->bSafeMode and address > script->uiMemsize-8 ) {
		printf("TaghaScript_read_float64 reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", script->ip, script->sp, address);
		exit(1);
	}
	return( *(double *)(script->pbMemory + address) );
}
void TaghaScript_write_float64(Script_t *restrict script, const double val, const Word_t address)
{
	if( !script )
		return;
	if( script->bSafeMode and address > script->uiMemsize-8 ) {
		printf("TaghaScript_write_float64 reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", script->ip, script->sp, address);
		exit(1);
	}
	*(double *)(script->pbMemory + address) = val;
}

uint TaghaScript_read_int32(Script_t *restrict script, const Word_t address)
{
	if( !script )
		return 0;
	if( script->bSafeMode and address > script->uiMemsize-4 ) {
		printf("TaghaScript_read_int32 reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", script->ip, script->sp, address);
		exit(1);
	}
	return( *(uint *)(script->pbMemory + address) );
	/*
	union conv_union conv;
	conv.c[0] = script->pbMemory[address];
	conv.c[1] = script->pbMemory[address+1];
	conv.c[2] = script->pbMemory[address+2];
	conv.c[3] = script->pbMemory[address+3];
	return conv.ui;
	*/
}
void TaghaScript_write_int32(Script_t *restrict script, const uint val, const Word_t address)
{
	if( !script )
		return;
	if( script->bSafeMode and address > script->uiMemsize-4 ) {
		printf("TaghaScript_write_int32 reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", script->ip, script->sp, address);
		exit(1);
	}
	*(uint *)(script->pbMemory + address) = val;
	/*
	union conv_union conv;
	conv.ui = val;
	script->pbMemory[address] = conv.c[0];
	script->pbMemory[address+1] = conv.c[1];
	script->pbMemory[address+2] = conv.c[2];
	script->pbMemory[address+3] = conv.c[3];
	*/
	//printf("wrote %" PRIu32 " to address: %" PRIu32 "\n" );
}

float TaghaScript_read_float32(Script_t *restrict script, const Word_t address)
{
	if( !script )
		return 0;
	if( script->bSafeMode and address > script->uiMemsize-4 ) {
		printf("TaghaScript_read_float32 reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", script->ip, script->sp, address);
		exit(1);
	}
	return( *(float *)(script->pbMemory + address) );
	/*
	union conv_union conv;
	conv.c[0] = script->pbMemory[address];
	conv.c[1] = script->pbMemory[address+1];
	conv.c[2] = script->pbMemory[address+2];
	conv.c[3] = script->pbMemory[address+3];
	return conv.f;
	*/
}
void TaghaScript_write_float32(Script_t *restrict script, const float val, const Word_t address)
{
	if( !script )
		return;
	if( script->bSafeMode and address > script->uiMemsize-4 ) {
		printf("TaghaScript_write_float32 reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", script->ip, script->sp, address);
		exit(1);
	}
	/*
	union conv_union conv;
	conv.f = val;
	script->pbMemory[address] = conv.c[0];
	script->pbMemory[address+1] = conv.c[1];
	script->pbMemory[address+2] = conv.c[2];
	script->pbMemory[address+3] = conv.c[3];
	*/
	*(float *)(script->pbMemory + address) = val;
}

void TaghaScript_write_short(Script_t *restrict script, const ushort val, const Word_t address)
{
	if( !script )
		return;
	if( script->bSafeMode and address > script->uiMemsize-2 ) {
		printf("TaghaScript_write_short reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", script->ip, script->sp, address);
		exit(1);
	}
	/*
	union conv_union conv;
	conv.us = val;
	script->pbMemory[address] = conv.c[0];
	script->pbMemory[address+1] = conv.c[1];
	*/
	*(ushort *)(script->pbMemory + address) = val;
}

ushort TaghaScript_read_short(Script_t *restrict script, const Word_t address)
{
	if( !script )
		return 0;
	if( script->bSafeMode and address > script->uiMemsize-2 ) {
		printf("TaghaScript_read_short reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", script->ip, script->sp, address);
		exit(1);
	}
	return( *(ushort *)(script->pbMemory + address) );
	/*
	union conv_union conv;
	conv.c[0] = script->pbMemory[address];
	conv.c[1] = script->pbMemory[address+1];
	return conv.us;
	*/
}

uchar TaghaScript_read_byte(Script_t *restrict script, const Word_t address)
{
	if( !script )
		return 0;
	if( script->bSafeMode and address > script->uiMemsize-1 ) {
		printf("TaghaScript_read_byte reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", script->ip, script->sp, address);
		exit(1);
	}
	return script->pbMemory[address];
}
void TaghaScript_write_byte(Script_t *restrict script, const uchar val, const Word_t address)
{
	if( !script )
		return;
	if( script->bSafeMode and address > script->uiMemsize-1 ) {
		printf("TaghaScript_write_byte reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", script->ip, script->sp, address);
		exit(1);
	}
	script->pbMemory[address] = val;
}

void TaghaScript_read_nbytes(Script_t *restrict script, void *restrict pBuffer, const Word_t bytesize, const Word_t address)
{
	if( !script )
		return;
	
	Word_t	addr = address;
	Word_t	i=0;
	while( i<bytesize ) {
		if( script->bSafeMode and addr > script->uiMemsize-i ) {
			printf("TaghaScript_read_array reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", script->ip, script->sp, addr);
			exit(1);
		}
		((uchar *)pBuffer)[i++] = script->pbMemory[addr++];
	}
}
void TaghaScript_write_nbytes(Script_t *restrict script, void *restrict pItem, const Word_t bytesize, const Word_t address)
{
	if( !script )
		return;
	
	Word_t	addr = address;
	Word_t	i=0;
	while( i<bytesize ) {
		if( script->bSafeMode and addr > script->uiMemsize+i ) {
			printf("TaghaScript_write_array reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", script->ip, script->sp, addr);
			exit(1);
		}
		//script->pbMemory[addr++] = val[i++];
		script->pbMemory[addr++] = ((uchar *)pItem)[i++];
	}
}


long double *TaghaScript_addr2ptr_longfloat(Script_t *restrict script, const Word_t address)
{
	if( !script )
		return NULL;
	else if( !script->pbMemory )
		return NULL;
	
	uint ldsize = sizeof(long double);
	if( script->bSafeMode and address > script->uiMemsize-ldsize ) {
		printf("TaghaScript_addr2ptr_longfloat reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", script->ip, script->sp, address);
		exit(1);
	}
	return (long double *)(script->pbMemory + address);
}

u64 *TaghaScript_addr2ptr_int64(Script_t *restrict script, const Word_t address)
{
	if( !script )
		return NULL;
	else if( !script->pbMemory )
		return NULL;
	
	if( script->bSafeMode and address > script->uiMemsize-8 ) {
		printf("TaghaScript_addr2ptr_int64 reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", script->ip, script->sp, address);
		exit(1);
	}
	return (u64 *)(script->pbMemory + address);
}

double *TaghaScript_addr2ptr_float64(Script_t *restrict script, const Word_t address)
{
	if( !script )
		return NULL;
	else if( !script->pbMemory )
		return NULL;
	
	if( script->bSafeMode and address > script->uiMemsize-8 ) {
		printf("TaghaScript_addr2ptr_float64 reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", script->ip, script->sp, address);
		exit(1);
	}
	return (double *)(script->pbMemory + address);
}

uint *TaghaScript_addr2ptr_int32(Script_t *restrict script, const Word_t address)
{
	if( !script )
		return NULL;
	else if( !script->pbMemory )
		return NULL;
	
	if( script->bSafeMode and address > script->uiMemsize-4 ) {
		printf("TaghaScript_addr2ptr_int32 reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", script->ip, script->sp, address);
		exit(1);
	}
	return (uint *)(script->pbMemory + address);
}

float *TaghaScript_addr2ptr_float32(Script_t *restrict script, const Word_t address)
{
	if( !script )
		return NULL;
	else if( !script->pbMemory )
		return NULL;
	
	if( script->bSafeMode and address > script->uiMemsize-4 ) {
		printf("TaghaScript_addr2ptr_float32 reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", script->ip, script->sp, address);
		exit(1);
	}
	return (float *)(script->pbMemory + address);
}

ushort *TaghaScript_addr2ptr_short(Script_t *restrict script, const Word_t address)
{
	if( !script )
		return NULL;
	else if( !script->pbMemory )
		return NULL;
	
	if( script->bSafeMode and address > script->uiMemsize-2 ) {
		printf("TaghaScript_addr2ptr_short reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", script->ip, script->sp, address);
		exit(1);
	}
	return (ushort *)(script->pbMemory + address);
}
uchar *TaghaScript_addr2ptr_byte(Script_t *restrict script, const Word_t address)
{
	if( !script )
		return NULL;
	else if( !script->pbMemory )
		return NULL;
	
	if( script->bSafeMode and address > script->uiMemsize-1 ) {
		printf("TaghaScript_addr2ptr_byte reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", script->ip, script->sp, address);
		exit(1);
	}
	return( script->pbMemory+address );
}


long double *TaghaScript_stkaddr2ptr_longfloat(Script_t *restrict script, const Word_t address)
{
	if( !script or address==0 )
		return NULL;
	else if( !script->pbStack )
		return NULL;
	uint ldsize = sizeof(long double);
	if( script->bSafeMode and address+(ldsize-1) > script->sp ) {
		printf("TaghaScript_stkaddr2ptr_longfloat reported: invalid stack address! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, address+(ldsize-1));
		exit(1);
	}
	return (long double *)(script->pbStack + address);
}

u64 *TaghaScript_stkaddr2ptr_int64(Script_t *restrict script, const Word_t address)
{
	if( !script or address==0 )
		return NULL;
	else if( !script->pbStack )
		return NULL;
	if( script->bSafeMode and address+7 > script->sp ) {
		printf("TaghaScript_stkaddr2ptr_int64 reported: invalid stack address! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, address+7);
		exit(1);
	}
	return (u64 *)(script->pbStack + address);
}

double *TaghaScript_stkaddr2ptr_float64(Script_t *restrict script, const Word_t address)
{
	if( !script or address==0 )
		return NULL;
	else if( !script->pbStack )
		return NULL;
	if( script->bSafeMode and address+7 > script->sp ) {
		printf("TaghaScript_stkaddr2ptr_float64 reported: invalid stack address! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, address+7);
		exit(1);
	}
	return (double *)(script->pbStack + address);
}

uint *TaghaScript_stkaddr2ptr_int32(Script_t *restrict script, const Word_t address)
{
	if( !script or address==0 )
		return NULL;
	else if( !script->pbStack )
		return NULL;
	if( script->bSafeMode and address+3 > script->sp ) {
		printf("TaghaScript_stkaddr2ptr_int32 reported: invalid stack address! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, address+3);
		exit(1);
	}
	return (uint *)(script->pbStack + address);
}

float *TaghaScript_stkaddr2ptr_float32(Script_t *restrict script, const Word_t address)
{
	if( !script or address==0 )
		return NULL;
	else if( !script->pbStack )
		return NULL;
	//if( script->bSafeMode and address-3 > script->sp ) {
	if( script->bSafeMode and address+3 > script->sp ) {
		printf("TaghaScript_stkaddr2ptr_float32 reported: invalid stack address! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, address-3);
		exit(1);
	}
	//return (float *)(script->pbStack + (address-3));
	return (float *)(script->pbStack + address);
}

ushort *TaghaScript_stkaddr2ptr_short(Script_t *restrict script, const Word_t address)
{
	if( !script or address==0 )
		return NULL;
	else if( !script->pbStack )
		return NULL;
	if( script->bSafeMode and address+1 > script->sp ) {
		printf("TaghaScript_stkaddr2ptr_short reported: invalid stack address! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, address+1);
		exit(1);
	}
	return (ushort *)(script->pbStack + address);
}
uchar *TaghaScript_stkaddr2ptr_byte(Script_t *restrict script, const Word_t address)
{
	if( !script or address==0 )
		return NULL;
	else if( !script->pbStack )
		return NULL;
	if( script->bSafeMode and address > script->sp ) {
		printf("TaghaScript_stkaddr2ptr_byte reported: invalid stack address! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", script->ip, address);
		exit(1);
	}
	return( script->pbStack + address );
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
		if( script->pbMemory[i] )
			printf("Memory[0x%x] == %" PRIu32 "\n", i, script->pbMemory[i]);
	printf("\n");
}
void TaghaScript_debug_print_stack(const Script_t *script)
{
	if( !script )
		return;
	else if( !script->pbStack )
		return;
	
	printf("DEBUG ...---===---... Printing Stack...\n");
	uint i;
	const uint stkPtr = script->sp;
	for( i=1 ; i<script->uiStksize and i<=stkPtr ; i++ ) {
		if( stkPtr == i )
			printf("TOS Stack[0x%x] == %" PRIu32 "\n", i, script->pbStack[i]);
		else printf("Stack[0x%x] == %" PRIu32 "\n", i, script->pbStack[i]);
	}
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


