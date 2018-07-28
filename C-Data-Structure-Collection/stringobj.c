
#include <stdlib.h>
#include <stdio.h>
#include "dsc.h"

/*
typedef struct String {
	size_t Len;
	char *CStr;
} String;
*/

struct String *String_New(void)
{
	return calloc(1, sizeof(struct String));
}

struct String *String_NewStr(const char *restrict cstr)
{
	struct String *str = calloc(1, sizeof *str);
	if( str )
		String_CopyStr(str, cstr);
	return str;
}

void String_Del(struct String *const restrict strobj)
{
	if( !strobj )
		return;
	
	if( strobj->CStr )
		free(strobj->CStr);
	strobj->CStr=NULL;
	strobj->Len = 0;
}

void String_Free(struct String **restrict strobjref)
{
	if( !*strobjref )
		return;
	
	String_Del(*strobjref);
	free(*strobjref);
	*strobjref=NULL;
}

void String_Init(struct String *const restrict strobj)
{
	if( !strobj )
		return;
	
	*strobj = (struct String){0};
}

void String_InitStr(struct String *const restrict strobj, const char *restrict cstr)
{
	if( !strobj )
		return;
	
	*strobj = (struct String){0};
	String_CopyStr(strobj, cstr);
}

void String_AddChar(struct String *const restrict strobj, const char c)
{
	if( !strobj )
		return;
	
	size_t oldsize = strobj->Len;
	// adjust old size for new char and null-term.
	char *newstr = calloc(oldsize+2, sizeof *newstr);
	if( !newstr )
		return;
	
	// alot more efficient to use a vector of chars
	// for adding chars as opposed to an immutable string...
	strobj->Len++;
	if( strobj->CStr ) {
		memcpy(newstr, strobj->CStr, oldsize);
		free(strobj->CStr);
		strobj->CStr=NULL;
	}
	strobj->CStr = newstr;
	strobj->CStr[strobj->Len-1] = c;
	strobj->CStr[strobj->Len] = 0;
}

void String_Add(struct String *const restrict strobjA, const struct String *const restrict strobjB)
{
	if( !strobjA or !strobjB or !strobjB->CStr )
		return;
	
	char *newstr = calloc(strobjA->Len + strobjB->Len + 1, sizeof *newstr);
	if( !newstr )
		return;
	
	strobjA->Len += strobjB->Len;
	if( strobjA->CStr ) {
		strcat(newstr, strobjA->CStr);
		free(strobjA->CStr);
		strobjA->CStr=NULL;
	}
	strcat(newstr, strobjB->CStr);
	strobjA->CStr = newstr;
	strobjA->CStr[strobjA->Len] = 0;
}

void String_AddStr(struct String *const restrict strobj, const char *restrict cstr)
{
	if( !strobj or !cstr )
		return;
	
	char *newstr = calloc(strobj->Len + strlen(cstr) + 1, sizeof *newstr);
	if( !newstr )
		return;
	
	strobj->Len += strlen(cstr);
	if( strobj->CStr ) {
		strcat(newstr, strobj->CStr);
		free(strobj->CStr);
		strobj->CStr=NULL;
	}
	strcat(newstr, cstr);
	strobj->CStr = newstr;
	strobj->CStr[strobj->Len] = 0;
}

char *String_GetStr(const struct String *const restrict strobj)
{
	return (strobj) ? strobj->CStr : NULL;
}

size_t String_Len(const struct String *const restrict strobj)
{
	return (strobj) ? strobj->Len : 0;
}

void String_Copy(struct String *const restrict strobjA, const struct String *const restrict strobjB)
{
	if( !strobjA or !strobjB or !strobjB->CStr )
		return;
	
	strobjA->Len = strobjB->Len;
	if( strobjA->CStr )
		free(strobjA->CStr), strobjA->CStr=NULL;
	
	strobjA->CStr = calloc(strobjA->Len+1, sizeof *strobjA->CStr);
	if( !strobjA->CStr )
		return;
	
	strcpy(strobjA->CStr, strobjB->CStr);
	strobjA->CStr[strobjA->Len] = 0;
}

void String_CopyStr(struct String *const restrict strobj, const char *restrict cstr)
{
	if( !strobj or !cstr )
		return;
	
	strobj->Len = strlen(cstr);
	if( strobj->CStr )
		free(strobj->CStr), strobj->CStr=NULL;
	
	strobj->CStr = calloc(strobj->Len+1, sizeof *strobj->CStr);
	if( !strobj->CStr )
		return;
	
	strcpy(strobj->CStr, cstr);
	strobj->CStr[strobj->Len] = 0;
}

int32_t String_CmpCStr(const struct String *const restrict strobj, const char *restrict cstr)
{
	if( !strobj or !cstr or !strobj->CStr )
		return -1;
	
	return strcmp(strobj->CStr, cstr);
}

int32_t String_CmpStr(const struct String *const restrict strobjA, const struct String *const restrict strobjB)
{
	if( !strobjA or !strobjB or !strobjA->CStr or !strobjB->CStr )
		return -1;
	
	return strcmp(strobjA->CStr, strobjB->CStr);
}

int32_t String_NCmpCStr(const struct String *const restrict strobj, const char *restrict cstr, const size_t len)
{
	if( !strobj or !cstr or !strobj->CStr )
		return -1;
	
	return strncmp(strobj->CStr, cstr, len);
}

int32_t String_NCmpStr(const struct String *const restrict strobjA, const struct String *const restrict strobjB, const size_t len)
{
	if( !strobjA or !strobjB or !strobjA->CStr or !strobjB->CStr )
		return -1;
	
	return strncmp(strobjA->CStr, strobjB->CStr, len);
}

bool String_IsEmpty(const struct String *const restrict strobj)
{
	return( !strobj or strobj->Len==0 or strobj->CStr==NULL or strobj->CStr[0]==0 );
}

bool String_Reserve(struct String *const restrict strobj, const size_t size)
{
	if( !strobj or !size )
		return false;
	
	strobj->CStr = calloc(size+1, sizeof *strobj->CStr);
	if( !strobj->CStr )
		return false;
	strobj->Len = size;
	return true;
}

char *String_fgets(struct String *const restrict strobj, const size_t count, void *ptr)
{
	if( !strobj or !ptr )
		return NULL;
	
	String_Del(strobj);
	FILE *restrict file = ptr;
	String_Reserve(strobj, count);
	return fgets(strobj->CStr, count, file);
}
