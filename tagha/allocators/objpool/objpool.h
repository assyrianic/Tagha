#ifndef HARBOL_OBJPOOL_INCLUDED
#	define HARBOL_OBJPOOL_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "../../harbol_common_defines.h"
#include "../../harbol_common_includes.h"


struct HarbolObjPool {
	uintptr_t
		mem,        /// Beginning of memory pool
		next        /// Num of next free block
	;
	size_t
		size,       /// Num of blocks.
		objsize,    /// size of each block
		free_blocks /// Num of remaining blocks
	;
};

HARBOL_EXPORT struct HarbolObjPool harbol_objpool_make(size_t objsize, size_t len, bool *res);
HARBOL_EXPORT NO_NULL bool harbol_objpool_init(struct HarbolObjPool *objpool, size_t objsize, size_t len);
HARBOL_EXPORT NO_NULL struct HarbolObjPool harbol_objpool_from_buffer(void *buf, size_t objsize, size_t len, bool *res);
HARBOL_EXPORT NO_NULL bool harbol_objpool_init_from_buffer(struct HarbolObjPool *objpool, void *buf, size_t objsize, size_t len);
HARBOL_EXPORT NO_NULL void harbol_objpool_clear(struct HarbolObjPool *objpool);

HARBOL_EXPORT NO_NULL void *harbol_objpool_alloc(struct HarbolObjPool *objpool);
HARBOL_EXPORT NEVER_NULL(1) void harbol_objpool_free(struct HarbolObjPool *objpool, void *ptr);
HARBOL_EXPORT NO_NULL void harbol_objpool_cleanup(struct HarbolObjPool *objpool, void **ptrref);
/********************************************************************/


#ifdef __cplusplus
}
#endif

#endif /** HARBOL_OBJPOOL_INCLUDED */