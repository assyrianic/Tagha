
#include <stdio.h>
#include <stdint.h>


/* FILE *get_stdout(void); */
static void native_get_stdout(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	TaghaScript_push_int64( script, (uintptr_t)stdout );
}

/* FILE *get_stdin(void); */
static void native_get_stdin(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	TaghaScript_push_int64( script, (uintptr_t)stdin );
}

/* FILE *get_stderr(void); */
static void native_get_stderr(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	TaghaScript_push_int64( script, (uintptr_t)stderr );
}

/*
 * File Ops
 */

/* int remove(const char *filename); */
static void native_remove(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	const char *filename = (const char *)(uintptr_t)TaghaScript_pop_int64(script);
	TaghaScript_push_int32( script, remove(filename) );
}

/* int rename(const char *oldname, const char *newname); */
static void native_rename(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	const char *filename = (const char *)(uintptr_t)TaghaScript_pop_int64(script);
	const char *newname = (const char *)(uintptr_t)TaghaScript_pop_int64(script);
	TaghaScript_push_int32( script, rename(filename, newname) );
}

/* FILE *tmpfile(void); */
static void native_tmpfile(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	FILE *pTemp = tmpfile();
	TaghaScript_push_int64( script, (uintptr_t)pTemp );
}

/* char *tmpnam(char *str); */
static void native_tmpnam(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	char *str = (char *)(uintptr_t)TaghaScript_pop_int64(script);
	TaghaScript_push_int64( script, (uintptr_t)tmpnam(str) );
}


/*
 * File Access
 */

/* int fclose(FILE *stream); */
static void native_fclose(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	if( !script )
		return;
	
	FILE *pFile = (FILE *)(uintptr_t) TaghaScript_pop_int64(script);
	if( pFile ) {
		printf("fclose:: closing FILE*\n");
		TaghaScript_push_int32(script, fclose(pFile));
		pFile=NULL;
		// erases the dangling reference from host data.
		//TaghaScript_del_hostdata(script, addr);
	}
	else {
		printf("fclose:: FILE* is NULL\n");
		TaghaScript_push_int32(script, -1);
	}
}

/* int fflush(FILE *stream); */
static void native_fflush(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	if( !script )
		return;
	
	FILE *pStream = (FILE *)(uintptr_t) TaghaScript_pop_int64(script);
	TaghaScript_push_int32(script, fflush(pStream));
	pStream = NULL;
}

/* FILE *fopen(const char *filename, const char *modes); */
static void native_fopen(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	if( !script )
		return;
	
	const char *filename = (const char *)(uintptr_t)TaghaScript_pop_int64(script);
	const char *mode = (const char *)(uintptr_t)TaghaScript_pop_int64(script);
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
static void native_freopen(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	if( !script )
		return;
	
	const char *filename = (const char *)(uintptr_t)TaghaScript_pop_int64(script);
	const char *mode = (const char *)(uintptr_t)TaghaScript_pop_int64(script);
	FILE *pStream = (FILE *)(uintptr_t)TaghaScript_pop_int64(script);
	
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
static void native_setbuf(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	if( !script )
		return;
	
	FILE *pStream = (FILE *)(uintptr_t) TaghaScript_pop_int64(script);
	char *pBuffer = (char *)(uintptr_t) TaghaScript_pop_int64(script);
	if( !pStream ) {
		puts("setbuf reported an ERROR :: **** param 'stream' is NULL ****\n");
		return;
	}
	setbuf(pStream, pBuffer);
}

/* int printf(const char *fmt, ...); */
static void native_printf(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	const char *str = (const char *)(uintptr_t)TaghaScript_pop_int64(script);
	
	if( !str ) {
		TaghaScript_push_int32(script, -1);
		puts("native_printf reported an ERROR :: **** param 'fmt' is NULL ****\n");
		return;
	}
	
	char *iter=(char *)str;
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
					chrs += sprintf(data_buffer, "%f", TaghaScript_pop_float64(script));
					printf("%s", data_buffer);
					break;
					
				case 'e':
				case 'E':
					chrs += sprintf(data_buffer, "%e", TaghaScript_pop_float64(script));
					printf("%s", data_buffer);
					break;
					
				case 'a':
				case 'A':
					chrs += sprintf(data_buffer, "%a", TaghaScript_pop_float64(script));
					printf("%s", data_buffer);
					break;
					
				case 'i':
				case 'd':
					chrs += sprintf(data_buffer, "%i", (int32_t)TaghaScript_pop_int32(script));
					printf("%s", data_buffer);
					break;
					
				case 'u':
					chrs += sprintf(data_buffer, "%u", TaghaScript_pop_int32(script));
					printf("%s", data_buffer);
					break;
					
				case 'x':
				case 'X':
					chrs += sprintf(data_buffer, "%x", TaghaScript_pop_int32(script));
					printf("%s", data_buffer);
					break;
					
				case 'o':
					chrs += sprintf(data_buffer, "%o", TaghaScript_pop_int32(script));
					printf("%s", data_buffer);
					break;
					
				case 'c':
					chrs += sprintf(data_buffer, "%c", TaghaScript_pop_byte(script));
					printf("%s", data_buffer);
					break;
					
				case 'p':
					chrs += sprintf(data_buffer, "%p", (void *)(uintptr_t)TaghaScript_pop_int64(script));
					printf("%s", data_buffer);
					break;
					
				case 's':
					chrs += sprintf(data_buffer, "%s", (char *)(uintptr_t)TaghaScript_pop_int64(script));
					printf("%s", data_buffer);
					break;
					
				default:
					printf("invalid format\n");
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
static void native_puts(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	const char *str = (const char *)(uintptr_t)TaghaScript_pop_int64(script);
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
static void native_setvbuf(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	FILE *stream = (FILE *)(uintptr_t)TaghaScript_pop_int64(script);
	char *buffer = (char *)(uintptr_t)TaghaScript_pop_int64(script);
	int32_t mode = TaghaScript_pop_int32(script);
	uint64_t size = TaghaScript_pop_int64(script);
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
static void native_fgetc(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	FILE *stream = (FILE *)(uintptr_t)TaghaScript_pop_int64(script);
	TaghaScript_push_int32(script, fgetc(stream));
}

/* char *fgets(char *str, int num, FILE *stream); */
static void native_fgets(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	char *str = (char *)(uintptr_t)TaghaScript_pop_int64(script);
	int num = TaghaScript_pop_int32(script);
	FILE *stream = (FILE *)(uintptr_t)TaghaScript_pop_int64(script);
	TaghaScript_push_int64(script, (uintptr_t)fgets(str, num, stream));
}

/* int fputc(int character, FILE *stream); */
static void native_fputc(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	int character = TaghaScript_pop_int32(script);
	FILE *stream = (FILE *)(uintptr_t)TaghaScript_pop_int64(script);
	TaghaScript_push_int32(script, fputc(character, stream));
}

/* int fputs(const char *str, FILE *stream); */
static void native_fputs(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	const char *str = (const char *)(uintptr_t)TaghaScript_pop_int64(script);
	FILE *stream = (FILE *)(uintptr_t)TaghaScript_pop_int64(script);
	TaghaScript_push_int32(script, fputs(str, stream));
}

/* int getc(FILE *stream); */
static void native_getc(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	FILE *stream = (FILE *)(uintptr_t)TaghaScript_pop_int64(script);
	TaghaScript_push_int32(script, getc(stream));
}

/* int getchar(void); */
static void native_getchar(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	TaghaScript_push_int32(script, getchar());
}

/* int putc(int character, FILE *stream); */
static void native_putc(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	int character = TaghaScript_pop_int32(script);
	FILE *stream = (FILE *)(uintptr_t)TaghaScript_pop_int64(script);
	TaghaScript_push_int32(script, putc(character, stream));
}

/* int putchar(int character); */
static void native_putchar(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	int character = TaghaScript_pop_int32(script);
	TaghaScript_push_int32(script, putchar(character));
}

/* int ungetc(int character, FILE *stream); */
static void native_ungetc(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	int character = TaghaScript_pop_int32(script);
	FILE *stream = (FILE *)(uintptr_t)TaghaScript_pop_int64(script);
	TaghaScript_push_int32(script, ungetc(character, stream));
}

/* size_t fread(void *ptr, size_t size, size_t count, FILE *stream); */
static void native_fread(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	void *ptr = (void *)(uintptr_t)TaghaScript_pop_int64(script);
	uint64_t size = TaghaScript_pop_int64(script);
	uint64_t count = TaghaScript_pop_int64(script);
	FILE *stream = (FILE *)(uintptr_t)TaghaScript_pop_int64(script);
	TaghaScript_push_int64(script, fread(ptr, size, count, stream));
}

/* size_t fwrite(const void *ptr, size_t size, size_t count, FILE *stream); */
static void native_fwrite(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	const void *ptr = (const void *)(uintptr_t)TaghaScript_pop_int64(script);
	uint64_t size = TaghaScript_pop_int64(script);
	uint64_t count = TaghaScript_pop_int64(script);
	FILE *stream = (FILE *)(uintptr_t)TaghaScript_pop_int64(script);
	TaghaScript_push_int64(script, fwrite(ptr, size, count, stream));
}

/* int fseek(FILE *stream, long int offset, int origin); */
static void native_fseek(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	FILE *stream = (FILE *)(uintptr_t)TaghaScript_pop_int64(script);
	uint64_t offset = TaghaScript_pop_int64(script);
	uint32_t origin = TaghaScript_pop_int32(script);
	TaghaScript_push_int32(script, fseek(stream, offset, origin));
}

/* long int ftell(FILE *stream); */
static void native_ftell(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	FILE *stream = (FILE *)(uintptr_t)TaghaScript_pop_int64(script);
	TaghaScript_push_int32(script, ftell(stream));
}

/* void rewind(FILE *stream); */
static void native_rewind(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	FILE *stream = (FILE *)(uintptr_t)TaghaScript_pop_int64(script);
	rewind(stream);
}

/* void clearerr(FILE *stream); */
static void native_clearerr(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	FILE *stream = (FILE *)(uintptr_t)TaghaScript_pop_int64(script);
	clearerr(stream);
}

/* int feof(FILE *stream); */
static void native_feof(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	FILE *stream = (FILE *)(uintptr_t)TaghaScript_pop_int64(script);
	TaghaScript_push_int32(script, feof(stream));
}

/* int ferror(FILE *stream); */
static void native_ferror(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	FILE *stream = (FILE *)(uintptr_t)TaghaScript_pop_int64(script);
	TaghaScript_push_int32(script, ferror(stream));
}

/* void perror(const char *str); */
static void native_perror(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	const char *str = (const char *)(uintptr_t)TaghaScript_pop_int64(script);
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











