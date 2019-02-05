#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tagha_libc.h"


int32_t _tagha_gen_printf(const char *fmt, struct HarbolString *buffer, size_t currarg, union TaghaVal params[], size_t paramsize);

/*
 * File Ops
 */

/* int remove(const char *filename); */
static void native_remove(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->Int32 = remove(params[0].Ptr);
}

/* int rename(const char *oldname, const char *newname); */
static void native_rename(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->Int32 = rename(params[0].Ptr, params[1].Ptr);
}

/* FILE *tmpfile(void); */
static void native_tmpfile(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args; (void)params;
	retval->Ptr = tmpfile();
}

/* char *tmpnam(char *str); */
/* tmpnam is dangerous to use.
static void native_tmpnam(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->Ptr = tmpnam(params[0].Ptr);
}
*/


/*
 * File Access
 */

/* int fclose(FILE *stream); */
static void native_fclose(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	FILE *const f = params[0].Ptr;
	retval->Int32 = f ? fclose(f) : -1;
}

/* int fflush(FILE *stream); */
static void native_fflush(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->Int32 = fflush(params[0].Ptr);
}

/* FILE *fopen(const char *filename, const char *modes); */
static void native_fopen(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	const char *restrict mode = params[1].Ptr;
	if( !mode )
		return;
	else retval->Ptr = fopen(params[0].Ptr, mode);
}

/* FILE *freopen(const char *filename, const char *mode, FILE *stream); */
static void native_freopen(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	const char *restrict filename = params[0].Ptr;
	const char *restrict mode = params[1].Ptr;
	FILE *const restrict stream = params[2].Ptr;
	
	if( !filename || !mode || !stream )
		return; /* retval data is already zeroed out. */
	else retval->Ptr = freopen(filename, mode, stream);
}

/* void setbuf(FILE *stream, char *buffer); */
static void native_setbuf(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args; (void)retval;
	FILE *const stream = params[0].Ptr;
	if( !stream )
		return;
	else setbuf(stream, params[1].Ptr);
}


/* int fprintf(FILE *stream, const char *format, ...); */
static void native_fprintf(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	FILE *const restrict stream = params[0].Ptr;
	const char *restrict fmt = params[1].Ptr;
	if( !stream || !fmt ) {
		retval->Int32 = -1;
		return;
	} else {
		struct HarbolString buf = {NULL,0};
		_tagha_gen_printf(fmt, &buf, 2, params, args);
		retval->Int32 = fprintf(stream, "%s", buf.CStr);
		harbol_string_del(&buf);
	}
}

/* int printf(const char *fmt, ...); */
static void native_printf(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	const char *restrict fmt = params[0].Ptr;
	if( !fmt ) {
		retval->Int32 = -1;
		return;
	} else {
		struct HarbolString buf = {NULL,0};
		_tagha_gen_printf(fmt, &buf, 1, params, args);
		retval->Int32 = printf("%s", buf.CStr);
		harbol_string_del(&buf);
	}
}

/* int sprintf(char *str, const char *format, ...); */
static void native_sprintf(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	char *restrict str = params[0].Ptr;
	const char *restrict fmt = params[1].Ptr;
	if( !str || !fmt ) {
		retval->Int32 = -1;
		return;
	} else {
		struct HarbolString buf = {NULL,0};
		_tagha_gen_printf(fmt, &buf, 2, params, args);
		retval->Int32 = sprintf(str, "%s", buf.CStr);
		harbol_string_del(&buf);
	}
}

/* int snprintf(char *s, size_t n, const char *format, ...); */
static void native_snprintf(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	char *restrict str = params[0].Ptr;
	const char *restrict fmt = params[2].Ptr;
	if( !str || !fmt ) {
		retval->Int32 = -1;
		return;
	} else {
		struct HarbolString buf = {NULL,0};
		_tagha_gen_printf(fmt, &buf, 3, params, args);
		retval->Int32 = snprintf(str, params[1].UInt64, "%s", buf.CStr);
		harbol_string_del(&buf);
	}
}

/* int vprintf(const char *fmt, va_list arg); */
static void native_vprintf(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	const char *restrict fmt = params[0].Ptr;
	if( !fmt ) {
		retval->Int32 = -1;
		return;
	} else {
		const struct Tagha_va_list *const restrict valist = params[1].Ptr;
		struct HarbolString buf = {NULL,0};
		_tagha_gen_printf(fmt, &buf, 0, valist->Area.PtrSelf, valist->Args);
		retval->Int32 = printf("%s", buf.CStr);
		harbol_string_del(&buf);
	}
}

/* int vfprintf(FILE *stream, const char *format, va_list arg); */
static void native_vfprintf(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	FILE *const restrict stream = params[0].Ptr;
	const char *restrict fmt = params[1].Ptr;
	if( !stream || !fmt ) {
		retval->Int32 = -1;
		return;
	} else {
		const struct Tagha_va_list *const valist = params[2].Ptr;
		struct HarbolString buf = {NULL,0};
		_tagha_gen_printf(fmt, &buf, 0, valist->Area.PtrSelf, valist->Args);
		retval->Int32 = fprintf(stream, "%s", buf.CStr);
		harbol_string_del(&buf);
	}
}

/* int vsprintf(char *s, const char *format, va_list arg); */
static void native_vsprintf(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	char *restrict str = params[0].Ptr;
	const char *restrict fmt = params[1].Ptr;
	if( !str || !fmt ) {
		retval->Int32 = -1;
		return;
	} else {
		const struct Tagha_va_list *const valist = params[2].Ptr;
		struct HarbolString buf = {NULL,0};
		_tagha_gen_printf(fmt, &buf, 0, valist->Area.PtrSelf, valist->Args);
		retval->Int32 = sprintf(str, "%s", buf.CStr);
		harbol_string_del(&buf);
	}
}

/* int vsnprintf(char *s, size_t n, const char *format, va_list arg); */
static void native_vsnprintf(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	char *restrict str = params[0].Ptr;
	const char *restrict fmt = params[2].Ptr;
	if( !str || !fmt ) {
		retval->Int32 = -1;
		return;
	} else {
		const struct Tagha_va_list *const restrict valist = params[3].Ptr;
		struct HarbolString buf = {NULL,0};
		_tagha_gen_printf(fmt, &buf, 0, valist->Area.PtrSelf, valist->Args);
		retval->Int32 = snprintf(str, params[1].UInt64, "%s", buf.CStr);
		harbol_string_del(&buf);
	}
}


/* int fscanf(FILE *stream, const char *format, ...); */
static void native_fscanf(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	FILE *const restrict stream = params[0].Ptr;
	const char *restrict fmt = params[1].Ptr;
	if( !stream || !fmt ) {
		retval->Int32 = -1;
		return;
	} else {
		retval->Int32 = -1;
	}
}

/* int puts(const char *s); */
static void native_puts(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->Int32 = puts(params[0].PtrConstChar);
}

/* int setvbuf(FILE *stream, char *buffer, int mode, size_t size); */
static void native_setvbuf(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	FILE *const restrict stream = params[0].Ptr;
	char *restrict buffer = params[1].Ptr;
	if( !stream || !buffer ) {
		retval->Int32 = -1;
		return;
	}
	else retval->Int32 = setvbuf(stream, buffer, params[2].Int32, params[3].UInt64);
}

/* int fgetc(FILE *stream); */
static void native_fgetc(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->Int32 = fgetc(params[0].Ptr);
}

/* char *fgets(char *str, int num, FILE *stream); */
static void native_fgets(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->Ptr = fgets(params[0].Ptr, params[1].Int32, params[2].Ptr);
}

/* int fputc(int character, FILE *stream); */
static void native_fputc(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->Int32 = fputc(params[0].Int32, params[1].Ptr);
}

/* int fputs(const char *str, FILE *stream); */
static void native_fputs(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->Int32 = fputs(params[0].Ptr, params[1].Ptr);
}

/* int getc(FILE *stream); */
static void native_getc(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->Int32 = getc(params[0].Ptr);
}

/* int getchar(void); */
static void native_getchar(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args; (void)params;
	retval->Int32 = getchar();
}

/* int putc(int character, FILE *stream); */
static void native_putc(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->Int32 = putc(params[0].Int32, params[1].Ptr);
}

/* int putchar(int character); */
static void native_putchar(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->Int32 = putchar(params[0].Int32);
}

/* int ungetc(int character, FILE *stream); */
static void native_ungetc(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->Int32 = ungetc(params[0].Int32, params[1].Ptr);
}

/* size_t fread(void *ptr, size_t size, size_t count, FILE *stream); */
static void native_fread(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->UInt64 = fread(params[0].Ptr, params[1].UInt64, params[2].UInt64, params[3].Ptr);
}

/* size_t fwrite(const void *ptr, size_t size, size_t count, FILE *stream); */
static void native_fwrite(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->UInt64 = fwrite(params[0].Ptr, params[1].UInt64, params[2].UInt64, params[3].Ptr);
}

/* int fseek(FILE *stream, long int offset, int origin); */
static void native_fseek(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->Int32 = fseek(params[0].Ptr, params[1].Int64, params[2].Int32);
}

/* long int ftell(FILE *stream); */
static void native_ftell(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->Int64 = ftell(params[0].Ptr);
}

/* void rewind(FILE *stream); */
static void native_rewind(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args; (void)retval;
	rewind(params[0].Ptr);
}

/* void clearerr(FILE *stream); */
static void native_clearerr(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args; (void)retval;
	clearerr(params[0].Ptr);
}

/* int feof(FILE *stream); */
static void native_feof(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->Int32 = feof(params[0].Ptr);
}

/* int ferror(FILE *stream); */
static void native_ferror(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args;
	retval->Int32 = ferror(params[0].Ptr);
}

/* void perror(const char *str); */
static void native_perror(struct TaghaModule *const module, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)module; (void)args; (void)retval;
	perror(params[0].Ptr);
}


bool tagha_module_load_stdio_natives(struct TaghaModule *const module)
{
	const struct TaghaNative libc_stdio_natives[] = {
		{"remove", native_remove},
		{"rename", native_rename},
		{"tmpfile", native_tmpfile},
		/*{"tmpnam", native_tmpnam},*/
		{"fprintf", native_fprintf},
		{"printf", native_printf},
		{"sprintf", native_sprintf},
		{"snprintf", native_snprintf},
		{"vprintf", native_vprintf},
		{"vfprintf", native_vfprintf},
		{"vsprintf", native_vsprintf},
		{"vsnprintf", native_vsnprintf},
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
	return module ? tagha_module_register_natives(module, libc_stdio_natives) : false;
}


int32_t _tagha_printf_write_decimal_int(struct HarbolString *const buffer, const union TaghaVal value, const bool is64bit)
{
	char buf[30u]={0};
	const int32_t chars_written = is64bit ? sprintf(buf, "%" PRIi64 "", value.Int64) : sprintf(buf, "%" PRIi32 "", value.Int32);
	buf[29]=0;
	harbol_string_add_cstr(buffer, buf);
	return chars_written;
}

int32_t _tagha_printf_write_decimal_uint(struct HarbolString *const buffer, const union TaghaVal value, const bool is64bit)
{
	char buf[30u]={0};
	const int32_t chars_written = is64bit ? sprintf(buf, "%" PRIu64 "", value.UInt64) : sprintf(buf, "%" PRIu32 "", value.UInt32);
	buf[29]=0;
	harbol_string_add_cstr(buffer, buf);
	return chars_written;
}

int32_t _tagha_printf_write_hex(struct HarbolString *const buffer, const union TaghaVal value, const bool is64bit, const bool capital_letters)
{
	char buf[30u]={0};
	const int32_t chars_written = is64bit
		? sprintf(buf, capital_letters ? "%" PRIX64 "" : "%" PRIx64 "", value.UInt64)
		: sprintf(buf, capital_letters ? "%" PRIX32 "" : "%" PRIx32 "", value.UInt32);
	buf[29]=0;
	harbol_string_add_cstr(buffer, buf);
	return chars_written;
}

int32_t _tagha_printf_write_octal(struct HarbolString *const buffer, const union TaghaVal value, const bool is64bit)
{
	char buf[30u]={0};
	const int32_t chars_written = is64bit
		? sprintf(buf, "%" PRIo64 "", value.UInt64)
		: sprintf(buf, "%" PRIo32 "", value.UInt32);
	buf[29]=0;
	harbol_string_add_cstr(buffer, buf);
	return chars_written;
}

int32_t _tagha_printf_write_float(struct HarbolString *const buffer, const union TaghaVal value, const bool is64bit)
{
	char buf[30u]={0};
	const int32_t chars_written = is64bit ? sprintf(buf, "%f", value.Double) : sprintf(buf, "%f", value.Float);
	buf[29]=0;
	harbol_string_add_cstr(buffer, buf);
	return chars_written;
}


int32_t _tagha_gen_printf(const char *restrict fmt, struct HarbolString *const restrict buffer, size_t currarg, union TaghaVal params[restrict], const size_t paramsize)
{
#	define FLAG_LONG	1
#	define FLAG_FLTLONG	2
	int32_t chars_written = 0;
	for( ; *fmt ; fmt++ ) {
		if( currarg>paramsize )
			break;
		uint_least8_t flags = 0;
		if( *fmt=='%' ) {
printf_loop_restart:
			fmt++;
			switch( *fmt ) {
				case 'l':
					flags |= FLAG_LONG;
					/* jumping from here so we can increment the char ptr for additional reading. */
					goto printf_loop_restart;
				case 'L':
					flags |= FLAG_FLTLONG;
					goto printf_loop_restart;
				/* print int */
				case 'i': case 'd':
					chars_written += _tagha_printf_write_decimal_int(buffer, params[currarg++], flags & FLAG_LONG);
					break;
				/* print uint */
				case 'u':
					chars_written += _tagha_printf_write_decimal_uint(buffer, params[currarg++], flags & FLAG_LONG);
					break;
				/* print hex */
				case 'x': case 'X':
					chars_written += _tagha_printf_write_hex(buffer, params[currarg++], flags & FLAG_LONG, *fmt=='X');
					break;
				/* print octal */
				case 'o':
					chars_written += _tagha_printf_write_octal(buffer, params[currarg++], flags & FLAG_LONG);
					break;
				/* print ptr */
				case 'p': case 'P':
					chars_written += _tagha_printf_write_hex(buffer, params[currarg++], true, *fmt=='P');
					break;
				/* print double */
				case 'f': case 'F':
					chars_written += _tagha_printf_write_float(buffer, params[currarg++], true);
					break;
				case 's': {
					const char *s = params[currarg++].Ptr;
					const size_t slen = strlen(s);
					harbol_string_add_cstr(buffer, s);
					chars_written += slen;
					break;
				}
				case '%':
					chars_written++;
					harbol_string_add_char(buffer, '%');
					break;
			}
		} else if( *fmt=='\\' ) {
			fmt++;
			switch( *fmt ) {
				case 'a': harbol_string_add_char(buffer, '\a'); chars_written++; break;
				case 'r': harbol_string_add_char(buffer, '\r'); chars_written++; break;
				case 'b': harbol_string_add_char(buffer, '\b'); chars_written++; break;
				case 't': harbol_string_add_char(buffer, '\t'); chars_written++; break;
				case 'v': harbol_string_add_char(buffer, '\v'); chars_written++; break;
				case 'n': harbol_string_add_char(buffer, '\n'); chars_written++; break;
				case 'f': harbol_string_add_char(buffer, '\f'); chars_written++; break;
				case '0': harbol_string_add_char(buffer, '\0'); chars_written++; break;
			}
		} else {
			chars_written++;
			harbol_string_add_char(buffer, *fmt);
		}
	}
	return chars_written;
}

