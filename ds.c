#include <stdlib.h>
#include <assert.h>
#include <iso646.h>		// and, or, not
#include <string.h>
#include <stdio.h>
#include "ds.h"

/*
static int strcmp(register const char *restrict s1, register const char *restrict s2)
{
	while (*s1 == *s2++)
		if (*s1++ == 0)
			return (0);
	return( *(const unsigned char *)s1 - *(const unsigned char *)(s2 - 1) );
}
*/

void vector_init(struct vector *v)
{
	if( !v )
		return;
	
	v->data = NULL;
	v->size = v->count = 0;
}

uint32_t vector_count(const struct vector *v)
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
		v->size = 4;
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

void vector_set(struct vector *restrict v, const uint32_t index, void *restrict e)
{
	if( !v or index >= v->count )
		return;
	
	v->data[index] = e;
}

void *vector_get(const struct vector *v, const uint32_t index)
{
	if( !v or index >= v->count )
		return NULL;
	
	return v->data[index];
}

void vector_delete(struct vector *v, const uint32_t index)
{
	if( !v or index >= v->count )
		return;
	
	for( uint32_t i = index+1, j = index ; i < v->count ; i++ )
		v->data[j++] = v->data[i];
	
	v->count--;
}

void vector_free(struct vector *v)
{
	if( !v or !v->data )
		return;
	if( v->data )
		free(v->data);
	vector_init(v);
}






uint32_t gethash32(const char *szKey)
{
	const uint8_t *us;
	uint32_t h = 0;
	for( us=(const uint8_t *)szKey ; *us ; us++ )
		h = 37 * h + *us;
	return h;
}

uint64_t gethash64(const char *szKey)
{
	char *us;
	uint64_t h = 0;
	for (us = (char *)szKey; *us; us++)
		h = h * 147 + *us;

	return h;
}

uint32_t int32hash(uint32_t x)
{
	x = ((x >> 16) ^ x) * 0x45d9f3b;
	x = ((x >> 16) ^ x) * 0x45d9f3b;
	x = (x >> 16) ^ x;
	return x;
}

uint64_t int64hash(uint64_t x)
{
	x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
	x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
	x = x ^ (x >> 31);
	return x;
}


void map_init(struct hashmap *map)
{
	if( !map )
		return;
	
	map->table = NULL;
	map->size = map->count = 0;
}

void map_free(struct hashmap *map)
{
	if( !map || !map->table )
		return;
	
	/*
	 * If the struct hashmapionary pointer is not "const", then
	 * you have to make two traversing struct kvnode pointers
	 * or else you'll get a nice little segfault ;)
	 * not sure why but whatever makes the code work I guess.
	*/
	struct kvnode
		*kv = NULL,
		*next = NULL
	;
	for( uint32_t i=0 ; i<map->size ; i++ ) {
		for( kv = map->table[i] ; kv ; kv = next ) {
			next = kv->pNext;
			kv->pData = 0;
			kv->strKey = NULL;
			free(kv), kv = NULL;
		}
	}
	if( map->table )
		free(map->table);
	map_init(map);
}

bool map_insert(struct hashmap *restrict map, const char *restrict szKey, const uint64_t pData)
{
	if( !map )
		return false;
	
	if( map->size == 0 ) {
		map->size = 8;
		map->table = calloc(map->size, sizeof(struct kvnode));
		
		if( !map->table ) {
			printf("**** Memory Allocation Error **** map_insert::map->table is NULL\n");
			map->size = 0;
			return false;
		}
	}
	else if( map->count >= map->size ) {
		map_rehash(map);
		//printf("**** Rehashed struct hashmapionary ****\n");
		//printf("**** struct hashmapionary Size is now %llu ****\n", map->size);
	}
	else if( map_has_key(map, szKey) ) {
		printf("map_insert::map already has entry!\n");
		return false;
	}
	
	struct kvnode *node = calloc(1, sizeof(struct kvnode) );
	if( !node ) {
		printf("**** Memory Allocation Error **** map_insert::node is NULL\n");
		return false;
	}
	node->strKey = szKey;
	node->pData = pData;
	
	uint32_t hash = gethash32(szKey) % map->size;
	node->pNext = map->table[hash];
	map->table[hash] = node;
	++map->count;
	return true;
}

uint64_t map_find(const struct hashmap *restrict map, const char *restrict szKey)
{
	if( !map || !map->table )
		return 0;
	/*
	 * if struct hashmapionary pointer is const, you only
	 * need to use one traversing struct kvnode
	 * pointer without worrying of a segfault
	*/
	struct kvnode *kv;
	uint32_t hash = gethash32(szKey) % map->size;
	for( kv = map->table[hash] ; kv ; kv = kv->pNext )
		if( !strcmp(kv->strKey, szKey) )
			return kv->pData;
	return 0;
}

void map_delete(struct hashmap *restrict map, const char *restrict szKey)
{
	if( !map )
		return;
	
	if( !map_has_key(map, szKey) )
		return;
	
	uint32_t hash = gethash32(szKey) % map->size;
	struct kvnode
		*kv = NULL,
		*next = NULL
		;
	for( kv = map->table[hash] ; kv ; kv = next ) {
		next = kv->pNext;
		if( !strcmp(kv->strKey, szKey) ) {
			map->table[hash] = kv->pNext;
			free(kv), kv = NULL;
			map->count--;
		}
	}
}

bool map_has_key(const struct hashmap *restrict map, const char *restrict szKey)
{
	if( !map || !map->table )
		return false;
	
	struct kvnode *prev;
	uint32_t hash = gethash32(szKey) % map->size;
	for( prev = map->table[hash] ; prev ; prev = prev->pNext )
		if( !strcmp(prev->strKey, szKey) )
			return true;
	
	return false;
}

uint64_t map_len(const struct hashmap *map)
{
	if( !map )
		return 0L;
	return map->count;
}

// Rehashing increases struct hashmapionary size by a factor of 2
void map_rehash(struct hashmap *map)
{
	if( !map || !map->table )
		return;
	
	uint32_t old_size = map->size;
	map->size <<= 1;
	map->count = 0;
	
	struct kvnode **curr, **temp;
	temp = calloc(map->size, sizeof(struct kvnode));
	if( !temp ) {
		printf("**** Memory Allocation Error **** map_insert::temp is NULL\n");
		map->size = 0;
		return;
	}
	
	curr = map->table;
	map->table = temp;
	
	struct kvnode
		*kv = NULL,
		*next = NULL
		;
	for( uint32_t i=0 ; i<old_size ; ++i ) {
		if( !curr[i] )
			continue;
		for( kv = curr[i] ; kv ; kv = next ) {
			next = kv->pNext;
			map_insert(map, kv->strKey, kv->pData);
			// free the inner nodes since they'll be re-hashed
			kv->strKey=NULL, kv->pData=0;
			free(kv), kv = NULL;
			//printf("**** Rehashed Entry ****\n");
		}
	}
	if( curr )
		free(curr);
	curr = NULL;
}

const char *map_get_key(const struct hashmap *restrict map, const char *restrict szKey)
{
	if( !map || !map->table )
		return NULL;
	
	struct kvnode *prev;
	uint32_t hash = gethash32(szKey) % map->size;
	for( prev = map->table[hash] ; prev ; prev = prev->pNext )
		if( !strcmp(prev->strKey, szKey) )
			return prev->strKey;
	
	return NULL;
}



/*
bool map_insert_int(struct hashmap *restrict map, const uint64_t key, void *restrict pData)
{
	if( !map )
		return false;
	
	if( map->size == 0 ) {
		map->size = 8;
		map->table = calloc(map->size, sizeof(struct kvnode));
		
		if( !map->table ) {
			printf("**** Memory Allocation Error **** map_insert_int::map->table is NULL\n");
			map->size = 0;
			return false;
		}
	}
	else if( map->count >= map->size ) {
		map_rehash(map);
		//printf("**** Rehashed struct hashmapionary ****\n");
		//printf("**** struct hashmapionary Size is now %llu ****\n", map->size);
	}
	else if( map_has_key_int(map, key) ) {
		printf("map_insert_int::map already has entry!\n");
		return false;
	}
	
	struct kvnode *node = calloc(1, sizeof(struct kvnode) );
	if( !node ) {
		printf("**** Memory Allocation Error **** map_insert_int::node is NULL\n");
		return false;
	}
	node->i64Key = key;
	node->pData = pData;
	
	uint64_t hash = int64hash(key) % map->size;
	node->pNext = map->table[hash];
	map->table[hash] = node;
	++map->count;
	return true;
}

void *map_find_int(const struct hashmap *map, const uint64_t key)
{
	if( !map )
		return NULL;
	else if( !map->table )
		return NULL;
	
	struct kvnode *kv;
	uint64_t hash = int64hash(key) % map->size;
	for( kv = map->table[hash] ; kv ; kv = kv->pNext )
		if( kv->i64Key==key )
			return kv->pData;
	
	return NULL;
}

void map_delete_int(struct hashmap *map, const uint64_t key)
{
	if( !map )
		return;
	
	if( !map_has_key_int(map, key) )
		return;
	
	uint64_t hash = int64hash(key) % map->size;
	struct kvnode
		*kv = NULL,
		*next = NULL
	;
	for( kv = map->table[hash] ; kv ; kv = next ) {
		next = kv->pNext;
		if( kv->i64Key==key ) {
			map->table[hash] = kv->pNext;
			free(kv), kv = NULL;
			map->count--;
		}
	}
}
bool map_has_key_int(const struct hashmap *map, const uint64_t key)
{
	if( !map )
		return false;
	
	struct kvnode *prev;
	uint64_t hash = int64hash(key) % map->size;
	for( prev = map->table[hash] ; prev ; prev = prev->pNext )
		if( prev->i64Key==key )
			return true;
	
	return false;
}

void map_rehash_int(struct hashmap *map)
{
	if( !map )
		return;
	
	uint32_t old_size = map->size;
	map->size <<= 1;
	map->count = 0;
	
	struct kvnode **curr, **temp;
	temp = calloc(map->size, sizeof(struct kvnode));
	if( !temp ) {
		printf("**** Memory Allocation Error **** map_insert::temp is NULL\n");
		map->size = 0;
		return;
	}
	
	curr = map->table;
	map->table = temp;
	
	struct kvnode
		*kv = NULL,
		*next = NULL
	;
	for( uint32_t i=0 ; i<old_size ; ++i ) {
		if( !curr[i] )
			continue;
		for( kv = curr[i] ; kv ; kv = next ) {
			next = kv->pNext;
			map_insert_int(map, kv->i64Key, kv->pData);
			// free the inner nodes since they'll be re-hashed
			free(kv), kv = NULL;
			//printf("**** Rehashed Entry ****\n");
		}
	}
	if( curr )
		free(curr);
	curr = NULL;
}
*/
