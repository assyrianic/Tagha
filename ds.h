
#ifndef VECTOR_H_INCLUDED
	#define VECTOR_H_INCLUDED

#include <stdbool.h>
#include <stdint.h>

typedef struct _vec {
	void		**data;
	uint32_t	size, count;
} Vec_t;

void		vector_init		(Vec_t *);
uint32_t	vector_count	(const Vec_t *);
void		vector_add		(Vec_t *, void *);
void		vector_set		(Vec_t *, const uint32_t, void *);
void		*vector_get		(const Vec_t *, const uint32_t);
void		vector_delete	(Vec_t *, const uint32_t);
void		vector_free		(Vec_t *);



typedef struct _kvnode {
	union {
		const char	*strKey;
		uint64_t	i64Key;
	};
	void			*pData;
	struct _kvnode	*pNext;
} kvnode_t;

typedef struct _map {
	kvnode_t		**table;
	uint64_t		size, count;
} Map_t;

void		map_init		(Map_t *);
void		map_free		(Map_t *);
unsigned	map_len			(const Map_t *);

void		map_rehash		(Map_t *);
bool		map_insert		(Map_t *, const char *, void *);
void		*map_find		(const Map_t *, const char *);
void		map_delete		(Map_t *, const char *);
bool		map_has_key		(const Map_t *, const char *);
const char	*map_get_key	(const Map_t *, const char *);

/*
void		map_rehash_int	(Map_t *);
bool		map_insert_int	(Map_t *, const uint64_t, void *);
void		*map_find_int	(const Map_t *, const uint64_t);
void		map_delete_int	(Map_t *, const uint64_t);
bool		map_has_key_int(const Map_t *, const uint64_t);
*/
uint64_t gethash64(const char *szKey);
uint32_t gethash32(const char *szKey);
uint64_t int64hash(uint64_t x);
uint32_t int32hash(uint32_t x);


#endif /* VECTOR_H_INCLUDED */
