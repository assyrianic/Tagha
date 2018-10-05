#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
static size_t GetFileSize(FILE *const restrict file)
{
	int64_t size = 0L;
	if( !file )
		return size;
	
	if( !fseek(file, 0, SEEK_END) ) {
		size = ftell(file);
		if( size == -1 )
			return 0L;
		rewind(file);
	}
	return (size_t)size;
}
*/
static uint8_t *Tagha_LoadModule(const char *restrict module_name)
{
	if( !module_name )
		return NULL;
	
	FILE *restrict tbcfile = fopen(module_name, "rb");
	if( !tbcfile ) {
		return NULL;
	}
	const size_t filesize = GetFileSize(tbcfile);
	uint8_t *restrict module = calloc(filesize, sizeof *module);
	if( !module ) {
		fclose(tbcfile), tbcfile=NULL;
		return NULL;
	}
	const size_t val = fread(module, sizeof(uint8_t), filesize, tbcfile);
	(void)val;
	fclose(tbcfile), tbcfile=NULL;
	return module;
}

/* void *Tagha_LoadModule(const char *tbc_module_name); */
void Native_TaghaLoadModule(struct Tagha *const sys, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)sys; (void)args;
	const char *restrict module_name = params[0].Ptr;
	uint8_t *restrict loaded_plugin = Tagha_LoadModule(module_name);
	retval->Ptr = loaded_plugin ? Tagha_New(loaded_plugin) : NULL;
}

/* void *Tagha_GetGlobal(void *module, const char *symname); */
void Native_TaghaGetGlobal(struct Tagha *const restrict sys, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)sys; (void)args;
	const char *restrict symname = params[1].Ptr;
	retval->Ptr = Tagha_GetGlobalVarByName(params[0].Ptr, symname);
}

/* int32_t Tagha_InvokeFunc(void *, const char *, union TaghaVal *, size_t, union TaghaVal []); */
void Native_TaghaInvoke(struct Tagha *const restrict sys, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)sys; (void)args;
	struct Tagha *const restrict module = params[0].Ptr;
	const char *restrict funcname = params[1].Ptr;
	union TaghaVal
		*const restrict retdata = params[2].SelfPtr,
		*const restrict array = params[4].SelfPtr
	;
	if( !module || !retdata ) {
		return;
	}
	const size_t array_size = params[3].UInt64;
	
	// make the call.
	retval->Int32 = Tagha_CallFunc(module, funcname, array_size, array);
	memcpy(retdata, &module->regAlaf, sizeof *retdata);
}

/* bool Tagha_FreeModule(void **module); */
void Native_TaghaFreeModule(struct Tagha *const restrict sys, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)sys; (void)args;
	struct Tagha **restrict module = params[0].Ptr;
	if( !module || !*module ) {
		return;
	}
	free((*module)->Header), (*module)->Header = NULL;
	Tagha_Free(module);
	retval->Bool = *module == NULL;
}


bool Tagha_LoadlibTaghaNatives(struct Tagha *const restrict sys)
{
	const struct NativeInfo dynamic_loading[] = {
		{"Tagha_LoadModule", Native_TaghaLoadModule},
		{"Tagha_GetGlobal", Native_TaghaGetGlobal},
		{"Tagha_InvokeFunc", Native_TaghaInvoke},
		{"Tagha_FreeModule", Native_TaghaFreeModule},
		{NULL, NULL}
	};
	return sys ? Tagha_RegisterNatives(sys, dynamic_loading) : false;
}

