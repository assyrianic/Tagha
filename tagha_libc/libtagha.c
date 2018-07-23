#include <stdlib.h>
#include <stdio.h>

static uint8_t *Tagha_LoadModule(const char *restrict module_name)
{
	if( !module_name )
		return NULL;
	
	FILE *restrict tbcfile = fopen(module_name, "rb");
	if( !tbcfile )
		return NULL;
	
	size_t filesize = 0L;
	if( !fseek(tbcfile, 0, SEEK_END) ) {
		int64_t size = ftell(tbcfile);
		if( size == -1LL ) {
			fclose(tbcfile), tbcfile=NULL;
			return NULL;
		}
		rewind(tbcfile);
		filesize = (size_t)size;
	}
	
	uint8_t buffer[filesize];
	const size_t val = fread(buffer, sizeof(uint8_t), filesize, tbcfile);
	(void)val;
	fclose(tbcfile), tbcfile=NULL;
	
	if( !(buffer[0] == 0xDE and buffer[1]==0xC0) )
		return NULL;
	
	uint8_t *module = calloc(filesize, sizeof *module);
	if( !module )
		return NULL;
	
	memcpy(module, buffer, filesize);
	return module;
}

/* void *Tagha_LoadModule(const char *tbc_module_name); */
static void Native_TaghaLoadModule(struct Tagha *const restrict sys, union Value *const restrict retval, const size_t args, union Value params[static args])
{
	(void)sys; (void)args;
	const char *restrict module_name = params[0].Ptr;
	retval->Ptr = Tagha_LoadModule(module_name);
}

/* void *Tagha_GetGlobal(void *module, const char *symname); */
static void Native_TaghaGetGlobal(struct Tagha *const restrict sys, union Value *const restrict retval, const size_t args, union Value params[static args])
{
	(void)sys; (void)args;
	const char *restrict symname = params[1].Ptr;
	
	union Pointer reader = (union Pointer){.Ptr = params[0].UCharPtr + 7};
	const uint32_t vartable_offset = *reader.UInt32Ptr++;
	reader.UInt8Ptr += vartable_offset;
	
	const uint32_t globalvars = *reader.UInt32Ptr++;
	for( uint32_t i=0 ; i<globalvars ; i++ ) {
		reader.UInt8Ptr++;
		const uint64_t sizes = *reader.UInt64Ptr++;
		const uint32_t cstrlen = sizes & 0xffFFffFF;
		const uint32_t datalen = sizes >> 32;
		if( !strcmp(symname, reader.CStrPtr) ) {
			retval->Ptr = reader.UInt8Ptr + cstrlen;
			break;
		}
		else reader.UInt8Ptr += (cstrlen + datalen);
	}
}

/* bool Tagha_InvokeFunc(void *, const char *, union Value *, size_t, union Value []); */
static void Native_TaghaInvoke(struct Tagha *const restrict sys, union Value *const retval, const size_t args, union Value params[static args])
{
	(void)sys; (void)args;
	uint8_t *const restrict module = params[0].Ptr;
	if( !module ) {
		return;
	}
	const char *restrict funcname = params[1].Ptr;
	if( !funcname ) {
		return;
	}
	union Value *const restrict retdata = params[2].SelfPtr;
	if( !retdata ) {
		return;
	}
	union Value *const restrict array = params[4].SelfPtr;
	if( !array ) {
		return;
	}
	
	/* do a context switch to run the function.
	 * have to do this because we don't want to overwrite
	 * the existing stack and just replace it with a new stack.
	 * doing so will make the old stack memory lost which is not good.
	 * So let's save some of that data to prevent stack frame corruption.
	 */
	struct Tagha *const restrict vmswitch = &(struct Tagha){0};
	Tagha_Init(vmswitch, module);
	
	// make the call.
	Tagha_CallFunc(vmswitch, funcname, params[3].UInt64, array);
	*retdata = vmswitch->Regs[regAlaf];
	retval->Bool = true;
}

/* bool Tagha_FreeModule(void **module); */
static void Native_TaghaFreeModule(struct Tagha *const restrict sys, union Value *const restrict retval, const size_t args, union Value params[restrict static args])
{
	(void)sys; (void)args;
	uint8_t **module = params[0].Ptr;
	if( !module or !*module ) {
		return;
	}
	free(*module), *module = NULL;
	retval->Bool = *module == NULL;
}


bool Tagha_Load_libTagha_Natives(struct Tagha *const restrict sys)
{
	if( !sys )
		return false;
	
	const struct NativeInfo dynamic_loading[] = {
		{"Tagha_LoadModule", Native_TaghaLoadModule}, {"Tagha_GetGlobal", Native_TaghaGetGlobal},
		{"Tagha_InvokeFunc", Native_TaghaInvoke}, {"Tagha_FreeModule", Native_TaghaFreeModule},
		
		{"tagha_load_module", Native_TaghaLoadModule},
		{"tagha_get_global", Native_TaghaGetGlobal},
		{"tagha_invoke_func", Native_TaghaInvoke},
		{"tagha_free_module", Native_TaghaFreeModule},
		{NULL, NULL}
	};
	return Tagha_RegisterNatives(sys, dynamic_loading);
}
