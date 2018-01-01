#ifndef TAGHA_BACKEND_H_INCLUDED
	#define TAGHA_BACKEND_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "tagha.h"

typedef struct TaghaTBC {
	uint32_t	m_uiLen, m_uiCount;
	uint8_t		*m_pPCode;
} TaghaTBC;

struct TaghaTBC	*TBCGen_New(void);
void	TBCGen_Init(struct TaghaTBC *);
void	TBCGen_WriteHeader(struct TaghaTBC *, uint32_t, uint32_t, uint8_t);
void	TBCGen_WriteNativeTable(struct TaghaTBC *, uint32_t, ...);
void	TBCGen_WriteFuncTable(struct TaghaTBC *, uint32_t, ...);
void	TBCGen_WriteGlobalVarTable(struct TaghaTBC *, uint32_t, ...);
void	TBCGen_WriteIntToDataSeg(struct TaghaTBC *, uint64_t, size_t);
void	TBCGen_WriteStrToDataSeg(struct TaghaTBC *, char *, size_t);

void	TBCGen_WriteOpcodeNoOpers(struct TaghaTBC *, enum InstrSet, enum AddrMode);
void	TBCGen_WriteOpcodeOneOper(struct TaghaTBC *, enum InstrSet, enum AddrMode, uint64_t, uint32_t *);
void	TBCGen_WriteOpcodeTwoOpers(struct TaghaTBC *, enum InstrSet, enum AddrMode, uint64_t, uint64_t, uint32_t *);
void	TBCGen_WriteNativeCall(struct TaghaTBC *, enum AddrMode, uint64_t, uint32_t *, uint32_t);

void	TBCGen_ToFile(struct TaghaTBC *, const char *);

#ifdef __cplusplus
}
#endif

#endif	// TAGHA_BACKEND_H_INCLUDED
