#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "tagha.h"

//#define TASM_DEBUG

struct LabelInfo {
	struct ByteBuffer Bytecode;
	uint64_t Addr;
	bool IsFunc : 1;
	bool IsNativeFunc : 1;
};

bool Label_Free(struct LabelInfo **);

struct TaghaAsmbler {
	struct String OutputName, *ActiveFuncLabel, *Lexeme;
	struct LinkMap
		*LabelTable, *FuncTable, *VarTable,
		*Opcodes, *Registers
	;
	FILE *Src;
	char *Iter;
	size_t SrcSize, ProgramCounter, CurrLine;
	uint32_t Stacksize;
};

bool TaghaAsm_Assemble(struct TaghaAsmbler *);


#ifdef __cplusplus
}
#endif
