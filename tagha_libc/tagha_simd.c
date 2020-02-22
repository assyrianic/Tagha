#include <time.h>
#include <string.h>
#include "tagha_libc.h"


#define SIMD_OP_NATIVE_REG(name, op) \
	{"##name_##op", native_##name_##op},

#define SIMD_OP_NATIVE(name, member, op, opname) \
	static union TaghaVal native_##name_##opname(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1]) \
	{ \
		(void)module; (void)args; \
		union TaghaVal v = {0}; \
		for( size_t i=0; i<sizeof v.member / v.member[0]; i++ ) \
			v.member[i] = params[0].member[i] op params[1].member[i]; \
		return v; \
	}

#define SIMD_OP_NATIVE_PACK(name, member, type) \
	static union TaghaVal native_##name_pack(struct TaghaModule *const module, const size_t args, const union TaghaVal params[const static 1]) \
	{ \
		(void)module; (void)args; \
		union TaghaVal v = {0}; \
		for( size_t i=0; i<sizeof v.member / v.member[0]; i++ ) \
			v.member[i] = params[i].type; \
		return v; \
	}

SIMD_OP_NATIVE_PACK(v2i32, int32a, int32)
SIMD_OP_NATIVE(v2i32, int32a, +, add)
SIMD_OP_NATIVE(v2i32, int32a, -, sub)
SIMD_OP_NATIVE(v2i32, int32a, *, mul)
SIMD_OP_NATIVE(v2i32, int32a, /, div)
SIMD_OP_NATIVE(v2i32, int32a, %, mod)
SIMD_OP_NATIVE(v2i32, int32a, &, and)
SIMD_OP_NATIVE(v2i32, int32a, |, or)
SIMD_OP_NATIVE(v2i32, int32a, ^, xor)
SIMD_OP_NATIVE(v2i32, int32a, <<, shl)
SIMD_OP_NATIVE(v2i32, int32a, >>, shr)

#ifdef TAGHA_FLOAT32_DEFINED
SIMD_OP_NATIVE_PACK(v2f32, float32a, float32)
SIMD_OP_NATIVE(v2f32, float32a, +, add)
SIMD_OP_NATIVE(v2f32, float32a, -, sub)
SIMD_OP_NATIVE(v2f32, float32a, *, mul)
SIMD_OP_NATIVE(v2f32, float32a, /, div)
SIMD_OP_NATIVE(v2f32, float32a, <, less)
SIMD_OP_NATIVE(v2f32, float32a, >, greater)
#endif

bool tagha_module_load_simd_natives(struct TaghaModule *const module)
{
	const struct TaghaNative simd_natives[] = {
		SIMD_OP_NATIVE_REG(v2i32, pack)
		SIMD_OP_NATIVE_REG(v2i32, add)
		SIMD_OP_NATIVE_REG(v2i32, sub)
		SIMD_OP_NATIVE_REG(v2i32, mul)
		SIMD_OP_NATIVE_REG(v2i32, div)
		SIMD_OP_NATIVE_REG(v2i32, mod)
		SIMD_OP_NATIVE_REG(v2i32, and)
		SIMD_OP_NATIVE_REG(v2i32, or)
		SIMD_OP_NATIVE_REG(v2i32, xor)
		SIMD_OP_NATIVE_REG(v2i32, shl)
		SIMD_OP_NATIVE_REG(v2i32, shr)
		{NULL, NULL}
	};
	return module ? tagha_module_register_natives(module, simd_natives) : false;
}

