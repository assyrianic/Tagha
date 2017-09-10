#include <stdlib.h>
#include <assert.h>
#include "vector.h"

void vector_init(struct vector *v)
{
	if( !v )
		return;
	
	v->data = NULL;
	v->size = v->count = 0;
}

inline unsigned vector_count(const struct vector *v)
{
	if( !v )
		return 0;
	return v->count;
}

void vector_add(struct vector *restrict v, void *restrict e)
{
	if( !v )
		return;
	
	if( v->size == 0 ) {
		v->size = 2;
		v->data = calloc(v->size, sizeof(void *));
		assert( v->data );
	}
	else if( v->size >= v->count ) {
		v->size <<= 1;
		v->data = realloc(v->data, sizeof(void *) * v->size);
		assert( v->data );
	}
	
	v->data[v->count] = e;
	v->count++;
}

void vector_set(struct vector *restrict v, const unsigned index, void *restrict e)
{
	if( !v )
		return;
	else if( index >= v->count )
		return;
	
	v->data[index] = e;
}

void *vector_get(const struct vector *v, const unsigned index)
{
	if( !v )
		return NULL;
	
	else if( index >= v->count )
		return NULL;

	return v->data[index];
}

void vector_delete(struct vector *v, const unsigned index)
{
	if( !v )
		return;
	
	else if( index >= v->count )
		return;
	
	for( unsigned i = index+1, j = index ; i < v->count ; i++ )
		v->data[j++] = v->data[i];
	
	v->count--;
}

void vector_free(struct vector *v)
{
	if( !v )
		return;
	if( v->data )
		free(v->data);
	vector_init(v);
}
