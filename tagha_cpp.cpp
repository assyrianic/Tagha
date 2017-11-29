#include "tagha.hpp"


void TaghaScript_::del()
{
	TaghaScript_free((struct TaghaScript *)this);
}

void TaghaScript_::debug_print_ptrs()
{
	TaghaScript_debug_print_ptrs((struct TaghaScript *)this);
}

void TaghaScript_::debug_print_memory()
{
	TaghaScript_debug_print_memory((struct TaghaScript *)this);
}

void TaghaScript_::debug_print_instrs()
{
	TaghaScript_debug_print_instrs((struct TaghaScript *)this);
}

void TaghaScript_::reset()
{
	TaghaScript_reset((struct TaghaScript *)this);
}

void *TaghaScript_::get_global_by_name(const char *strGlobalName)
{
	return TaghaScript_get_global_by_name((struct TaghaScript *)this, strGlobalName);
}

bool TaghaScript_::bind_global_ptr(const char *strGlobalName, void *pVar)
{
	return TaghaScript_bind_global_ptr((struct TaghaScript *)this, strGlobalName, pVar);
}

void TaghaScript_::push_value(const Val_t value)
{
	TaghaScript_push_value((struct TaghaScript *)this, value);
}

Val_t TaghaScript_::pop_value()
{
	return TaghaScript_pop_value((struct TaghaScript *)this);
}

uint32_t TaghaScript_::memsize()
{
	return TaghaScript_memsize((struct TaghaScript *)this);
}

uint32_t TaghaScript_::instrsize()
{
	return TaghaScript_instrsize((struct TaghaScript *)this);
}

uint32_t TaghaScript_::maxinstrs()
{
	return TaghaScript_maxinstrs((struct TaghaScript *)this);
}

uint32_t TaghaScript_::nativecount()
{
	return TaghaScript_nativecount((struct TaghaScript *)this);
}

uint32_t TaghaScript_::funccount()
{
	return TaghaScript_funcs((struct TaghaScript *)this);
}

uint32_t TaghaScript_::globalcount()
{
	return TaghaScript_globals((struct TaghaScript *)this);
}

bool TaghaScript_::safemode_active()
{
	return TaghaScript_safemode_active((struct TaghaScript *)this);
}

bool TaghaScript_::debug_active()
{
	return TaghaScript_debug_active((struct TaghaScript *)this);
}


TaghaVM_::TaghaVM_()
{
	Tagha_init((struct TaghaVM *)this);
}

void TaghaVM_::del()
{
	Tagha_free((struct TaghaVM *)this);
}

void TaghaVM_::load_script_by_name(char *filename)
{
	Tagha_load_script_by_name((struct TaghaVM *)this, filename);
}

bool TaghaVM_::register_natives(NativeInfo_ arrNatives[])
{
	return Tagha_register_natives((struct TaghaVM *)this, (struct NativeInfo *)arrNatives);
}

int32_t TaghaVM_::call_script_func(const char *strFunc)
{
	return Tagha_call_script_func((struct TaghaVM *)this, strFunc);
}

TaghaScript_ *TaghaVM_::get_script()
{
	return (TaghaScript_ *)Tagha_get_script((struct TaghaVM *)this);
}

void TaghaVM_::set_script(TaghaScript_ *script)
{
	Tagha_set_script((struct TaghaVM *)this, (struct TaghaScript *)script);
}

void TaghaVM_::exec(uint8_t *oldbp)
{
	Tagha_exec((struct TaghaVM *)this, oldbp);
}

void TaghaVM_::load_libc_natives()
{
	Tagha_load_libc_natives((struct TaghaVM *)this);
}

void TaghaVM_::load_self_natives()
{
	Tagha_load_self_natives((struct TaghaVM *)this);
}





