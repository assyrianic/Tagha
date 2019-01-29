#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "tagha_assembler.h"


bool tagha_asm_parse_RegRegInstr(struct TaghaAsmbler *, bool);
bool tagha_asm_parse_OneRegInstr(struct TaghaAsmbler *, bool);
bool tagha_asm_parse_OneImmInstr(struct TaghaAsmbler *, bool);
bool tagha_asm_parse_RegMemInstr(struct TaghaAsmbler *, bool);
bool tagha_asm_parse_MemRegInstr(struct TaghaAsmbler *, bool);
bool tagha_asm_parse_RegImmInstr(struct TaghaAsmbler *, bool);

static inline bool is_space(const char c)
{
	return( c == ' ' || c == '\t' || c == '\r' || c == '\v' || c == '\f' );
}

static inline bool is_decimal(const char c)
{
	return( c >= '0' && c <= '9' );
}

static inline bool is_hex(const char c)
{
	return( (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F') || is_decimal(c) );
}

static inline bool is_binary(const char c)
{
	return( c=='0' || c=='1' );
}

static inline bool is_octal(const char c)
{
	return( c >= '0' && c <= '7' );
}

static inline bool is_possible_ident(const char c)
{
	return( (c >= 'a' && c <= 'z')
		|| (c >= 'A' && c <= 'Z')
		|| c=='_' || c=='@' || c=='$'
		|| (c >= '0' && c <= '9')
		|| c < -1 );
}

static inline bool is_alphabetic(const char c)
{
	return( (c >= 'a' && c <= 'z')
		|| (c >= 'A' && c <= 'Z')
		|| c=='_' || c=='@' || c=='$'
		|| c < -1 );
}

inline bool is_valid_ucn(const int32_t c) {
	if( 0xD800<=c && c<=0xDFFF )
		return false;
	else return( 0xA0<=c||c=='$'||c=='@'||c=='`' );
}


static bool skip_whitespace(char **restrict strRef)
{
	if( !*strRef || !**strRef )
		return false;
	
	while( **strRef && is_space(**strRef) )
		(*strRef)++;
	return **strRef != 0;
}

static bool lex_identifier(char **restrict strRef, struct HarbolString *const restrict strobj)
{
	if( !*strRef || !**strRef || !strobj )
		return false;
	
	harbol_string_del(strobj);
	harbol_string_add_char(strobj, *(*strRef)++);
	while( **strRef && is_possible_ident(**strRef) )
		harbol_string_add_char(strobj, *(*strRef)++);
	
	return strobj->Len > 0;
}

static bool lex_number(char **restrict strRef, struct HarbolString *const restrict strobj)
{
	if( !*strRef || !**strRef || !strobj )
		return false;
	
	harbol_string_del(strobj);
	if( **strRef=='0' ) {
		if( (*strRef)[1]=='x' || (*strRef)[1]=='X' ) { // hexadecimal.
			harbol_string_add_cstr(strobj, (*strRef)[1]=='x' ? "0x" : "0X");
			(*strRef) += 2;
			while( **strRef && is_hex(**strRef) )
				harbol_string_add_char(strobj, *(*strRef)++);
		}
		else if( (*strRef)[1]=='b' || (*strRef)[1]=='B' ) { // binary.
			harbol_string_add_cstr(strobj, (*strRef)[1]=='b' ? "0b" : "0B");
			(*strRef) += 2;
			while( **strRef && (**strRef=='1' || **strRef=='0') )
				harbol_string_add_char(strobj, *(*strRef)++);
		}
		else { // octal.
			harbol_string_add_char(strobj, *(*strRef)++);
			while( **strRef && is_octal(**strRef) )
				harbol_string_add_char(strobj, *(*strRef)++);
		}
	}
	else {
		while( **strRef && is_decimal(**strRef) )
			harbol_string_add_char(strobj, *(*strRef)++);
	}
	/*
	if( **strRef=='.' ) { // add float support.
		harbol_string_add_char(strobj, *(*strRef)++);
		while( **strRef && is_decimal(**strRef) )
			harbol_string_add_char(strobj, *(*strRef)++);
	}
	*/
	return strobj->Len > 0;
}

void tagha_asm_err_out(struct TaghaAsmbler *const restrict tasm, const char err[restrict], ...)
{
	va_list args;
	va_start(args, err);
	printf("Tagha Assembler Error: **** ");
	vprintf(err, args);
	printf(" **** | line %zu in file '%s'\n", tasm->CurrLine, tasm->OutputName.CStr);
	va_end(args);
	tasm->Error = true;
}

void write_utf8(struct TaghaAsmbler *const tasm, const int32_t chr)
{
	const uint32_t rune = (uint32_t)chr;
	if (rune < 0x80) {
		harbol_string_add_char(tasm->Lexeme, rune);
		return;
	}
	if (rune < 0x800) {
		harbol_string_add_char(tasm->Lexeme, 0xC0 | (rune >> 6));
		harbol_string_add_char(tasm->Lexeme, 0x80 | (rune & 0x3F));
		return;
	}
	if (rune < 0x10000) {
		harbol_string_add_char(tasm->Lexeme, 0xE0 | (rune >> 12));
		harbol_string_add_char(tasm->Lexeme, 0x80 | ((rune >> 6) & 0x3F));
		harbol_string_add_char(tasm->Lexeme, 0x80 | (rune & 0x3F));
		return;
	}
	if (rune < 0x200000) {
		harbol_string_add_char(tasm->Lexeme, 0xF0 | (rune >> 18));
		harbol_string_add_char(tasm->Lexeme, 0x80 | ((rune >> 12) & 0x3F));
		harbol_string_add_char(tasm->Lexeme, 0x80 | ((rune >> 6) & 0x3F));
		harbol_string_add_char(tasm->Lexeme, 0x80 | (rune & 0x3F));
		return;
	}
	tagha_asm_err_out(tasm, "invalid unicode character: \\U%08x", rune);
}

// $stacksize <number>
bool tagha_asm_parse_stack_directive(struct TaghaAsmbler *const restrict tasm)
{
	if( !tasm || !tasm->Src || !tasm->Iter )
		return false;
	
	skip_whitespace(&tasm->Iter);
	if( !is_decimal(*tasm->Iter) ) {
		tagha_asm_err_out(tasm, "stack size directive requires a valid number!");
		return false;
	}
	lex_number(&tasm->Iter, tasm->Lexeme);
	const bool is_binary = !harbol_string_ncmpcstr(tasm->Lexeme, "0b", 2) || !harbol_string_ncmpcstr(tasm->Lexeme, "0B", 2) ? true : false;
	tasm->Stacksize = strtoul(is_binary ? tasm->Lexeme->CStr+2 : tasm->Lexeme->CStr, NULL, is_binary ? 2 : 0);
	return true;
}

// $global varname bytes ...
bool tagha_asm_parse_globalvar_directive(struct TaghaAsmbler *const restrict tasm)
{
	if( !tasm || !tasm->Src || !tasm->Iter )
		return false;
	
	skip_whitespace(&tasm->Iter);
	if( !is_alphabetic(*tasm->Iter) ) {
		tagha_asm_err_out(tasm, "global directive requires the 1st argument to be a variable name!");
		return false;
	}
	
	struct HarbolString *varname = &(struct HarbolString){0};
	lex_identifier(&tasm->Iter, varname);
	skip_whitespace(&tasm->Iter);
	if( *tasm->Iter==',' )
		tasm->Iter++;
	skip_whitespace(&tasm->Iter);
	
	if( !is_decimal(*tasm->Iter) ) {
		tagha_asm_err_out(tasm, "missing byte size number in global directive");
		return false;
	}
	
	lex_number(&tasm->Iter, tasm->Lexeme);
	const bool is_binary = !harbol_string_ncmpcstr(tasm->Lexeme, "0b", 2) || !harbol_string_ncmpcstr(tasm->Lexeme, "0B", 2) ? true : false;
	size_t bytes = strtoull(is_binary ? tasm->Lexeme->CStr+2 : tasm->Lexeme->CStr, NULL, is_binary ? 2 : 0);
	struct HarbolByteBuffer *vardata = harbol_bytebuffer_new();
	if( !vardata ) {
		tagha_asm_err_out(tasm, "out of memory trying to allocate bytebuffer for global var data!");
		return false;
	}
	skip_whitespace(&tasm->Iter);
	if( *tasm->Iter==',' )
		tasm->Iter++;
	
	while( bytes ) {
		skip_whitespace(&tasm->Iter);
		if( *tasm->Iter=='"' ) { // string
			harbol_string_del(tasm->Lexeme);
			const char quote = *tasm->Iter++;
			while( *tasm->Iter && *tasm->Iter != quote ) {
				const char charval = *tasm->Iter++;
				if( !charval ) { // sudden EOF?
					tagha_asm_err_out(tasm, "sudden EOF while reading global directive string!");
					return false;
				}
				// handle escape chars
				if( charval=='\\' ) {
					const char escape = *tasm->Iter++;
					switch( escape ) {
						case 'a': harbol_string_add_char(tasm->Lexeme, '\a'); break;
						case 'b': harbol_string_add_char(tasm->Lexeme, '\b'); break;
						case 'n': harbol_string_add_char(tasm->Lexeme, '\n'); break;
						case 'r': harbol_string_add_char(tasm->Lexeme, '\r'); break;
						case 't': harbol_string_add_char(tasm->Lexeme, '\t'); break;
						case 'v': harbol_string_add_char(tasm->Lexeme, '\v'); break;
						case 'f': harbol_string_add_char(tasm->Lexeme, '\f'); break;
						case '0': harbol_string_add_char(tasm->Lexeme, '\0'); break;
						case 'U': {
							int32_t r = 0;
							const size_t encoding = 4;
							for( size_t i=0 ; i<encoding*2 ; i++ ) {
								const int32_t c = *tasm->Iter++;
								switch( c ) {
									case '0' ... '9':
										r = (r << 4) | (c - '0'); break;
									case 'a' ... 'f':
										r = (r << 4) | (c - 'a' + 10); break;
									case 'A' ... 'F':
										r = (r << 4) | (c - 'A' + 10); break;
									default:
										tagha_asm_err_out(tasm, "invalid unicode character: '%c'", c);
										return false;
								}
							}
							if( !is_valid_ucn(r) ) {
								tagha_asm_err_out(tasm, "invalid universal character: '\\U%0*x'", encoding, r);
								return false;
							} else write_utf8(tasm, r);
							break;
						}
						case 'u': {
							int32_t r = 0;
							const size_t encoding = 2;
							for( size_t i=0 ; i<encoding*2 ; i++ ) {
								const int32_t c = *tasm->Iter++;
								switch( c ) {
									case '0' ... '9':
										r = (r << 4) | (c - '0'); break;
									case 'a' ... 'f':
										r = (r << 4) | (c - 'a' + 10); break;
									case 'A' ... 'F':
										r = (r << 4) | (c - 'A' + 10); break;
									default:
										tagha_asm_err_out(tasm, "invalid unicode character: '%c'", c);
										return false;
								}
							}
							if( !is_valid_ucn(r) ) {
								tagha_asm_err_out(tasm, "invalid universal character: '\\u%0*x'", encoding, r);
								return false;
							} else write_utf8(tasm, r);
							break;
						}
						default: harbol_string_add_char(tasm->Lexeme, escape);
					}
				} else {
					harbol_string_add_char(tasm->Lexeme, charval);
				}
			}
#ifdef TASM_DEBUG
			printf("tasm: global string '%s'\n", tasm->Lexeme->CStr);
#endif
			harbol_bytebuffer_insert_cstr(vardata, tasm->Lexeme->CStr, tasm->Lexeme->Len);
			bytes = 0;
		}
		else if( *tasm->Iter==',' )
			tasm->Iter++;
		else if( is_decimal(*tasm->Iter) ) {
			lex_number(&tasm->Iter, tasm->Lexeme);
			const bool is_binary = !harbol_string_ncmpcstr(tasm->Lexeme, "0b", 2) || !harbol_string_ncmpcstr(tasm->Lexeme, "0B", 2) ? true : false;
			const uint64_t data = strtoull(is_binary ? tasm->Lexeme->CStr+2 : tasm->Lexeme->CStr, NULL, is_binary ? 2 : 0);
			if( data ) {
				tagha_asm_err_out(tasm, "single numeric arguments for global vars must be 0!");
				return false;
			}
			while( bytes-- )
				harbol_bytebuffer_insert_byte(vardata, 0);
			break;
		}
		else if( is_alphabetic(*tasm->Iter) ) {
			lex_identifier(&tasm->Iter, tasm->Lexeme);
			if( !harbol_string_ncmpcstr(tasm->Lexeme, "byte", sizeof "byte") ) {
				skip_whitespace(&tasm->Iter);
				lex_number(&tasm->Iter, tasm->Lexeme);
				const bool is_binary = !harbol_string_ncmpcstr(tasm->Lexeme, "0b", 2) || !harbol_string_ncmpcstr(tasm->Lexeme, "0B", 2) ? true : false;
				const uint8_t data = strtoull(is_binary ? tasm->Lexeme->CStr+2 : tasm->Lexeme->CStr, NULL, is_binary ? 2 : 0);
				harbol_bytebuffer_insert_byte(vardata, data);
				bytes--;
			}
			else if( !harbol_string_ncmpcstr(tasm->Lexeme, "half", sizeof "half") ) {
				skip_whitespace(&tasm->Iter);
				lex_number(&tasm->Iter, tasm->Lexeme);
				const bool is_binary = !harbol_string_ncmpcstr(tasm->Lexeme, "0b", 2) || !harbol_string_ncmpcstr(tasm->Lexeme, "0B", 2) ? true : false;
				const uint16_t data = strtoull(is_binary ? tasm->Lexeme->CStr+2 : tasm->Lexeme->CStr, NULL, is_binary ? 2 : 0);
				harbol_bytebuffer_insert_integer(vardata, data, sizeof(uint16_t));
				bytes -= sizeof(uint16_t);
			}
			else if( !harbol_string_ncmpcstr(tasm->Lexeme, "long", sizeof "long") ) {
				skip_whitespace(&tasm->Iter);
				lex_number(&tasm->Iter, tasm->Lexeme);
				const bool is_binary = !harbol_string_ncmpcstr(tasm->Lexeme, "0b", 2) || !harbol_string_ncmpcstr(tasm->Lexeme, "0B", 2) ? true : false;
				const uint32_t data = strtoull(is_binary ? tasm->Lexeme->CStr+2 : tasm->Lexeme->CStr, NULL, is_binary ? 2 : 0);
				harbol_bytebuffer_insert_integer(vardata, data, sizeof(uint32_t));
				bytes -= sizeof(uint32_t);
			}
			else if( !harbol_string_ncmpcstr(tasm->Lexeme, "word", sizeof "word") ) {
				skip_whitespace(&tasm->Iter);
				lex_number(&tasm->Iter, tasm->Lexeme);
				const bool is_binary = !harbol_string_ncmpcstr(tasm->Lexeme, "0b", 2) || !harbol_string_ncmpcstr(tasm->Lexeme, "0B", 2) ? true : false;
				const uint64_t data = strtoull(is_binary ? tasm->Lexeme->CStr+2 : tasm->Lexeme->CStr, NULL, is_binary ? 2 : 0);
				harbol_bytebuffer_insert_integer(vardata, data, sizeof(uint64_t));
				bytes -= sizeof(uint64_t);
			}
		}
		else {
			tagha_asm_err_out(tasm, "global var directive data set is incomplete, must be equal to bytesize given.");
			return false;
		}
	}
#ifdef TASM_DEBUG
	printf("tasm debug: vardata.Count: %zu\n", vardata->Count);
#endif
	/*
	printf("tasm: adding global var '%s'\n", varname->CStr);
	for( size_t i=0 ; i<harbol_bytebuffer_get_count(vardata) ; i++ )
		printf("tasm: global var[%zu] == %u\n", i, vardata->Buffer[i]);
	*/
	harbol_linkmap_insert(tasm->VarTable, varname->CStr, (union HarbolValue){.ByteBufferPtr=vardata});
	harbol_string_del(varname);
	return true;
}

// $native %name
bool tagha_asm_parse_native_directive(struct TaghaAsmbler *const restrict tasm)
{
	if( !tasm || !tasm->Src || !tasm->Iter )
		return false;
	
	skip_whitespace(&tasm->Iter);
	if( *tasm->Iter != '%' ) {
		tagha_asm_err_out(tasm, "missing %% for native name declaration!");
		return false;
	}
	lex_identifier(&tasm->Iter, tasm->Lexeme);
	
	struct LabelInfo *label = calloc(1, sizeof *label);
	if( !label ) {
		tagha_asm_err_out(tasm, "out of memory trying to allocate label!");
		return false;
	}
	label->Addr = 0;
	label->IsFunc = label->IsNativeFunc = true;
	union HarbolValue native = (union HarbolValue){.Ptr = label};
	harbol_linkmap_insert(tasm->FuncTable, tasm->Lexeme->CStr, native);
#ifdef TASM_DEBUG
	printf("tasm: added native function '%s'\n", tasm->Lexeme->CStr);
#endif
	return true;
}

int64_t lex_imm_value(struct TaghaAsmbler *const restrict tasm)
{
	lex_number(&tasm->Iter, tasm->Lexeme);
	tasm->ProgramCounter += 8;
	const bool is_binary = !harbol_string_ncmpcstr(tasm->Lexeme, "0b", 2) || !harbol_string_ncmpcstr(tasm->Lexeme, "0B", 2) ? true : false;
	return strtoll(is_binary ? tasm->Lexeme->CStr+2 : tasm->Lexeme->CStr, NULL, is_binary ? 2 : 0);
}

uint8_t lex_reg_id(struct TaghaAsmbler *const restrict tasm)
{
	lex_identifier(&tasm->Iter, tasm->Lexeme);
	if( !harbol_linkmap_has_key(tasm->Registers, tasm->Lexeme->CStr) ) {
		tagha_asm_err_out(tasm, "invalid register name '%s'", tasm->Lexeme->CStr);
		return 0;
	}
	tasm->ProgramCounter++;
	return harbol_linkmap_get(tasm->Registers, tasm->Lexeme->CStr).UInt64;
}

void lex_register_deref(struct TaghaAsmbler *const restrict tasm, uint8_t *restrict idref, int32_t *restrict offsetref)
{
	tasm->Iter++; // iterate past '['
	tasm->ProgramCounter += 5; // 1 for byte as register id + 4 byte offset.
	skip_whitespace(&tasm->Iter);
	lex_identifier(&tasm->Iter, tasm->Lexeme);
	if( !harbol_linkmap_has_key(tasm->Registers, tasm->Lexeme->CStr) ) {
		tagha_asm_err_out(tasm, "invalid register name '%s' in register indirection", tasm->Lexeme->CStr);
		return;
	}
	*idref = harbol_linkmap_get(tasm->Registers, tasm->Lexeme->CStr).UInt64;
	*offsetref = 0;
	
	skip_whitespace(&tasm->Iter);
	// if there's no plus/minus equation, assume `[reg+0]`
	// TODO: allow for scaled indexing like * typesize -> [reg+14*4] for easier array accessing.
	const char closer = *tasm->Iter;
	if( closer != '-' && closer != '+' && closer != ']' ) {
		tagha_asm_err_out(tasm, "invalid offset math operator '%c' in register indirection", closer);
		return;
	}
	else if( closer=='-' || closer=='+' ) {
		tasm->Iter++;
		skip_whitespace(&tasm->Iter);
		if( !is_decimal(*tasm->Iter) ) {
			tagha_asm_err_out(tasm, "invalid offset '%s' in register indirection", tasm->Lexeme->CStr);
			return;
		}
		lex_number(&tasm->Iter, tasm->Lexeme);
		skip_whitespace(&tasm->Iter);
		const bool is_binary = !harbol_string_ncmpcstr(tasm->Lexeme, "0b", 2) || !harbol_string_ncmpcstr(tasm->Lexeme, "0B", 2) ? true : false;
		int32_t offset = strtol(is_binary ? tasm->Lexeme->CStr+2 : tasm->Lexeme->CStr, NULL, is_binary ? 2 : 0);
		offset = closer=='-' ? -offset : offset;
		//printf("offset == %i\n", offset);
		*offsetref = offset;
	}
	if( *tasm->Iter != ']' ) {
		tagha_asm_err_out(tasm, "missing closing ']' bracket in register indirection");
		return;
	}
	tasm->Iter++;
}

int64_t lex_label_value(struct TaghaAsmbler *const restrict tasm, const bool firstpass)
{
	const bool isfunclbl = *tasm->Iter=='%';
	lex_identifier(&tasm->Iter, tasm->Lexeme);
	tasm->ProgramCounter += 8;
	if( !isfunclbl )
		harbol_string_add_str(tasm->Lexeme, tasm->ActiveFuncLabel);
	
	if( !firstpass && !harbol_linkmap_has_key(isfunclbl ? tasm->FuncTable : tasm->LabelTable, tasm->Lexeme->CStr) ) {
		tagha_asm_err_out(tasm, "undefined label '%s'", tasm->Lexeme->CStr);
		return 0;
	}
	if( !firstpass ) {
		struct LabelInfo *label = harbol_linkmap_get(isfunclbl ? tasm->FuncTable : tasm->LabelTable, tasm->Lexeme->CStr).Ptr;
		if( !isfunclbl ) {
		#ifdef TASM_DEBUG
			printf("label->Addr (%" PRIu64 ") - tasm->ProgramCounter (%zu) == '%" PRIi64 "'\n", label->Addr, tasm->ProgramCounter, label->Addr - tasm->ProgramCounter);
		#endif
			return label->Addr - tasm->ProgramCounter;
		}
		else {
			return label->IsNativeFunc ? -((int64_t)harbol_linkmap_get_index_by_name(tasm->FuncTable, tasm->Lexeme->CStr) + 1) : ((int64_t)harbol_linkmap_get_index_by_name(tasm->FuncTable, tasm->Lexeme->CStr) + 1);
		}
	}
	return 0;
}



bool tagha_asm_parse_RegRegInstr(struct TaghaAsmbler *const restrict tasm, const bool firstpass)
{
	if( !tasm || !tasm->Src || !tasm->Iter )
		return false;
	
	tasm->ProgramCounter++;
	skip_whitespace(&tasm->Iter);
	
	if( *tasm->Iter != 'r' ) {
		tagha_asm_err_out(tasm, "opcode requires a register as 1st operand.");
		return false;
	}
	const uint8_t destreg = lex_reg_id(tasm);
	
	// ok, let's read the secondary operand!
	skip_whitespace(&tasm->Iter);
	if( *tasm->Iter==',' )
		tasm->Iter++;
	
	skip_whitespace(&tasm->Iter);
	
	if( *tasm->Iter != 'r' ) {
		tagha_asm_err_out(tasm, "opcode requires a register as 2nd operand.");
		return false;
	}
	const uint8_t srcreg = lex_reg_id(tasm);
	
	if( !firstpass ) {
		struct LabelInfo *label = harbol_linkmap_get(tasm->FuncTable, tasm->ActiveFuncLabel->CStr).Ptr;
		if( !label ) {
			tagha_asm_err_out(tasm, "undefined label '%s'.", tasm->ActiveFuncLabel->CStr);
			return false;
		}
		harbol_bytebuffer_insert_byte(&label->Bytecode, destreg);
		harbol_bytebuffer_insert_byte(&label->Bytecode, srcreg);
	}
	return true;
}
bool tagha_asm_parse_OneRegInstr(struct TaghaAsmbler *const restrict tasm, const bool firstpass)
{
	if( !tasm || !tasm->Src || !tasm->Iter )
		return false;
	
	tasm->ProgramCounter++;
	skip_whitespace(&tasm->Iter);
	if( *tasm->Iter != 'r' ) {
		tagha_asm_err_out(tasm, "opcode requires a register as 1st operand.");
		return false;
	}
	const uint8_t regid = lex_reg_id(tasm);
	if( !firstpass ) {
		struct LabelInfo *label = harbol_linkmap_get(tasm->FuncTable, tasm->ActiveFuncLabel->CStr).Ptr;
		if( !label ) {
			tagha_asm_err_out(tasm, "undefined label '%s'.", tasm->ActiveFuncLabel->CStr);
			return false;
		}
		harbol_bytebuffer_insert_byte(&label->Bytecode, regid);
	}
	return true;
}
bool tagha_asm_parse_OneImmInstr(struct TaghaAsmbler *const restrict tasm, const bool firstpass)
{
	if( !tasm || !tasm->Src || !tasm->Iter )
		return false;
	
	tasm->ProgramCounter++;
	skip_whitespace(&tasm->Iter);
	
	int64_t immval = 0;
	// imm value.
	if( is_decimal(*tasm->Iter) )
		immval = lex_imm_value(tasm);
	// label value.
	else if( *tasm->Iter=='.' || *tasm->Iter=='%' )
		immval = lex_label_value(tasm, firstpass);
	// global variable label.
	else if( is_alphabetic(*tasm->Iter) ) {
		lex_identifier(&tasm->Iter, tasm->Lexeme);
		if( !harbol_linkmap_has_key(tasm->VarTable, tasm->Lexeme->CStr) ) {
			tagha_asm_err_out(tasm, "undefined global var '%s' in opcode.", tasm->Lexeme->CStr);
			return false;
		}
		tasm->ProgramCounter += 8;
		immval = harbol_linkmap_get_index_by_name(tasm->VarTable, tasm->Lexeme->CStr);
	#ifdef TASM_DEBUG
		printf("tasm: global's '%s' index is '%zu'\n", tasm->Lexeme->CStr, harbol_linkmap_get_index_by_name(tasm->VarTable, tasm->Lexeme->CStr));
	#endif
	}
	else {
		tagha_asm_err_out(tasm, "opcode requires an immediate or label value as 1st operand.");
		return false;
	}
	
	if( !firstpass ) {
		struct LabelInfo *label = harbol_linkmap_get(tasm->FuncTable, tasm->ActiveFuncLabel->CStr).Ptr;
		if( !label ) {
			tagha_asm_err_out(tasm, "undefined label '%s'.", tasm->ActiveFuncLabel->CStr);
			return false;
		}
		harbol_bytebuffer_insert_integer(&label->Bytecode, immval, sizeof immval);
	}
	return true;
}
bool tagha_asm_parse_RegMemInstr(struct TaghaAsmbler *const restrict tasm, const bool firstpass)
{
	if( !tasm || !tasm->Src || !tasm->Iter )
		return false;
	
	tasm->ProgramCounter++;
	skip_whitespace(&tasm->Iter);
	
	if( *tasm->Iter != 'r' ) {
		tagha_asm_err_out(tasm, "opcode requires a register as 1st operand.");
		return false;
	}
	const uint8_t destreg = lex_reg_id(tasm);
	
	// ok, let's read the secondary operand!
	skip_whitespace(&tasm->Iter);
	if( *tasm->Iter==',' )
		tasm->Iter++;
	
	skip_whitespace(&tasm->Iter);
	
	if( *tasm->Iter != '[' ) {
		tagha_asm_err_out(tasm, "opcode requires a memory dereference as 1st operand.");
		return false;
	}
	uint8_t srcreg;
	int32_t offset;
	lex_register_deref(tasm, &srcreg, &offset);
	
	if( !firstpass ) {
		struct LabelInfo *label = harbol_linkmap_get(tasm->FuncTable, tasm->ActiveFuncLabel->CStr).Ptr;
		if( !label ) {
			tagha_asm_err_out(tasm, "undefined label '%s'.", tasm->ActiveFuncLabel->CStr);
			return false;
		}
		harbol_bytebuffer_insert_byte(&label->Bytecode, destreg);
		harbol_bytebuffer_insert_byte(&label->Bytecode, srcreg);
		harbol_bytebuffer_insert_integer(&label->Bytecode, offset, sizeof offset);
	}
	return true;
}
bool tagha_asm_parse_MemRegInstr(struct TaghaAsmbler *const restrict tasm, const bool firstpass)
{
	if( !tasm || !tasm->Src || !tasm->Iter )
		return false;
	
	tasm->ProgramCounter++;
	skip_whitespace(&tasm->Iter);
	
	if( *tasm->Iter != '[' ) {
		tagha_asm_err_out(tasm, "opcode requires a memory dereference as 1st operand.");
		return false;
	}
	uint8_t destreg;
	int32_t offset;
	lex_register_deref(tasm, &destreg, &offset);
	
	// ok, let's read the secondary operand!
	skip_whitespace(&tasm->Iter);
	if( *tasm->Iter==',' )
		tasm->Iter++;
	
	skip_whitespace(&tasm->Iter);
	if( *tasm->Iter != 'r' ) {
		tagha_asm_err_out(tasm, "opcode requires a register as 2nd operand.");
		return false;
	}
	const uint8_t srcreg = lex_reg_id(tasm);
	
	if( !firstpass ) {
		struct LabelInfo *label = harbol_linkmap_get(tasm->FuncTable, tasm->ActiveFuncLabel->CStr).Ptr;
		if( !label ) {
			tagha_asm_err_out(tasm, "undefined label '%s'.", tasm->ActiveFuncLabel->CStr);
			return false;
		}
		harbol_bytebuffer_insert_byte(&label->Bytecode, destreg);
		harbol_bytebuffer_insert_byte(&label->Bytecode, srcreg);
		harbol_bytebuffer_insert_integer(&label->Bytecode, offset, sizeof offset);
	}
	return true;
}
bool tagha_asm_parse_RegImmInstr(struct TaghaAsmbler *const restrict tasm, const bool firstpass)
{
	if( !tasm || !tasm->Src || !tasm->Iter )
		return false;
	
	tasm->ProgramCounter++;
	skip_whitespace(&tasm->Iter);
	if( *tasm->Iter != 'r' ) {
		tagha_asm_err_out(tasm, "opcode requires a register as 1st operand.");
		return false;
	}
	const uint8_t regid = lex_reg_id(tasm);
	
	// ok, let's read the secondary operand!
	skip_whitespace(&tasm->Iter);
	if( *tasm->Iter==',' )
		tasm->Iter++;
	
	skip_whitespace(&tasm->Iter);
	
	int64_t immval = 0;
	// imm value.
	if( is_decimal(*tasm->Iter) )
		immval = lex_imm_value(tasm);
	// label value.
	else if( *tasm->Iter=='.' || *tasm->Iter=='%' )
		immval = lex_label_value(tasm, firstpass);
	// global variable label.
	else if( is_alphabetic(*tasm->Iter) ) {
		lex_identifier(&tasm->Iter, tasm->Lexeme);
		if( !harbol_linkmap_has_key(tasm->VarTable, tasm->Lexeme->CStr) ) {
			tagha_asm_err_out(tasm, "undefined global var '%s' in opcode.", tasm->Lexeme->CStr);
			return false;
		}
		tasm->ProgramCounter += 8;
		immval = harbol_linkmap_get_index_by_name(tasm->VarTable, tasm->Lexeme->CStr);
	#ifdef TASM_DEBUG
		printf("tasm: global's '%s' index is '%zu'\n", tasm->Lexeme->CStr, harbol_linkmap_get_index_by_name(tasm->VarTable, tasm->Lexeme->CStr));
	#endif
	}
	else {
		tagha_asm_err_out(tasm, "opcode requires an immediate or label value as 2nd operand.");
		return false;
	}
	
	if( !firstpass ) {
		struct LabelInfo *label = harbol_linkmap_get(tasm->FuncTable, tasm->ActiveFuncLabel->CStr).Ptr;
		if( !label ) {
			tagha_asm_err_out(tasm, "undefined label '%s'.", tasm->ActiveFuncLabel->CStr);
			return false;
		}
		harbol_bytebuffer_insert_byte(&label->Bytecode, regid);
		harbol_bytebuffer_insert_integer(&label->Bytecode, immval, sizeof immval);
	}
	return true;
}

bool tagha_asm_assemble(struct TaghaAsmbler *const restrict tasm)
{
	if( !tasm || !tasm->Src )
		return false;
	
	// set up our data.
	tasm->Lexeme = &(struct HarbolString){0},
	tasm->LabelTable = &(struct HarbolLinkMap){0},
	tasm->FuncTable = &(struct HarbolLinkMap){0},
	tasm->VarTable = &(struct HarbolLinkMap){0},
	tasm->Opcodes = &(struct HarbolLinkMap){0},
	tasm->Registers = &(struct HarbolLinkMap){0};
	
	// set up registers + map their IDs
	harbol_linkmap_insert(tasm->Registers, "RALAF", (union HarbolValue){.UInt64 = regAlaf});
	harbol_linkmap_insert(tasm->Registers, "ralaf", (union HarbolValue){.UInt64 = regAlaf});
	
	harbol_linkmap_insert(tasm->Registers, "RBETH", (union HarbolValue){.UInt64 = regBeth});
	harbol_linkmap_insert(tasm->Registers, "rbeth", (union HarbolValue){.UInt64 = regBeth});
	
	harbol_linkmap_insert(tasm->Registers, "RGAMAL", (union HarbolValue){.UInt64 = regGamal});
	harbol_linkmap_insert(tasm->Registers, "rgamal", (union HarbolValue){.UInt64 = regGamal});
	
	harbol_linkmap_insert(tasm->Registers, "RDALATH", (union HarbolValue){.UInt64 = regDalath});
	harbol_linkmap_insert(tasm->Registers, "rdalath", (union HarbolValue){.UInt64 = regDalath});
	
	harbol_linkmap_insert(tasm->Registers, "RHEH", (union HarbolValue){.UInt64 = regHeh});
	harbol_linkmap_insert(tasm->Registers, "rheh", (union HarbolValue){.UInt64 = regHeh});
	
	harbol_linkmap_insert(tasm->Registers, "RWAW", (union HarbolValue){.UInt64 = regWaw});
	harbol_linkmap_insert(tasm->Registers, "rwaw", (union HarbolValue){.UInt64 = regWaw});
	
	harbol_linkmap_insert(tasm->Registers, "RZAIN", (union HarbolValue){.UInt64 = regZain});
	harbol_linkmap_insert(tasm->Registers, "rzain", (union HarbolValue){.UInt64 = regZain});
	
	harbol_linkmap_insert(tasm->Registers, "RHETH", (union HarbolValue){.UInt64 = regHeth});
	harbol_linkmap_insert(tasm->Registers, "rheth", (union HarbolValue){.UInt64 = regHeth});
	
	harbol_linkmap_insert(tasm->Registers, "RTETH", (union HarbolValue){.UInt64 = regTeth});
	harbol_linkmap_insert(tasm->Registers, "rteth", (union HarbolValue){.UInt64 = regTeth});
	
	harbol_linkmap_insert(tasm->Registers, "RYODH", (union HarbolValue){.UInt64 = regYodh});
	harbol_linkmap_insert(tasm->Registers, "ryodh", (union HarbolValue){.UInt64 = regYodh});
	
	harbol_linkmap_insert(tasm->Registers, "RKAF", (union HarbolValue){.UInt64 = regKaf});
	harbol_linkmap_insert(tasm->Registers, "rkaf", (union HarbolValue){.UInt64 = regKaf});
	
	harbol_linkmap_insert(tasm->Registers, "RLAMADH", (union HarbolValue){.UInt64 = regLamadh});
	harbol_linkmap_insert(tasm->Registers, "rlamadh", (union HarbolValue){.UInt64 = regLamadh});
	
	harbol_linkmap_insert(tasm->Registers, "RMEEM", (union HarbolValue){.UInt64 = regMeem});
	harbol_linkmap_insert(tasm->Registers, "rmeem", (union HarbolValue){.UInt64 = regMeem});
	
	harbol_linkmap_insert(tasm->Registers, "RNOON", (union HarbolValue){.UInt64 = regNoon});
	harbol_linkmap_insert(tasm->Registers, "rnoon", (union HarbolValue){.UInt64 = regNoon});
	
	harbol_linkmap_insert(tasm->Registers, "RSEMKATH", (union HarbolValue){.UInt64 = regSemkath});
	harbol_linkmap_insert(tasm->Registers, "rsemkath", (union HarbolValue){.UInt64 = regSemkath});
	
	harbol_linkmap_insert(tasm->Registers, "R_EH", (union HarbolValue){.UInt64 = reg_Eh});
	harbol_linkmap_insert(tasm->Registers, "r_eh", (union HarbolValue){.UInt64 = reg_Eh});
	
	harbol_linkmap_insert(tasm->Registers, "RPEH", (union HarbolValue){.UInt64 = regPeh});
	harbol_linkmap_insert(tasm->Registers, "rpeh", (union HarbolValue){.UInt64 = regPeh});
	
	harbol_linkmap_insert(tasm->Registers, "RSADHE", (union HarbolValue){.UInt64 = regSadhe});
	harbol_linkmap_insert(tasm->Registers, "rsadhe", (union HarbolValue){.UInt64 = regSadhe});
	
	harbol_linkmap_insert(tasm->Registers, "RQOF", (union HarbolValue){.UInt64 = regQof});
	harbol_linkmap_insert(tasm->Registers, "rqof", (union HarbolValue){.UInt64 = regQof});
	
	harbol_linkmap_insert(tasm->Registers, "RREESH", (union HarbolValue){.UInt64 = regReesh});
	harbol_linkmap_insert(tasm->Registers, "rreesh", (union HarbolValue){.UInt64 = regReesh});
	
	harbol_linkmap_insert(tasm->Registers, "RSHEEN", (union HarbolValue){.UInt64 = regSheen});
	harbol_linkmap_insert(tasm->Registers, "rsheen", (union HarbolValue){.UInt64 = regSheen});
	
	harbol_linkmap_insert(tasm->Registers, "RTAW", (union HarbolValue){.UInt64 = regTaw});
	harbol_linkmap_insert(tasm->Registers, "rtaw", (union HarbolValue){.UInt64 = regTaw});
	
	harbol_linkmap_insert(tasm->Registers, "RSP", (union HarbolValue){.UInt64 = regStk});
	harbol_linkmap_insert(tasm->Registers, "rsp", (union HarbolValue){.UInt64 = regStk});
	
	harbol_linkmap_insert(tasm->Registers, "RBP", (union HarbolValue){.UInt64 = regBase});
	harbol_linkmap_insert(tasm->Registers, "rbp", (union HarbolValue){.UInt64 = regBase});
	/*
	harbol_linkmap_insert(tasm->Registers, "ܐܠܦ", (union HarbolValue){.UInt64 = regAlaf});
	harbol_linkmap_insert(tasm->Registers, "ܒܝܬ", (union HarbolValue){.UInt64 = regBeth});
	harbol_linkmap_insert(tasm->Registers, "ܓܡܠ", (union HarbolValue){.UInt64 = regGamal});
	harbol_linkmap_insert(tasm->Registers, "ܕܠܬ", (union HarbolValue){.UInt64 = regDalath});
	harbol_linkmap_insert(tasm->Registers, "ܗܐ", (union HarbolValue){.UInt64 = regHeh});
	harbol_linkmap_insert(tasm->Registers, "ܘܘ", (union HarbolValue){.UInt64 = regWaw});
	harbol_linkmap_insert(tasm->Registers, "ܙܝܢ", (union HarbolValue){.UInt64 = regZain});
	harbol_linkmap_insert(tasm->Registers, "ܚܝܬ", (union HarbolValue){.UInt64 = regHeth});
	harbol_linkmap_insert(tasm->Registers, "ܛܝܬ", (union HarbolValue){.UInt64 = regTeth});
	harbol_linkmap_insert(tasm->Registers, "ܝܘܕ", (union HarbolValue){.UInt64 = regYodh});
	harbol_linkmap_insert(tasm->Registers, "ܟܦ", (union HarbolValue){.UInt64 = regKaf});
	harbol_linkmap_insert(tasm->Registers, "ܠܡܕ", (union HarbolValue){.UInt64 = regLamadh});
	harbol_linkmap_insert(tasm->Registers, "ܡܝܡ", (union HarbolValue){.UInt64 = regMeem});
	harbol_linkmap_insert(tasm->Registers, "ܢܘܢ", (union HarbolValue){.UInt64 = regNoon});
	harbol_linkmap_insert(tasm->Registers, "ܣܡܟܬ", (union HarbolValue){.UInt64 = regSemkath});
	harbol_linkmap_insert(tasm->Registers, "ܥܐ", (union HarbolValue){.UInt64 = reg_Eh});
	harbol_linkmap_insert(tasm->Registers, "ܦܐ", (union HarbolValue){.UInt64 = regPeh});
	harbol_linkmap_insert(tasm->Registers, "ܨܕܐ", (union HarbolValue){.UInt64 = regSadhe});
	harbol_linkmap_insert(tasm->Registers, "ܩܘܦ", (union HarbolValue){.UInt64 = regQof});
	harbol_linkmap_insert(tasm->Registers, "ܪܝܫ", (union HarbolValue){.UInt64 = regReesh});
	harbol_linkmap_insert(tasm->Registers, "ܫܝܢ", (union HarbolValue){.UInt64 = regSheen});
	harbol_linkmap_insert(tasm->Registers, "ܬܘ", (union HarbolValue){.UInt64 = regTaw});
	harbol_linkmap_insert(tasm->Registers, "ܪܝܫ_ܟܫܐ", (union HarbolValue){.UInt64 = regStk});
	harbol_linkmap_insert(tasm->Registers, "ܐܫܬ_ܟܫܐ", (union HarbolValue){.UInt64 = regBase});
	*/
	// set up our instruction set!
	#define X(x) harbol_linkmap_insert(tasm->Opcodes, #x, (union HarbolValue){.UInt64 = x});
		TAGHA_INSTR_SET;
	#undef X
	// add additional for specific opcodes.
	harbol_linkmap_insert(tasm->Opcodes, "and", (union HarbolValue){.UInt64 = bit_and});
	harbol_linkmap_insert(tasm->Opcodes, "or", (union HarbolValue){.UInt64 = bit_or});
	harbol_linkmap_insert(tasm->Opcodes, "xor", (union HarbolValue){.UInt64 = bit_xor});
	harbol_linkmap_insert(tasm->Opcodes, "not", (union HarbolValue){.UInt64 = bit_not});
	harbol_linkmap_insert(tasm->Opcodes, "loadvar", (union HarbolValue){.UInt64 = loadglobal});
	/*
	harbol_linkmap_insert(tasm->Opcodes, "ܟܠܐ", (union HarbolValue){.UInt64 = halt});
	harbol_linkmap_insert(tasm->Opcodes, "ܨܘܝ.ܡ", (union HarbolValue){.UInt64 = pushi});
	*/
	
	/* FIRST PASS. Collect labels + their PC relative addresses */
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
			if( tasm->Error )
				break;
			harbol_string_del(tasm->Lexeme);
			// skip whitespace.
			skip_whitespace(&tasm->Iter);
			
			// skip to next line if comment or directive.
			if( *tasm->Iter=='\n' || *tasm->Iter==';' || *tasm->Iter=='#' )
				break;
			
			else if( *tasm->Iter=='}' ) {
				tasm->Iter++;
				harbol_string_del(tasm->ActiveFuncLabel);
				tasm->ActiveFuncLabel = NULL;
				tasm->ProgramCounter = 0;
				break;
			}
			// parse the directives!
			else if( *tasm->Iter=='$' ) {
				lex_identifier(&tasm->Iter, tasm->Lexeme);
				if( !harbol_string_ncmpcstr(tasm->Lexeme, "$stacksize", sizeof "$stacksize") ) {
					tagha_asm_parse_stack_directive(tasm);
				#ifdef TASM_DEBUG
					printf("tasm: Stack size set to: %u\n", tasm->Stacksize);
				#endif
				}
				else if( !harbol_string_ncmpcstr(tasm->Lexeme, "$global", sizeof "$global") )
					tagha_asm_parse_globalvar_directive(tasm);
				else if( !harbol_string_ncmpcstr(tasm->Lexeme, "$native", sizeof "$native") )
					tagha_asm_parse_native_directive(tasm);
				else if( !harbol_string_ncmpcstr(tasm->Lexeme, "$safemode", sizeof "$safemode") || !harbol_string_ncmpcstr(tasm->Lexeme, "$safe", sizeof "$safe")  )
					tasm->Safemode = 1;
				break;
			}
			
			// holy cannoli, we found a label!
			else if( *tasm->Iter=='.' || *tasm->Iter=='%' ) {
				const bool funclbl = *tasm->Iter == '%';
				// the dot || percent is added to our lexeme
				lex_identifier(&tasm->Iter, tasm->Lexeme);
				skip_whitespace(&tasm->Iter);
				if( *tasm->Iter == ':' )
					tasm->Iter++;
				
				skip_whitespace(&tasm->Iter);
				if( funclbl ) {
					if( *tasm->Iter != '{' ) {
						tagha_asm_err_out(tasm, "missing curly '{' bracket! Curly bracket must be on the same line as label.");
						continue;
					}
					else tasm->Iter++;
				}
				
				if( !is_alphabetic(tasm->Lexeme->CStr[1]) ) {
					tagha_asm_err_out(tasm, "%s labels must have alphabetic names!", funclbl ? "function" : "jump");
					continue;
				} else if( harbol_linkmap_has_key(funclbl ? tasm->FuncTable : tasm->LabelTable, tasm->Lexeme->CStr) ) {
					tagha_asm_err_out(tasm, "redefinition of label '%s'.", tasm->Lexeme->CStr);
					continue;
				}
				
				struct LabelInfo *label = calloc(1, sizeof *label);
				if( !label ) {
					tagha_asm_err_out(tasm, "out of memory trying to allocate a new label!");
					continue;
				}
				if( funclbl ) {
					tasm->ActiveFuncLabel = &(struct HarbolString){0};
					harbol_string_copy_str(tasm->ActiveFuncLabel, tasm->Lexeme);
				} else {
					if( !tasm->ActiveFuncLabel ) {
						tagha_asm_err_out(tasm, "jump label outside of function block!");
						break;
					} else {
						harbol_string_add_str(tasm->Lexeme, tasm->ActiveFuncLabel);
					}
				}
				label->Addr = tasm->ProgramCounter;
				label->IsFunc = funclbl;
#ifdef TASM_DEBUG
				printf("%s Label '%s' is located at address: %zu\n", funclbl ? "Func" : "Local", tasm->Lexeme->CStr, tasm->ProgramCounter);
#endif
				harbol_linkmap_insert(funclbl ? tasm->FuncTable : tasm->LabelTable, tasm->Lexeme->CStr, (union HarbolValue){.Ptr = label});
			}
			// it's an opcode!
			else if( is_alphabetic(*tasm->Iter) ) {
				if( !tasm->ActiveFuncLabel ) {
					tagha_asm_err_out(tasm, "opcode outside of function block!");
					continue;
				}
				lex_identifier(&tasm->Iter, tasm->Lexeme);
				if( !harbol_linkmap_has_key(tasm->Opcodes, tasm->Lexeme->CStr) ) {
					tagha_asm_err_out(tasm, "unknown opcode '%s'!", tasm->Lexeme->CStr);
					continue;
				}
				uint8_t opcode = harbol_linkmap_get(tasm->Opcodes, tasm->Lexeme->CStr).UInt64;
				switch( opcode ) {
					// opcodes that take no args
					case halt: case ret: case nop:
						tasm->ProgramCounter++; break;
					
					// opcodes that only take an imm operand.
					case pushi: case jmp: case jz: case jnz:
					case call: //case syscall:
						tagha_asm_parse_OneImmInstr(tasm, true); break;
					
					// opcodes that only take a register operand.
					case push: case pop: case bit_not:
					case inc: case dec: case neg:
					case callr: //case syscallr:
				#ifdef TAGHA_FLOATING_POINT_OPS
					case flt2dbl: case dbl2flt: case int2dbl: case int2flt:
					case incf: case decf: case negf:
				#endif
						tagha_asm_parse_OneRegInstr(tasm, true); break;
					
					// opcodes reg<-imm
					case loadglobal: case loadfunc: case movi:
						tagha_asm_parse_RegImmInstr(tasm, true); break;
					
					// opcodes reg<-mem (load)
					case loadaddr:
					case ld1: case ld2: case ld4: case ld8:
						tagha_asm_parse_RegMemInstr(tasm, true); break;
					
					// opcodes mem<-reg (store)
					case st1: case st2: case st4: case st8:
						tagha_asm_parse_MemRegInstr(tasm, true); break;
					
					// opcodes reg<-reg
					case mov:
					case add: case sub: case mul: case divi: case mod:
					case bit_and: case bit_or: case bit_xor: case shl: case shr:
					case ilt: case ile: case igt: case ige:
					case ult: case ule: case ugt: case uge:
					case cmp: case neq:
				#ifdef TAGHA_FLOATING_POINT_OPS
					case addf: case subf: case mulf: case divf:
					case ltf: case lef: case gtf: case gef: case cmpf: case neqf:
				#endif
						tagha_asm_parse_RegRegInstr(tasm, true); break;
				}
			}
		}
	}
	if( tasm->Error )
		goto assembling_err_exit;
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
			if( tasm->Error )
				break;
			harbol_string_del(tasm->Lexeme);
			// skip whitespace.
			skip_whitespace(&tasm->Iter);
			
			// skip to next line if comment or directive.
			if( *tasm->Iter=='\n' || *tasm->Iter==';' || *tasm->Iter=='#' || *tasm->Iter=='$' )
				break;
			else if( *tasm->Iter=='}' ) {
				tasm->Iter++;
				harbol_string_del(tasm->ActiveFuncLabel);
				tasm->ActiveFuncLabel = NULL;
				tasm->ProgramCounter = 0;
				break;
			}
			// skip labels in second pass.
			else if( *tasm->Iter=='.' || *tasm->Iter=='%' ) {
				const bool funclbl = *tasm->Iter == '%';
				lex_identifier(&tasm->Iter, tasm->Lexeme);
				tasm->Iter++;
				skip_whitespace(&tasm->Iter);
				if( funclbl ) {
					tasm->Iter++;
					tasm->ActiveFuncLabel = &(struct HarbolString){0};
					harbol_string_copy_str(tasm->ActiveFuncLabel, tasm->Lexeme);
					skip_whitespace(&tasm->Iter);
				}
			}
			// parse opcode!
			if( is_alphabetic(*tasm->Iter) ) {
				lex_identifier(&tasm->Iter, tasm->Lexeme);
				if( !harbol_linkmap_has_key(tasm->Opcodes, tasm->Lexeme->CStr) ) {
					tagha_asm_err_out(tasm, "unknown opcode '%s'!", tasm->Lexeme->CStr);
					continue;
				}
				uint8_t opcode = harbol_linkmap_get(tasm->Opcodes, tasm->Lexeme->CStr).UInt64;
				struct LabelInfo *label = harbol_linkmap_get(tasm->FuncTable, tasm->ActiveFuncLabel->CStr).Ptr;
				harbol_bytebuffer_insert_byte(&label->Bytecode, opcode);
				switch( opcode ) {
					// opcodes that take no args
					case halt: case ret: case nop:
						break;
					
					// opcodes that only take an imm operand.
					case pushi: case jmp: case jz: case jnz:
					case call: //case syscall:
						tagha_asm_parse_OneImmInstr(tasm, false); break;
					
					// opcodes that only take a register operand.
					case push: case pop: case bit_not:
					case inc: case dec: case neg:
					case callr: //case syscallr:
				#ifdef TAGHA_FLOATING_POINT_OPS
					case flt2dbl: case dbl2flt: case int2dbl: case int2flt:
					case incf: case decf: case negf:
				#endif
						tagha_asm_parse_OneRegInstr(tasm, false); break;
					
					// opcodes reg<-imm
					case loadglobal: case loadfunc: case movi:
						tagha_asm_parse_RegImmInstr(tasm, false); break;
					
					// opcodes reg<-mem (load)
					case loadaddr:
					case ld1: case ld2: case ld4: case ld8:
						tagha_asm_parse_RegMemInstr(tasm, false); break;
					
					// opcodes mem<-reg (store)
					case st1: case st2: case st4: case st8:
						tagha_asm_parse_MemRegInstr(tasm, false); break;
					
					// opcodes reg<-reg
					case mov:
					case add: case sub: case mul: case divi: case mod:
					case bit_and: case bit_or: case bit_xor: case shl: case shr:
					case ilt: case ile: case igt: case ige:
					case ult: case ule: case ugt: case uge:
					case cmp: case neq:
				#ifdef TAGHA_FLOATING_POINT_OPS
					case addf: case subf: case mulf: case divf:
					case ltf: case lef: case gtf: case gef: case cmpf: case neqf:
				#endif
						tagha_asm_parse_RegRegInstr(tasm, false); break;
				}
			}
		}
	}
	
	if( tasm->Error )
		goto assembling_err_exit;
#ifdef TASM_DEBUG
	puts("\ntasm: SECOND PASS End!\n");
#endif
	if( !tasm->Stacksize )
		tasm->Stacksize = 128;
	
	// build our func table & global var table
	struct HarbolByteBuffer functable; harbol_bytebuffer_init(&functable);
	struct HarbolByteBuffer datatable; harbol_bytebuffer_init(&datatable);
	
	for( size_t i=0 ; i<tasm->FuncTable->Map.Count ; i++ ) {
		struct HarbolKeyValPair *node = harbol_linkmap_get_node_by_index(tasm->FuncTable, i);
		struct LabelInfo *label = node->Data.Ptr;
		if( !label )
			continue;
		
		// write flag
		harbol_bytebuffer_insert_byte(&functable, label->IsNativeFunc);
		// write strlen
		harbol_bytebuffer_insert_integer(&functable, node->KeyName.Len, sizeof(uint32_t));
		
		// write HarbolString
		label->IsNativeFunc ? 
			harbol_bytebuffer_insert_integer(&functable, 8, sizeof(uint32_t))
				: harbol_bytebuffer_insert_integer(&functable, label->Bytecode.Count, sizeof(uint32_t));
		
		// write instrlen.
		harbol_bytebuffer_insert_cstr(&functable, node->KeyName.CStr+1, node->KeyName.Len-1);
		label->IsNativeFunc ?
			harbol_bytebuffer_insert_integer(&functable, 0, sizeof(uint64_t))
				: harbol_bytebuffer_append(&functable, &label->Bytecode) ;
	#ifdef TASM_DEBUG
		printf("func label: %s\nData:\n", node->KeyName.CStr);
		for( size_t i=0 ; i<label->Bytecode.Count ; i++ )
			printf("bytecode[%zu] == %u\n", i, label->Bytecode.Buffer[i]);
		puts("\n");
	#endif
	}
	
	for( size_t i=0 ; i<tasm->VarTable->Map.Count ; i++ ) {
		struct HarbolKeyValPair *node = harbol_linkmap_get_node_by_index(tasm->VarTable, i);
		struct HarbolByteBuffer *bytedata = node->Data.Ptr;
		if( !bytedata )
			continue;
		
		// write flag.
		harbol_bytebuffer_insert_byte(&datatable, 0);
		// write strlen.
		harbol_bytebuffer_insert_integer(&datatable, node->KeyName.Len+1, sizeof(uint32_t));
		// write byte count.
		harbol_bytebuffer_insert_integer(&datatable, bytedata->Count, sizeof(uint32_t));
		// write HarbolString.
		harbol_bytebuffer_insert_cstr(&datatable, node->KeyName.CStr, node->KeyName.Len);
		// write byte data.
		harbol_bytebuffer_append(&datatable, bytedata);
	#ifdef TASM_DEBUG
		printf("global var: %s\nData:\n", node->KeyName.CStr);
		for( size_t i=0 ; i<bytedata->Count ; i++ )
			printf("bytecode[%zu] == %u\n", i, bytedata->Buffer[i]);
		puts("\n");
	#endif
	}
	
	// build the header table.
	struct HarbolByteBuffer tbcfile; harbol_bytebuffer_init(&tbcfile);
	harbol_bytebuffer_insert_integer(&tbcfile, 0xC0DE, sizeof(uint16_t));
	harbol_bytebuffer_insert_integer(&tbcfile, tasm->Stacksize, sizeof tasm->Stacksize);
	
	// write flags, currently none.
	harbol_bytebuffer_insert_byte(&tbcfile, tasm->Safemode | 0);
	
	// now build function table.
	harbol_bytebuffer_insert_integer(&tbcfile, tasm->FuncTable->Map.Count, sizeof(uint32_t));
	harbol_bytebuffer_append(&tbcfile, &functable);
	harbol_bytebuffer_del(&functable);
	
	// now build global variable table.
	harbol_bytebuffer_insert_integer(&tbcfile, tasm->VarTable->Map.Count, sizeof(uint32_t));
	harbol_bytebuffer_append(&tbcfile, &datatable);
	harbol_bytebuffer_del(&datatable);
	
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
		tagha_asm_err_out(tasm, "unable to create output file!");
		goto assembling_err_exit;
	}
	//for( size_t n=0 ; n<(tasm->Stacksize*8) ; n++ )
	//	harbol_bytebuffer_insert_byte(&tbcfile, 0);
	harbol_bytebuffer_to_file(&tbcfile, tbcscript);
	fclose(tbcscript); tbcscript=NULL;
	harbol_bytebuffer_del(&tbcfile);

assembling_err_exit:
	harbol_string_del(&tasm->OutputName);
	harbol_string_del(tasm->Lexeme);
	harbol_string_del(tasm->ActiveFuncLabel);
	
	harbol_linkmap_del(tasm->LabelTable, label_free);
	harbol_linkmap_del(tasm->FuncTable, label_free);
	harbol_linkmap_del(tasm->VarTable, (fnDestructor *)harbol_bytebuffer_free);
	harbol_linkmap_del(tasm->Opcodes, NULL);
	harbol_linkmap_del(tasm->Registers, NULL);
	fclose(tasm->Src); tasm->Src=NULL;
	memset(tasm, 0, sizeof *tasm);
	return true;
}

bool label_free(void *p)
{
	struct LabelInfo **restrict labelref = p;
	if( !labelref || !*labelref )
		return false;
	
	harbol_bytebuffer_del(&(*labelref)->Bytecode);
	free(*labelref), *labelref=NULL;
	return true;
}

static size_t get_filesize(FILE *const restrict file)
{
	long size = 0L;
	if( !file )
		return size;
	
	if( !fseek(file, 0, SEEK_END) ) {
		size = ftell(file);
		if( size<=-1 )
			return 0L;
		rewind(file);
	}
	return (size_t)size;
}

int main(int argc, char *argv[restrict static argc+1])
{
	if( argc<=1 )
		return -1;
	else if( !strcmp(argv[1], "--help") ) {
		puts("Tagha Assembler. Part of the Tagha Runtime Environment Toolkit\nTo compile a tasm script to tbc, supply a script name as a command-line argument to the program.\nExample: './tagha_asm script.tasm'");
	}
	else if( !strcmp(argv[1], "--version") ) {
		puts("Tagha Assembler Version 1.0.2");
	}
	else {
		for( int i=1 ; i<argc ; i++ ) {
			FILE *tasmfile = fopen(argv[i], "r");
			if( !tasmfile )
				continue;
			
			struct TaghaAsmbler *const restrict tasm = &(struct TaghaAsmbler){0};
			tasm->Src = tasmfile;
			tasm->SrcSize = get_filesize(tasmfile);
			harbol_string_init_cstr(&tasm->OutputName, argv[i]);
			tagha_asm_assemble(tasm);
		}
	}
}

