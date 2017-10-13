
#include "tagha.h"
#include "tagha_libc/tagha_stdio.c"
#include "tagha_libc/tagha_stdlib.c"

/* Script_t *get_self(void); */
static void native_get_script_self(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	TaghaScript_push_int64(script, (uintptr_t)script);
}


/* void script_push_int64(Script_t *script, const uint64_t val); */
static void native_script_push_int64(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	struct TaghaScript *restrict pScript = (struct TaghaScript *)(uintptr_t)TaghaScript_pop_int64(script);
	uint64_t val = TaghaScript_pop_int64(script);
	TaghaScript_push_int64(pScript, val);
}

/* uint64_t script_pop_int64(Script_t *script); */
static void native_script_pop_int64(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	struct TaghaScript *restrict pScript = (struct TaghaScript *)(uintptr_t)TaghaScript_pop_int64(script);
	uint64_t val = TaghaScript_pop_int64(pScript);
	TaghaScript_push_int64(script, val);
}


/* void script_push_int32(Script_t *script, const uint32_t val); */
static void native_script_push_int32(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	struct TaghaScript *restrict pScript = (struct TaghaScript *)(uintptr_t)TaghaScript_pop_int64(script);
	uint32_t val = TaghaScript_pop_int32(script);
	TaghaScript_push_int32(pScript, val);
}

/* uint32_t script_pop_int32(Script_t *script); */
static void native_script_pop_int32(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	struct TaghaScript *restrict pScript = (struct TaghaScript *)(uintptr_t)TaghaScript_pop_int64(script);
	uint32_t val = TaghaScript_pop_int32(pScript);
	TaghaScript_push_int32(script, val);
}

/* void script_push_double(Script_t *script, const double val); */
static void native_script_push_double(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	struct TaghaScript *restrict pScript = (struct TaghaScript *)(uintptr_t)TaghaScript_pop_int64(script);
	double val = TaghaScript_pop_double(script);
	TaghaScript_push_double(pScript, val);
}

/* double script_pop_double(Script_t *script); */
static void native_script_pop_double(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	struct TaghaScript *restrict pScript = (struct TaghaScript *)(uintptr_t)TaghaScript_pop_int64(script);
	double val = TaghaScript_pop_double(pScript);
	TaghaScript_push_double(script, val);
}

/* void script_push_float(Script_t *script, const float val); */
static void native_script_push_float(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	struct TaghaScript *restrict pScript = (struct TaghaScript *)(uintptr_t)TaghaScript_pop_int64(script);
	float val = TaghaScript_pop_float(script);
	TaghaScript_push_float(pScript, val);
}

/* float script_pop_float(Script_t *script); */
static void native_script_pop_float(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	struct TaghaScript *restrict pScript = (struct TaghaScript *)(uintptr_t)TaghaScript_pop_int64(script);
	float val = TaghaScript_pop_float(pScript);
	TaghaScript_push_float(script, val);
}


/* void script_push_short(Script_t *script, const uint16_t val); */
static void native_script_push_short(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	struct TaghaScript *restrict pScript = (struct TaghaScript *)(uintptr_t)TaghaScript_pop_int64(script);
	uint16_t val = TaghaScript_pop_short(script);
	TaghaScript_push_short(pScript, val);
}

/* uint16_t script_pop_short(Script_t *script); */
static void native_script_pop_short(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	struct TaghaScript *restrict pScript = (struct TaghaScript *)(uintptr_t)TaghaScript_pop_int64(script);
	uint16_t val = TaghaScript_pop_short(pScript);
	TaghaScript_push_short(script, val);
}


/* void script_push_byte(Script_t *script, const uint8_t val); */
static void native_script_push_byte(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	struct TaghaScript *restrict pScript = (struct TaghaScript *)(uintptr_t)TaghaScript_pop_int64(script);
	uint8_t val = TaghaScript_pop_byte(script);
	TaghaScript_push_byte(pScript, val);
}

/* uint8_t script_pop_byte(Script_t *script); */
static void native_script_pop_byte(struct TaghaScript *restrict script, const uint32_t argc, const uint32_t bytes)
{
	struct TaghaScript *restrict pScript = (struct TaghaScript *)(uintptr_t)TaghaScript_pop_int64(script);
	uint8_t val = TaghaScript_pop_byte(pScript);
	TaghaScript_push_byte(script, val);
}



void Tagha_load_libc_natives(struct TaghaVM *vm)
{
	if( !vm )
		return;
	
	Tagha_load_stdio_natives(vm);
	Tagha_load_stdlib_natives(vm);
}

void Tagha_load_self_natives(struct TaghaVM *vm)
{
	if( !vm )
		return;
	
	NativeInfo_t libc_self_natives[] = {
		{"get_self", native_get_script_self},
		{"script_push_int64", native_script_push_int64},
		{"script_pop_int64", native_script_pop_int64},
		{"script_push_int32", native_script_push_int32},
		{"script_pop_int32", native_script_pop_int32},
		{"script_push_double", native_script_push_double},
		{"script_pop_double", native_script_pop_double},
		{"script_push_float", native_script_push_float},
		{"script_pop_float", native_script_pop_float},
		{"script_push_short", native_script_push_short},
		{"script_pop_short", native_script_pop_short},
		{"script_push_byte", native_script_push_byte},
		{"script_pop_byte", native_script_pop_byte},
		{NULL, NULL}
	};
	Tagha_register_natives(vm, libc_self_natives);
}










