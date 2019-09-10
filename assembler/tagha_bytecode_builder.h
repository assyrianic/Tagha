#pragma once

#include "libharbol/harbol.h"
#include "../tagha/tagha.h"


static inline size_t tagha_bc_op(struct HarbolByteBuf *const tbc, const enum TaghaInstrSet op)
{
	if( tbc != NULL )
		harbol_bytebuffer_insert_byte(tbc, op);
	return 1;
}

static inline size_t tagha_bc_op_imm(struct HarbolByteBuf *const tbc, const enum TaghaInstrSet op, const uint64_t imm)
{
	if( tbc != NULL ) {
		harbol_bytebuffer_insert_byte(tbc, op);
		harbol_bytebuffer_insert_int64(tbc, imm);
	}
	return 9;
}

static inline size_t tagha_bc_op_reg(struct HarbolByteBuf *const tbc, const enum TaghaInstrSet op, const enum TaghaRegID reg)
{
	if( tbc != NULL ) {
		harbol_bytebuffer_insert_byte(tbc, op);
		harbol_bytebuffer_insert_byte(tbc, reg);
	}
	return 2;
}

static inline size_t tagha_bc_op_reg_reg(struct HarbolByteBuf *const tbc, const enum TaghaInstrSet op, const enum TaghaRegID dest, const enum TaghaRegID src)
{
	if( tbc != NULL ) {
		harbol_bytebuffer_insert_byte(tbc, op);
		harbol_bytebuffer_insert_byte(tbc, dest);
		harbol_bytebuffer_insert_byte(tbc, src);
	}
	return 3;
}

static inline size_t tagha_bc_op_reg_imm(struct HarbolByteBuf *const tbc, const enum TaghaInstrSet op, const enum TaghaRegID reg, const uint64_t imm)
{
	if( tbc != NULL ) {
		harbol_bytebuffer_insert_byte(tbc, op);
		harbol_bytebuffer_insert_byte(tbc, reg);
		harbol_bytebuffer_insert_int64(tbc, imm);
	}
	return 10;
}

static inline size_t tagha_bc_op_reg_mem(struct HarbolByteBuf *const tbc, const enum TaghaInstrSet op, const enum TaghaRegID dest, const enum TaghaRegID src, const int32_t offset)
{
	if( tbc != NULL ) {
		harbol_bytebuffer_insert_byte(tbc, op);
		harbol_bytebuffer_insert_byte(tbc, dest);
		harbol_bytebuffer_insert_byte(tbc, src);
		harbol_bytebuffer_insert_int32(tbc, (uint32_t)offset);
	}
	return 7;
}
