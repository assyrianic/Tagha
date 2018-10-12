#include <stdlib.h>
#include <stdio.h>
#include <string.h>


static uint8_t *Tagha_LoadModule(const char *restrict module_name)
{
	if( !module_name )
		return NULL;
	
	FILE *script = fopen(module_name, "rb");
	if( !script )
		return NULL;
	
	fseek(script, 0, SEEK_END);
	const int64_t filesize = ftell(script);
	if( filesize <= -1 ) {
		fclose(script);
		return NULL;
	}
	rewind(script);
	
	uint8_t *restrict process = calloc(filesize, sizeof *process);
	if( fread(process, sizeof *process, filesize, script) != (size_t)filesize ) {
		free(process), process=NULL;
		fclose(script), script=NULL;
		return NULL;
	} else {
		fclose(script), script=NULL;
		return process;
	}
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
	*retdata = module->regAlaf;
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

