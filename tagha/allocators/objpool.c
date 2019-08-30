#include "objpool.h"

#ifdef OS_WINDOWS
#	define HARBOL_LIB
#endif

typedef union {
	uint8_t *const byte;
	size_t *const index;
} ObjInfo_t;


HARBOL_EXPORT struct HarbolObjPool harbol_objpool_create(const size_t objsize, const size_t len)
{
	struct HarbolObjPool objpool = EMPTY_HARBOL_OBJPOOL;
	if( len==0UL || objsize==0UL )
		return objpool;
	else {
		objpool.objsize = harbol_align_size(objsize, sizeof(size_t));
		objpool.size = objpool.free_blocks = len;
		objpool.mem = calloc(objpool.size, objpool.objsize);
		if( objpool.mem==NULL ) {
			objpool.size = 0UL;
			return objpool;
		} else {
			for( uindex_t i=0; i<objpool.free_blocks; i++ ) {
				ObjInfo_t block = { .byte = &objpool.mem[i * objpool.objsize] };
				*block.index = i + 1;
			}
			objpool.next = objpool.mem;
			return objpool;
		}
	}
}

HARBOL_EXPORT struct HarbolObjPool harbol_objpool_from_buffer(void *const buf, const size_t objsize, const size_t len)
{
	struct HarbolObjPool objpool = EMPTY_HARBOL_OBJPOOL;
	
	// If the object index isn't large enough to align to a size_t, then we can't use it.
	if( len==0UL || objsize<sizeof(size_t) || objsize * len != harbol_align_size(objsize, sizeof(size_t)) * len )
		return objpool;
	else {
		objpool.objsize = harbol_align_size(objsize, sizeof(size_t));
		objpool.size = objpool.free_blocks = len;
		objpool.mem = buf;
		for( uindex_t i=0; i<objpool.free_blocks; i++ ) {
			ObjInfo_t block = { .byte = &objpool.mem[i * objpool.objsize] };
			*block.index = i + 1;
		}
		objpool.next = objpool.mem;
		return objpool;
	}
}

HARBOL_EXPORT bool harbol_objpool_clear(struct HarbolObjPool *const objpool)
{
	if( objpool->mem==NULL )
		return false;
	else {
		free(objpool->mem);
		*objpool = (struct HarbolObjPool)EMPTY_HARBOL_OBJPOOL;
		return true;
	}
}

HARBOL_EXPORT void *harbol_objpool_alloc(struct HarbolObjPool *const objpool)
{
	if( objpool->free_blocks>0UL ) {
		// for first allocation, head points to the very first index.
		// next = &pool[0];
		// ret = next == ret = &pool[0];
		ObjInfo_t ret = { .byte = objpool->next };
		objpool->free_blocks--;
		
		// after allocating, we set head to the address of the index that *next holds.
		// next = &pool[*next * pool.objsize];
		objpool->next = ( objpool->free_blocks != 0UL ) ? objpool->mem + (*ret.index * objpool->objsize) : NULL;
		memset(ret.byte, 0, objpool->objsize);
		return ret.byte;
	}
	else return NULL;
}

HARBOL_EXPORT bool harbol_objpool_free(struct HarbolObjPool *const restrict objpool, void *ptr)
{
	ObjInfo_t p = { .byte = ptr };
	if( ptr==NULL || p.byte < objpool->mem || p.byte > objpool->mem + objpool->size * objpool->objsize )
		return false;
	else {
		// when we free our pointer, we recycle the pointer space to store the previous index
		// and then we push it as our new head.
		
		// *p = index of next in relation to the buffer;
		// next = p;
		*p.index = ( objpool->next != NULL ) ? (objpool->next - objpool->mem) / objpool->objsize : objpool->size;
		objpool->next = p.byte;
		++objpool->free_blocks;
		return true;
	}
}

HARBOL_EXPORT bool harbol_objpool_cleanup(struct HarbolObjPool *const restrict objpool, void **const restrict ptrref)
{
	if( *ptrref==NULL )
		return false;
	else {
		const bool free_result = harbol_objpool_free(objpool, *ptrref);
		*ptrref = NULL;
		return free_result;
	}
}
