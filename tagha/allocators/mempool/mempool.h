#ifndef HARBOL_MEMPOOL_INCLUDED
#	define HARBOL_MEMPOOL_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "../../harbol_common_defines.h"
#include "../../harbol_common_includes.h"
#include "../region/region.h"


struct HarbolMemNode {
	size_t                size;
	struct HarbolMemNode *next, *prev;
};

HARBOL_EXPORT NO_NULL NONNULL_RET struct HarbolMemNode *harbol_memnode_split(struct HarbolMemNode *const node, const size_t bytes);
HARBOL_EXPORT NO_NULL void harbol_memnode_replace(struct HarbolMemNode *old, struct HarbolMemNode *replace);


struct HarbolFreeList {
	struct HarbolMemNode *head, *tail;
	size_t                len;
};

struct HarbolMemPool;

HARBOL_EXPORT NO_NULL void harbol_freelist_insert(struct HarbolMemPool *mempool, struct HarbolFreeList *list, struct HarbolMemNode *node, bool is_bucket);
HARBOL_EXPORT NO_NULL NONNULL_RET struct HarbolMemNode *harbol_freelist_remove(struct HarbolFreeList *list, struct HarbolMemNode *node);
HARBOL_EXPORT NO_NULL void harbol_freelist_insert_before(struct HarbolFreeList *list, struct HarbolMemNode *curr, struct HarbolMemNode *insert);
HARBOL_EXPORT NO_NULL struct HarbolMemNode *harbol_freelist_find(struct HarbolFreeList *list, size_t size);


enum {
	HARBOL_BUCKET_SIZE = 8,
	HARBOL_BUCKET_BITS = 3,
	MEM_SPLIT_THRESHOLD = 32
};

struct HarbolMemPool {
	struct HarbolFreeList
		large, /// large free list.
		buckets[HARBOL_BUCKET_SIZE] /// bucket free list for smaller allocations.
	;
	struct HarbolRegion stack;
};


HARBOL_EXPORT NO_NULL bool harbol_mempool_init(struct HarbolMemPool *mempool, size_t size);
HARBOL_EXPORT struct HarbolMemPool harbol_mempool_make(size_t bytes, bool *res);

HARBOL_EXPORT NO_NULL bool harbol_mempool_init_from_buffer(struct HarbolMemPool *mempool, void *buf, size_t size);
HARBOL_EXPORT NO_NULL struct HarbolMemPool harbol_mempool_make_from_buffer(void *buf, size_t bytes, bool *res);

HARBOL_EXPORT NO_NULL void harbol_mempool_clear(struct HarbolMemPool *mempool);

HARBOL_EXPORT NO_NULL void *harbol_mempool_alloc(struct HarbolMemPool *mempool, size_t bytes);
HARBOL_EXPORT NEVER_NULL(1) void *harbol_mempool_realloc(struct HarbolMemPool *mempool, void *ptr, size_t bytes);
HARBOL_EXPORT NEVER_NULL(1) bool harbol_mempool_free(struct HarbolMemPool *mempool, void *ptr);
HARBOL_EXPORT NO_NULL bool harbol_mempool_cleanup(struct HarbolMemPool *mempool, void **ptrref);

HARBOL_EXPORT NO_NULL size_t harbol_mempool_mem_remaining(const struct HarbolMemPool *mempool);
/********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /** HARBOL_MEMPOOL_INCLUDED */
