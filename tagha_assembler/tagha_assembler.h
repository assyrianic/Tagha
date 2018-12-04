#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "../libharbol/harbol.h"
#include "../tagha.h"

//#define TASM_DEBUG

struct LabelInfo {
	struct HarbolByteBuffer Bytecode;
	uint64_t Addr;
	bool IsFunc : 1;
	bool IsNativeFunc : 1;
};

bool label_free(void *);

struct TaghaAsmbler {
	struct HarbolString OutputName, *ActiveFuncLabel, *Lexeme;
	struct HarbolLinkMap
		*LabelTable, *FuncTable, *VarTable,
		*Opcodes, *Registers
	;
	FILE *Src;
	char *Iter;
	size_t SrcSize, ProgramCounter, CurrLine;
	uint32_t Stacksize;
	uint8_t Safemode : 1;
};

bool tagha_asm_parse_RegRegInstr(struct TaghaAsmbler *, bool);
bool tagha_asm_parse_OneRegInstr(struct TaghaAsmbler *, bool);
bool tagha_asm_parse_OneImmInstr(struct TaghaAsmbler *, bool);
bool tagha_asm_parse_RegMemInstr(struct TaghaAsmbler *, bool);
bool tagha_asm_parse_MemRegInstr(struct TaghaAsmbler *, bool);
bool tagha_asm_parse_RegImmInstr(struct TaghaAsmbler *, bool);

bool tagha_asm_assemble(struct TaghaAsmbler *);


#ifdef __cplusplus
}
#endif
