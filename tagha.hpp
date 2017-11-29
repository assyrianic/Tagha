
#ifndef TAGHA_HPP_INCLUDED
#define TAGHA_HPP_INCLUDED

#include "tagha.h"


struct TaghaScript_;
struct TaghaVM_;
struct NativeInfo_;

struct TaghaScript_ {
	char m_strName[64];	// script's name
	uint8_t
		*m_pMemory,	// stack and data stream. Used for stack and data segment
		*m_pText	// instruction stream.
	;
	union CValue m_Regs[regsize];
	char	**m_pstrNatives;	// natives table as stored strings.
	struct hashmap
		*m_pmapFuncs,	// stores the functions compiled to script.
		*m_pmapGlobals	// stores global vars like string literals or variables.
	;
	uint32_t
		m_uiMemsize,	// size of m_pMemory
		m_uiInstrSize,	// size of m_pText
		m_uiMaxInstrs,	// max amount of instrs a script can execute.
		m_uiNatives,	// amount of natives the script uses.
		m_uiFuncs,	// how many functions the script has.
		m_uiGlobals	// how many globals variables the script has.
	;
	bool	m_bSafeMode : 1;	// does the script want bounds checking?
	bool	m_bDebugMode : 1;	// print debug info.
	uint8_t	m_ucZeroFlag : 1;	// conditional zero flag.
	
	void del();
	void debug_print_ptrs();
	void debug_print_memory();
	void debug_print_instrs();
	void reset();
	void *get_global_by_name(const char *strGlobalName);
	bool bind_global_ptr(const char *strGlobalName, void *pVar);
	void push_value(const Val_t value);
	Val_t pop_value();
	uint32_t memsize();
	uint32_t instrsize();
	uint32_t maxinstrs();
	uint32_t nativecount();
	uint32_t funccount();
	uint32_t globalcount();
	bool safemode_active();
	bool debug_active();
};

struct TaghaVM_ {
	struct TaghaScript	*m_pScript;
	struct hashmap		*m_pmapNatives;
	
	TaghaVM_(void);
	void del();
	void load_script_by_name(char *filename);
	bool register_natives(NativeInfo_ arrNatives[]);
	int32_t call_script_func(const char *strFunc);
	TaghaScript_ *get_script();
	void set_script(TaghaScript_ *script);
	void exec(uint8_t *oldbp);
	void load_libc_natives();
	void load_self_natives();
};


typedef		void (*fnNative_)(struct TaghaScript_ *, union CValue [], union CValue *, const uint32_t, struct TaghaVM_ *);

struct NativeInfo_ {
	const char	*strName;	// use as string literals
	fnNative_	pFunc;
};


#endif	// TAGHA_HPP_INCLUDED

