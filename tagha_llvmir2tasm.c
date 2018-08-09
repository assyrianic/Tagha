#include <stdio.h>
#include <stdlib.h>
#include "tagha_llvmir2tasm.h"

static inline bool IsSpace(const char c)
{
	return( c == ' ' or c == '\t' or c == '\r' or c == '\v' or c == '\f' );
}

static inline bool IsDecimal(const char c)
{
	return( c >= '0' and c <= '9' );
}

static inline bool IsHex(const char c)
{
	return( (c >= 'a' and c <= 'f') or (c >= 'A' and c <= 'F') or IsDecimal(c) );
}

static inline bool IsBinary(const char c)
{
	return( c=='0' or c=='1' );
}

static inline bool IsOctal(const char c)
{
	return( c >= '0' and c <= '7' );
}

static inline bool IsPossibleIdentifier(const char c)
{
	return( (c >= 'a' and c <= 'z')
		or (c >= 'A' and c <= 'Z')
		or c == '_'
		or (c >= '0' and c <= '9')
		or c < -1 );
}

static inline bool IsAlphabetic(const char c)
{
	return( (c >= 'a' and c <= 'z')
		or (c >= 'A' and c <= 'Z')
		or c == '_'
		or c < -1 );
}

static bool SkipWhiteSpace(char **restrict strRef)
{
	if( !*strRef or !**strRef )
		return false;
	
	while( **strRef and IsSpace(**strRef) )
		(*strRef)++;
	return **strRef != 0;
}

static bool SkipToChar(char **restrict strRef, const char c)
{
	if( !*strRef or !**strRef )
		return false;
	
	while( **strRef and **strRef != c )
		(*strRef)++;
	return **strRef != 0;
}

static bool LexIdentifier(char **restrict strRef, struct String *const restrict strobj)
{
	if( !*strRef or !**strRef or !strobj )
		return false;
	
	String_Del(strobj);
	String_AddChar(strobj, *(*strRef)++);
	while( **strRef and IsPossibleIdentifier(**strRef) )
		String_AddChar(strobj, *(*strRef)++);
	
	return strobj->Len > 0;
}

static bool LexNumber(char **restrict strRef, struct String *const restrict strobj)
{
	if( !*strRef or !**strRef or !strobj )
		return false;
	
	String_Del(strobj);
	if( **strRef=='0' ) {
		if( (*strRef)[1]=='x' or (*strRef)[1]=='X' ) { // hexadecimal.
			String_AddStr(strobj, (*strRef)[1]=='x' ? "0x" : "0X");
			(*strRef) += 2;
			while( **strRef and IsHex(**strRef) )
				String_AddChar(strobj, *(*strRef)++);
		}
		else if( (*strRef)[1]=='b' or (*strRef)[1]=='B' ) { // binary.
			String_AddStr(strobj, (*strRef)[1]=='b' ? "0b" : "0B");
			(*strRef) += 2;
			while( **strRef and **strRef=='1' or **strRef=='0' )
				String_AddChar(strobj, *(*strRef)++);
		}
		else { // octal.
			String_AddChar(strobj, *(*strRef)++);
			while( **strRef and IsOctal(**strRef) )
				String_AddChar(strobj, *(*strRef)++);
		}
	}
	else {
		while( **strRef and IsDecimal(**strRef) )
			String_AddChar(strobj, *(*strRef)++);
	}
	return strobj->Len > 0;
}

/*
void LLVM2Tagha_Init(struct LLVMTaghaTranspiler *const restrict tp, const char *code)
{
	if( !tp or !code )
		return;
	
	tp->Src = tp->Iter = code;
	tp->CurrLine = 1;
}
*/

static void ParseDeclareNative(struct LLVMTaghaTranspiler *const restrict tp)
{
	if( !tp or !tp->Dest )
		return;
	
	SkipToChar(&tp->Iter, '@'); tp->Iter++;
	LexIdentifier(&tp->Iter, tp->Lexeme);
	if( strstr(tp->Lexeme->CStr, "llvm") != NULL )
		return;
	fprintf(tp->Dest, "$native %%%s\n", tp->Lexeme->CStr);
	LinkMap_Insert(&tp->FuncTable, tp->Lexeme->CStr, (union Value){.Bool=true});
}

static void ParseDefinedFunc(struct LLVMTaghaTranspiler *const restrict tp)
{
	if( !tp or !tp->Dest )
		return;
	
	SkipToChar(&tp->Iter, '@'); tp->Iter++;
	LexIdentifier(&tp->Iter, tp->Lexeme);
	if( strstr(tp->Lexeme->CStr, "llvm") != NULL )
		return;
	
	fprintf(tp->Dest, "\n%%%s: {\n}\n", tp->Lexeme->CStr);
	LinkMap_Insert(&tp->FuncTable, tp->Lexeme->CStr, (union Value){.Bool=true});
}


// void (%struct.Tagha*, %union.Value*, i32, %union.Value*)*
// func ptrs can also take func ptrs in and of themselves:
// %struct.structype = type { void (float, i16)*, void (float, void (float, i16)*)* }
static void SkipFuncPtrs(struct LLVMTaghaTranspiler *const restrict tp)
{
	if( !tp )
		return;
	while( *tp->Iter and *tp->Iter != ')' ) {
		if( IsAlphabetic(*tp->Iter) ) {
			LexIdentifier(&tp->Iter, tp->Lexeme);
			SkipWhiteSpace(&tp->Iter);
			if( *tp->Iter=='*' ) {
				// adjust for multi-level pointers.
				while( *tp->Iter and *tp->Iter=='*' )
					tp->Iter++;
			}
			// check if it's a function pointer...
			// void (%struct.Tagha*, %union.Value*, i32, %union.Value*)*
			if( *tp->Iter == '(' or !String_NCmpCStr(tp->Lexeme, "void", sizeof "void") ) {
				SkipFuncPtrs(tp);
			}
		}
		else if( *tp->Iter=='%' ) {
			tp->Iter++; LexIdentifier(&tp->Iter, tp->Lexeme);
			tp->Iter++; LexIdentifier(&tp->Iter, tp->Lexeme);
			if( *tp->Iter=='*' ) {
				// adjust for multi-level pointers.
				while( *tp->Iter and *tp->Iter=='*' )
					tp->Iter++;
			}
		}
		else if( *tp->Iter=='[' ) {
			tp->Iter++;
			ParseArrays(tp->Iter);
		}
		tp->Iter++;
	}
	tp->Iter++;
	if( *tp->Iter=='*' )
		tp->Iter++;
}


// [40 x i8]
// [3 x [3 x i16]] - 2D array of [3][3]int16.
static size_t ParseArrays(struct LLVMTaghaTranspiler *const restrict tp)
{
	size_t array_size = 0;
	if( !tp )
		return array_size;
	
	LexNumber(&tp->Iter, tp->Lexeme);
	size_t num = strtoul(tp->Lexeme->CStr, NULL, 0);
	size_t basicsizes = 0;
	tp->Iter += 3; // skip past the 'x'
	if( IsAlphabetic(*tp->Iter) ) {
		LexIdentifier(&tp->Iter, tp->Lexeme);
		if( !String_NCmpCStr(tp->Lexeme, "i64", sizeof "i64") )
			basicsizes = num * sizeof(int64_t);
		else if( !String_NCmpCStr(tp->Lexeme, "i32", sizeof "i32") )
			basicsizes = num * sizeof(int32_t);
		else if( !String_NCmpCStr(tp->Lexeme, "i16", sizeof "i16") )
			basicsizes = num * sizeof(int16_t);
		else if( !String_NCmpCStr(tp->Lexeme, "i8", sizeof "i8") )
			basicsizes = num * sizeof(int8_t);
		else if( !String_NCmpCStr(tp->Lexeme, "float", sizeof "float") )
			basicsizes = num * sizeof(float);
		else if( !String_NCmpCStr(tp->Lexeme, "double", sizeof "double") )
			basicsizes = num * sizeof(double);
		
		SkipWhiteSpace(&tp->Iter);
		// POINTER!!!!!!!!1
		if( *tp->Iter=='*' ) {
			// adjust for multi-level pointers.
			while( *tp->Iter and *tp->Iter=='*' )
				tp->Iter++;
			basicsizes = num * 8; // Tagha pointers are 8 bytes.
		}
		// check if it's a function pointer...
		// void (%struct.Tagha*, %union.Value*, i32, %union.Value*)*
		if( *tp->Iter == '(' or !String_NCmpCStr(tp->Lexeme, "void", sizeof "void") ) {
			SkipFuncPtrs(tp);
			basicsizes = num * 8;
		}
		array_size += basicsizes;
	}
	else if( *tp->Iter=='%' ) {
		struct String *subtype = &(struct String){0};
		// add the percent and type set
		LexIdentifier(&tp->Iter, tp->Lexeme);
		String_Add(subtype, tp->Lexeme);
		// add the dot.
		String_AddChar(subtype, *tp->Iter++);
		// add the type name.
		LexIdentifier(&tp->Iter, tp->Lexeme);
		String_Add(subtype, tp->Lexeme);
		size_t typesize = 0;
		if( *tp->Iter=='*' ) {
			// adjust for multi-level pointers.
			while( *tp->Iter and *tp->Iter=='*' )
				tp->Iter++;
			typesize = 8; // Tagha pointers are 8 bytes.
		}
		else typesize = LinkMap_Get(&tp->TypeTable, subtype->CStr).UInt32Array[1];
		array_size += num * typesize;
		String_Del(subtype);
	}
	else if( *tp->Iter=='[' )
		array_size = num * ParseArrays(tp);
	
	return array_size;
}

// idea, make this recursive?
static void ParseLLVMTypes(struct LLVMTaghaTranspiler *const restrict tp)
{
	if( !tp )
		return;
	
	struct String *typename = &(struct String){0};
	LexIdentifier(&tp->Iter, tp->Lexeme);
	String_Add(typename, tp->Lexeme);
	// %struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i32, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i32, i32, [40 x i8] }
	// %struct.Player = type { %struct.Tagha, float, i32, i32 }
	// %union.Value = type { [2 x i32] }
	if( !String_NCmpCStr(tp->Lexeme, "%struct", sizeof "%struct") or !String_NCmpCStr(tp->Lexeme, "union", sizeof "union") ) {
		LexIdentifier(&tp->Iter, tp->Lexeme);
		String_Add(typename, tp->Lexeme);
		union Value typedatum;
		typedatum.UInt32Array[0] = !String_NCmpCStr(typename, "%struct", sizeof "%struct");
		typedatum.UInt32Array[1] = 0;
		LinkMap_Insert(&tp->TypeTable, typename, typedatum);
		String_Add(typename, tp->Lexeme);
		// read its data!
		size_t structbytes = 0;
		SkipToChar(&tp->Iter, '{');
		while( *tp->Iter and *tp->Iter != '}' ) {
			SkipWhiteSpace(&tp->Iter);
			// process basic types
			if( IsAlphabetic(*tp->Iter) ) {
				size_t basicsizes = 0;
				LexIdentifier(&tp->Iter, tp->Lexeme);
				if( !String_NCmpCStr(tp->Lexeme, "i64", sizeof "i64") )
					basicsizes = sizeof(int64_t);
				else if( !String_NCmpCStr(tp->Lexeme, "i32", sizeof "i32") )
					basicsizes = sizeof(int32_t);
				else if( !String_NCmpCStr(tp->Lexeme, "i16", sizeof "i16") )
					basicsizes = sizeof(int16_t);
				else if( !String_NCmpCStr(tp->Lexeme, "i8", sizeof "i8") )
					basicsizes = sizeof(int8_t);
				else if( !String_NCmpCStr(tp->Lexeme, "float", sizeof "float") )
					basicsizes = sizeof(float);
				else if( !String_NCmpCStr(tp->Lexeme, "double", sizeof "double") )
					basicsizes = sizeof(double);
				
				SkipWhiteSpace(&tp->Iter);
				// POINTER!!!!!!!!1
				if( *tp->Iter=='*' ) {
					// adjust for multi-level pointers.
					while( *tp->Iter and *tp->Iter=='*' )
						tp->Iter++;
					basicsizes = 8; // Tagha pointers are 8 bytes.
				}
				// check if it's a function pointer...
				// void (%struct.Tagha*, %union.Value*, i32, %union.Value*)*
				if( *tp->Iter == '(' or !String_NCmpCStr(tp->Lexeme, "void", sizeof "void") ) {
					SkipFuncPtrs(tp);
					basicsizes = 8;
				}
				structbytes += basicsizes;
			}
			// process array
			// [2 x i32]
			else if( *tp->Iter=='[' ) {
				tp->Iter++;
				structbytes += ParseArrays(tp);
				tp->Iter++;
			}
			// process struct/union scalar.
			// %struct.Tagha
			else if( *tp->Iter=='%' ) {
				struct String *subtype = &(struct String){0};
				// add the percent and type set
				LexIdentifier(&tp->Iter, tp->Lexeme);
				String_Add(subtype, tp->Lexeme);
				// add the dot.
				String_AddChar(subtype, *tp->Iter++);
				// add the type name.
				LexIdentifier(&tp->Iter, tp->Lexeme);
				String_Add(subtype, tp->Lexeme);
				size_t typesize = 0;
				if( *tp->Iter=='*' ) {
					// adjust for multi-level pointers.
					while( *tp->Iter and *tp->Iter=='*' )
						tp->Iter++;
					typesize = 8; // Tagha pointers are 8 bytes.
				}
				else typesize = LinkMap_Get(&tp->TypeTable, subtype->CStr).UInt32Array[1];
				structbytes += typesize;
				String_Del(subtype);
			}
			tp->Iter += 2;
		}
		LinkMap_Get(&tp->TypeTable, typename).UInt32Array[1] = structbytes;
		String_Del(typename);
	}
}

static void ParseGlobalDefines(struct LLVMTaghaTranspiler *const restrict tp)
{
	if( !tp or !tp->Dest )
		return;
	
	struct String *AnonStr = &(struct String){0};
	tp->Lexeme = &(struct String){0};
	// possibly an anonymous string?
	if( *tp->Iter=='.' ) {
		tp->Iter++;
		LexIdentifier(&tp->Iter, tp->Lexeme);
		/* definitely anonymous string, parse it!
		 * usually strings in llvm bitcode are like: "@.str" if that's the first string stored.
		 * for more strings, llvm bitcode stores them as "@.str.#" where '#' is a number.
		 * A typical anonymous string in llvm looks like this:
		 * '@.str.2 = private unnamed_addr constant [5 x i8] c"puts\00", align 1'
		 */
		if( !String_NCmpCStr(tp->Lexeme, "str", sizeof "str") ) {
			String_Add(AnonStr, tp->Lexeme);
			// add the other '.'
			if( *tp->Iter=='.' ) {
				tp->Iter++;
				LexNumber(&tp->Iter, tp->Lexeme);
				String_Add(AnonStr, tp->Lexeme);
			}
			// go to the string's size.
			SkipToChar(&tp->Iter, '[');
			tp->Iter++;
			LexNumber(&tp->Iter, tp->Lexeme);
			size_t strbytes = strtoul(tp->Lexeme->CStr, NULL, 10);
			String_Del(tp->Lexeme);
			SkipToChar(&tp->Iter, 'x'); // go to x multiplier symbol
			tp->Iter += 2;
			if( !strncmp(tp->Iter, "i8", 2) )
				strbytes *= sizeof(int8_t);
			else if( !strncmp(tp->Iter, "i16", 3) )
				strbytes *= sizeof(int16_t);
			else if( !strncmp(tp->Iter, "i32", 3) )
				strbytes *= sizeof(int32_t);
			else if( !strncmp(tp->Iter, "i64", 3) )
				strbytes *= sizeof(int64_t);
			
			SkipToChar(&tp->Iter, '"');
			const char quote = *tp->Iter++;
			while( *tp->Iter and *tp->Iter != quote ) {
				const char charval = *tp->Iter++;
				// handle escape chars.
				if( charval=='\\' ) {
					char byteval[3] = {0};
					byteval[0] = *tp->Iter++;
					byteval[1] = *tp->Iter++;
					const char esc = strtol(byteval, NULL, 16);
					switch( esc ) {
						case 7: String_AddStr(tp->Lexeme, "\\a"); break;
						case 9: String_AddStr(tp->Lexeme, "\\t"); break;
						case 10: String_AddStr(tp->Lexeme, "\\n"); break;
						case 11: String_AddStr(tp->Lexeme, "\\v"); break;
						case 12: String_AddStr(tp->Lexeme, "\\f"); break;
						case 13: String_AddStr(tp->Lexeme, "\\r"); break;
					}
				}
				else String_AddChar(tp->Lexeme, charval);
			}
			fprintf(tp->Dest, "$global %s, %zu, \"%s\"\n", AnonStr->CStr, strbytes, tp->Lexeme->CStr);
		}
	}
	// is a global variable or static local variable.
	else if( IsAlphabetic(*tp->Iter) ) {
		LexIdentifier(&tp->Iter, tp->Lexeme);
		const bool isLocal = LinkMap_HasKey(&tp->FuncTable, tp->Lexeme->CStr);
		// possibly a string assigned to either a local array or static local var.
		// handle cases like: '@main.i = private unnamed_addr constant [22 x i8] c"hello from main argv!\00", align 1'
		
		// @main.host_natives = private unnamed_addr constant [5 x %struct.NativeInfo] [%struct.NativeInfo { i8* getelementptr inbounds ([5 x i8], [5 x i8]* @.str.2, i32 0, i32 0), void (%struct.Tagha*, %union.Value*, i32, %union.Value*)* @Native_puts }, %struct.NativeInfo { i8* getelementptr inbounds ([6 x i8], [6 x i8]* @.str.3, i32 0, i32 0), void (%struct.Tagha*, %union.Value*, i32, %union.Value*)* @Native_fgets }, %struct.NativeInfo { i8* getelementptr inbounds ([7 x i8], [7 x i8]* @.str.4, i32 0, i32 0), void (%struct.Tagha*, %union.Value*, i32, %union.Value*)* @Native_strlen }, %struct.NativeInfo { i8* getelementptr inbounds ([7 x i8], [7 x i8]* @.str.5, i32 0, i32 0), void (%struct.Tagha*, %union.Value*, i32, %union.Value*)* @Native_AddOne }, %struct.NativeInfo zeroinitializer]
		if( isLocal and *tp->Iter=='.' ) {
			tp->Iter++;
			LexIdentifier(&tp->Iter, tp->Lexeme);
			String_Add(AnonStr, tp->Lexeme);
			SkipToChar(&tp->Iter, '=');
			tp->Iter += 2; LexIdentifier(&tp->Iter, tp->Lexeme);
			tp->Iter++; LexIdentifier(&tp->Iter, tp->Lexeme);
			tp->Iter++; LexIdentifier(&tp->Iter, tp->Lexeme);
			tp->Iter++;
			
			// if [ then it's an array type.
			if( *tp->Iter=='[' ) {
				tp->Iter++;
				LexNumber(&tp->Iter, tp->Lexeme);
				size_t strbytes = strtoul(tp->Lexeme->CStr, NULL, 10);
				String_Del(tp->Lexeme);
				SkipToChar(&tp->Iter, 'x'); // go to x multiplier symbol
				tp->Iter += 2;
				if( !strncmp(tp->Iter, "i8", 2) )
					strbytes *= sizeof(int8_t);
				else if( !strncmp(tp->Iter, "i16", 3) )
					strbytes *= sizeof(int16_t);
				else if( !strncmp(tp->Iter, "i32", 3) )
					strbytes *= sizeof(int32_t);
				else if( !strncmp(tp->Iter, "i64", 3) )
					strbytes *= sizeof(int64_t);
				
				SkipToChar(&tp->Iter, '"');
				const char quote = *tp->Iter++;
				while( *tp->Iter and *tp->Iter != quote ) {
					const char charval = *tp->Iter++;
					// handle escape chars.
					if( charval=='\\' ) {
						char byteval[3] = {0};
						byteval[0] = *tp->Iter++;
						byteval[1] = *tp->Iter++;
						const char esc = strtol(byteval, NULL, 16);
						switch( esc ) {
							case 7: String_AddStr(tp->Lexeme, "\\a"); break;
							case 9: String_AddStr(tp->Lexeme, "\\t"); break;
							case 10: String_AddStr(tp->Lexeme, "\\n"); break;
							case 11: String_AddStr(tp->Lexeme, "\\v"); break;
							case 12: String_AddStr(tp->Lexeme, "\\f"); break;
							case 13: String_AddStr(tp->Lexeme, "\\r"); break;
						}
					}
					else String_AddChar(tp->Lexeme, charval);
				}
				fprintf(tp->Dest, "$global %s, %zu, \"%s\"\n", AnonStr->CStr, strbytes, tp->Lexeme->CStr);
			}
		}
		// it's a normal global variable.
		else {
			//String_Add(AnonStr, tp->Lexeme);
		}
	}
	String_Del(AnonStr);
}


void LLVM2Tagha_Transpile(struct LLVMTaghaTranspiler *const restrict tp)
{
	if( !tp or !tp->Src )
		return;
	
#define MAX_LINE_CHARS 2048
	char line_buffer[MAX_LINE_CHARS] = {0};
	
	// set up the .tasm file.
	char *iter = tp->OutputName.CStr;
	size_t len = 0;
	while( *++iter ); // go to null terminator.
	while( *--iter != '.' ) // now go backwards to the '.' denoting file type.
		++len;
	
	memset(iter, 0, len); // nullify the dot and following chars.
	
	// use line_buffer instead of wasting more stack space.
	memset(line_buffer, 0, sizeof line_buffer);
	sprintf(line_buffer, "%.2000s.tasm", tp->OutputName.CStr);
	tp->Dest = fopen(line_buffer, "w");
	if( !tp->Dest ) {
		printf("llvm2tagha error: cannot create file '%s'\n", line_buffer);
		exit(-1);
	}
	fprintf(tp->Dest, "; .tasm file generated by 'llvm2tagha', an llvm bitcode to tagha assembly converter tool.\n; written by Assyrianic @ 'https://github.com/assyrianic' for the Tagha Virtual Machine Project.\n\n");
	
#ifdef LLVM2TAGHA_DEBUG
	puts("\nllvm2tagha: first check Begin!\n");
#endif
	
	tp->Lexeme = &(struct String){0};
	
	for( tp->Iter = fgets(line_buffer, MAX_LINE_CHARS, tp->Src) ; tp->Iter ; tp->Iter = fgets(line_buffer, MAX_LINE_CHARS, tp->Src) ) {
		// set up first line for error checks.
		tp->CurrLine++;
		while( *tp->Iter ) {
			String_Del(tp->Lexeme);
			// skip whitespace.
			SkipWhiteSpace(&tp->Iter);
			if( *tp->Iter=='\n' or *tp->Iter=='#' )
				break;
			
			else if( *tp->Iter=='d' ) {
				LexIdentifier(&tp->Iter, tp->Lexeme);
				if( !String_NCmpCStr(tp->Lexeme, "define", sizeof "define") )
					ParseDefinedFunc(tp);
				else if( !String_NCmpCStr(tp->Lexeme, "declare", sizeof "declare") )
					ParseDeclareNative(tp);
				break;
			}
			// parse the types so we can calculate sizes.
			else if( *tp->Iter=='%' ) {
				ParseLLVMTypes(tp);
				break;
			}
			tp->Iter++;
		}
	}
	// do a second run so we can properly parse static local vars.
	rewind(tp->Src);
#ifdef LLVM2TAGHA_DEBUG
	puts("\nllvm2tagha: first check End!\n");
#endif
	
#ifdef LLVM2TAGHA_DEBUG
	for( struct LinkNode *x=tp->TypeTable.Head ; x ; x=x->After ) {
		printf("TypeTable ={ %s : %u }", );
	}
#endif
	
	
#ifdef LLVM2TAGHA_DEBUG
	puts("\nllvm2tagha: second check Begin!\n");
#endif
	tp->CurrLine = 0;
	for( tp->Iter = fgets(line_buffer, MAX_LINE_CHARS, tp->Src) ; tp->Iter ; tp->Iter = fgets(line_buffer, MAX_LINE_CHARS, tp->Src) ) {
		// set up first line for error checks.
		tp->CurrLine++;
		while( *tp->Iter ) {
			String_Del(tp->Lexeme);
			// skip whitespace.
			SkipWhiteSpace(&tp->Iter);
			if( *tp->Iter=='\n' or *tp->Iter=='#' )
				break;
			// parse the types AGAIN so we can complete calculating sizes.
			else if( *tp->Iter=='%' ) {
				ParseLLVMTypes(tp);
				break;
			}
			else if( *tp->Iter=='@' ) {
				tp->Iter++;
				ParseGlobalDefines(tp);
				break;
			}
			tp->Iter++;
		}
	}
#ifdef LLVM2TAGHA_DEBUG
	puts("\nllvm2tagha: second check End!\n");
#endif
	
	String_Del(tp->Lexeme);
	fclose(tp->Dest); tp->Dest=NULL;
	fclose(tp->Src); tp->Src=NULL;
}

int main(int argc, char *argv[restrict static argc])
{
	if( argc<=1 )
		return -1;
	
	for( int i=1 ; i<argc ; i++ ) {
		FILE *llvmfile = fopen(argv[i], "r");
		if( !llvmfile )
			continue;
		
		struct LLVMTaghaTranspiler *restrict tp = &(struct LLVMTaghaTranspiler){0};
		tp->Src = llvmfile;
		String_InitStr(&tp->OutputName, argv[i]);
		LLVM2Tagha_Transpile(tp);
	}
}
