
#ifndef TAGHA_HPP_INCLUDED
#define TAGHA_HPP_INCLUDED

#include "tagha.h"


struct TaghaScript_;
struct TaghaVM_;
struct NativeInfo_;

struct TaghaScript_ {
	char m_strName[64];		// script's name
	CValue m_Regs[regsize];
	uint8_t
		*m_pMemory,			// script memory, entirely aligned by 8 bytes.
		*m_pStackSegment,	// stack segment ptr where the stack's lowest address lies.
		*m_pDataSegment,	// data segment is the address AFTER the stack segment ptr. Aligned by 8 bytes.
		*m_pTextSegment		// text segment is the address after the last global variable AKA the last opcode.
		// rip register will start at m_pMemory + 0.
	;
	char **m_pstrNativeCalls;	// natives string table.
	struct hashmap
		*m_pmapFuncs,		// stores the functions compiled to script.
		*m_pmapGlobals		// stores global vars like string literals or variables.
	;
	uint32_t
		m_uiMemsize,		// total size of m_pMemory
		m_uiInstrSize,		// size of the text segment
		m_uiMaxInstrs,		// max amount of instrs a script can execute.
		m_uiNatives,		// amount of natives the script uses.
		m_uiFuncs,			// how many functions the script has.
		m_uiGlobals			// how many globals variables the script has.
	;
	bool	m_bSafeMode : 1;	// does the script want bounds checking?
	bool	m_bDebugMode : 1;	// print debug info.
	bool	m_bZeroFlag : 1;	// conditional zero flag.
	
	void Delete();
	void PrintPtrs();
	void PrintStack();
	void PrintData();
	void PrintInstrs();
	void PrintRegData();
	void Reset();
	void *GetGlobalByName(const char *strGlobalName);
	bool BindGlobalPtr(const char *strGlobalName, void *pVar);
	void PushValue(const CValue value);
	CValue PopValue();
	uint32_t GetMemSize();
	uint32_t GetInstrSize();
	uint32_t GetMaxInstrs();
	uint32_t GetNativeCount();
	uint32_t GetFuncCount();
	uint32_t GetGlobalsCount();
	bool IsSafemodeActive();
	bool IsDebugActive();
};

struct TaghaVM_ {
	struct TaghaScript	*m_pScript;
	
	// native C/C++ interface hashmap.
	// stores a C/C++ function ptr using the script-side name as the key.
	struct hashmap		*m_pmapNatives;
	
	TaghaVM_(void);
	void Delete();
	void LoadScriptByName(char *filename);
	bool RegisterNatives(NativeInfo_ arrNatives[]);
	int32_t CallScriptFunc(const char *strFunc);
	TaghaScript_ *GetScript();
	void SetScript(TaghaScript_ *script);
	void Exec(int argc, CValue argv[]);
	void LoadLibCNatives();
	void LoadSelfNatives();
};

TaghaScript_ *TaghaScriptBuildFromFile(const char *filename);


typedef		void (*fnNative_)(struct TaghaScript_ *, union CValue [], union CValue *, const uint32_t, struct TaghaVM_ *);

struct NativeInfo_ {
	const char	*strName;	// use as string literals
	fnNative_	pFunc;
};


#endif	// TAGHA_HPP_INCLUDED

