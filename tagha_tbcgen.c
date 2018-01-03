#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include "tagha_tbcgen.h"


static struct ByteBuffer	*ByteBuffer_New(void);
static void		ByteBuffer_Init		(struct ByteBuffer *);
static size_t	ByteBuffer_Len		(const struct ByteBuffer *);
static void		ByteBuffer_Insert	(struct ByteBuffer *, uint8_t);
static void		ByteBuffer_InsertInt	(struct ByteBuffer *, uint64_t, size_t);
static void		ByteBuffer_InsertFloat	(struct ByteBuffer *, float);
static void		ByteBuffer_InsertDouble	(struct ByteBuffer *, double);
static void		ByteBuffer_InsertString	(struct ByteBuffer *, const char *, size_t);
static void		ByteBuffer_InsertObject	(struct ByteBuffer *, const void *, size_t);
static void		ByteBuffer_Delete	(struct ByteBuffer *, size_t);
static void		ByteBuffer_Free		(struct ByteBuffer *);
static void		ByteBuffer_Resize	(struct ByteBuffer *);
static void		ByteBuffer_DumpToFile(struct ByteBuffer *, FILE *);
static void		ByteBuffer_ReadFromFile(struct ByteBuffer *, FILE *);
static void		ByteBuffer_Append	(struct ByteBuffer *, struct ByteBuffer *);
static uint32_t	FloatToInt			(float);
static uint64_t	DoubleToLong		(double);

static const uint16_t TBCMagic = 0xC0DE;

static uint32_t FloatToInt(const float fval)
{
	union {
		float f;
		uint32_t i;
	} conv;
	conv.f = fval;
	return conv.i;
}

static uint64_t DoubleToLong(const double fval)
{
	union {
		double f;
		uint64_t i;
	} conv;
	conv.f = fval;
	return conv.i;
}


static struct ByteBuffer *ByteBuffer_New(void)
{
	return calloc(1, sizeof(struct ByteBuffer));
}

static void ByteBuffer_Init(struct ByteBuffer *restrict const p)
{
	if( !p )
		return;
	p->data = NULL;
	p->size = p->count = 0;
}

static size_t ByteBuffer_Len(const struct ByteBuffer *restrict const p)
{
	return p ? p->count : 0;
}

static void ByteBuffer_Insert(struct ByteBuffer *restrict const p, const uint8_t byte)
{
	if( !p )
		return;
	else if( p->count >= p->size )
		ByteBuffer_Resize(p);
	
	p->data[p->count++] = byte;
}

static void ByteBuffer_InsertInt(struct ByteBuffer *restrict const p, const uint64_t value, const size_t bytes)
{
	if( !p )
		return;
	else if( p->count+bytes >= p->size )
		while( p->count+bytes >= p->size )
			ByteBuffer_Resize(p);
	
	switch( bytes ) {
		case 2:
			memcpy(p->data+p->count, &value, 2); p->count += 2;
			break;
		case 4:
			memcpy(p->data+p->count, &value, 4); p->count += 4;
			break;
		default:	// assume default as 8 bytes.
			memcpy(p->data+p->count, &value, 8); p->count += 8;
			break;
	}
}

static void ByteBuffer_InsertFloat(struct ByteBuffer *restrict const p, const float fval)
{
	if( !p )
		return;
	else if( p->count+sizeof(float) >= p->size )
		while( p->count+sizeof(float) >= p->size )
			ByteBuffer_Resize(p);
	
	memcpy(p->data+p->count, &fval, sizeof(float)); p->count += sizeof(float);
}

static void ByteBuffer_InsertDouble(struct ByteBuffer *restrict const p, const double fval)
{
	if( !p )
		return;
	else if( p->count+sizeof(double) >= p->size )
		while( p->count+sizeof(double) >= p->size )
			ByteBuffer_Resize(p);
	
	memcpy(p->data+p->count, &fval, sizeof(double)); p->count += sizeof(double);
}

static void ByteBuffer_InsertString(struct ByteBuffer *restrict const p, const char *restrict str, const size_t strsize)
{
	if( !p )
		return;
	else if( p->count+strsize+1 >= p->size )
		while( p->count+strsize+1 >= p->size )
			ByteBuffer_Resize(p);
	
	memcpy(p->data+p->count, str, strsize); p->count += strsize;
	p->data[p->count++] = 0;	// add null terminator.
}

static void ByteBuffer_InsertObject(struct ByteBuffer *restrict const p, const void *restrict o, const size_t size)
{
	if( !p )
		return;
	else if( p->count+size >= p->size )
		while( p->count+size >= p->size )
			ByteBuffer_Resize(p);
	
	memcpy(p->data+p->count, o, size);
	p->count += size;
}

static void ByteBuffer_Delete(struct ByteBuffer *restrict const p, const size_t index)
{
	if( !p or index >= p->count )
		return;
	
	size_t
		i=index+1,
		j=index
	;
	memmove(p->data+j, p->data+i, p->count); p->count--;
}

static void ByteBuffer_Free(struct ByteBuffer *restrict const p)
{
	if( !p )
		return;
	
	if( p->data )
		free(p->data);
	ByteBuffer_Init(p);
}

static void ByteBuffer_Resize(struct ByteBuffer *restrict const p)
{
	if( !p )
		return;
	
	// first we get our old size.
	// then resize the actual size.
	size_t oldsize = p->size;
	p->size <<= 1;
	if( p->size==0 )
		p->size=8;
	
	// allocate new table.
	uint8_t *newdata = calloc(p->size, sizeof(uint8_t));
	assert( newdata );
	
	// copy the old table to new then free old table.
	if( p->data ) {
		memcpy(newdata, p->data, oldsize);
		free(p->data); p->data = NULL;
	}
	p->data = newdata;
}

static void ByteBuffer_DumpToFile(struct ByteBuffer *restrict const p, FILE *restrict file)
{
	if( !p or !p->data or !file )
		return;
	
	fwrite(p->data, sizeof(uint8_t), p->count, file);
}

static void ByteBuffer_ReadFromFile(struct ByteBuffer *restrict const p, FILE *restrict file)
{
	if( !p or !p->data or !file )
		return;
	
	// get the total file size.
	uint64_t filesize = 0;
	if( !fseek(file, 0, SEEK_END) ) {
		filesize = (uint64_t)ftell(file);
		rewind(file);
	}
	
	// check if buffer can hold it.
	// if not, resize until it can.
	if( p->count+filesize >= p->size )
		while( p->count+filesize >= p->size )
			ByteBuffer_Resize(p);
	
	// read in the data.
	fread(p->data, sizeof(uint8_t), filesize, file);
	p->count += filesize;
}

static void ByteBuffer_Append(struct ByteBuffer *restrict p, struct ByteBuffer *o)
{
	if( !p or !o )
		return;
	
	if( p->count+o->count >= p->size )
		while( p->count+o->count >= p->size )
			ByteBuffer_Resize(p);
	
	memcpy(p->data+p->count, o->data, o->count);
	p->count += o->count;
}


struct TaghaTBC *TBCGen_New()
{
	return calloc(1, sizeof(struct TaghaTBC));
}

void TBCGen_Init(struct TaghaTBC *restrict const pTBC)
{
	if( !pTBC )
		return;
	
	// set every struct to 0 in one fell swoop.
	memset(pTBC, 0, sizeof(struct TaghaTBC));
}

void TBCGen_Free(struct TaghaTBC *restrict const pTBC)
{
	if( !pTBC )
		return;
	
	for( uint32_t i=0 ; i<TotalIndices ; i++ )
		ByteBuffer_Free(pTBC->m_arrBytes+i);
}

void TBCGen_WriteHeader(struct TaghaTBC *restrict const pTBC, const uint32_t stacksize, const uint32_t datasize, const uint8_t modeflags)
{
	if( !pTBC )
		return;
	
	ByteBuffer_InsertInt(pTBC->m_arrBytes+Header, TBCMagic, sizeof TBCMagic);
	ByteBuffer_InsertInt(pTBC->m_arrBytes+Header, stacksize, sizeof stacksize);
	ByteBuffer_InsertInt(pTBC->m_arrBytes+Header, datasize, sizeof datasize);
	ByteBuffer_Insert(pTBC->m_arrBytes+Header, modeflags);
}

void TBCGen_WriteNativeTable(struct TaghaTBC *restrict const pTBC, const uint32_t nativecount, ...)
{
	if( !pTBC )
		return;
	
	va_list natives;
	va_start(natives, nativecount);
	char *buf;
	size_t strsize;
	for( uint32_t i=0 ; i<nativecount ; i++ ) {
		buf=va_arg(natives, char*);
		strsize = strlen(buf); // account for '\0' null term.
		
		// write size of the string + null term to native table byte buffer.
		ByteBuffer_InsertInt(pTBC->m_arrBytes+NativeTable, strsize+1, sizeof strsize);
		ByteBuffer_InsertString(pTBC->m_arrBytes+NativeTable, buf, strsize);
	}
	va_end(natives);
}

void TBCGen_WriteFuncTable(struct TaghaTBC *restrict const pTBC, const uint32_t funccount, ...)
{
	if( !pTBC )
		return;
	
	va_list funcs;
	va_start(funcs, funccount);
	char *buf;
	size_t strsize;
	uint32_t offset;
	for( uint32_t i=0 ; i<funccount ; i++ ) {
		buf=va_arg(funcs, char*);
		strsize = strlen(buf); // account for '\0' null term.
		
		// write size of the string + null term to func table byte buffer.
		ByteBuffer_InsertInt(pTBC->m_arrBytes+FuncTable, strsize+1, sizeof strsize);
		ByteBuffer_InsertString(pTBC->m_arrBytes+FuncTable, buf, strsize);
		
		offset = va_arg(funcs, uint32_t);
		ByteBuffer_InsertInt(pTBC->m_arrBytes+FuncTable, offset, sizeof offset);
	}
	va_end(funcs);
}

void TBCGen_WriteGlobalVarTable(struct TaghaTBC *restrict const pTBC, const uint32_t globalcount, ...)
{
	if( !pTBC )
		return;
	
	va_list globals;
	va_start(globals, globalcount);
	char *buf;
	size_t strsize;
	uint32_t offset;
	for( uint32_t i=0 ; i<globalcount ; i++ ) {
		buf=va_arg(globals, char*);
		strsize = strlen(buf); // account for '\0' null term.
		
		ByteBuffer_InsertInt(pTBC->m_arrBytes+GlobalTable, strsize+1, sizeof strsize);
		ByteBuffer_InsertString(pTBC->m_arrBytes+GlobalTable, buf, strsize);
		
		offset = va_arg(globals, uint32_t);
		ByteBuffer_InsertInt(pTBC->m_arrBytes+GlobalTable, offset, sizeof offset);
	}
	va_end(globals);
}

void TBCGen_WriteIntToDataSeg(struct TaghaTBC *restrict const pTBC, const uint64_t data, const size_t size)
{
	if( !pTBC )
		return;
	
	ByteBuffer_InsertInt(pTBC->m_arrBytes+DataSegment, data, size);
}

void TBCGen_WriteStrToDataSeg(struct TaghaTBC *restrict const pTBC, char *restrict str, const size_t strsize)
{
	if( !pTBC )
		return;
	
	ByteBuffer_InsertString(pTBC->m_arrBytes+DataSegment, str, strsize);
}

void TBCGen_WriteOpcodeNoOpers(struct TaghaTBC *restrict const pTBC, const enum InstrSet opcode, const enum AddrMode addrmode)
{
	if( !pTBC )
		return;
	
	ByteBuffer_Insert(pTBC->m_arrBytes+ByteCode, opcode);
	ByteBuffer_Insert(pTBC->m_arrBytes+ByteCode, addrmode);
}

void TBCGen_WriteOpcodeOneOper(struct TaghaTBC *restrict const pTBC, const enum InstrSet opcode, const enum AddrMode addrmode, const uint64_t oper, uint32_t *restrict offset)
{
	if( !pTBC )
		return;
	
	ByteBuffer_Insert(pTBC->m_arrBytes+ByteCode, opcode);
	ByteBuffer_Insert(pTBC->m_arrBytes+ByteCode, addrmode);
	ByteBuffer_InsertInt(pTBC->m_arrBytes+ByteCode, oper, sizeof oper);
	
	// offsets are meant to be used only for using a register as a memory address.
	if( offset )
		ByteBuffer_InsertInt(pTBC->m_arrBytes+ByteCode, *offset, sizeof(uint32_t));
}

void TBCGen_WriteOpcodeTwoOpers(struct TaghaTBC *restrict const pTBC, const enum InstrSet opcode, const enum AddrMode addrmode, const uint64_t oper1, const uint64_t oper2, uint32_t *restrict offset)
{
	if( !pTBC )
		return;
	
	ByteBuffer_Insert(pTBC->m_arrBytes+ByteCode, opcode);
	ByteBuffer_Insert(pTBC->m_arrBytes+ByteCode, addrmode);
	ByteBuffer_InsertInt(pTBC->m_arrBytes+ByteCode, oper1, sizeof oper1);
	ByteBuffer_InsertInt(pTBC->m_arrBytes+ByteCode, oper2, sizeof oper2);
	if( offset )
		ByteBuffer_InsertInt(pTBC->m_arrBytes+ByteCode, *offset, sizeof(uint32_t));
}


void TBCGen_WriteNativeCall(struct TaghaTBC *restrict const pTBC, const enum AddrMode addrmode, const uint64_t oper, uint32_t *restrict offset, const uint32_t argcount)
{
	if( !pTBC )
		return;
	
	ByteBuffer_Insert(pTBC->m_arrBytes+ByteCode, callnat);
	ByteBuffer_Insert(pTBC->m_arrBytes+ByteCode, addrmode);
	ByteBuffer_InsertInt(pTBC->m_arrBytes+ByteCode, oper, sizeof oper);
	if( offset )
		ByteBuffer_InsertInt(pTBC->m_arrBytes+ByteCode, *offset, sizeof(uint32_t));
	ByteBuffer_InsertInt(pTBC->m_arrBytes+ByteCode, argcount, sizeof argcount);
}


void TBCGen_ToFile(struct TaghaTBC *restrict const pTBC, FILE *restrict const pFile)
{
	if( !pTBC or !pFile )
		return;
	
	struct ByteBuffer TBCScript; ByteBuffer_Init(&TBCScript);
	ByteBuffer_Append(&TBCScript, pTBC->m_arrBytes+Header);
	ByteBuffer_Append(&TBCScript, pTBC->m_arrBytes+NativeTable);
	ByteBuffer_Append(&TBCScript, pTBC->m_arrBytes+GlobalTable);
	ByteBuffer_Append(&TBCScript, pTBC->m_arrBytes+DataSegment);
	ByteBuffer_Append(&TBCScript, pTBC->m_arrBytes+ByteCode);
	ByteBuffer_DumpToFile(&TBCScript, pFile);
	ByteBuffer_Free(&TBCScript);
}
