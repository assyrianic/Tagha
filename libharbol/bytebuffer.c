
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#ifdef OS_WINDOWS
	#define HARBOL_LIB
#endif
#include "harbol.h"


HARBOL_EXPORT struct HarbolByteBuffer *harbol_bytebuffer_new(void)
{
	return calloc(1, sizeof(struct HarbolByteBuffer));
}

HARBOL_EXPORT void harbol_bytebuffer_init(struct HarbolByteBuffer *const p)
{
	if( !p )
		return;
	
	memset(p, 0, sizeof *p);
}

HARBOL_EXPORT size_t harbol_bytebuffer_get_len(const struct HarbolByteBuffer *const p)
{
	return p ? p->Len : 0;
}

HARBOL_EXPORT size_t harbol_bytebuffer_get_count(const struct HarbolByteBuffer *const p)
{
	return p ? p->Count : 0;
}

HARBOL_EXPORT uint8_t *harbol_bytebuffer_get_raw_buffer(const struct HarbolByteBuffer *const p)
{
	return p ? p->Buffer : NULL;
}

HARBOL_EXPORT void harbol_bytebuffer_insert_byte(struct HarbolByteBuffer *const p, const uint8_t byte)
{
	if( !p )
		return;
	else if( p->Count >= p->Len )
		harbol_bytebuffer_resize(p);
	
	p->Buffer[p->Count++] = byte;
}

HARBOL_EXPORT void harbol_bytebuffer_insert_integer(struct HarbolByteBuffer *const p, const uint64_t value, const size_t bytes)
{
	if( !p )
		return;
	else if( p->Count+bytes >= p->Len )
		while( p->Count+bytes >= p->Len )
			harbol_bytebuffer_resize(p);
	
	memcpy(p->Buffer+p->Count, &value, bytes);
	p->Count += bytes;
}

HARBOL_EXPORT void harbol_bytebuffer_insert_float32(struct HarbolByteBuffer *const p, const float fval)
{
	if( !p )
		return;
	else if( p->Count+sizeof fval >= p->Len )
		while( p->Count+sizeof fval >= p->Len )
			harbol_bytebuffer_resize(p);
	
	memcpy(p->Buffer+p->Count, &fval, sizeof fval);
	p->Count += sizeof fval;
}

HARBOL_EXPORT void harbol_bytebuffer_insert_float64(struct HarbolByteBuffer *const p, const double fval)
{
	if( !p )
		return;
	else if( p->Count+sizeof fval >= p->Len )
		while( p->Count+sizeof fval >= p->Len )
			harbol_bytebuffer_resize(p);
	
	memcpy(p->Buffer+p->Count, &fval, sizeof fval);
	p->Count += sizeof fval;
}

HARBOL_EXPORT void harbol_bytebuffer_insert_cstr(struct HarbolByteBuffer *const restrict p, const char str[restrict], const size_t strsize)
{
	if( !p )
		return;
	else if( p->Count+strsize+1 >= p->Len )
		while( p->Count+strsize+1 >= p->Len )
			harbol_bytebuffer_resize(p);
	
	memcpy(p->Buffer+p->Count, str, strsize);
	p->Count += strsize;
	p->Buffer[p->Count++] = 0;	// add null terminat||.
}

HARBOL_EXPORT void harbol_bytebuffer_insert_obj(struct HarbolByteBuffer *const restrict p, const void *restrict o, const size_t size)
{
	if( !p )
		return;
	else if( p->Count+size >= p->Len )
		while( p->Count+size >= p->Len )
			harbol_bytebuffer_resize(p);
	
	memcpy(p->Buffer+p->Count, o, size);
	p->Count += size;
}

HARBOL_EXPORT void harbol_bytebuffer_insert_zeros(struct HarbolByteBuffer *const p, const size_t zeroes)
{
	if( !p )
		return;
	else if( p->Count+zeroes >= p->Len )
		while( p->Count+zeroes >= p->Len )
			harbol_bytebuffer_resize(p);
	
	memset(p->Buffer+p->Count, 0, zeroes);
	p->Count += zeroes;
}

HARBOL_EXPORT void harbol_bytebuffer_delete_byte(struct HarbolByteBuffer *const p, const size_t index)
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

HARBOL_EXPORT void harbol_bytebuffer_del(struct HarbolByteBuffer *const p)
{
	if( !p )
		return;
	
	if( p->Buffer )
		free(p->Buffer);
	
	memset(p, 0, sizeof *p);
}

HARBOL_EXPORT void harbol_bytebuffer_free(struct HarbolByteBuffer **pref)
{
	if( !*pref )
		return;
	
	harbol_bytebuffer_del(*pref);
	free(*pref), *pref=NULL;
}

HARBOL_EXPORT void harbol_bytebuffer_resize(struct HarbolByteBuffer *const restrict p)
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

HARBOL_EXPORT void harbol_bytebuffer_to_file(const struct HarbolByteBuffer *const p, FILE *const file)
{
	if( !p || !p->Buffer || !file )
		return;
	
	fwrite(p->Buffer, sizeof *p->Buffer, p->Count, file);
}

HARBOL_EXPORT size_t harbol_bytebuffer_read_from_file(struct HarbolByteBuffer *const p, FILE *const file)
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
			harbol_bytebuffer_resize(p);
	
	// read in the data.
	const size_t val = fread(p->Buffer, sizeof *p->Buffer, filesize, file);
	p->Count += filesize;
	return val;
}

HARBOL_EXPORT void harbol_bytebuffer_append(struct HarbolByteBuffer *restrict p, struct HarbolByteBuffer *restrict o)
{
	if( !p || !o || !o->Buffer || p==o )
		return;
	
	if( p->Count+o->Count >= p->Len )
		while( p->Count+o->Count >= p->Len )
			harbol_bytebuffer_resize(p);
	
	memcpy(p->Buffer+p->Count, o->Buffer, o->Count);
	p->Count += o->Count;
}
