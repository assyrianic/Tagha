#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "dsc.h"

#define LLVM2TAGHA_DEBUG

struct LLVMLexer {
	
};

struct LLVMTree {
	
};


struct LLVMTaghaTranspiler {
	struct LinkMap GlobalVarTable; // 
	struct LinkMap FuncTable; // stores function names as confirmation of their existence.
	struct LinkMap TypeTable; // stores type sizes.
	struct String OutputName, *Lexeme;
	FILE *Src, *Dest;
	char *Iter;
	size_t CurrLine;
};

//void LLVM2Tagha_Init(struct LLVMTaghaTranspiler *, const char *);
void LLVM2Tagha_Transpile(struct LLVMTaghaTranspiler *);


#ifdef __cplusplus
}
#endif
