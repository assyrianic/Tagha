
#include <stdio.h>


/* FILE *get_stdout(void); */
static void native_get_stdout(Script_t *restrict script, Param_t params[], const uint32_t argc)
{
	TaghaScript_push_int64( script, (uintptr_t)stdout );
}

/* FILE *get_stdin(void); */
static void native_get_stdin(Script_t *restrict script, Param_t params[], const uint32_t argc)
{
	TaghaScript_push_int64( script, (uintptr_t)stdin );
}

/* FILE *get_stderr(void); */
static void native_get_stderr(Script_t *restrict script, Param_t params[], const uint32_t argc)
{
	TaghaScript_push_int64( script, (uintptr_t)stderr );
}

/*
 * File Ops
 */

/* int remove(const char *filename); */
static void native_remove(Script_t *restrict script, Param_t params[], const uint32_t argc)
{
	const char *filename = params[0].String;//(const char *)(uintptr_t)TaghaScript_pop_int64(script);
	TaghaScript_push_int32( script, remove(filename) );
}

/* int rename(const char *oldname, const char *newname); */
static void native_rename(Script_t *restrict script, Param_t params[], const uint32_t argc)
{
	const char *filename = params[0].String;//(const char *)(uintptr_t)TaghaScript_pop_int64(script);
	const char *newname = params[1].String;//(const char *)(uintptr_t)TaghaScript_pop_int64(script);
	TaghaScript_push_int32( script, rename(filename, newname) );
}

/* FILE *tmpfile(void); */
static void native_tmpfile(Script_t *restrict script, Param_t params[], const uint32_t argc)
{
	FILE *pTemp = tmpfile();
	TaghaScript_push_int64( script, (uintptr_t)pTemp );
}

/* char *tmpnam(char *str); */
static void native_tmpnam(Script_t *restrict script, Param_t params[], const uint32_t argc)
{
	char *str = params[0].Pointer;//(char *)(uintptr_t)TaghaScript_pop_int64(script);
	TaghaScript_push_int64( script, (uintptr_t)tmpnam(str) );
}


/*
 * File Access
 */

/* int fclose(FILE *stream); */
static void native_fclose(Script_t *restrict script, Param_t params[], const uint32_t argc)
{
	if( !script )
		return;
	
	FILE *pFile = params[0].Pointer;//(FILE *)(uintptr_t) TaghaScript_pop_int64(script);
	if( pFile ) {
		printf("fclose:: closing FILE*\n");
		TaghaScript_push_int32(script, fclose(pFile));
		pFile=NULL;
	}
	else {
		printf("fclose:: FILE* is NULL\n");
		TaghaScript_push_int32(script, -1);
	}
}

/* int fflush(FILE *stream); */
static void native_fflush(Script_t *restrict script, Param_t params[], const uint32_t argc)
{
	if( !script )
		return;
	
	FILE *pStream = params[0].Pointer;//(FILE *)(uintptr_t) TaghaScript_pop_int64(script);
	TaghaScript_push_int32(script, fflush(pStream));
	pStream = NULL;
}

/* FILE *fopen(const char *filename, const char *modes); */
static void native_fopen(Script_t *restrict script, Param_t params[], const uint32_t argc)
{
	if( !script )
		return;
	
	const char *filename = params[0].String;//(const char *)(uintptr_t)TaghaScript_pop_int64(script);
	const char *mode = params[1].String;////(const char *)(uintptr_t)TaghaScript_pop_int64(script);
	if( !filename ) {
		puts("fopen reported an ERROR :: **** param 'filename' is NULL ****\n");
		goto error;
	}
	else if( !mode ) {
		puts("fopen reported an ERROR :: **** param 'modes' is NULL ****\n");
		goto error;
	}
	
	FILE *pFile = fopen(filename, mode);
	if( pFile ) {
		printf("fopen:: opening file \'%s\' with mode: \'%s\'\n", filename, mode);
		TaghaScript_push_int64(script, (uintptr_t)pFile);
	}
	else {
		printf("fopen: failed to get filename: \'%s\'\n", filename);
		TaghaScript_push_int64(script, 0);
	}
	return;
error:;
	TaghaScript_push_int64(script, 0);
}

/* FILE *freopen(const char *filename, const char *mode, FILE *stream); */
static void native_freopen(Script_t *restrict script, Param_t params[], const uint32_t argc)
{
	if( !script )
		return;
	
	const char *filename = params[0].String;//(const char *)(uintptr_t)TaghaScript_pop_int64(script);
	const char *mode = params[1].String;//(const char *)(uintptr_t)TaghaScript_pop_int64(script);
	FILE *pStream = params[2].Pointer;//(FILE *)(uintptr_t)TaghaScript_pop_int64(script);
	
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
	if( pFile ) {
		printf("freopen:: opening file \'%s\' with mode: \'%s\'\n", filename, mode);
		TaghaScript_push_int64(script, (uintptr_t)pFile);
	}
	else {
		printf("freopen: failed to get filename: \'%s\'\n", filename);
		TaghaScript_push_int64(script, 0);
	}
	return;
error:;
	TaghaScript_push_int64(script, 0);
}

/* void setbuf(FILE *stream, char *buffer); */
static void native_setbuf(Script_t *restrict script, Param_t params[], const uint32_t argc)
{
	if( !script )
		return;
	
	FILE *pStream = params[0].Pointer;//(FILE *)(uintptr_t) TaghaScript_pop_int64(script);
	char *pBuffer = params[1].Pointer;//(char *)(uintptr_t) TaghaScript_pop_int64(script);
	if( !pStream ) {
		puts("setbuf reported an ERROR :: **** param 'stream' is NULL ****\n");
		return;
	}
	setbuf(pStream, pBuffer);
}

void string_format(const char *str);

/* int printf(const char *fmt, ...); */
static void native_printf(Script_t *restrict script, Param_t params[], const uint32_t argc)
{
	const char *str = params[0].String;//(const char *)(uintptr_t)TaghaScript_pop_int64(script);
	if( !str ) {
		TaghaScript_push_int32(script, -1);
		puts("native_printf reported an ERROR :: **** param 'fmt' is NULL ****\n");
		return;
	}
	
	char *iter=(char *)str;
	uint8_t param = 1;
	
	int32_t chrs=0;
	while( *iter ) {
		if( *iter=='%' ) {
			iter++;
			char data_buffer[1024];
			switch( *iter ) {
				case '%':
					printf("%%");
					chrs++;
					break;
					
				case 'f':
				case 'F':
					chrs += sprintf(data_buffer, "%f", params[param].Double);
					printf("%s", data_buffer);
					param++;
					break;
					
				case 'e':
				case 'E':
					chrs += sprintf(data_buffer, "%e", params[param].Double);
					printf("%s", data_buffer);
					param++;
					break;
					
				case 'a':
				case 'A':
					chrs += sprintf(data_buffer, "%a", params[param].Double);
					printf("%s", data_buffer);
					param++;
					break;
					
				case 'i':
				case 'd':
					chrs += sprintf(data_buffer, "%i", params[param].Int32);
					printf("%s", data_buffer);
					param++;
					break;
					
				case 'u':
					chrs += sprintf(data_buffer, "%u", params[param].UInt32);
					printf("%s", data_buffer);
					param++;
					break;
					
				case 'x':
				case 'X':
					chrs += sprintf(data_buffer, "%x", params[param++].UInt32);
					printf("%s", data_buffer);
					break;
					
				case 'o':
					chrs += sprintf(data_buffer, "%o", params[param++].UInt32);
					printf("%s", data_buffer);
					break;
					
				case 'c':
					chrs += sprintf(data_buffer, "%c", params[param++].Char);
					printf("%s", data_buffer);
					break;
					
				case 'p':
					chrs += sprintf(data_buffer, "%p", params[param++].Pointer);
					printf("%s", data_buffer);
					break;
					
				case 's':
					chrs += sprintf(data_buffer, "%s", params[param++].String);
					printf("%s", data_buffer);
					break;
					
				default:
					puts("invalid format\n");
					TaghaScript_push_int32(script, -1);
					return;
			} /* switch( *iter ) */
		} /* if( *iter=='%' ) */
		else putchar(*iter);
		chrs++, iter++;
	} /* while( *iter ) */
	iter = NULL;
	TaghaScript_push_int32(script, (uint32_t)chrs);
}

/* int puts(const char *s); */
static void native_puts(Script_t *restrict script, Param_t params[], const uint32_t argc)
{
	const char *str = params[0].String; //(const char *)(uintptr_t)TaghaScript_pop_int64(script);
	if( !str ) {
		TaghaScript_push_int32(script, -1);
		puts("native_puts reported an ERROR :: **** param 's' is NULL ****\n");
		return;
	}
	
	// push back the value of the return val of puts.
	TaghaScript_push_int32(script, puts(str));
	str = NULL;
}

/* int setvbuf(FILE *stream, char *buffer, int mode, size_t size); */
static void native_setvbuf(Script_t *restrict script, Param_t params[], const uint32_t argc)
{
	FILE *stream = params[0].Pointer;//(FILE *)(uintptr_t)TaghaScript_pop_int64(script);
	char *buffer = params[1].Pointer;//(char *)(uintptr_t)TaghaScript_pop_int64(script);
	int32_t mode = params[2].Int32;//TaghaScript_pop_int32(script);
	uint64_t size = params[3].Int64;//TaghaScript_pop_int64(script);
	if( !stream ) {
		puts("setvbuf reported an ERROR :: **** param 'stream' is NULL ****\n");
		goto error;
	}
	// push back the value of the return val of puts.
	TaghaScript_push_int32(script, setvbuf(stream, buffer, mode, size));
	return;
error:;
	TaghaScript_push_int32(script, (uint32_t)-1);
}

/* int fgetc(FILE *stream); */
static void native_fgetc(Script_t *restrict script, Param_t params[], const uint32_t argc)
{
	FILE *stream = params[0].Pointer;//(FILE *)(uintptr_t)TaghaScript_pop_int64(script);
	TaghaScript_push_int32(script, fgetc(stream));
}

/* char *fgets(char *str, int num, FILE *stream); */
static void native_fgets(Script_t *restrict script, Param_t params[], const uint32_t argc)
{
	char *str = params[0].Pointer;//(char *)(uintptr_t)TaghaScript_pop_int64(script);
	int num = params[1].Int32;//TaghaScript_pop_int32(script);
	FILE *stream = params[2].Pointer;//(FILE *)(uintptr_t)TaghaScript_pop_int64(script);
	TaghaScript_push_int64(script, (uintptr_t)fgets(str, num, stream));
}

/* int fputc(int character, FILE *stream); */
static void native_fputc(Script_t *restrict script, Param_t params[], const uint32_t argc)
{
	int character = params[0].Int32;//TaghaScript_pop_int32(script);
	FILE *stream = params[1].Pointer;//(FILE *)(uintptr_t)TaghaScript_pop_int64(script);
	TaghaScript_push_int32(script, fputc(character, stream));
}

/* int fputs(const char *str, FILE *stream); */
static void native_fputs(Script_t *restrict script, Param_t params[], const uint32_t argc)
{
	const char *str = params[0].String;//(const char *)(uintptr_t)TaghaScript_pop_int64(script);
	FILE *stream = params[1].Pointer;//(FILE *)(uintptr_t)TaghaScript_pop_int64(script);
	TaghaScript_push_int32(script, fputs(str, stream));
}

/* int getc(FILE *stream); */
static void native_getc(Script_t *restrict script, Param_t params[], const uint32_t argc)
{
	FILE *stream = params[0].Pointer;//(FILE *)(uintptr_t)TaghaScript_pop_int64(script);
	TaghaScript_push_int32(script, getc(stream));
}

/* int getchar(void); */
static void native_getchar(Script_t *restrict script, Param_t params[], const uint32_t argc)
{
	TaghaScript_push_int32(script, getchar());
}

/* int putc(int character, FILE *stream); */
static void native_putc(Script_t *restrict script, Param_t params[], const uint32_t argc)
{
	int character = params[0].Int32;//TaghaScript_pop_int32(script);
	FILE *stream = params[1].Pointer;//(FILE *)(uintptr_t)TaghaScript_pop_int64(script);
	TaghaScript_push_int32(script, putc(character, stream));
}

/* int putchar(int character); */
static void native_putchar(Script_t *restrict script, Param_t params[], const uint32_t argc)
{
	int character = params[0].Int32;//TaghaScript_pop_int32(script);
	TaghaScript_push_int32(script, putchar(character));
}

/* int ungetc(int character, FILE *stream); */
static void native_ungetc(Script_t *restrict script, Param_t params[], const uint32_t argc)
{
	int character = params[0].Int32;//TaghaScript_pop_int32(script);
	FILE *stream = params[1].Pointer;//(FILE *)(uintptr_t)TaghaScript_pop_int64(script);
	TaghaScript_push_int32(script, ungetc(character, stream));
}

/* size_t fread(void *ptr, size_t size, size_t count, FILE *stream); */
static void native_fread(Script_t *restrict script, Param_t params[], const uint32_t argc)
{
	void *ptr = params[0].Pointer;//(void *)(uintptr_t)TaghaScript_pop_int64(script);
	uint64_t size = params[1].UInt64;//TaghaScript_pop_int64(script);
	uint64_t count = params[2].UInt64;//TaghaScript_pop_int64(script);
	FILE *stream = params[3].Pointer;//(FILE *)(uintptr_t)TaghaScript_pop_int64(script);
	TaghaScript_push_int64(script, fread(ptr, size, count, stream));
}

/* size_t fwrite(const void *ptr, size_t size, size_t count, FILE *stream); */
static void native_fwrite(Script_t *restrict script, Param_t params[], const uint32_t argc)
{
	const void *ptr = params[0].Pointer;//(const void *)(uintptr_t)TaghaScript_pop_int64(script);
	uint64_t size = params[1].UInt64;//TaghaScript_pop_int64(script);
	uint64_t count = params[2].UInt64;//TaghaScript_pop_int64(script);
	FILE *stream = params[3].Pointer;//(FILE *)(uintptr_t)TaghaScript_pop_int64(script);
	TaghaScript_push_int64(script, fwrite(ptr, size, count, stream));
}

/* int fseek(FILE *stream, long int offset, int origin); */
static void native_fseek(Script_t *restrict script, Param_t params[], const uint32_t argc)
{
	FILE *stream = params[0].Pointer;//(FILE *)(uintptr_t)TaghaScript_pop_int64(script);
	uint64_t offset = params[1].UInt64;//TaghaScript_pop_int64(script);
	uint32_t origin = params[2].UInt32;//TaghaScript_pop_int32(script);
	TaghaScript_push_int32(script, fseek(stream, offset, origin));
}

/* long int ftell(FILE *stream); */
static void native_ftell(Script_t *restrict script, Param_t params[], const uint32_t argc)
{
	FILE *stream = params[0].Pointer;//(FILE *)(uintptr_t)TaghaScript_pop_int64(script);
	TaghaScript_push_int64(script, (uint64_t)ftell(stream));
}

/* void rewind(FILE *stream); */
static void native_rewind(Script_t *restrict script, Param_t params[], const uint32_t argc)
{
	FILE *stream = params[0].Pointer;//(FILE *)(uintptr_t)TaghaScript_pop_int64(script);
	rewind(stream);
}

/* void clearerr(FILE *stream); */
static void native_clearerr(Script_t *restrict script, Param_t params[], const uint32_t argc)
{
	FILE *stream = params[0].Pointer;//(FILE *)(uintptr_t)TaghaScript_pop_int64(script);
	clearerr(stream);
}

/* int feof(FILE *stream); */
static void native_feof(Script_t *restrict script, Param_t params[], const uint32_t argc)
{
	FILE *stream = params[0].Pointer;//(FILE *)(uintptr_t)TaghaScript_pop_int64(script);
	TaghaScript_push_int32(script, feof(stream));
}

/* int ferror(FILE *stream); */
static void native_ferror(Script_t *restrict script, Param_t params[], const uint32_t argc)
{
	FILE *stream = params[0].Pointer;//(FILE *)(uintptr_t)TaghaScript_pop_int64(script);
	TaghaScript_push_int32(script, ferror(stream));
}

/* void perror(const char *str); */
static void native_perror(Script_t *restrict script, Param_t params[], const uint32_t argc)
{
	const char *str = params[0].String;//(const char *)(uintptr_t)TaghaScript_pop_int64(script);
	perror(str);
}



void Tagha_load_stdio_natives(struct TaghaVM *vm)
{
	if( !vm )
		return;
	
	NativeInfo_t libc_stdio_natives[] = {
		{"get_stdout", native_get_stdout},
		{"get_stdin", native_get_stdin},
		{"get_stderr", native_get_stderr},
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
	Tagha_register_natives(vm, libc_stdio_natives);
}











