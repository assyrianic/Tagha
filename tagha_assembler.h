#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "tagha.h"

#define DEBUG	1

struct LabelInfo {
	uint64_t Addr;
	bool IsFunc : 1;
	bool IsNativeFunc : 1;
};

struct TaghaAsmbler {
	struct Vector Instrs;
	struct String OutputName, *Lexeme;
	struct LinkMap
		*LabelTable, *VarTable,
		*Opcodes, *Registers
	;
	struct ByteBuffer *Bytecode;
	FILE *Src;
	char *Iter;
	size_t SrcSize, ProgramCounter, CurrLine;
	uint32_t Stacksize;
};

bool TaghaAsm_Assemble(struct TaghaAsmbler *);
bool TaghaAsm_Del(struct TaghaAsmbler *);


#ifdef __cplusplus
}
#endif
