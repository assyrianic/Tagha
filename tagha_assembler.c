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
				char charval = *tasm->Iter++;
				if( !charval ) { // sudden EOF?
					puts("tasm error: sudden EOF in global directive string copying!");
					exit(-1);
				}
				String_AddChar(tasm->Lexeme, charval);
				// handle escape chars
				if( charval=='\\' )
					String_AddChar(tasm->Lexeme, *tasm->Iter++);
			}
			ByteBuffer_InsertString(vardata, tasm->Lexeme->CStr, tasm->Lexeme->Len);
			bytes = 0;
		}
		else if( *tasm->Iter==',' )
			tasm->Iter++;
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
				ByteBuffer_InsertInt(vardata, data, sizeof data);
				bytes -= sizeof data;
			}
			else if( !String_NCmpCStr(tasm->Lexeme, "long", sizeof "long") ) {
				SkipWhiteSpace(&tasm->Iter);
				LexNumber(&tasm->Iter, tasm->Lexeme);
				const bool isbinary = !String_NCmpCStr(tasm->Lexeme, "0b", 2) or !String_NCmpCStr(tasm->Lexeme, "0B", 2) ? true : false;
				const uint32_t data = strtoull(isbinary ? tasm->Lexeme->CStr+2 : tasm->Lexeme->CStr, NULL, isbinary ? 2 : 0);
				ByteBuffer_InsertInt(vardata, data, sizeof data);
				bytes -= sizeof data;
			}
			else if( !String_NCmpCStr(tasm->Lexeme, "word", sizeof "word") ) {
				SkipWhiteSpace(&tasm->Iter);
				LexNumber(&tasm->Iter, tasm->Lexeme);
				const bool isbinary = !String_NCmpCStr(tasm->Lexeme, "0b", 2) or !String_NCmpCStr(tasm->Lexeme, "0B", 2) ? true : false;
				const uint64_t data = strtoull(isbinary ? tasm->Lexeme->CStr+2 : tasm->Lexeme->CStr, NULL, isbinary ? 2 : 0);
				ByteBuffer_InsertInt(vardata, data, sizeof data);
				bytes -= sizeof data;
			}
		}
		else {
			printf("tasm error: global var directive data set is incomplete, must be equal to bytesize given. line: %zu\n", tasm->CurrLine);
			exit(-1);
		}
	}
	printf("tasm: adding global var '%s'\n", varname->CStr);
	for( size_t i=0 ; i<ByteBuffer_Count(vardata) ; i++ )
		printf("tasm: global var[%zu] == %u\n", i, vardata->Buffer[i]);
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
	LinkMap_Insert(tasm->LabelTable, tasm->Lexeme->CStr, native);
	printf("tasm: added native function '%s'\n", tasm->Lexeme->CStr);
	return true;
}

// possible formats:
// opcode register, imm
// opcode register, register
// opcode register, [size register + offset]
// opcode [size register + offset], imm
// opcode [size register + offset], register
bool TaghaAsm_ParseTwoOpInstr(struct TaghaAsmbler *const restrict tasm, const bool firstpass)
{
	if( !tasm or !tasm->Src or !tasm->Iter )
		return false;
	
	tasm->ProgramCounter++;
	SkipWhiteSpace(&tasm->Iter);
	bool IsMemoryDeref = false;
	uint8_t
		addrmode1=0,
		addrmode2=0,
		reg1=0
	;
	union Value imm = (union Value){0};
	int32_t offset = 0;
	
	// check the first operand.
	if( *tasm->Iter=='r' ) {
		// register operand and register addressing mode.
		tasm->ProgramCounter += 2;
		LexIdentifier(&tasm->Iter, tasm->Lexeme);
		if( !LinkMap_HasKey(tasm->Registers, tasm->Lexeme->CStr) ) {
			printf("tasm error: invalid register name '%s' on line: %zu\n", tasm->Lexeme->CStr, tasm->CurrLine);
			exit(-1);
		}
		addrmode1 |= Register;
		reg1 = LinkMap_Get(tasm->Registers, tasm->Lexeme->CStr).UInt64;
	}
	else if( *tasm->Iter=='[' ) {
		tasm->Iter++;
		IsMemoryDeref = true;
		// register indirection operand + register indirect addressing mode + 4 byte offset.
		tasm->ProgramCounter += 6;
		SkipWhiteSpace(&tasm->Iter);
		
		// get memory dereferencing size.
		LexIdentifier(&tasm->Iter, tasm->Lexeme);
		if( !String_NCmpCStr(tasm->Lexeme, "byte", sizeof "byte")
				or !String_NCmpCStr(tasm->Lexeme, "half", sizeof "half")
				or !String_NCmpCStr(tasm->Lexeme, "long", sizeof "long")
				or !String_NCmpCStr(tasm->Lexeme, "word", sizeof "word") )
		{
			addrmode1 |= RegIndirect;
			switch( *tasm->Lexeme->CStr ) {
				case 'b': addrmode1 |= Byte; break;
				case 'h': addrmode1 |= TwoBytes; break;
				case 'l': addrmode1 |= FourBytes; break;
				case 'w': addrmode1 |= EightBytes; break;
			}
			SkipWhiteSpace(&tasm->Iter);
			
			// get register name.
			LexIdentifier(&tasm->Iter, tasm->Lexeme);
			if( !LinkMap_HasKey(tasm->Registers, tasm->Lexeme->CStr) ) {
				printf("tasm error: invalid register name '%s' in register indirection on line: %zu\n", tasm->Lexeme->CStr, tasm->CurrLine);
				exit(-1);
			}
			reg1 = LinkMap_Get(tasm->Registers, tasm->Lexeme->CStr).UInt64;
			
			SkipWhiteSpace(&tasm->Iter);
			// if there's no plus/minus equation, assume `[reg+0]`.
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
				const bool negate = closer=='-';
				const bool isbinary = !String_NCmpCStr(tasm->Lexeme, "0b", 2) or !String_NCmpCStr(tasm->Lexeme, "0B", 2) ? true : false;
				offset = strtol(isbinary ? tasm->Lexeme->CStr+2 : tasm->Lexeme->CStr, NULL, isbinary ? 2 : 0);
				offset = negate ? -offset : offset;
				printf("offset == %i\n", offset);
			}
			if( *tasm->Iter != ']' ) {
				printf("tasm error: missing closing ']' bracket in register indirection on line: %zu\n", tasm->CurrLine);
				exit(-1);
			}
			tasm->Iter++;
		}
		else {
			printf("tasm error: missing or invalid indirection size '%s' on line: %zu\n", tasm->Lexeme->CStr, tasm->CurrLine);
			exit(-1);
		}
	}
	else {
		printf("tasm error: invalid 1st operand on line: %zu\n", tasm->CurrLine);
		exit(-1);
	}
	
	
	SkipWhiteSpace(&tasm->Iter);
	// ok, let's read the secondary operand!
	// the second operand can be an imm, register, or register indirection.
	if( *tasm->Iter==',' )
		tasm->Iter++;
	
	SkipWhiteSpace(&tasm->Iter);
	// immediate/constant operand value.
	if( IsDecimal(*tasm->Iter) ) {
		LexNumber(&tasm->Iter, tasm->Lexeme);
		tasm->ProgramCounter += 9;
		
		addrmode2 |= Immediate;
		const bool isbinary = !String_NCmpCStr(tasm->Lexeme, "0b", 2) or !String_NCmpCStr(tasm->Lexeme, "0B", 2) ? true : false;
		imm.UInt64 = strtoull(isbinary ? tasm->Lexeme->CStr+2 : tasm->Lexeme->CStr, NULL, isbinary ? 2 : 0);
	}
	else if( *tasm->Iter=='r' ) {
		// register operand and register addressing mode.
		tasm->ProgramCounter += 2;
		LexIdentifier(&tasm->Iter, tasm->Lexeme);
		if( !LinkMap_HasKey(tasm->Registers, tasm->Lexeme->CStr) ) {
			printf("tasm error: invalid register name '%s' in line: %zu\n", tasm->Lexeme->CStr, tasm->CurrLine);
			exit(-1);
		}
		addrmode2 |= Register;
		imm.UInt64 = LinkMap_Get(tasm->Registers, tasm->Lexeme->CStr).UInt64;
	}
	else if( *tasm->Iter=='[' ) {
		tasm->Iter++;
		if( IsMemoryDeref ) {
			printf("tasm error: memory to memory operations are not allowed! line: %zu\n", tasm->CurrLine);
			exit(-1);
		}
		
		// register indirection operand + register indirect addressing mode + 4 byte offset.
		tasm->ProgramCounter += 6;
		SkipWhiteSpace(&tasm->Iter);
		LexIdentifier(&tasm->Iter, tasm->Lexeme);
		if( !String_NCmpCStr(tasm->Lexeme, "byte", sizeof "byte")
				or !String_NCmpCStr(tasm->Lexeme, "half", sizeof "half")
				or !String_NCmpCStr(tasm->Lexeme, "long", sizeof "long")
				or !String_NCmpCStr(tasm->Lexeme, "word", sizeof "word") )
		{
			addrmode2 |= RegIndirect;
			switch( *tasm->Lexeme->CStr ) {
				case 'b': addrmode2 |= Byte; break;
				case 'h': addrmode2 |= TwoBytes; break;
				case 'l': addrmode2 |= FourBytes; break;
				case 'w': addrmode2 |= EightBytes; break;
			}
			
			SkipWhiteSpace(&tasm->Iter);
			LexIdentifier(&tasm->Iter, tasm->Lexeme);
			if( !LinkMap_HasKey(tasm->Registers, tasm->Lexeme->CStr) ) {
				printf("tasm error: invalid register name '%s' in register indirection on line: %zu\n", tasm->Lexeme->CStr, tasm->CurrLine);
				exit(-1);
			}
			imm.UInt64 = LinkMap_Get(tasm->Registers, tasm->Lexeme->CStr).UInt64;
			
			SkipWhiteSpace(&tasm->Iter);
			// if there's no plus/minus equation, assume `[reg+0]`
			if( *tasm->Iter=='-' or *tasm->Iter=='+' ) {
				tasm->Iter++;
				SkipWhiteSpace(&tasm->Iter);
				if( !IsDecimal(*tasm->Iter) ) {
					printf("tasm error: invalid offset '%s' in register indirection on line: %zu\n", tasm->Lexeme->CStr, tasm->CurrLine);
					exit(-1);
				}
				LexNumber(&tasm->Iter, tasm->Lexeme);
				SkipWhiteSpace(&tasm->Iter);
				const bool isbinary = !String_NCmpCStr(tasm->Lexeme, "0b", 2) or !String_NCmpCStr(tasm->Lexeme, "0B", 2) ? true : false;
				offset = strtol(isbinary ? tasm->Lexeme->CStr+2 : tasm->Lexeme->CStr, NULL, isbinary ? 2 : 0);
			}
			if( *tasm->Iter != ']' ) {
				printf("tasm error: missing closing ']' bracket in register indirection on line: %zu\n", tasm->CurrLine);
				exit(-1);
			}
			tasm->Iter++;
		}
		else {
			printf("tasm error: missing or invalid indirection size '%s' on line: %zu\n", tasm->Lexeme->CStr, tasm->CurrLine);
			exit(-1);
		}
	}
	if( !firstpass ) {
		ByteBuffer_InsertByte(tasm->Bytecode, addrmode1);
		ByteBuffer_InsertByte(tasm->Bytecode, addrmode2);
		ByteBuffer_InsertByte(tasm->Bytecode, reg1);
		if( addrmode2 & Immediate )
			ByteBuffer_InsertInt(tasm->Bytecode, imm.UInt64, sizeof imm.UInt64);
		else ByteBuffer_InsertByte(tasm->Bytecode, imm.UChar);
		if( IsMemoryDeref )
			ByteBuffer_InsertInt(tasm->Bytecode, (uint64_t)offset, sizeof offset);
	}
	return true;
}

// possible formats:
// opcode imm
// opcode register
// opcode [size register + offset]
bool TaghaAsm_ParseOneOpInstr(struct TaghaAsmbler *const restrict tasm, const bool firstpass)
{
	if( !tasm or !tasm->Src or !tasm->Iter )
		return false;
	
	tasm->ProgramCounter++;
	SkipWhiteSpace(&tasm->Iter);
	bool IsMemoryDeref = false;
	uint8_t addrmode1=0;
	union Value imm = (union Value){0};
	int32_t offset = 0;
	
	// immediate/constant operand value.
	if( IsDecimal(*tasm->Iter) ) {
		LexNumber(&tasm->Iter, tasm->Lexeme);
		tasm->ProgramCounter += 9;
		
		addrmode1 |= Immediate;
		const bool isbinary = !String_NCmpCStr(tasm->Lexeme, "0b", 2) or !String_NCmpCStr(tasm->Lexeme, "0B", 2) ? true : false;
		imm.UInt64 = strtoull(isbinary ? tasm->Lexeme->CStr+2 : tasm->Lexeme->CStr, NULL, isbinary ? 2 : 0);
	}
	// label value.
	else if( *tasm->Iter=='.' or *tasm->Iter=='%' ) {
		LexIdentifier(&tasm->Iter, tasm->Lexeme);
		tasm->ProgramCounter += 9;
		if( !firstpass and !LinkMap_HasKey(tasm->LabelTable, tasm->Lexeme->CStr) ) {
			printf("tasm error: undefined label '%s' on line: %zu\n", tasm->Lexeme->CStr, tasm->CurrLine);
			exit(-1);
		}
		addrmode1 |= Immediate;
		if( !firstpass ) {
			struct LabelInfo *label = LinkMap_Get(tasm->LabelTable, tasm->Lexeme->CStr).Ptr;
			imm.Int64 = label->Addr - tasm->ProgramCounter;
		}
	}
	else if( *tasm->Iter=='r' ) {
		// register operand and register addressing mode.
		tasm->ProgramCounter += 2;
		LexIdentifier(&tasm->Iter, tasm->Lexeme);
		if( !LinkMap_HasKey(tasm->Registers, tasm->Lexeme->CStr) ) {
			printf("tasm error: invalid register name '%s' in line: %zu\n", tasm->Lexeme->CStr, tasm->CurrLine);
			exit(-1);
		}
		addrmode1 |= Register;
		imm.UInt64 = LinkMap_Get(tasm->Registers, tasm->Lexeme->CStr).UInt64;
	}
	else if( *tasm->Iter=='[' ) {
		tasm->Iter++;
		IsMemoryDeref = true;
		
		// register indirection operand + register indirect addressing mode + 4 byte offset.
		tasm->ProgramCounter += 6;
		SkipWhiteSpace(&tasm->Iter);
		LexIdentifier(&tasm->Iter, tasm->Lexeme);
		if( !String_NCmpCStr(tasm->Lexeme, "byte", sizeof "byte")
				or !String_NCmpCStr(tasm->Lexeme, "half", sizeof "half")
				or !String_NCmpCStr(tasm->Lexeme, "long", sizeof "long")
				or !String_NCmpCStr(tasm->Lexeme, "word", sizeof "word") )
		{
			addrmode1 |= RegIndirect;
			switch( *tasm->Lexeme->CStr ) {
				case 'b': addrmode1 |= Byte; break;
				case 'h': addrmode1 |= TwoBytes; break;
				case 'l': addrmode1 |= FourBytes; break;
				case 'w': addrmode1 |= EightBytes; break;
			}
			
			SkipWhiteSpace(&tasm->Iter);
			LexIdentifier(&tasm->Iter, tasm->Lexeme);
			if( !LinkMap_HasKey(tasm->Registers, tasm->Lexeme->CStr) ) {
				printf("tasm error: invalid register name '%s' in register indirection on line: %zu\n", tasm->Lexeme->CStr, tasm->CurrLine);
				exit(-1);
			}
			imm.UInt64 = LinkMap_Get(tasm->Registers, tasm->Lexeme->CStr).UInt64;
			
			SkipWhiteSpace(&tasm->Iter);
			// if there's no plus/minus equation, assume `[reg+0]`
			if( *tasm->Iter=='-' or *tasm->Iter=='+' ) {
				tasm->Iter++;
				SkipWhiteSpace(&tasm->Iter);
				if( !IsDecimal(*tasm->Iter) ) {
					printf("tasm error: invalid offset '%s' in register indirection on line: %zu\n", tasm->Lexeme->CStr, tasm->CurrLine);
					exit(-1);
				}
				LexNumber(&tasm->Iter, tasm->Lexeme);
				SkipWhiteSpace(&tasm->Iter);
				const bool isbinary = !String_NCmpCStr(tasm->Lexeme, "0b", 2) or !String_NCmpCStr(tasm->Lexeme, "0B", 2) ? true : false;
				offset = strtol(isbinary ? tasm->Lexeme->CStr+2 : tasm->Lexeme->CStr, NULL, isbinary ? 2 : 0);
			}
			if( *tasm->Iter != ']' ) {
				printf("tasm error: missing closing ']' bracket in register indirection on line: %zu\n", tasm->CurrLine);
				exit(-1);
			}
			tasm->Iter++;
		}
		else {
			printf("tasm error: missing or invalid indirection size '%s' on line: %zu\n", tasm->Lexeme->CStr, tasm->CurrLine);
			exit(-1);
		}
	}
	if( !firstpass ) {
		ByteBuffer_InsertByte(tasm->Bytecode, addrmode1);
		if( addrmode1 & Immediate )
			ByteBuffer_InsertInt(tasm->Bytecode, imm.UInt64, sizeof imm.UInt64);
		else ByteBuffer_InsertByte(tasm->Bytecode, imm.UChar);
		if( IsMemoryDeref )
			ByteBuffer_InsertInt(tasm->Bytecode, (uint64_t)offset, sizeof offset);
	}
	return true;
}

bool TaghaAsm_ParseNoOpsInstr(struct TaghaAsmbler *const restrict tasm, const bool firstpass)
{
	if( !tasm or !tasm->Src or !tasm->Iter )
		return false;
	
	tasm->ProgramCounter += 2;
	SkipWhiteSpace(&tasm->Iter);
	if( !firstpass )
		ByteBuffer_InsertByte(tasm->Bytecode, 0);
	return true;
}

// possible formats:
// lea register, global_var_name
// lea register, func_label (can be a native func as well)
// lea register, [size register + offset]
bool TaghaAsm_ParseLEAInstr(struct TaghaAsmbler *const restrict tasm, const bool firstpass)
{
	if( !tasm or !tasm->Src or !tasm->Iter )
		return false;
	
	tasm->ProgramCounter += 2; // addrmode + register id
	SkipWhiteSpace(&tasm->Iter);
	bool IsMemoryDeref = false;
	uint8_t
		addrmode1=0, reg=0
	;
	union Value imm = (union Value){0};
	int32_t offset = 0;
	
	if( !IsAlphabetic(*tasm->Iter) ) {
		printf("tasm error: LEA opcode takes only a register as the first argument on line: %zu\n", tasm->CurrLine);
		exit(-1);
	}
	
	LexIdentifier(&tasm->Iter, tasm->Lexeme);
	if( !LinkMap_HasKey(tasm->Registers, tasm->Lexeme->CStr) ) {
		printf("tasm error: invalid register name '%s' on line: %zu\n", tasm->Lexeme->CStr, tasm->CurrLine);
		exit(-1);
	}
	reg = LinkMap_Get(tasm->Registers, tasm->Lexeme->CStr).UInt64;
	SkipWhiteSpace(&tasm->Iter);
	if( *tasm->Iter==',' )
		tasm->Iter++;
	SkipWhiteSpace(&tasm->Iter);
	
	// global var name.
	if( IsAlphabetic(*tasm->Iter) ) {
		addrmode1 |= Immediate;
		tasm->ProgramCounter += 8;
		LexIdentifier(&tasm->Iter, tasm->Lexeme);
		if( !LinkMap_HasKey(tasm->VarTable, tasm->Lexeme->CStr) ) {
			printf("tasm error: undefined global var '%s' in LEA opcode on line: %zu\n", tasm->Lexeme->CStr, tasm->CurrLine);
			exit(-1);
		}
		imm.UInt64 = LinkMap_GetIndexByName(tasm->VarTable, tasm->Lexeme->CStr);
		printf("tasm: lea global's '%s' index is '%zu'\n", tasm->Lexeme->CStr, LinkMap_GetIndexByName(tasm->VarTable, tasm->Lexeme->CStr));
	}
	// function label
	else if( *tasm->Iter=='%' ) {
		addrmode1 |= Register;
		tasm->ProgramCounter += 8;
		LexIdentifier(&tasm->Iter, tasm->Lexeme);
		if( !firstpass ) {
			if( !LinkMap_HasKey(tasm->LabelTable, tasm->Lexeme->CStr) ) {
				printf("tasm error: undefined func label '%s' in LEA opcode on line: %zu\n", tasm->Lexeme->CStr, tasm->CurrLine);
				exit(-1);
			}
			imm.UInt64 = LinkMap_GetIndexByName(tasm->LabelTable, tasm->Lexeme->CStr);
			printf("tasm: lea func label's '%s' index is '%zu'\n", tasm->Lexeme->CStr, LinkMap_GetIndexByName(tasm->LabelTable, tasm->Lexeme->CStr));
		}
	}
	// register indirection
	else if( *tasm->Iter=='[' ) {
		tasm->Iter++;
		IsMemoryDeref = true;
		// register indirection operand + register indirect addressing mode + 4 byte offset.
		tasm->ProgramCounter += 5;
		SkipWhiteSpace(&tasm->Iter);
		
		// get memory dereferencing size.
		LexIdentifier(&tasm->Iter, tasm->Lexeme);
		if( !String_NCmpCStr(tasm->Lexeme, "byte", sizeof "byte")
				or !String_NCmpCStr(tasm->Lexeme, "half", sizeof "half")
				or !String_NCmpCStr(tasm->Lexeme, "long", sizeof "long")
				or !String_NCmpCStr(tasm->Lexeme, "word", sizeof "word") )
		{
			addrmode1 |= RegIndirect;
			switch( *tasm->Lexeme->CStr ) {
				case 'b': addrmode1 |= Byte; break;
				case 'h': addrmode1 |= TwoBytes; break;
				case 'l': addrmode1 |= FourBytes; break;
				case 'w': addrmode1 |= EightBytes; break;
			}
			SkipWhiteSpace(&tasm->Iter);
			
			// get register name.
			LexIdentifier(&tasm->Iter, tasm->Lexeme);
			if( !LinkMap_HasKey(tasm->Registers, tasm->Lexeme->CStr) ) {
				printf("tasm error: invalid register name '%s' in register indirection on line: %zu\n", tasm->Lexeme->CStr, tasm->CurrLine);
				exit(-1);
			}
			reg = LinkMap_Get(tasm->Registers, tasm->Lexeme->CStr).UInt64;
			
			SkipWhiteSpace(&tasm->Iter);
			// if there's no plus/minus equation, assume `[reg+0]`.
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
				const bool negate = closer=='-';
				const bool isbinary = !String_NCmpCStr(tasm->Lexeme, "0b", 2) or !String_NCmpCStr(tasm->Lexeme, "0B", 2) ? true : false;
				offset = strtol(isbinary ? tasm->Lexeme->CStr+2 : tasm->Lexeme->CStr, NULL, isbinary ? 2 : 0);
				offset = negate ? -offset : offset;
				printf("offset == %i\n", offset);
			}
			if( *tasm->Iter != ']' ) {
				printf("tasm error: missing closing ']' bracket in register indirection on line: %zu\n", tasm->CurrLine);
				exit(-1);
			}
			tasm->Iter++;
		}
		else {
			printf("tasm error: missing or invalid indirection size '%s' on line: %zu\n", tasm->Lexeme->CStr, tasm->CurrLine);
			exit(-1);
		}
	}
	else {
		printf("tasm error: invalid second operand for LEA opcode on line: %zu\n", tasm->CurrLine);
		exit(-1);
	}
	
	if( !firstpass ) {
		ByteBuffer_InsertByte(tasm->Bytecode, addrmode1);
		ByteBuffer_InsertByte(tasm->Bytecode, reg);
		if( addrmode1 & (Immediate|Register) )
			ByteBuffer_InsertInt(tasm->Bytecode, imm.UInt64, sizeof imm.UInt64);
		else ByteBuffer_InsertByte(tasm->Bytecode, imm.UChar);
		if( IsMemoryDeref )
			ByteBuffer_InsertInt(tasm->Bytecode, (uint64_t)offset, sizeof offset);
	}
	return true;
}

// syscall func_name, number_of_arguments
bool TaghaAsm_ParseSysCallInstr(struct TaghaAsmbler *const restrict tasm, const bool firstpass)
{
	if( !tasm or !tasm->Src or !tasm->Iter )
		return false;
	
	bool IsMemoryDeref = false;
	uint8_t addrmode1=0;
	union Value imm = (union Value){0};
	int32_t
		offset=0, argcount=0
	;
	
	tasm->ProgramCounter += 6;
	SkipWhiteSpace(&tasm->Iter);
	if( *tasm->Iter=='%' ) {
		addrmode1 |= Immediate;
		LexIdentifier(&tasm->Iter, tasm->Lexeme);
		tasm->ProgramCounter += 8;
		if( !LinkMap_HasKey(tasm->LabelTable, tasm->Lexeme->CStr) ) {
			printf("tasm error: undefined native '%s', you can define the native by writing '$native %s'. on line %zu\n", tasm->Lexeme->CStr, tasm->Lexeme->CStr, tasm->CurrLine);
			exit(-1);
		}
		
		struct LabelInfo *native_label = LinkMap_Get(tasm->LabelTable, tasm->Lexeme->CStr).Ptr;
		if( !native_label->IsNativeFunc ) {
			printf("tasm error: label '%s' is NOT a native, in line %zu\n", tasm->Lexeme->CStr, tasm->CurrLine);
			exit(-1);
		}
		const size_t addr = LinkMap_GetIndexByName(tasm->LabelTable, tasm->Lexeme->CStr);
		imm.Int64 = -(addr + 1);
	}
	else if( *tasm->Iter=='r' ) {
		addrmode1 |= Register;
		LexIdentifier(&tasm->Iter, tasm->Lexeme);
		if( !LinkMap_HasKey(tasm->Registers, tasm->Lexeme->CStr) ) {
			printf("tasm error: invalid register name '%s' in line: %zu\n", tasm->Lexeme->CStr, tasm->CurrLine);
			exit(-1);
		}
		tasm->ProgramCounter++;
		imm.UInt64 = LinkMap_Get(tasm->Registers, tasm->Lexeme->CStr).UInt64;
	}
	else if( *tasm->Iter=='[' ) {
		tasm->Iter++;
		IsMemoryDeref = true;
		
		// register indirection operand + register indirect addressing mode + 4 byte offset.
		tasm->ProgramCounter += 4;
		SkipWhiteSpace(&tasm->Iter);
		LexIdentifier(&tasm->Iter, tasm->Lexeme);
		if( !String_NCmpCStr(tasm->Lexeme, "byte", sizeof "byte")
				or !String_NCmpCStr(tasm->Lexeme, "half", sizeof "half")
				or !String_NCmpCStr(tasm->Lexeme, "long", sizeof "long")
				or !String_NCmpCStr(tasm->Lexeme, "word", sizeof "word") )
		{
			addrmode1 |= RegIndirect;
			switch( *tasm->Lexeme->CStr ) {
				case 'b': addrmode1 |= Byte; break;
				case 'h': addrmode1 |= TwoBytes; break;
				case 'l': addrmode1 |= FourBytes; break;
				case 'w': addrmode1 |= EightBytes; break;
			}
			
			SkipWhiteSpace(&tasm->Iter);
			LexIdentifier(&tasm->Iter, tasm->Lexeme);
			if( !LinkMap_HasKey(tasm->Registers, tasm->Lexeme->CStr) ) {
				printf("tasm error: invalid register name '%s' in register indirection on line: %zu\n", tasm->Lexeme->CStr, tasm->CurrLine);
				exit(-1);
			}
			imm.UInt64 = LinkMap_Get(tasm->Registers, tasm->Lexeme->CStr).UInt64;
			
			SkipWhiteSpace(&tasm->Iter);
			// if there's no plus/minus equation, assume `[reg+0]`
			if( *tasm->Iter=='-' or *tasm->Iter=='+' ) {
				tasm->Iter++;
				SkipWhiteSpace(&tasm->Iter);
				if( !IsDecimal(*tasm->Iter) ) {
					printf("tasm error: invalid offset '%s' in register indirection on line: %zu\n", tasm->Lexeme->CStr, tasm->CurrLine);
					exit(-1);
				}
				LexNumber(&tasm->Iter, tasm->Lexeme);
				SkipWhiteSpace(&tasm->Iter);
				const bool isbinary = !String_NCmpCStr(tasm->Lexeme, "0b", 2) or !String_NCmpCStr(tasm->Lexeme, "0B", 2) ? true : false;
				offset = strtol(isbinary ? tasm->Lexeme->CStr+2 : tasm->Lexeme->CStr, NULL, isbinary ? 2 : 0);
			}
			if( *tasm->Iter != ']' ) {
				printf("tasm error: missing closing ']' bracket in register indirection on line: %zu\n", tasm->CurrLine);
				exit(-1);
			}
			tasm->Iter++;
		}
		else {
			printf("tasm error: missing or invalid indirection size '%s' on line: %zu\n", tasm->Lexeme->CStr, tasm->CurrLine);
			exit(-1);
		}
	}
	else {
		printf("tasm error: invalid syntax for syscall! line: %zu\n", tasm->CurrLine);
		exit(-1);
	}
	
	SkipWhiteSpace(&tasm->Iter);
	if( *tasm->Iter==',' )
		tasm->Iter++;
	
	SkipWhiteSpace(&tasm->Iter);
	if( !IsDecimal(*tasm->Iter) ) {
		printf("tasm error: missing argument number for syscall! line: %zu\n", tasm->CurrLine);
		exit(-1);
	}
	LexNumber(&tasm->Iter, tasm->Lexeme);
	const bool isbinary = !String_NCmpCStr(tasm->Lexeme, "0b", 2) or !String_NCmpCStr(tasm->Lexeme, "0B", 2) ? true : false;
	argcount = strtoul(isbinary ? tasm->Lexeme->CStr+2 : tasm->Lexeme->CStr, NULL, isbinary ? 2 : 0);
	SkipWhiteSpace(&tasm->Iter);
	
	if( !firstpass ) {
		ByteBuffer_InsertByte(tasm->Bytecode, addrmode1);
		ByteBuffer_InsertInt(tasm->Bytecode, argcount, sizeof argcount);
		if( addrmode1 & Immediate )
			ByteBuffer_InsertInt(tasm->Bytecode, imm.UInt64, sizeof imm.UInt64);
		else ByteBuffer_InsertByte(tasm->Bytecode, imm.UChar);
		if( IsMemoryDeref )
			ByteBuffer_InsertInt(tasm->Bytecode, (uint64_t)offset, sizeof offset);
	}
	return true;
}

#ifdef FLOATING_POINT_OPS
bool TaghaAsm_ParseFloatConvInstr(struct TaghaAsmbler *const restrict tasm, const bool firstpass)
{
	if( !tasm or !tasm->Src or !tasm->Iter )
		return false;
	
	tasm->ProgramCounter += 2;
	SkipWhiteSpace(&tasm->Iter);
	uint8_t reg = 0;
	if( !IsAlphabetic(*tasm->Iter) ) {
		printf("tasm error: float conversion ops only take register arguments! line: %zu\n", tasm->CurrLine);
		exit(-1)
	}
	LexIdentifier(&tasm->Iter, tasm->Lexeme);
	if( !LinkMap_HasKey(tasm->Registers, tasm->Lexeme->CStr) ) {
		printf("tasm error: invalid register name '%s' in register indirection on line: %zu\n", tasm->Lexeme->CStr, tasm->CurrLine);
		exit(-1);
	}
	reg = LinkMap_Get(tasm->Registers, tasm->Lexeme->CStr).UInt64;
	if( !firstpass ) {
		ByteBuffer_InsertByte(tasm->Bytecode, Register);
		ByteBuffer_InsertByte(tasm->Bytecode, reg);
	}
	return true;
}
#endif


bool TaghaAsm_Assemble(struct TaghaAsmbler *const restrict tasm)
{
	if( !tasm or !tasm->Src )
		return false;
	
	// set up our data.
	tasm->Lexeme = &(struct String){0},
	tasm->LabelTable = &(struct LinkMap){0},
	tasm->VarTable = &(struct LinkMap){0},
	tasm->Opcodes = &(struct LinkMap){0},
	tasm->Registers = &(struct LinkMap){0};
	tasm->Bytecode = &(struct ByteBuffer){0};
	
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
	
	LinkMap_Insert(tasm->Registers, "RTAW", (union Value){.UInt64 = regTaw});
	LinkMap_Insert(tasm->Registers, "rtaw", (union Value){.UInt64 = regTaw});
	
	LinkMap_Insert(tasm->Registers, "RTAW", (union Value){.UInt64 = regTaw});
	LinkMap_Insert(tasm->Registers, "rtaw", (union Value){.UInt64 = regTaw});
	
	LinkMap_Insert(tasm->Registers, "RSP", (union Value){.UInt64 = regStk});
	LinkMap_Insert(tasm->Registers, "rsp", (union Value){.UInt64 = regStk});
	
	LinkMap_Insert(tasm->Registers, "RBP", (union Value){.UInt64 = regBase});
	LinkMap_Insert(tasm->Registers, "rbp", (union Value){.UInt64 = regBase});
	
	LinkMap_Insert(tasm->Registers, "RIP", (union Value){.UInt64 = regInstr});
	LinkMap_Insert(tasm->Registers, "rip", (union Value){.UInt64 = regInstr});
	
	// set up our instruction set!
	#define X(x) LinkMap_Insert(tasm->Opcodes, #x, (union Value){.UInt64 = x});
		INSTR_SET;
	#undef X
	
	/* FIRST PASS. Collect labels and their PC relative addresses */
	#define MAX_LINE_CHARS 2048
	char line_buffer[MAX_LINE_CHARS] = {0};
	
#ifdef DEBUG
	puts("\ntasm: FIRST PASS Begin!\n");
#endif
	
	for( tasm->Iter = fgets(line_buffer, MAX_LINE_CHARS, tasm->Src) ; tasm->Iter ; tasm->Iter = fgets(line_buffer, MAX_LINE_CHARS, tasm->Src) ) {
		// set up first line for error checks.
		tasm->CurrLine++;
	#ifdef DEBUG
		//printf("tasm debug: printing line:: '%s'\n", tasm->Iter);
	#endif
		while( *tasm->Iter ) {
			String_Del(tasm->Lexeme);
			// skip whitespace.
			SkipWhiteSpace(&tasm->Iter);
			
			// skip to next line if comment or directive.
			if( *tasm->Iter=='\n' or *tasm->Iter==';' or *tasm->Iter=='#' )
				break;
			
			// parse the directives!
			else if( *tasm->Iter=='$' ) {
				LexIdentifier(&tasm->Iter, tasm->Lexeme);
				if( !String_NCmpCStr(tasm->Lexeme, "$stacksize", sizeof "$stacksize") ) {
					TaghaAsm_ParseStackDirective(tasm);
					if( !tasm->Stacksize )
						tasm->Stacksize = 128;
				#ifdef DEBUG
					printf("tasm: Stack size set to: %u\n", tasm->Stacksize);
				#endif
				}
				else if( !String_NCmpCStr(tasm->Lexeme, "$global", sizeof "$global") )
					TaghaAsm_ParseGlobalVarDirective(tasm);
				else if( !String_NCmpCStr(tasm->Lexeme, "$native", sizeof "$native") )
					TaghaAsm_ParseNativeDirective(tasm);
				break;
			}
			
			// holy cannoli, we found a label!
			else if( *tasm->Iter=='.' or *tasm->Iter=='%' ) {
				const bool funclbl = *tasm->Iter == '%';
				// the dot or percent is added to our lexeme
				LexIdentifier(&tasm->Iter, tasm->Lexeme);
				
				if( *tasm->Iter != ':' ) {
					printf("tasm error: label is missing ':' colon! on line: %zu\n", tasm->CurrLine);
					exit(-1);
				}
				tasm->Iter++;
				if( !IsAlphabetic(tasm->Lexeme->CStr[1]) ) {
					printf("tasm error: %s labels must have alphabetic names! line: %zu\n", funclbl ? "function" : "jump", tasm->CurrLine);
					exit(-1);
				}
				else if( LinkMap_HasKey(tasm->LabelTable, tasm->Lexeme->CStr) ) {
					printf("tasm error: redefinition of label '%s' on line: %zu\n", tasm->Lexeme->CStr, tasm->CurrLine);
					exit(-1);
				}
				
				struct LabelInfo *label = calloc(1, sizeof *label);
				if( !label ) {
					puts("tasm error: OUT OF MEMORY! Exiting...");
					exit(-1);
				}
				label->Addr = tasm->ProgramCounter;
				label->IsFunc = funclbl;
#ifdef DEBUG
				printf("%s Label '%s' is located at address: %zu\n", funclbl ? "Func" : "Local", tasm->Lexeme->CStr, tasm->ProgramCounter);
#endif
				LinkMap_Insert(tasm->LabelTable, tasm->Lexeme->CStr, (union Value){.Ptr = label});
			}
			// it's an opcode!
			if( IsAlphabetic(*tasm->Iter) ) {
				LexIdentifier(&tasm->Iter, tasm->Lexeme);
				if( !LinkMap_HasKey(tasm->Opcodes, tasm->Lexeme->CStr) ) {
					printf("tasm error: unknown opcode '%s' on line: %zu\n", tasm->Lexeme->CStr, tasm->CurrLine);
					exit(-1);
				}
				uint8_t opcode = LinkMap_Get(tasm->Opcodes, tasm->Lexeme->CStr).UInt64;
				switch( opcode ) {
					case mov:
					case add: case sub: case mul: case divi: case mod:
					case andb: case orb: case xorb: case shl: case shr:
					case lt: case gt: case cmp: case neq:
						TaghaAsm_ParseTwoOpInstr(tasm, true);
						break;
					
					case push: case pop: case notb:
					case inc: case dec: case neg:
					case jz: case jnz: case jmp: case call:
						TaghaAsm_ParseOneOpInstr(tasm, true);
						break;
					
					case ret: case nop:
						TaghaAsm_ParseNoOpsInstr(tasm, true);
						break;
					case lea:
						TaghaAsm_ParseLEAInstr(tasm, true);
						break;
					case syscall:
						TaghaAsm_ParseSysCallInstr(tasm, true);
						break;
				#ifdef FLOATING_POINT_OPS
					case flt2dbl: case dbl2flt:
						TaghaAsm_ParseFloatConvInstr(tasm, true);
						break;
					case addf: case subf: case mulf: case divf:
					case incf: case decf: case negf:
					case ltf: case gtf: case cmpf: case neqf:
						break;
				#endif
				}
			}
		}
	}
#ifdef DEBUG
	puts("\ntasm: FIRST PASS End!\n");
#endif
	rewind(tasm->Src);
	
#ifdef DEBUG
	puts("\ntasm: SECOND PASS Start!\n");
#endif
	tasm->CurrLine = 0;
	
	for( tasm->Iter = fgets(line_buffer, MAX_LINE_CHARS, tasm->Src) ; tasm->Iter ; tasm->Iter = fgets(line_buffer, MAX_LINE_CHARS, tasm->Src) ) {
		// set up first line for error checks.
		tasm->CurrLine++;
	#ifdef DEBUG
		//printf("tasm debug: printing line:: '%s'\n", tasm->Iter);
	#endif
		while( *tasm->Iter ) {
			String_Del(tasm->Lexeme);
			// skip whitespace.
			SkipWhiteSpace(&tasm->Iter);
			
			// skip to next line if comment or directive.
			if( *tasm->Iter=='\n' or *tasm->Iter==';' or *tasm->Iter=='#' or *tasm->Iter=='$' )
				break;
			
			// skip labels in second pass.
			else if( *tasm->Iter=='.' or *tasm->Iter=='%' ) {
				LexIdentifier(&tasm->Iter, tasm->Lexeme);
				tasm->Iter++;
			}
			// parse opcode!
			if( IsAlphabetic(*tasm->Iter) ) {
				LexIdentifier(&tasm->Iter, tasm->Lexeme);
				if( !LinkMap_HasKey(tasm->Opcodes, tasm->Lexeme->CStr) ) {
					printf("tasm error: unknown opcode '%s' on line: %zu\n", tasm->Lexeme->CStr, tasm->CurrLine);
					exit(-1);
				}
				uint8_t opcode = LinkMap_Get(tasm->Opcodes, tasm->Lexeme->CStr).UInt64;
				ByteBuffer_InsertByte(tasm->Bytecode, opcode);
				switch( opcode ) {
					case mov:
					case add: case sub: case mul: case divi: case mod:
					case andb: case orb: case xorb: case shl: case shr:
					case lt: case gt: case cmp: case neq:
						TaghaAsm_ParseTwoOpInstr(tasm, false);
						break;
					
					case push: case pop: case notb:
					case inc: case dec: case neg:
					case jz: case jnz: case jmp: case call:
						TaghaAsm_ParseOneOpInstr(tasm, false);
						break;
					
					case ret: case nop:
						TaghaAsm_ParseNoOpsInstr(tasm, false);
						break;
					case lea:
						TaghaAsm_ParseLEAInstr(tasm, false);
						break;
					case syscall:
						TaghaAsm_ParseSysCallInstr(tasm, false);
						break;
				#ifdef FLOATING_POINT_OPS
					case flt2dbl: case dbl2flt:
						TaghaAsm_ParseFloatConvInstr(tasm, false);
						break;
					case addf: case subf: case mulf: case divf:
					case incf: case decf: case negf:
					case ltf: case gtf: case cmpf: case neqf:
						break;
				#endif
				}
			}
		}
	}
	
#ifdef DEBUG
	puts("\ntasm: SECOND PASS End!\n");
#endif
	
	
	
	LinkMap_Del(tasm->LabelTable);
	LinkMap_Del(tasm->VarTable);
	LinkMap_Del(tasm->Opcodes);
	LinkMap_Del(tasm->Registers);
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

int main(int argc, char *argv[static argc])
{
	if( !argv[1] ) {
		return -1;
	}
	
	FILE *tasmfile = fopen(argv[1], "r");
	if( !tasmfile )
		return -1;
	
	struct TaghaAsmbler *tasm = &(struct TaghaAsmbler){0};
	tasm->Src = tasmfile;
	tasm->SrcSize = GetFileSize(tasmfile);
	String_InitStr(&tasm->OutputName, argv[1]);
	return !TaghaAsm_Assemble(tasm);
}
