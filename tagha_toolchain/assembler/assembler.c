#include "../libharbol/harbol.h"
#include "../../tagha/tagha.h"
#include "../instr_gen.h"
#include "../module_gen.h"

//#define TAGHA_ASM_DEBUG

#define COLOR_RED       "\x1B[31m"
#define COLOR_MAGENTA   "\x1B[35m"
#define COLOR_GREEN     "\x1B[32m"
#define COLOR_RESET     "\033[0m"


static inline bool is_possible_ident(const int c)
{
	return( (c >= 'a' && c <= 'z')
		|| (c >= 'A' && c <= 'Z')
		|| c=='_' || c=='@' || c=='$'
		|| (c >= '0' && c <= '9')
		|| c < -1 );
}

static inline bool is_alphabetical(const int c)
{
	return( (c >= 'a' && c <= 'z')
		|| (c >= 'A' && c <= 'Z')
		|| c=='_' || c=='@' || c=='$'
		|| c < -1 );
}

static bool lex_id(const char str[static 1], const char **const end, struct HarbolString *const restrict buf)
{
	harbol_string_clear(buf);
	if( is_alphabetical(*str) ) {
		harbol_string_add_char(buf, *str++);
		return lex_identifier(str, end, buf, is_possible_ident);
	} else {
		return false;
	}
}

typedef struct {
	struct HarbolByteBuf data;
	size_t offset;
	bool
		is_func   : 1,
		is_native : 1,
		is_extern : 1
	;
} Label;

void label_free(void **const p)
{
	Label *const l = *p;
	harbol_bytebuffer_clear(&l->data);
}


struct {
	struct HarbolLinkMap
		labels,
		funcs,
		vars,
		opcodes
	;
	struct HarbolString src, outfile, lexeme, *active_label;
	const char *iter;
	size_t line, pc;
	uint32_t callstacksize, opstacksize, heapsize;
	bool err : 1;
} tagha_asm;

static void _tagha_asm_setup_opcodes(void)
{
	tagha_asm.opcodes = harbol_linkmap_create(sizeof(uint8_t));
#define X(x)    harbol_linkmap_insert(&tagha_asm.opcodes, #x, &(uint8_t){x});
	TAGHA_INSTR_SET;
#undef X
	harbol_linkmap_insert(&tagha_asm.opcodes, "and", &(uint8_t){bit_and});
	harbol_linkmap_insert(&tagha_asm.opcodes, "or",  &(uint8_t){bit_or});
	harbol_linkmap_insert(&tagha_asm.opcodes, "xor", &(uint8_t){bit_xor});
	harbol_linkmap_insert(&tagha_asm.opcodes, "not", &(uint8_t){bit_not});
}
static void _tagha_asm_setup_data(void)
{
	tagha_asm.labels = harbol_linkmap_create(sizeof(Label));
	tagha_asm.funcs = harbol_linkmap_create(sizeof(Label));
	tagha_asm.vars = harbol_linkmap_create(sizeof(struct HarbolByteBuf));
}

static void _tagha_asm_clear_data(void)
{
	harbol_linkmap_clear(&tagha_asm.labels, label_free);
	harbol_linkmap_clear(&tagha_asm.funcs, label_free);
	harbol_linkmap_clear(&tagha_asm.vars, ( void(*)(void**) )harbol_bytebuffer_free);
	harbol_string_clear(&tagha_asm.outfile);
	harbol_string_clear(&tagha_asm.src);
}
static void _tagha_asm_zero_out(void)
{
	harbol_linkmap_clear(&tagha_asm.opcodes, NULL);
	memset(&tagha_asm, 0, sizeof tagha_asm);
}

static void _tagha_asm_skip_whitespace(void)
{
	tagha_asm.iter = skip_chars_until_newline(tagha_asm.iter, is_whitespace);
}

static void _tagha_asm_skip_delim(const int c)
{
	_tagha_asm_skip_whitespace();
	if( *tagha_asm.iter==c )
		tagha_asm.iter++;
	_tagha_asm_skip_whitespace();
}


static NEVER_NULL(5) void _tagha_asm_err(const char filename[restrict], const char errtype[restrict], const size_t line, const size_t col, const char err[restrict static 1], ...)
{
	va_list args;
	va_start(args, err);
	
	printf("(%s:%zu:%zu) %s%s%s: **** ", filename, line, col, COLOR_RED, errtype, COLOR_RESET);
	vprintf(err, args);
	printf(" ****\n");
	va_end(args);
	tagha_asm.err = true;
}

static bool string_to_int(struct HarbolString *const str, int64_t *const i)
{
	harbol_string_clear(str);
	const char *ender = NULL;
	bool isfloat = false;
	const bool res = lex_c_style_number(tagha_asm.iter, &ender, str, &isfloat);
	if( res ) {
		const bool is_binary = !harbol_string_cmpcstr(str, "0b") || !harbol_string_cmpcstr(str, "0B") ? true : false;
		*i = isfloat ? ( int64_t )strtod(str->cstr, NULL) : strtoll(is_binary ? str->cstr+2 : str->cstr, NULL, 0);
		if( ender != NULL )
			tagha_asm.iter = ender;
		else tagha_asm.iter++;
	} else {
		_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "bad number string.");
	}
	return res;
}

/// $opstack_size ####
static void tagha_asm_parse_opstacksize(void)
{
	_tagha_asm_skip_whitespace();
	if( !is_decimal(*tagha_asm.iter) ) {
		_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "opstack size directive requires a valid number!");
		return;
	}
	
	int64_t num = 0;
	if( string_to_int(&tagha_asm.lexeme, &num) )
		tagha_asm.opstacksize = ( uint32_t )(num * sizeof(union TaghaVal));
	else {
		_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "bad opstack_size value: '%s'", tagha_asm.lexeme.cstr);
	}
	
#ifdef TAGHA_ASM_DEBUG
	printf("opstack size = %u\n", tagha_asm.opstacksize);
#endif
}

/// $callstack_size ####
static void tagha_asm_parse_callstacksize(void)
{
	_tagha_asm_skip_whitespace();
	if( !is_decimal(*tagha_asm.iter) ) {
		_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "callstack size directive requires a valid number!");
		return;
	}
	
	int64_t num = 0;
	if( string_to_int(&tagha_asm.lexeme, &num) )
		tagha_asm.callstacksize = ( uint32_t )(num * sizeof(uintptr_t));
	else {
		_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "bad callstack_size value: '%s'", tagha_asm.lexeme.cstr);
	}
	
#ifdef TAGHA_ASM_DEBUG
	printf("callstack size = %u\n", tagha_asm.callstacksize);
#endif
}

/// $heap_size ####
static void tagha_asm_parse_heapsize(void)
{
	_tagha_asm_skip_whitespace();
	if( !is_decimal(*tagha_asm.iter) ) {
		_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "heap size directive requires a valid number!");
		return;
	}
	
	int64_t num = 0;
	if( string_to_int(&tagha_asm.lexeme, &num) )
		tagha_asm.heapsize = ( uint32_t )num;
	else {
		_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "bad heap_size value: '%s'", tagha_asm.lexeme.cstr);
	}
#ifdef TAGHA_ASM_DEBUG
	printf("heap size = %u\n", tagha_asm.heapsize);
#endif
}

/// $global varname bytes ...
static void tagha_asm_parse_global(void)
{
	_tagha_asm_skip_whitespace();
	if( !is_alphabetical(*tagha_asm.iter) ) {
		_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "global directive requires the 1st argument to be a variable name!");
		return;
	}
	
	struct HarbolByteBuf vardata = harbol_bytebuffer_create();
	struct HarbolString varname = harbol_string_create("");
	const char *end = NULL;
	if( lex_id(tagha_asm.iter, &end, &varname) ) {
		tagha_asm.iter = end;
	} else {
		_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "invalid var name for global data: '%s'", varname.cstr);
		goto tagha_asm_global_err;
	}

#ifdef TAGHA_ASM_DEBUG
	printf("var name: '%s'\n", varname.cstr);
#endif
	
	_tagha_asm_skip_delim(',');
	
	if( *tagha_asm.iter=='"' || *tagha_asm.iter=='\'' || *tagha_asm.iter=='`' ) {
		struct HarbolString string_data = harbol_string_create("");
		const char *end = NULL;
		if( lex_go_style_str(tagha_asm.iter, &end, &string_data) ) {
			tagha_asm.iter = end;
		#ifdef TAGHA_ASM_DEBUG
			printf("string data: '%s'\n", string_data.cstr);
		#endif
			harbol_bytebuffer_insert_cstr(&vardata, string_data.cstr);
			harbol_string_clear(&string_data);
		} else {
			_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "bad string data for global var '%s'", varname.cstr);
			harbol_string_clear(&string_data);
			goto tagha_asm_global_err;
		}
	} else if( is_decimal(*tagha_asm.iter) ) {
		int64_t num = 0;
		if( !string_to_int(&tagha_asm.lexeme, &num) ) {
			_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "invalid byte size in global directive '%" PRIi64 "'", num);
			goto tagha_asm_global_err;
		}
		size_t bytes = ( size_t )num;
	#ifdef TAGHA_ASM_DEBUG
		printf("bytesize: '%zu'\n", bytes);
	#endif
		
		_tagha_asm_skip_delim(',');
		
		while( bytes != 0 ) {
			_tagha_asm_skip_delim(',');
			
			if( is_decimal(*tagha_asm.iter) ) {
				int64_t arg = 0; string_to_int(&tagha_asm.lexeme, &arg);
				if( arg != 0LL ) {
					_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "bad global argument number '%s'", tagha_asm.lexeme.cstr);
					goto tagha_asm_global_err;
				}
				tagha_asm.iter++;
				_tagha_asm_skip_whitespace();
				harbol_bytebuffer_insert_zeros(&vardata, bytes);
				bytes = 0;
			} else if( is_alphabetical(*tagha_asm.iter) ) {
				if( !lex_id(tagha_asm.iter, &tagha_asm.iter, &tagha_asm.lexeme) ) {
					_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "bad global argument data size.");
					goto tagha_asm_global_err;
				}
			#ifdef TAGHA_ASM_DEBUG
				printf("data size name: '%s'\n", tagha_asm.lexeme.cstr);
			#endif
				_tagha_asm_skip_whitespace();
				if( !harbol_string_cmpcstr(&tagha_asm.lexeme, "byte") ) {
					tagha_asm.iter = skip_chars_until_newline(tagha_asm.iter, is_whitespace);
					if( !string_to_int(&tagha_asm.lexeme, &num) ) {
						_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "invalid byte arg in global directive '%" PRIi64 "'", num);
						goto tagha_asm_global_err;
					}
				#ifdef TAGHA_ASM_DEBUG
					printf("data size byte: '%u'\n", ( uint8_t )num);
				#endif
					harbol_bytebuffer_insert_byte(&vardata, ( uint8_t )num);
					bytes--;
				} else if( !harbol_string_cmpcstr(&tagha_asm.lexeme, "half") ) {
					_tagha_asm_skip_whitespace();
					if( !string_to_int(&tagha_asm.lexeme, &num) ) {
						_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "invalid half arg in global directive '%" PRIi64 "'", num);
						goto tagha_asm_global_err;
					}
				#ifdef TAGHA_ASM_DEBUG
					printf("data size half: '%u'\n", ( uint16_t )num);
				#endif
					harbol_bytebuffer_insert_int16(&vardata, ( uint16_t )num);
					bytes -= sizeof(uint16_t);
				} else if( !harbol_string_cmpcstr(&tagha_asm.lexeme, "long") ) {
					_tagha_asm_skip_whitespace();
					if( !string_to_int(&tagha_asm.lexeme, &num) ) {
						_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "invalid long arg in global directive '%" PRIi64 "'", num);
						goto tagha_asm_global_err;
					}
				#ifdef TAGHA_ASM_DEBUG
					printf("data size long: '%u'\n", ( uint32_t )num);
				#endif
					harbol_bytebuffer_insert_int32(&vardata, ( uint32_t )num);
					bytes -= sizeof(uint32_t);
				} else if( !harbol_string_cmpcstr(&tagha_asm.lexeme, "word") ) {
					_tagha_asm_skip_whitespace();
					if( !string_to_int(&tagha_asm.lexeme, &num) ) {
						_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "invalid word arg in global directive '%" PRIi64 "'", num);
						goto tagha_asm_global_err;
					}
				#ifdef TAGHA_ASM_DEBUG
					printf("data size long: '%" PRIu64 "'\n", num);
				#endif
					harbol_bytebuffer_insert_int64(&vardata, ( uint64_t )num);
					bytes -= sizeof(uint64_t);
				}
			} else {
				_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "global var directive data set for '%s' is incomplete, must be equal to bytesize given.", varname.cstr);
				goto tagha_asm_global_err;
			}
		}
	} else {
		_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "missing byte size number or string literal in global directive.");
		goto tagha_asm_global_err;
	}
	
#ifdef TAGHA_ASM_DEBUG
	printf("tasm debug: vardata->count: %zu\n", vardata.count);
#endif
	harbol_linkmap_insert(&tagha_asm.vars, varname.cstr, &vardata);
	
tagha_asm_global_err:
	harbol_string_clear(&varname);
}

/// $native <name>
static void tagha_asm_parse_native(void)
{
	_tagha_asm_skip_whitespace();
	if( !is_alphabetic(*tagha_asm.iter) ) {
		_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "native directive requires a valid identifier!");
		return;
	}
	
	if( lex_id(tagha_asm.iter, &tagha_asm.iter, &tagha_asm.lexeme) ) {
	#ifdef TAGHA_ASM_DEBUG
		printf("native func: %s\n", tagha_asm.lexeme.cstr);
	#endif
		Label native = { EMPTY_HARBOL_BYTEBUF, 0, true, true, false };
		harbol_linkmap_insert(&tagha_asm.funcs, tagha_asm.lexeme.cstr, &native);
	} else {
		_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "bad name '%s' for native directive", tagha_asm.lexeme.cstr);
		return;
	}
}

/// $extern <name>
static void tagha_asm_parse_extern(void)
{
	_tagha_asm_skip_whitespace();
	if( !is_alphabetic(*tagha_asm.iter) ) {
		_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "extern directive requires a valid identifier!");
		return;
	}
	
	if( lex_id(tagha_asm.iter, &tagha_asm.iter, &tagha_asm.lexeme) ) {
	#ifdef TAGHA_ASM_DEBUG
		printf("extern func: %s\n", tagha_asm.lexeme.cstr);
	#endif
		Label extrn = { EMPTY_HARBOL_BYTEBUF, 0, true, false, true };
		harbol_linkmap_insert(&tagha_asm.funcs, tagha_asm.lexeme.cstr, &extrn);
	} else {
		_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "bad name '%s' for extern directive", tagha_asm.lexeme.cstr);
	}
}


static void tagha_asm_assemble(void)
{
	_tagha_asm_setup_data();
	
	struct HarbolString _active = {0};
	
#ifdef TAGHA_ASM_DEBUG
	puts("First Pass\n");
#endif
	/// First Pass.
	tagha_asm.iter = tagha_asm.src.cstr;
	tagha_asm.line = 1;
	tagha_asm.pc = 0;
	while( *tagha_asm.iter != 0 ) {
		harbol_string_clear(&tagha_asm.lexeme);
		if( tagha_asm.err ) {
			goto tagha_asm_err;
		}
		_tagha_asm_skip_whitespace();
		
		if( *tagha_asm.iter=='\n' ) {
			tagha_asm.line++;
			tagha_asm.iter++;
			continue;
		} else if( *tagha_asm.iter==';' ) {
			tagha_asm.iter = skip_single_line_comment(tagha_asm.iter);
			continue;
		} else if( !strncmp(tagha_asm.iter, "/*", sizeof "/*" - 1) ) {
			tagha_asm.iter = skip_multi_line_comment(tagha_asm.iter, "*/", sizeof "*/" - 1);
			continue;
		} else if( *tagha_asm.iter=='$' ) {
			harbol_string_add_char(&tagha_asm.lexeme, *tagha_asm.iter++);
			lex_c_style_identifier(tagha_asm.iter, &tagha_asm.iter, &tagha_asm.lexeme);
			if( !harbol_string_cmpcstr(&tagha_asm.lexeme, "$opstack_size") ) {
				tagha_asm_parse_opstacksize();
			} else if( !harbol_string_cmpcstr(&tagha_asm.lexeme, "$callstack_size") ) {
				tagha_asm_parse_callstacksize();
			} else if( !harbol_string_cmpcstr(&tagha_asm.lexeme, "$heap_size") ) {
				tagha_asm_parse_heapsize();
			} else if( !harbol_string_cmpcstr(&tagha_asm.lexeme, "$global") ) {
				tagha_asm_parse_global();
			} else if( !harbol_string_cmpcstr(&tagha_asm.lexeme, "$native") ) {
				tagha_asm_parse_native();
			} else if( !harbol_string_cmpcstr(&tagha_asm.lexeme, "$extern") ) {
				tagha_asm_parse_extern();
			} else {
				_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "unknown directive: '%s'", tagha_asm.lexeme.cstr);
				goto tagha_asm_err;
			}
		} else if( *tagha_asm.iter=='}' ) {
			tagha_asm.iter++;
		#ifdef TAGHA_ASM_DEBUG
			printf("end of func definition: %s\n", tagha_asm.active_label->cstr);
		#endif
			harbol_string_clear(tagha_asm.active_label);
			tagha_asm.active_label = NULL;
			tagha_asm.pc = 0;
		} else if( is_alphabetic(*tagha_asm.iter) ) {
			lex_id(tagha_asm.iter, &tagha_asm.iter, &tagha_asm.lexeme);
		#ifdef TAGHA_ASM_DEBUG
			printf("id definition: %s\n", tagha_asm.lexeme.cstr);
		#endif
			if( tagha_asm.active_label==NULL ) {
				if( harbol_linkmap_has_key(&tagha_asm.opcodes, tagha_asm.lexeme.cstr) ) {
					_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "opcode '%s' outside of function!", tagha_asm.lexeme.cstr);
					goto tagha_asm_err;
				} else if( harbol_linkmap_has_key(&tagha_asm.funcs, tagha_asm.lexeme.cstr) ) {
					_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "redefinition of function '%s'", tagha_asm.lexeme.cstr);
					goto tagha_asm_err;
				}
				
			#ifdef TAGHA_ASM_DEBUG
				printf("func definition: %s\n", tagha_asm.lexeme.cstr);
			#endif
				/// found a function name.
				tagha_asm.active_label = &_active;
				harbol_string_copy_str(tagha_asm.active_label, &tagha_asm.lexeme);
				_tagha_asm_skip_delim(':');
				
				if( *tagha_asm.iter != '{' ) {
					_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "missing beginning curly bracket '{' for function '%s'", tagha_asm.active_label->cstr);
					goto tagha_asm_err;
				} else {
					tagha_asm.iter++;
					Label label = { EMPTY_HARBOL_BYTEBUF, tagha_asm.pc, true, false, false };
					harbol_linkmap_insert(&tagha_asm.funcs, tagha_asm.lexeme.cstr, &label);
				}
			} else if( !harbol_linkmap_has_key(&tagha_asm.opcodes, tagha_asm.lexeme.cstr) ) {
				_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "unknown opcode '%s'", tagha_asm.lexeme.cstr);
				goto tagha_asm_err;
			} else {
			#ifdef TAGHA_ASM_DEBUG
				printf("opcode definition: %s\n", tagha_asm.lexeme.cstr);
			#endif
				const uint8_t opcode = *( const uint8_t* )harbol_linkmap_key_get(&tagha_asm.opcodes, tagha_asm.lexeme.cstr);
				tagha_asm.pc += tagha_instr_gen(NULL, opcode);
				
				/// ignore opcode args until second pass.
				tagha_asm.iter = skip_single_line_comment(tagha_asm.iter);
			#ifdef TAGHA_ASM_DEBUG
				printf("tagha_asm.pc: %zu\n", tagha_asm.pc);
			#endif
			}
		} else if( *tagha_asm.iter=='.' ) {
			tagha_asm.iter++;
			if( !lex_id(tagha_asm.iter, &tagha_asm.iter, &tagha_asm.lexeme) ) {
				_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "bad label: '%s'", tagha_asm.lexeme.cstr);
				goto tagha_asm_err;
			} else {
				harbol_string_add_str(&tagha_asm.lexeme, tagha_asm.active_label);
				
				_tagha_asm_skip_delim(':');
				
				Label label = { EMPTY_HARBOL_BYTEBUF, tagha_asm.pc, false, false, false };
				harbol_linkmap_insert(&tagha_asm.labels, tagha_asm.lexeme.cstr, &label);
			#ifdef TAGHA_ASM_DEBUG
				printf("label: %s - tagha_asm.pc: %zu\n", tagha_asm.lexeme.cstr, tagha_asm.pc);
			#endif
			}
		} else {
			_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "unknown char: '%c'", *tagha_asm.iter);
			goto tagha_asm_err;
		}
	}
	
	if( tagha_asm.active_label != NULL ) {
		harbol_string_clear(tagha_asm.active_label);
		tagha_asm.active_label = NULL;
	}
	
#define X(x) #x ,
	/** for debugging purposes. */
	static const char *op_to_cstr[] = { TAGHA_INSTR_SET };
#undef X
	
#ifdef TAGHA_ASM_DEBUG
	puts("\nSecond Pass\n");
#endif
	/// Second Pass.
	tagha_asm.pc = 0;
	tagha_asm.line = 1;
	tagha_asm.iter = tagha_asm.src.cstr;
	while( *tagha_asm.iter != 0 ) {
		harbol_string_clear(&tagha_asm.lexeme);
		if( tagha_asm.err ) {
			goto tagha_asm_err;
		}
		_tagha_asm_skip_whitespace();
		
		if( *tagha_asm.iter=='\n' ) {
			tagha_asm.line++;
			tagha_asm.iter++;
			continue;
		} else if( *tagha_asm.iter==';' || *tagha_asm.iter=='$' ) {
			/// skip directives & comments.
			tagha_asm.iter = skip_single_line_comment(tagha_asm.iter);
			continue;
		} else if( !strncmp(tagha_asm.iter, "/*", sizeof "/*" - 1) ) {
			tagha_asm.iter = skip_multi_line_comment(tagha_asm.iter, "*/", sizeof "*/" - 1);
			continue;
		} else if( *tagha_asm.iter=='}' ) {
			tagha_asm.iter++;
		#ifdef TAGHA_ASM_DEBUG
			printf("end of func definition: %s\n", tagha_asm.active_label->cstr);
		#endif
			harbol_string_clear(tagha_asm.active_label);
			tagha_asm.active_label = NULL;
			tagha_asm.pc = 0;
		} else if( *tagha_asm.iter=='.' ) {
			/// skip labels, we already calc'd their address/offset.
			/// labels COULD be on the same line as an opcode, so be careful!
			tagha_asm.iter++;
			lex_id(tagha_asm.iter, &tagha_asm.iter, &tagha_asm.lexeme);
			_tagha_asm_skip_delim(':');
			continue;
		} else if( is_alphabetic(*tagha_asm.iter) ) {
			lex_id(tagha_asm.iter, &tagha_asm.iter, &tagha_asm.lexeme);
		#ifdef TAGHA_ASM_DEBUG
			printf("id definition: %s\n", tagha_asm.lexeme.cstr);
		#endif
			if( tagha_asm.active_label==NULL ) {
			#ifdef TAGHA_ASM_DEBUG
				printf("func definition: %s\n", tagha_asm.lexeme.cstr);
			#endif
				/// found a function name.
				tagha_asm.active_label = &_active;
				harbol_string_copy_str(tagha_asm.active_label, &tagha_asm.lexeme);
				_tagha_asm_skip_delim(':');
				tagha_asm.iter++; /// skip '{' which we verified in first pass.
			} else {
			#ifdef TAGHA_ASM_DEBUG
				printf("opcode definition: %s\n", tagha_asm.lexeme.cstr);
			#endif
				const uint8_t *const restrict opcode = harbol_linkmap_key_get(&tagha_asm.opcodes, tagha_asm.lexeme.cstr);
				if( opcode==NULL ) {
					_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "opcode pointer is NULL from key: '%s'", tagha_asm.lexeme.cstr);
					goto tagha_asm_err;
				}
				
				Label *const func = harbol_linkmap_key_get(&tagha_asm.funcs, tagha_asm.active_label->cstr);
				_tagha_asm_skip_whitespace();
				switch( *opcode ) {
					/// one uint8 immediate.
					case alloc: case redux: {
						uint64_t imm = 0;
						if( string_to_int(&tagha_asm.lexeme, ( int64_t* )&imm) && imm < 256 ) {
						#ifdef TAGHA_ASM_DEBUG
							printf("opcode value: %u\n", ( uint8_t )imm);
						#endif
							tagha_asm.pc += tagha_instr_gen(&func->data, *opcode, ( int )imm);
						} else {
							_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "opcode '%s' requires 0-255 number operand", op_to_cstr[*opcode]);
							goto tagha_asm_err;
						}
						break;
					}
					
					/// one uint8 register operand.
					case neg: case fneg:
					case bit_not: case setc:
					case callr:
					case f32tof64: case f64tof32:
					case itof64: case itof32:
					case f64toi: case f32toi: {
						uint64_t reg = 0;
						if( *tagha_asm.iter=='r' || *tagha_asm.iter=='R' )
							tagha_asm.iter++;
						
						if( string_to_int(&tagha_asm.lexeme, ( int64_t* )&reg) && reg < 256 ) {
						#ifdef TAGHA_ASM_DEBUG
							printf("opcode reg: r%u\n", ( uint8_t )reg);
						#endif
							tagha_asm.pc += tagha_instr_gen(&func->data, *opcode, ( int )reg);
						} else {
							_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "opcode '%s' requires a single r0-r255 register operand", op_to_cstr[*opcode]);
							goto tagha_asm_err;
						}
						break;
					}
					
					/// two uint8 register operands.
					case mov:
					case add: case sub: case mul: case idiv: case mod:
					case fadd: case fsub: case fmul: case fdiv:
					case bit_and: case bit_or: case bit_xor: case shl: case shr: case shar:
					case ilt: case ile: case ult: case ule: case cmp: case flt: case fle: {
						uint64_t
							reg1 = 0,
							reg2 = 0
						;
						if( *tagha_asm.iter=='r' || *tagha_asm.iter=='R' )
							tagha_asm.iter++;
						
						const bool res1 = string_to_int(&tagha_asm.lexeme, ( int64_t* )&reg1);
						_tagha_asm_skip_delim(',');
						
						if( *tagha_asm.iter=='r' || *tagha_asm.iter=='R' )
							tagha_asm.iter++;
						
						const bool res2 = string_to_int(&tagha_asm.lexeme, ( int64_t* )&reg2);
						
						if( res1 && res2 && reg1 < 256 && reg2 < 256 ) {
						#ifdef TAGHA_ASM_DEBUG
							printf("opcode reg: r%u, r%u\n", ( uint8_t )reg1, ( uint8_t )reg2);
						#endif
							tagha_asm.pc += tagha_instr_gen(&func->data, *opcode, ( int )reg1, ( int )reg2);
						} else {
							_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "opcode '%s' requires two r0-r255 register operands", op_to_cstr[*opcode]);
							goto tagha_asm_err;
						}
						break;
					}
					
					/// one uint8 register and uint16 imm operand.
					case lra: {
						uint64_t reg = 0;
						if( *tagha_asm.iter=='r' || *tagha_asm.iter=='R' )
							tagha_asm.iter++;
						
						const bool res1 = string_to_int(&tagha_asm.lexeme, ( int64_t* )&reg);
						if( res1 && reg < 256 ) {
							_tagha_asm_skip_delim(',');
							uint64_t offset = 0;
							const bool res2 = string_to_int(&tagha_asm.lexeme, ( int64_t* )&offset);
							if( res2 && offset<0xFFFF ) {
							#ifdef TAGHA_ASM_DEBUG
								printf("opcode value: r%u, %u\n", ( uint8_t )reg, ( uint16_t )offset);
							#endif
								tagha_asm.pc += tagha_instr_gen(&func->data, *opcode, ( int )reg, ( int )offset);
							} else {
								_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "opcode '%s' requires a 2-byte immediate as second operand", op_to_cstr[*opcode]);
								goto tagha_asm_err;
							}
						} else {
							_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "opcode '%s' requires a r0-r255 register as first operand", op_to_cstr[*opcode]);
							goto tagha_asm_err;
						}
						break;
					}
					
					/// one uint8 register and uint16 name value.
					case ldvar: case ldfn: {
						uint64_t reg = 0;
						if( *tagha_asm.iter=='r' || *tagha_asm.iter=='R' )
							tagha_asm.iter++;
						
						const bool res1 = string_to_int(&tagha_asm.lexeme, ( int64_t* )&reg);
						if( res1 && reg < 256 ) {
							_tagha_asm_skip_delim(',');
							const bool use_vars = *opcode==ldvar;
							if( lex_id(tagha_asm.iter, &tagha_asm.iter, &tagha_asm.lexeme) ) {
								if( !harbol_linkmap_has_key(use_vars ? &tagha_asm.vars : &tagha_asm.funcs, tagha_asm.lexeme.cstr) ) {
									_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "opcode '%s' %s value ('%s') doesn't exist", op_to_cstr[*opcode], use_vars ? "global var" : "func", tagha_asm.lexeme.cstr);
									goto tagha_asm_err;
								}
								const index_t id = harbol_linkmap_get_key_index(use_vars ? &tagha_asm.vars : &tagha_asm.funcs, tagha_asm.lexeme.cstr);
							#ifdef TAGHA_ASM_DEBUG
								printf("opcode value: r%u, %s (%u)\n", ( uint8_t )reg, tagha_asm.lexeme.cstr, ( uint16_t )id);
							#endif
								tagha_asm.pc += tagha_instr_gen(&func->data, *opcode, ( int )reg, ( int )id);
							} else {
								_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "opcode '%s' requires a %s name as second operand", op_to_cstr[*opcode], use_vars ? "global var" : "func");
								goto tagha_asm_err;
							}
						} else {
							_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "opcode '%s' requires a r0-r255 register as first operand", op_to_cstr[*opcode]);
							goto tagha_asm_err;
						}
						break;
					}
					
					/// uint16 name value.
					case call: {
						_tagha_asm_skip_whitespace();
						if( lex_id(tagha_asm.iter, &tagha_asm.iter, &tagha_asm.lexeme) ) {
							if( !harbol_linkmap_has_key(&tagha_asm.funcs, tagha_asm.lexeme.cstr) ) {
								_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "opcode '%s' function value ('%s') doesn't exist", op_to_cstr[*opcode], tagha_asm.lexeme.cstr);
								goto tagha_asm_err;
							}
							const index_t id = harbol_linkmap_get_key_index(&tagha_asm.funcs, tagha_asm.lexeme.cstr) + 1;
						#ifdef TAGHA_ASM_DEBUG
							printf("opcode value: %s (%u)\n", tagha_asm.lexeme.cstr, ( uint16_t )id);
						#endif
							tagha_asm.pc += tagha_instr_gen(&func->data, *opcode, ( int )id);
						} else {
							_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "opcode '%s' requires a func name as first operand", op_to_cstr[*opcode]);
							goto tagha_asm_err;
						}
						break;
					}
					
					/// two uint8 registers + int16 offset.  
					case lea:
					case ld1: case ld2: case ld4: case ld8: case ldu1: case ldu2: case ldu4: {
						uint64_t
							reg1 = 0,
							reg2 = 0
						;
						if( *tagha_asm.iter=='r' || *tagha_asm.iter=='R' )
							tagha_asm.iter++;
						
						const bool res1 = string_to_int(&tagha_asm.lexeme, ( int64_t* )&reg1);
						_tagha_asm_skip_delim(',');
						_tagha_asm_skip_delim('[');
						
						if( *tagha_asm.iter=='r' || *tagha_asm.iter=='R' )
							tagha_asm.iter++;
						
						const bool res2 = string_to_int(&tagha_asm.lexeme, ( int64_t* )&reg2);
					#ifdef TAGHA_ASM_DEBUG
						printf("opcode regs: r%u, [r%u]\n", ( uint8_t )reg1, ( uint8_t )reg2);
					#endif
						if( !res1 || !res2 || reg1 > 255 || reg2 > 255 ) {
							_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "opcode '%s' requires r0-r255 register first operand and r0-255 register + offset dereference second operand", op_to_cstr[*opcode]);
							goto tagha_asm_err;
						}
						_tagha_asm_skip_whitespace();
						int64_t offset = 0;
						if( *tagha_asm.iter=='+' || *tagha_asm.iter=='-' ) {
							const bool neg = *tagha_asm.iter=='-';
							tagha_asm.iter++;
							tagha_asm.iter = skip_chars_until_newline(tagha_asm.iter, is_whitespace);
							if( !is_decimal(*tagha_asm.iter) ) {
								_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "invalid dereference offset at opcode '%s'", op_to_cstr[*opcode]);
								goto tagha_asm_err;
							}
							string_to_int(&tagha_asm.lexeme, &offset);
							if( neg )
								offset = -offset;
						}
						_tagha_asm_skip_delim(']');
						
					#ifdef TAGHA_ASM_DEBUG
						printf("opcode reg: r%u, [r%u%s%u]\n", ( uint8_t )reg1, ( uint8_t )reg2, offset<0 ? "-" : "+", ( int16_t )offset);
					#endif
						tagha_asm.pc += tagha_instr_gen(&func->data, *opcode, ( int )reg1, ( int )reg2, ( int )offset);
						break;
					}
					
					case st1: case st2: case st4: case st8: {
						uint64_t
							reg1 = 0,
							reg2 = 0
						;
						
						_tagha_asm_skip_delim('[');
						if( *tagha_asm.iter=='r' || *tagha_asm.iter=='R' )
							tagha_asm.iter++;
						
						const bool res1 = string_to_int(&tagha_asm.lexeme, ( int64_t* )&reg1);
						if( !res1 || reg1 > 255 ) {
							_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "opcode '%s' requires r0-255 register + offset dereference first operand and r0-r255 register second operand", op_to_cstr[*opcode]);
							goto tagha_asm_err;
						}
						int64_t offset = 0;
						if( *tagha_asm.iter=='+' || *tagha_asm.iter=='-' ) {
							const bool neg = *tagha_asm.iter=='-';
							tagha_asm.iter++;
							tagha_asm.iter = skip_chars_until_newline(tagha_asm.iter, is_whitespace);
							if( !is_decimal(*tagha_asm.iter) ) {
								_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "invalid dereference offset at opcode '%s'", op_to_cstr[*opcode]);
								goto tagha_asm_err;
							}
							string_to_int(&tagha_asm.lexeme, &offset);
							if( neg )
								offset = -offset;
						}
						_tagha_asm_skip_delim(']');
						
						_tagha_asm_skip_delim(',');
						
						if( *tagha_asm.iter=='r' || *tagha_asm.iter=='R' )
							tagha_asm.iter++;
						
						const bool res2 = string_to_int(&tagha_asm.lexeme, ( int64_t* )&reg2);
					#ifdef TAGHA_ASM_DEBUG
						printf("opcode regs: [r%u], r%u\n", ( uint8_t )reg1, ( uint8_t )reg2);
					#endif
						if( !res2 || reg2 > 255 ) {
							_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "opcode '%s' requires r0-255 register + offset dereference first operand and r0-r255 register second operand", op_to_cstr[*opcode]);
							goto tagha_asm_err;
						} else if( !is_whitespace(*tagha_asm.iter) ) {
							_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "opcode '%s' has too many items", op_to_cstr[*opcode]);
							goto tagha_asm_err;
						}
						_tagha_asm_skip_whitespace();
						
					#ifdef TAGHA_ASM_DEBUG
						printf("opcode reg: [r%u%s%u], r%u\n", ( uint8_t )reg1, offset<0 ? "-" : "+", ( int16_t )offset, ( uint8_t )reg2);
					#endif
						tagha_asm.pc += tagha_instr_gen(&func->data, *opcode, ( int )reg1, ( int )reg2, ( int )offset);
						break;
					}
					
					/// int32 offset.
					case jmp: case jz: case jnz: {
						_tagha_asm_skip_whitespace();
						if( *tagha_asm.iter != '.' ) {
							_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "opcode '%s' requires a label value (format: '.mylabel:'", op_to_cstr[*opcode]);
							goto tagha_asm_err;
						} else {
							tagha_asm.iter++;
							if( lex_id(tagha_asm.iter, &tagha_asm.iter, &tagha_asm.lexeme) ) {
								harbol_string_add_str(&tagha_asm.lexeme, tagha_asm.active_label);
								if( !harbol_linkmap_has_key(&tagha_asm.labels, tagha_asm.lexeme.cstr) ) {
									_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "opcode '%s' label ('%s') doesn't exist", op_to_cstr[*opcode], tagha_asm.lexeme.cstr);
									goto tagha_asm_err;
								}
								Label *const label = harbol_linkmap_key_get(&tagha_asm.labels, tagha_asm.lexeme.cstr);
							#ifdef TAGHA_ASM_DEBUG
								printf("label value: %s (%i)\n", tagha_asm.lexeme.cstr, ( int32_t )label->offset);
							#endif
								tagha_asm.pc += tagha_instr_gen(NULL, *opcode);
								const int32_t offs = ( ssize_t )label->offset - ( ssize_t )tagha_asm.pc;
								tagha_instr_gen(&func->data, *opcode, offs);
							} else {
								_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "opcode '%s' requires a label name as first operand", op_to_cstr[*opcode]);
								goto tagha_asm_err;
							}
						}
						break;
					}
					case movi: {
						_tagha_asm_skip_whitespace();
						uint64_t reg = 0;
						if( *tagha_asm.iter=='r' || *tagha_asm.iter=='R' )
							tagha_asm.iter++;
						
						if( !string_to_int(&tagha_asm.lexeme, ( int64_t* )&reg) || reg > 256 ) {
							_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "opcode '%s' requires r0-r255 register as first operand", op_to_cstr[*opcode]);
							goto tagha_asm_err;
						} else {
							_tagha_asm_skip_delim(',');
							union TaghaVal val = {0};
							if( !string_to_int(&tagha_asm.lexeme, &val.int64) ) {
								_tagha_asm_err(tagha_asm.outfile.cstr, "error", tagha_asm.line, 0, "invalid immediate value ('%s') in second operand of opcode '%s'", op_to_cstr[*opcode], tagha_asm.lexeme.cstr);
								goto tagha_asm_err;
							} else {
								tagha_asm.pc += tagha_instr_gen(&func->data, *opcode, ( int )reg, val);
							}
						}
						break;
					}
					
					/// no operands
					case ret: case halt: case nop: case pushlr: case poplr: {
						tagha_asm.pc += tagha_instr_gen(&func->data, *opcode);
					}
					default: break;
				}
				
			#ifdef TAGHA_ASM_DEBUG
				printf("tagha_asm.pc: %zu\n", tagha_asm.pc);
			#endif
			}
			continue;
		}
	}
	
	enum {
		TAGHA_ASM_DEFAULT_MEMSIZE = 0x1000
	};
	
	if( tagha_asm.callstacksize == 0 )
		tagha_asm.callstacksize = TAGHA_ASM_DEFAULT_MEMSIZE;
	if( tagha_asm.opstacksize == 0 )
		tagha_asm.opstacksize = TAGHA_ASM_DEFAULT_MEMSIZE;
	if( tagha_asm.heapsize == 0 )
		tagha_asm.heapsize = TAGHA_ASM_DEFAULT_MEMSIZE;
	
	/// calculate extra heap size for internal tagha data.
	const bool is_64_bit = sizeof(intptr_t)==8;
	const size_t tagha_ptr_size = (is_64_bit) ? sizeof(intptr_t) : sizeof(intptr_t) << 1;
	const size_t memnode_size = harbol_align_size(sizeof(struct HarbolMemNode), tagha_ptr_size);
	
	const struct TaghaSymTable syms = {0};
	const size_t tagha_sym_arr_size = harbol_align_size(sizeof *syms.table, tagha_ptr_size);
	
	uint32_t mem_region_size = memnode_size + tagha_ptr_size * 2;
	
	mem_region_size += (tagha_sym_arr_size * tagha_asm.funcs.map.count + memnode_size);
	mem_region_size += ((tagha_ptr_size * tagha_asm.funcs.map.count * 3) + (memnode_size * 3));
	
	mem_region_size += (tagha_sym_arr_size * tagha_asm.vars.map.count + memnode_size);
	mem_region_size += ((tagha_ptr_size * tagha_asm.vars.map.count * 3) + (memnode_size * 3));
	
	struct TaghaModGen modgen = tagha_mod_gen_create();
	tagha_mod_gen_write_header(&modgen, tagha_asm.opstacksize, tagha_asm.callstacksize, tagha_asm.heapsize+ ( uint32_t )harbol_align_size(mem_region_size, 8), 0);
	
	for( size_t i=0; i<tagha_asm.funcs.map.count; i++ ) {
		struct HarbolKeyVal *node = harbol_linkmap_index_get_kv(&tagha_asm.funcs, i);
		Label *const label = harbol_linkmap_index_get(&tagha_asm.funcs, i);
		if( label==NULL )
			continue;
		
		uint32_t flags = 0;
		if( label->is_native )
			flags |= TAGHA_FLAG_NATIVE;
		if( label->is_extern )
			flags |= TAGHA_FLAG_EXTERN;
		tagha_mod_gen_write_func(&modgen, flags, node->key.cstr, &label->data);
		
	#ifdef TAGHA_ASM_DEBUG
		printf("func label: %s\nData:\n", node->key.cstr);
		if( !label->is_native )
			for( size_t i=0; i<label->data.count; i++ )
				printf("bytecode[%zu] == %u\n", i, label->data.table[i]);
		puts("\n");
	#endif
	}
	
	for( size_t i=0; i<tagha_asm.vars.map.count; i++ ) {
		struct HarbolKeyVal *const node = harbol_linkmap_index_get_kv(&tagha_asm.vars, i);
		struct HarbolByteBuf *const bytedata = harbol_linkmap_index_get(&tagha_asm.vars, i);
		if( bytedata==NULL )
			continue;
		
		tagha_mod_gen_write_var(&modgen, 0, node->key.cstr, bytedata);
		
	#ifdef TAGHA_ASM_DEBUG
		printf("global var: %s\nData:\n", node->key.cstr);
		for( size_t i=0; i<bytedata->count; i++ )
			printf("bytecode[%zu] == %u\n", i, bytedata->table[i]);
		puts("\n");
	#endif
	}
	
	{
		char *iter = tagha_asm.outfile.cstr;
		size_t len = 0;
		while( *++iter );
		while( tagha_asm.outfile.cstr<iter && *--iter != '.' )
			++len;
		
		memset(iter, 0, len);
		harbol_string_add_cstr(&tagha_asm.outfile, ".tbc");
	}
	
	printf("Tagha Assembler: file '%s' %s\n", tagha_asm.outfile.cstr, tagha_mod_gen_create_file(&modgen, tagha_asm.outfile.cstr) ? "successfully generated" : "generation failed");
	
#ifdef TAGHA_ASM_DEBUG
	puts("tagha_asm_assemble end");
#endif
tagha_asm_err:
	_tagha_asm_clear_data();
}


NO_NULL int main(const int argc, char *argv[restrict static 1])
{
	if( argc<=1 ) {
		fprintf(stderr, "Tagha Assembler - usage: %s [.tasm file...]\n", argv[0]);
		return 1;
	} else if( !strcmp(argv[1], "--help") ) {
		puts("Tagha Assembler - Tagha Runtime Environment Toolkit\nTo compile a tasm script to tbc, supply a script name as a command-line argument to the program.\nExample: './tagha_asm [options] script.tasm'");
	} else if( !strcmp(argv[1], "--version") ) {
		puts("Tagha Assembler Version 1.0.0");
	} else {
		for( int i=1; i<argc; i++ ) {
			FILE *restrict tasmfile = fopen(argv[i], "r");
			if( tasmfile==NULL )
				continue;
			else if( harbol_string_read_file(&tagha_asm.src, tasmfile) ) {
			#ifdef TAGHA_ASM_DEBUG
				printf("read file called: '%s'\n", argv[i]);
			#endif
				fclose(tasmfile), tasmfile=NULL;
				tagha_asm.outfile = harbol_string_create(argv[i]);
				_tagha_asm_setup_opcodes();
				tagha_asm_assemble();
				_tagha_asm_zero_out();
			}
		}
	}
}