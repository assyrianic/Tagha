
#ifndef __DICT_H__INCLUDED
	#define __DICT_H__INCLUDED

#include <stdbool.h>

typedef struct _kvnode {
	union {
		const char	*strKey;
		unsigned	uiKey;
	};
	void	*pData;
	struct _kvnode	*pNext;
} kvnode_t;

typedef struct _dict {
	kvnode_t	**table;
	unsigned	size, count;
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
bool		dict_insert_int	(dict *, const unsigned, void *);
void		*dict_find_int	(const dict *, const unsigned);
void		dict_delete_int	(dict *, const unsigned);
bool		dict_has_key_int(const dict *, const unsigned);
*/

#endif	// __DICT_H__INCLUDED
