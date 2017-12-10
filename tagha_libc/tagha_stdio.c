
#include <stdio.h>
#include <string.h>

/*
 * File Ops
 */

/* int remove(const char *filename); */
static void native_remove(struct TaghaScript *script, union CValue params[], union CValue *restrict retval, const uint32_t argc, struct TaghaVM *env)
{
	const char *filename = params[0].String;
	retval->Int32 = remove(filename);
}

/* int rename(const char *oldname, const char *newname); */
static void native_rename(struct TaghaScript *script, union CValue params[], union CValue *restrict retval, const uint32_t argc, struct TaghaVM *env)
{
	const char *filename = params[0].String;
	const char *newname = params[1].String;
	retval->Int32 = rename(filename, newname);
}

/* FILE *tmpfile(void); */
static void native_tmpfile(struct TaghaScript *script, union CValue params[], union CValue *restrict retval, const uint32_t argc, struct TaghaVM *env)
{
	retval->Ptr = tmpfile();
}

/* char *tmpnam(char *str); */
static void native_tmpnam(struct TaghaScript *script, union CValue params[], union CValue *restrict retval, const uint32_t argc, struct TaghaVM *env)
{
	char *str = params[0].Ptr;
	retval->Ptr = tmpnam(str);
}


/*
 * File Access
 */

/* int fclose(FILE *stream); */
static void native_fclose(struct TaghaScript *script, union CValue params[], union CValue *restrict retval, const uint32_t argc, struct TaghaVM *env)
{
	FILE *pFile = params[0].Ptr;
	if( pFile ) {
		puts("fclose:: closing FILE*\n");
		retval->Int32 = fclose(pFile);
		pFile=NULL;
	}
	else {
		puts("fclose:: FILE* is NULL\n");
		retval->Int32 = -1;
	}
}

/* int fflush(FILE *stream); */
static void native_fflush(struct TaghaScript *script, union CValue params[], union CValue *restrict retval, const uint32_t argc, struct TaghaVM *env)
{
	FILE *pStream = params[0].Ptr;
	retval->Int32 = fflush(pStream);
	pStream = NULL;
}

/* FILE *fopen(const char *filename, const char *modes); */
static void native_fopen(struct TaghaScript *script, union CValue params[], union CValue *restrict retval, const uint32_t argc, struct TaghaVM *env)
{
	const char *filename = params[0].String;
	const char *mode = params[1].String;
	if( !filename ) {
		puts("fopen reported an ERROR :: **** param 'filename' is NULL ****\n");
		goto error;
	}
	else if( !mode ) {
		puts("fopen reported an ERROR :: **** param 'modes' is NULL ****\n");
		goto error;
	}
	
	FILE *pFile = fopen(filename, mode);
	if( pFile )
		printf("fopen:: opening file \'%s\' with mode: \'%s\'\n", filename, mode);
	else printf("fopen: failed to get filename: \'%s\'\n", filename);
	retval->Ptr = pFile;
	return;
error:;
	retval->Ptr = NULL;
}

/* FILE *freopen(const char *filename, const char *mode, FILE *stream); */
static void native_freopen(struct TaghaScript *script, union CValue params[], union CValue *restrict retval, const uint32_t argc, struct TaghaVM *env)
{
	const char *filename = params[0].String;
	const char *mode = params[1].String;
	FILE *pStream = params[2].Ptr;
	
	if( !filename ) {
		puts("freopen reported an ERROR :: **** param 'filename' is NULL ****\n");
		goto error;
	} else if( !mode ) {
		puts("freopen reported an ERROR :: **** param 'modes' is NULL ****\n");
		goto error;
	} else if( !pStream ) {
		puts("freopen reported an ERROR :: **** param 'stream' is NULL ****\n");
		goto error;
	}
	
	FILE *pFile = freopen(filename, mode, pStream);
	if( pFile )
		printf("freopen:: opening file \'%s\' with mode: \'%s\'\n", filename, mode);
	else printf("freopen: failed to get filename: \'%s\'\n", filename);
	retval->Ptr = pFile;
	return;
error:;
	retval->Ptr = NULL;
}

/* void setbuf(FILE *stream, char *buffer); */
static void native_setbuf(struct TaghaScript *script, union CValue params[], union CValue *restrict retval, const uint32_t argc, struct TaghaVM *env)
{
	FILE *pStream = params[0].Ptr;
	char *pBuffer = params[1].Ptr;
	
	if( !pStream ) {
		puts("setbuf reported an ERROR :: **** param 'stream' is NULL ****\n");
		return;
	}
	setbuf(pStream, pBuffer);
}


/*
 * File Access
 */

int32_t gnprintf(char *buffer, size_t maxlen, const char *format, CValue params[], uint32_t numparams, uint32_t *curparam);

/* int fprintf(FILE *stream, const char *format, ...); */
static void native_fprintf(struct TaghaScript *script, union CValue params[], union CValue *restrict retval, const uint32_t argc, struct TaghaVM *env)
{
	FILE *stream = params[0].Ptr;
	if( !stream ) {
		retval->Int32 = -1;
		puts("fprintf reported an ERROR :: **** param 'stream' is NULL ****\n");
		return;
	}
	
	const char *format = params[1].String;
	if( !format ) {
		retval->Int32 = -1;
		puts("fprintf reported an ERROR :: **** param 'format' is NULL ****\n");
		return;
	}
	
	char data_buffer[1024] = {0};
	uint32_t param = 2;
	gnprintf(data_buffer, 1024, format, params, argc-2, &param);
	data_buffer[1023] = 0;
	retval->Int32 = fprintf(stream, "%s", data_buffer);
}

/* int fscanf(FILE *stream, const char *format, ...); */
static void native_fscanf(struct TaghaScript *script, union CValue params[], union CValue *restrict retval, const uint32_t argc, struct TaghaVM *env)
{
	FILE *stream = params[0].Ptr;
	if( !stream ) {
		retval->Int32 = -1;
		puts("fscanf reported an ERROR :: **** param 'stream' is NULL ****\n");
		return;
	}
	
	const char *format = params[1].String;
	if( !format ) {
		retval->Int32 = -1;
		puts("fscanf reported an ERROR :: **** param 'format' is NULL ****\n");
		return;
	}
	
	retval->Int32 = -1;
}

/* int printf(const char *fmt, ...); */
static void native_printf(struct TaghaScript *script, union CValue params[], union CValue *restrict retval, const uint32_t argc, struct TaghaVM *env)
{
	const char *str = params[0].String;
	if( !str ) {
		retval->Int32 = -1;
		puts("printf reported an ERROR :: **** param 'fmt' is NULL ****\n");
		return;
	}
	char data_buffer[4096] = {0};
	uint32_t param = 1;
	retval->Int32 = gnprintf(data_buffer, 4096, str, params, argc-1, &param);
	data_buffer[4095] = 0;	// make sure we null terminator.
	printf("%s", data_buffer);
}

/* int puts(const char *s); */
static void native_puts(struct TaghaScript *script, union CValue params[], union CValue *restrict retval, const uint32_t argc, struct TaghaVM *env)
{
	const char *str = params[0].String;
	if( !str ) {
		retval->Int32 = -1;
		puts("native_puts reported an ERROR :: **** param 's' is NULL ****\n");
		return;
	}
	// push back the value of the return val of puts.
	retval->Int32 = puts(str);
	str = NULL;
}

/* int setvbuf(FILE *stream, char *buffer, int mode, size_t size); */
static void native_setvbuf(struct TaghaScript *script, union CValue params[], union CValue *restrict retval, const uint32_t argc, struct TaghaVM *env)
{
	FILE *stream = params[0].Ptr;
	char *buffer = params[1].Ptr;
	if( !stream ) {
		puts("native_setvbuf reported an ERROR :: **** param 'stream' is NULL ****\n");
		retval->Int32 = -1;
		return;
	} else if( !buffer ) {
		puts("native_setvbuf reported an ERROR :: **** param 'buffer' is NULL ****\n");
		retval->Int32 = -1;
		return;
	}
	int32_t mode = params[2].Int32;
	uint64_t size = params[3].Int64;
	// push back the value of the return val of puts.
	retval->Int32 = setvbuf(stream, buffer, mode, size);
}

/* int fgetc(FILE *stream); */
static void native_fgetc(struct TaghaScript *script, union CValue params[], union CValue *restrict retval, const uint32_t argc, struct TaghaVM *env)
{
	FILE *stream = params[0].Ptr;
	retval->Int32 = fgetc(stream);
}

/* char *fgets(char *str, int num, FILE *stream); */
static void native_fgets(struct TaghaScript *script, union CValue params[], union CValue *restrict retval, const uint32_t argc, struct TaghaVM *env)
{
	char *str = params[0].Ptr;
	int num = params[1].Int32;
	FILE *stream = params[2].Ptr;
	retval->Ptr = fgets(str, num, stream);
}

/* int fputc(int character, FILE *stream); */
static void native_fputc(struct TaghaScript *script, union CValue params[], union CValue *restrict retval, const uint32_t argc, struct TaghaVM *env)
{
	int character = params[0].Int32;
	FILE *stream = params[1].Ptr;
	retval->Int32 = fputc(character, stream);
}

/* int fputs(const char *str, FILE *stream); */
static void native_fputs(struct TaghaScript *script, union CValue params[], union CValue *restrict retval, const uint32_t argc, struct TaghaVM *env)
{
	const char *str = params[0].String;
	FILE *stream = params[1].Ptr;
	retval->Int32 = fputs(str, stream);
}

/* int getc(FILE *stream); */
static void native_getc(struct TaghaScript *script, union CValue params[], union CValue *restrict retval, const uint32_t argc, struct TaghaVM *env)
{
	FILE *stream = params[0].Ptr;
	retval->Int32 = getc(stream);
}

/* int getchar(void); */
static void native_getchar(struct TaghaScript *script, union CValue params[], union CValue *restrict retval, const uint32_t argc, struct TaghaVM *env)
{
	retval->Int32 = getchar();
}

/* int putc(int character, FILE *stream); */
static void native_putc(struct TaghaScript *script, union CValue params[], union CValue *restrict retval, const uint32_t argc, struct TaghaVM *env)
{
	int character = params[0].Int32;
	FILE *stream = params[1].Ptr;
	retval->Int32 = putc(character, stream);
}

/* int putchar(int character); */
static void native_putchar(struct TaghaScript *script, union CValue params[], union CValue *restrict retval, const uint32_t argc, struct TaghaVM *env)
{
	int character = params[0].Int32;
	retval->Int32 = putchar(character);
}

/* int ungetc(int character, FILE *stream); */
static void native_ungetc(struct TaghaScript *script, union CValue params[], union CValue *restrict retval, const uint32_t argc, struct TaghaVM *env)
{
	int character = params[0].Int32;
	FILE *stream = params[1].Ptr;
	retval->Int32 = ungetc(character, stream);
}

/* size_t fread(void *ptr, size_t size, size_t count, FILE *stream); */
static void native_fread(struct TaghaScript *script, union CValue params[], union CValue *restrict retval, const uint32_t argc, struct TaghaVM *env)
{
	void *ptr = params[0].Ptr;
	uint64_t size = params[1].UInt64;
	uint64_t count = params[2].UInt64;
	FILE *stream = params[3].Ptr;
	retval->UInt64 = fread(ptr, size, count, stream);
}

/* size_t fwrite(const void *ptr, size_t size, size_t count, FILE *stream); */
static void native_fwrite(struct TaghaScript *script, union CValue params[], union CValue *restrict retval, const uint32_t argc, struct TaghaVM *env)
{
	const void *ptr = params[0].Ptr;
	uint64_t size = params[1].UInt64;
	uint64_t count = params[2].UInt64;
	FILE *stream = params[3].Ptr;
	retval->UInt64 = fwrite(ptr, size, count, stream);
}

/* int fseek(FILE *stream, long int offset, int origin); */
static void native_fseek(struct TaghaScript *script, union CValue params[], union CValue *restrict retval, const uint32_t argc, struct TaghaVM *env)
{
	FILE *stream = params[0].Ptr;
	uint64_t offset = params[1].UInt64;
	uint32_t origin = params[2].UInt32;
	retval->Int32 = fseek(stream, offset, origin);
}

/* long int ftell(FILE *stream); */
static void native_ftell(struct TaghaScript *script, union CValue params[], union CValue *restrict retval, const uint32_t argc, struct TaghaVM *env)
{
	FILE *stream = params[0].Ptr;
	retval->Int64 = ftell(stream);
}

/* void rewind(FILE *stream); */
static void native_rewind(struct TaghaScript *script, union CValue params[], union CValue *restrict retval, const uint32_t argc, struct TaghaVM *env)
{
	FILE *stream = params[0].Ptr;
	rewind(stream);
}

/* void clearerr(FILE *stream); */
static void native_clearerr(struct TaghaScript *script, union CValue params[], union CValue *restrict retval, const uint32_t argc, struct TaghaVM *env)
{
	FILE *stream = params[0].Ptr;
	clearerr(stream);
}

/* int feof(FILE *stream); */
static void native_feof(struct TaghaScript *script, union CValue params[], union CValue *restrict retval, const uint32_t argc, struct TaghaVM *env)
{
	FILE *stream = params[0].Ptr;
	retval->Int32 = feof(stream);
}

/* int ferror(FILE *stream); */
static void native_ferror(struct TaghaScript *script, union CValue params[], union CValue *restrict retval, const uint32_t argc, struct TaghaVM *env)
{
	FILE *stream = params[0].Ptr;
	retval->Int32 = ferror(stream);
}

/* void perror(const char *str); */
static void native_perror(struct TaghaScript *script, union CValue params[], union CValue *restrict retval, const uint32_t argc, struct TaghaVM *env)
{
	const char *str = params[0].String;
	perror(str);
}



void Tagha_load_stdio_natives(struct TaghaVM *vm)
{
	if( !vm )
		return;
	
	NativeInfo libc_stdio_natives[] = {
		{"remove", native_remove},
		{"rename", native_rename},
		{"tmpfile", native_tmpfile},
		{"printf", native_printf},
		{"puts", native_puts},
		{"fopen", native_fopen},
		{"fclose", native_fclose},
		{"freopen", native_freopen},
		{"setbuf", native_setbuf},
		{"fflush", native_fflush},
		{"setvbuf", native_setvbuf},
		{"fgetc", native_fgetc},
		{"fgets", native_fgets},
		{"fputc", native_fputc},
		{"fputs", native_fputs},
		{"getc", native_getc},
		{"getchar", native_getchar},
		{"putc", native_putc},
		{"putchar", native_putchar},
		{"ungetc", native_ungetc},
		{"fread", native_fread},
		{"fwrite", native_fwrite},
		{"fseek", native_fseek},
		{"ftell", native_ftell},
		{"rewind", native_rewind},
		{"clearerr", native_clearerr},
		{"feof", native_feof},
		{"ferror", native_ferror},
		{"perror", native_perror},
		{NULL, NULL}
	};
	Tagha_RegisterNatives(vm, libc_stdio_natives);
}











