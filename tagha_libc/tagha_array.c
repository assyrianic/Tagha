#include <stdlib.h>
#include <stddef.h>

struct Vector {
	void **Table;
	size_t Len, Count;
};

struct Vector *Vector_New(void);
void Vector_Init(struct Vector *);
void Vector_Del(struct Vector *);
void Vector_Free(struct Vector **);

size_t Vector_Len(const struct Vector *);
size_t Vector_Count(const struct Vector *);
void **Vector_GetTable(const struct Vector *);
void Vector_Resize(struct Vector *);
void Vector_Truncate(struct Vector *);

bool Vector_Insert(struct Vector *, void *);
void *Vector_Pop(struct Vector *);
void *Vector_Get(const struct Vector *, size_t);
void Vector_Set(struct Vector *, size_t, void *);

void Vector_Delete(struct Vector *, size_t);
void Vector_Add(struct Vector *, const struct Vector *);
void Vector_Copy(struct Vector *, const struct Vector *);

static void safefree(void *ptr)
{
	void **ptrref = ptr;
	if( !ptrref || !*ptrref )
		return;
	free(*ptrref), *ptrref=NULL;
}

struct Vector *Vector_New(void)
{
	struct Vector *v = calloc(1, sizeof *v);
	return v;
}

void Vector_Init(struct Vector *const v)
{
	if( !v )
		return;
	
	*v = (struct Vector){0};
}

void Vector_Del(struct Vector *const v)
{
	if( !v || !v->Table )
		return;
	
	safefree(&v->Table);
	Vector_Init(v);
}

void Vector_Free(struct Vector **vecref)
{
	if( !*vecref )
		return;
	
	Vector_Del(*vecref);
	safefree(vecref);
}

size_t Vector_Len(const struct Vector *const v)
{
	return v ? v->Len : 0;
}

size_t Vector_Count(const struct Vector *const v)
{
	return v && v->Table ? v->Count : 0;
}
void **Vector_GetTable(const struct Vector *const v)
{
	return v ? v->Table : NULL;
}

void Vector_Resize(struct Vector *const restrict v)
{
	if( !v )
		return;
	
	// first we get our old size.
	// then resize the actual size.
	const size_t oldsize = v->Len;
	v->Len <<= 1;
	if( !v->Len )
		v->Len = 4;
	
	// allocate new table.
	void **newdata = calloc(v->Len, sizeof *newdata);
	if( !newdata ) {
		v->Len >>= 1;
		if( v->Len == 1 )
			v->Len=0;
		return;
	}
	
	// copy the old table to new then free old table.
	if( v->Table ) {
		memcpy(newdata, v->Table, sizeof *newdata * oldsize);
		safefree(&v->Table);
	}
	v->Table = newdata;
}

void Vector_Truncate(struct Vector *const restrict v)
{
	if( !v )
		return;
	
	if( v->Count < v->Len>>1 ) {
		v->Len >>= 1;
		// allocate new table.
		void **newdata = calloc(v->Len, sizeof *newdata);
		if( !newdata )
			return;
		
		// copy the old table to new then free old table.
		if( v->Table ) {
			memcpy(newdata, v->Table, sizeof *newdata * v->Len);
			safefree(&v->Table);
		}
		v->Table = newdata;
	}
}


bool Vector_Insert(struct Vector *const restrict v, const void *val)
{
	if( !v )
		return false;
	else if( !v->Table || v->Count >= v->Len )
		Vector_Resize(v);
	
	v->Table[v->Count++] = val;
	return true;
}

void *Vector_Pop(struct Vector *const v)
{
	return ( !v || !v->Table || !v->Count ) ? NULL : v->Table[--v->Count];
}

void *Vector_Get(const struct Vector *const v, const size_t index)
{
	return (!v || !v->Table || index >= v->Count) ? NULL : v->Table[index];
}

void Vector_Set(struct Vector *const restrict v, const size_t index, const void *val)
{
	if( !v || !v->Table || index >= v->Count )
		return;
	
	v->Table[index] = val;
}

void Vector_Delete(struct Vector *const v, const size_t index)
{
	if( !v || !v->Table || index >= v->Count )
		return;
	
	size_t
		i=index+1,
		j=index
	;
	v->Count--;
	memmove(v->Table+j, v->Table+i, (v->Count-j) * sizeof *v->Table);
}


/* tagha_vector *tagha_vector_new(void); */
static void native_tagha_vector_new(struct Tagha *const restrict sys, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	retval->Ptr = Vector_New();
}

/* bool tagha_vector_free(tagha_vector **vecref); */
static void native_tagha_vector_free(struct Tagha *const restrict sys, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	struct Vector **vecref = params[0].Ptr;
	Vector_Free(vecref);
	retval->Bool = *vecref==NULL;
}

/* bool tagha_vector_insert(struct tagha_vector *vec, void *ptr); */
static void native_tagha_vector_insert(struct Tagha *const restrict sys, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	retval->Bool = Vector_Insert(params[0].Ptr, params[1].Ptr);
}

/* void *tagha_vector_pop(const struct tagha_vector *vec); */
static void native_tagha_vector_pop(struct Tagha *const restrict sys, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	retval->Ptr = Vector_Pop(params[0].Ptr);
}

/* void *tagha_vector_get(const struct tagha_vector *vec, size_t index); */
static void native_tagha_vector_get(struct Tagha *const restrict sys, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	retval->Ptr = Vector_Get(params[0].Ptr, params[1].UInt64);
}

/* void tagha_vector_set(const struct tagha_vector *vec, size_t index, const void *ptr); */
static void native_tagha_vector_set(struct Tagha *const restrict sys, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	Vector_Set(params[0].Ptr, params[1].UInt64, params[2].Ptr);
}

/* void tagha_vector_del(struct tagha_vector *vec, size_t index); */
static void native_tagha_vector_del(struct Tagha *const restrict sys, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	Vector_Delete(params[0].Ptr, params[1].UInt64);
}

/* size_t tagha_vector_len(const struct tagha_vector *vec); */
static void native_tagha_vector_len(struct Tagha *const restrict sys, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	retval->UInt64 = Vector_Len(params[0].Ptr);
}

/* size_t tagha_vector_count(const struct tagha_vector *vec); */
static void native_tagha_vector_count(struct Tagha *const restrict sys, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	retval->UInt64 = Vector_Count(params[0].Ptr);
}

/* void tagha_vector_truncate(struct tagha_vector *vec); */
static void native_tagha_vector_truncate(struct Tagha *const restrict sys, union TaghaVal *const restrict retval, const size_t argc, union TaghaVal params[restrict static argc])
{
	Vector_Truncate(params[0].Ptr);
}


bool Tagha_Load_array_Natives(struct Tagha *const restrict sys)
{
	const struct NativeInfo tagha_array_natives[] = {
		{"tagha_vector_new", native_tagha_vector_new},
		{"tagha_vector_free", native_tagha_vector_free},
		{"tagha_vector_insert", native_tagha_vector_insert},
		{"tagha_vector_pop", native_tagha_vector_pop},
		{"tagha_vector_get", native_tagha_vector_get},
		{"tagha_vector_set", native_tagha_vector_set},
		{"tagha_vector_del", native_tagha_vector_del},
		{"tagha_vector_len", native_tagha_vector_len},
		{"tagha_vector_count", native_tagha_vector_count},
		{"tagha_vector_truncate", native_tagha_vector_truncate},
		{NULL, NULL}
	};
	return sys ? Tagha_RegisterNatives(sys, tagha_array_natives) : false;
}
