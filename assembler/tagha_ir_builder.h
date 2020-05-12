#pragma once

#include "libharbol/harbol.h"
#include "../tagha/tagha.h"
#include "tagha_bytecode_builder.h"


#define TAGHA_IR_VERSION_MAJOR    1
#define TAGHA_IR_VERSION_MINOR    0
#define TAGHA_IR_VERSION_PATCH    0
#define TAGHA_IR_VERSION_PHASE    'beta'

typedef enum TaghaIRType {
	/// int arithmetic ops
	TIR_ADD,
	TIR_SUB,
	TIR_MUL,
	TIR_DIV,
	TIR_MOD,
	TIR_NEG,
	
	/// int bitwise ops
	TIR_AND,
	TIR_OR,
	TIR_XOR,
	TIR_NOT,
	TIR_SHL,
	TIR_SHR,
	
	/// int relational ops
	TIR_EQ,
	TIR_LE,
	TIR_LT,
	
	/// data transfer
	TIR_PUSH,
	TIR_POP,
	TIR_MOV,
	TIR_MOV_IMM,
	TIR_SETC,
	
	/// control flow
	TIR_RET,
	TIR_CALL,
	TIR_JMP,
	TIR_BR,
	
	/// memory loads
	TIR_LOAD1,
	TIR_LOAD2,
	TIR_LOAD4,
	TIR_LOAD8,
	TIR_LOADBP,
	TIR_LOADFN,
	TIR_LOADVAR,
	
	/// memory stores
	TIR_STORE1,
	TIR_STORE2,
	TIR_STORE4,
	TIR_STORE8,
	
	TIR_NOP,
	
	/// float arithmetic ops
	TIR_FADD,
	TIR_FSUB,
	TIR_FMUL,
	TIR_FDIV,
	TIR_FNEG,
	
	/// float conversion ops
	TIR_F32_TO_F64,
	TIR_F64_TO_F32,
	TIR_INT_TO_F32,
	TIR_INT_TO_F64,
	TIR_F64_TO_INT,
	TIR_F32_TO_INT,
	
	/// float relational ops
	TIR_FLE,
	TIR_FLT,
} ETaghaIRType;


typedef struct TaghaIR {
	union {
		struct {
			int64_t id;
			size_t nargs;
			bool is_native : 1, is_reg : 1;
		} call;
		struct {
			uint64_t val;
			enum TaghaRegID r;
		} imm;
		struct {
			int32_t offset;
			enum TaghaRegID regs[2]; /// 0 is dest, 1 is src reg.
		} mem;
		struct {
			size_t label;
			bool cond : 1; /// conditional jump?
		} jmp;
		enum TaghaRegID regs[2];
	} ir;
	enum TaghaIRType op : 8;
	bool usign : 1;
} STaghaIR;

/** use for arithmetic and other ops like copying/moving. */
static inline struct TaghaIR tagha_ir_binop(const enum TaghaIRType op, const enum TaghaRegID r1, const enum TaghaRegID r2, const bool usign)
{
	return (struct TaghaIR){ .ir.regs = { r1,r2 }, op, usign };
}

static inline struct TaghaIR tagha_ir_unop(const enum TaghaIRType op, const enum TaghaRegID r)
{
	return (struct TaghaIR){ .ir.regs = { r,-1 }, op };
}

static inline struct TaghaIR tagha_ir_mov_imm(const enum TaghaRegID r, const uint64_t val)
{
	return (struct TaghaIR){ .ir.imm.val=val, .ir.imm.r = r, TIR_MOV_IMM };
}

static inline struct TaghaIR tagha_ir_mov_imm_f32(const enum TaghaRegID r, const float32_t val)
{
	union {
		uint64_t u64;
		float32_t f32;
	} pun = {0};
	pun.f32 = val;
	return (struct TaghaIR){ .ir.imm.val=pun.u64, .ir.imm.r = r, TIR_MOV_IMM };
}

static inline struct TaghaIR tagha_ir_mov_imm_f64(const enum TaghaRegID r, const float64_t val)
{
	union {
		float64_t f64;
		uint64_t u64;
	} pun = {val};
	return (struct TaghaIR){ .ir.imm.val=pun.u64, .ir.imm.r = r, TIR_MOV_IMM };
}

static inline struct TaghaIR tagha_ir_setc(const enum TaghaRegID r)
{
	return (struct TaghaIR){ .ir.regs = { r,-1 }, TIR_SETC };
}

static inline struct TaghaIR tagha_ir_push(const enum TaghaRegID r)
{
	return (struct TaghaIR){ .ir.regs = { r,-1 }, TIR_PUSH };
}

static inline struct TaghaIR tagha_ir_pop(const enum TaghaRegID r)
{
	return (struct TaghaIR){ .ir.regs = { r,-1 }, TIR_POP };
}

static inline struct TaghaIR tagha_ir_load_var(const enum TaghaRegID r, const uint64_t global)
{
	return (struct TaghaIR){ .ir.imm.val = global, .ir.imm.r = r, TIR_LOADVAR };
}

static inline struct TaghaIR tagha_ir_load_func(const enum TaghaRegID r, const int64_t func)
{
	return (struct TaghaIR){ .ir.imm.val = (uint64_t)func, .ir.imm.r = r, TIR_LOADFN };
}

static inline struct TaghaIR tagha_ir_load_bp(const enum TaghaRegID r, const int32_t offset)
{
	return (struct TaghaIR){ .ir.mem.offset = offset, .ir.mem.regs = { r, bp }, TIR_LOADBP };
}

static inline struct TaghaIR tagha_ir_load(const enum TaghaRegID r1, const enum TaghaRegID r2, const int32_t offset, const size_t bytes)
{
	switch( bytes ) {
		case 2: return (struct TaghaIR){ .ir.mem.offset = offset, .ir.mem.regs = { r1, r2 }, TIR_LOAD2 };
		case 4: return (struct TaghaIR){ .ir.mem.offset = offset, .ir.mem.regs = { r1, r2 }, TIR_LOAD4 };
		case 8: return (struct TaghaIR){ .ir.mem.offset = offset, .ir.mem.regs = { r1, r2 }, TIR_LOAD8 };
		default: return (struct TaghaIR){ .ir.mem.offset = offset, .ir.mem.regs = { r1, r2 }, TIR_LOAD1 };
	}
}

static inline struct TaghaIR tagha_ir_store(const enum TaghaRegID r1, const enum TaghaRegID r2, const int32_t offset, const size_t bytes)
{
	switch( bytes ) {
		case 2: return (struct TaghaIR){ .ir.mem.offset = offset, .ir.mem.regs = { r1, r2 }, TIR_STORE2 };
		case 4: return (struct TaghaIR){ .ir.mem.offset = offset, .ir.mem.regs = { r1, r2 }, TIR_STORE4 };
		case 8: return (struct TaghaIR){ .ir.mem.offset = offset, .ir.mem.regs = { r1, r2 }, TIR_STORE8 };
		default: return (struct TaghaIR){ .ir.mem.offset = offset, .ir.mem.regs = { r1, r2 }, TIR_STORE1 };
	}
}

static inline struct TaghaIR tagha_ir_call(const int64_t funcid, const size_t arg_count, const bool native, const bool from_reg)
{
	return (struct TaghaIR){ .ir.call.id = funcid, .ir.call.nargs = arg_count, .ir.call.is_native = native, .ir.call.is_reg = from_reg, TIR_CALL };
}

static inline struct TaghaIR tagha_ir_jmp(const size_t label)
{
	return (struct TaghaIR){ .ir.jmp.label = label, .ir.jmp.cond = true, TIR_JMP };
}

static inline struct TaghaIR tagha_ir_br(const size_t label, const bool not_zero)
{
	return (struct TaghaIR){ .ir.jmp.label = label, .ir.jmp.cond = not_zero, TIR_BR };
}

static inline struct TaghaIR tagha_ir_ret(void)
{
	return (struct TaghaIR){ .op = TIR_RET };
}

static inline struct TaghaIR tagha_ir_nop(void)
{
	return (struct TaghaIR){ .op = TIR_NOP };
}


typedef struct TaghaIRBlock {
	struct HarbolVector ir;
	size_t offset;
} STaghaIRBlock;

static inline struct TaghaIRBlock taghair_bb_create(void)
{
	struct TaghaIRBlock tbb = { harbol_vector_create(sizeof(struct TaghaIR), 1), 0 };
	return tbb;
}

static inline NO_NULL bool taghair_bb_clear(struct TaghaIRBlock *const tbb)
{
	return harbol_vector_clear(&tbb->ir, NULL);
}

static inline NO_NULL bool taghair_bb_add(struct TaghaIRBlock *const tbb, struct TaghaIR *const tir)
{
	return harbol_vector_insert(&tbb->ir, tir);
}


typedef struct TaghaIRFunc {
	struct HarbolVector blocks;
} STaghaIRFunc;

static inline NO_NULL struct TaghaIRFunc taghair_func_create(void)
{
	struct TaghaIRFunc irfunc = { harbol_vector_create(sizeof(struct TaghaIRBlock), 1) };
	return irfunc;
}

static inline NO_NULL bool taghair_func_add(struct TaghaIRFunc *const tfn, struct TaghaIRBlock *const tbb)
{
	return harbol_vector_insert(&tfn->blocks, tbb);
}

static inline NO_NULL void taghair_func_clear(struct TaghaIRFunc *const tfn)
{
	const struct TaghaIRBlock *const bbend = harbol_vector_get_iter_end_count(&tfn->blocks);
	for( struct TaghaIRBlock *b = harbol_vector_get_iter(&tfn->blocks); b != NULL && b < bbend; b++ )
		taghair_bb_clear(b);
	harbol_vector_clear(&tfn->blocks, NULL);
}


static NO_NULL struct HarbolByteBuf taghair_func_to_bytecode(const struct TaghaIRFunc *const tfn)
{
	size_t prog_counter = 0;
	/// first pass, collect jump addresses.
	const struct TaghaIRBlock *const bbend = harbol_vector_get_iter_end_count(&tfn->blocks);
	for( struct TaghaIRBlock *b = harbol_vector_get_iter(&tfn->blocks); b != NULL && b < bbend; b++ ) {
		b->offset = prog_counter;
		const struct TaghaIR *const irend = harbol_vector_get_iter_end_count(&b->ir);
		for( const struct TaghaIR *i = harbol_vector_get_iter(&b->ir); i != NULL && i < irend; i++ ) {
			switch( i->op ) {
				/// reg <-- reg type instrs.
				case TIR_MOV: prog_counter += tagha_bc_op_reg_reg(NULL, mov, i->ir.regs[0], i->ir.regs[1]); break;
				case TIR_ADD: prog_counter += tagha_bc_op_reg_reg(NULL, add, i->ir.regs[0], i->ir.regs[1]); break;
				case TIR_SUB: prog_counter += tagha_bc_op_reg_reg(NULL, sub, i->ir.regs[0], i->ir.regs[1]); break;
				case TIR_MUL: prog_counter += tagha_bc_op_reg_reg(NULL, mul, i->ir.regs[0], i->ir.regs[1]); break;
				case TIR_DIV: prog_counter += tagha_bc_op_reg_reg(NULL, divi, i->ir.regs[0], i->ir.regs[1]); break;
				case TIR_MOD: prog_counter += tagha_bc_op_reg_reg(NULL, mod, i->ir.regs[0], i->ir.regs[1]); break;
				
				case TIR_AND: prog_counter += tagha_bc_op_reg_reg(NULL, bit_and, i->ir.regs[0], i->ir.regs[1]); break;
				case TIR_OR: prog_counter += tagha_bc_op_reg_reg(NULL, bit_or, i->ir.regs[0], i->ir.regs[1]); break;
				case TIR_XOR: prog_counter += tagha_bc_op_reg_reg(NULL, bit_xor, i->ir.regs[0], i->ir.regs[1]); break;
				case TIR_SHL: prog_counter += tagha_bc_op_reg_reg(NULL, shl, i->ir.regs[0], i->ir.regs[1]); break;
				case TIR_SHR: prog_counter += tagha_bc_op_reg_reg(NULL, shr, i->ir.regs[0], i->ir.regs[1]); break;
				
				case TIR_EQ: prog_counter += tagha_bc_op_reg_reg(NULL, cmp, i->ir.regs[0], i->ir.regs[1]); break;
				case TIR_LE: prog_counter += tagha_bc_op_reg_reg(NULL, (i->usign) ? ule: ile, i->ir.regs[0], i->ir.regs[1]); break;
				case TIR_LT: prog_counter += tagha_bc_op_reg_reg(NULL, (i->usign) ? ult: ilt, i->ir.regs[0], i->ir.regs[1]); break;
				
				case TIR_FADD: prog_counter += tagha_bc_op_reg_reg(NULL, addf, i->ir.regs[0], i->ir.regs[1]); break;
				case TIR_FSUB: prog_counter += tagha_bc_op_reg_reg(NULL, subf, i->ir.regs[0], i->ir.regs[1]); break;
				case TIR_FMUL: prog_counter += tagha_bc_op_reg_reg(NULL, mulf, i->ir.regs[0], i->ir.regs[1]); break;
				case TIR_FDIV: prog_counter += tagha_bc_op_reg_reg(NULL, divf, i->ir.regs[0], i->ir.regs[1]); break;
				
				case TIR_FLE: prog_counter += tagha_bc_op_reg_reg(NULL, lef, i->ir.regs[0], i->ir.regs[1]); break;
				case TIR_FLT: prog_counter += tagha_bc_op_reg_reg(NULL, ltf, i->ir.regs[0], i->ir.regs[1]); break;
				
				/// reg only type instrs.
				case TIR_POP: prog_counter += tagha_bc_op_reg(NULL, pop, i->ir.regs[0]); break;
				case TIR_NOT: prog_counter += tagha_bc_op_reg(NULL, bit_not, i->ir.regs[0]); break;
				case TIR_NEG: prog_counter += tagha_bc_op_reg(NULL, neg, i->ir.regs[0]); break;
				case TIR_PUSH: prog_counter += tagha_bc_op_reg(NULL, push, i->ir.regs[0]); break;
				case TIR_SETC: prog_counter += tagha_bc_op_reg(NULL, setc, i->ir.regs[0]); break;
				
				case TIR_F32_TO_F64: prog_counter += tagha_bc_op_reg(NULL, f32tof64, i->ir.regs[0]); break;
				case TIR_F64_TO_F32: prog_counter += tagha_bc_op_reg(NULL, f64tof32, i->ir.regs[0]); break;
				case TIR_INT_TO_F32: prog_counter += tagha_bc_op_reg(NULL, itof32, i->ir.regs[0]); break;
				case TIR_INT_TO_F64: prog_counter += tagha_bc_op_reg(NULL, itof64, i->ir.regs[0]); break;
				case TIR_F64_TO_INT: prog_counter += tagha_bc_op_reg(NULL, f64toi, i->ir.regs[0]); break;
				case TIR_F32_TO_INT: prog_counter += tagha_bc_op_reg(NULL, f32toi, i->ir.regs[0]); break;
				case TIR_FNEG: prog_counter += tagha_bc_op_reg(NULL, negf, i->ir.regs[0]); break;
				
				/// reg <-- *(reg + offset) load instrs.
				case TIR_LOAD1: prog_counter += tagha_bc_op_reg_mem(NULL, ld1, i->ir.mem.regs[0], i->ir.mem.regs[1], i->ir.mem.offset); break;
				case TIR_LOAD2: prog_counter += tagha_bc_op_reg_mem(NULL, ld2, i->ir.mem.regs[0], i->ir.mem.regs[1], i->ir.mem.offset); break;
				case TIR_LOAD4: prog_counter += tagha_bc_op_reg_mem(NULL, ld4, i->ir.mem.regs[0], i->ir.mem.regs[1], i->ir.mem.offset); break;
				case TIR_LOAD8: prog_counter += tagha_bc_op_reg_mem(NULL, ld8, i->ir.mem.regs[0], i->ir.mem.regs[1], i->ir.mem.offset); break;
				case TIR_LOADBP: prog_counter += tagha_bc_op_reg_mem(NULL, ldaddr, i->ir.mem.regs[0], i->ir.mem.regs[1], i->ir.mem.offset); break;
				
				/// *(reg + offset) <-- reg store instrs.
				case TIR_STORE1: prog_counter += tagha_bc_op_reg_mem(NULL, st1, i->ir.mem.regs[0], i->ir.mem.regs[1], i->ir.mem.offset); break;
				case TIR_STORE2: prog_counter += tagha_bc_op_reg_mem(NULL, st2, i->ir.mem.regs[0], i->ir.mem.regs[1], i->ir.mem.offset); break;
				case TIR_STORE4: prog_counter += tagha_bc_op_reg_mem(NULL, st4, i->ir.mem.regs[0], i->ir.mem.regs[1], i->ir.mem.offset); break;
				case TIR_STORE8: prog_counter += tagha_bc_op_reg_mem(NULL, st8, i->ir.mem.regs[0], i->ir.mem.regs[1], i->ir.mem.offset); break;
				
				/// reg <-- imm type instrs.
				case TIR_LOADFN: prog_counter += tagha_bc_op_reg_imm(NULL, ldfunc, i->ir.imm.r, i->ir.imm.val); break;
				case TIR_LOADVAR: prog_counter += tagha_bc_op_reg_imm(NULL, ldvar, i->ir.imm.r, i->ir.imm.val); break;
				case TIR_MOV_IMM: prog_counter += tagha_bc_op_reg_imm(NULL, movi, i->ir.imm.r, i->ir.imm.val); break;
				
				/// imm only instrs.
				case TIR_CALL:
					if( i->ir.call.is_native )
						prog_counter += tagha_bc_op_reg_imm(NULL, movi, alaf, i->ir.call.nargs);
					
					if( i->ir.call.is_reg )
						prog_counter += tagha_bc_op_reg(NULL, callr, i->ir.call.id);
					else prog_counter += tagha_bc_op_imm(NULL, call, (uint64_t)i->ir.call.id);
					break;
				
				case TIR_JMP: prog_counter += tagha_bc_op_imm(NULL, jmp, 0); break;
				case TIR_BR: prog_counter += tagha_bc_op_imm(NULL, (i->ir.jmp.cond) ? jnz : jz, 0); break;
				
				/// no operand instrs
				case TIR_RET: prog_counter += tagha_bc_op(NULL, ret); break;
				case TIR_NOP: prog_counter += tagha_bc_op(NULL, nop); break;
			}
		}
	}
	
	prog_counter = 0;
	struct HarbolByteBuf bytecode = harbol_bytebuffer_create();
	/// second pass, generate actual bytecode.
	for( const struct TaghaIRBlock *b = harbol_vector_get_iter(&tfn->blocks); b != NULL && b < bbend; b++ ) {
		const struct TaghaIR *const irend = harbol_vector_get_iter_end_count(&b->ir);
		for( const struct TaghaIR *i = harbol_vector_get_iter(&b->ir); i != NULL && i < irend; i++ ) {
			switch( i->op ) {
				/// reg <-- reg type instrs.
				case TIR_MOV: prog_counter += tagha_bc_op_reg_reg(&bytecode, mov, i->ir.regs[0], i->ir.regs[1]); break;
				case TIR_ADD: prog_counter += tagha_bc_op_reg_reg(&bytecode, add, i->ir.regs[0], i->ir.regs[1]); break;
				case TIR_SUB: prog_counter += tagha_bc_op_reg_reg(&bytecode, sub, i->ir.regs[0], i->ir.regs[1]); break;
				case TIR_MUL: prog_counter += tagha_bc_op_reg_reg(&bytecode, mul, i->ir.regs[0], i->ir.regs[1]); break;
				case TIR_DIV: prog_counter += tagha_bc_op_reg_reg(&bytecode, divi, i->ir.regs[0], i->ir.regs[1]); break;
				case TIR_MOD: prog_counter += tagha_bc_op_reg_reg(&bytecode, mod, i->ir.regs[0], i->ir.regs[1]); break;
				
				case TIR_AND: prog_counter += tagha_bc_op_reg_reg(&bytecode, bit_and, i->ir.regs[0], i->ir.regs[1]); break;
				case TIR_OR: prog_counter += tagha_bc_op_reg_reg(&bytecode, bit_or, i->ir.regs[0], i->ir.regs[1]); break;
				case TIR_XOR: prog_counter += tagha_bc_op_reg_reg(&bytecode, bit_xor, i->ir.regs[0], i->ir.regs[1]); break;
				case TIR_SHL: prog_counter += tagha_bc_op_reg_reg(&bytecode, shl, i->ir.regs[0], i->ir.regs[1]); break;
				case TIR_SHR: prog_counter += tagha_bc_op_reg_reg(&bytecode, shr, i->ir.regs[0], i->ir.regs[1]); break;
				
				case TIR_EQ: prog_counter += tagha_bc_op_reg_reg(&bytecode, cmp, i->ir.regs[0], i->ir.regs[1]); break;
				case TIR_LE: prog_counter += tagha_bc_op_reg_reg(&bytecode, (i->usign) ? ule: ile, i->ir.regs[0], i->ir.regs[1]); break;
				case TIR_LT: prog_counter += tagha_bc_op_reg_reg(&bytecode, (i->usign) ? ult: ilt, i->ir.regs[0], i->ir.regs[1]); break;
				
				case TIR_FADD: prog_counter += tagha_bc_op_reg_reg(&bytecode, addf, i->ir.regs[0], i->ir.regs[1]); break;
				case TIR_FSUB: prog_counter += tagha_bc_op_reg_reg(&bytecode, subf, i->ir.regs[0], i->ir.regs[1]); break;
				case TIR_FMUL: prog_counter += tagha_bc_op_reg_reg(&bytecode, mulf, i->ir.regs[0], i->ir.regs[1]); break;
				case TIR_FDIV: prog_counter += tagha_bc_op_reg_reg(&bytecode, divf, i->ir.regs[0], i->ir.regs[1]); break;
				
				case TIR_FLE: prog_counter += tagha_bc_op_reg_reg(&bytecode, lef, i->ir.regs[0], i->ir.regs[1]); break;
				case TIR_FLT: prog_counter += tagha_bc_op_reg_reg(&bytecode, ltf, i->ir.regs[0], i->ir.regs[1]); break;
				
				/// reg only type instrs.
				case TIR_POP: prog_counter += tagha_bc_op_reg(&bytecode, pop, i->ir.regs[0]); break;
				case TIR_NOT: prog_counter += tagha_bc_op_reg(&bytecode, bit_not, i->ir.regs[0]); break;
				case TIR_NEG: prog_counter += tagha_bc_op_reg(&bytecode, neg, i->ir.regs[0]); break;
				case TIR_PUSH: prog_counter += tagha_bc_op_reg(&bytecode, push, i->ir.regs[0]); break;
				case TIR_SETC: prog_counter += tagha_bc_op_reg(&bytecode, setc, i->ir.regs[0]); break;
				
				case TIR_F32_TO_F64: prog_counter += tagha_bc_op_reg(&bytecode, f32tof64, i->ir.regs[0]); break;
				case TIR_F64_TO_F32: prog_counter += tagha_bc_op_reg(&bytecode, f64tof32, i->ir.regs[0]); break;
				case TIR_INT_TO_F32: prog_counter += tagha_bc_op_reg(&bytecode, itof32, i->ir.regs[0]); break;
				case TIR_INT_TO_F64: prog_counter += tagha_bc_op_reg(&bytecode, itof64, i->ir.regs[0]); break;
				case TIR_F64_TO_INT: prog_counter += tagha_bc_op_reg(NULL, f64toi, i->ir.regs[0]); break;
				case TIR_F32_TO_INT: prog_counter += tagha_bc_op_reg(NULL, f32toi, i->ir.regs[0]); break;
				case TIR_FNEG: prog_counter += tagha_bc_op_reg(&bytecode, negf, i->ir.regs[0]); break;
				
				/// reg <-- *(reg + offset) load instrs.
				case TIR_LOAD1: prog_counter += tagha_bc_op_reg_mem(&bytecode, ld1, i->ir.mem.regs[0], i->ir.mem.regs[1], i->ir.mem.offset); break;
				case TIR_LOAD2: prog_counter += tagha_bc_op_reg_mem(&bytecode, ld2, i->ir.mem.regs[0], i->ir.mem.regs[1], i->ir.mem.offset); break;
				case TIR_LOAD4: prog_counter += tagha_bc_op_reg_mem(&bytecode, ld4, i->ir.mem.regs[0], i->ir.mem.regs[1], i->ir.mem.offset); break;
				case TIR_LOAD8: prog_counter += tagha_bc_op_reg_mem(&bytecode, ld8, i->ir.mem.regs[0], i->ir.mem.regs[1], i->ir.mem.offset); break;
				case TIR_LOADBP: prog_counter += tagha_bc_op_reg_mem(&bytecode, ldaddr, i->ir.mem.regs[0], i->ir.mem.regs[1], i->ir.mem.offset); break;
				
				/// *(reg + offset) <-- reg store instrs.
				case TIR_STORE1: prog_counter += tagha_bc_op_reg_mem(&bytecode, st1, i->ir.mem.regs[0], i->ir.mem.regs[1], i->ir.mem.offset); break;
				case TIR_STORE2: prog_counter += tagha_bc_op_reg_mem(&bytecode, st2, i->ir.mem.regs[0], i->ir.mem.regs[1], i->ir.mem.offset); break;
				case TIR_STORE4: prog_counter += tagha_bc_op_reg_mem(&bytecode, st4, i->ir.mem.regs[0], i->ir.mem.regs[1], i->ir.mem.offset); break;
				case TIR_STORE8: prog_counter += tagha_bc_op_reg_mem(&bytecode, st8, i->ir.mem.regs[0], i->ir.mem.regs[1], i->ir.mem.offset); break;
				
				/// reg <-- imm type instrs.
				case TIR_LOADFN: prog_counter += tagha_bc_op_reg_imm(&bytecode, ldfunc, i->ir.imm.r, i->ir.imm.val); break;
				case TIR_LOADVAR: prog_counter += tagha_bc_op_reg_imm(&bytecode, ldvar, i->ir.imm.r, i->ir.imm.val); break;
				case TIR_MOV_IMM: prog_counter += tagha_bc_op_reg_imm(&bytecode, movi, i->ir.imm.r, i->ir.imm.val); break;
				
				/// imm only instrs.
				case TIR_CALL:
					if( i->ir.call.is_native )
						prog_counter += tagha_bc_op_reg_imm(&bytecode, movi, alaf, i->ir.call.nargs);
					
					if( i->ir.call.is_reg )
						prog_counter += tagha_bc_op_reg(&bytecode, callr, i->ir.call.id);
					else prog_counter += tagha_bc_op_imm(&bytecode, call, (uint64_t)i->ir.call.id);
					break;
				
				case TIR_JMP: {
					const struct TaghaIRBlock *const jump_to = harbol_vector_get(&tfn->blocks, i->ir.jmp.label);
					prog_counter += tagha_bc_op_imm(NULL, 0, 0);
					tagha_bc_op_imm(&bytecode, jmp, jump_to->offset - prog_counter);
					break;
				}
				case TIR_BR: {
					const struct TaghaIRBlock *const jump_to = harbol_vector_get(&tfn->blocks, i->ir.jmp.label);
					prog_counter += tagha_bc_op_imm(NULL, 0, 0);
					tagha_bc_op_imm(&bytecode, (i->ir.jmp.cond) ? jnz : jz, jump_to->offset - prog_counter);
					break;
				}
				/// no operand instrs
				case TIR_RET: prog_counter += tagha_bc_op(&bytecode, ret); break;
				case TIR_NOP: prog_counter += tagha_bc_op(&bytecode, nop); break;
			}
		}
	}
	return bytecode;
}


static NO_NULL struct HarbolString taghair_func_to_tasm(const struct TaghaIRFunc *const tfn)
{
	struct HarbolString str = harbol_string_create("");
	
#define X(x) #x ,
	const char *opcode2str[] = { TAGHA_INSTR_SET };
#undef X
#define Y(y) #y ,
	const char *regs2str[] = { TAGHA_REG_FILE };
#undef Y
	
	/// first pass, collect jump addresses.
	const struct TaghaIRBlock *const bbend = harbol_vector_get_iter_end_count(&tfn->blocks);
	size_t block_id = 0;
	for( const struct TaghaIRBlock *b = harbol_vector_get_iter(&tfn->blocks); b != NULL && b < bbend; b++ ) {
		harbol_string_add_format(&str, ".L%zu:\n", block_id++);
		const struct TaghaIR *const irend = harbol_vector_get_iter_end_count(&b->ir);
		for( const struct TaghaIR *i = harbol_vector_get_iter(&b->ir); i != NULL && i < irend; i++ ) {
			switch( i->op ) {
				/// reg <-- reg type instrs.
				case TIR_MOV:
					harbol_string_add_format(&str, "    %s        r%s, r%s\n", opcode2str[mov], regs2str[i->ir.regs[0]], regs2str[i->ir.regs[1]]); break;
				case TIR_ADD:
					harbol_string_add_format(&str, "    %s        r%s, r%s\n", opcode2str[add], regs2str[i->ir.regs[0]], regs2str[i->ir.regs[1]]); break;
				case TIR_SUB:
					harbol_string_add_format(&str, "    %s        r%s, r%s\n", opcode2str[sub], regs2str[i->ir.regs[0]], regs2str[i->ir.regs[1]]); break;
				case TIR_MUL:
					harbol_string_add_format(&str, "    %s        r%s, r%s\n", opcode2str[mul], regs2str[i->ir.regs[0]], regs2str[i->ir.regs[1]]); break;
				case TIR_DIV:
					harbol_string_add_format(&str, "    %s       r%s, r%s\n", opcode2str[divi], regs2str[i->ir.regs[0]], regs2str[i->ir.regs[1]]); break;
				case TIR_MOD:
					harbol_string_add_format(&str, "    %s        r%s, r%s\n", opcode2str[mod], regs2str[i->ir.regs[0]], regs2str[i->ir.regs[1]]); break;
				
				case TIR_AND:
					harbol_string_add_format(&str, "    %s r%s, r%s\n", opcode2str[bit_and], regs2str[i->ir.regs[0]], regs2str[i->ir.regs[1]]); break;
				case TIR_OR:
					harbol_string_add_format(&str, "    %s  r%s, r%s\n", opcode2str[bit_or], regs2str[i->ir.regs[0]], regs2str[i->ir.regs[1]]); break;
				case TIR_XOR:
					harbol_string_add_format(&str, "    %s r%s, r%s\n", opcode2str[bit_xor], regs2str[i->ir.regs[0]], regs2str[i->ir.regs[1]]); break;
				case TIR_SHL:
					harbol_string_add_format(&str, "    %s        r%s, r%s\n", opcode2str[shl], regs2str[i->ir.regs[0]], regs2str[i->ir.regs[1]]); break;
				case TIR_SHR:
					harbol_string_add_format(&str, "    %s        r%s, r%s\n", opcode2str[shr], regs2str[i->ir.regs[0]], regs2str[i->ir.regs[1]]); break;
				
				case TIR_EQ:
					harbol_string_add_format(&str, "    %s        r%s, r%s\n", opcode2str[cmp], regs2str[i->ir.regs[0]], regs2str[i->ir.regs[1]]); break;
				case TIR_LE:
					harbol_string_add_format(&str, "    %s        r%s, r%s\n", opcode2str[(i->usign) ? ule : ile], regs2str[i->ir.regs[0]], regs2str[i->ir.regs[1]]); break;
				case TIR_LT:
					harbol_string_add_format(&str, "    %s        r%s, r%s\n", opcode2str[(i->usign) ? ult : ilt], regs2str[i->ir.regs[0]], regs2str[i->ir.regs[1]]); break;
				
				case TIR_FADD:
					harbol_string_add_format(&str, "    %s       r%s, r%s\n", opcode2str[addf], regs2str[i->ir.regs[0]], regs2str[i->ir.regs[1]]); break;
				case TIR_FSUB:
					harbol_string_add_format(&str, "    %s       r%s, r%s\n", opcode2str[subf], regs2str[i->ir.regs[0]], regs2str[i->ir.regs[1]]); break;
				case TIR_FMUL:
					harbol_string_add_format(&str, "    %s       r%s, r%s\n", opcode2str[mulf], regs2str[i->ir.regs[0]], regs2str[i->ir.regs[1]]); break;
				case TIR_FDIV:
					harbol_string_add_format(&str, "    %s       r%s, r%s\n", opcode2str[divf], regs2str[i->ir.regs[0]], regs2str[i->ir.regs[1]]); break;
				
				case TIR_FLE:
					harbol_string_add_format(&str, "    %s        r%s, r%s\n", opcode2str[lef], regs2str[i->ir.regs[0]], regs2str[i->ir.regs[1]]); break;
				case TIR_FLT:
					harbol_string_add_format(&str, "    %s        r%s, r%s\n", opcode2str[ltf], regs2str[i->ir.regs[0]], regs2str[i->ir.regs[1]]); break;
				
				/// reg only type instrs.
				case TIR_POP:
					harbol_string_add_format(&str, "    %s        r%s\n", opcode2str[pop], regs2str[i->ir.regs[0]]); break;
				case TIR_NOT:
					harbol_string_add_format(&str, "    %s r%s\n", opcode2str[bit_not], regs2str[i->ir.regs[0]]); break;
				case TIR_NEG:
					harbol_string_add_format(&str, "    %s        r%s\n", opcode2str[neg], regs2str[i->ir.regs[0]]); break;
				case TIR_PUSH:
					harbol_string_add_format(&str, "    %s       r%s\n", opcode2str[push], regs2str[i->ir.regs[0]]); break;
				case TIR_SETC:
					harbol_string_add_format(&str, "    %s       r%s\n", opcode2str[setc], regs2str[i->ir.regs[0]]); break;
				
				case TIR_F32_TO_F64:
					harbol_string_add_format(&str, "    %s r%s\n", opcode2str[f32tof64], regs2str[i->ir.regs[0]]); break;
				case TIR_F64_TO_F32:
					harbol_string_add_format(&str, "    %s r%s\n", opcode2str[f64tof32], regs2str[i->ir.regs[0]]); break;
				case TIR_INT_TO_F32:
					harbol_string_add_format(&str, "    %s r%s\n", opcode2str[itof32], regs2str[i->ir.regs[0]]); break;
				case TIR_INT_TO_F64:
					harbol_string_add_format(&str, "    %s r%s\n", opcode2str[itof64], regs2str[i->ir.regs[0]]); break;
				case TIR_F64_TO_INT:
					harbol_string_add_format(&str, "    %s r%s\n", opcode2str[f64toi], regs2str[i->ir.regs[0]]); break;
				case TIR_F32_TO_INT:
					harbol_string_add_format(&str, "    %s r%s\n", opcode2str[f32toi], regs2str[i->ir.regs[0]]); break;
				case TIR_FNEG:
					harbol_string_add_format(&str, "    %s       r%s\n", opcode2str[negf], regs2str[i->ir.regs[0]]); break;
				
				/// reg <-- *(reg + offset) load instrs.
				case TIR_LOAD1:
					harbol_string_add_format(&str, "    %s        r%s, [r%s + %i]\n", opcode2str[ld1], regs2str[i->ir.mem.regs[0]], regs2str[i->ir.mem.regs[1]], i->ir.mem.offset); break;
				case TIR_LOAD2:
					harbol_string_add_format(&str, "    %s        r%s, [r%s + %i]\n", opcode2str[ld2], regs2str[i->ir.mem.regs[0]], regs2str[i->ir.mem.regs[1]], i->ir.mem.offset); break;
				case TIR_LOAD4:
					harbol_string_add_format(&str, "    %s        r%s, [r%s + %i]\n", opcode2str[ld4], regs2str[i->ir.mem.regs[0]], regs2str[i->ir.mem.regs[1]], i->ir.mem.offset); break;
				case TIR_LOAD8:
					harbol_string_add_format(&str, "    %s        r%s, [r%s + %i]\n", opcode2str[ld8], regs2str[i->ir.mem.regs[0]], regs2str[i->ir.mem.regs[1]], i->ir.mem.offset); break;
				case TIR_LOADBP:
					harbol_string_add_format(&str, "    %s r%s, [r%s + %i]\n", opcode2str[ldaddr], regs2str[i->ir.mem.regs[0]], regs2str[i->ir.mem.regs[1]], i->ir.mem.offset); break;
				
				/// *(reg + offset) <-- reg store instrs.
				case TIR_STORE1:
					harbol_string_add_format(&str, "    %s        [r%s + %i], r%s\n", opcode2str[st1], regs2str[i->ir.mem.regs[0]], i->ir.mem.offset, regs2str[i->ir.mem.regs[1]]); break;
				case TIR_STORE2:
					harbol_string_add_format(&str, "    %s        [r%s + %i], r%s\n", opcode2str[st2], regs2str[i->ir.mem.regs[0]], i->ir.mem.offset, regs2str[i->ir.mem.regs[1]]); break;
				case TIR_STORE4:
					harbol_string_add_format(&str, "    %s        [r%s + %i], r%s\n", opcode2str[st4], regs2str[i->ir.mem.regs[0]], i->ir.mem.offset, regs2str[i->ir.mem.regs[1]]); break;
				case TIR_STORE8:
					harbol_string_add_format(&str, "    %s        [r%s + %i], r%s\n", opcode2str[st8], regs2str[i->ir.mem.regs[0]], i->ir.mem.offset, regs2str[i->ir.mem.regs[1]]); break;
				
				/// reg <-- imm type instrs.
				case TIR_LOADFN:
					harbol_string_add_format(&str, "    %s r%s, %%F%" PRIu64 "\n", opcode2str[ldfunc], regs2str[i->ir.imm.r], i->ir.imm.val); break;
				case TIR_LOADVAR:
					harbol_string_add_format(&str, "    %s r%s, %%V%" PRIu64 "\n", opcode2str[ldvar], regs2str[i->ir.imm.r], i->ir.imm.val); break;
				case TIR_MOV_IMM:
					harbol_string_add_format(&str, "    %s       r%s, %" PRIu64 "\n", opcode2str[movi], regs2str[i->ir.imm.r], i->ir.imm.val); break;
				
				/// imm only instrs.
				case TIR_CALL:
					if( i->ir.call.is_native )
						harbol_string_add_format(&str, "    %s        r%s, %" PRIu64 "\n", opcode2str[movi], regs2str[alaf], i->ir.imm.val);
					
					if( i->ir.call.is_reg )
						harbol_string_add_format(&str, "    %s         r%s\n", opcode2str[callr], regs2str[i->ir.call.id]);
					else harbol_string_add_format(&str, "    %s       %" PRIu64 "\n", opcode2str[call], (uint64_t)i->ir.call.id);
					break;
				
				case TIR_JMP:
					harbol_string_add_format(&str, "    %s         .L%" PRIu64 "\n", opcode2str[jmp], (uint64_t)i->ir.jmp.label); break;
				case TIR_BR:
					harbol_string_add_format(&str, "    %s         .L%" PRIu64 "\n", opcode2str[(i->ir.jmp.cond) ? jnz : jz], (uint64_t)i->ir.jmp.label); break;
				
				/// no operand instrs
				case TIR_RET: harbol_string_add_format(&str, "    %s\n\n", opcode2str[ret]); break;
				case TIR_NOP: harbol_string_add_format(&str, "    %s\n", opcode2str[nop]); break;
			}
		}
	}
	return str;
}