
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

void tagha_init(TaghaVM_t *restrict vm)
{
	if( !vm )
		return;
	
	vm->pbMemory = NULL;
	vm->pbStack = NULL;
	vm->fnpNative = NULL;
	vm->bSafeMode = true;
	
	vm->ip = vm->sp = vm->bp = 0;
	vm->pInstrStream = NULL;
	vm->uiMaxInstrs = 200;	// helps to stop infinite/runaway loops
}

void tagha_load_code(TaghaVM_t *restrict vm, char *restrict filename)
{
	if( !vm )
		return;
	
	FILE *pFile = fopen(filename, "rb");
	if( !pFile ) {
		 printf("[Tagha Error]: File not found: \'%s\'\n", filename);
		return;
	}
	
	u64 size = get_file_size(pFile);
	vm->pInstrStream = calloc(size, sizeof(uchar));
	assert( vm->pInstrStream );
	fread(vm->pInstrStream, sizeof(uchar), size, pFile);
	fclose(pFile); pFile=NULL;
	
	uchar *verify = vm->pInstrStream;
	// verify that this is executable code.
	if( *(ushort *)verify == 0xC0DE ) {
		printf("tagha_load_code :: verified code!\n");
		verify += 2;
		vm->ip = *(uint *)verify;
		printf("tagha_load_code :: ip starts at %u\n", *(uint *)verify);
		
		vm->pbStack = calloc(STK_SIZE, sizeof(uchar));	//&(uchar[STK_SIZE]){0};
		assert( vm->pbStack );
		vm->pbMemory = calloc(MEM_SIZE, sizeof(uchar));		//&(uchar[MEM_SIZE]){0};
		assert( vm->pbMemory );
	}
	else {
		printf("tagha_load_code :: unknown file memory format\n");
		free(vm->pInstrStream); vm->pInstrStream=NULL;
	}
}

void tagha_free(TaghaVM_t *vm)
{
	if( !vm )
		return;
	
	if( vm->pbStack )
		free(vm->pbStack);
	vm->pbStack = NULL;
	
	if( vm->pbMemory )
		free(vm->pbMemory);
	vm->pbMemory = NULL;
	
	if( vm->pInstrStream )
		free(vm->pInstrStream);
	vm->pInstrStream = NULL;
	tagha_init(vm);
}


void tagha_reset(TaghaVM_t *vm)
{
	if( !vm )
		return;
	
	uint i;
	for( i=0 ; i<MEM_SIZE ; i++ )
		vm->pbMemory[i] = 0;
	for( i=0 ; i<STK_SIZE ; i++ )
		vm->pbStack[i] = 0;
	
	vm->sp = vm->bp = 0;
}

/*
int tagha_register_funcs(TaghaVM_t *restrict vm, NativeInfo_t **Natives)
{
	if( !vm or !Natives )
		return 0;
	
	
	return 1;
}
*/
int tagha_register_func(TaghaVM_t *restrict vm, fnNative pNative)
{
	if( !vm or !pNative )
		return 0;
	
	vm->fnpNative = pNative;
	return 1;
}



void tagha_push_longfloat(TaghaVM_t *restrict vm, const long double val)
{
	if( !vm )
		return;
	
	// long doubles are usually 12 or 16 bytes, adjust for both.
	uint ldsize = sizeof(long double);
	if( vm->bSafeMode and (vm->sp+ldsize) >= STK_SIZE ) {
		printf("tagha_push_longfloat reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp+ldsize);
		exit(1);
	}
	long double longdbl = val;
	uint i = 0;
	while( i<ldsize )
		vm->pbStack[++vm->sp] = ((uchar *)&longdbl)[i++];
}

long double tagha_pop_longfloat(TaghaVM_t *vm)
{
	if( !vm )
		return 0;
	uint ldsize = sizeof(long double);
	if( vm->bSafeMode and (vm->sp+ldsize) >= STK_SIZE ) {
		printf("tagha_pop_longfloat reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp+ldsize);
		exit(1);
	}
	long double longdbl;
	uint i = ldsize-1;
	while( i<ldsize )
		((uchar *)&longdbl)[i--] = vm->pbStack[vm->sp--];
	return longdbl;
}

void tagha_push_int64(TaghaVM_t *restrict vm, const u64 val)
{
	if( !vm )
		return;
	if( vm->bSafeMode and (vm->sp+8) >= STK_SIZE ) {
		printf("tagha_push_int64 reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp+8);
		exit(1);
	}
	union conv_union conv;
	conv.ull = val;
	vm->pbStack[++vm->sp] = conv.c[0];
	vm->pbStack[++vm->sp] = conv.c[1];
	vm->pbStack[++vm->sp] = conv.c[2];
	vm->pbStack[++vm->sp] = conv.c[3];
	vm->pbStack[++vm->sp] = conv.c[4];
	vm->pbStack[++vm->sp] = conv.c[5];
	vm->pbStack[++vm->sp] = conv.c[6];
	vm->pbStack[++vm->sp] = conv.c[7];
}
u64 tagha_pop_int64(TaghaVM_t *vm)
{
	if( !vm )
		return 0L;
	if( vm->bSafeMode and (vm->sp-8) >= STK_SIZE ) {
		printf("tagha_pop_int64 reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-8);
		exit(1);
	}
	union conv_union conv;
	conv.c[7] = vm->pbStack[vm->sp--];
	conv.c[6] = vm->pbStack[vm->sp--];
	conv.c[5] = vm->pbStack[vm->sp--];
	conv.c[4] = vm->pbStack[vm->sp--];
	conv.c[3] = vm->pbStack[vm->sp--];
	conv.c[2] = vm->pbStack[vm->sp--];
	conv.c[1] = vm->pbStack[vm->sp--];
	conv.c[0] = vm->pbStack[vm->sp--];
	return conv.ull;
}

void tagha_push_float64(TaghaVM_t *restrict vm, const double val)
{
	if( !vm )
		return;
	if( vm->bSafeMode and (vm->sp+8) >= STK_SIZE ) {
		printf("tagha_push_float64 reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp+8);
		exit(1);
	}
	union conv_union conv;
	conv.dbl = val;
	vm->pbStack[++vm->sp] = conv.c[0];
	vm->pbStack[++vm->sp] = conv.c[1];
	vm->pbStack[++vm->sp] = conv.c[2];
	vm->pbStack[++vm->sp] = conv.c[3];
	vm->pbStack[++vm->sp] = conv.c[4];
	vm->pbStack[++vm->sp] = conv.c[5];
	vm->pbStack[++vm->sp] = conv.c[6];
	vm->pbStack[++vm->sp] = conv.c[7];
}
double tagha_pop_float64(TaghaVM_t *vm)
{
	if( !vm )
		return 0;
	if( vm->bSafeMode and (vm->sp-8) >= STK_SIZE ) {
		printf("tagha_pop_float64 reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-8);
		exit(1);
	}
	union conv_union conv;
	conv.c[7] = vm->pbStack[vm->sp--];
	conv.c[6] = vm->pbStack[vm->sp--];
	conv.c[5] = vm->pbStack[vm->sp--];
	conv.c[4] = vm->pbStack[vm->sp--];
	conv.c[3] = vm->pbStack[vm->sp--];
	conv.c[2] = vm->pbStack[vm->sp--];
	conv.c[1] = vm->pbStack[vm->sp--];
	conv.c[0] = vm->pbStack[vm->sp--];
	return conv.dbl;
}

void tagha_push_int32(TaghaVM_t *restrict vm, const uint val)
{
	if( !vm )
		return;
	if( vm->bSafeMode and (vm->sp+4) >= STK_SIZE ) {
		printf("tagha_push_int32 reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp+4);
		exit(1);
	}
	union conv_union conv;
	conv.ui = val;
	vm->pbStack[++vm->sp] = conv.c[0];
	vm->pbStack[++vm->sp] = conv.c[1];
	vm->pbStack[++vm->sp] = conv.c[2];
	vm->pbStack[++vm->sp] = conv.c[3];
}
uint tagha_pop_int32(TaghaVM_t *vm)
{
	if( !vm )
		return 0;
	if( vm->bSafeMode and (vm->sp-4) >= STK_SIZE ) {	// we're subtracting, did we integer underflow?
		printf("tagha_pop_int32 reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-4);
		exit(1);
	}
	union conv_union conv;
	conv.c[3] = vm->pbStack[vm->sp--];
	conv.c[2] = vm->pbStack[vm->sp--];
	conv.c[1] = vm->pbStack[vm->sp--];
	conv.c[0] = vm->pbStack[vm->sp--];
	return conv.ui;
}

void tagha_push_float32(TaghaVM_t *restrict vm, const float val)
{
	if( !vm )
		return;
	if( vm->bSafeMode and (vm->sp+4) >= STK_SIZE ) {
		printf("tagha_push_float32 reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp+4);
		exit(1);
	}
	union conv_union conv;
	conv.f = val;
	vm->pbStack[++vm->sp] = conv.c[0];
	vm->pbStack[++vm->sp] = conv.c[1];
	vm->pbStack[++vm->sp] = conv.c[2];
	vm->pbStack[++vm->sp] = conv.c[3];
}
float tagha_pop_float32(TaghaVM_t *vm)
{
	if( !vm )
		return 0;
	if( vm->bSafeMode and (vm->sp-4) >= STK_SIZE ) {
		printf("tagha_pop_float32 reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-4);
		exit(1);
	}
	union conv_union conv;
	conv.c[3] = vm->pbStack[vm->sp--];
	conv.c[2] = vm->pbStack[vm->sp--];
	conv.c[1] = vm->pbStack[vm->sp--];
	conv.c[0] = vm->pbStack[vm->sp--];
	return conv.f;
}

void tagha_push_short(TaghaVM_t *restrict vm, const ushort val)
{
	if( !vm )
		return;
	if( vm->bSafeMode and (vm->sp+2) >= STK_SIZE ) {
		printf("tagha_push_short reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp+2);
		exit(1);
	}
	union conv_union conv;
	conv.us = val;
	vm->pbStack[++vm->sp] = conv.c[0];
	vm->pbStack[++vm->sp] = conv.c[1];
}
ushort tagha_pop_short(TaghaVM_t *vm)
{
	if( !vm )
		return 0;
	if( vm->bSafeMode and (vm->sp-2) >= STK_SIZE ) {
		printf("tagha_pop_short reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-2);
		exit(1);
	}
	union conv_union conv;
	conv.c[1] = vm->pbStack[vm->sp--];
	conv.c[0] = vm->pbStack[vm->sp--];
	return conv.us;
}

void tagha_push_byte(TaghaVM_t *restrict vm, const uchar val)
{
	if( !vm )
		return;
	if( vm->bSafeMode and (vm->sp+1) >= STK_SIZE ) {
		printf("tagha_push_byte reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp+1);
		exit(1);
	}
	vm->pbStack[++vm->sp] = val;
}
uchar tagha_pop_byte(TaghaVM_t *vm)
{
	if( !vm )
		return 0;
	if( vm->bSafeMode and (vm->sp-1) >= STK_SIZE ) {
		printf("tagha_pop_byte reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-1);
		exit(1);
	}
	
	return vm->pbStack[vm->sp--];
}

void tagha_push_nbytes(TaghaVM_t *restrict vm, void *restrict pItem, const Word_t bytesize)
{
	if( !vm )
		return;
	if( vm->bSafeMode and (vm->sp+bytesize) >= STK_SIZE ) {
		printf("tagha_push_nbytes reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp+1);
		exit(1);
	}
	Word_t i=0;
	for( i=0 ; i<bytesize ; i++ )
		vm->pbStack[++vm->sp] = ((uchar *)pItem)[i];
}
void tagha_pop_nbytes(TaghaVM_t *restrict vm, void *restrict pBuffer, const Word_t bytesize)
{
	if( !vm )
		return;
	if( vm->bSafeMode and (vm->sp-bytesize) >= STK_SIZE ) {
		printf("tagha_pop_nbytes reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp+1);
		exit(1);
	}
	Word_t i=0;
	// should stop when the integer underflows
	for( i=bytesize-1 ; i<bytesize ; i-- )
		((uchar *)pBuffer)[i] = vm->pbStack[vm->sp--];
}


long double tagha_read_longfloat(TaghaVM_t *restrict vm, const Word_t address)
{
	if( !vm )
		return 0;
	
	uint ldsize = sizeof(long double);
	if( vm->bSafeMode and address > MEM_SIZE-ldsize ) {
		printf("tagha_read_longfloat reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, address);
		exit(1);
	}
	return( *(long double *)(vm->pbMemory + address) );
}
void tagha_write_longfloat(TaghaVM_t *restrict vm, const long double val, const Word_t address)
{
	if( !vm )
		return;
		
	uint ldsize = sizeof(long double);
	if( vm->bSafeMode and address > MEM_SIZE-ldsize ) {
		printf("tagha_write_longfloat reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, address);
		exit(1);
	}
	*(long double *)(vm->pbMemory + address) = val;
}

u64 tagha_read_int64(TaghaVM_t *restrict vm, const Word_t address)
{
	if( !vm )
		return 0;
	if( vm->bSafeMode and address > MEM_SIZE-8 ) {
		printf("tagha_read_int64 reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, address);
		exit(1);
	}
	return( *(u64 *)(vm->pbMemory + address) );
}
void tagha_write_int64(TaghaVM_t *restrict vm, const u64 val, const Word_t address)
{
	if( !vm )
		return;
	if( vm->bSafeMode and address > MEM_SIZE-8 ) {
		printf("tagha_write_int64 reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, address);
		exit(1);
	}
	*(u64 *)(vm->pbMemory + address) = val;
}

double tagha_read_float64(TaghaVM_t *restrict vm, const Word_t address)
{
	if( !vm )
		return 0;
	if( vm->bSafeMode and address > MEM_SIZE-8 ) {
		printf("tagha_read_float64 reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, address);
		exit(1);
	}
	return( *(double *)(vm->pbMemory + address) );
}
void tagha_write_float64(TaghaVM_t *restrict vm, const double val, const Word_t address)
{
	if( !vm )
		return;
	if( vm->bSafeMode and address > MEM_SIZE-8 ) {
		printf("tagha_write_float64 reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, address);
		exit(1);
	}
	*(double *)(vm->pbMemory + address) = val;
}

uint tagha_read_int32(TaghaVM_t *restrict vm, const Word_t address)
{
	if( !vm )
		return 0;
	if( vm->bSafeMode and address > MEM_SIZE-4 ) {
		printf("tagha_read_int32 reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, address);
		exit(1);
	}
	return( *(uint *)(vm->pbMemory + address) );
	/*
	union conv_union conv;
	conv.c[0] = vm->pbMemory[address];
	conv.c[1] = vm->pbMemory[address+1];
	conv.c[2] = vm->pbMemory[address+2];
	conv.c[3] = vm->pbMemory[address+3];
	return conv.ui;
	*/
}
void tagha_write_int32(TaghaVM_t *restrict vm, const uint val, const Word_t address)
{
	if( !vm )
		return;
	if( vm->bSafeMode and address > MEM_SIZE-4 ) {
		printf("tagha_write_int32 reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, address);
		exit(1);
	}
	*(uint *)(vm->pbMemory + address) = val;
	/*
	union conv_union conv;
	conv.ui = val;
	vm->pbMemory[address] = conv.c[0];
	vm->pbMemory[address+1] = conv.c[1];
	vm->pbMemory[address+2] = conv.c[2];
	vm->pbMemory[address+3] = conv.c[3];
	*/
	//printf("wrote %" PRIu32 " to address: %" PRIu32 "\n" );
}

float tagha_read_float32(TaghaVM_t *restrict vm, const Word_t address)
{
	if( !vm )
		return 0;
	if( vm->bSafeMode and address > MEM_SIZE-4 ) {
		printf("tagha_read_float32 reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, address);
		exit(1);
	}
	return( *(float *)(vm->pbMemory + address) );
	/*
	union conv_union conv;
	conv.c[0] = vm->pbMemory[address];
	conv.c[1] = vm->pbMemory[address+1];
	conv.c[2] = vm->pbMemory[address+2];
	conv.c[3] = vm->pbMemory[address+3];
	return conv.f;
	*/
}
void tagha_write_float32(TaghaVM_t *restrict vm, const float val, const Word_t address)
{
	if( !vm )
		return;
	if( vm->bSafeMode and address > MEM_SIZE-4 ) {
		printf("tagha_write_float32 reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, address);
		exit(1);
	}
	/*
	union conv_union conv;
	conv.f = val;
	vm->pbMemory[address] = conv.c[0];
	vm->pbMemory[address+1] = conv.c[1];
	vm->pbMemory[address+2] = conv.c[2];
	vm->pbMemory[address+3] = conv.c[3];
	*/
	*(float *)(vm->pbMemory + address) = val;
}

void tagha_write_short(TaghaVM_t *restrict vm, const ushort val, const Word_t address)
{
	if( !vm )
		return;
	if( vm->bSafeMode and address > MEM_SIZE-2 ) {
		printf("tagha_write_short reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, address);
		exit(1);
	}
	/*
	union conv_union conv;
	conv.us = val;
	vm->pbMemory[address] = conv.c[0];
	vm->pbMemory[address+1] = conv.c[1];
	*/
	*(ushort *)(vm->pbMemory + address) = val;
}

ushort tagha_read_short(TaghaVM_t *restrict vm, const Word_t address)
{
	if( !vm )
		return 0;
	if( vm->bSafeMode and address > MEM_SIZE-2 ) {
		printf("tagha_read_short reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, address);
		exit(1);
	}
	return( *(ushort *)(vm->pbMemory + address) );
	/*
	union conv_union conv;
	conv.c[0] = vm->pbMemory[address];
	conv.c[1] = vm->pbMemory[address+1];
	return conv.us;
	*/
}

uchar tagha_read_byte(TaghaVM_t *restrict vm, const Word_t address)
{
	if( !vm )
		return 0;
	if( vm->bSafeMode and address >= MEM_SIZE ) {
		printf("tagha_read_byte reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, address);
		exit(1);
	}
	return vm->pbMemory[address];
}
void tagha_write_byte(TaghaVM_t *restrict vm, const uchar val, const Word_t address)
{
	if( !vm )
		return;
	if( vm->bSafeMode and address >= MEM_SIZE ) {
		printf("tagha_write_byte reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, address);
		exit(1);
	}
	vm->pbMemory[address] = val;
}

void tagha_read_nbytes(TaghaVM_t *restrict vm, void *restrict pBuffer, const Word_t bytesize, const Word_t address)
{
	if( !vm )
		return;
	
	Word_t	addr = address;
	Word_t	i=0;
	while( i<bytesize ) {
		if( vm->bSafeMode and addr >= MEM_SIZE-i ) {
			printf("tagha_read_array reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, addr);
			exit(1);
		}
		((uchar *)pBuffer)[i++] = vm->pbMemory[addr++];
		//buffer[i++] = vm->pbMemory[addr++];
	}
}
void tagha_write_nbytes(TaghaVM_t *restrict vm, void *restrict pItem, const Word_t bytesize, const Word_t address)
{
	if( !vm )
		return;
	
	Word_t	addr = address;
	Word_t	i=0;
	while( i<bytesize ) {
		if( vm->bSafeMode and addr >= MEM_SIZE+i ) {
			printf("tagha_write_array reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, addr);
			exit(1);
		}
		//vm->pbMemory[addr++] = val[i++];
		vm->pbMemory[addr++] = ((uchar *)pItem)[i++];
	}
}


long double *tagha_addr2ptr_longdouble(TaghaVM_t *restrict vm, const Word_t address)
{
	if( !vm )
		return NULL;
	else if( !vm->pbMemory )
		return NULL;
	
	uint ldsize = sizeof(long double);
	if( vm->bSafeMode and address > MEM_SIZE-ldsize ) {
		printf("tagha_addr2ptr_longdouble reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, address);
		exit(1);
	}
	return (long double *)(vm->pbMemory + address);
}

u64 *tagha_addr2ptr_int64(TaghaVM_t *restrict vm, const Word_t address)
{
	if( !vm )
		return NULL;
	else if( !vm->pbMemory )
		return NULL;
	
	if( vm->bSafeMode and address > MEM_SIZE-8 ) {
		printf("tagha_addr2ptr_int64 reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, address);
		exit(1);
	}
	return (u64 *)(vm->pbMemory + address);
}

double *tagha_addr2ptr_float64(TaghaVM_t *restrict vm, const Word_t address)
{
	if( !vm )
		return NULL;
	else if( !vm->pbMemory )
		return NULL;
	
	if( vm->bSafeMode and address > MEM_SIZE-8 ) {
		printf("tagha_addr2ptr_float64 reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, address);
		exit(1);
	}
	return (double *)(vm->pbMemory + address);
}

uint *tagha_addr2ptr_int32(TaghaVM_t *restrict vm, const Word_t address)
{
	if( !vm )
		return NULL;
	else if( !vm->pbMemory )
		return NULL;
	
	if( vm->bSafeMode and address > MEM_SIZE-4 ) {
		printf("tagha_addr2ptr_int32 reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, address);
		exit(1);
	}
	return (uint *)(vm->pbMemory + address);
}

float *tagha_addr2ptr_float32(TaghaVM_t *restrict vm, const Word_t address)
{
	if( !vm )
		return NULL;
	else if( !vm->pbMemory )
		return NULL;
	
	if( vm->bSafeMode and address > MEM_SIZE-4 ) {
		printf("tagha_addr2ptr_float32 reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, address);
		exit(1);
	}
	return (float *)(vm->pbMemory + address);
}

ushort *tagha_addr2ptr_short(TaghaVM_t *restrict vm, const Word_t address)
{
	if( !vm )
		return NULL;
	else if( !vm->pbMemory )
		return NULL;
	
	if( vm->bSafeMode and address > MEM_SIZE-2 ) {
		printf("tagha_addr2ptr_short reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, address);
		exit(1);
	}
	return (ushort *)(vm->pbMemory + address);
}
uchar *tagha_addr2ptr_byte(TaghaVM_t *restrict vm, const Word_t address)
{
	if( !vm )
		return NULL;
	else if( !vm->pbMemory )
		return NULL;
	
	if( vm->bSafeMode and address > MEM_SIZE-1 ) {
		printf("tagha_addr2ptr_byte reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, address);
		exit(1);
	}
	return( vm->pbMemory+address );
}


long double *tagha_stkaddr2ptr_longdouble(TaghaVM_t *restrict vm, const Word_t address)
{
	if( !vm or address==0 )
		return NULL;
	else if( !vm->pbStack )
		return NULL;
	uint ldsize = sizeof(long double);
	if( vm->bSafeMode and address+(ldsize-1) > vm->sp ) {
		printf("tagha_stkaddr2ptr_longdouble reported: invalid stack address! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, address+(ldsize-1));
		exit(1);
	}
	return (long double *)(vm->pbStack + address);
}

u64 *tagha_stkaddr2ptr_int64(TaghaVM_t *restrict vm, const Word_t address)
{
	if( !vm or address==0 )
		return NULL;
	else if( !vm->pbStack )
		return NULL;
	if( vm->bSafeMode and address+7 > vm->sp ) {
		printf("tagha_stkaddr2ptr_int64 reported: invalid stack address! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, address+7);
		exit(1);
	}
	return (u64 *)(vm->pbStack + address);
}

double *tagha_stkaddr2ptr_float64(TaghaVM_t *restrict vm, const Word_t address)
{
	if( !vm or address==0 )
		return NULL;
	else if( !vm->pbStack )
		return NULL;
	if( vm->bSafeMode and address+7 > vm->sp ) {
		printf("tagha_stkaddr2ptr_float64 reported: invalid stack address! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, address+7);
		exit(1);
	}
	return (double *)(vm->pbStack + address);
}

uint *tagha_stkaddr2ptr_int32(TaghaVM_t *restrict vm, const Word_t address)
{
	if( !vm or address==0 )
		return NULL;
	else if( !vm->pbStack )
		return NULL;
	if( vm->bSafeMode and address+3 > vm->sp ) {
		printf("tagha_stkaddr2ptr_int32 reported: invalid stack address! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, address+3);
		exit(1);
	}
	return (uint *)(vm->pbStack + address);
}

float *tagha_stkaddr2ptr_float32(TaghaVM_t *restrict vm, const Word_t address)
{
	if( !vm or address==0 )
		return NULL;
	else if( !vm->pbStack )
		return NULL;
	//if( vm->bSafeMode and address-3 > vm->sp ) {
	if( vm->bSafeMode and address+3 > vm->sp ) {
		printf("tagha_stkaddr2ptr_float32 reported: invalid stack address! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, address-3);
		exit(1);
	}
	//return (float *)(vm->pbStack + (address-3));
	return (float *)(vm->pbStack + address);
}

ushort *tagha_stkaddr2ptr_short(TaghaVM_t *restrict vm, const Word_t address)
{
	if( !vm or address==0 )
		return NULL;
	else if( !vm->pbStack )
		return NULL;
	if( vm->bSafeMode and address+1 > vm->sp ) {
		printf("tagha_stkaddr2ptr_short reported: invalid stack address! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, address+1);
		exit(1);
	}
	return (ushort *)(vm->pbStack + address);
}
uchar *tagha_stkaddr2ptr_byte(TaghaVM_t *restrict vm, const Word_t address)
{
	if( !vm or address==0 )
		return NULL;
	else if( !vm->pbStack )
		return NULL;
	if( vm->bSafeMode and address > vm->sp ) {
		printf("tagha_stkaddr2ptr_byte reported: invalid stack address! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, address);
		exit(1);
	}
	return( vm->pbStack + address );
}




void tagha_debug_print_memory(const TaghaVM_t *vm)
{
	if( !vm )
		return;
	else if( !vm->pbMemory )
		return;
	
	printf("DEBUG ...---===---... Printing Memory...\n");
	uint i;
	for( i=0 ; i<MEM_SIZE ; i++ )
		if( vm->pbMemory[i] )
			printf("Memory[0x%x] == %" PRIu32 "\n", i, vm->pbMemory[i]);
	printf("\n");
}
void tagha_debug_print_stack(const TaghaVM_t *vm)
{
	if( !vm )
		return;
	else if( !vm->pbStack )
		return;
	
	printf("DEBUG ...---===---... Printing Stack...\n");
	uint i;
	for( i=1 ; i<STK_SIZE and i<=vm->sp ; i++ ) {
		if( vm->sp == i )
			printf("TOS Stack[0x%x] == %" PRIu32 "\n", i, vm->pbStack[i]);
		else printf("Stack[0x%x] == %" PRIu32 "\n", i, vm->pbStack[i]);
	}
	printf("\n");
}
void tagha_debug_print_ptrs(const TaghaVM_t *vm)
{
	if( !vm )
		return;
	
	printf("DEBUG ...---===---... Printing Pointers...\n");
	printf("Instruction Pointer: %" PRIu32 "\
			\nStack Pointer: %" PRIu32 "\
			\nStack Frame Pointer: %" PRIu32 "\n", vm->ip, vm->sp, vm->bp);
	printf("\n");
}
