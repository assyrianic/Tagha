
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * File Ops
 */

/* int remove(const char *filename); */
static void native_remove(struct Tagha *const restrict sys, union Value *const restrict RetVal, const size_t argc, union Value params[restrict static argc])
{
	RetVal->Int32 = remove(params[0].Ptr);
}

/* int rename(const char *oldname, const char *newname); */
static void native_rename(struct Tagha *const restrict sys, union Value *const restrict RetVal, const size_t argc, union Value params[restrict static argc])
{
	RetVal->Int32 = rename(params[0].Ptr, params[1].Ptr);
}

/* FILE *tmpfile(void); */
static void native_tmpfile(struct Tagha *const restrict sys, union Value *const restrict RetVal, const size_t argc, union Value params[restrict static argc])
{
	RetVal->Ptr = tmpfile();
}

/* char *tmpnam(char *str); */
static void native_tmpnam(struct Tagha *const restrict sys, union Value *const restrict RetVal, const size_t argc, union Value params[restrict static argc])
{
	RetVal->Ptr = tmpnam(params[0].Ptr);
}


/*
 * File Access
 */

/* int fclose(FILE *stream); */
static void native_fclose(struct Tagha *const restrict sys, union Value *const restrict RetVal, const size_t argc, union Value params[restrict static argc])
{
	if( params[0].Ptr ) {
		RetVal->Int32 = fclose(params[0].Ptr);
		return;
	}
	RetVal->Int32 = -1;
}

/* int fflush(FILE *stream); */
static void native_fflush(struct Tagha *const restrict sys, union Value *const restrict RetVal, const size_t argc, union Value params[restrict static argc])
{
	RetVal->Int32 = fflush(params[0].Ptr);
}

/* FILE *fopen(const char *filename, const char *modes); */
static void native_fopen(struct Tagha *const restrict sys, union Value *const restrict RetVal, const size_t argc, union Value params[restrict static argc])
{
	const char *mode = params[1].Ptr;
	if( !mode ) {
		RetVal->Ptr = NULL;
		return;
	}
	RetVal->Ptr = fopen(params[0].Ptr, mode);
}

/* FILE *freopen(const char *filename, const char *mode, FILE *stream); */
static void native_freopen(struct Tagha *const restrict sys, union Value *const restrict RetVal, const size_t argc, union Value params[restrict static argc])
{
	const char *filename = params[0].Ptr;
	const char *mode = params[1].Ptr;
	FILE *pStream = params[2].Ptr;
	
	if( !filename or !mode or !pStream )
		goto error;
	
	RetVal->Ptr = freopen(filename, mode, pStream);
	return;
error:;
	RetVal->Ptr = NULL;
}

/* void setbuf(FILE *stream, char *buffer); */
static void native_setbuf(struct Tagha *const restrict sys, union Value *const restrict RetVal, const size_t argc, union Value params[restrict static argc])
{
	FILE *pStream = params[0].Ptr;
	if( !pStream ) {
		return;
	}
	setbuf(pStream, params[1].Ptr);
}


/*
 * File Access
 */

int32_t gnprintf(char *buffer, size_t maxlen, const char *format, union Value params[], size_t numparams, size_t *curparam);

/* int fprintf(FILE *stream, const char *format, ...); */
static void native_fprintf(struct Tagha *const restrict sys, union Value *const restrict RetVal, const size_t argc, union Value params[restrict static argc])
{
	FILE *stream = params[0].Ptr;
	if( !stream ) {
		RetVal->Int32 = -1;
		return;
	}
	
	const char *format = params[1].Ptr;
	if( !format ) {
		RetVal->Int32 = -1;
		return;
	}
	
	char data_buffer[1024] = {0};
	uint32_t param = 2;
	gnprintf(data_buffer, 1024, format, params, argc-2, &param);
	data_buffer[1023] = 0;
	RetVal->Int32 = fprintf(stream, "%s", data_buffer);
}

/* int fscanf(FILE *stream, const char *format, ...); */
static void native_fscanf(struct Tagha *const restrict sys, union Value *const restrict RetVal, const size_t argc, union Value params[restrict static argc])
{
	FILE *stream = params[0].Ptr;
	if( !stream ) {
		RetVal->Int32 = -1;
		return;
	}
	
	const char *fmt = params[1].Ptr;
	if( !fmt ) {
		RetVal->Int32 = -1;
		return;
	}
	// flags
#define LONGADJ  0x00000001  /* adjust for longer values like "lli" */
#define LONGDBLADJ  0x00000002  /* adjust for long double like "Lf" */
#define HALFADJ  0x00000004  /* adjust for long double like "Lf" */
#define HALFHALFADJ  0x00000008  /* adjust for long double like "Lf" */
	
	// %[*][width][length]specifier
	size_t width=0;
	
	for( ; *fmt ; fmt++ ) {
		if( *fmt != '%' )
			continue;
		// found a width, parse its number.
		if( *fmt>='0' and *fmt<='9' )
			while( *fmt>='0' and *fmt<='9' )
				width = width * 10 + *fmt++ - '0';
		
		if( *fmt=='d' or *fmt=='i' ) {
			
		}
		else if( *fmt=='u' or *fmt=='x' or *fmt=='o' ) {
			
		}
	}
	
	RetVal->Int32 = -1;
	// Need some help with this one...
}

/* int printf(const char *fmt, ...); */
static void native_printf(struct Tagha *const restrict sys, union Value *const restrict RetVal, const size_t argc, union Value params[restrict static argc])
{
	const char *str = params[0].Ptr;
	if( !str ) {
		RetVal->Int32 = -1;
		return;
	}
	char data_buffer[4096] = {0};
	const size_t param = 1;
	RetVal->Int32 = gnprintf(data_buffer, 4096, str, params, argc-1, &param);
	data_buffer[4095] = 0;	// make sure we null terminator.
	printf("%s", data_buffer);
}

/* int puts(const char *s); */
static void native_puts(struct Tagha *const restrict sys, union Value *const restrict RetVal, const size_t argc, union Value params[restrict static argc])
{
	const char *str = params[0].Ptr;
	if( !str ) {
		RetVal->Int32 = -1;
		return;
	}
	// push back the value of the return val of puts.
	RetVal->Int32 = puts(str);
}

/* int setvbuf(FILE *stream, char *buffer, int mode, size_t size); */
static void native_setvbuf(struct Tagha *const restrict sys, union Value *const restrict RetVal, const size_t argc, union Value params[restrict static argc])
{
	FILE *stream = params[0].Ptr;
	char *buffer = params[1].Ptr;
	if( !stream ) {
		RetVal->Int32 = -1;
		return;
	}
	else if( !buffer ) {
		RetVal->Int32 = -1;
		return;
	}
	RetVal->Int32 = setvbuf(stream, buffer, params[2].Int32, params[3].UInt64);
}

/* int fgetc(FILE *stream); */
static void native_fgetc(struct Tagha *const restrict sys, union Value *const restrict RetVal, const size_t argc, union Value params[restrict static argc])
{
	RetVal->Int32 = fgetc(params[0].Ptr);
}

/* char *fgets(char *str, int num, FILE *stream); */
static void native_fgets(struct Tagha *const restrict sys, union Value *const restrict RetVal, const size_t argc, union Value params[restrict static argc])
{
	RetVal->Ptr = fgets(params[0].Ptr, params[1].Int32, params[2].Ptr);
}

/* int fputc(int character, FILE *stream); */
static void native_fputc(struct Tagha *const restrict sys, union Value *const restrict RetVal, const size_t argc, union Value params[restrict static argc])
{
	RetVal->Int32 = fputc(params[0].Int32, params[1].Ptr);
}

/* int fputs(const char *str, FILE *stream); */
static void native_fputs(struct Tagha *const restrict sys, union Value *const restrict RetVal, const size_t argc, union Value params[restrict static argc])
{
	RetVal->Int32 = fputs(params[0].Ptr, params[1].Ptr);
}

/* int getc(FILE *stream); */
static void native_getc(struct Tagha *const restrict sys, union Value *const restrict RetVal, const size_t argc, union Value params[restrict static argc])
{
	RetVal->Int32 = getc(params[0].Ptr);
}

/* int getchar(void); */
static void native_getchar(struct Tagha *const restrict sys, union Value *const restrict RetVal, const size_t argc, union Value params[restrict static argc])
{
	RetVal->Int32 = getchar();
}

/* int putc(int character, FILE *stream); */
static void native_putc(struct Tagha *const restrict sys, union Value *const restrict RetVal, const size_t argc, union Value params[restrict static argc])
{
	RetVal->Int32 = putc(params[0].Int32, params[1].Ptr);
}

/* int putchar(int character); */
static void native_putchar(struct Tagha *const restrict sys, union Value *const restrict RetVal, const size_t argc, union Value params[restrict static argc])
{
	RetVal->Int32 = putchar(params[0].Int32);
}

/* int ungetc(int character, FILE *stream); */
static void native_ungetc(struct Tagha *const restrict sys, union Value *const restrict RetVal, const size_t argc, union Value params[restrict static argc])
{
	RetVal->Int32 = ungetc(params[0].Int32, params[1].Ptr);
}

/* size_t fread(void *ptr, size_t size, size_t count, FILE *stream); */
static void native_fread(struct Tagha *const restrict sys, union Value *const restrict RetVal, const size_t argc, union Value params[restrict static argc])
{
	RetVal->UInt64 = fread(params[0].Ptr, params[1].UInt64, params[2].UInt64, params[3].Ptr);
}

/* size_t fwrite(const void *ptr, size_t size, size_t count, FILE *stream); */
static void native_fwrite(struct Tagha *const restrict sys, union Value *const restrict RetVal, const size_t argc, union Value params[restrict static argc])
{
	RetVal->UInt64 = fwrite(params[0].Ptr, params[1].UInt64, params[2].UInt64, params[3].Ptr);
}

/* int fseek(FILE *stream, long int offset, int origin); */
static void native_fseek(struct Tagha *const restrict sys, union Value *const restrict RetVal, const size_t argc, union Value params[restrict static argc])
{
	RetVal->Int32 = fseek(params[0].Ptr, params[1].UInt64, params[2].Int32);
}

/* long int ftell(FILE *stream); */
static void native_ftell(struct Tagha *const restrict sys, union Value *const restrict RetVal, const size_t argc, union Value params[restrict static argc])
{
	RetVal->Int64 = ftell(params[0].Ptr);
}

/* void rewind(FILE *stream); */
static void native_rewind(struct Tagha *const restrict sys, union Value *const restrict RetVal, const size_t argc, union Value params[restrict static argc])
{
	rewind(params[0].Ptr);
}

/* void clearerr(FILE *stream); */
static void native_clearerr(struct Tagha *const restrict sys, union Value *const restrict RetVal, const size_t argc, union Value params[restrict static argc])
{
	clearerr(params[0].Ptr);
}

/* int feof(FILE *stream); */
static void native_feof(struct Tagha *const restrict sys, union Value *const restrict RetVal, const size_t argc, union Value params[restrict static argc])
{
	RetVal->Int32 = feof(params[0].Ptr);
}

/* int ferror(FILE *stream); */
static void native_ferror(struct Tagha *const restrict sys, union Value *const restrict RetVal, const size_t argc, union Value params[restrict static argc])
{
	RetVal->Int32 = ferror(params[0].Ptr);
}

/* void perror(const char *str); */
static void native_perror(struct Tagha *const restrict sys, union Value *const restrict RetVal, const size_t argc, union Value params[restrict static argc])
{
	perror(params[0].Ptr);
}


// Code from SourceMod
// Copyright (C) 2004-2015 AlliedModders LLC.  All rights reserved.
// Under GNU General Public License, version 3.0
#define LADJUST			0x00000001		/* left adjustment */
#define ZEROPAD			0x00000002		/* zero (as opposed to blank) pad */
#define UPPERDIGITS		0x00000004		/* make alpha digits uppercase */
#define NOESCAPE		0x00000008		/* do not escape strings (they are only escaped if a database connection is provided) */
#define LONGADJ			0x00000010		/* adjusting for longer values like "lli", etc. Added by Assyrianic */

#define to_digit(c)		((c) - '0')
#define is_digit(c)		((uint32_t)to_digit(c) <= 9)

// minor edits is removing database string AND 'maxlen' is changed from a reference to a pointer.
static bool AddString(char **buf_p, size_t *maxlen, const char *string, int width, int prec, int flags)
{
	int size = 0;
	char *buf;
	static char nlstr[] = {'(','n','u','l','l',')','\0'};

	buf = *buf_p;

	if (string == NULL)
	{
		string = nlstr;
		prec = -1;
		flags |= NOESCAPE;
	}

	if (prec >= 0)
	{
		for (size = 0; size < prec; size++) 
		{
			if (string[size] == '\0')
			{
				break;
			}
		}
	}
	else
	{
		while (string[size++]);
		size--;
	}

	if (size > (int)*maxlen)
	{
		size = *maxlen;
	}

	width -= size;
	*maxlen -= size;

	while (size--)
	{
		*buf++ = *string++;
	}

	while ((width-- > 0) && *maxlen)
	{
		*buf++ = ' ';
		--*maxlen;
	}

	*buf_p = buf;
	return true;
}

#include <float.h>
#include <math.h>
void AddFloat(char **buf_p, size_t *maxlen, double fval, int width, int prec, int flags)
{
	int digits;					// non-fraction part digits
	double tmp;					// temporary
	char *buf = *buf_p;			// output buffer pointer
	int64_t val;				// temporary
	int sign = 0;				// 0: positive, 1: negative
	int fieldlength;			// for padding
	int significant_digits = 0;	// number of significant digits written
	const int MAX_SIGNIFICANT_DIGITS = 16;

	if (fval != fval)
	{
		AddString(buf_p, maxlen, "NaN", width, prec, flags | NOESCAPE);
		return;
	}

	// default precision
	if (prec < 0)
	{
		prec = 6;
	}

	// get the sign
	if (fval < 0)
	{
		fval = -fval;
		sign = 1;
	}

	// compute whole-part digits count
	digits = (int)log10(fval) + 1;

	// Only print 0.something if 0 < fval < 1
	if (digits < 1)
	{
		digits = 1;
	}

	// compute the field length
	fieldlength = digits + prec + ((prec > 0) ? 1 : 0) + sign;

	// minus sign BEFORE left padding if padding with zeros
	if (sign && *maxlen && (flags & ZEROPAD))
	{
		*buf++ = '-';
		--*maxlen;
	}

	// right justify if required
	if ((flags & LADJUST) == 0)
	{
		while ((fieldlength < width) && *maxlen)
		{
			*buf++ = (flags & ZEROPAD) ? '0' : ' ';
			width--;
			--*maxlen;
		}
	}

	// minus sign AFTER left padding if padding with spaces
	if (sign && maxlen && !(flags & ZEROPAD))
	{
		*buf++ = '-';
		--*maxlen;
	}

	// write the whole part
	tmp = pow(10.0, digits-1);
	while ((digits--) && *maxlen)
	{
		if (++significant_digits > MAX_SIGNIFICANT_DIGITS)
		{
			*buf++ = '0';
		}
		else
		{
			val = (int64_t)(fval / tmp);
			*buf++ = '0' + val;
			fval -= val * tmp;
			tmp *= 0.1;
		}
		--*maxlen;
	}

	// write the fraction part
	if (*maxlen && prec)
	{
		*buf++ = '.';
		--*maxlen;
	}

	tmp = pow(10.0, prec);

	fval *= tmp;
	while (prec-- && *maxlen)
	{
		if (++significant_digits > MAX_SIGNIFICANT_DIGITS)
		{
			*buf++ = '0';
		}
		else
		{
			tmp *= 0.1;
			val = (int64_t)(fval / tmp);
			*buf++ = '0' + val;
			fval -= val * tmp;
		}
		--*maxlen;
	}

	// left justify if required
	if (flags & LADJUST)
	{
		while ((fieldlength < width) && *maxlen)
		{
			// right-padding only with spaces, ZEROPAD is ignored
			*buf++ = ' ';
			width--;
			--*maxlen;
		}
	}

	// update parent's buffer pointer
	*buf_p = buf;
}

void AddBinary(char **buf_p, size_t *maxlen, uint64_t val, int width, int flags)
{
	char text[64];
	int digits;
	char *buf;

	digits = 0;
	do
	{
		if (val & 1)
		{
			text[digits++] = '1';
		}
		else
		{
			text[digits++] = '0';
		}
		val >>= 1;
	} while (val);

	buf = *buf_p;

	if (!(flags & LADJUST))
	{
		while (digits < width && *maxlen)
		{
			*buf++ = (flags & ZEROPAD) ? '0' : ' ';
			width--;
			--*maxlen;
		}
	}

	while (digits-- && *maxlen)
	{
		*buf++ = text[digits];
		width--;
		--*maxlen;
	}

	if (flags & LADJUST)
	{
		while (width-- && *maxlen)
		{
			*buf++ = (flags & ZEROPAD) ? '0' : ' ';
			--*maxlen;
		}
	}

	*buf_p = buf;
}

void AddUInt(char **buf_p, size_t *maxlen, uint64_t val, int width, int flags)
{
	char text[64];
	int digits;
	char *buf;

	digits = 0;
	do
	{
		text[digits++] = '0' + val % 10;
		val /= 10;
	} while (val);

	buf = *buf_p;

	if (!(flags & LADJUST))
	{
		while (digits < width && *maxlen)
		{
			*buf++ = (flags & ZEROPAD) ? '0' : ' ';
			width--;
			--*maxlen;
		}
	}

	while (digits-- && *maxlen)
	{
		*buf++ = text[digits];
		width--;
		--*maxlen;
	}

	if (flags & LADJUST)
	{
		while (width-- && *maxlen)
		{
			*buf++ = (flags & ZEROPAD) ? '0' : ' ';
			--*maxlen;
		}
	}

	*buf_p = buf;
}

void AddInt(char **buf_p, size_t *maxlen, int64_t val, int width, int flags)
{
	char text[64];
	int digits;
	int signedVal;
	char *buf;
	uint64_t unsignedVal;

	digits = 0;
	signedVal = val;
	if (val < 0)
	{
		/* we want the unsigned version */
		unsignedVal = llabs(val);
	}
	else
	{
		unsignedVal = val;
	}

	do
	{
		text[digits++] = '0' + unsignedVal % 10;
		unsignedVal /= 10;
	} while (unsignedVal);

	if (signedVal < 0)
	{
		text[digits++] = '-';
	}

	buf = *buf_p;

	if (!(flags & LADJUST))
	{
		while ((digits < width) && *maxlen)
		{
			*buf++ = (flags & ZEROPAD) ? '0' : ' ';
			width--;
			--*maxlen;
		}
	}

	while (digits-- && *maxlen)
	{
		*buf++ = text[digits];
		width--;
		--*maxlen;
	}

	if (flags & LADJUST)
	{
		while (width-- && *maxlen)
		{
			*buf++ = (flags & ZEROPAD) ? '0' : ' ';
			--*maxlen;
		}
	}

	*buf_p = buf;
}

void AddHex(char **buf_p, size_t *maxlen, uint64_t val, int width, int flags)
{
	char text[64];
	int digits;
	char *buf;
	char digit;
	int hexadjust;

	if (flags & UPPERDIGITS)
	{
		hexadjust = 'A' - '9' - 1;
	}
	else
	{
		hexadjust = 'a' - '9' - 1;
	}

	digits = 0;
	do 
	{
		digit = ('0' + val % 16);
		if (digit > '9')
		{
			digit += hexadjust;
		}

		text[digits++] = digit;
		val /= 16;
	} while(val);

	buf = *buf_p;

	if (!(flags & LADJUST))
	{
		while (digits < width && *maxlen)
		{
			*buf++ = (flags & ZEROPAD) ? '0' : ' ';
			width--;
			--*maxlen;
		}
	}

	while (digits-- && *maxlen)
	{
		*buf++ = text[digits];
		width--;
		--*maxlen;
	}

	if (flags & LADJUST)
	{
		while (width-- && *maxlen)
		{
			*buf++ = (flags & ZEROPAD) ? '0' : ' ';
			--*maxlen;
		}
	}

	*buf_p = buf;
}

// Modified from AddHex for printing octal values.
void AddOctal(char **buf_p, size_t *maxlen, uint64_t val, int width, int flags)
{
	char text[64];
	int digits;
	char *buf;
	char digit;
	int octadjust;

	digits = 0;
	do {
		text[digits++] = ('0' + val % 8);
		val /= 8;
	} while(val);

	buf = *buf_p;

	if (!(flags & LADJUST))
	{
		while (digits < width && *maxlen)
		{
			*buf++ = (flags & ZEROPAD) ? '0' : ' ';
			width--;
			--*maxlen;
		}
	}

	while (digits-- && *maxlen)
	{
		*buf++ = text[digits];
		width--;
		--*maxlen;
	}

	if (flags & LADJUST)
	{
		while (width-- && *maxlen)
		{
			*buf++ = (flags & ZEROPAD) ? '0' : ' ';
			--*maxlen;
		}
	}

	*buf_p = buf;
}

//		Edits done:
// void* array is changed to union Value.
// removed pPhrases, pOutPutLength, and pFailPhrase.
// Added extra formats for other data like '%p' for pointers.
int32_t gnprintf(char *buffer, size_t maxlen, const char *format, union Value params[], size_t numparams, size_t *const restrict curparam)
{
	if (!buffer || !maxlen)
	{
		return -1;
	}
	
	char
		*buf_p,
		ch, sign
	;
	int
		flags,
		width,
		prec,
		n, arg=0
	;
	const char *fmt;
	size_t llen = maxlen - 1;

	buf_p = buffer;
	fmt = format;

	while (true)
	{
		// run through the format string until we hit a '%' or '\0'
		for (ch = *fmt; llen && ((ch = *fmt) != '\0') && (ch != '%'); fmt++)
		{
			*buf_p++ = ch;
			llen--;
		}
		if ((ch == '\0') || (llen <= 0))
		{
			goto done;
		}

		// skip over the '%'
		fmt++;

		// reset formatting state
		flags = 0;
		width = 0;
		prec = -1;
		sign = '\0';

rflag:
		ch = *fmt++;
reswitch:
		switch(ch)
		{
			case '-':
			{
				flags |= LADJUST;
				goto rflag;
			}
			case '.':
			{
				n = 0;
				while(is_digit((ch = *fmt++)))
				{
					n = 10 * n + (ch - '0');
				}
				prec = (n < 0) ? -1 : n;
				goto reswitch;
			}
			case '0':
			{
				flags |= ZEROPAD;
				goto rflag;
			}
			case '1' ... '9':
			{
				n = 0;
				do
				{
					n = 10 * n + (ch - '0');
					ch = *fmt++;
				} while(is_digit(ch));
				width = n;
				goto reswitch;
			}
			case 'c': case 'C':
			{
				if (!llen)
				{
					goto done;
				}
				char c = params[*curparam].Char;
				++*curparam;
				*buf_p++ = c;
				llen--;
				arg++;
				break;
			}
			case 'b':
			{
				if( flags & LONGADJ )
					AddBinary(&buf_p, &llen, params[*curparam].Int64, width, flags);
				else AddBinary(&buf_p, &llen, params[*curparam].Int32, width, flags);
				++*curparam;
				arg++;
				break;
			}
			case 'd': case 'i':
			{
				if( flags & LONGADJ )
					AddInt(&buf_p, &llen, params[*curparam].Int64, width, flags);
				else AddInt(&buf_p, &llen, params[*curparam].Int32, width, flags);
				++*curparam;
				arg++;
				break;
			}
			case 'u':
			{
				if( flags & LONGADJ )
					AddUInt(&buf_p, &llen, params[*curparam].UInt64, width, flags);
				else AddUInt(&buf_p, &llen, params[*curparam].UInt32, width, flags);
				++*curparam;
				arg++;
				break;
			}
			case 'F': case 'E': case 'G':
			case 'f': case 'e': case 'g':
			{
				double value = params[*curparam].Double;
				++*curparam;
				AddFloat(&buf_p, &llen, value, width, prec, flags);
				arg++;
				break;
			}
			case 's':
			{
				const char *str = params[*curparam].String;
				++*curparam;
				AddString(&buf_p, &llen, str, width, prec, flags);
				arg++;
				break;
			}
			case 'X':
				flags |= UPPERDIGITS;
			case 'x':
			{
				if( flags & LONGADJ )
					AddHex(&buf_p, &llen, params[*curparam].UInt64, width, flags);
				else AddHex(&buf_p, &llen, params[*curparam].UInt32, width, flags);
				++*curparam;
				arg++;
				break;
			}
			case 'o': case 'O': {
				if( flags & LONGADJ )
					AddOctal(&buf_p, &llen, params[*curparam].UInt64, width, flags);
				else AddOctal(&buf_p, &llen, params[*curparam].UInt32, width, flags);
				++*curparam;
				arg++;
				break;
			}
			case 'P':
				flags |= UPPERDIGITS;
			case 'p': {
				uint64_t value = (uintptr_t)params[*curparam].Ptr;
				++*curparam;
				AddHex(&buf_p, &llen, value, width, flags);
				arg++;
				break;
			}
			case '%':
			{
				if (!llen)
				{
					goto done;
				}
				*buf_p++ = ch;
				llen--;
				break;
			}
			case 'L':
				flags |= UPPERDIGITS;
			case 'l':
				flags |= LONGADJ;
				goto rflag;
			
			case '\0':
			{
				if (!llen)
				{
					goto done;
				}
				*buf_p++ = '%';
				llen--;
				goto done;
			}
			default:
			{
				if (!llen)
				{
					goto done;
				}
				*buf_p++ = ch;
				llen--;
				break;
			}
		}
	}
	
done:
	*buf_p = '\0';
	return (maxlen - llen - 1);
}
/////////////////////////////////////////////////////////////////////////////////


bool Tagha_LoadNatives_stdio(struct Tagha *const restrict sys)
{
	if( !sys )
		return false;
	
	const struct NativeInfo libc_stdio_natives[] = {
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
	return Tagha_RegisterNatives(sys, libc_stdio_natives);
}
