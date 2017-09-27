#include <stdlib.h>
#include <assert.h>
#include <iso646.h>		// and, or, not
#include <string.h>
#include <stdio.h>
#include "ds.h"

void vector_init(Vec_t *v)
{
	if( !v )
		return;
	
	v->data = NULL;
	v->size = v->count = 0;
}

uint32_t vector_count(const Vec_t *v)
{
	if( !v )
		return 0;
	return v->count;
}

void vector_add(Vec_t *restrict v, void *restrict e)
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

void vector_set(Vec_t *restrict v, const uint32_t index, void *restrict e)
{
	if( !v or index >= v->count )
		return;
	
	v->data[index] = e;
}

void *vector_get(const Vec_t *v, const uint32_t index)
{
	if( !v or index >= v->count )
		return NULL;
	
	return v->data[index];
}

void vector_delete(Vec_t *v, const uint32_t index)
{
	if( !v or index >= v->count )
		return;
	
	for( uint32_t i = index+1, j = index ; i < v->count ; i++ )
		v->data[j++] = v->data[i];
	
	v->count--;
}

void vector_free(Vec_t *v)
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


void map_init(Map_t *map)
{
	if( !map )
		return;
	
	map->table = NULL;
	map->size = map->count = 0;
}

void map_free(Map_t *map)
{
	if( !map || !map->table )
		return;
	
	/*
	 * If the Map_tionary pointer is not "const", then
	 * you have to make two traversing kvnode_t pointers
	 * or else you'll get a nice little segfault ;)
	 * not sure why but whatever makes the code work I guess.
	*/
	kvnode_t
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

bool map_insert(Map_t *restrict map, const char *restrict szKey, const uint64_t pData)
{
	if( !map )
		return false;
	
	if( map->size == 0 ) {
		map->size = 8;
		map->table = calloc(map->size, sizeof(kvnode_t));
		
		if( !map->table ) {
			printf("**** Memory Allocation Error **** map_insert::map->table is NULL\n");
			map->size = 0;
			return false;
		}
	}
	else if( map->count >= map->size ) {
		map_rehash(map);
		//printf("**** Rehashed Map_tionary ****\n");
		//printf("**** Map_tionary Size is now %llu ****\n", map->size);
	}
	else if( map_has_key(map, szKey) ) {
		printf("map_insert::map already has entry!\n");
		return false;
	}
	
	kvnode_t *node = malloc( sizeof(kvnode_t) );
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

uint64_t map_find(const Map_t *restrict map, const char *restrict szKey)
{
	if( !map )
		return 0;
	else if( !map->table )
		return 0;
	/*
	 * if Map_tionary pointer is const, you only
	 * need to use one traversing kvnode_t
	 * pointer without worrying of a segfault
	*/
	kvnode_t *kv;
	uint32_t hash = gethash32(szKey) % map->size;
	for( kv = map->table[hash] ; kv ; kv = kv->pNext )
		if( !strcmp(kv->strKey, szKey) )
			return kv->pData;
	return 0;
}

void map_delete(Map_t *restrict map, const char *restrict szKey)
{
	if( !map )
		return;
	
	if( !map_has_key(map, szKey) )
		return;
	
	uint32_t hash = gethash32(szKey) % map->size;
	kvnode_t
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

bool map_has_key(const Map_t *restrict map, const char *restrict szKey)
{
	if( !map || !map->table )
		return false;
	
	kvnode_t *prev;
	uint32_t hash = gethash32(szKey) % map->size;
	for( prev = map->table[hash] ; prev ; prev = prev->pNext )
		if( !strcmp(prev->strKey, szKey) )
			return true;
	
	return false;
}

uint64_t map_len(const Map_t *map)
{
	if( !map )
		return 0L;
	return map->count;
}

// Rehashing increases Map_tionary size by a factor of 2
void map_rehash(Map_t *map)
{
	if( !map )
		return;
	
	uint32_t old_size = map->size;
	map->size <<= 1;
	map->count = 0;
	
	kvnode_t **curr, **temp;
	temp = calloc(map->size, sizeof(kvnode_t));
	if( !temp ) {
		printf("**** Memory Allocation Error **** map_insert::temp is NULL\n");
		map->size = 0;
		return;
	}
	
	curr = map->table;
	map->table = temp;
	
	kvnode_t
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

const char *map_get_key(const Map_t *restrict map, const char *restrict szKey)
{
	if( !map || !map->table )
		return NULL;
	
	kvnode_t *prev;
	uint32_t hash = gethash32(szKey) % map->size;
	for( prev = map->table[hash] ; prev ; prev = prev->pNext )
		if( !strcmp(prev->strKey, szKey) )
			return prev->strKey;
	
	return NULL;
}



/*
bool map_insert_int(Map_t *restrict map, const uint64_t key, void *restrict pData)
{
	if( !map )
		return false;
	
	if( map->size == 0 ) {
		map->size = 8;
		map->table = calloc(map->size, sizeof(kvnode_t));
		
		if( !map->table ) {
			printf("**** Memory Allocation Error **** map_insert_int::map->table is NULL\n");
			map->size = 0;
			return false;
		}
	}
	else if( map->count >= map->size ) {
		map_rehash(map);
		//printf("**** Rehashed Map_tionary ****\n");
		//printf("**** Map_tionary Size is now %llu ****\n", map->size);
	}
	else if( map_has_key_int(map, key) ) {
		printf("map_insert_int::map already has entry!\n");
		return false;
	}
	
	kvnode_t *node = malloc( sizeof(kvnode_t) );
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

void *map_find_int(const Map_t *map, const uint64_t key)
{
	if( !map )
		return NULL;
	else if( !map->table )
		return NULL;
	
	kvnode_t *kv;
	uint64_t hash = int64hash(key) % map->size;
	for( kv = map->table[hash] ; kv ; kv = kv->pNext )
		if( kv->i64Key==key )
			return kv->pData;
	
	return NULL;
}

void map_delete_int(Map_t *map, const uint64_t key)
{
	if( !map )
		return;
	
	if( !map_has_key_int(map, key) )
		return;
	
	uint64_t hash = int64hash(key) % map->size;
	kvnode_t
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
bool map_has_key_int(const Map_t *map, const uint64_t key)
{
	if( !map )
		return false;
	
	kvnode_t *prev;
	uint64_t hash = int64hash(key) % map->size;
	for( prev = map->table[hash] ; prev ; prev = prev->pNext )
		if( prev->i64Key==key )
			return true;
	
	return false;
}

void map_rehash_int(Map_t *map)
{
	if( !map )
		return;
	
	uint32_t old_size = map->size;
	map->size <<= 1;
	map->count = 0;
	
	kvnode_t **curr, **temp;
	temp = calloc(map->size, sizeof(kvnode_t));
	if( !temp ) {
		printf("**** Memory Allocation Error **** map_insert::temp is NULL\n");
		map->size = 0;
		return;
	}
	
	curr = map->table;
	map->table = temp;
	
	kvnode_t
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
