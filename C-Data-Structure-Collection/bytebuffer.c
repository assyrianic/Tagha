
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "dsc.h"


struct ByteBuffer *ByteBuffer_New(void)
{
	return calloc(1, sizeof(struct ByteBuffer));
}

void ByteBuffer_Init(struct ByteBuffer *const p)
{
	if( !p )
		return;
	
	*p = (struct ByteBuffer){0};
}

size_t ByteBuffer_Len(const struct ByteBuffer *const p)
{
	return p ? p->Len : 0;
}
size_t ByteBuffer_Count(const struct ByteBuffer *const p)
{
	return p ? p->Count : 0;
}
uint8_t *ByteBuffer_GetBuffer(const struct ByteBuffer *const p)
{
	return p ? p->Buffer : NULL;
}

void ByteBuffer_InsertByte(struct ByteBuffer *const p, const uint8_t byte)
{
	if( !p )
		return;
	else if( p->Count >= p->Len )
		ByteBuffer_Resize(p);
	
	p->Buffer[p->Count++] = byte;
}

void ByteBuffer_InsertInt(struct ByteBuffer *const p, const uint64_t value, const size_t bytes)
{
	if( !p )
		return;
	else if( p->Count+bytes >= p->Len )
		while( p->Count+bytes >= p->Len )
			ByteBuffer_Resize(p);
	
	memcpy(p->Buffer+p->Count, &value, bytes);
	p->Count += bytes;
}

void ByteBuffer_InsertFloat(struct ByteBuffer *const p, const float fval)
{
	if( !p )
		return;
	else if( p->Count+sizeof fval >= p->Len )
		while( p->Count+sizeof fval >= p->Len )
			ByteBuffer_Resize(p);
	
	memcpy(p->Buffer+p->Count, &fval, sizeof fval);
	p->Count += sizeof fval;
}

void ByteBuffer_InsertDouble(struct ByteBuffer *const p, const double fval)
{
	if( !p )
		return;
	else if( p->Count+sizeof fval >= p->Len )
		while( p->Count+sizeof fval >= p->Len )
			ByteBuffer_Resize(p);
	
	memcpy(p->Buffer+p->Count, &fval, sizeof fval);
	p->Count += sizeof fval;
}

void ByteBuffer_InsertString(struct ByteBuffer *const restrict p, const char *restrict str, const size_t strsize)
{
	if( !p )
		return;
	else if( p->Count+strsize+1 >= p->Len )
		while( p->Count+strsize+1 >= p->Len )
			ByteBuffer_Resize(p);
	
	memcpy(p->Buffer+p->Count, str, strsize);
	p->Count += strsize;
	p->Buffer[p->Count++] = 0;	// add null terminat||.
}

void ByteBuffer_InsertObject(struct ByteBuffer *const restrict p, const void *restrict o, const size_t size)
{
	if( !p )
		return;
	else if( p->Count+size >= p->Len )
		while( p->Count+size >= p->Len )
			ByteBuffer_Resize(p);
	
	memcpy(p->Buffer+p->Count, o, size);
	p->Count += size;
}

void ByteBuffer_InsertZeroes(struct ByteBuffer *const p, const size_t zeroes)
{
	if( !p )
		return;
	else if( p->Count+zeroes >= p->Len )
		while( p->Count+zeroes >= p->Len )
			ByteBuffer_Resize(p);
	
	memset(p->Buffer+p->Count, 0, zeroes);
	p->Count += zeroes;
}

void ByteBuffer_Delete(struct ByteBuffer *const p, const size_t index)
{
	if( !p || index >= p->Count )
		return;
	
	const size_t
		i=index+1,
		j=index
	;
	memmove(p->Buffer+j, p->Buffer+i, p->Count);
	p->Count--;
}

void ByteBuffer_Del(struct ByteBuffer *const p)
{
	if( !p )
		return;
	
	if( p->Buffer )
		free(p->Buffer);
	
	*p = (struct ByteBuffer){0};
}

void ByteBuffer_Free(struct ByteBuffer **pref)
{
	if( !*pref )
		return;
	
	ByteBuffer_Del(*pref);
	free(*pref);
	*pref=NULL;
}

void ByteBuffer_Resize(struct ByteBuffer *const restrict p)
{
	if( !p )
		return;
	
	// first we get our old size.
	// then resize the actual size.
	size_t oldsize = p->Len;
	p->Len <<= 1;
	if( p->Len==0 )
		p->Len=4;
	
	// allocate new table.
	uint8_t *newdata = calloc(p->Len, sizeof *newdata);
	assert( newdata );
	
	// copy the old table to new then free old table.
	if( p->Buffer ) {
		memcpy(newdata, p->Buffer, oldsize);
		free(p->Buffer), p->Buffer = NULL;
	}
	p->Buffer = newdata;
}

void ByteBuffer_DumpToFile(const struct ByteBuffer *const p, FILE *const file)
{
	if( !p || !p->Buffer || !file )
		return;
	
	fwrite(p->Buffer, sizeof *p->Buffer, p->Count, file);
}

size_t ByteBuffer_ReadFromFile(struct ByteBuffer *const p, FILE *const file)
{
	if( !p || !file )
		return 0;
	
	// get the total file size.
	fseek(file, 0, SEEK_END);
	const int64_t filesize = ftell(file);
	if( filesize <= -1 )
		return 0;
	
	rewind(file);
	
	// check if buffer can hold it.
	// if not, resize until it can.
	if( p->Count+filesize >= p->Len )
		while( p->Count+filesize >= p->Len )
			ByteBuffer_Resize(p);
	
	// read in the data.
	const size_t val = fread(p->Buffer, sizeof *p->Buffer, filesize, file);
	p->Count += filesize;
	return val;
}

void ByteBuffer_Append(struct ByteBuffer *restrict p, struct ByteBuffer *restrict o)
{
	if( !p || !o || !o->Buffer || p==o )
		return;
	
	if( p->Count+o->Count >= p->Len )
		while( p->Count+o->Count >= p->Len )
			ByteBuffer_Resize(p);
	
	memcpy(p->Buffer+p->Count, o->Buffer, o->Count);
	p->Count += o->Count;
}
