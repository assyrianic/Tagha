
#ifndef TAGHA_HPP_INCLUDED
#define TAGHA_HPP_INCLUDED

#include "tagha.h"


struct TaghaScriptCPP;
struct TaghaVMCPP;

struct TaghaScriptCPP {
	Script_t *script;
	
	TaghaScriptCPP();
	TaghaScriptCPP(Script_t *);
	void del();
	void debug_print_ptrs();
	void debug_print_memory();
	void debug_print_instrs();
	void reset();
	void *get_global_by_name(const char *strGlobalName);
	void push_value(const Val_t value);
	Val_t pop_value();
	uint32_t stacksize();
	uint32_t instrsize();
	uint32_t maxinstrs();
	uint32_t nativecount();
	uint32_t funccount();
	uint32_t globalcount();
	bool safemode_active();
	bool debug_active();
};

struct TaghaVMCPP {
	TaghaVM_t *vm;
	
	TaghaVMCPP();
	TaghaVMCPP(TaghaVM_t *);
	void del();
	void load_script_by_name(char *filename);
	bool register_natives(NativeInfo_t arrNatives[]);
	void call_script_func(const char *strFunc);
	Script_t *get_script();
	void set_script(Script_t *script);
	void exec(uint8_t *oldbp);
	void load_libc_natives();
	void load_self_natives();
};

#endif	// TAGHA_HPP_INCLUDED

