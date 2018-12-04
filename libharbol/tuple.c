#include <stdlib.h>
#include <stdio.h>
#include <stdalign.h>
#ifdef OS_WINDOWS
	#define HARBOL_LIB
#endif
#include "harbol.h"

typedef uint32_t tuple_size_t;
typedef union {
	uint64_t PackedInt64;
	struct { tuple_size_t Offset, Size; };
	union HarbolValue Val;
} TupleElement;

/*
static size_t CalcPadding(const size_t offset, const size_t align)
{
	return -offset & (align - 1);
}
*/

static size_t AlignSize(const size_t size, const size_t align)
{
	return (size + (align-1)) & -align;
}


HARBOL_EXPORT struct HarbolTuple *harbol_tuple_new(const size_t array_len, const size_t datasizes[static array_len], const bool packed)
{
	struct HarbolTuple *tup = calloc(1, sizeof *tup);
	harbol_tuple_init(tup, array_len, datasizes, packed);
	return tup;
}

HARBOL_EXPORT bool harbol_tuple_free(struct HarbolTuple **tupref)
{
	if( !tupref || !*tupref )
		return false;
	harbol_tuple_del(*tupref);
	free(*tupref), *tupref=NULL;
	return true;
}

HARBOL_EXPORT void harbol_tuple_init(struct HarbolTuple *const tup, const size_t array_len, const size_t datasizes[static array_len], const bool packed)
{
	if( !tup )
		return;
	
	memset(tup, 0, sizeof *tup);
	const size_t sizeptr = sizeof(intptr_t);
	size_t largestmemb=0;
	// first we find the largest member of the tuple:
	for( size_t i=0 ; i<array_len ; i++ )
		if( largestmemb<datasizes[i] )
			largestmemb=datasizes[i];
	
	// next, compute padding and alignment:
	// we do this by having a next and previous size.
	size_t
		totalsize=0,
		prevsize=0
	;
	for( size_t i=0 ; i<array_len ; i++ ) {
		totalsize += datasizes[i];
		if( packed || array_len==1 )
			continue;
		const size_t offalign = (i+1<array_len) ? datasizes[i+1] : prevsize;
		totalsize = AlignSize(totalsize, offalign>=sizeptr ? sizeptr : offalign);
		prevsize = datasizes[i];
	}
	// now do a final size alignment with the largest member.
	const size_t aligned_total = AlignSize(totalsize, largestmemb>=sizeptr ? sizeptr : largestmemb);
	
	tup->Datum = calloc(packed ? totalsize : aligned_total, sizeof *tup->Datum);
	if( !tup->Datum )
		return;
	
	tup->Len = packed ? totalsize : aligned_total;
	tuple_size_t offset = 0;
	for( size_t i=0 ; i<array_len ; i++ ) {
		TupleElement field = {0};
		field.Size = datasizes[i];
		field.Offset = offset;
		
		harbol_vector_insert(&tup->Fields, field.Val);
		offset += datasizes[i];
		if( packed || array_len==1 )
			continue;
		const size_t offalign = (i+1<array_len) ? datasizes[i+1] : prevsize;
		offset = AlignSize(offset, offalign>=sizeptr ? sizeptr : offalign);
		prevsize = datasizes[i];
	}
}

HARBOL_EXPORT void harbol_tuple_del(struct HarbolTuple *const tup)
{
	if( !tup )
		return;
	harbol_vector_del(&tup->Fields, NULL);
	free(tup->Datum);
	memset(tup, 0, sizeof *tup);
}

HARBOL_EXPORT size_t harbol_tuple_get_len(const struct HarbolTuple *const tup)
{
	return tup ? tup->Len : 0;
}

HARBOL_EXPORT void *harbol_tuple_get_field(const struct HarbolTuple *const tup, const size_t index)
{
	if( !tup || !tup->Datum || index>=tup->Fields.Count )
		return NULL;
	const TupleElement field = {harbol_vector_get(&tup->Fields, index).UInt64};
	return ( field.Offset >= tup->Len ) ? NULL : tup->Datum + field.Offset;
}

HARBOL_EXPORT void *harbol_tuple_set_field(const struct HarbolTuple *const restrict tup, const size_t index, void *restrict ptrvalue)
{
	if( !tup || !tup->Datum || !ptrvalue )
		return NULL;
	const TupleElement field = {harbol_vector_get(&tup->Fields, index).UInt64};
	if( field.Offset >= tup->Len )
		return NULL;
	void *restrict ptr_field = tup->Datum + field.Offset;
	memcpy(ptr_field, ptrvalue, field.Size);
	return ptr_field;
}

HARBOL_EXPORT size_t harbol_tuple_get_field_size(const struct HarbolTuple *const tup, const size_t index)
{
	if( !tup || !tup->Datum || index>=tup->Fields.Count )
		return 0;
	const TupleElement field = {harbol_vector_get(&tup->Fields, index).UInt64};
	return field.Size;
}

HARBOL_EXPORT bool harbol_tuple_is_packed(const struct HarbolTuple *const tup)
{
	return tup ? tup->Packed : false;
}

HARBOL_EXPORT bool harbol_tuple_to_struct(const struct HarbolTuple *const restrict tup, void *restrict structptr)
{
	if( !tup || !structptr || !tup->Datum )
		return false;
	
	memcpy(structptr, tup->Datum, tup->Len);
	return true;
}
