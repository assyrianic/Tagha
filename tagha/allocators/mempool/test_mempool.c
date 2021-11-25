#include <assert.h>
#include <stdalign.h>
#include <time.h>
#include "mempool.h"

void test_harbol_mempool(FILE *debug_stream);

#ifdef HARBOL_USE_MEMPOOL
struct HarbolMemPool *g_pool;
#endif

union Value {
	int64_t int64;
};

int main(void)
{
	FILE *debug_stream = fopen("harbol_mempool_output.txt", "w");
	if( debug_stream==NULL )
		return -1;
	
#ifdef HARBOL_USE_MEMPOOL
	struct HarbolMemPool m = harbol_mempool_create(1000000);
	g_pool = &m;
#endif
	test_harbol_mempool(debug_stream);
	
	fclose(debug_stream); debug_stream=NULL;
#ifdef HARBOL_USE_MEMPOOL
	harbol_mempool_clear(g_pool);
#endif
}

static void _print_mempool_nodes(const struct HarbolMemPool *const mempool, FILE *const debug_stream)
{
	fputs("\nmempool :: printing mempool free bucket.\n", debug_stream);
	for( size_t i=0; i<HARBOL_BUCKET_SIZE; i++ ) {
		fprintf(debug_stream, "mempool bucket :: %zu | freenodes: %zu.\n", i, mempool->buckets[i].len);
		for( struct HarbolMemNode *n=mempool->buckets[i].head; n != NULL; n = n->next )
			fprintf(debug_stream, "mempool bucket[%zu] node :: n (%" PRIuPTR ") size == %zu.\n", i, ( uintptr_t )n, n->size);
	}
	
	fputs("\nmempool :: printing mempool free list.\n", debug_stream);
	for( struct HarbolMemNode *n = mempool->large.head; n != NULL; n = n->next )
		fprintf(debug_stream, "mempool list node :: n (%" PRIuPTR ") size == %zu.\n", ( uintptr_t )n, n->size);
	
	fprintf(debug_stream, "mempool memory remaining :: %zu | freenodes: %zu.\n", harbol_mempool_mem_remaining(mempool), mempool->large.len);
}


void test_harbol_mempool(FILE *const debug_stream)
{
	/// Test allocation and initializations
	fputs("mempool :: test allocation/initialization.\n", debug_stream);
	
	struct HarbolMemPool i = harbol_mempool_make(1000, &( bool ){false});
	fprintf(debug_stream, "remaining heap mem: '%zu'\n", harbol_mempool_mem_remaining(&i));
	
	const clock_t start = clock();
	/// test giving memory
	fputs("mempool :: test giving memory.\n", debug_stream);
	fputs("\nmempool :: allocating int ptr.\n", debug_stream);
	int *p = harbol_mempool_alloc(&i, sizeof *p);
	fprintf(debug_stream, "p is null? '%s'\n", p ? "no" : "yes");
	if( p ) {
		*p = 500;
		fprintf(debug_stream, "p's value: %i\n", *p);
	}
	fprintf(debug_stream, "remaining heap mem: '%zu'\n", harbol_mempool_mem_remaining(&i));
	
	fputs("\nmempool :: allocating float ptr.\n", debug_stream);
	float *f = harbol_mempool_alloc(&i, sizeof *f);
	fprintf(debug_stream, "f is null? '%s'\n", f ? "no" : "yes");
	if( f ) {
		*f = 500.5f;
		fprintf(debug_stream, "f's value: %f\n", *f);
	}
	fprintf(debug_stream, "remaining heap mem: '%zu'\n", harbol_mempool_mem_remaining(&i));
	
	/// test releasing memory
	fputs("mempool :: test releasing memory.\n", debug_stream);
	harbol_mempool_free(&i, f), f=NULL;
	harbol_mempool_free(&i, p), p=NULL;
	
	/// test re-giving memory
	fputs("mempool :: test regiving memory.\n", debug_stream);
	p = harbol_mempool_alloc(&i, sizeof *p);
	fprintf(debug_stream, "p is null? '%s'\n", p ? "no" : "yes");
	if( p ) {
		*p = 532;
		fprintf(debug_stream, "p's value: %i\n", *p);
	}
	f = harbol_mempool_alloc(&i, sizeof *f);
	fprintf(debug_stream, "f is null? '%s'\n", f ? "no" : "yes");
	if( f ) {
		*f = 466.5f;
		fprintf(debug_stream, "f's value: %f\n", *f);
	}
	fprintf(debug_stream, "remaining heap mem: '%zu'\n", harbol_mempool_mem_remaining(&i));
	harbol_mempool_free(&i, p), p=NULL; /// release memory that's from different region.
	harbol_mempool_free(&i, f), f=NULL;
	fprintf(debug_stream, "remaining heap mem: '%zu'\n", harbol_mempool_mem_remaining(&i));
	
	/// test giving array memory
	fputs("\nmempool :: test giving array memory.\n", debug_stream);
	const size_t arrsize = 100;
	p = harbol_mempool_alloc(&i, sizeof *p * arrsize);
	fprintf(debug_stream, "p is null? '%s'\n", p ? "no" : "yes");
	if( p != NULL ) {
		for( size_t i=0; i<arrsize; i++ ) {
			p[i] = i+1;
			fprintf(debug_stream, "p[%zu] value: %i\n", i, p[i]);
		}
	}
	fprintf(debug_stream, "remaining heap mem: '%zu'\n", harbol_mempool_mem_remaining(&i));
	f = harbol_mempool_alloc(&i, sizeof *f * arrsize);
	fprintf(debug_stream, "f is null? '%s'\n", f ? "no" : "yes");
	if( f != NULL ) {
		for( size_t i=0; i<arrsize; i++ ) {
			f[i] = i+1.15f;
			fprintf(debug_stream, "f[%zu] value: %f\n", i, f[i]);
		}
	}
	fprintf(debug_stream, "remaining heap mem: '%zu'\n", harbol_mempool_mem_remaining(&i));
	harbol_mempool_free(&i, p), p=NULL;
	harbol_mempool_free(&i, f), f=NULL;
	fprintf(debug_stream, "remaining heap mem: '%zu'\n", harbol_mempool_mem_remaining(&i));
	_print_mempool_nodes(&i, debug_stream);
	
	/// test using heap to make a unilinked list!
	fputs("\nmempool :: test using mempool for unilist.\n", debug_stream);
	struct LinkList {
		struct UniNode {
			uint8_t        *data;
			struct UniNode *next;
		}      *head, *tail;
		size_t len;
	} *list  = harbol_mempool_alloc(&i, sizeof *list);
	assert( list );
	struct HarbolMemNode *b = ( struct HarbolMemNode* )(( uint8_t* )list - sizeof *b);
	fprintf(debug_stream, "mempool :: list (%" PRIuPTR ") alloc node size: %zu, sizeof *list: %zu, b node: (%" PRIuPTR "), offset: %" PRIiPTR "; base mem: (%" PRIuPTR ")\n", ( uintptr_t )list, b->size, sizeof *list, ( uintptr_t )b, ( uintptr_t )b - i.stack.mem, i.stack.mem);
	fprintf(debug_stream, "remaining heap mem: '%zu'\n", harbol_mempool_mem_remaining(&i));
	
	struct UniNode *node1 = harbol_mempool_alloc(&i, sizeof *node1);
	assert( node1 );
	
	b = ( struct HarbolMemNode* )(( uint8_t* )node1 - sizeof *b);
	fprintf(debug_stream, "mempool :: node1 (%" PRIuPTR ") alloc node size: %zu, sizeof *node1: %zu, b node: (%" PRIuPTR "), offset: %" PRIiPTR "\n", ( uintptr_t )node1, b->size, sizeof *node1, ( uintptr_t )b, ( uintptr_t )b - i.stack.mem);
	
	fprintf(debug_stream, "remaining heap mem: '%zu'\n", harbol_mempool_mem_remaining(&i));
	node1->data = ( uint8_t* ) &( union Value ){.int64 = 1};
	{ list->head = list->tail = node1; }
	
	struct UniNode *node2 = harbol_mempool_alloc(&i, sizeof *node2);
	assert( node2 );
	b = ( struct HarbolMemNode* )(( uint8_t* )node2 - sizeof *b);
	fprintf(debug_stream, "mempool :: node2 (%" PRIuPTR ") alloc node size: %zu, sizeof *node2: %zu, b node: (%" PRIuPTR "), offset: %" PRIiPTR "\n", ( uintptr_t )node2, b->size, sizeof *node2, ( uintptr_t )b, ( uintptr_t )b - i.stack.mem);
	fprintf(debug_stream, "remaining heap mem: '%zu'\n", harbol_mempool_mem_remaining(&i));
	node2->data = ( uint8_t* ) &( union Value ){.int64 = 2};
	{ list->tail = node2; list->head->next = node2; }
	
	struct UniNode *node3 = harbol_mempool_alloc(&i, sizeof *node3);
	assert( node3 );
	b = ( struct HarbolMemNode* )(( uint8_t* )node3 - sizeof *b);
	fprintf(debug_stream, "mempool :: node3 (%" PRIuPTR ") alloc node size: %zu, sizeof *node3: %zu, b node: (%" PRIuPTR "), offset: %" PRIiPTR "\n", ( uintptr_t )node3, b->size, sizeof *node3, ( uintptr_t )b, ( uintptr_t )b - i.stack.mem);
	fprintf(debug_stream, "remaining heap mem: '%zu'\n", harbol_mempool_mem_remaining(&i));
	node3->data = ( uint8_t* ) &( union Value ){.int64 = 3};
	{ list->tail->next = node3; list->tail = node3; }
	
	struct UniNode *node4 = harbol_mempool_alloc(&i, sizeof *node4);
	assert( node4 );
	b = ( struct HarbolMemNode* )(( uint8_t* )node4 - sizeof *b);
	fprintf(debug_stream, "mempool :: node4 (%" PRIuPTR ") alloc node size: %zu, sizeof *node4: %zu, b node: (%" PRIuPTR "), offset: %" PRIiPTR "\n", ( uintptr_t )node4, b->size, sizeof *node4, ( uintptr_t )b, ( uintptr_t )b - i.stack.mem);
	fprintf(debug_stream, "remaining heap mem: '%zu'\n", harbol_mempool_mem_remaining(&i));
	node4->data = ( uint8_t* ) &( union Value ){.int64 = 4};
	{ list->tail->next = node4; list->tail = node4; }
	
	struct UniNode *node5 = harbol_mempool_alloc(&i, sizeof *node5);
	assert( node5 );
	b = ( struct HarbolMemNode* )(( uint8_t* )node5 - sizeof *b);
	fprintf(debug_stream, "mempool :: node5 (%" PRIuPTR ") alloc node size: %zu, sizeof *node5: %zu, b node: (%" PRIuPTR "), offset: %" PRIiPTR "\n", ( uintptr_t )node5, b->size, sizeof *node5, ( uintptr_t )b, ( uintptr_t )b - i.stack.mem);
	fprintf(debug_stream, "remaining heap mem: '%zu'\n", harbol_mempool_mem_remaining(&i));
	node5->data = ( uint8_t* ) &( union Value ){.int64 = 5};
	{ list->tail->next = node5; list->tail = node5; }
	
	struct UniNode *node6 = harbol_mempool_alloc(&i, sizeof *node6);
	assert( node6 );
	b = ( struct HarbolMemNode* )(( uint8_t* )node6 - sizeof *b);
	fprintf(debug_stream, "mempool :: node6 (%" PRIuPTR ") alloc node size: %zu, sizeof *node6: %zu, b node: (%" PRIuPTR "), offset: %" PRIiPTR "\n", ( uintptr_t )node6, b->size, sizeof *node6, ( uintptr_t )b, ( uintptr_t )b - i.stack.mem);
	fprintf(debug_stream, "remaining heap mem: '%zu'\n", harbol_mempool_mem_remaining(&i));
	node6->data = ( uint8_t* ) &( union Value ){.int64 = 6};
	{ list->tail->next = node6; list->tail = node6; }
	
	_print_mempool_nodes(&i, debug_stream);
	
	for( struct UniNode *n=list->head; n != NULL; n = n->next )
		fprintf(debug_stream, "uninode value : %" PRIi64 "\n", (( const union Value* )n->data)->int64);
	
	harbol_mempool_free(&i, list),  list =NULL;
	harbol_mempool_free(&i, node1), node1=NULL;
	harbol_mempool_free(&i, node2), node2=NULL;
	harbol_mempool_free(&i, node3), node3=NULL;
	harbol_mempool_free(&i, node4), node4=NULL;
	harbol_mempool_free(&i, node5), node5=NULL;
	harbol_mempool_free(&i, node6), node6=NULL;
	_print_mempool_nodes(&i, debug_stream);
	
	/// test "double freeing"
	fputs("\nmempool :: test double freeing.\n", debug_stream);
	p = harbol_mempool_alloc(&i, sizeof *p);
	fprintf(debug_stream, "p is null? '%s'\n", p ? "no" : "yes");
	if( p ) {
		*p = 500;
		fprintf(debug_stream, "p's value: %i\n", *p);
	}
	harbol_mempool_free(&i, p);
	harbol_mempool_free(&i, p);
	harbol_mempool_cleanup(&i, (void**)&p);
	
	fprintf(debug_stream, "\nmempool :: pool size == %zu.\n", harbol_mempool_mem_remaining(&i));
	_print_mempool_nodes(&i, debug_stream);
	
	float *hk = harbol_mempool_alloc(&i, sizeof *hk * 99);
	double *fg = harbol_mempool_alloc(&i, sizeof *fg * 10);
	char *fff = harbol_mempool_alloc(&i, sizeof *fff * 50);
	float *f32 = harbol_mempool_alloc(&i, sizeof *f32 * 23);
	char *jj = harbol_mempool_alloc(&i, sizeof *jj * 100);
	struct HarbolMemNode *ac = harbol_mempool_alloc(&i, sizeof *ac * 31);
	harbol_mempool_free(&i, fff);
	harbol_mempool_free(&i, fg);
	harbol_mempool_free(&i, ac);
	harbol_mempool_free(&i, f32);
	fprintf(debug_stream, "\nmempool :: pool size == %zu.\n", harbol_mempool_mem_remaining(&i));
	_print_mempool_nodes(&i, debug_stream);
	fprintf(debug_stream, "mempool :: heap bottom (%zu).\n", i.stack.offs);
	
	harbol_mempool_free(&i, hk);
	fprintf(debug_stream, "\ncrazy mempool :: pool size == %zu.\n", harbol_mempool_mem_remaining(&i));
	_print_mempool_nodes(&i, debug_stream);
		
	harbol_mempool_free(&i, jj);
	
	fprintf(debug_stream, "\nlast mempool :: pool size == %zu.\n", harbol_mempool_mem_remaining(&i));
	_print_mempool_nodes(&i, debug_stream);
	
	
	fprintf(debug_stream, "\nmempool :: pool size == %zu.\n", harbol_mempool_mem_remaining(&i));
	fputs("\n", debug_stream);
	_print_mempool_nodes(&i, debug_stream);
	
	fputs("\nmempool :: test reallocating jj to a single value.\n", debug_stream);
	jj = harbol_mempool_alloc(&i, sizeof *jj);
	*jj = 50;
	fprintf(debug_stream, "mempool :: jj == %i.\n", *jj);
	
	int *newer = harbol_mempool_realloc(&i, jj, sizeof *newer);
	fputs("\nmempool :: test reallocating jj to int ptr 'newer'.\n", debug_stream);
	fprintf(debug_stream, "mempool :: newer == %i.\n", *newer);
	
	jj = harbol_mempool_realloc(&i, newer, sizeof *jj);
	fputs("\nmempool :: test reallocating newer back to jj.\n", debug_stream);
	fprintf(debug_stream, "mempool :: jj == %i.\n", *jj);
	
	newer = harbol_mempool_realloc(&i, jj, sizeof *newer * 10);
	fputs("\nmempool :: test reallocating jj back to newer as an array of int[10].\n", debug_stream);
	for( size_t i=0; i<10; i++ ) {
		newer[i] = i+1;
		fprintf(debug_stream, "mempool :: newer[%zu] == %i.\n", i, newer[i]);
	}
	fputs("\n", debug_stream);
	newer = harbol_mempool_realloc(&i, newer, sizeof *newer * 5);
	for( size_t i=0; i<5; i++ )
		fprintf(debug_stream, "mempool :: reallocated newer[%zu] == %i.\n", i, newer[i]);
	harbol_mempool_free(&i, newer);
	
	const clock_t end = clock();
	printf("memory pool run time: %f\n", (end-start)/( double )CLOCKS_PER_SEC);
	/// free data
	fputs("\nmempool :: test destruction.\n", debug_stream);
	harbol_mempool_clear(&i);
	fprintf(debug_stream, "i's heap is null? '%s'\n", i.stack.mem != NIL ? "no" : "yes");
	fprintf(debug_stream, "i's freelist is null? '%s'\n", i.large.head != NULL ? "no" : "yes");
}