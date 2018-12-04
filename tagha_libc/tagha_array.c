#include <stdlib.h>
#include <stddef.h>


/* struct HarbolVector *harbol_vector_new(void); */
static void native_harbol_vector_new(struct Tagha *const restrict module, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	retval->Ptr = harbol_vector_new();
}

/* bool harbol_vector_free(struct HarbolVector **vecref); */
static void native_harbol_vector_free(struct Tagha *const restrict module, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	struct HarbolVector **vecref = params[0].Ptr;
	harbol_vector_free(vecref);
	retval->Bool = *vecref==NULL;
}

/* bool harbol_vector_insert(struct HarbolVector *vec, void *ptr); */
static void native_harbol_vector_insert(struct Tagha *const restrict module, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	union HarbolValue insert_value = {0};
	insert_value.Ptr = params[1].Ptr;
	retval->Bool = harbol_vector_insert(params[0].Ptr, insert_value);
}

/* void *harbol_vector_pop(const struct HarbolVector *vec); */
static void native_harbol_vector_pop(struct Tagha *const restrict module, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	retval->Ptr = harbol_vector_pop(params[0].Ptr);
}

/* void *harbol_vector_get(const struct HarbolVector *vec, size_t index); */
static void native_harbol_vector_get(struct Tagha *const restrict module, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	retval->Ptr = harbol_vector_get(params[0].Ptr, params[1].UInt64).Ptr;
}

/* void harbol_vector_set(const struct HarbolVector *vec, size_t index, const void *ptr); */
static void native_harbol_vector_set(struct Tagha *const restrict module, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	union HarbolValue insert_value = {0};
	insert_value.Ptr = params[2].Ptr;
	harbol_vector_Set(params[0].Ptr, params[1].UInt64, insert_value);
}

/* void harbol_vector_del(struct HarbolVector *vec, size_t index); */
static void native_harbol_vector_del(struct Tagha *const restrict module, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	harbol_vector_Delete(params[0].Ptr, params[1].UInt64);
}

/* size_t harbol_vector_len(const struct HarbolVector *vec); */
static void native_harbol_vector_len(struct Tagha *const restrict module, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	retval->UInt64 = harbol_vector_Len(params[0].Ptr);
}

/* size_t harbol_vector_count(const struct HarbolVector *vec); */
static void native_harbol_vector_count(struct Tagha *const restrict module, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	retval->UInt64 = harbol_vector_Count(params[0].Ptr);
}

/* void harbol_vector_truncate(struct HarbolVector *vec); */
static void native_harbol_vector_truncate(struct Tagha *const restrict module, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	harbol_vector_Truncate(params[0].Ptr);
}


bool tagha_module_load_array_natives(struct Tagha *const module)
{
	const struct NativeInfo tagha_array_natives[] = {
		{"harbol_vector_new", native_harbol_vector_new},
		{"harbol_vector_free", native_harbol_vector_free},
		{"harbol_vector_insert", native_harbol_vector_insert},
		{"harbol_vector_pop", native_harbol_vector_pop},
		{"harbol_vector_get", native_harbol_vector_get},
		{"harbol_vector_set", native_harbol_vector_set},
		{"harbol_vector_del", native_harbol_vector_del},
		{"harbol_vector_len", native_harbol_vector_len},
		{"harbol_vector_count", native_harbol_vector_count},
		{"harbol_vector_truncate", native_harbol_vector_truncate},
		{NULL, NULL}
	};
	return module ? tagha_module_register_natives(module, tagha_array_natives) : false;
}
