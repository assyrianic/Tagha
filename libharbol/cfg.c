#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef OS_WINDOWS
	#define HARBOL_LIB
#endif
#include "harbol.h"

/* CFG Parser in EBNF grammar
	keyval = <string> [':'] (<value>|<section>) [','] ;
	section = '{' <keyval> '}' ;
	value = <string> | <number> | <color> | "true" | "false" | "null" ;
	matrix = '[' <number> [','] <number> [','] <number> [','] <number> ']' ;
	color = 'c' <matrix> ;
	vecf = 'v' <matrix> ;
	string = '"' chars '"' | "'" chars "'" ;
*/

static size_t __LineNo;

static bool IsDecimal(const char c)
{
	return( c >= '0' && c <= '9' );
}

static bool IsOctal(const char c)
{
	return( c >= '0' && c <= '7' );
}

static bool IsHex(const char c)
{
	return( (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F') || IsDecimal(c) );
}

static bool IsWhiteSpace(const char c)
{
	return( c==' ' || c=='\t' || c=='\r' || c=='\v' || c=='\f' || c=='\n' );
}


static void SkipSingleLineComment(const char **strRef)
{
	if( !*strRef || !**strRef )
		return;
	
	for( ; **strRef != '\n' ; (*strRef)++ );
	__LineNo++;
}

static void SkipMultiLineComment(const char **strRef)
{
	if( !*strRef || !**strRef )
		return;
	// skip past '/' && '*'
	*strRef += 2;
	do {
		if( !**strRef || !(*strRef)[1] )
			break;
		else if( **strRef=='\n' )
			__LineNo++;
		(*strRef)++;
	} while( **strRef!='*' && (*strRef)[1]!='/' );
	*strRef += 2;
}

static bool SkipWhiteSpace(const char **strRef)
{
	if( !*strRef || !**strRef )
		return false;
	
	while( **strRef && IsWhiteSpace(**strRef) ) {
		if( **strRef=='\n' )
			__LineNo++;
		(*strRef)++;
	}
	return **strRef != 0;
}

static bool SkipWSandComments(const char **strRef)
{
	if( !strRef || !*strRef || !**strRef )
		return false;
	
	while( **strRef && (IsWhiteSpace(**strRef) || // white space
			**strRef=='#' || (**strRef=='/' && (*strRef)[1]=='/') || // single line comment
			(**strRef=='/' && (*strRef)[1]=='*') || // multi-line comment
			**strRef==':' || **strRef==',') ) // delimiters.
	{
		if( IsWhiteSpace(**strRef) )
			SkipWhiteSpace(strRef);
		else if( **strRef=='#' || (**strRef=='/' && (*strRef)[1]=='/') )
			SkipSingleLineComment(strRef);
		else if( **strRef=='/' && (*strRef)[1]=='*' )
			SkipMultiLineComment(strRef);
		else if( **strRef==':' || **strRef==',' )
			(*strRef)++;
	}
	return **strRef != 0;
}

static bool LexString(const char **restrict strRef, struct HarbolString *const restrict str)
{
	if( !*strRef || !**strRef || !str )
		return false;
	
	else if( **strRef != '"' && **strRef != '\'' )
		return false;
	const char quote = *(*strRef)++;
	while( **strRef && **strRef != quote ) {
		if( **strRef=='\\' ) {
			const char chr = *(*strRef)++;
			switch( chr ) {
				case 'a': harbol_string_add_char(str, '\a'); break;
				case 'r': harbol_string_add_char(str, '\r'); break;
				case 't': harbol_string_add_char(str, '\t'); break;
				case 'v': harbol_string_add_char(str, '\v'); break;
				case 'n': harbol_string_add_char(str, '\n'); break;
				case 'f': harbol_string_add_char(str, '\f'); break;
				default: harbol_string_add_char(str, chr);
			}
		}
		else harbol_string_add_char(str, *(*strRef)++);
	}
	if( **strRef==quote )
		(*strRef)++;
	return **strRef != 0;
}

static bool LexNumber(const char **restrict strRef, struct HarbolString *const restrict str, enum HarbolCfgType *const typeref)
{
	if( !*strRef || !**strRef || !str || !typeref )
		return false;
	
	if( **strRef=='-' || **strRef=='+' )
		harbol_string_add_char(str, *(*strRef)++);
	
	if( !IsDecimal(**strRef) && **strRef!='.' )
		return false;
	
	if( **strRef=='0' ) {
		harbol_string_add_char(str, *(*strRef)++);
		const char numtype = *(*strRef)++;
		harbol_string_add_char(str, numtype);
		*typeref = HarbolTypeInt;
		
		switch( numtype ) {
			case 'X': case 'x': // hex
				harbol_string_add_char(str, *(*strRef)++);
				while( IsHex(**strRef) )
					harbol_string_add_char(str, *(*strRef)++);
				break;
			case '.': // float
				*typeref = HarbolTypeFloat;
				harbol_string_add_char(str, *(*strRef)++);
				while( IsDecimal(**strRef) || **strRef=='e' || **strRef=='E' || **strRef=='f' || **strRef=='F' )
					harbol_string_add_char(str, *(*strRef)++);
				break;
			default: // octal
				while( IsOctal(**strRef) )
					harbol_string_add_char(str, *(*strRef)++);
		}
	}
	else if( IsDecimal(**strRef) ) { // numeric value. Check if float possibly.
		*typeref = HarbolTypeInt;
		while( IsDecimal(**strRef) )
			harbol_string_add_char(str, *(*strRef)++);
		
		if( **strRef=='.' ) { // definitely float value.
			*typeref = HarbolTypeFloat;
			harbol_string_add_char(str, *(*strRef)++);
			while( IsDecimal(**strRef) || **strRef=='e' || **strRef=='E' || **strRef=='f' || **strRef=='F' )
				harbol_string_add_char(str, *(*strRef)++);
		}
	}
	else if( **strRef=='.' ) { // float value.
		*typeref = HarbolTypeFloat;
		harbol_string_add_char(str, *(*strRef)++);
		while( IsDecimal(**strRef) || **strRef=='e' || **strRef=='E' )
			harbol_string_add_char(str, *(*strRef)++);
	}
	return str->Len > 0;
}

static bool harbol_cfg_ParseSection(struct HarbolLinkMap *, const char **);
static bool harbol_cfg_ParseNumber(struct HarbolLinkMap *, const struct HarbolString *, const char **);

// keyval = <string> [':'] (<value>|<section>) [','] ;
static bool harbol_cfg_ParseKeyVal(struct HarbolLinkMap *const restrict map, const char **cfgcoderef)
{
	if( !map || !*cfgcoderef || !**cfgcoderef )
		return false;
	
	if( !SkipWSandComments(cfgcoderef) ) {
		fprintf(stderr, "Harbol Config Parser :: unexpected end of file. Line: %zu\n", __LineNo);
		return false;
	}
	else if( **cfgcoderef!='"' && **cfgcoderef!='\'' ) {
		fprintf(stderr, "Harbol Config Parser :: missing beginning quote for key. Line: %zu\n", __LineNo);
		return false;
	}
	
	struct HarbolString keystr = {NULL, 0};
	const bool strresult = LexString(cfgcoderef, &keystr);
	if( !strresult ) {
		fprintf(stderr, "Harbol Config Parser :: invalid string key (%s). Line: %zu\n", keystr.CStr, __LineNo);
		harbol_string_del(&keystr);
		return false;
	}
	SkipWSandComments(cfgcoderef);
	
	bool res = false;
	// it's a section!
	if( **cfgcoderef=='{' ) {
		struct HarbolLinkMap *restrict subsection = harbol_linkmap_new();
		res = harbol_cfg_ParseSection(subsection, cfgcoderef);
		struct HarbolVariant *var = harbol_variant_new((union HarbolValue){.Ptr=subsection}, HarbolTypeLinkMap);
		harbol_linkmap_insert(map, keystr.CStr, (union HarbolValue){ .VarPtr=var });
	}
	// string value.
	else if( **cfgcoderef=='"'||**cfgcoderef=='\'' ) {
		struct HarbolString *str = harbol_string_new();
		res = LexString(cfgcoderef, str);
		if( !res ) {
			if( !str )
				fprintf(stderr, "Harbol Config Parser :: unable to allocate string value. Line: %zu\n", __LineNo);
			else fprintf(stderr, "Harbol Config Parser :: invalid string value (%s). Line: %zu\n", str->CStr, __LineNo);
			return false;
		}
		struct HarbolVariant *var = harbol_variant_new((union HarbolValue){.Ptr=str}, HarbolTypeString);
		harbol_linkmap_insert(map, keystr.CStr, (union HarbolValue){ .VarPtr=var });
	}
	// color/vector value!
	else if( **cfgcoderef=='c' || **cfgcoderef=='v' ) {
		const char valtype = *(*cfgcoderef)++;
		SkipWSandComments(cfgcoderef);
		
		if( **cfgcoderef!='[' ) {
			fprintf(stderr, "Harbol Config Parser :: missing '['. Line: %zu\n", __LineNo);
			harbol_string_del(&keystr);
			return false;
		}
		(*cfgcoderef)++;
		SkipWSandComments(cfgcoderef);
		
		union {
			union HarbolColor color;
			union HarbolVec4D vec4d;
		} matrix_value = {0};
		
		size_t iterations = 0;
		while( **cfgcoderef && **cfgcoderef != ']' ) {
			struct HarbolString numstr = {NULL, 0};
			enum HarbolCfgType type = HarbolTypeNull;
			const bool result = LexNumber(cfgcoderef, &numstr, &type);
			if( iterations<4 ) {
				if( valtype=='c' ) {
					matrix_value.color.RGBA[iterations++] = (uint8_t)strtoul(numstr.CStr, NULL, 0);
				}
				else {
					matrix_value.vec4d.XYZW[iterations++] = (float)strtof(numstr.CStr, NULL);
				}
			}
			harbol_string_del(&numstr);
			if( !result ) {
				fprintf(stderr, "Harbol Config Parser :: invalid number in [] array. Line: %zu\n", __LineNo);
				harbol_string_del(&keystr);
				return false;
			}
			SkipWSandComments(cfgcoderef);
		}
		if( !**cfgcoderef ) {
			fprintf(stderr, "Harbol Config Parser :: unexpected end of file with missing ending ']'. Line: %zu\n", __LineNo);
			return false;
		}
		(*cfgcoderef)++;
		
		const size_t matrix_data[] = { valtype=='c' ? sizeof(union HarbolColor) : sizeof(union HarbolVec4D) };
		struct HarbolTuple *tuple = harbol_tuple_new(1, matrix_data, false);
		valtype=='c' ?
			harbol_tuple_set_field(tuple, 0, &matrix_value.color.UIntColor)
			: harbol_tuple_set_field(tuple, 0, &matrix_value.vec4d.XYZW[0]);
		
		struct HarbolVariant *var = harbol_variant_new((union HarbolValue){.TuplePtr=tuple}, valtype=='c' ? HarbolTypeColor : HarbolTypeVec4D);
		res = harbol_linkmap_insert(map, keystr.CStr, (union HarbolValue){ .VarPtr=var });
	}
	// true bool value.
	else if( **cfgcoderef=='t' ) {
		if( strncmp("true", *cfgcoderef, strlen("true")) ) {
			fprintf(stderr, "Harbol Config Parser :: invalid word value, only 'true', 'false' or 'null' are allowed. Line: %zu\n", __LineNo);
			harbol_string_del(&keystr);
			return false;
		}
		*cfgcoderef += strlen("true");
		struct HarbolVariant *var = harbol_variant_new((union HarbolValue){.Bool=true}, HarbolTypeBool);
		res = harbol_linkmap_insert(map, keystr.CStr, (union HarbolValue){ .VarPtr=var });
	}
	// false bool value.
	else if( **cfgcoderef=='f' ) {
		if( strncmp("false", *cfgcoderef, strlen("false")) ) {
			fprintf(stderr, "Harbol Config Parser :: invalid word value, only 'true', 'false' or 'null' are allowed. Line: %zu\n", __LineNo);
			harbol_string_del(&keystr);
			return false;
		}
		*cfgcoderef += strlen("false");
		struct HarbolVariant *var = harbol_variant_new((union HarbolValue){.Bool=false}, HarbolTypeBool);
		res = harbol_linkmap_insert(map, keystr.CStr, (union HarbolValue){ .VarPtr=var });
	}
	// null value.
	else if( **cfgcoderef=='n' ) {
		if( strncmp("null", *cfgcoderef, strlen("null")) ) {
			fprintf(stderr, "Harbol Config Parser :: invalid word value, only 'true', 'false' or 'null' are allowed. Line: %zu\n", __LineNo);
			harbol_string_del(&keystr);
			return false;
		}
		*cfgcoderef += strlen("null");
		struct HarbolVariant *var = harbol_variant_new((union HarbolValue){0}, HarbolTypeNull);
		res = harbol_linkmap_insert(map, keystr.CStr, (union HarbolValue){ .VarPtr=var });
	}
	// numeric value.
	else if( IsDecimal(**cfgcoderef) || **cfgcoderef=='.' || **cfgcoderef=='-' || **cfgcoderef=='+' ) {
		res = harbol_cfg_ParseNumber(map, &keystr, cfgcoderef);
	}
	else if( **cfgcoderef=='[' ) {
		fprintf(stderr, "Harbol Config Parser :: array bracket missing 'c' or 'v' tag. Line: %zu\n", __LineNo);
		harbol_string_del(&keystr);
		return false;
	}
	else {
		fprintf(stderr, "Harbol Config Parser :: unknown character detected (%c). Line: %zu\n", __LineNo, **cfgcoderef);
		res = false;
	}
	harbol_string_del(&keystr);
	SkipWSandComments(cfgcoderef);
	return res;
}

static bool harbol_cfg_ParseNumber(struct HarbolLinkMap *const restrict map, const struct HarbolString *const restrict key, const char **cfgcoderef)
{
	struct HarbolString numstr = {NULL, 0};
	enum HarbolCfgType type = HarbolTypeNull;
	
	const bool result = LexNumber(cfgcoderef, &numstr, &type);
	if( !result ) {
		fprintf(stderr, "Harbol Config Parser :: invalid number. Line: %zu\n", __LineNo);
		harbol_string_del(&numstr);
		return result;
	}
	struct HarbolVariant *var = harbol_variant_new(type==HarbolTypeFloat ? (union HarbolValue){.Double=strtod(numstr.CStr, NULL)} : (union HarbolValue){.Int64=strtoll(numstr.CStr, NULL, 0)}, type);
	harbol_string_del(&numstr);
	return harbol_linkmap_insert(map, key->CStr, (union HarbolValue){ .VarPtr=var });
}

// section = '{' <keyval> '}' ;
static bool harbol_cfg_ParseSection(struct HarbolLinkMap *const restrict map, const char **cfgcoderef)
{
	if( **cfgcoderef!='{' ) {
		fprintf(stderr, "Harbol Config Parser :: missing '{' for section. Line: %zu\n", __LineNo);
		return false;
	}
	(*cfgcoderef)++;
	SkipWSandComments(cfgcoderef);
	
	while( **cfgcoderef && **cfgcoderef != '}' ) {
		const bool res = harbol_cfg_ParseKeyVal(map, cfgcoderef);
		if( !res )
			return false;
	}
	if( !**cfgcoderef ) {
		fprintf(stderr, "Harbol Config Parser :: unexpected end of file with missing '}' for section. Line: %zu\n", __LineNo);
		return false;
	}
	(*cfgcoderef)++;
	return true;
}


HARBOL_EXPORT struct HarbolLinkMap *harbol_cfg_from_file(const char filename[restrict])
{
	if( !filename )
		return NULL;
	
	FILE *restrict cfgfile = fopen(filename, "r");
	if( !cfgfile ) {
		fputs("harbol_cfg_from_file :: unable to find file.\n", stderr);
		return NULL;
	}
	fseek(cfgfile, 0, SEEK_END);
	const int64_t filesize = ftell(cfgfile);
	if( filesize <= -1 ) {
		fprintf(stderr, "harbol_cfg_from_file :: size of file (%" PRIi64 ") is negative!\n", filesize);
		fclose(cfgfile), cfgfile=NULL;
		return NULL;
	}
	rewind(cfgfile);
	
	char *cfgcode = calloc(filesize+1, sizeof *cfgcode);
	if( !cfgcode ) {
		fputs("harbol_cfg_from_file :: unable to allocate buffer for file.\n", stderr);
		fclose(cfgfile), cfgfile=NULL;
		return NULL;
	}
	const size_t val = fread(cfgcode, sizeof *cfgcode, filesize, cfgfile);
	fclose(cfgfile), cfgfile=NULL;
	if( val != filesize ) {
		fprintf(stderr, "harbol_cfg_from_file :: filesize (%" PRIi64 ") does not match 'fread' return value! (%zu)\n", filesize, val);
		free(cfgcode), cfgcode=NULL;
		return NULL;
	}
	struct HarbolLinkMap *const restrict objs = harbol_cfg_parse_cstr(cfgcode);
	free(cfgcode), cfgcode=NULL;
	return objs;
}


HARBOL_EXPORT struct HarbolLinkMap *harbol_cfg_parse_cstr(const char cfgcode[])
{
	if( !cfgcode )
		return NULL;
	__LineNo = 0;
	const char *iter = cfgcode;
	struct HarbolLinkMap *objs = harbol_linkmap_new();
	while( harbol_cfg_ParseKeyVal(objs, &iter) );
	return objs;
}

HARBOL_EXPORT bool harbol_cfg_free(struct HarbolLinkMap **mapref)
{
	if( !mapref || !*mapref )
		return false;
	
	const union HarbolValue *const end = harbol_linkmap_get_iter_end_count(*mapref);
	for( union HarbolValue *iter = harbol_linkmap_get_iter(*mapref) ; iter && iter<end ; iter++ ) {
		struct HarbolKeyValPair *n = iter->KvPairPtr;
		switch( n->Data.VarPtr->TypeTag ) {
			case HarbolTypeLinkMap: {
				struct HarbolLinkMap *innermap = harbol_linkmap_get_val(n->Data.VarPtr).LinkMapPtr;
				harbol_cfg_free(&innermap);
				break;
			}
			case HarbolTypeString: {
				struct HarbolString *strtype = harbol_linkmap_get_val(n->Data.VarPtr).StrObjPtr;
				harbol_string_free(&strtype);
				break;
			}
			case HarbolTypeColor:
			case HarbolTypeVec4D: {
				struct HarbolTuple *tup = harbol_linkmap_get_val(n->Data.VarPtr).TuplePtr;
				harbol_tuple_free(&tup);
				break;
			}
			default:
				break;
		}
		harbol_variant_free(&n->Data.VarPtr, NULL);
	}
	harbol_linkmap_free(mapref, NULL);
	return *mapref==NULL;
}

/*
static const char *Harbol_GetTypeName(const enum HarbolCfgType type)
{
	switch( type ) {
		case HarbolTypeNull:        return "Null";
		case HarbolTypeLinkMap:     return "LinkMap";
		case HarbolTypeString:      return "String";
		case HarbolTypeFloat:       return "Float";
		case HarbolTypeInt:         return "Int";
		case HarbolTypeBool:        return "Boolean";
		case HarbolTypeColor:       return "Color";
		case HarbolTypeVec4D:       return "Vector";
		default: return "Unknown Type";
	}
}
*/

HARBOL_EXPORT bool harbol_cfg_to_str(const struct HarbolLinkMap *const restrict map, struct HarbolString *const str)
{
	if( !map || !str )
		return false;
	
	# define BUFFER_SIZE    512
	const union HarbolValue *const end = harbol_linkmap_get_iter_end_count(map);
	for( union HarbolValue *iter = harbol_linkmap_get_iter(map) ; iter && iter<end ; iter++ ) {
		const struct HarbolKeyValPair *kv = iter->KvPairPtr;
		const int32_t type = kv->Data.VarPtr->TypeTag;
		// print out key and notation.
		harbol_string_add_char(str, '"');
		harbol_string_add_str(str, &kv->KeyName);
		harbol_string_add_cstr(str, "\": ");
		
		char buffer[BUFFER_SIZE] = {0};
		switch( type ) {
			case HarbolTypeNull:
				harbol_string_add_cstr(str, "null\n");
				break;
			case HarbolTypeLinkMap: {
				harbol_string_add_cstr(str, "{\n");
				harbol_cfg_to_str(kv->Data.VarPtr->Val.LinkMapPtr, str);
				harbol_string_add_cstr(str, "}\n");
				break;
			}
			case HarbolTypeString:
				harbol_string_add_cstr(str, "\"");
				harbol_string_add_str(str, kv->Data.VarPtr->Val.StrObjPtr);
				harbol_string_add_cstr(str, "\"\n");
				break;
			case HarbolTypeFloat:
				snprintf(buffer, BUFFER_SIZE, "%f\n", kv->Data.VarPtr->Val.Double);
				harbol_string_add_cstr(str, buffer);
				break;
			case HarbolTypeInt:
				snprintf(buffer, BUFFER_SIZE, "%" PRIi64 "\n", kv->Data.VarPtr->Val.Int64);
				harbol_string_add_cstr(str, buffer);
				break;
			case HarbolTypeBool:
				harbol_string_add_cstr(str, kv->Data.VarPtr->Val.Bool ? "true\n" : "false\n");
				break;
			case HarbolTypeColor: {
				harbol_string_add_cstr(str, "c[ ");
				struct { uint8_t r,g,b,a; } color = {0};
				harbol_tuple_to_struct(kv->Data.VarPtr->Val.TuplePtr, &color);
				snprintf(buffer, BUFFER_SIZE, "%u, %u, %u, %u ]\n", color.r, color.g, color.b, color.a);
				harbol_string_add_cstr(str, buffer);
				break;
			}
			case HarbolTypeVec4D: {
				harbol_string_add_cstr(str, "v[ ");
				struct { float x,y,z,w; } vec4 = {0};
				harbol_tuple_to_struct(kv->Data.VarPtr->Val.TuplePtr, &vec4);
				snprintf(buffer, BUFFER_SIZE, "%f, %f, %f, %f ]\n", vec4.x, vec4.y, vec4.z, vec4.w);
				harbol_string_add_cstr(str, buffer);
				break;
			}
		}
	}
	return str->Len > 0;
}

static bool harbol_cfg_ParseTargetStringPath(const char key[], struct HarbolString *const restrict str)
{
	if( !key || !str )
		return false;
	
	// parse something like: "root.section1.section2.section3"
	const char *iter = key;
	
	// iterate to the null terminator and then work backwards to the last dot.
	// ughhh too many while loops lmao.
	iter += strlen(key);
	while( *iter != '.' ) 
		--iter;
	iter++;
	
	// now we save the target section and then 
	while( *iter )
		harbol_string_add_char(str, *iter++);
	return str->Len > 0;
}

HARBOL_EXPORT struct HarbolLinkMap *harbol_cfg_get_section_by_key(struct HarbolLinkMap *const cfgmap, const char key[])
{
	if( !cfgmap || !key )
		return NULL;
	
	const struct HarbolVariant *restrict var = NULL;
	// first check if we're getting a singular value OR we iterate through a sectional path.
	const bool has_dot_path = strchr(key, '.') != NULL;
	if( !has_dot_path ) {
		var = harbol_linkmap_get(cfgmap, key).VarPtr;
		return ( !var || var->TypeTag != HarbolTypeLinkMap ) ? NULL : var->Val.LinkMapPtr;
	}
	// ok, not a singular value, iterate to the specific linkmap section then.
	else {
		// parse the target key first.
		const char *iter = key;
		struct HarbolString
			sectionstr = {NULL, 0},
			targetstr = {NULL, 0}
		;
		harbol_cfg_ParseTargetStringPath(key, &targetstr);
		struct HarbolLinkMap
			*itermap = cfgmap,
			*restrict retmap = NULL
		;
		while( !retmap ) {
			harbol_string_del(&sectionstr);
			while( *iter && *iter != '.' )
				harbol_string_add_char(&sectionstr, *iter++);
			if( *iter )
				iter++;
			
			var = harbol_linkmap_get(itermap, sectionstr.CStr).VarPtr;
			
			if( !var || var->TypeTag != HarbolTypeLinkMap )
				break;
			else if( !harbol_string_cmpstr(&sectionstr, &targetstr) )
				retmap = var->Val.LinkMapPtr;
			else itermap = var->Val.LinkMapPtr;
		}
		harbol_string_del(&sectionstr);
		harbol_string_del(&targetstr);
		return retmap;
	}
}

HARBOL_EXPORT struct HarbolString *harbol_cfg_get_str_by_key(struct HarbolLinkMap *const cfgmap, const char key[])
{
	if( !cfgmap || !key )
		return NULL;
	
	const struct HarbolVariant *restrict var = NULL;
	// first check if we're getting a singular value OR we iterate through a sectional path.
	const bool has_dot_path = strchr(key, '.') != NULL;
	if( !has_dot_path ) {
		var = harbol_linkmap_get(cfgmap, key).VarPtr;
		return ( !var || var->TypeTag != HarbolTypeString ) ? NULL : var->Val.StrObjPtr;
	}
	// ok, not a singular value, iterate to the specific linkmap section then.
	else {
		// parse the target key first.
		const char *iter = key;
		struct HarbolString
			sectionstr = {NULL, 0},
			targetstr = {NULL, 0}
		;
		harbol_cfg_ParseTargetStringPath(key, &targetstr);
		struct HarbolLinkMap *itermap = cfgmap;
		struct HarbolString *restrict retstr = NULL;
		
		while( !retstr ) {
			harbol_string_del(&sectionstr);
			while( *iter && *iter != '.' )
				harbol_string_add_char(&sectionstr, *iter++);
			if( *iter )
				iter++;
			
			var = harbol_linkmap_get(itermap, sectionstr.CStr).VarPtr;
			
			if( !var )
				break;
			else if( var->TypeTag==HarbolTypeString && !harbol_string_cmpstr(&sectionstr, &targetstr) )
				retstr = var->Val.StrObjPtr;
			else if( var->TypeTag==HarbolTypeLinkMap )
				itermap = var->Val.LinkMapPtr;
			else break;
		}
		harbol_string_del(&sectionstr);
		harbol_string_del(&targetstr);
		return retstr;
	}
}

HARBOL_EXPORT double harbol_cfg_get_float_by_key(struct HarbolLinkMap *const cfgmap, const char key[])
{
	if( !cfgmap || !key )
		return 0.0;
	
	const struct HarbolVariant *restrict var = NULL;
	// first check if we're getting a singular value OR we iterate through a sectional path.
	const bool has_dot_path = strchr(key, '.') != NULL;
	if( !has_dot_path ) {
		var = harbol_linkmap_get(cfgmap, key).VarPtr;
		return ( !var || var->TypeTag != HarbolTypeFloat ) ? 0.0 : var->Val.Double;
	}
	// ok, not a singular value, iterate to the specific linkmap section then.
	else {
		// parse the target key first.
		const char *iter = key;
		struct HarbolString
			sectionstr = {NULL, 0},
			targetstr = {NULL, 0}
		;
		harbol_cfg_ParseTargetStringPath(key, &targetstr);
		struct HarbolLinkMap *itermap = cfgmap;
		double retval = 0.0;
		
		while( itermap ) {
			harbol_string_del(&sectionstr);
			while( *iter && *iter != '.' )
				harbol_string_add_char(&sectionstr, *iter++);
			if( *iter )
				iter++;
			
			var = harbol_linkmap_get(itermap, sectionstr.CStr).VarPtr;
			
			if( !var )
				break;
			else if( var->TypeTag==HarbolTypeFloat && !harbol_string_cmpstr(&sectionstr, &targetstr) ) {
				retval = var->Val.Double;
				break;
			}
			else if( var->TypeTag==HarbolTypeLinkMap )
				itermap = var->Val.LinkMapPtr;
		}
		harbol_string_del(&sectionstr);
		harbol_string_del(&targetstr);
		return retval;
	}
}

HARBOL_EXPORT int64_t harbol_cfg_get_int_by_key(struct HarbolLinkMap *const cfgmap, const char key[])
{
	if( !cfgmap || !key )
		return 0LL;
	
	const struct HarbolVariant *restrict var = NULL;
	// first check if we're getting a singular value OR we iterate through a sectional path.
	const bool has_dot_path = strchr(key, '.') != NULL;
	if( !has_dot_path ) {
		var = harbol_linkmap_get(cfgmap, key).VarPtr;
		return ( !var || var->TypeTag != HarbolTypeInt ) ? 0 : var->Val.Int64;
	}
	// ok, not a singular value, iterate to the specific linkmap section then.
	else {
		// parse the target key first.
		const char *iter = key;
		struct HarbolString
			sectionstr = {NULL, 0},
			targetstr = {NULL, 0}
		;
		harbol_cfg_ParseTargetStringPath(key, &targetstr);
		struct HarbolLinkMap *itermap = cfgmap;
		int64_t retval = 0;
		
		while( itermap ) {
			harbol_string_del(&sectionstr);
			while( *iter && *iter != '.' )
				harbol_string_add_char(&sectionstr, *iter++);
			if( *iter )
				iter++;
			
			var = harbol_linkmap_get(itermap, sectionstr.CStr).VarPtr;
			if( !var )
				break;
			else if( var->TypeTag==HarbolTypeInt && !harbol_string_cmpstr(&sectionstr, &targetstr) ) {
				retval = var->Val.Int64;
				break;
			}
			else if( var->TypeTag==HarbolTypeLinkMap )
				itermap = var->Val.LinkMapPtr;
		}
		harbol_string_del(&sectionstr);
		harbol_string_del(&targetstr);
		return retval;
	}
}

HARBOL_EXPORT bool harbol_cfg_get_bool_by_key(struct HarbolLinkMap *const cfgmap, const char key[], bool *const restrict boolref)
{
	if( !cfgmap || !key || !boolref )
		return false;
	
	const struct HarbolVariant *restrict var = NULL;
	// first check if we're getting a singular value OR we iterate through a sectional path.
	const bool has_dot_path = strchr(key, '.') != NULL;
	if( !has_dot_path ) {
		var = harbol_linkmap_get(cfgmap, key).VarPtr;
		if( !var || var->TypeTag != HarbolTypeBool )
			return false;
		*boolref = var->Val.Bool;
		return true;
	}
	// ok, not a singular value, iterate to the specific linkmap section then.
	else {
		// parse the target key first.
		const char *iter = key;
		struct HarbolString
			sectionstr = {NULL, 0},
			targetstr = {NULL, 0}
		;
		harbol_cfg_ParseTargetStringPath(key, &targetstr);
		struct HarbolLinkMap *itermap = cfgmap;
		bool success = false;
		
		while( itermap ) {
			harbol_string_del(&sectionstr);
			while( *iter && *iter != '.' )
				harbol_string_add_char(&sectionstr, *iter++);
			if( *iter )
				iter++;
			
			var = harbol_linkmap_get(itermap, sectionstr.CStr).VarPtr;
			
			if( !var )
				break;
			else if( var->TypeTag==HarbolTypeBool && !harbol_string_cmpstr(&sectionstr, &targetstr) ) {
				*boolref = var->Val.Bool;
				success = true;
				break;
			}
			else if( var->TypeTag==HarbolTypeLinkMap )
				itermap = var->Val.LinkMapPtr;
		}
		harbol_string_del(&sectionstr);
		harbol_string_del(&targetstr);
		return success;
	}
}

HARBOL_EXPORT bool harbol_cfg_get_color_by_key(struct HarbolLinkMap *const cfgmap, const char key[], union HarbolColor *const restrict colorref)
{
	if( !cfgmap || !key || !colorref )
		return false;
	
	const struct HarbolVariant *restrict var = NULL;
	// first check if we're getting a singular value OR we iterate through a sectional path.
	const bool has_dot_path = strchr(key, '.') != NULL;
	if( !has_dot_path ) {
		var = harbol_linkmap_get(cfgmap, key).VarPtr;
		return ( !var || var->TypeTag != HarbolTypeColor ) ? false : harbol_tuple_to_struct(var->Val.TuplePtr, colorref);
	}
	// ok, not a singular value, iterate to the specific linkmap section then.
	else {
		// parse the target key first.
		const char *iter = key;
		struct HarbolString
			sectionstr = {NULL, 0},
			targetstr = {NULL, 0}
		;
		harbol_cfg_ParseTargetStringPath(key, &targetstr);
		struct HarbolLinkMap *itermap = cfgmap;
		bool success = false;
		
		while( itermap ) {
			harbol_string_del(&sectionstr);
			while( *iter && *iter != '.' )
				harbol_string_add_char(&sectionstr, *iter++);
			if( *iter )
				iter++;
			
			var = harbol_linkmap_get(itermap, sectionstr.CStr).VarPtr;
			if( !var )
				break;
			else if( var->TypeTag==HarbolTypeColor && !harbol_string_cmpstr(&sectionstr, &targetstr) ) {
				success = harbol_tuple_to_struct(var->Val.TuplePtr, colorref);
				break;
			}
			else if( var->TypeTag==HarbolTypeLinkMap )
				itermap = var->Val.LinkMapPtr;
		}
		harbol_string_del(&sectionstr);
		harbol_string_del(&targetstr);
		return success;
	}
}


HARBOL_EXPORT bool harbol_cfg_get_vec4D_by_key(struct HarbolLinkMap *const cfgmap, const char key[], union HarbolVec4D *const restrict vec4ref)
{
	if( !cfgmap || !key || !vec4ref )
		return false;
	
	const struct HarbolVariant *restrict var = NULL;
	// first check if we're getting a singular value OR we iterate through a sectional path.
	const bool has_dot_path = strchr(key, '.') != NULL;
	if( !has_dot_path ) {
		var = harbol_linkmap_get(cfgmap, key).VarPtr;
		return ( !var || var->TypeTag != HarbolTypeVec4D ) ? false : harbol_tuple_to_struct(var->Val.TuplePtr, vec4ref);
	}
	// ok, not a singular value, iterate to the specific linkmap section then.
	else {
		// parse the target key first.
		const char *iter = key;
		struct HarbolString
			sectionstr = {NULL, 0},
			targetstr = {NULL, 0}
		;
		harbol_cfg_ParseTargetStringPath(key, &targetstr);
		struct HarbolLinkMap *itermap = cfgmap;
		bool success = false;
		
		while( itermap ) {
			harbol_string_del(&sectionstr);
			while( *iter && *iter != '.' )
				harbol_string_add_char(&sectionstr, *iter++);
			if( *iter )
				iter++;
			
			var = harbol_linkmap_get(itermap, sectionstr.CStr).VarPtr;
			if( !var )
				break;
			else if( var->TypeTag==HarbolTypeVec4D && !harbol_string_cmpstr(&sectionstr, &targetstr) ) {
				success = harbol_tuple_to_struct(var->Val.TuplePtr, vec4ref);
				break;
			}
			else if( var->TypeTag==HarbolTypeLinkMap )
				itermap = var->Val.LinkMapPtr;
		}
		harbol_string_del(&sectionstr);
		harbol_string_del(&targetstr);
		return success;
	}
}
