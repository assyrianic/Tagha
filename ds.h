
#ifndef DS_H_INCLUDED
	#define DS_H_INCLUDED

#include <stdbool.h>
#include <stdint.h>


typedef struct vector {
	void		**data;
	uint32_t	size, count;
} vector;

void		vector_init		(struct vector *);
uint32_t	vector_count	(const struct vector *);
void		vector_add		(struct vector *, void *);
void		vector_set		(struct vector *, const uint32_t, void *);
void		*vector_get		(const struct vector *, const uint32_t);
void		vector_delete	(struct vector *, const uint32_t);
void		vector_free		(struct vector *);



typedef struct kvnode {
	uint64_t		pData;
	const char		*strKey;
	struct kvnode	*pNext;
} kvnode;

typedef struct hashmap {
	struct kvnode	**table;
	uint64_t		size, count;
} hashmap;

void		map_init		(struct hashmap *);
void		map_free		(struct hashmap *);
uint64_t	map_len			(const struct hashmap *);

void		map_rehash		(struct hashmap *);
bool		map_insert		(struct hashmap *, const char *, const uint64_t);
uint64_t	map_find		(const struct hashmap *, const char *);
void		map_delete		(struct hashmap *, const char *);
bool		map_has_key		(const struct hashmap *, const char *);
const char	*map_get_key	(const struct hashmap *, const char *);

/*
void		map_rehash_int	(struct hashmap *);
bool		map_insert_int	(struct hashmap *, const uint64_t, void *);
void		*map_find_int	(const struct hashmap *, const uint64_t);
void		map_delete_int	(struct hashmap *, const uint64_t);
bool		map_has_key_int	(const struct hashmap *, const uint64_t);
*/
uint64_t gethash64(const char *szKey);
uint32_t gethash32(const char *szKey);
uint64_t int64hash(uint64_t x);
uint32_t int32hash(uint32_t x);


#endif /* DS_H_INCLUDED */
