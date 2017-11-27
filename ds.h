
#ifndef DS_H_INCLUDED
	#define DS_H_INCLUDED

#include <stdbool.h>
#include <stdint.h>

struct vec;
struct kvnode;
struct hashmap;

typedef struct vec		Vec_t, vec_t;
typedef struct kvnode	kvnode_t, mapnode_t;
typedef struct hashmap	Map_t, map_t;


struct vec {
	void		**data;
	uint32_t	size, count;
};

void		vector_init		(Vec_t *);
uint32_t	vector_count	(const Vec_t *);
void		vector_add		(Vec_t *, void *);
void		vector_set		(Vec_t *, const uint32_t, void *);
void		*vector_get		(const Vec_t *, const uint32_t);
void		vector_delete	(Vec_t *, const uint32_t);
void		vector_free		(Vec_t *);



struct kvnode {
	//union {
	const char	*strKey;
	//uint64_t	i64Key;
	//};
	// large enough to store a pointer for 32-bit and 64-bit
	uint64_t		pData;
	struct kvnode	*pNext;
};

struct hashmap {
	kvnode_t		**table;
	uint64_t		size, count;
};

void		map_init		(Map_t *);
void		map_free		(Map_t *);
uint64_t	map_len			(const Map_t *);

void		map_rehash		(Map_t *);
bool		map_insert		(Map_t *, const char *, const uint64_t);
uint64_t	map_find		(const Map_t *, const char *);
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


#endif /* DS_H_INCLUDED */
