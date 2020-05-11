#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "libharbol/harbol.h"
#include "../tagha/tagha.h"
#include "tagha_script_builder.h"

//#define TASM_DEBUG

enum {
	DEFAULT_STACK_SIZE = 0x1000, /// 4kb
	DEFAULT_HEAP_SIZE  = 0x1000
};

struct Label {
	struct HarbolByteBuf bytecode;
	uint64_t addr;
	bool
		is_func : 1,
		is_native : 1,
		is_extern : 1
	;
};

void label_free(void **);

struct TaghaAssembler {
	struct HarbolLinkMap
		labelmap, funcmap, varmap,
		opcodes, regs
	;
	struct HarbolString outname, *active_func_label, *lexeme;
	FILE *src;
	const char *iter;
	size_t srcsize, prog_counter, currline;
	uint32_t stacksize, heapsize;
	bool errored : 1;
};

#define EMPTY_TAGHA_ASSEMBLER    { EMPTY_HARBOL_LINKMAP, EMPTY_HARBOL_LINKMAP, EMPTY_HARBOL_LINKMAP, EMPTY_HARBOL_LINKMAP, EMPTY_HARBOL_LINKMAP, EMPTY_HARBOL_STRING, NULL,NULL,NULL,NULL, 0,0,0,0,0, false }


typedef void TASMParseFunc(struct TaghaAssembler *, bool);
TASMParseFunc
	tagha_asm_parse_reg_reg,
	tagha_asm_parse_reg,
	tagha_asm_parse_imm,
	tagha_asm_parse_reg_mem,
	tagha_asm_parse_mem_reg,
	tagha_asm_parse_reg_imm
;


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


static NO_NULL NONNULL_RET const char *lex_identifier(const char str[restrict static 1], struct HarbolString *const restrict strobj)
{
	if( *str==0 )
		return str;
	else {
		harbol_string_clear(strobj);
		harbol_string_add_char(strobj, *str++);
		while( *str != 0 && is_possible_ident(*str) )
			harbol_string_add_char(strobj, *str++);
		return str;
	}
}

static NO_NULL NONNULL_RET const char *lex_number(const char str[restrict static 1], struct HarbolString *const restrict strobj)
{
	if( *str==0 )
		return str;
	else {
		harbol_string_clear(strobj);
		if( *str=='0' ) {
			if( str[1]=='x' || str[1]=='X' ) { /// hexadecimal.
				harbol_string_add_cstr(strobj, str[1]=='x' ? "0x" : "0X");
				str += 2;
				while( *str != 0 && is_hex(*str) )
					harbol_string_add_char(strobj, *str++);
			} else if( str[1]=='b' || str[1]=='B' ) { /// binary.
				harbol_string_add_cstr(strobj, str[1]=='b' ? "0b" : "0B");
				str += 2;
				while( *str != 0 && (*str=='1' || *str=='0') )
					harbol_string_add_char(strobj, *str++);
			} else { /// octal.
				harbol_string_add_char(strobj, *str++);
				while( *str != 0 && is_octal(*str) )
					harbol_string_add_char(strobj, *str++);
			}
		} else {
			while( *str != 0 && is_decimal(*str) )
				harbol_string_add_char(strobj, *str++);
		}
		/*
		if( *str=='.' ) { // add float32_t support.
			harbol_string_add_char(strobj, *str++);
			while( *str != 0 && is_decimal(*str) )
				harbol_string_add_char(strobj, *str++);
		}
		*/
		return str;
	}
}

static NO_NULL void string_to_int(struct HarbolString *const restrict strobj, const size_t bytes, void *const restrict p, const bool usign)
{
	const bool is_binary = !harbol_string_cmpcstr(strobj, "0b") || !harbol_string_cmpcstr(strobj, "0B") ? true : false;
	switch( bytes ) {
		case 4: {
			if( usign ) {
				uint32_t *const restrict i = p;
				*i = ( uint32_t )strtoul(is_binary ? strobj->cstr+2 : strobj->cstr, NULL, is_binary ? 2 : 0);
			} else {
				int32_t *const restrict i = p;
				*i = ( int32_t )strtol(is_binary ? strobj->cstr+2 : strobj->cstr, NULL, is_binary ? 2 : 0);
			}
			break;
		}
		case 8: {
			if( usign ) {
				uint64_t *const restrict i = p;
				*i = strtoull(is_binary ? strobj->cstr+2 : strobj->cstr, NULL, is_binary ? 2 : 0);
			} else {
				int64_t *const restrict i = p;
				*i = strtoll(is_binary ? strobj->cstr+2 : strobj->cstr, NULL, is_binary ? 2 : 0);
			}
		}
	}
}

static NO_NULL void tagha_asm_err_out(struct TaghaAssembler *const restrict tasm, const char err[restrict static 1], ...)
{
	va_list args;
	va_start(args, err);
	printf("%s: **** ", tasm->outname.cstr);
	vprintf(err, args);
	printf(" **** | L:%zu\n", tasm->currline);
	va_end(args);
	tasm->errored = true;
}

static NO_NULL void write_utf8(struct TaghaAssembler *const tasm, const int32_t chr)
{
	const uint32_t rune = (uint32_t)chr;
	if (rune < 0x80) {
		harbol_string_add_char(tasm->lexeme, rune);
		return;
	}
	if (rune < 0x800) {
		harbol_string_add_char(tasm->lexeme, 0xC0 | (rune >> 6));
		harbol_string_add_char(tasm->lexeme, 0x80 | (rune & 0x3F));
		return;
	}
	if (rune < 0x10000) {
		harbol_string_add_char(tasm->lexeme, 0xE0 | (rune >> 12));
		harbol_string_add_char(tasm->lexeme, 0x80 | ((rune >> 6) & 0x3F));
		harbol_string_add_char(tasm->lexeme, 0x80 | (rune & 0x3F));
		return;
	}
	if (rune < 0x200000) {
		harbol_string_add_char(tasm->lexeme, 0xF0 | (rune >> 18));
		harbol_string_add_char(tasm->lexeme, 0x80 | ((rune >> 12) & 0x3F));
		harbol_string_add_char(tasm->lexeme, 0x80 | ((rune >> 6) & 0x3F));
		harbol_string_add_char(tasm->lexeme, 0x80 | (rune & 0x3F));
		return;
	}
	tagha_asm_err_out(tasm, "invalid unicode character: \\U%08x", rune);
}

/// $stacksize <number>
NO_NULL void tagha_asm_parse_stack_directive(struct TaghaAssembler *const tasm)
{
	if( tasm->src==NULL || tasm->iter==NULL )
		return;
	
	tasm->iter = skip_whitespace(tasm->iter);
	if( !is_decimal(*tasm->iter) ) {
		tagha_asm_err_out(tasm, "stack size directive requires a valid number!");
		return;
	}
	
	tasm->iter = lex_number(tasm->iter, tasm->lexeme);
	string_to_int(tasm->lexeme, sizeof tasm->stacksize, &tasm->stacksize, true);
}

/// $heapsize <number>
NO_NULL void tagha_asm_parse_heap_directive(struct TaghaAssembler *const tasm)
{
	if( tasm->src==NULL || tasm->iter==NULL )
		return;
	
	tasm->iter = skip_whitespace(tasm->iter);
	if( !is_decimal(*tasm->iter) ) {
		tagha_asm_err_out(tasm, "heap size directive requires a valid number!");
		return;
	}
	
	tasm->iter = lex_number(tasm->iter, tasm->lexeme);
	string_to_int(tasm->lexeme, sizeof tasm->heapsize, &tasm->heapsize, true);
}

/// $global varname bytes ...
NO_NULL void tagha_asm_parse_globalvar_directive(struct TaghaAssembler *const tasm)
{
	if( tasm->src==NULL || tasm->iter==NULL )
		return;
	
	tasm->iter = skip_whitespace(tasm->iter);
	if( !is_alphabetic(*tasm->iter) ) {
		tagha_asm_err_out(tasm, "global directive requires the 1st argument to be a variable name!");
		return;
	}
	
	struct HarbolString varname = harbol_string_create("");
	tasm->iter = lex_identifier(tasm->iter, &varname);
	tasm->iter = skip_whitespace(tasm->iter);
	if( *tasm->iter==',' )
		tasm->iter++;
	tasm->iter = skip_whitespace(tasm->iter);
	
	if( !is_decimal(*tasm->iter) ) {
		tagha_asm_err_out(tasm, "missing byte size number in global directive");
		return;
	}
	
	tasm->iter = lex_number(tasm->iter, tasm->lexeme);
	size_t bytes = 0;
	string_to_int(tasm->lexeme, sizeof bytes, &bytes, true);
	
	struct HarbolByteBuf vardata = harbol_bytebuffer_create();
	tasm->iter = skip_whitespace(tasm->iter);
	if( *tasm->iter==',' )
		tasm->iter++;
	
	while( bytes != 0 ) {
		tasm->iter = skip_whitespace(tasm->iter);
		if( *tasm->iter=='"' ) { /// string
			harbol_string_clear(tasm->lexeme);
			const char quote = *tasm->iter++;
			while( *tasm->iter && *tasm->iter != quote ) {
				const char charval = *tasm->iter++;
				if( !charval ) { /// sudden EOF?
					tagha_asm_err_out(tasm, "sudden EOF while reading global directive string!");
					return;
				}
				/// handle escape chars
				if( charval=='\\' ) {
					const char escape = *tasm->iter++;
					switch( escape ) {
						case 'a': harbol_string_add_char(tasm->lexeme, '\a'); break;
						case 'b': harbol_string_add_char(tasm->lexeme, '\b'); break;
						case 'n': harbol_string_add_char(tasm->lexeme, '\n'); break;
						case 'r': harbol_string_add_char(tasm->lexeme, '\r'); break;
						case 't': harbol_string_add_char(tasm->lexeme, '\t'); break;
						case 'v': harbol_string_add_char(tasm->lexeme, '\v'); break;
						case 'f': harbol_string_add_char(tasm->lexeme, '\f'); break;
						case '0': harbol_string_add_char(tasm->lexeme, '\0'); break;
						case 'U': {
							int32_t r = 0;
							const size_t encoding = 4;
							for( size_t i=0; i<encoding*2; i++ ) {
								const int32_t c = *tasm->iter++;
								switch( c ) {
									case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
										r = (r << 4) | (c - '0'); break;
									case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
										r = (r << 4) | (c - 'a' + 10); break;
									case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
										r = (r << 4) | (c - 'A' + 10); break;
									default:
										tagha_asm_err_out(tasm, "invalid unicode character: '%c'", c);
										return;
								}
							}
							if( !is_valid_ucn(r) ) {
								tagha_asm_err_out(tasm, "invalid universal character: '\\U%0*x'", encoding, r);
								return;
							}
							else write_utf8(tasm, r);
							break;
						}
						case 'u': {
							int32_t r = 0;
							const size_t encoding = 2;
							for( size_t i=0; i<encoding*2; i++ ) {
								const int32_t c = *tasm->iter++;
								switch( c ) {
									case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
										r = (r << 4) | (c - '0'); break;
									case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
										r = (r << 4) | (c - 'a' + 10); break;
									case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
										r = (r << 4) | (c - 'A' + 10); break;
									default:
										tagha_asm_err_out(tasm, "invalid unicode character: '%c'", c);
										return;
								}
							}
							if( !is_valid_ucn(r) ) {
								tagha_asm_err_out(tasm, "invalid universal character: '\\u%0*x'", encoding, r);
								return;
							}
							else write_utf8(tasm, r);
							break;
						}
						default: harbol_string_add_char(tasm->lexeme, escape);
					}
				}
				else harbol_string_add_char(tasm->lexeme, charval);
			}
#ifdef TASM_DEBUG
			printf("tasm: global string '%s'\n", tasm->lexeme->cstr);
#endif
			harbol_bytebuffer_insert_cstr(&vardata, tasm->lexeme->cstr);
			bytes = 0;
		} else if( *tasm->iter==',' )
			tasm->iter++;
		else if( is_decimal(*tasm->iter) ) {
			tasm->iter = lex_number(tasm->iter, tasm->lexeme);
			uint64_t data = 0;
			string_to_int(tasm->lexeme, sizeof data, &data, true);
			
			if( data ) {
				tagha_asm_err_out(tasm, "single numeric arguments for global vars must be 0!");
				return;
			}
			while( bytes-- )
				harbol_bytebuffer_insert_byte(&vardata, 0);
			break;
		} else if( is_alphabetic(*tasm->iter) ) {
			tasm->iter = lex_identifier(tasm->iter, tasm->lexeme);
			if( !harbol_string_cmpcstr(tasm->lexeme, "byte") ) {
				tasm->iter = skip_whitespace(tasm->iter);
				tasm->iter = lex_number(tasm->iter, tasm->lexeme);
				uint32_t data = 0;
				string_to_int(tasm->lexeme, sizeof data, &data, true);
				harbol_bytebuffer_insert_byte(&vardata, ( uint8_t )data);
				bytes--;
			} else if( !harbol_string_cmpcstr(tasm->lexeme, "half") ) {
				tasm->iter = skip_whitespace(tasm->iter);
				tasm->iter = lex_number(tasm->iter, tasm->lexeme);
				uint32_t data = 0;
				string_to_int(tasm->lexeme, sizeof data, &data, true);
				harbol_bytebuffer_insert_int16(&vardata, ( uint16_t )data);
				bytes -= sizeof(uint16_t);
			} else if( !harbol_string_cmpcstr(tasm->lexeme, "long") ) {
				tasm->iter = skip_whitespace(tasm->iter);
				tasm->iter = lex_number(tasm->iter, tasm->lexeme);
				uint32_t data = 0;
				string_to_int(tasm->lexeme, sizeof data, &data, true);
				harbol_bytebuffer_insert_int32(&vardata, data);
				bytes -= sizeof(uint32_t);
			} else if( !harbol_string_cmpcstr(tasm->lexeme, "word") ) {
				tasm->iter = skip_whitespace(tasm->iter);
				tasm->iter = lex_number(tasm->iter, tasm->lexeme);
				uint64_t data = 0;
				string_to_int(tasm->lexeme, sizeof data, &data, true);
				harbol_bytebuffer_insert_int64(&vardata, data);
				bytes -= sizeof(uint64_t);
			}
		} else {
			tagha_asm_err_out(tasm, "global var directive data set is incomplete, must be equal to bytesize given.");
			return;
		}
	}
#ifdef TASM_DEBUG
	printf("tasm debug: vardata->count: %zu\n", vardata.count);
#endif
	harbol_linkmap_insert(&tasm->varmap, varname.cstr, &vardata);
	harbol_string_clear(&varname);
}

/// $native %name
NO_NULL void tagha_asm_parse_native_directive(struct TaghaAssembler *const tasm)
{
	if( tasm->src==NULL || tasm->iter==NULL )
		return;
	
	tasm->iter = skip_whitespace(tasm->iter);
	if( *tasm->iter != '%' ) {
		tagha_asm_err_out(tasm, "missing '%%' for native name declaration!");
		return;
	}
	tasm->iter = lex_identifier(tasm->iter, tasm->lexeme);
	
	struct Label label = { EMPTY_HARBOL_BYTEBUF, 0, true, true, false };
	harbol_linkmap_insert(&tasm->funcmap, tasm->lexeme->cstr, &label);
#ifdef TASM_DEBUG
	printf("tasm: added native function '%s'\n", tasm->lexeme->cstr);
#endif
}

/// $extern %module@name
NO_NULL void tagha_asm_parse_extern_directive(struct TaghaAssembler *const tasm)
{
	if( tasm->src==NULL || tasm->iter==NULL )
		return;
	
	tasm->iter = skip_whitespace(tasm->iter);
	if( *tasm->iter != '%' ) {
		tagha_asm_err_out(tasm, "missing '%%' for extern name declaration!");
		return;
	}
	tasm->iter = lex_identifier(tasm->iter, tasm->lexeme);
	if( strchr(tasm->lexeme->cstr, '@')==NULL ) {
		tagha_asm_err_out(tasm, "missing '@' for extern name declaration! (format: 'module@func')");
		return;
	}
	
	struct Label label = { EMPTY_HARBOL_BYTEBUF, 0, true, false, true };
	harbol_linkmap_insert(&tasm->funcmap, tasm->lexeme->cstr, &label);
#ifdef TASM_DEBUG
	printf("tasm: added extern function '%s'\n", tasm->lexeme->cstr);
#endif
}

NO_NULL int64_t lex_imm_value(struct TaghaAssembler *const tasm)
{
	tasm->iter = lex_number(tasm->iter, tasm->lexeme);
	tasm->prog_counter += 8;
	int64_t imm = 0;
	string_to_int(tasm->lexeme, sizeof imm, &imm, true);
	return imm;
}

NO_NULL uint8_t lex_reg_id(struct TaghaAssembler *const tasm)
{
	tasm->iter = lex_identifier(tasm->iter, tasm->lexeme);
	if( !harbol_linkmap_has_key(&tasm->regs, tasm->lexeme->cstr) ) {
		tagha_asm_err_out(tasm, "invalid register name '%s'", tasm->lexeme->cstr);
		return 0;
	}
	tasm->prog_counter++;
	return *( uint8_t* )harbol_linkmap_key_get(&tasm->regs, tasm->lexeme->cstr);
}

NO_NULL void lex_register_deref(struct TaghaAssembler *const restrict tasm, uint8_t *const restrict idref, int32_t *const restrict offsetref)
{
	tasm->iter++; /// iterate past '['
	tasm->prog_counter += 5; /// 1 for byte as register id + 4 byte offset.
	tasm->iter = skip_whitespace(tasm->iter);
	tasm->iter = lex_identifier(tasm->iter, tasm->lexeme);
	if( !harbol_linkmap_has_key(&tasm->regs, tasm->lexeme->cstr) ) {
		tagha_asm_err_out(tasm, "invalid register name '%s' in register indirection", tasm->lexeme->cstr);
		return;
	}
	*idref = *( uint8_t* )harbol_linkmap_key_get(&tasm->regs, tasm->lexeme->cstr);
	*offsetref = 0;
	
	tasm->iter = skip_whitespace(tasm->iter);
	/// if there's no plus/minus equation, assume `[reg+0]`
	/// TODO: allow for scaled indexing like * typesize -> [reg+14*4] for easier array accessing.
	const char closer = *tasm->iter;
	if( closer != '-' && closer != '+' && closer != ']' ) {
		tagha_asm_err_out(tasm, "invalid offset math operator '%c' in register indirection", closer);
		return;
	} else if( closer=='-' || closer=='+' ) {
		tasm->iter++;
		tasm->iter = skip_whitespace(tasm->iter);
		if( !is_decimal(*tasm->iter) ) {
			tagha_asm_err_out(tasm, "invalid offset '%s' in register indirection", tasm->lexeme->cstr);
			return;
		}
		tasm->iter = lex_number(tasm->iter, tasm->lexeme);
		tasm->iter = skip_whitespace(tasm->iter);
		int32_t offset = 0;
		string_to_int(tasm->lexeme, sizeof offset, &offset, false);
		//printf("offset == %i\n", offset);
		*offsetref = closer=='-' ? -offset : offset;
	}
	
	if( *tasm->iter != ']' ) {
		tagha_asm_err_out(tasm, "missing closing ']' bracket in register indirection");
		return;
	}
	tasm->iter++;
}

NO_NULL int64_t lex_label_value(struct TaghaAssembler *const tasm, const bool firstpass)
{
	const bool isfunclbl = *tasm->iter=='%';
	tasm->iter = lex_identifier(tasm->iter, tasm->lexeme);
	tasm->prog_counter += 8;
	if( !isfunclbl )
		harbol_string_add_str(tasm->lexeme, tasm->active_func_label);
	
	if( !firstpass && !harbol_linkmap_has_key(isfunclbl ? &tasm->funcmap : &tasm->labelmap, tasm->lexeme->cstr) ) {
		tagha_asm_err_out(tasm, "undefined label '%s'", tasm->lexeme->cstr);
		return 0;
	}
	if( !firstpass ) {
		struct Label *label = harbol_linkmap_key_get(isfunclbl ? &tasm->funcmap : &tasm->labelmap, tasm->lexeme->cstr);
		if( !isfunclbl ) {
		#ifdef TASM_DEBUG
			printf("label->addr (%" PRIu64 ") - tasm->prog_counter (%zu) == '%" PRIi64 "'\n", label->addr, tasm->prog_counter, label->addr - tasm->prog_counter);
		#endif
			return label->addr - tasm->prog_counter;
		}
		else return label->is_native ? -(( int64_t )harbol_linkmap_get_key_index(&tasm->funcmap, tasm->lexeme->cstr) + 1LL) : (( int64_t )harbol_linkmap_get_key_index(&tasm->funcmap, tasm->lexeme->cstr) + 1LL);
	}
	return 0;
}


NO_NULL void tagha_asm_parse_reg_reg(struct TaghaAssembler *const tasm, const bool firstpass)
{
	if( tasm->src==NULL || tasm->iter==NULL )
		return;
	
	tasm->prog_counter++;
	tasm->iter = skip_whitespace(tasm->iter);
	
	if( *tasm->iter != 'r' ) {
		tagha_asm_err_out(tasm, "opcode requires a register as 1st operand.");
		return;
	}
	const uint8_t destreg = lex_reg_id(tasm);
	
	/// ok, let's read the secondary operand!
	tasm->iter = skip_whitespace(tasm->iter);
	if( *tasm->iter==',' )
		tasm->iter++;
	
	tasm->iter = skip_whitespace(tasm->iter);
	
	if( *tasm->iter != 'r' ) {
		tagha_asm_err_out(tasm, "opcode requires a register as 2nd operand.");
		return;
	}
	const uint8_t srcreg = lex_reg_id(tasm);
	
	if( !firstpass ) {
		struct Label *label = harbol_linkmap_key_get(&tasm->funcmap, tasm->active_func_label->cstr);
		if( !label ) {
			tagha_asm_err_out(tasm, "undefined label '%s'.", tasm->active_func_label->cstr);
			return;
		}
		harbol_bytebuffer_insert_byte(&label->bytecode, destreg);
		harbol_bytebuffer_insert_byte(&label->bytecode, srcreg);
	}
}

NO_NULL void tagha_asm_parse_reg(struct TaghaAssembler *const tasm, const bool firstpass)
{
	if( tasm->src==NULL || tasm->iter==NULL )
		return;
	
	tasm->prog_counter++;
	tasm->iter = skip_whitespace(tasm->iter);
	if( *tasm->iter != 'r' ) {
		tagha_asm_err_out(tasm, "opcode requires a register as 1st operand.");
		return;
	}
	const uint8_t regid = lex_reg_id(tasm);
	if( !firstpass ) {
		struct Label *label = harbol_linkmap_key_get(&tasm->funcmap, tasm->active_func_label->cstr);
		if( !label ) {
			tagha_asm_err_out(tasm, "undefined label '%s'.", tasm->active_func_label->cstr);
			return;
		}
		harbol_bytebuffer_insert_byte(&label->bytecode, regid);
	}
}

NO_NULL void tagha_asm_parse_imm(struct TaghaAssembler *const tasm, const bool firstpass)
{
	if( tasm->src==NULL || tasm->iter==NULL )
		return;
	
	tasm->prog_counter++;
	tasm->iter = skip_whitespace(tasm->iter);
	
	int64_t immval = 0;
	/// imm value.
	if( is_decimal(*tasm->iter) )
		immval = lex_imm_value(tasm);
	/// label value.
	else if( *tasm->iter=='.' || *tasm->iter=='%' )
		immval = lex_label_value(tasm, firstpass);
	/// global variable label.
	else if( is_alphabetic(*tasm->iter) ) {
		tasm->iter = lex_identifier(tasm->iter, tasm->lexeme);
		if( !harbol_linkmap_has_key(&tasm->varmap, tasm->lexeme->cstr) ) {
			tagha_asm_err_out(tasm, "undefined global var '%s' in opcode.", tasm->lexeme->cstr);
			return;
		}
		tasm->prog_counter += 8;
		immval = harbol_linkmap_get_key_index(&tasm->varmap, tasm->lexeme->cstr);
	#ifdef TASM_DEBUG
		printf("tasm: global's '%s' index is '%zu'\n", tasm->lexeme->cstr, harbol_linkmap_get_key_index(&tasm->varmap, tasm->lexeme->cstr));
	#endif
	} else {
		tagha_asm_err_out(tasm, "opcode requires an immediate or label value as 1st operand.");
		return;
	}
	
	if( !firstpass ) {
		struct Label *label = harbol_linkmap_key_get(&tasm->funcmap, tasm->active_func_label->cstr);
		if( !label ) {
			tagha_asm_err_out(tasm, "undefined label '%s'.", tasm->active_func_label->cstr);
			return;
		}
		harbol_bytebuffer_insert_int64(&label->bytecode, ( uint64_t )immval);
	}
}

NO_NULL void tagha_asm_parse_reg_mem(struct TaghaAssembler *const tasm, const bool firstpass)
{
	if( tasm->src==NULL || tasm->iter==NULL )
		return;
	
	tasm->prog_counter++;
	tasm->iter = skip_whitespace(tasm->iter);
	
	if( *tasm->iter != 'r' ) {
		tagha_asm_err_out(tasm, "opcode requires a register as 1st operand.");
		return;
	}
	const uint8_t destreg = lex_reg_id(tasm);
	
	/// ok, let's read the secondary operand!
	tasm->iter = skip_whitespace(tasm->iter);
	if( *tasm->iter==',' )
		tasm->iter++;
	
	tasm->iter = skip_whitespace(tasm->iter);
	
	if( *tasm->iter != '[' ) {
		tagha_asm_err_out(tasm, "opcode requires a memory dereference as 1st operand.");
		return;
	}
	uint8_t srcreg;
	int32_t offset;
	lex_register_deref(tasm, &srcreg, &offset);
	
	if( !firstpass ) {
		struct Label *label = harbol_linkmap_key_get(&tasm->funcmap, tasm->active_func_label->cstr);
		if( !label ) {
			tagha_asm_err_out(tasm, "undefined label '%s'.", tasm->active_func_label->cstr);
			return;
		}
		harbol_bytebuffer_insert_byte(&label->bytecode, destreg);
		harbol_bytebuffer_insert_byte(&label->bytecode, srcreg);
		harbol_bytebuffer_insert_int32(&label->bytecode, ( uint32_t )offset);
	}
}

NO_NULL void tagha_asm_parse_mem_reg(struct TaghaAssembler *const tasm, const bool firstpass)
{
	if( tasm->src==NULL || tasm->iter==NULL )
		return;
	
	tasm->prog_counter++;
	tasm->iter = skip_whitespace(tasm->iter);
	
	if( *tasm->iter != '[' ) {
		tagha_asm_err_out(tasm, "opcode requires a memory dereference as 1st operand.");
		return;
	}
	uint8_t destreg;
	int32_t offset;
	lex_register_deref(tasm, &destreg, &offset);
	
	/// ok, let's read the secondary operand!
	tasm->iter = skip_whitespace(tasm->iter);
	if( *tasm->iter==',' )
		tasm->iter++;
	
	tasm->iter = skip_whitespace(tasm->iter);
	if( *tasm->iter != 'r' ) {
		tagha_asm_err_out(tasm, "opcode requires a register as 2nd operand.");
		return;
	}
	const uint8_t srcreg = lex_reg_id(tasm);
	
	if( !firstpass ) {
		struct Label *label = harbol_linkmap_key_get(&tasm->funcmap, tasm->active_func_label->cstr);
		if( !label ) {
			tagha_asm_err_out(tasm, "undefined label '%s'.", tasm->active_func_label->cstr);
			return;
		}
		harbol_bytebuffer_insert_byte(&label->bytecode, destreg);
		harbol_bytebuffer_insert_byte(&label->bytecode, srcreg);
		harbol_bytebuffer_insert_int32(&label->bytecode, ( uint32_t )offset);
	}
}

NO_NULL void tagha_asm_parse_reg_imm(struct TaghaAssembler *const tasm, const bool firstpass)
{
	if( tasm->src==NULL || tasm->iter==NULL )
		return;
	
	tasm->prog_counter++;
	tasm->iter = skip_whitespace(tasm->iter);
	if( *tasm->iter != 'r' ) {
		tagha_asm_err_out(tasm, "opcode requires a register as 1st operand.");
		return;
	}
	const uint8_t regid = lex_reg_id(tasm);
	
	/// ok, let's read the secondary operand!
	tasm->iter = skip_whitespace(tasm->iter);
	if( *tasm->iter==',' )
		tasm->iter++;
	
	tasm->iter = skip_whitespace(tasm->iter);
	
	int64_t immval = 0;
	/// imm value.
	if( is_decimal(*tasm->iter) )
		immval = lex_imm_value(tasm);
	/// label value.
	else if( *tasm->iter=='.' || *tasm->iter=='%' )
		immval = lex_label_value(tasm, firstpass);
	/// global variable label.
	else if( is_alphabetic(*tasm->iter) ) {
		tasm->iter = lex_identifier(tasm->iter, tasm->lexeme);
		if( !harbol_linkmap_has_key(&tasm->varmap, tasm->lexeme->cstr) ) {
			tagha_asm_err_out(tasm, "undefined global var '%s' in opcode.", tasm->lexeme->cstr);
			return;
		}
		tasm->prog_counter += 8;
		immval = ( int64_t )harbol_linkmap_get_key_index(&tasm->varmap, tasm->lexeme->cstr);
	#ifdef TASM_DEBUG
		printf("tasm: global's '%s' index is '%zu'\n", tasm->lexeme->cstr, harbol_linkmap_get_key_index(&tasm->varmap, tasm->lexeme->cstr));
	#endif
	} else {
		tagha_asm_err_out(tasm, "opcode requires an immediate or label value as 2nd operand.");
		return;
	}
	
	if( !firstpass ) {
		struct Label *label = harbol_linkmap_key_get(&tasm->funcmap, tasm->active_func_label->cstr);
		if( !label ) {
			tagha_asm_err_out(tasm, "undefined label '%s'.", tasm->active_func_label->cstr);
			return;
		}
		harbol_bytebuffer_insert_byte(&label->bytecode, regid);
		harbol_bytebuffer_insert_int64(&label->bytecode, ( uint64_t )immval);
	}
}

NO_NULL bool tagha_asm_assemble(struct TaghaAssembler *const tasm)
{
	if( tasm->src==NULL )
		return false;
	
	/// set up our data.
	tasm->lexeme = &(struct HarbolString)EMPTY_HARBOL_STRING;
	tasm->labelmap = harbol_linkmap_create(sizeof(struct Label));
	tasm->funcmap = harbol_linkmap_create(sizeof(struct Label));
	tasm->varmap = harbol_linkmap_create(sizeof(struct HarbolByteBuf));
	tasm->opcodes = harbol_linkmap_create(sizeof(uint8_t));
	tasm->regs = harbol_linkmap_create(sizeof(uint8_t));
	
	/// set up registers + map their IDs
#define Y(y)    harbol_linkmap_insert(&tasm->regs, "r"#y, &(uint8_t){y});
	TAGHA_REG_FILE;
#undef Y
	harbol_linkmap_insert(&tasm->regs, "rfp", &(uint8_t){bp});
	
	/// for helping clarity and convenience, names like r0, r5, etc. can be used as an alternative!
	for( enum TaghaRegID id=alaf; id<MaxRegisters; id++ ) {
		char reg_num[10] = {0};
		snprintf(reg_num, sizeof reg_num, "r%d", id);
		harbol_linkmap_insert(&tasm->regs, reg_num, &(uint8_t){id});
	}
	
	/// for strictly params/args, arg0, arg5, arg13, etc. can also be used as an alternative!
	for( enum TaghaRegID id=TAGHA_FIRST_PARAM_REG; id<=TAGHA_LAST_PARAM_REG; id++ ) {
		char reg_param[10] = {0};
		snprintf(reg_param, sizeof reg_param, "rarg%d", id - TAGHA_FIRST_PARAM_REG);
		harbol_linkmap_insert(&tasm->regs, reg_param, &(uint8_t){id});
	}
	
	/// set up our instruction set!
#define X(x)    harbol_linkmap_insert(&tasm->opcodes, #x, &(uint8_t){x});
	TAGHA_INSTR_SET;
#undef X
	
	/// add additional for specific opcodes.
	harbol_linkmap_insert(&tasm->opcodes, "and", &(uint8_t){bit_and});
	harbol_linkmap_insert(&tasm->opcodes, "or", &(uint8_t){bit_or});
	harbol_linkmap_insert(&tasm->opcodes, "xor", &(uint8_t){bit_xor});
	harbol_linkmap_insert(&tasm->opcodes, "not", &(uint8_t){bit_not});
	
	/** FIRST PASS. Collect labels + their PC relative addresses */
	enum{ MAX_LINE_CHARS=2048 };
	char line_buffer[MAX_LINE_CHARS] = {0};
	
#ifdef TASM_DEBUG
	puts("\ntasm: FIRST PASS Begin!\n");
#endif
	
	for( tasm->iter = fgets(line_buffer, MAX_LINE_CHARS, tasm->src) ; tasm->iter != NULL ; tasm->iter = fgets(line_buffer, MAX_LINE_CHARS, tasm->src) ) {
		/// set up first line for error checks.
		tasm->currline++;
	#ifdef TASM_DEBUG
		//printf("tasm debug: printing line:: '%s'\n", tasm->iter);
	#endif
		while( *tasm->iter != 0 ) {
			harbol_string_clear(tasm->lexeme);
			/// skip whitespace.
			tasm->iter = skip_whitespace(tasm->iter);
			
			/// skip to next line if comment.
			if( *tasm->iter=='\n' || *tasm->iter==';' || *tasm->iter=='#' )
				break;
			else if( *tasm->iter=='}' ) {
				tasm->iter++;
				harbol_string_clear(tasm->active_func_label);
				tasm->active_func_label = NULL;
				tasm->prog_counter = 0;
				break;
			}
			/// parse the directives!
			else if( *tasm->iter=='$' ) {
				tasm->iter = lex_identifier(tasm->iter, tasm->lexeme);
				if( !harbol_string_cmpcstr(tasm->lexeme, "$stacksize") ) {
					tagha_asm_parse_stack_directive(tasm);
				#ifdef TASM_DEBUG
					printf("tasm: stack size set to: %u\n", tasm->stacksize);
				#endif
				} else if( !harbol_string_cmpcstr(tasm->lexeme, "$heapsize") )
					tagha_asm_parse_heap_directive(tasm);
				else if( !harbol_string_cmpcstr(tasm->lexeme, "$global") )
					tagha_asm_parse_globalvar_directive(tasm);
				else if( !harbol_string_cmpcstr(tasm->lexeme, "$native") )
					tagha_asm_parse_native_directive(tasm);
				else if( !harbol_string_cmpcstr(tasm->lexeme, "$extern") )
					tagha_asm_parse_extern_directive(tasm);
				break;
			}
			/// holy cannoli, we found a label!
			else if( *tasm->iter=='.' || *tasm->iter=='%' ) {
				const bool funclbl = *tasm->iter == '%';
				/// the dot or percent is added to our lexeme
				tasm->iter = lex_identifier(tasm->iter, tasm->lexeme);
				tasm->iter = skip_whitespace(tasm->iter);
				if( *tasm->iter == ':' )
					tasm->iter++;
				
				tasm->iter = skip_whitespace(tasm->iter);
				if( funclbl ) {
					if( *tasm->iter != '{' ) {
						tagha_asm_err_out(tasm, "missing curly '{' bracket! Curly bracket must be on the same line as label.");
						continue;
					}
					else tasm->iter++;
				}
				
				if( !is_alphabetic(tasm->lexeme->cstr[1]) ) {
					tagha_asm_err_out(tasm, "%s labels must have alphabetic names!", funclbl ? "function" : "jump");
					continue;
				} else if( harbol_linkmap_has_key(funclbl ? &tasm->funcmap : &tasm->labelmap, tasm->lexeme->cstr) ) {
					tagha_asm_err_out(tasm, "redefinition of label '%s'.", tasm->lexeme->cstr);
					continue;
				}
				
				if( funclbl ) {
					tasm->active_func_label = &(struct HarbolString){0};
					harbol_string_copy_str(tasm->active_func_label, tasm->lexeme);
				} else {
					if( !tasm->active_func_label ) {
						tagha_asm_err_out(tasm, "jump label '%s' outside of function block!", tasm->lexeme->cstr);
						break;
					}
					else harbol_string_add_str(tasm->lexeme, tasm->active_func_label);
				}
				struct Label label = { EMPTY_HARBOL_BYTEBUF, tasm->prog_counter, funclbl, false, false };
#ifdef TASM_DEBUG
				printf("%s Label '%s' is located at address: %zu\n", funclbl ? "Func" : "Local", tasm->lexeme->cstr, tasm->prog_counter);
#endif
				harbol_linkmap_insert(funclbl ? &tasm->funcmap : &tasm->labelmap, tasm->lexeme->cstr, &label);
			}
			/// it's an opcode!
			else if( is_alphabetic(*tasm->iter) ) {
				if( !tasm->active_func_label ) {
					tagha_asm_err_out(tasm, "opcode outside of function block!");
					continue;
				}
				tasm->iter = lex_identifier(tasm->iter, tasm->lexeme);
				if( !harbol_linkmap_has_key(&tasm->opcodes, tasm->lexeme->cstr) ) {
					tagha_asm_err_out(tasm, "unknown opcode '%s'!", tasm->lexeme->cstr);
					continue;
				}
				const uint8_t opcode = *( uint8_t* )harbol_linkmap_key_get(&tasm->opcodes, tasm->lexeme->cstr);
				switch( opcode ) {
					/// opcodes that take no args
					case halt: case ret: case nop:
						tasm->prog_counter++; break;
					
					/// opcodes that only take an imm operand.
					case jmp: case jz: case jnz: case call:
						tagha_asm_parse_imm(tasm, true); break;
					
					/// opcodes that only take a register operand.
					case push: case pop: case bit_not: case neg: case callr:
					case f32tof64: case f64tof32: case itof64: case itof32: case f64toi: case f32toi: case negf:
						tagha_asm_parse_reg(tasm, true); break;
					
					/// opcodes reg<-imm
					case ldvar: case ldfunc: case movi:
						tagha_asm_parse_reg_imm(tasm, true); break;
					
					/// opcodes reg<-mem (load)
					case ldaddr: case ld1: case ld2: case ld4: case ld8:
						tagha_asm_parse_reg_mem(tasm, true); break;
					
					/// opcodes mem<-reg (store)
					case st1: case st2: case st4: case st8:
						tagha_asm_parse_mem_reg(tasm, true); break;
					
					/// opcodes reg<-reg
					case mov:
					case add: case sub: case mul: case divi: case mod:
					case bit_and: case bit_or: case bit_xor: case shl: case shr:
					case ilt: case ile: case ult: case ule: case cmp:
				
					case addf: case subf: case mulf: case divf:
					case ltf: case lef:
						tagha_asm_parse_reg_reg(tasm, true); break;
				}
			}
		}
	}
	if( tasm->errored )
		goto assembling_err_exit;
#ifdef TASM_DEBUG
	puts("\ntasm: FIRST PASS End!\n");
#endif
	rewind(tasm->src);
	
#ifdef TASM_DEBUG
	puts("\ntasm: SECOND PASS Start!\n");
#endif
	tasm->currline = 0;
	
	for( tasm->iter = fgets(line_buffer, MAX_LINE_CHARS, tasm->src) ; tasm->iter != NULL ; tasm->iter = fgets(line_buffer, MAX_LINE_CHARS, tasm->src) ) {
		/// set up first line for error checks.
		tasm->currline++;
	#ifdef TASM_DEBUG
		//printf("tasm debug: printing line:: '%s'\n", tasm->iter);
	#endif
		while( *tasm->iter ) {
			harbol_string_clear(tasm->lexeme);
			/// skip whitespace.
			tasm->iter = skip_whitespace(tasm->iter);
			
			/// skip to next line if comment or directive.
			if( *tasm->iter=='\n' || *tasm->iter==';' || *tasm->iter=='#' || *tasm->iter=='$' )
				break;
			else if( *tasm->iter=='}' ) {
				tasm->iter++;
				harbol_string_clear(tasm->active_func_label);
				tasm->active_func_label = NULL;
				tasm->prog_counter = 0;
				break;
			}
			/// skip labels in second pass.
			else if( *tasm->iter=='.' || *tasm->iter=='%' ) {
				const bool funclbl = *tasm->iter == '%';
				tasm->iter = lex_identifier(tasm->iter, tasm->lexeme);
				tasm->iter++;
				tasm->iter = skip_whitespace(tasm->iter);
				if( funclbl ) {
					tasm->iter++;
					tasm->active_func_label = &(struct HarbolString){0};
					harbol_string_copy_str(tasm->active_func_label, tasm->lexeme);
					tasm->iter = skip_whitespace(tasm->iter);
				}
			}
			/// parse opcode!
			if( is_alphabetic(*tasm->iter) ) {
				tasm->iter = lex_identifier(tasm->iter, tasm->lexeme);
				if( !harbol_linkmap_has_key(&tasm->opcodes, tasm->lexeme->cstr) ) {
					tagha_asm_err_out(tasm, "unknown opcode '%s'!", tasm->lexeme->cstr);
					continue;
				}
				const uint8_t opcode = *( uint8_t* )harbol_linkmap_key_get(&tasm->opcodes, tasm->lexeme->cstr);
				struct Label *label = harbol_linkmap_key_get(&tasm->funcmap, tasm->active_func_label->cstr);
				harbol_bytebuffer_insert_byte(&label->bytecode, opcode);
				switch( opcode ) {
					/// opcodes that take no args
					case halt: case ret: case nop:
						break;
					
					/// opcodes that only take an imm operand.
					case jmp: case jz: case jnz: case call:
						tagha_asm_parse_imm(tasm, false); break;
					
					/// opcodes that only take a register operand.
					case push: case pop: case bit_not: case neg: case callr:
					case f32tof64: case f64tof32: case itof64: case itof32: case f64toi: case f32toi: case negf:
						tagha_asm_parse_reg(tasm, false); break;
					
					/// opcodes reg<-imm
					case ldvar: case ldfunc: case movi:
						tagha_asm_parse_reg_imm(tasm, false); break;
					
					/// opcodes reg<-mem (load)
					case ldaddr: case ld1: case ld2: case ld4: case ld8:
						tagha_asm_parse_reg_mem(tasm, false); break;
					
					/// opcodes mem<-reg (store)
					case st1: case st2: case st4: case st8:
						tagha_asm_parse_mem_reg(tasm, false); break;
					
					/// opcodes reg<-reg
					case mov:
					case add: case sub: case mul: case divi: case mod:
					case bit_and: case bit_or: case bit_xor: case shl: case shr:
					case ilt: case ile: case ult: case ule: case cmp:
					case addf: case subf: case mulf: case divf:
					case ltf: case lef:
						tagha_asm_parse_reg_reg(tasm, false); break;
				}
			}
		}
	}
	
	if( tasm->errored )
		goto assembling_err_exit;
#ifdef TASM_DEBUG
	puts("\ntasm: SECOND PASS End!\n");
#endif
	
	if( tasm->stacksize==0 )
		tasm->stacksize = DEFAULT_STACK_SIZE;
	
	bool is_64_bit = sizeof(intptr_t)==8;
	const size_t memnode_size = (is_64_bit) ? sizeof(struct HarbolMemNode) : sizeof(struct HarbolMemNode) << 1;
	const size_t tagha_kvsize = (is_64_bit) ? sizeof(struct TaghaKeyVal)   : sizeof(struct TaghaKeyVal) << 1;
	
	const struct TaghaItemMap empty_set = {0};
	const size_t tagha_set_buckets_size = (is_64_bit) ? sizeof *empty_set.buckets : (sizeof *empty_set.buckets) << 1;
	const size_t tagha_set_arr_size     = (is_64_bit) ? sizeof *empty_set.array   : (sizeof *empty_set.array) << 1;
	
	uint32_t mem_region_size = tasm->stacksize * sizeof(union TaghaVal) + memnode_size;
	mem_region_size += sizeof &empty_set * 2;
	
	/// create initial script header.
	struct TaghaScriptBuilder tbc = tagha_tbc_gen_create();
	
	/// build our func table & global var table so that we can calculate total memory usage.
	/// first build func table.
	mem_region_size += tagha_set_buckets_size * tasm->funcmap.map.count + memnode_size;
	mem_region_size += tagha_set_arr_size * tasm->funcmap.map.count + memnode_size;
	
	for( size_t i=0; i<tasm->funcmap.map.count; i++ ) {
		struct HarbolKeyVal *node = harbol_linkmap_index_get_kv(&tasm->funcmap, i);
		struct Label *label = harbol_linkmap_index_get(&tasm->funcmap, i);
		if( label==NULL )
			continue;
		
		uint32_t flags = 0;
		if( label->is_native )
			flags |= TAGHA_FLAG_NATIVE;
		if( label->is_extern )
			flags |= TAGHA_FLAG_EXTERN;
		tagha_tbc_gen_write_func(&tbc, flags, node->key.cstr+1, &label->bytecode);
		
	#ifdef TASM_DEBUG
		printf("func label: %s\nData:\n", node->key.cstr);
		if( !label->is_native )
			for( size_t i=0; i<label->bytecode.count; i++ )
				printf("bytecode[%zu] == %u\n", i, label->bytecode.table[i]);
		puts("\n");
	#endif
		mem_region_size += tagha_kvsize + memnode_size;
	}
	
	/// now the global var table.
	mem_region_size += tagha_set_buckets_size * tasm->varmap.map.count + memnode_size;
	mem_region_size += tagha_set_arr_size * tasm->varmap.map.count + memnode_size;
	for( size_t i=0; i<tasm->varmap.map.count; i++ ) {
		struct HarbolKeyVal *node = harbol_linkmap_index_get_kv(&tasm->varmap, i);
		struct HarbolByteBuf *bytedata = harbol_linkmap_index_get(&tasm->varmap, i);
		if( bytedata==NULL )
			continue;
		
		tagha_tbc_gen_write_var(&tbc, 0, node->key.cstr, bytedata);
		
	#ifdef TASM_DEBUG
		printf("global var: %s\nData:\n", node->key.cstr);
		for( size_t i=0; i<bytedata->count; i++ )
			printf("bytecode[%zu] == %u\n", i, bytedata->table[i]);
		puts("\n");
	#endif
		mem_region_size += tagha_kvsize + memnode_size;
	}
	if( tasm->heapsize==0 )
		tasm->heapsize = DEFAULT_HEAP_SIZE;
	mem_region_size += tasm->heapsize;
	
	/// now that we've made the tables and calculated how much memory we need, we finally initialize our header.
	tagha_tbc_gen_write_header(&tbc, tasm->stacksize, (uint32_t)harbol_align_size(mem_region_size, 8), 0);
	
	{
		char *iter = tasm->outname.cstr;
		size_t len = 0;
		while( *++iter );
		while( tasm->outname.cstr < iter && *--iter != '.' )
			++len;
		
		memset(iter, 0, len);
	}
	
	/// use line_buffer instead of wasting more stack space.
	memset(line_buffer, 0, sizeof line_buffer);
	sprintf(line_buffer, "%.2000s.tbc", tasm->outname.cstr);
	
	/** tagha_tbc_create_file will write in the final data needed to make the tbc.
	 * Will also free the TaghaScriptBuilder data.
	 */
	tagha_tbc_gen_create_file(&tbc, line_buffer);
	
assembling_err_exit:
	harbol_string_clear(&tasm->outname);
	harbol_string_clear(tasm->lexeme);
	if( tasm->active_func_label != NULL )
		harbol_string_clear(tasm->active_func_label), tasm->active_func_label = NULL;
	
	harbol_linkmap_clear(&tasm->labelmap, (void(*)(void**))label_free);
	harbol_linkmap_clear(&tasm->funcmap, label_free);
	harbol_linkmap_clear(&tasm->varmap, (void(*)(void**))harbol_bytebuffer_free);
	harbol_linkmap_clear(&tasm->opcodes, NULL);
	harbol_linkmap_clear(&tasm->regs, NULL);
	fclose(tasm->src); tasm->src=NULL;
	memset(tasm, 0, sizeof *tasm);
	return true;
}

void label_free(void **p)
{
	struct Label *const label = *p;
	harbol_bytebuffer_clear(&label->bytecode);
	free(*p), *p=NULL;
}

static void tagha_asm_parse_opts(struct TaghaAssembler *const restrict tasm, const char arg[static 1])
{
	(void)tasm;
	(void)arg;
	/*const char *opt = &arg[1];
	if( !strncmp(opt, "stack", sizeof "stack"-1) ) {
		const char *arg = opt + sizeof "stack" - 1;
		if( *arg != '=' ) {
			fprintf(stderr, "Tagha Assembler - bad command-line option argument :: '%s'\n", arg);
		} else {
			arg++;
			const size_t num = strtoul(arg, NULL, 10);
			tasm->stacksize = num;
		}
	} else if( !strncmp(opt, "heap", sizeof "heap"-1) ) {
		const char *arg = opt + sizeof "heap" - 1;
		if( *arg != '=' ) {
			fprintf(stderr, "Tagha Assembler - bad command-line option argument :: '%s'\n", arg);
		} else {
			arg++;
			const size_t num = strtoul(arg, NULL, 10);
			tasm->heapsize = num;
		}
	} else {
		fprintf(stderr, "Tagha Assembler - unknown option arg :: '%s'\n", arg);
	}*/
}

int main(const int argc, char *argv[restrict static 1])
{
	if( argc<=1 ) {
		fprintf(stderr, "Tagha Assembler - usage: %s [.tasm file...]\n", argv[0]);
		return -1;
	} else if( !strcmp(argv[1], "--help") ) {
		puts("Tagha Assembler - Tagha Runtime Environment Toolkit\nTo compile a tasm script to tbc, supply a script name as a command-line argument to the program.\nExample: './tagha_asm [options] script.tasm'");
	} else if( !strcmp(argv[1], "--version") ) {
		puts("Tagha Assembler Version 1.0.4");
	} else {
		struct TaghaAssembler tasm = EMPTY_TAGHA_ASSEMBLER;
		for( int i=1; i<argc; i++ ) {
			if( argv[i][0]=='-' ) {
				tagha_asm_parse_opts(&tasm, argv[i]);
			} else {
				FILE *tasmfile = fopen(argv[i], "r");
				if( tasmfile==NULL )
					continue;
				
				tasm.src = tasmfile;
				tasm.srcsize = get_file_size(tasmfile);
				tasm.outname = harbol_string_create(argv[i]);
				tagha_asm_assemble(&tasm);
			}
		}
	}
}
