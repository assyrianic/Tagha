#ifndef HARBOL_BISTACK_INCLUDED
#	define HARBOL_BISTACK_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "../../harbol_common_defines.h"
#include "../../harbol_common_includes.h"


/// Double-Ended Stack Allocator
struct HarbolBiStack {
    uintptr_t mem, front, back;
    size_t    size;
};


HARBOL_EXPORT struct HarbolBiStack harbol_bistack_make(size_t len, bool *res);
HARBOL_EXPORT NO_NULL bool harbol_bistack_init(struct HarbolBiStack *bistack, size_t len);
HARBOL_EXPORT NO_NULL struct HarbolBiStack harbol_bistack_make_from_buffer(void *buf, size_t len);
HARBOL_EXPORT NO_NULL void harbol_bistack_clear(struct HarbolBiStack *bistack);

HARBOL_EXPORT NO_NULL void *harbol_bistack_alloc_front(struct HarbolBiStack *bistack, size_t size);
HARBOL_EXPORT NO_NULL void *harbol_bistack_alloc_back(struct HarbolBiStack *bistack, size_t size);

HARBOL_EXPORT NO_NULL void harbol_bistack_reset_front(struct HarbolBiStack *bistack);
HARBOL_EXPORT NO_NULL void harbol_bistack_reset_back(struct HarbolBiStack *bistack);
HARBOL_EXPORT NO_NULL void harbol_bistack_reset_all(struct HarbolBiStack *bistack);

HARBOL_EXPORT intptr_t harbol_bistack_get_margins(struct HarbolBiStack bistack);

/// Warning: Resizing WILL reset the memory margins.
/// So don't resize unless you're absolutely done using the data before resizing.
/// Note: This will resize the bistack using a heap-allocated buffer.
HARBOL_EXPORT NO_NULL bool harbol_bistack_resize(struct HarbolBiStack *bistack, size_t new_size);
/********************************************************************/


#ifdef __cplusplus
}
#endif

#endif /** HARBOL_BISTACK_INCLUDED */