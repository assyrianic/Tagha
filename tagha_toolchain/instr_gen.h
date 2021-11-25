#ifndef TAGHA_INSTR_GEN
#	define TAGHA_INSTR_GEN

#include "libharbol/harbol.h"
#include "../tagha/tagha.h"
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif


static inline size_t tagha_instr_gen(struct HarbolByteBuf *const tbc, const enum TaghaInstrSet op, ...)
{
	if( op >= MaxOps )
		return 0;
	
	size_t bytes = 1;
	if( tbc != NULL )
		harbol_bytebuffer_insert_byte(tbc, op);
	
	va_list ap; va_start(ap, op);
	switch( op ) {
		/// one byte operand.
		case alloc: case redux: case setelen:
		case neg: case fneg:
		case bit_not: case setc:
		case callr:
		case f32tof64: case f64tof32:
		case itof64: case itof32:
		case f64toi: case f32toi:
		case vneg: case vfneg:
		case vnot: {
			if( tbc != NULL ) {
				const int oper1 = va_arg(ap, int);
				harbol_bytebuffer_insert_byte(tbc, oper1);
			}
			bytes += 1;
			break;
		}
		
		/// two byte operands.
		case mov:
		case add: case sub: case mul: case idiv: case mod:
		case fadd: case fsub: case fmul: case fdiv:
		case bit_and: case bit_or: case bit_xor: case shl: case shr: case shar:
		case ilt: case ile: case ult: case ule: case cmp: case flt: case fle:
		case vmov:
		case vadd: case vsub: case vmul: case vdiv: case vmod:
		case vfadd: case vfsub: case vfmul: case vfdiv:
		case vand: case vor: case vxor: case vshl: case vshr: case vshar:
		case vcmp: case vilt: case vile: case vult: case vule: case vflt: case vfle: {
			if( tbc != NULL ) {
				const int oper1 = va_arg(ap, int);
				const int oper2 = va_arg(ap, int);
				harbol_bytebuffer_insert_byte(tbc, oper1);
				harbol_bytebuffer_insert_byte(tbc, oper2);
			}
			bytes += 2;
			break;
		}
		
		case call: case setvlen: {
			if( tbc != NULL ) {
				const int oper1 = va_arg(ap, int);
				harbol_bytebuffer_insert_int16(tbc, ( uint16_t )oper1);
			}
			bytes += 2;
			break;
		}
		
		/// one byte + unsigned 2-byte int.
		case lra: case ldvar: case ldfn: {
			if( tbc != NULL ) {
				const int oper1 = va_arg(ap, int);
				const int oper2 = va_arg(ap, int);
				harbol_bytebuffer_insert_byte(tbc, oper1);
				harbol_bytebuffer_insert_int16(tbc, oper2);
			}
			bytes += 3;
			break;
		}
		
		/// two bytes + signed 2-byte int.
		case lea:
		case ld1: case ld2: case ld4: case ld8: case ldu1: case ldu2: case ldu4:
		case st1: case st2: case st4: case st8: {
			if( tbc != NULL ) {
				const int oper1     = va_arg(ap, int);
				const int oper2     = va_arg(ap, int);
				const int _oper3    = va_arg(ap, int);
				const int16_t oper3 = ( int16_t )_oper3;
				harbol_bytebuffer_insert_byte(tbc, oper1);
				harbol_bytebuffer_insert_byte(tbc, oper2);
				harbol_bytebuffer_insert_int16(tbc, ( uint16_t )oper3);
			}
			bytes += 4;
			break;
		}
		
		/// signed 4-byte operand.
		case jmp: case jz: case jnz: {
			if( tbc != NULL ) {
				const int32_t oper1 = va_arg(ap, int32_t);
				harbol_bytebuffer_insert_int32(tbc, ( uint32_t )oper1);
			}
			bytes += 4;
			break;
		}
		
		/// byte + 8-byte int operand.
		case movi: {
			if( tbc != NULL ) {
				const int oper1 = va_arg(ap, int);
				const union TaghaVal oper2 = va_arg(ap, union TaghaVal);
				harbol_bytebuffer_insert_byte(tbc, oper1);
				harbol_bytebuffer_insert_int64(tbc, oper2.uint64);
			}
			bytes += 9;
			break;
		}
		
		/// no operands.
		case ret: case halt: case nop: case pushlr: case poplr: case MaxOps: {
			break;
		}
	}
	va_end(ap);
	return bytes;
}




#ifdef __cplusplus
} /** extern "C" */
#endif

#endif /** TAGHA_INSTR_GEN */