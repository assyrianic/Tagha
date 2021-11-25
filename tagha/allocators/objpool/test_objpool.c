#include <assert.h>
#include <stdalign.h>
#include <time.h>
#include "objpool.h"

void test_harbol_objpool(FILE *debug_stream);

#ifdef HARBOL_USE_MEMPOOL
struct HarbolMemPool *g_pool;
#endif

union Value {
	int64_t int64;
};

int main(void)
{
	FILE *debug_stream = fopen("harbol_objpool_output.txt", "w");
	if( debug_stream==NULL )
		return -1;
	
#ifdef HARBOL_USE_MEMPOOL
	struct HarbolMemPool m = harbol_mempool_create(1000000);
	g_pool = &m;
#endif
	test_harbol_objpool(debug_stream);
	
	fclose(debug_stream); debug_stream=NULL;
#ifdef HARBOL_USE_MEMPOOL
	harbol_mempool_clear(g_pool);
#endif
}


void test_harbol_objpool(FILE *const debug_stream)
{
	/// Test allocation and initializations
	fputs("objpool :: test allocation/initialization.\n", debug_stream);
	
	struct HarbolObjPool i = harbol_objpool_make(sizeof(union Value), 5, &( bool ){false});
	fprintf(debug_stream, "remaining object pool mem: '%zu'\n", i.free_blocks);
	
	fputs("\nobjpool :: test allocing values.\n", debug_stream);
	union Value *valtable[5] = {NULL};
	for( size_t n=0; n<(1[&valtable] - valtable); n++ ) {
		valtable[n] = harbol_objpool_alloc(&i);
		valtable[n]->int64 = n + 1;
		fprintf(debug_stream, "valtable[%zu]: %" PRIi64 "\n", n, valtable[n]->int64);
		fprintf(debug_stream, "post-allocation remaining object pool mem: '%zu'\n", i.free_blocks);
	}
	
	fputs("\nobjpool :: test freeing values.\n", debug_stream);
	for( size_t n=0; n<(1[&valtable] - valtable); n++ ) {
		harbol_objpool_free(&i, valtable[n]);
		fprintf(debug_stream, "post-free remaining object pool mem: '%zu'\n", i.free_blocks);
	}
	for( size_t n=0; n<(1[&valtable] - valtable); n++ ) {
		valtable[n] = harbol_objpool_alloc(&i);
		valtable[n]->int64 = n + 1;
		fprintf(debug_stream, "valtable[%zu]: %" PRIi64 "\n", n, valtable[n]->int64);
		fprintf(debug_stream, "post-allocation remaining object pool mem: '%zu'\n", i.free_blocks);
	}
	
	/// free data
	fputs("\nobjpool :: test destruction.\n", debug_stream);
	harbol_objpool_clear(&i);
	fprintf(debug_stream, "i's heap is null? '%s'\n", i.mem != NIL ? "no" : "yes");
	fprintf(debug_stream, "i's next is null? '%s'\n", i.next != NIL ? "no" : "yes");
}