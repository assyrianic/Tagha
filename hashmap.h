
#ifndef __DICT_H__INCLUDED
	#define __DICT_H__INCLUDED

#include <stdbool.h>

typedef struct _kvnode {
	const char	*strKey;
	void	*pData;
	struct _kvnode	*pNext;
} kvnode_t;

typedef struct _dict {
	kvnode_t	**table;
	unsigned	size, count;
} dict;

void			dict_init		(dict *);
void			dict_free		(dict *, const bool);
void			dict_rehash		(dict *);
bool			dict_insert		(dict *, const char *, void *);
void			*dict_find		(const dict *, const char *);
void			dict_delete		(dict *, const char *);
bool			dict_has_key		(const dict *, const char *);
unsigned		dict_len		(const dict *);

#endif	// __DICT_H__INCLUDED
