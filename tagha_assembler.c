#include <stdio.h>
#include <stdlib.h>
#include "tagha_assembler.h"


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
	/*
	if( **strRef=='.' ) { // add float support.
		String_AddChar(strobj, *(*strRef)++);
		while( **strRef and IsDecimal(**strRef) )
			String_AddChar(strobj, *(*strRef)++);
	}
	*/
	return strobj->Len > 0;
}

// $stacksize <number>
bool TaghaAsm_ParseStackDirective(struct TaghaAsmbler *const restrict tasm)
{
	if( !tasm or !tasm->Src or !tasm->Iter )
		return false;
	
	SkipWhiteSpace(&tasm->Iter);
	if( !IsDecimal(*tasm->Iter) ) {
		printf("tasm error: stack size directive requires a valid number! line: %zu\n", tasm->CurrLine);
		exit(-1);
	}
	LexNumber(&tasm->Iter, tasm->Lexeme);
	const bool isbinary = !String_NCmpCStr(tasm->Lexeme, "0b", 2) or !String_NCmpCStr(tasm->Lexeme, "0B", 2) ? true : false;
	tasm->Stacksize = strtoul(isbinary ? tasm->Lexeme->CStr+2 : tasm->Lexeme->CStr, NULL, isbinary ? 2 : 0);
	return true;
}

// $global varname bytes ...
bool TaghaAsm_ParseGlobalVarDirective(struct TaghaAsmbler *const restrict tasm)
{
	if( !tasm or !tasm->Src or !tasm->Iter )
		return false;
	
	SkipWhiteSpace(&tasm->Iter);
	if( !IsAlphabetic(*tasm->Iter) ) {
		printf("tasm error: global directive requires the 1st argument to be a variable name! line: %zu\n", tasm->CurrLine);
		exit(-1);
	}
	
	struct String *varname = &(struct String){0};
	LexIdentifier(&tasm->Iter, varname);
	SkipWhiteSpace(&tasm->Iter);
	if( *tasm->Iter==',' )
		tasm->Iter++;
	SkipWhiteSpace(&tasm->Iter);
	
	if( !IsDecimal(*tasm->Iter) ) {
		printf("tasm error: missing byte size number in global directive on line: %zu\n", tasm->CurrLine);
		exit(-1);
	}
	LexNumber(&tasm->Iter, tasm->Lexeme);
	const bool isbinary = !String_NCmpCStr(tasm->Lexeme, "0b", 2) or !String_NCmpCStr(tasm->Lexeme, "0B", 2) ? true : false;
	size_t bytes = strtoull(isbinary ? tasm->Lexeme->CStr+2 : tasm->Lexeme->CStr, NULL, isbinary ? 2 : 0);
	struct ByteBuffer *vardata = ByteBuffer_New();
	if( !vardata ) {
		puts("tasm error: out of memory!");
		exit(-1);
	}
	SkipWhiteSpace(&tasm->Iter);
	if( *tasm->Iter==',' )
		tasm->Iter++;
	
	while( bytes ) {
		SkipWhiteSpace(&tasm->Iter);
		if( *tasm->Iter=='"' ) { // string
			String_Del(tasm->Lexeme);
			const char quote = *tasm->Iter++;
			while( *tasm->Iter and *tasm->Iter != quote ) {
				const char charval = *tasm->Iter++;
				if( !charval ) { // sudden EOF?
					puts("tasm error: sudden EOF in global directive string copying!");
					exit(-1);
				}
				// handle escape chars
				if( charval=='\\' ) {
					const char escape = *tasm->Iter++;
					switch( escape ) {
						case 'a':
							String_AddChar(tasm->Lexeme, '\a'); break;
						case 'n':
							String_AddChar(tasm->Lexeme, '\n'); break;
						case 'r':
							String_AddChar(tasm->Lexeme, '\r'); break;
						case 't':
							String_AddChar(tasm->Lexeme, '\t'); break;
						case 'v':
							String_AddChar(tasm->Lexeme, '\v'); break;
						case 'f':
							String_AddChar(tasm->Lexeme, '\f'); break;
						case '\'':
							String_AddChar(tasm->Lexeme, '\''); break;
						case '"':
							String_AddChar(tasm->Lexeme, '"'); break;
						case '\\':
							String_AddChar(tasm->Lexeme, '\\'); break;
						case '0':
							String_AddChar(tasm->Lexeme, '\0'); break;
					}
				}
				else String_AddChar(tasm->Lexeme, charval);
			}
			ByteBuffer_InsertString(vardata, tasm->Lexeme->CStr, tasm->Lexeme->Len);
			bytes = 0;
		}
		else if( *tasm->Iter==',' )
			tasm->Iter++;
		else if( IsDecimal(*tasm->Iter) ) {
			LexNumber(&tasm->Iter, tasm->Lexeme);
			const bool isbinary = !String_NCmpCStr(tasm->Lexeme, "0b", 2) or !String_NCmpCStr(tasm->Lexeme, "0B", 2) ? true : false;
			const uint64_t data = strtoull(isbinary ? tasm->Lexeme->CStr+2 : tasm->Lexeme->CStr, NULL, isbinary ? 2 : 0);
			if( data ) {
				printf("tasm error: single numeric arguments for global vars must be 0! on line: %zu\n", tasm->CurrLine);
				exit(-1);
			}
			while( bytes-- )
				ByteBuffer_InsertByte(vardata, 0);
			break;
		}
		else if( IsAlphabetic(*tasm->Iter) ) {
			LexIdentifier(&tasm->Iter, tasm->Lexeme);
			if( !String_NCmpCStr(tasm->Lexeme, "byte", sizeof "byte") ) {
				SkipWhiteSpace(&tasm->Iter);
				LexNumber(&tasm->Iter, tasm->Lexeme);
				const bool isbinary = !String_NCmpCStr(tasm->Lexeme, "0b", 2) or !String_NCmpCStr(tasm->Lexeme, "0B", 2) ? true : false;
				const uint8_t data = strtoull(isbinary ? tasm->Lexeme->CStr+2 : tasm->Lexeme->CStr, NULL, isbinary ? 2 : 0);
				ByteBuffer_InsertByte(vardata, data);
				bytes--;
			}
			else if( !String_NCmpCStr(tasm->Lexeme, "half", sizeof "half") ) {
				SkipWhiteSpace(&tasm->Iter);
				LexNumber(&tasm->Iter, tasm->Lexeme);
				const bool isbinary = !String_NCmpCStr(tasm->Lexeme, "0b", 2) or !String_NCmpCStr(tasm->Lexeme, "0B", 2) ? true : false;
				const uint16_t data = strtoull(isbinary ? tasm->Lexeme->CStr+2 : tasm->Lexeme->CStr, NULL, isbinary ? 2 : 0);
				ByteBuffer_InsertInt(vardata, data, sizeof(uint16_t));
				bytes -= sizeof(uint16_t);
			}
			else if( !String_NCmpCStr(tasm->Lexeme, "long", sizeof "long") ) {
				SkipWhiteSpace(&tasm->Iter);
				LexNumber(&tasm->Iter, tasm->Lexeme);
				const bool isbinary = !String_NCmpCStr(tasm->Lexeme, "0b", 2) or !String_NCmpCStr(tasm->Lexeme, "0B", 2) ? true : false;
				const uint32_t data = strtoull(isbinary ? tasm->Lexeme->CStr+2 : tasm->Lexeme->CStr, NULL, isbinary ? 2 : 0);
				ByteBuffer_InsertInt(vardata, data, sizeof(uint32_t));
				bytes -= sizeof(uint32_t);
			}
			else if( !String_NCmpCStr(tasm->Lexeme, "word", sizeof "word") ) {
				SkipWhiteSpace(&tasm->Iter);
				LexNumber(&tasm->Iter, tasm->Lexeme);
				const bool isbinary = !String_NCmpCStr(tasm->Lexeme, "0b", 2) or !String_NCmpCStr(tasm->Lexeme, "0B", 2) ? true : false;
				const uint64_t data = strtoull(isbinary ? tasm->Lexeme->CStr+2 : tasm->Lexeme->CStr, NULL, isbinary ? 2 : 0);
				ByteBuffer_InsertInt(vardata, data, sizeof(uint64_t));
				bytes -= sizeof(uint64_t);
			}
		}
		else {
			printf("tasm error: global var directive data set is incomplete, must be equal to bytesize given. line: %zu\n", tasm->CurrLine);
			exit(-1);
		}
	}
#ifdef TASM_DEBUG
	printf("tasm debug: vardata.Count: %zu\n", vardata->Count);
#endif
	/*
	printf("tasm: adding global var '%s'\n", varname->CStr);
	for( size_t i=0 ; i<ByteBuffer_Count(vardata) ; i++ )
		printf("tasm: global var[%zu] == %u\n", i, vardata->Buffer[i]);
	*/
	LinkMap_Insert(tasm->VarTable, varname->CStr, (union Value){.BufferPtr=vardata});
	String_Del(varname);
	return true;
}

// $native %name
bool TaghaAsm_ParseNativeDirective(struct TaghaAsmbler *const restrict tasm)
{
	if( !tasm or !tasm->Src or !tasm->Iter )
		return false;
	
	SkipWhiteSpace(&tasm->Iter);
	if( *tasm->Iter != '%' ) {
		printf("tasm error: missing %% for native name declaration! line: %zu\n", tasm->CurrLine);
		exit(-1);
	}
	LexIdentifier(&tasm->Iter, tasm->Lexeme);
	
	struct LabelInfo *label = calloc(1, sizeof *label);
	if( !label ) {
		puts("tasm error: OUT OF MEMORY! Exiting...");
		exit(-1);
	}
	label->Addr = 0;
	label->IsFunc = label->IsNativeFunc = true;
	union Value native = (union Value){.Ptr = label};
	LinkMap_Insert(tasm->FuncTable, tasm->Lexeme->CStr, native);
#ifdef TASM_DEBUG
	printf("tasm: added native function '%s'\n", tasm->Lexeme->CStr);
#endif
	return true;
}

int64_t LexImmValue(struct TaghaAsmbler *const restrict tasm)
{
	LexNumber(&tasm->Iter, tasm->Lexeme);
	tasm->ProgramCounter += 8;
	const bool isbinary = !String_NCmpCStr(tasm->Lexeme, "0b", 2) or !String_NCmpCStr(tasm->Lexeme, "0B", 2) ? true : false;
	return strtoll(isbinary ? tasm->Lexeme->CStr+2 : tasm->Lexeme->CStr, NULL, isbinary ? 2 : 0);
}

uint8_t LexRegisterID(struct TaghaAsmbler *const restrict tasm)
{
	LexIdentifier(&tasm->Iter, tasm->Lexeme);
	if( !LinkMap_HasKey(tasm->Registers, tasm->Lexeme->CStr) ) {
		printf("tasm error: invalid register name '%s' on line: %zu\n", tasm->Lexeme->CStr, tasm->CurrLine);
		exit(-1);
	}
	tasm->ProgramCounter++;
	return LinkMap_Get(tasm->Registers, tasm->Lexeme->CStr).UInt64;
}

void LexRegDeref(struct TaghaAsmbler *const restrict tasm, uint8_t *restrict idref, int32_t *offsetref)
{
	tasm->Iter++; // iterate past '['
	tasm->ProgramCounter += 5; // 1 for byte as register id and 4 byte offset.
	SkipWhiteSpace(&tasm->Iter);
	LexIdentifier(&tasm->Iter, tasm->Lexeme);
	if( !LinkMap_HasKey(tasm->Registers, tasm->Lexeme->CStr) ) {
		printf("tasm error: invalid register name '%s' in register indirection on line: %zu\n", tasm->Lexeme->CStr, tasm->CurrLine);
		exit(-1);
	}
	*idref = LinkMap_Get(tasm->Registers, tasm->Lexeme->CStr).UInt64;
	*offsetref = 0;
	
	SkipWhiteSpace(&tasm->Iter);
	// if there's no plus/minus equation, assume `[reg+0]`
	// TODO: allow for scaled indexing like * typesize -> [reg+14*4] for easier array accessing.
	const char closer = *tasm->Iter;
	if( closer != '-' and closer != '+' and closer != ']' ) {
		printf("tasm error: invalid offset math operator '%c' in register indirection on line: %zu\n", closer, tasm->CurrLine);
		exit(-1);
	}
	else if( closer=='-' or closer=='+' ) {
		tasm->Iter++;
		SkipWhiteSpace(&tasm->Iter);
		if( !IsDecimal(*tasm->Iter) ) {
			printf("tasm error: invalid offset '%s' in register indirection on line: %zu\n", tasm->Lexeme->CStr, tasm->CurrLine);
			exit(-1);
		}
		LexNumber(&tasm->Iter, tasm->Lexeme);
		SkipWhiteSpace(&tasm->Iter);
		const bool isbinary = !String_NCmpCStr(tasm->Lexeme, "0b", 2) or !String_NCmpCStr(tasm->Lexeme, "0B", 2) ? true : false;
		int32_t offset = strtol(isbinary ? tasm->Lexeme->CStr+2 : tasm->Lexeme->CStr, NULL, isbinary ? 2 : 0);
		offset = closer=='-' ? -offset : offset;
		//printf("offset == %i\n", offset);
		*offsetref = offset;
	}
	if( *tasm->Iter != ']' ) {
		printf("tasm error: missing closing ']' bracket in register indirection on line: %zu\n", tasm->CurrLine);
		exit(-1);
	}
	tasm->Iter++;
}

int64_t LexLabelValue(struct TaghaAsmbler *const restrict tasm, const bool firstpass)
{
	const bool isfunclbl = *tasm->Iter=='%';
	LexIdentifier(&tasm->Iter, tasm->Lexeme);
	tasm->ProgramCounter += 8;
	if( !firstpass and !LinkMap_HasKey(isfunclbl ? tasm->FuncTable : tasm->LabelTable, tasm->Lexeme->CStr) ) {
		printf("tasm error: undefined label '%s' on line: %zu\n", tasm->Lexeme->CStr, tasm->CurrLine);
		exit(-1);
	}
	if( !firstpass ) {
		struct LabelInfo *label = LinkMap_Get(isfunclbl ? tasm->FuncTable : tasm->LabelTable, tasm->Lexeme->CStr).Ptr;
		if( !isfunclbl ) {
		#ifdef TASM_DEBUG
			printf("label->Addr (%llu) - tasm->ProgramCounter (%zu) == '%lli'\n", label->Addr, tasm->ProgramCounter, label->Addr - tasm->ProgramCounter);
		#endif
			return label->Addr - tasm->ProgramCounter;
		}
		else {
			return label->IsNativeFunc ? -(LinkMap_GetIndexByName(tasm->FuncTable, tasm->Lexeme->CStr) + 1) : (LinkMap_GetIndexByName(tasm->FuncTable, tasm->Lexeme->CStr) + 1);
		}
	}
	return 0;
}



bool TaghaAsm_ParseRegRegInstr(struct TaghaAsmbler *const restrict tasm, const bool firstpass)
{
	if( !tasm or !tasm->Src or !tasm->Iter )
		return false;
	
	tasm->ProgramCounter++;
	SkipWhiteSpace(&tasm->Iter);
	
	if( *tasm->Iter != 'r' ) {
		printf("tasm error: opcode requires a register as 1st operand. line: %zu\n", tasm->CurrLine);
		exit(-1);
	}
	const uint8_t destreg = LexRegisterID(tasm);
	
	// ok, let's read the secondary operand!
	SkipWhiteSpace(&tasm->Iter);
	if( *tasm->Iter==',' )
		tasm->Iter++;
	
	SkipWhiteSpace(&tasm->Iter);
	
	if( *tasm->Iter != 'r' ) {
		printf("tasm error: opcode requires a register as 2nd operand. line: %zu\n", tasm->CurrLine);
		exit(-1);
	}
	const uint8_t srcreg = LexRegisterID(tasm);
	
	if( !firstpass ) {
		struct LabelInfo *label = LinkMap_Get(tasm->FuncTable, tasm->ActiveFuncLabel->CStr).Ptr;
		if( !label ) {
			printf("tasm error: undefined label '%s' on line: %zu\n", tasm->ActiveFuncLabel->CStr, tasm->CurrLine);
			exit(-1);
		}
		ByteBuffer_InsertByte(&label->Bytecode, destreg);
		ByteBuffer_InsertByte(&label->Bytecode, srcreg);
	}
	return true;
}
bool TaghaAsm_ParseOneRegInstr(struct TaghaAsmbler *const restrict tasm, const bool firstpass)
{
	if( !tasm or !tasm->Src or !tasm->Iter )
		return false;
	
	tasm->ProgramCounter++;
	SkipWhiteSpace(&tasm->Iter);
	if( *tasm->Iter != 'r' ) {
		printf("tasm error: opcode requires a register as 1st operand. line: %zu\n", tasm->CurrLine);
		exit(-1);
	}
	const uint8_t regid = LexRegisterID(tasm);
	if( !firstpass ) {
		struct LabelInfo *label = LinkMap_Get(tasm->FuncTable, tasm->ActiveFuncLabel->CStr).Ptr;
		if( !label ) {
			printf("tasm error: undefined label '%s' on line: %zu\n", tasm->ActiveFuncLabel->CStr, tasm->CurrLine);
			exit(-1);
		}
		ByteBuffer_InsertByte(&label->Bytecode, regid);
	}
	return true;
}
bool TaghaAsm_ParseOneImmInstr(struct TaghaAsmbler *const restrict tasm, const bool firstpass)
{
	if( !tasm or !tasm->Src or !tasm->Iter )
		return false;
	
	tasm->ProgramCounter++;
	SkipWhiteSpace(&tasm->Iter);
	
	int64_t immval = 0;
	// imm value.
	if( IsDecimal(*tasm->Iter) )
		immval = LexImmValue(tasm);
	// label value.
	else if( *tasm->Iter=='.' or *tasm->Iter=='%' )
		immval = LexLabelValue(tasm, firstpass);
	// global variable label.
	else if( IsAlphabetic(*tasm->Iter) ) {
		LexIdentifier(&tasm->Iter, tasm->Lexeme);
		if( !LinkMap_HasKey(tasm->VarTable, tasm->Lexeme->CStr) ) {
			printf("tasm error: undefined global var '%s' in opcode on line: %zu\n", tasm->Lexeme->CStr, tasm->CurrLine);
			exit(-1);
		}
		tasm->ProgramCounter += 8;
		immval = LinkMap_GetIndexByName(tasm->VarTable, tasm->Lexeme->CStr);
	#ifdef TASM_DEBUG
		printf("tasm: global's '%s' index is '%zu'\n", tasm->Lexeme->CStr, LinkMap_GetIndexByName(tasm->VarTable, tasm->Lexeme->CStr));
	#endif
	}
	else {
		printf("tasm error: opcode requires an immediate or label value as 1st operand. line: %zu\n", tasm->CurrLine);
		exit(-1);
	}
	
	if( !firstpass ) {
		struct LabelInfo *label = LinkMap_Get(tasm->FuncTable, tasm->ActiveFuncLabel->CStr).Ptr;
		if( !label ) {
			printf("tasm error: undefined label '%s' on line: %zu\n", tasm->ActiveFuncLabel->CStr, tasm->CurrLine);
			exit(-1);
		}
		ByteBuffer_InsertInt(&label->Bytecode, immval, sizeof immval);
	}
	return true;
}
bool TaghaAsm_ParseRegMemInstr(struct TaghaAsmbler *const restrict tasm, const bool firstpass)
{
	if( !tasm or !tasm->Src or !tasm->Iter )
		return false;
	
	tasm->ProgramCounter++;
	SkipWhiteSpace(&tasm->Iter);
	
	if( *tasm->Iter != 'r' ) {
		printf("tasm error: opcode requires a register as 1st operand. line: %zu\n", tasm->CurrLine);
		exit(-1);
	}
	const uint8_t destreg = LexRegisterID(tasm);
	
	// ok, let's read the secondary operand!
	SkipWhiteSpace(&tasm->Iter);
	if( *tasm->Iter==',' )
		tasm->Iter++;
	
	SkipWhiteSpace(&tasm->Iter);
	
	if( *tasm->Iter != '[' ) {
		printf("tasm error: opcode requires a memory dereference as 1st operand. line: %zu\n", tasm->CurrLine);
		exit(-1);
	}
	uint8_t srcreg;
	int32_t offset;
	LexRegDeref(tasm, &srcreg, &offset);
	
	if( !firstpass ) {
		struct LabelInfo *label = LinkMap_Get(tasm->FuncTable, tasm->ActiveFuncLabel->CStr).Ptr;
		if( !label ) {
			printf("tasm error: undefined label '%s' on line: %zu\n", tasm->ActiveFuncLabel->CStr, tasm->CurrLine);
			exit(-1);
		}
		ByteBuffer_InsertByte(&label->Bytecode, destreg);
		ByteBuffer_InsertByte(&label->Bytecode, srcreg);
		ByteBuffer_InsertInt(&label->Bytecode, offset, sizeof offset);
	}
	return true;
}
bool TaghaAsm_ParseMemRegInstr(struct TaghaAsmbler *const restrict tasm, const bool firstpass)
{
	if( !tasm or !tasm->Src or !tasm->Iter )
		return false;
	
	tasm->ProgramCounter++;
	SkipWhiteSpace(&tasm->Iter);
	
	if( *tasm->Iter != '[' ) {
		printf("tasm error: opcode requires a memory dereference as 1st operand. line: %zu\n", tasm->CurrLine);
		exit(-1);
	}
	uint8_t destreg;
	int32_t offset;
	LexRegDeref(tasm, &destreg, &offset);
	
	// ok, let's read the secondary operand!
	SkipWhiteSpace(&tasm->Iter);
	if( *tasm->Iter==',' )
		tasm->Iter++;
	
	SkipWhiteSpace(&tasm->Iter);
	if( *tasm->Iter != 'r' ) {
		printf("tasm error: opcode requires a register as 2nd operand. line: %zu\n", tasm->CurrLine);
		exit(-1);
	}
	const uint8_t srcreg = LexRegisterID(tasm);
	
	if( !firstpass ) {
		struct LabelInfo *label = LinkMap_Get(tasm->FuncTable, tasm->ActiveFuncLabel->CStr).Ptr;
		if( !label ) {
			printf("tasm error: undefined label '%s' on line: %zu\n", tasm->ActiveFuncLabel->CStr, tasm->CurrLine);
			exit(-1);
		}
		ByteBuffer_InsertByte(&label->Bytecode, destreg);
		ByteBuffer_InsertByte(&label->Bytecode, srcreg);
		ByteBuffer_InsertInt(&label->Bytecode, offset, sizeof offset);
	}
	return true;
}
bool TaghaAsm_ParseRegImmInstr(struct TaghaAsmbler *const restrict tasm, const bool firstpass)
{
	if( !tasm or !tasm->Src or !tasm->Iter )
		return false;
	
	tasm->ProgramCounter++;
	SkipWhiteSpace(&tasm->Iter);
	if( *tasm->Iter != 'r' ) {
		printf("tasm error: opcode requires a register as 1st operand. line: %zu\n", tasm->CurrLine);
		exit(-1);
	}
	const uint8_t regid = LexRegisterID(tasm);
	
	// ok, let's read the secondary operand!
	SkipWhiteSpace(&tasm->Iter);
	if( *tasm->Iter==',' )
		tasm->Iter++;
	
	SkipWhiteSpace(&tasm->Iter);
	
	int64_t immval = 0;
	// imm value.
	if( IsDecimal(*tasm->Iter) )
		immval = LexImmValue(tasm);
	// label value.
	else if( *tasm->Iter=='.' or *tasm->Iter=='%' )
		immval = LexLabelValue(tasm, firstpass);
	// global variable label.
	else if( IsAlphabetic(*tasm->Iter) ) {
		LexIdentifier(&tasm->Iter, tasm->Lexeme);
		if( !LinkMap_HasKey(tasm->VarTable, tasm->Lexeme->CStr) ) {
			printf("tasm error: undefined global var '%s' in opcode on line: %zu\n", tasm->Lexeme->CStr, tasm->CurrLine);
			exit(-1);
		}
		tasm->ProgramCounter += 8;
		immval = LinkMap_GetIndexByName(tasm->VarTable, tasm->Lexeme->CStr);
	#ifdef TASM_DEBUG
		printf("tasm: global's '%s' index is '%zu'\n", tasm->Lexeme->CStr, LinkMap_GetIndexByName(tasm->VarTable, tasm->Lexeme->CStr));
	#endif
	}
	else {
		printf("tasm error: opcode requires an immediate or label value as 2nd operand. line: %zu\n", tasm->CurrLine);
		exit(-1);
	}
	
	if( !firstpass ) {
		struct LabelInfo *label = LinkMap_Get(tasm->FuncTable, tasm->ActiveFuncLabel->CStr).Ptr;
		if( !label ) {
			printf("tasm error: undefined label '%s' on line: %zu\n", tasm->ActiveFuncLabel->CStr, tasm->CurrLine);
			exit(-1);
		}
		ByteBuffer_InsertByte(&label->Bytecode, regid);
		ByteBuffer_InsertInt(&label->Bytecode, immval, sizeof immval);
	}
	return true;
}

bool TaghaAsm_Assemble(struct TaghaAsmbler *const restrict tasm)
{
	if( !tasm or !tasm->Src )
		return false;
	
	// set up our data.
	tasm->Lexeme = &(struct String){0},
	tasm->LabelTable = &(struct LinkMap){0},
	tasm->FuncTable = &(struct LinkMap){0},
	tasm->VarTable = &(struct LinkMap){0},
	tasm->Opcodes = &(struct LinkMap){0},
	tasm->Registers = &(struct LinkMap){0};
	
	// set up registers and map their IDs
	LinkMap_Insert(tasm->Registers, "RALAF", (union Value){.UInt64 = regAlaf});
	LinkMap_Insert(tasm->Registers, "ralaf", (union Value){.UInt64 = regAlaf});
	
	LinkMap_Insert(tasm->Registers, "RBETH", (union Value){.UInt64 = regBeth});
	LinkMap_Insert(tasm->Registers, "rbeth", (union Value){.UInt64 = regBeth});
	
	LinkMap_Insert(tasm->Registers, "RGAMAL", (union Value){.UInt64 = regGamal});
	LinkMap_Insert(tasm->Registers, "rgamal", (union Value){.UInt64 = regGamal});
	
	LinkMap_Insert(tasm->Registers, "RDALATH", (union Value){.UInt64 = regDalath});
	LinkMap_Insert(tasm->Registers, "rdalath", (union Value){.UInt64 = regDalath});
	
	LinkMap_Insert(tasm->Registers, "RHEH", (union Value){.UInt64 = regHeh});
	LinkMap_Insert(tasm->Registers, "rheh", (union Value){.UInt64 = regHeh});
	
	LinkMap_Insert(tasm->Registers, "RWAW", (union Value){.UInt64 = regWaw});
	LinkMap_Insert(tasm->Registers, "rwaw", (union Value){.UInt64 = regWaw});
	
	LinkMap_Insert(tasm->Registers, "RZAIN", (union Value){.UInt64 = regZain});
	LinkMap_Insert(tasm->Registers, "rzain", (union Value){.UInt64 = regZain});
	
	LinkMap_Insert(tasm->Registers, "RHETH", (union Value){.UInt64 = regHeth});
	LinkMap_Insert(tasm->Registers, "rheth", (union Value){.UInt64 = regHeth});
	
	LinkMap_Insert(tasm->Registers, "RTETH", (union Value){.UInt64 = regTeth});
	LinkMap_Insert(tasm->Registers, "rteth", (union Value){.UInt64 = regTeth});
	
	LinkMap_Insert(tasm->Registers, "RYODH", (union Value){.UInt64 = regYodh});
	LinkMap_Insert(tasm->Registers, "ryodh", (union Value){.UInt64 = regYodh});
	
	LinkMap_Insert(tasm->Registers, "RKAF", (union Value){.UInt64 = regKaf});
	LinkMap_Insert(tasm->Registers, "rkaf", (union Value){.UInt64 = regKaf});
	
	LinkMap_Insert(tasm->Registers, "RLAMADH", (union Value){.UInt64 = regLamadh});
	LinkMap_Insert(tasm->Registers, "rlamadh", (union Value){.UInt64 = regLamadh});
	
	LinkMap_Insert(tasm->Registers, "RMEEM", (union Value){.UInt64 = regMeem});
	LinkMap_Insert(tasm->Registers, "rmeem", (union Value){.UInt64 = regMeem});
	
	LinkMap_Insert(tasm->Registers, "RNOON", (union Value){.UInt64 = regNoon});
	LinkMap_Insert(tasm->Registers, "rnoon", (union Value){.UInt64 = regNoon});
	
	LinkMap_Insert(tasm->Registers, "RSEMKATH", (union Value){.UInt64 = regSemkath});
	LinkMap_Insert(tasm->Registers, "rsemkath", (union Value){.UInt64 = regSemkath});
	
	LinkMap_Insert(tasm->Registers, "R_EH", (union Value){.UInt64 = reg_Eh});
	LinkMap_Insert(tasm->Registers, "r_eh", (union Value){.UInt64 = reg_Eh});
	
	LinkMap_Insert(tasm->Registers, "RPEH", (union Value){.UInt64 = regPeh});
	LinkMap_Insert(tasm->Registers, "rpeh", (union Value){.UInt64 = regPeh});
	
	LinkMap_Insert(tasm->Registers, "RSADHE", (union Value){.UInt64 = regSadhe});
	LinkMap_Insert(tasm->Registers, "rsadhe", (union Value){.UInt64 = regSadhe});
	
	LinkMap_Insert(tasm->Registers, "RQOF", (union Value){.UInt64 = regQof});
	LinkMap_Insert(tasm->Registers, "rqof", (union Value){.UInt64 = regQof});
	
	LinkMap_Insert(tasm->Registers, "RREESH", (union Value){.UInt64 = regReesh});
	LinkMap_Insert(tasm->Registers, "rreesh", (union Value){.UInt64 = regReesh});
	
	LinkMap_Insert(tasm->Registers, "RSHEEN", (union Value){.UInt64 = regSheen});
	LinkMap_Insert(tasm->Registers, "rsheen", (union Value){.UInt64 = regSheen});
	
	LinkMap_Insert(tasm->Registers, "RTAW", (union Value){.UInt64 = regTaw});
	LinkMap_Insert(tasm->Registers, "rtaw", (union Value){.UInt64 = regTaw});
	
	LinkMap_Insert(tasm->Registers, "RSP", (union Value){.UInt64 = regStk});
	LinkMap_Insert(tasm->Registers, "rsp", (union Value){.UInt64 = regStk});
	
	LinkMap_Insert(tasm->Registers, "RBP", (union Value){.UInt64 = regBase});
	LinkMap_Insert(tasm->Registers, "rbp", (union Value){.UInt64 = regBase});
	
	// set up our instruction set!
	#define X(x) LinkMap_Insert(tasm->Opcodes, #x, (union Value){.UInt64 = x});
		INSTR_SET;
	#undef X
	
	/* FIRST PASS. Collect labels and their PC relative addresses */
	#define MAX_LINE_CHARS 2048
	char line_buffer[MAX_LINE_CHARS] = {0};
	
#ifdef TASM_DEBUG
	puts("\ntasm: FIRST PASS Begin!\n");
#endif
	
	for( tasm->Iter = fgets(line_buffer, MAX_LINE_CHARS, tasm->Src) ; tasm->Iter ; tasm->Iter = fgets(line_buffer, MAX_LINE_CHARS, tasm->Src) ) {
		// set up first line for error checks.
		tasm->CurrLine++;
	#ifdef TASM_DEBUG
		//printf("tasm debug: printing line:: '%s'\n", tasm->Iter);
	#endif
		while( *tasm->Iter ) {
			String_Del(tasm->Lexeme);
			// skip whitespace.
			SkipWhiteSpace(&tasm->Iter);
			
			// skip to next line if comment or directive.
			if( *tasm->Iter=='\n' or *tasm->Iter==';' or *tasm->Iter=='#' )
				break;
			
			else if( *tasm->Iter=='}' ) {
				tasm->Iter++;
				String_Del(tasm->ActiveFuncLabel);
				tasm->ActiveFuncLabel = NULL;
				tasm->ProgramCounter = 0;
				break;
			}
			// parse the directives!
			else if( *tasm->Iter=='$' ) {
				LexIdentifier(&tasm->Iter, tasm->Lexeme);
				if( !String_NCmpCStr(tasm->Lexeme, "$stacksize", sizeof "$stacksize") ) {
					TaghaAsm_ParseStackDirective(tasm);
				#ifdef TASM_DEBUG
					printf("tasm: Stack size set to: %u\n", tasm->Stacksize);
				#endif
				}
				else if( !String_NCmpCStr(tasm->Lexeme, "$global", sizeof "$global") )
					TaghaAsm_ParseGlobalVarDirective(tasm);
				else if( !String_NCmpCStr(tasm->Lexeme, "$native", sizeof "$native") )
					TaghaAsm_ParseNativeDirective(tasm);
				else if( !String_NCmpCStr(tasm->Lexeme, "$safemode", sizeof "$safemode") || !String_NCmpCStr(tasm->Lexeme, "$safe", sizeof "$safe")  )
					tasm->Safemode = 1;
				break;
			}
			
			// holy cannoli, we found a label!
			else if( *tasm->Iter=='.' or *tasm->Iter=='%' ) {
				const bool funclbl = *tasm->Iter == '%';
				// the dot or percent is added to our lexeme
				LexIdentifier(&tasm->Iter, tasm->Lexeme);
				SkipWhiteSpace(&tasm->Iter);
				if( *tasm->Iter == ':' )
					tasm->Iter++;
				
				SkipWhiteSpace(&tasm->Iter);
				if( funclbl ) {
					if( *tasm->Iter != '{' ) {
						printf("tasm error: missing curly '{' bracket! Curly bracket must be on the same line as label. line: %zu\n", tasm->CurrLine);
						exit(-1);
					}
					tasm->Iter++;
				}
				
				if( !IsAlphabetic(tasm->Lexeme->CStr[1]) ) {
					printf("tasm error: %s labels must have alphabetic names! line: %zu\n", funclbl ? "function" : "jump", tasm->CurrLine);
					exit(-1);
				}
				else if( LinkMap_HasKey(funclbl ? tasm->FuncTable : tasm->LabelTable, tasm->Lexeme->CStr) ) {
					printf("tasm error: redefinition of label '%s' on line: %zu\n", tasm->Lexeme->CStr, tasm->CurrLine);
					exit(-1);
				}
				
				struct LabelInfo *label = calloc(1, sizeof *label);
				if( !label ) {
					puts("tasm error: OUT OF MEMORY! Exiting...");
					exit(-1);
				}
				if( funclbl ) {
					tasm->ActiveFuncLabel = &(struct String){0};
					String_Copy(tasm->ActiveFuncLabel, tasm->Lexeme);
				}
				label->Addr = tasm->ProgramCounter;
				label->IsFunc = funclbl;
#ifdef TASM_DEBUG
				printf("%s Label '%s' is located at address: %zu\n", funclbl ? "Func" : "Local", tasm->Lexeme->CStr, tasm->ProgramCounter);
#endif
				LinkMap_Insert(funclbl ? tasm->FuncTable : tasm->LabelTable, tasm->Lexeme->CStr, (union Value){.Ptr = label});
			}
			// it's an opcode!
			else if( IsAlphabetic(*tasm->Iter) ) {
				if( !tasm->ActiveFuncLabel ) {
					printf("tasm error: opcode outside of function blocks on line: %zu\n", tasm->CurrLine);
					exit(-1);
				}
				LexIdentifier(&tasm->Iter, tasm->Lexeme);
				if( !LinkMap_HasKey(tasm->Opcodes, tasm->Lexeme->CStr) ) {
					printf("tasm error: unknown opcode '%s' on line: %zu\n", tasm->Lexeme->CStr, tasm->CurrLine);
					exit(-1);
				}
				uint8_t opcode = LinkMap_Get(tasm->Opcodes, tasm->Lexeme->CStr).UInt64;
				switch( opcode ) {
					// opcodes that take no args
					case halt: case ret: case nop:
						tasm->ProgramCounter++; break;
					
					// opcodes that only take an imm operand.
					case pushi: case jmp: case jz: case jnz:
					case call: case syscall:
						TaghaAsm_ParseOneImmInstr(tasm, true); break;
					
					// opcodes that only take a register operand.
					case push: case pop: case bit_not:
					case inc: case dec: case neg:
					case callr: case syscallr:
				#ifdef FLOATING_POINT_OPS
					case flt2dbl: case dbl2flt: case int2dbl: case int2flt:
					case incf: case decf: case negf:
				#endif
						TaghaAsm_ParseOneRegInstr(tasm, true); break;
					
					// opcodes reg<-imm
					case loadglobal: case loadfunc: case movi:
						TaghaAsm_ParseRegImmInstr(tasm, true); break;
					
					// opcodes reg<-mem (load)
					case loadaddr:
					case ld1: case ld2: case ld4: case ld8:
						TaghaAsm_ParseRegMemInstr(tasm, true); break;
					
					// opcodes mem<-reg (store)
					case st1: case st2: case st4: case st8:
						TaghaAsm_ParseMemRegInstr(tasm, true); break;
					
					// opcodes reg<-reg
					case mov:
					case add: case sub: case mul: case divi: case mod:
					case bit_and: case bit_or: case bit_xor: case shl: case shr:
					case ilt: case ile: case igt: case ige:
					case ult: case ule: case ugt: case uge:
					case cmp: case neq:
				#ifdef FLOATING_POINT_OPS
					case addf: case subf: case mulf: case divf:
					case ltf: case lef: case gtf: case gef: case cmpf: case neqf:
				#endif
						TaghaAsm_ParseRegRegInstr(tasm, true); break;
				}
			}
		}
	}
#ifdef TASM_DEBUG
	puts("\ntasm: FIRST PASS End!\n");
#endif
	rewind(tasm->Src);
	
#ifdef TASM_DEBUG
	puts("\ntasm: SECOND PASS Start!\n");
#endif
	tasm->CurrLine = 0;
	
	for( tasm->Iter = fgets(line_buffer, MAX_LINE_CHARS, tasm->Src) ; tasm->Iter ; tasm->Iter = fgets(line_buffer, MAX_LINE_CHARS, tasm->Src) ) {
		// set up first line for error checks.
		tasm->CurrLine++;
	#ifdef TASM_DEBUG
		//printf("tasm debug: printing line:: '%s'\n", tasm->Iter);
	#endif
		while( *tasm->Iter ) {
			String_Del(tasm->Lexeme);
			// skip whitespace.
			SkipWhiteSpace(&tasm->Iter);
			
			// skip to next line if comment or directive.
			if( *tasm->Iter=='\n' or *tasm->Iter==';' or *tasm->Iter=='#' or *tasm->Iter=='$' )
				break;
			else if( *tasm->Iter=='}' ) {
				tasm->Iter++;
				String_Del(tasm->ActiveFuncLabel);
				tasm->ActiveFuncLabel = NULL;
				tasm->ProgramCounter = 0;
				break;
			}
			// skip labels in second pass.
			else if( *tasm->Iter=='.' or *tasm->Iter=='%' ) {
				const bool funclbl = *tasm->Iter == '%';
				LexIdentifier(&tasm->Iter, tasm->Lexeme);
				tasm->Iter++;
				SkipWhiteSpace(&tasm->Iter);
				if( funclbl ) {
					tasm->Iter++;
					tasm->ActiveFuncLabel = &(struct String){0};
					String_Copy(tasm->ActiveFuncLabel, tasm->Lexeme);
					SkipWhiteSpace(&tasm->Iter);
				}
			}
			// parse opcode!
			if( IsAlphabetic(*tasm->Iter) ) {
				LexIdentifier(&tasm->Iter, tasm->Lexeme);
				if( !LinkMap_HasKey(tasm->Opcodes, tasm->Lexeme->CStr) ) {
					printf("tasm error: unknown opcode '%s' on line: %zu\n", tasm->Lexeme->CStr, tasm->CurrLine);
					exit(-1);
				}
				uint8_t opcode = LinkMap_Get(tasm->Opcodes, tasm->Lexeme->CStr).UInt64;
				struct LabelInfo *label = LinkMap_Get(tasm->FuncTable, tasm->ActiveFuncLabel->CStr).Ptr;
				ByteBuffer_InsertByte(&label->Bytecode, opcode);
				switch( opcode ) {
					// opcodes that take no args
					case halt: case ret: case nop:
						break;
					
					// opcodes that only take an imm operand.
					case pushi: case jmp: case jz: case jnz:
					case call: case syscall:
						TaghaAsm_ParseOneImmInstr(tasm, false); break;
					
					// opcodes that only take a register operand.
					case push: case pop: case bit_not:
					case inc: case dec: case neg:
					case callr: case syscallr:
				#ifdef FLOATING_POINT_OPS
					case flt2dbl: case dbl2flt: case int2dbl: case int2flt:
					case incf: case decf: case negf:
				#endif
						TaghaAsm_ParseOneRegInstr(tasm, false); break;
					
					// opcodes reg<-imm
					case loadglobal: case loadfunc: case movi:
						TaghaAsm_ParseRegImmInstr(tasm, false); break;
					
					// opcodes reg<-mem (load)
					case loadaddr:
					case ld1: case ld2: case ld4: case ld8:
						TaghaAsm_ParseRegMemInstr(tasm, false); break;
					
					// opcodes mem<-reg (store)
					case st1: case st2: case st4: case st8:
						TaghaAsm_ParseMemRegInstr(tasm, false); break;
					
					// opcodes reg<-reg
					case mov:
					case add: case sub: case mul: case divi: case mod:
					case bit_and: case bit_or: case bit_xor: case shl: case shr:
					case ilt: case ile: case igt: case ige:
					case ult: case ule: case ugt: case uge:
					case cmp: case neq:
				#ifdef FLOATING_POINT_OPS
					case addf: case subf: case mulf: case divf:
					case ltf: case lef: case gtf: case gef: case cmpf: case neqf:
				#endif
						TaghaAsm_ParseRegRegInstr(tasm, false); break;
				}
			}
		}
	}
	
#ifdef TASM_DEBUG
	puts("\ntasm: SECOND PASS End!\n");
#endif
	if( !tasm->Stacksize )
		tasm->Stacksize = 128;
	
	// build our func table & global var table
	struct ByteBuffer functable; ByteBuffer_Init(&functable);
	struct ByteBuffer datatable; ByteBuffer_Init(&datatable);
	
	for( size_t i=0 ; i<tasm->FuncTable->Count ; i++ ) {
		struct KeyNode *node = LinkMap_GetNodeByIndex(tasm->FuncTable, i);
		struct LabelInfo *label = node->Data.Ptr;
		if( !label )
			continue;
		
		// write flag
		ByteBuffer_InsertByte(&functable, label->IsNativeFunc);
		// write strlen
		ByteBuffer_InsertInt(&functable, node->KeyName.Len, sizeof(uint32_t));
		
		// write string
		label->IsNativeFunc ? 
			ByteBuffer_InsertInt(&functable, 8, sizeof(uint32_t))
				: ByteBuffer_InsertInt(&functable, label->Bytecode.Count, sizeof(uint32_t));
		
		// write instrlen.
		ByteBuffer_InsertString(&functable, node->KeyName.CStr+1, node->KeyName.Len-1);
		label->IsNativeFunc ?
			ByteBuffer_InsertInt(&functable, 0, sizeof(uint64_t))
				: ByteBuffer_Append(&functable, &label->Bytecode) ;
	#ifdef TASM_DEBUG
		printf("func label: %s\nData:\n", node->KeyName.CStr);
		for( size_t i=0 ; i<label->Bytecode.Count ; i++ )
			printf("bytecode[%zu] == %u\n", i, label->Bytecode.Buffer[i]);
		puts("\n");
	#endif
	}
	
	for( size_t i=0 ; i<tasm->VarTable->Count ; i++ ) {
		struct KeyNode *node = LinkMap_GetNodeByIndex(tasm->VarTable, i);
		struct ByteBuffer *bytedata = node->Data.Ptr;
		if( !bytedata )
			continue;
		
		// write flag.
		ByteBuffer_InsertByte(&datatable, 0);
		// write strlen.
		ByteBuffer_InsertInt(&datatable, node->KeyName.Len+1, sizeof(uint32_t));
		// write byte count.
		ByteBuffer_InsertInt(&datatable, bytedata->Count, sizeof(uint32_t));
		// write string.
		ByteBuffer_InsertString(&datatable, node->KeyName.CStr, node->KeyName.Len);
		// write byte data.
		ByteBuffer_Append(&datatable, bytedata);
	#ifdef TASM_DEBUG
		printf("global var: %s\nData:\n", node->KeyName.CStr);
		for( size_t i=0 ; i<bytedata->Count ; i++ )
			printf("bytecode[%zu] == %u\n", i, bytedata->Buffer[i]);
		puts("\n");
	#endif
	}
	
	// build the header table.
	struct ByteBuffer tbcfile; ByteBuffer_Init(&tbcfile);
	ByteBuffer_InsertInt(&tbcfile, 0xC0DE, sizeof(uint16_t));
	ByteBuffer_InsertInt(&tbcfile, tasm->Stacksize, sizeof tasm->Stacksize);
	// write function table offset.
	ByteBuffer_InsertInt(&tbcfile, sizeof(struct TaghaHeader), sizeof(uint32_t));
	
	// write global variable table offset.
	ByteBuffer_InsertInt(&tbcfile, functable.Count + sizeof(struct TaghaHeader) + 4, sizeof(uint32_t));
	
	// write stack offset.
	ByteBuffer_InsertInt(&tbcfile, functable.Count + datatable.Count + sizeof(struct TaghaHeader) + 8, sizeof(uint32_t));
	
	// write flags, currently none.
	ByteBuffer_InsertByte(&tbcfile, tasm->Safemode | 0);
	
	// now build function table.
	ByteBuffer_InsertInt(&tbcfile, tasm->FuncTable->Count, sizeof(uint32_t));
	ByteBuffer_Append(&tbcfile, &functable);
	ByteBuffer_Del(&functable);
	
	// now build global variable table.
	ByteBuffer_InsertInt(&tbcfile, tasm->VarTable->Count, sizeof(uint32_t));
	ByteBuffer_Append(&tbcfile, &datatable);
	ByteBuffer_Del(&datatable);
	
	// now build stack
	for( size_t i=0 ; i<tasm->Stacksize ; i++ )
		ByteBuffer_InsertInt(&tbcfile, 0, sizeof(union TaghaVal));
	
	{ // scoping this section off so we can use restricted pointer.
		char *restrict iter = tasm->OutputName.CStr;
		size_t len = 0;
		while( *++iter );
		while( *--iter != '.' )
			++len;
	
		memset(iter, 0, len);
	}
	
	// use line_buffer instead of wasting more stack space.
	memset(line_buffer, 0, sizeof line_buffer);
	sprintf(line_buffer, "%.2000s.tbc", tasm->OutputName.CStr);
	
	FILE *tbcscript = fopen(line_buffer, "w+");
	if( !tbcscript ) {
		puts("tasm error: unable to create output file!");
		exit(-1);
	}
	//for( size_t n=0 ; n<(tasm->Stacksize*8) ; n++ )
	//	ByteBuffer_InsertByte(&tbcfile, 0);
	ByteBuffer_DumpToFile(&tbcfile, tbcscript);
	fclose(tbcscript); tbcscript=NULL;
	
	ByteBuffer_Del(&tbcfile);
	String_Del(&tasm->OutputName);
	String_Del(tasm->Lexeme);
	String_Del(tasm->ActiveFuncLabel);
	
	LinkMap_Del(tasm->LabelTable, Label_Free);
	LinkMap_Del(tasm->FuncTable, Label_Free);
	LinkMap_Del(tasm->VarTable, (bool(*)(void*))ByteBuffer_Free);
	LinkMap_Del(tasm->Opcodes, NULL);
	LinkMap_Del(tasm->Registers, NULL);
	fclose(tasm->Src); tasm->Src=NULL;
	return true;
}

bool Label_Free(void *p)
{
	struct LabelInfo **restrict labelref = p;
	if( !labelref || !*labelref )
		return false;
	
	ByteBuffer_Del(&(*labelref)->Bytecode);
	free(*labelref), *labelref=NULL;
	return true;
}

static size_t GetFileSize(FILE *const restrict file)
{
	int64_t size = 0L;
	if( !file )
		return size;
	
	if( !fseek(file, 0, SEEK_END) ) {
		size = ftell(file);
		if( size == -1 )
			return 0L;
		rewind(file);
	}
	return (size_t)size;
}

int main(int argc, char *argv[])
{
	if( argc<=1 )
		return -1;
	
	for( int i=1 ; i<argc ; i++ ) {
		FILE *tasmfile = fopen(argv[i], "r");
		if( !tasmfile )
			continue;
		
		struct TaghaAsmbler *restrict tasm = &(struct TaghaAsmbler){0};
		tasm->Src = tasmfile;
		tasm->SrcSize = GetFileSize(tasmfile);
		String_InitStr(&tasm->OutputName, argv[i]);
		TaghaAsm_Assemble(tasm);
	}
}

