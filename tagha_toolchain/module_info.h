#ifndef TAGHA_MODULE_INFO
#	define TAGHA_MODULE_INFO


#include "../tagha/tagha.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline NO_NULL void tagha_module_print_header(const struct TaghaModule *const mod, FILE *const stream)
{
#ifdef __cplusplus
	const struct TaghaModuleHeader *const hdr = reinterpret_cast< decltype(hdr) >(mod->script);
#else
	const struct TaghaModuleHeader *const hdr = ( const struct TaghaModuleHeader* )(mod->script);
#endif
	fprintf(stream, "Tagha Module::\nOperand Stack Size: %u bytes\nCall Stack Size: %u bytes\nTotal Stack Size: %u bytes\nHeap Size: %u bytes\nTotal Memory: %u bytes\nFunction Count: %u\nGlobal Var Count: %u\nScript Flags: %u\n", hdr->opstacksize, hdr->callstacksize, hdr->stacksize, hdr->heapsize, hdr->memsize, hdr->func_count, hdr->var_count, hdr->flags);
}

static inline NO_NULL void tagha_module_print_opstack(const struct TaghaModule *const mod, FILE *const stream)
{
	size_t i = 0;
#ifdef __cplusplus
	const union TaghaVal *const top = reinterpret_cast< decltype(top) >(mod->opstack + mod->opstack_size);
	for( const union TaghaVal *head = reinterpret_cast< decltype(opstk) >(mod->osp); head < top; head++ ) {
#else
	const union TaghaVal *const top = ( const union TaghaVal* )(mod->opstack + mod->opstack_size);
	for( const union TaghaVal *head = ( const union TaghaVal* )(mod->osp); head < top; head++ ) {
#endif
		fprintf(stream, "op stack : %-12zu - '%#" PRIx64 "'\n", i, head->uint64);
		i++;
	}
}

static inline NO_NULL void tagha_module_print_callstack(const struct TaghaModule *const mod, FILE *const stream)
{
	size_t i = 0;
#ifdef __cplusplus
	const uintptr_t *const base = reinterpret_cast< decltype(top) >(mod->callstack);
	for( const uintptr_t *head = reinterpret_cast< decltype(head) >(mod->csp); head >= base; head-- ) {
#else
	const uintptr_t *const base = ( const uintptr_t* )(mod->callstack);
	for( const uintptr_t *head = ( const uintptr_t* )(mod->csp); head >= base; head-- ) {
#endif
		fprintf(stream, "call stack : %-10zu - '%" PRIuPTR "'\n", i, *head);
		i++;
	}
}

#ifdef __cplusplus
} /** extern "C" */
#endif

#endif /** TAGHA_MODULE_INFO */