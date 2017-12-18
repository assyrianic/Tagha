
#ifndef TAGHA_HPP_INCLUDED
#define TAGHA_HPP_INCLUDED

#include "tagha.h"


struct Tagha_;
struct NativeInfo_;

struct Tagha_ {
	union CValue m_Regs[regsize];
	uint8_t
		*m_pMemory,			// script memory, entirely aligned by 8 bytes.
		*m_pStackSegment,	// stack segment ptr where the stack's lowest address lies.
		*m_pDataSegment,	// data segment is the address AFTER the stack segment ptr. Aligned by 8 bytes.
		*m_pTextSegment		// text segment is the address after the last global variable AKA the last opcode.
		// rip register will start at m_pMemory + 0.
	;
	// stores a C/C++ function ptr using the script-side name as the key.
	char **m_pstrNativeCalls;		// natives string table.
	struct hashmap *m_pmapNatives;	// native C/C++ interface hashmap.
	
	union CValue *m_pArgv;	// forcing char** to 8 bytes
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
	int32_t m_iArgc;
	bool
		m_bSafeMode : 1,	// does the script want bounds checking?
		m_bDebugMode : 1,	// print debug info.
		m_bZeroFlag : 1		// conditional zero flag.
	;
	
	
	Tagha_(void);
	void PrintPtrs(void);
	void PrintStack(void);
	void PrintData(void);
	void PrintInstrs(void);
	void PrintRegData(void);
	void Reset(void);
	void *GetGlobalByName(const char *strGlobalName);
	bool BindGlobalPtr(const char *strGlobalName, void *pVar);
	void PushValue(const CValue value);
	CValue PopValue(void);
	void SetCmdArgs(char *argv[]);
	
	uint32_t GetMemSize(void);
	uint32_t GetInstrSize(void);
	uint32_t GetMaxInstrs(void);
	uint32_t GetNativeCount(void);
	uint32_t GetFuncCount(void);
	uint32_t GetGlobalsCount(void);
	bool IsSafemodeActive(void);
	bool IsDebugActive(void);
	
	void Delete(void);
	void LoadScriptByName(char *filename);
	void BuildFromFile(const char *filename);
	bool RegisterNatives(NativeInfo_ arrNatives[]);
	int32_t RunScript(void);
	int32_t CallFunc(const char *strFunc);
	void LoadLibCNatives(void);
	void LoadSelfNatives(void);
};


typedef		void (*fnNative_)(struct Tagha_ *, union CValue [], union CValue *, const uint32_t);

struct NativeInfo_ {
	const char	*strName;	// use as string literals
	fnNative_	pFunc;
};


#endif	// TAGHA_HPP_INCLUDED

