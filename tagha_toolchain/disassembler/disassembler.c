#include "../libharbol/harbol.h"
#include "../../tagha/tagha.h"


bool tagha_disasm_module(const char filename[restrict static 1])
{
	uint8_t *filedata = make_buffer_from_binary(filename);
	if( filedata==NULL ) {
		fprintf(stderr, "Tagha Disassembler Error: **** Couldn't load Tagha Module file: '%s' ****\n", filename);
		return false;
	}
	
	const struct TaghaModuleHeader *const hdr = ( const struct TaghaModuleHeader* )(filedata);
	if( hdr->magic != TAGHA_MAGIC_VERIFIER ) {
		free(filedata); filedata=NULL;
		fprintf(stderr, "Tagha Disassembler Error: **** Invalid Tagha Module file: '%s' ****\n", filename);
		return false;
	}
	
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
	const char *restrict opcode_strs[] = { TAGHA_INSTR_SET };
#undef X
	
	/// collect header info.
	harbol_string_add_format(&header, ";; '%s' disassembled by the official tagha disassembler.\n", filename);
	harbol_string_add_format(&header, "$opstack_size          %d\n", hdr->opstacksize / sizeof(union TaghaVal));
	harbol_string_add_format(&header, "$callstack_size        %d\n", hdr->callstacksize / sizeof(uintptr_t));
	harbol_string_add_format(&header, ";; total stacks size   '%d'\n\n", hdr->stacksize);
	harbol_string_add_format(&header, "$heap_size     %d\n", hdr->heapsize);
	harbol_string_add_format(&header, ";; total memory usage: '%d' bytes\n\n", hdr->memsize);
	
	const uint32_t func_table_size = hdr->func_count;
	const uint32_t var_table_size  = hdr->var_count;
	harbol_string_add_format(&header, ";; function count: %u\n", func_table_size);
	harbol_string_add_format(&header, ";; global var count: %u\n\n", var_table_size);
	union HarbolBinIter iter = { .uint8 = filedata + hdr->funcs_offset };
	{
		union HarbolBinIter first_run = iter;
		for( uint32_t i=0; i<func_table_size; i++ ) {
			const struct TaghaItemEntry *const entry = first_run.ptr;
			first_run.uint8 += sizeof *entry;
			struct HarbolString s = harbol_string_create(first_run.string);
			harbol_vector_insert(&func_names, &s);
			first_run.uint8 += entry->name_len;
			if( !entry->flags )
				first_run.uint8 += entry->data_len;
		}
		
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
			harbol_string_add_format(&bc_funcs, ";; bytecode len: %u\n%s: {\n", entry->data_len, iter.string);
			iter.uint8 += entry->name_len;
			union HarbolBinIter pc = iter;
			iter.uint8 += entry->data_len;
			const uintptr_t offs = ( uintptr_t )(pc.uint8) + 1;
			while( *pc.uint8 != 0 && pc.uint8<iter.uint8 ) {
				const uint8_t opcode = *pc.uint8++;
				switch( opcode ) {
					/// opcodes that have no operands.
					case ret: case halt: case nop: case pushlr: case poplr: case restore: {
						harbol_string_add_format(&bc_funcs, "    %-10s ;; offset: %" PRIuPTR "\n", opcode_strs[opcode], ( uintptr_t )(pc.uint8) - offs);
						break;
					}
					
					/// u8 operand.
					case alloc: case redux: case setelen: case leave: case remit: case enter: {
						const uintptr_t addr = ( uintptr_t )(pc.uint8) - offs;
						const uint32_t u8 = *pc.uint8++;
						const uintptr_t addr2 = ( uintptr_t )(pc.uint8) - offs;
						harbol_string_add_format(&bc_funcs, "    %-10s %u ;; offset: %" PRIuPTR " - %" PRIuPTR "\n", opcode_strs[opcode], u8, addr, addr2);
						break;
					}
					
					/// u8 register operand.
					case neg: case fneg:
					case _not: case setc:
					case callr:
					case f32tof64: case f64tof32:
					case itof64: case itof32:
					case f64toi: case f32toi:
					case vneg: case vfneg:
					case vnot: {
						const uintptr_t addr = ( uintptr_t )(pc.uint8) - offs;
						const uint32_t reg = *pc.uint8++;
						const uintptr_t addr2 = ( uintptr_t )(pc.uint8) - offs;
						harbol_string_add_format(&bc_funcs, "    %-10s r%u ;; offset: %" PRIuPTR " - %" PRIuPTR "\n", opcode_strs[opcode], reg, addr, addr2);
						break;
					}
					
					/// two u8 register operands.
					case mov:
					case add: case sub: case mul: case idiv: case mod:
					case fadd: case fsub: case fmul: case fdiv:
					case _and: case _or: case _xor: case sll: case srl: case sra:
					case ilt: case ile: case ult: case ule: case cmp: case flt: case fle:
					case vmov:
					case vadd: case vsub: case vmul: case vdiv: case vmod:
					case vfadd: case vfsub: case vfmul: case vfdiv:
					case vand: case vor: case vxor: case vsll: case vsrl: case vsra:
					case vcmp: case vilt: case vile: case vult: case vule: case vflt: case vfle: {
						const uintptr_t addr = ( uintptr_t )(pc.uint8) - offs;
						const uint32_t dst = *pc.uint8++;
						const uint32_t src = *pc.uint8++;
						const uintptr_t addr2 = ( uintptr_t )(pc.uint8) - offs;
						
						harbol_string_add_format(&bc_funcs, "    %-10s r%u, r%u ;; offset: %" PRIuPTR " - %" PRIuPTR "\n", opcode_strs[opcode], dst, src, addr, addr2);
						break;
					}
					
					/// named u16 operand.
					case call: {
						const uintptr_t addr = ( uintptr_t )(pc.uint8) - offs;
						const uint32_t index = *pc.uint16++;
						const uintptr_t addr2 = ( uintptr_t )(pc.uint8) - offs;
						
						const struct HarbolString *const func_name = harbol_vector_get(&func_names, index - 1LL);
						harbol_string_add_format(&bc_funcs, "    %-10s %s ;; offset: %" PRIuPTR " - %" PRIuPTR "\n", opcode_strs[opcode], func_name->cstr, addr, addr2);
						break;
					}
					
					/// u16 imm
					case setvlen: {
						const uintptr_t addr = ( uintptr_t )(pc.uint8) - offs;
						const uint32_t width = *pc.uint16++;
						const uintptr_t addr2 = ( uintptr_t )(pc.uint8) - offs;
						harbol_string_add_format(&bc_funcs, "    %-10s %u ;; offset: %" PRIuPTR " - %" PRIuPTR "\n", opcode_strs[opcode], width, addr, addr2);
						break;
					}
					
					/// u8 reg + u16 offset.
					case lra: {
						const uintptr_t addr = ( uintptr_t )(pc.uint8) - offs;
						const uint32_t reg = *pc.uint8++;
						const uint32_t offset = *pc.uint16++;
						const uintptr_t addr2 = ( uintptr_t )(pc.uint8) - offs;
						harbol_string_add_format(&bc_funcs, "    %-10s r%u, %u ;; offset: %" PRIuPTR " - %" PRIuPTR "\n", opcode_strs[opcode], reg, offset, addr, addr2);
						break;
					}
					
					/// u8 reg + named u16 offset.
					case ldvar: case ldfn: {
						const uintptr_t addr = ( uintptr_t )(pc.uint8) - offs;
						const uint32_t regid = *pc.uint8++;
						const uint32_t label = *pc.uint16++;
						const uintptr_t addr2 = ( uintptr_t )(pc.uint8) - offs;
						const struct HarbolString *const label_name = harbol_vector_get((opcode==ldvar) ? &var_names : &func_names, (opcode==ldvar) ? label : label - 1);
						
						harbol_string_add_format(&bc_funcs, "    %-10s r%u, %s ;; offset: %" PRIuPTR " - %" PRIuPTR "\n", opcode_strs[opcode], regid, label_name->cstr, addr, addr2);
						break;
					}
					
					case lea:
					case ld1: case ld2: case ld4: case ld8: case ldu1: case ldu2: case ldu4: {
						const uintptr_t addr = ( uintptr_t )(pc.uint8) - offs;
						const uint32_t dst = *pc.uint8++;
						const uint32_t src = *pc.uint8++;
						const int32_t offset = *pc.int16++;
						const uintptr_t addr2 = ( uintptr_t )(pc.uint8) - offs;
						const bool neg    = offset < 0;
						
						harbol_string_add_format(&bc_funcs, "    %-10s r%u, [r%u%s%d] ;; offset: %" PRIuPTR " - %" PRIuPTR "\n", opcode_strs[opcode], dst, src, !neg ? "+" : "", offset, addr, addr2);
						break;
					}
					
					case st1: case st2: case st4: case st8: {
						const uintptr_t addr = ( uintptr_t )(pc.uint8) - offs;
						const uint32_t dst = *pc.uint8++;
						const uint32_t src = *pc.uint8++;
						const int32_t offset = *pc.int16++;
						const uintptr_t addr2 = ( uintptr_t )(pc.uint8) - offs;
						const bool neg    = offset < 0;
						
						harbol_string_add_format(&bc_funcs, "    %-10s [r%u%s%d], r%u ;; offset: %" PRIuPTR " - %" PRIuPTR "\n", opcode_strs[opcode], dst, !neg ? "+" : "", offset, src, addr, addr2);
						break;
					}
					case jmp: case jz: case jnz: {
						const uintptr_t addr = ( uintptr_t )(pc.uint8) - offs;
						const int32_t label = *pc.int32++;
						const uintptr_t addr2 = ( uintptr_t )(pc.uint8) - offs;
						
						const uintptr_t label_addr = ( uintptr_t )(( int32_t )addr2 + label + 1);
						harbol_string_add_format(&bc_funcs, "    %-10s %d ;; label addr: %" PRIuPTR " | offset: %" PRIuPTR " - %" PRIuPTR "\n", opcode_strs[opcode], label, label_addr, addr, addr2);
						break;
					}
					
					case movi: {
						const uintptr_t addr = ( uintptr_t )(pc.uint8) - offs;
						const uint32_t reg = *pc.uint8++;
						const int64_t imm = *pc.int64++;
						const uintptr_t addr2 = ( uintptr_t )(pc.uint8) - offs;
						
						harbol_string_add_format(&bc_funcs, "    %-10s r%u, %#" PRIx64 " ;; offset: %" PRIuPTR " - %" PRIuPTR "\n", opcode_strs[opcode], reg, imm, addr, addr2);
						break;
					}
					default: break;
				}
			}
			harbol_string_add_format(&bc_funcs, "}\n\n");
		} else {
			if( entry->flags & TAGHA_FLAG_NATIVE ) {
				harbol_string_add_format(&nbc_funcs, "$native    %s\n", iter.string);
			} else if( entry->flags & TAGHA_FLAG_EXTERN ) {
				harbol_string_add_format(&nbc_funcs, "$extern    %s\n", iter.string);
			}
			iter.uint8 += entry->name_len;
		}
	}
	harbol_string_add_cstr(&nbc_funcs, "\n");
	
	/// iterate var table and get bytecode sizes.
	for( uint32_t i=0; i<var_table_size; i++ ) {
		const struct TaghaItemEntry *const entry = iter.ptr;
		iter.uint8 += sizeof *entry;
		uint32_t bytes = entry->data_len;
		harbol_string_add_format(&vars, "$global    %-25s, %u", iter.string, entry->data_len);
		iter.uint8 += entry->name_len;
		union HarbolBinIter data = iter;
		iter.uint8 += entry->data_len;
		for( ; bytes / sizeof(union TaghaVal) > 0; bytes -= sizeof(union TaghaVal) ) {
			harbol_string_add_format(&vars, ", word %#" PRIx64 "", *data.uint64++);
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
	free(filedata); filedata=NULL;
	
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
	harbol_string_add_str(&final_output, &header);    harbol_string_clear(&header);
	harbol_string_add_str(&final_output, &vars);      harbol_string_clear(&vars);
	harbol_string_add_str(&final_output, &nbc_funcs); harbol_string_clear(&nbc_funcs);
	harbol_string_add_str(&final_output, &bc_funcs);  harbol_string_clear(&bc_funcs);
	
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
	( void )(arg);
}

int main(const int argc, char *argv[restrict static 1])
{
	if( argc<=1 ) {
		fprintf(stderr, "Tagha Disassembler - usage: %s [.tbc file...]\n", argv[0]);
		return -1;
	} else if( !strcmp(argv[1], "--help") ) {
		puts("Tagha Disassembler - Tagha Runtime Environment Toolkit\nTo decompile a tbc script to tasm, supply a script name as a command-line argument to the program.\nExample: './tagha_disasm [options] script.tbc'");
	} else if( !strcmp(argv[1], "--version") ) {
		puts("Tagha Disassembler Version 1.0.0");
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
