#include "tagha.hpp"

TaghaScriptCPP::TaghaScriptCPP()
{
	this->script = new Script_t;
	if( !this->script )
		return;
}

TaghaScriptCPP::TaghaScriptCPP(Script_t *script)
{
	this->script = script;
}

void TaghaScriptCPP::del()
{
	TaghaScript_free(this->script);
}

void TaghaScriptCPP::debug_print_ptrs()
{
	TaghaScript_debug_print_ptrs(this->script);
}

void TaghaScriptCPP::debug_print_memory()
{
	TaghaScript_debug_print_memory(this->script);
}

void TaghaScriptCPP::debug_print_instrs()
{
	TaghaScript_debug_print_instrs(this->script);
}

void TaghaScriptCPP::reset()
{
	TaghaScript_reset(this->script);
}

void *TaghaScriptCPP::get_global_by_name(const char *strGlobalName)
{
	return TaghaScript_get_global_by_name(this->script, strGlobalName);
}

bool TaghaScriptCPP::bind_global_ptr(const char *strGlobalName, void *pVar)
{
	return TaghaScript_bind_global_ptr(this->script, strGlobalName, pVar);
}

void TaghaScriptCPP::push_value(const Val_t value)
{
	TaghaScript_push_value(this->script, value);
}

Val_t TaghaScriptCPP::pop_value()
{
	return TaghaScript_pop_value(this->script);
}

uint32_t TaghaScriptCPP::stacksize()
{
	return TaghaScript_stacksize(this->script);
}

uint32_t TaghaScriptCPP::instrsize()
{
	return TaghaScript_instrsize(this->script);
}

uint32_t TaghaScriptCPP::maxinstrs()
{
	return TaghaScript_maxinstrs(this->script);
}

uint32_t TaghaScriptCPP::nativecount()
{
	return TaghaScript_nativecount(this->script);
}

uint32_t TaghaScriptCPP::funccount()
{
	return TaghaScript_funcs(this->script);
}

uint32_t TaghaScriptCPP::globalcount()
{
	return TaghaScript_globals(this->script);
}

bool TaghaScriptCPP::safemode_active()
{
	return TaghaScript_safemode_active(this->script);
}

bool TaghaScriptCPP::debug_active()
{
	return TaghaScript_debug_active(this->script);
}




TaghaVMCPP::TaghaVMCPP()
{
	this->vm = new TaghaVM_t;
	if( !this->vm )
		return;
	
	Tagha_init(this->vm);
}

TaghaVMCPP::TaghaVMCPP(TaghaVM_t *vm)
{
	this->vm = vm;
}

void TaghaVMCPP::del()
{
	Tagha_free(this->vm);
	delete this->vm;
	this->vm = nullptr;
}

void TaghaVMCPP::load_script_by_name(char *filename)
{
	Tagha_load_script_by_name(this->vm, filename);
}

bool TaghaVMCPP::register_natives(NativeInfo_t arrNatives[])
{
	return Tagha_register_natives(this->vm, arrNatives);
}

void TaghaVMCPP::call_script_func(const char *strFunc)
{
	Tagha_call_script_func(this->vm, strFunc);
}

Script_t *TaghaVMCPP::get_script()
{
	return Tagha_get_script(this->vm);
}

void TaghaVMCPP::set_script(Script_t *script)
{
	Tagha_set_script(this->vm, script);
}

void TaghaVMCPP::exec(uint8_t *oldbp)
{
	Tagha_exec(this->vm, oldbp);
}

void TaghaVMCPP::load_libc_natives()
{
	Tagha_load_libc_natives(this->vm);
}

void TaghaVMCPP::load_self_natives()
{
	Tagha_load_self_natives(this->vm);
}





