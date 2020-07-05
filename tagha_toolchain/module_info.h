#ifndef TAGHA_MODULE_INFO
#	define TAGHA_MODULE_INFO

#include "libharbol/harbol.h"
#include "../tagha/tagha.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline NO_NULL void tagha_module_print_header(const struct TaghaModule *const mod)
{
	const struct TaghaModuleHeader *const hdr = ( const struct TaghaModuleHeader* )mod->script;
	printf("Tagha Module::\nOperand Stack Size: %u bytes\nCall Stack Size: %u bytes\nTotal Stack Size: %u bytes\nHeap Size: %u bytes\nTotal Memory: %u bytes\nFunction Count: %u\nGlobal Var Count: %u\nScript Flags: %u\n", hdr->opstacksize, hdr->callstacksize, hdr->stacksize, hdr->heapsize, hdr->memsize, hdr->func_count, hdr->var_count, hdr->flags);
}

static inline NO_NULL void tagha_module_print_opstack(const struct TaghaModule *const mod)
{
	const uintptr_t max = mod->opstack + mod->opstack_size;
	size_t i=0;
	for( uintptr_t op_stack=mod->osp; op_stack<max; op_stack += sizeof(union TaghaVal), i++ ) {
		const union TaghaVal *const rsp = ( const union TaghaVal* )op_stack;
		printf("op stack : %-12zu - '%#" PRIx64 "'\n", i, rsp->uint64);
	}
}

static inline NO_NULL void tagha_module_print_callstack(const struct TaghaModule *const mod)
{
	const uintptr_t min = mod->callstack;
	size_t i=0;
	for( uintptr_t call_stack=mod->csp; call_stack>=min; call_stack -= sizeof(uintptr_t), i++ ) {
		const uintptr_t *const call = ( const uintptr_t* )call_stack;
		printf("call stack : %-10zu - '%" PRIuPTR "'\n", i, *call);
	}
}

#ifdef __cplusplus
} /** extern "C" */
#endif

#endif /** TAGHA_MODULE_INFO */