#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#ifdef OS_WINDOWS
	#define HARBOL_LIB
#endif
#include "harbol.h"

/*
typedef struct HarbolString {
	char *CStr;
	size_t Len;
} HarbolString;
*/

HARBOL_EXPORT struct HarbolString *HarbolString_New(void)
{
	return calloc(1, sizeof(struct HarbolString));
}

HARBOL_EXPORT struct HarbolString *HarbolString_NewStr(const char *restrict cstr)
{
	struct HarbolString *restrict str = calloc(1, sizeof *str);
	if( str )
		HarbolString_CopyStr(str, cstr);
	return str;
}

HARBOL_EXPORT void HarbolString_Del(struct HarbolString *const strobj)
{
	if( !strobj )
		return;
	
	if( strobj->CStr )
		free(strobj->CStr);
	memset(strobj, 0, sizeof *strobj);
}

HARBOL_EXPORT bool HarbolString_Free(struct HarbolString **strobjref)
{
	if( !strobjref || !*strobjref )
		return false;
	
	HarbolString_Del(*strobjref);
	free(*strobjref); *strobjref=NULL;
	return *strobjref==NULL;
}

HARBOL_EXPORT void HarbolString_Init(struct HarbolString *const strobj)
{
	if( !strobj )
		return;
	
	memset(strobj, 0, sizeof *strobj);
}

HARBOL_EXPORT void HarbolString_InitStr(struct HarbolString *const restrict strobj, const char *restrict cstr)
{
	if( !strobj )
		return;
	
	memset(strobj, 0, sizeof *strobj);
	HarbolString_CopyStr(strobj, cstr);
}

HARBOL_EXPORT void HarbolString_AddChar(struct HarbolString *const restrict strobj, const char c)
{
	if( !strobj )
		return;
	
	size_t oldsize = strobj->Len;
	// adjust old size for new char && null-term.
	char *newstr = calloc(oldsize+2, sizeof *newstr);
	if( !newstr )
		return;
	
	// alot more efficient to use a vector of chars for adding chars as opposed to an immutable string...
	// TODO: check if the array is filled before adding a new char so we can avoid reallocating.
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

HARBOL_EXPORT void HarbolString_Add(struct HarbolString *const restrict strobjA, const struct HarbolString *const restrict strobjB)
{
	if( !strobjA || !strobjB || !strobjB->CStr )
		return;
	
	char *newstr = calloc(strobjA->Len + strobjB->Len + 1, sizeof *newstr);
	if( !newstr )
		return;
	
	strobjA->Len += strobjB->Len;
	if( strobjA->CStr ) {
		strcat(newstr, strobjA->CStr);
		free(strobjA->CStr), strobjA->CStr=NULL;
	}
	strcat(newstr, strobjB->CStr);
	strobjA->CStr = newstr;
	strobjA->CStr[strobjA->Len] = 0;
}

HARBOL_EXPORT void HarbolString_AddStr(struct HarbolString *const restrict strobj, const char *restrict cstr)
{
	if( !strobj || !cstr )
		return;
	
	char *newstr = calloc(strobj->Len + strlen(cstr) + 1, sizeof *newstr);
	if( !newstr )
		return;
	
	strobj->Len += strlen(cstr);
	if( strobj->CStr ) {
		strcat(newstr, strobj->CStr);
		free(strobj->CStr), strobj->CStr=NULL;
	}
	strcat(newstr, cstr);
	strobj->CStr = newstr;
	strobj->CStr[strobj->Len] = 0;
}

HARBOL_EXPORT char *HarbolString_GetStr(const struct HarbolString *const strobj)
{
	return (strobj) ? strobj->CStr : NULL;
}

HARBOL_EXPORT size_t HarbolString_Len(const struct HarbolString *const strobj)
{
	return (strobj) ? strobj->Len : 0;
}

HARBOL_EXPORT void HarbolString_Copy(struct HarbolString *const restrict strobjA, const struct HarbolString *const restrict strobjB)
{
	if( !strobjA || !strobjB || !strobjB->CStr )
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

HARBOL_EXPORT void HarbolString_CopyStr(struct HarbolString *const restrict strobj, const char *restrict cstr)
{
	if( !strobj || !cstr )
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

HARBOL_EXPORT int32_t HarbolString_Format(struct HarbolString *const restrict strobj, const char *restrict fmt, ...)
{
	if( !strobj || !fmt )
		return -1;
	
	va_list ap;
	va_start(ap, fmt);
	const int32_t result = vsnprintf(strobj->CStr, strobj->Len, fmt, ap);
	va_end(ap);
	return result;
}

HARBOL_EXPORT int32_t HarbolString_CmpCStr(const struct HarbolString *const restrict strobj, const char *restrict cstr)
{
	return ( !strobj || !cstr || !strobj->CStr ) ? -1 : strcmp(strobj->CStr, cstr);
}

HARBOL_EXPORT int32_t HarbolString_CmpStr(const struct HarbolString *const restrict strobjA, const struct HarbolString *const restrict strobjB)
{
	return ( !strobjA || !strobjB || !strobjA->CStr || !strobjB->CStr ) ? -1 : strcmp(strobjA->CStr, strobjB->CStr);
}

HARBOL_EXPORT int32_t HarbolString_NCmpCStr(const struct HarbolString *const restrict strobj, const char *restrict cstr, const size_t len)
{
	return ( !strobj || !cstr || !strobj->CStr ) ? -1 : strncmp(strobj->CStr, cstr, len);
}

HARBOL_EXPORT int32_t HarbolString_NCmpStr(const struct HarbolString *const restrict strobjA, const struct HarbolString *const restrict strobjB, const size_t len)
{
	return ( !strobjA || !strobjB || !strobjA->CStr || !strobjB->CStr ) ? -1 : strncmp(strobjA->CStr, strobjB->CStr, len);
}

HARBOL_EXPORT bool HarbolString_IsEmpty(const struct HarbolString *const strobj)
{
	return( !strobj || strobj->Len==0 || strobj->CStr==NULL || strobj->CStr[0]==0 );
}

HARBOL_EXPORT bool HarbolString_Reserve(struct HarbolString *const strobj, const size_t size)
{
	if( !strobj || !size )
		return false;
	
	strobj->CStr = realloc(strobj->CStr, size+1 * sizeof strobj->CStr);
	if( !strobj->CStr )
		return false;
	memset(strobj->CStr, 0, size);
	strobj->Len = size;
	return true;
}

HARBOL_EXPORT char *HarbolString_fgets(struct HarbolString *const strobj, FILE *const file)
{
	return ( !strobj || !file ) ? NULL : fgets(strobj->CStr, strobj->Len, file);
}

HARBOL_EXPORT void HarbolString_Empty(struct HarbolString *const strobj)
{
	if( !strobj || !strobj->CStr )
		return;
	
	memset(strobj->CStr, 0, strobj->Len);
}
