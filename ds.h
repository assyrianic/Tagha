
#ifndef DS_H_INCLUDED
	#define DS_H_INCLUDED

#include <stdbool.h>
#include <stdint.h>

/*
 * type generic Vector
 */
typedef struct Vector {
	uint32_t	size, count;
	uint64_t	*data;
} Vector;

struct Vector *Vector_New(void);
void		Vector_Init		(struct Vector *);
uint32_t	Vector_Len		(const struct Vector *);
void		Vector_Insert	(struct Vector *, const uint64_t);
void		Vector_Set		(struct Vector *, const uint32_t, const uint64_t);
uint64_t	Vector_Get		(const struct Vector *, const uint32_t);
void		Vector_Delete	(struct Vector *, const uint32_t);
void		Vector_Free		(struct Vector *);


/*
 * type generic Hashmap (uses 64-bit int for pointers to accomodate 32-bit and 64-bit)
 */
typedef struct KeyNode {
	uint64_t		pData;
	const char		*strKey;
	struct KeyNode	*pNext;
} KeyNode;

typedef struct Hashmap {
	struct KeyNode	**table;
	uint64_t		size, count;
} Hashmap;

struct Hashmap *Map_New(void);
void		Map_Init		(struct Hashmap *);
void		Map_Free		(struct Hashmap *);
uint64_t	Map_Len			(const struct Hashmap *);

void		Map_Rehash		(struct Hashmap *);
bool		Map_Insert		(struct Hashmap *, const char *, const uint64_t);
uint64_t	Map_Get			(const struct Hashmap *, const char *);
void		Map_Delete		(struct Hashmap *, const char *);
bool		Map_HasKey		(const struct Hashmap *, const char *);
const char	*Map_GetKey		(const struct Hashmap *, const char *);

/*
void		Map_Rehash_int	(struct Hashmap *);
bool		Map_Insert_int	(struct Hashmap *, const uint64_t, void *);
void		*Map_Get_int	(const struct Hashmap *, const uint64_t);
void		Map_Delete_int	(struct Hashmap *, const uint64_t);
bool		Map_HasKey_int	(const struct Hashmap *, const uint64_t);
*/
uint64_t gethash64(const char *szKey);
uint32_t gethash32(const char *szKey);
uint64_t int64hash(uint64_t x);
uint32_t int32hash(uint32_t x);

#endif /* DS_H_INCLUDED */
