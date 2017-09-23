#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "hashmap.h"

uint32_t gethash32(const char *szKey)
{
	const unsigned char *us;
	unsigned h = 0;
	for( us=(unsigned const char *)szKey ; *us ; us++ )
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


void dict_init(dict *d)
{
	if( !d )
		return;
	
	d->table = NULL;
	d->size = d->count = 0;
}

void dict_free(dict *d)
{
	if( !d || !d->table )
		return;
	
	/*
	 * If the dictionary pointer is not "const", then
	 * you have to make two traversing kvnode_t pointers
	 * or else you'll get a nice little segfault ;)
	 * not sure why but whatever makes the code work I guess.
	*/
	kvnode_t
		*kv = NULL,
		*next = NULL
		;
	for( unsigned i=0 ; i<d->size ; i++ ) {
		for( kv = d->table[i] ; kv ; kv = next ) {
			next = kv->pNext;
			kv->pData = NULL;
			kv->strKey = NULL;
			free(kv), kv = NULL;
		}
	}
	if( d->table )
		free(d->table);
	dict_init(d);
}

bool dict_insert(dict *restrict d, const char *restrict szKey, void *restrict pData)
{
	if( !d )
		return false;
	
	if( d->size == 0 ) {
		d->size = 8;
		d->table = calloc(d->size, sizeof(kvnode_t));
		
		if( !d->table ) {
			printf("**** Memory Allocation Error **** dict_insert::d->table is NULL\n");
			d->size = 0;
			return false;
		}
	}
	else if( d->count >= d->size ) {
		dict_rehash(d);
		//printf("**** Rehashed Dictionary ****\n");
		//printf("**** Dictionary Size is now %llu ****\n", d->size);
	}
	else if( dict_has_key(d, szKey) ) {
		printf("dict_insert::d already has entry!\n");
		return false;
	}
	
	kvnode_t *node = malloc( sizeof(kvnode_t) );
	if( !node ) {
		printf("**** Memory Allocation Error **** dict_insert::node is NULL\n");
		return false;
	}
	node->strKey = szKey;
	node->pData = pData;
	
	uint32_t hash = gethash32(szKey) % d->size;
	node->pNext = d->table[hash];
	d->table[hash] = node;
	++d->count;
	return true;
}

void *dict_find(const dict *restrict d, const char *restrict szKey)
{
	if( !d )
		return NULL;
	else if( !d->table )
		return NULL;
	/*
	 * if dictionary pointer is const, you only
	 * need to use one traversing kvnode_t
	 * pointer without worrying of a segfault
	*/
	kvnode_t *kv;
	unsigned hash = gethash32(szKey) % d->size;
	for( kv = d->table[hash] ; kv ; kv = kv->pNext )
		if( !strcmp(kv->strKey, szKey) )
			return kv->pData;
	return NULL;
}

void dict_delete(dict *restrict d, const char *restrict szKey)
{
	if( !d )
		return;
	
	if( !dict_has_key(d, szKey) )
		return;
	
	uint32_t hash = gethash32(szKey) % d->size;
	kvnode_t
		*kv = NULL,
		*next = NULL
		;
	for( kv = d->table[hash] ; kv ; kv = next ) {
		next = kv->pNext;
		if( !strcmp(kv->strKey, szKey) ) {
			d->table[hash] = kv->pNext;
			free(kv), kv = NULL;
			d->count--;
		}
	}
}

bool dict_has_key(const dict *restrict d, const char *restrict szKey)
{
	if( !d || !d->table )
		return false;
	
	kvnode_t *prev;
	uint32_t hash = gethash32(szKey) % d->size;
	for( prev = d->table[hash] ; prev ; prev = prev->pNext )
		if( !strcmp(prev->strKey, szKey) )
			return true;
	
	return false;
}

uint32_t dict_len(const dict *d)
{
	if( !d )
		return 0L;
	return d->count;
}

// Rehashing increases dictionary size by a factor of 2
void dict_rehash(dict *d)
{
	if( !d )
		return;
	
	uint32_t old_size = d->size;
	d->size <<= 1;
	d->count = 0;
	
	kvnode_t **curr, **temp;
	temp = calloc(d->size, sizeof(kvnode_t));
	if( !temp ) {
		printf("**** Memory Allocation Error **** dict_insert::temp is NULL\n");
		d->size = 0;
		return;
	}
	
	curr = d->table;
	d->table = temp;
	
	kvnode_t
		*kv = NULL,
		*next = NULL
		;
	for( uint32_t i=0 ; i<old_size ; ++i ) {
		if( !curr[i] )
			continue;
		for( kv = curr[i] ; kv ; kv = next ) {
			next = kv->pNext;
			dict_insert(d, kv->strKey, kv->pData);
			// free the inner nodes since they'll be re-hashed
			kv->strKey=NULL, kv->pData=NULL;
			free(kv), kv = NULL;
			//printf("**** Rehashed Entry ****\n");
		}
	}
	if( curr )
		free(curr);
	curr = NULL;
}

const char *dict_get_key(const dict *restrict d, const char *restrict szKey)
{
	if( !d || !d->table )
		return NULL;
	
	kvnode_t *prev;
	uint32_t hash = gethash32(szKey) % d->size;
	for( prev = d->table[hash] ; prev ; prev = prev->pNext )
		if( !strcmp(prev->strKey, szKey) )
			return prev->strKey;
	
	return NULL;
}



/*
bool dict_insert_int(dict *restrict d, const uint64_t key, void *restrict pData)
{
	if( !d )
		return false;
	
	if( d->size == 0 ) {
		d->size = 8;
		d->table = calloc(d->size, sizeof(kvnode_t));
		
		if( !d->table ) {
			printf("**** Memory Allocation Error **** dict_insert_int::d->table is NULL\n");
			d->size = 0;
			return false;
		}
	}
	else if( d->count >= d->size ) {
		dict_rehash(d);
		//printf("**** Rehashed Dictionary ****\n");
		//printf("**** Dictionary Size is now %llu ****\n", d->size);
	}
	else if( dict_has_key_int(d, key) ) {
		printf("dict_insert_int::d already has entry!\n");
		return false;
	}
	
	kvnode_t *node = malloc( sizeof(kvnode_t) );
	if( !node ) {
		printf("**** Memory Allocation Error **** dict_insert_int::node is NULL\n");
		return false;
	}
	node->i64Key = key;
	node->pData = pData;
	
	uint64_t hash = int64hash(key) % d->size;
	node->pNext = d->table[hash];
	d->table[hash] = node;
	++d->count;
	return true;
}

void *dict_find_int(const dict *d, const uint64_t key)
{
	if( !d )
		return NULL;
	else if( !d->table )
		return NULL;
	
	kvnode_t *kv;
	uint64_t hash = int64hash(key) % d->size;
	for( kv = d->table[hash] ; kv ; kv = kv->pNext )
		if( kv->i64Key==key )
			return kv->pData;
	
	return NULL;
}

void dict_delete_int(dict *d, const uint64_t key)
{
	if( !d )
		return;
	
	if( !dict_has_key_int(d, key) )
		return;
	
	uint64_t hash = int64hash(key) % d->size;
	kvnode_t
		*kv = NULL,
		*next = NULL
	;
	for( kv = d->table[hash] ; kv ; kv = next ) {
		next = kv->pNext;
		if( kv->i64Key==key ) {
			d->table[hash] = kv->pNext;
			free(kv), kv = NULL;
			d->count--;
		}
	}
}
bool dict_has_key_int(const dict *d, const uint64_t key)
{
	if( !d )
		return false;
	
	kvnode_t *prev;
	uint64_t hash = int64hash(key) % d->size;
	for( prev = d->table[hash] ; prev ; prev = prev->pNext )
		if( prev->i64Key==key )
			return true;
	
	return false;
}

void dict_rehash_int(dict *d)
{
	if( !d )
		return;
	
	uint32_t old_size = d->size;
	d->size <<= 1;
	d->count = 0;
	
	kvnode_t **curr, **temp;
	temp = calloc(d->size, sizeof(kvnode_t));
	if( !temp ) {
		printf("**** Memory Allocation Error **** dict_insert::temp is NULL\n");
		d->size = 0;
		return;
	}
	
	curr = d->table;
	d->table = temp;
	
	kvnode_t
		*kv = NULL,
		*next = NULL
	;
	for( uint32_t i=0 ; i<old_size ; ++i ) {
		if( !curr[i] )
			continue;
		for( kv = curr[i] ; kv ; kv = next ) {
			next = kv->pNext;
			dict_insert_int(d, kv->i64Key, kv->pData);
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





