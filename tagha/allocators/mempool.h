#ifndef HARBOL_MEMPOOL_INCLUDED
#	define HARBOL_MEMPOOL_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "../harbol_common_defines.h"
#include "../harbol_common_includes.h"


struct HarbolMemNode {
	size_t size;
	struct HarbolMemNode *next, *prev;
};

struct HarbolMemPool {
	struct {
		struct HarbolMemNode *head, *tail;
		size_t len, max_nodes;
		bool auto_defrag : 1;
	} freelist;
	
	struct {
		uint8_t *mem, *base;
		size_t size;
	} stack;
	
	// hold 32 byte, 64 byte, and 128 byte sizes into a bucket.
#	define HARBOL_BUCKET_SIZE    8
#	define HARBOL_BUCKET_BITS    3
	struct HarbolMemNode *buckets[HARBOL_BUCKET_SIZE];
};

#define EMPTY_HARBOL_MEMPOOL    { {NULL,NULL,0,0,false}, {NULL,NULL,0}, {NULL} }


HARBOL_EXPORT struct HarbolMemPool harbol_mempool_create(size_t bytes);
HARBOL_EXPORT NO_NULL struct HarbolMemPool harbol_mempool_from_buffer(void *buf, size_t bytes);
HARBOL_EXPORT NO_NULL bool harbol_mempool_clear(struct HarbolMemPool *mempool);

HARBOL_EXPORT NO_NULL void *harbol_mempool_alloc(struct HarbolMemPool *mempool, size_t bytes);
HARBOL_EXPORT NEVER_NULL(1) void *harbol_mempool_realloc(struct HarbolMemPool *mempool, void *ptr, size_t bytes);
HARBOL_EXPORT NEVER_NULL(1) bool harbol_mempool_free(struct HarbolMemPool *mempool, void *ptr);
HARBOL_EXPORT NO_NULL bool harbol_mempool_cleanup(struct HarbolMemPool *mempool, void **ptrref);

HARBOL_EXPORT NO_NULL size_t harbol_mempool_mem_remaining(const struct HarbolMemPool *mempool);
HARBOL_EXPORT NO_NULL bool harbol_mempool_defrag(struct HarbolMemPool *mempool);
HARBOL_EXPORT NO_NULL void harbol_mempool_set_max_nodes(struct HarbolMemPool *mempool, size_t nodes);
HARBOL_EXPORT NO_NULL void harbol_mempool_toggle_auto_defrag(struct HarbolMemPool *mempool);
/********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* HARBOL_MEMPOOL_INCLUDED */
