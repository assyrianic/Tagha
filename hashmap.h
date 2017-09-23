
#ifndef __DICT_H__INCLUDED
	#define __DICT_H__INCLUDED

#include <stdbool.h>
#include <stdint.h>

typedef struct _kvnode {
	union {
		const char	*strKey;
		uint64_t	i64Key;
	};
	void			*pData;
	struct _kvnode	*pNext;
} kvnode_t;

typedef struct _dict {
	kvnode_t		**table;
	uint32_t		size, count;
} dict;

void		dict_init		(dict *);
void		dict_free		(dict *);
unsigned	dict_len		(const dict *);

void		dict_rehash		(dict *);
bool		dict_insert		(dict *, const char *, void *);
void		*dict_find		(const dict *, const char *);
void		dict_delete		(dict *, const char *);
bool		dict_has_key	(const dict *, const char *);
const char	*dict_get_key	(const dict *, const char *);
/*

void		dict_rehash_int	(dict *);
bool		dict_insert_int	(dict *, const uint64_t, void *);
void		*dict_find_int	(const dict *, const uint64_t);
void		dict_delete_int	(dict *, const uint64_t);
bool		dict_has_key_int(const dict *, const uint64_t);
*/
uint64_t gethash64(const char *szKey);
uint32_t gethash32(const char *szKey);
uint64_t int64hash(uint64_t x);
uint32_t int32hash(uint32_t x);


#endif	// __DICT_H__INCLUDED
