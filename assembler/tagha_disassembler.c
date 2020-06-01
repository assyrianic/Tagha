#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "libharbol/harbol.h"
#include "../tagha/tagha.h"


/// Idea: bytecode instruction to string function?

bool tagha_disasm_module(const char filename[restrict static 1])
{
	uint8_t *filedata = make_buffer_from_binary(filename);
	if( filedata==NULL ) {
		fprintf(stderr, "Tagha Disassembler Error: **** Couldn't load Tagha Module file: '%s' ****\n", filename);
		return false;
	}
	
	const struct TaghaHeader *const hdr = ( const struct TaghaHeader* )filedata;
	if( hdr->magic != 0x7A6AC0DE ) {
		free(filedata), filedata=NULL;
		fprintf(stderr, "Tagha Disassembler Error: **** Invalid Tagha Module file: '%s' ****\n", filename);
		return false;
	}
	
	union HarbolBinIter iter = { .uint8 = filedata + sizeof *hdr };
	struct HarbolString
		header    = harbol_string_create(NULL),
		nbc_funcs = harbol_string_create(NULL), /// NBC "Non-ByteCode" functions.
		vars      = harbol_string_create(NULL),
		bc_funcs  = harbol_string_create(NULL)
	;
	struct HarbolVector
		func_names = harbol_vector_create(sizeof(struct HarbolString), 0),
		var_names  = harbol_vector_create(sizeof(struct HarbolString), 0)
	;
	
#define X(x) #x ,
	const char *opcode_strs[] = { TAGHA_INSTR_SET };
#undef X
	
#define Y(y) #y ,
	const char *reg_strs[] = { TAGHA_REG_FILE };
#undef Y
	
	/// collect header info.
	harbol_string_add_format(&header, "; %s disassembled by the official tagha disassembler.\n", filename);
	harbol_string_add_format(&header, "$stacksize    %d\n", hdr->stacksize);
	harbol_string_add_format(&header, "$heapsize     %d\n\n", hdr->memsize - hdr->stacksize);
	
	const uint32_t func_table_size = *iter.uint32++;
	{
		union HarbolBinIter first_run = iter;
		harbol_string_add_format(&header, "; function count: %u\n", func_table_size);
		for( uint32_t i=0; i<func_table_size; i++ ) {
			const struct TaghaItemEntry *const entry = first_run.ptr;
			first_run.uint8 += sizeof *entry;
			struct HarbolString s = harbol_string_create(first_run.string);
			harbol_vector_insert(&func_names, &s);
			first_run.uint8 += entry->name_len;
			if( !entry->flags )
				first_run.uint8 += entry->data_len;
		}
		
		const uint32_t var_table_size = *first_run.uint32++;
		harbol_string_add_format(&header, "; global var count: %u\n\n", var_table_size);
		for( uint32_t i=0; i<var_table_size; i++ ) {
			const struct TaghaItemEntry *const entry = first_run.ptr;
			first_run.uint8 += sizeof *entry;
			struct HarbolString s = harbol_string_create(first_run.string);
			harbol_vector_insert(&var_names, &s);
			first_run.uint8 += entry->name_len + entry->data_len;
		}
	}
	
	for( uint32_t i=0; i<func_table_size; i++ ) {
		const struct TaghaItemEntry *const entry = iter.ptr;
		iter.uint8 += sizeof *entry;
		const bool is_bytecode = entry->flags==0;
		if( is_bytecode ) {
			harbol_string_add_format(&bc_funcs, "; bytecode len: %u\n%%%s: {\n", entry->data_len, iter.string);
			iter.uint8 += entry->name_len;
			union HarbolBinIter pc = iter;
			iter.uint8 += entry->data_len;
			uintptr_t offs = ( uintptr_t )pc.uint8 + 1;
			while( *pc.uint8 != 0 && pc.uint8<iter.uint8 ) {
				const uint8_t opcode = *pc.uint8++;
				switch( opcode ) {
					/// opcodes that have no operands.
					case halt: case nop: case ret: {
						harbol_string_add_format(&bc_funcs, "    %-10s ; offset: %" PRIuPTR "\n", opcode_strs[opcode], ( uintptr_t )pc.uint8 - offs);
						break;
					}
					
					/// opcodes that take an immediate value.
					case jmp: case jz: case jnz: case call: {
						const uintptr_t addr = ( uintptr_t )pc.uint8 - offs;
						const int64_t label = *pc.int64++;
						const uintptr_t addr2 = ( uintptr_t )pc.uint8 - offs;
						
						if( opcode==call ) {
							const struct HarbolString *func_name = harbol_vector_get(&func_names, label < 0LL ? -1LL - label : label - 1LL);
							harbol_string_add_format(&bc_funcs, "    %-10s %%%s ; offset: %" PRIuPTR " - %" PRIuPTR "\n", opcode_strs[opcode], func_name->cstr, addr, addr2);
						} else {
							const uintptr_t label_addr = ( uintptr_t )(( int64_t )addr2 + label + 1);
							harbol_string_add_format(&bc_funcs, "    %-10s %" PRIi64 " ; label addr: %" PRIuPTR " | offset: %" PRIuPTR " - %" PRIuPTR "\n", opcode_strs[opcode], label, label_addr, addr, addr2);
						}
						break;
					}
					/// opcodes that only take a register operand.
					case push: case pop: case bit_not: case neg: case callr:
					case f32tof64: case f64tof32: case itof64: case itof32: case f64toi: case f32toi: case negf: {
						const uintptr_t addr = ( uintptr_t )pc.uint8 - offs;
						const uint8_t regid = *pc.uint8++;
						const uintptr_t addr2 = ( uintptr_t )pc.uint8 - offs;
						
						harbol_string_add_format(&bc_funcs, "    %-10s r%s ; offset: %" PRIuPTR " - %" PRIuPTR "\n", opcode_strs[opcode], reg_strs[regid], addr, addr2);
						break;
					}
					
					/// opcodes that take a register and an immediate value.
					case ldvar: {
						const uintptr_t addr = ( uintptr_t )pc.uint8 - offs;
						const uint8_t regid = *pc.uint8++;
						const int64_t label = *pc.int64++;
						const uintptr_t addr2 = ( uintptr_t )pc.uint8 - offs;
						const struct HarbolString *var_name = harbol_vector_get(&var_names, label);
						
						harbol_string_add_format(&bc_funcs, "    %-10s r%s, %s ; offset: %" PRIuPTR " - %" PRIuPTR "\n", opcode_strs[opcode], reg_strs[regid], var_name->cstr, addr, addr2);
						break;
					}
					case ldfunc: {
						const uintptr_t addr = ( uintptr_t )pc.uint8 - offs;
						const uint8_t regid = *pc.uint8++;
						const int64_t label = *pc.int64++;
						const uintptr_t addr2 = ( uintptr_t )pc.uint8 - offs;
						const struct HarbolString *funcld_name = harbol_vector_get(&func_names, label < 0LL ? -1LL - label : label - 1LL);
						if( funcld_name==NULL ) {
							fprintf(stderr, "Tagha Disassembler Error: **** name for label %" PRIi64 " doesn't exist! ****\n", label);
						} else {
							harbol_string_add_format(&bc_funcs, "    %-10s r%s, %%%s ; offset: %" PRIuPTR " - %" PRIuPTR "\n", opcode_strs[opcode], reg_strs[regid], funcld_name->cstr, addr, addr2);
						}
						break;
					}
					case movi: {
						const uintptr_t addr = ( uintptr_t )pc.uint8 - offs;
						const uint8_t regid = *pc.uint8++;
						const int64_t label = *pc.int64++;
						const uintptr_t addr2 = ( uintptr_t )pc.uint8 - offs;
						
						harbol_string_add_format(&bc_funcs, "    %-10s r%s, %#" PRIx64 " ; offset: %" PRIuPTR " - %" PRIuPTR "\n", opcode_strs[opcode], reg_strs[regid], label, addr, addr2);
						break;
					}
					
					/// opcodes that take two registers as a load memory op.
					case ldaddr: case ld1: case ld2: case ld4: case ld8: case lds1: case lds2: case lds4: {
						const uintptr_t addr = ( uintptr_t )pc.uint8 - offs;
						const uint8_t regid1 = *pc.uint8++;
						const uint8_t regid2 = *pc.uint8++;
						const int32_t offset = *pc.int32++;
						const uintptr_t addr2 = ( uintptr_t )pc.uint8 - offs;
						const bool    neg    = offset < 0;
						
						harbol_string_add_format(&bc_funcs, "    %-10s r%s, [r%s%s%d] ; offset: %" PRIuPTR " - %" PRIuPTR "\n", opcode_strs[opcode], reg_strs[regid1], reg_strs[regid2], !neg ? "+" : "", offset, addr, addr2);
						break;
					}
					
					/// opcodes that take two registers as a store memory op
					case st1: case st2: case st4: case st8: {
						const uintptr_t addr = ( uintptr_t )pc.uint8 - offs;
						const uint8_t regid1 = *pc.uint8++;
						const uint8_t regid2 = *pc.uint8++;
						const int32_t offset = *pc.int32++;
						const uintptr_t addr2 = ( uintptr_t )pc.uint8 - offs;
						const bool    neg    = offset < 0;
						
						harbol_string_add_format(&bc_funcs, "    %-10s [r%s%s%d], r%s ; offset: %" PRIuPTR " - %" PRIuPTR "\n", opcode_strs[opcode], reg_strs[regid1], !neg ? "+" : "", offset, reg_strs[regid2], addr, addr2);
						break;
					}
					
					case mov:
					case add: case sub: case mul: case divi: case mod:
					case bit_and: case bit_or: case bit_xor: case shl: case shr: case shar:
					case ilt: case ile: case ult: case ule: case cmp:
					case addf: case subf: case mulf: case divf:
					case ltf: case lef: {
						const uintptr_t addr = ( uintptr_t )pc.uint8 - offs;
						const uint8_t regid1 = *pc.uint8++;
						const uint8_t regid2 = *pc.uint8++;
						const uintptr_t addr2 = ( uintptr_t )pc.uint8 - offs;
						
						harbol_string_add_format(&bc_funcs, "    %-10s r%s, r%s ; offset: %" PRIuPTR " - %" PRIuPTR "\n", opcode_strs[opcode], reg_strs[regid1], reg_strs[regid2], addr, addr2);
						break;
					}
					default: break;
				}
			}
			harbol_string_add_format(&bc_funcs, "}\n\n");
		} else {
			if( entry->flags & TAGHA_FLAG_NATIVE )
				harbol_string_add_format(&nbc_funcs, "$native    %%%s\n", iter.string);
			else if( entry->flags & TAGHA_FLAG_EXTERN )
				harbol_string_add_format(&nbc_funcs, "$extern    %%%s\n", iter.string);
			iter.uint8 += entry->name_len;
		}
	}
	harbol_string_add_cstr(&nbc_funcs, "\n");
	
	/// iterate var table and get bytecode sizes.
	const uint32_t var_table_size = *iter.uint32++;
	for( uint32_t i=0; i<var_table_size; i++ ) {
		const struct TaghaItemEntry *const entry = iter.ptr;
		iter.uint8 += sizeof *entry;
		uint32_t bytes = entry->data_len;
		harbol_string_add_format(&vars, "$global    %-25s, %u", iter.string, entry->data_len);
		iter.uint8 += entry->name_len;
		union HarbolBinIter data = iter;
		iter.uint8 += entry->data_len;
		for( uint32_t n=0; n<bytes / sizeof(union TaghaVal); n++ ) {
			harbol_string_add_format(&vars, ", word %#" PRIx64 "", *data.uint64++);
			bytes -= sizeof(union TaghaVal);
		}
		switch( bytes ) {
			case 4:
				harbol_string_add_format(&vars, ", long %#x", *data.uint32++);
				bytes -= 4;
				break;
			case 2:
				harbol_string_add_format(&vars, ", half %#x", *data.uint16++);
				bytes -= 2;
				break;
			case 1:
				harbol_string_add_format(&vars, ", byte %#x", *data.uint8++);
				bytes -= 1;
				break;
		}
		harbol_string_add_format(&vars, "\n");
	}
	harbol_string_add_cstr(&vars, "\n");
	free(filedata);
	
	for( size_t i=0; i<func_names.count; i++ ) {
		struct HarbolString *name = harbol_vector_get(&func_names, i);
		harbol_string_clear(name);
	}
	harbol_vector_clear(&func_names, NULL);
	
	for( size_t i=0; i<var_names.count; i++ ) {
		struct HarbolString *name = harbol_vector_get(&var_names,  i);
		harbol_string_clear(name);
	}
	harbol_vector_clear(&var_names,  NULL);
	
	struct HarbolString
		final_output = harbol_string_create(NULL),
		output_name  = harbol_string_create(filename)
	;
	harbol_string_add_str(&final_output, &header); harbol_string_clear(&header);
	harbol_string_add_str(&final_output, &vars);   harbol_string_clear(&vars);
	harbol_string_add_str(&final_output, &nbc_funcs); harbol_string_clear(&nbc_funcs);
	harbol_string_add_str(&final_output, &bc_funcs); harbol_string_clear(&bc_funcs);
	
	harbol_string_add_cstr(&output_name, "_disasm.tasm");
	FILE *disasm_output = fopen(output_name.cstr, "w");
	harbol_string_clear(&output_name);
	if( disasm_output==NULL ) {
		fprintf(stderr, "Tagha Disassembler Error: **** Failed to produce output disassembly. ****\n");
		return false;
	}
	fwrite(final_output.cstr, sizeof *final_output.cstr, final_output.len, disasm_output);
	harbol_string_clear(&final_output);
	fclose(disasm_output), disasm_output=NULL;
	return true;
}

static void tagha_disasm_parse_opts(const char arg[static 1])
{
	(void)arg;
}

int main(const int argc, char *argv[restrict static 1])
{
	if( argc<=1 ) {
		fprintf(stderr, "Tagha Disassembler - usage: %s [.tbc file...]\n", argv[0]);
		return -1;
	} else if( !strcmp(argv[1], "--help") ) {
		puts("Tagha Disassembler - Tagha Runtime Environment Toolkit\nTo decompile a tbc script to tasm, supply a script name as a command-line argument to the program.\nExample: './tagha_disasm [options] script.tbc'");
	} else if( !strcmp(argv[1], "--version") ) {
		puts("Tagha Disassembler Version 1.0.1");
	} else {
		for( int i=1; i<argc; i++ ) {
			if( argv[i][0]=='-' ) {
				tagha_disasm_parse_opts(argv[i]);
			} else {
				tagha_disasm_module(argv[i]);
			}
		}
	}
}
