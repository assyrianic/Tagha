#include <stdlib.h>
#include "tagha_backend.h"


static uint32_t FloatToInt(const float fval)
{
	union {
		float f;
		uint32_t i;
	} conv;
	conv.f = fval;
	return conv.i;
}

static uint64_t DoubleToLong(const double fval)
{
	union {
		double f;
		uint64_t i;
	} conv;
	conv.f = fval;
	return conv.i;
}

static void TBC_Resize(struct TaghaTBC *restrict const pTBC)
{
	if( !pTBC )
		return;
	
}

struct TaghaTBC *TBCGen_New()
{
	return calloc(1, sizeof(struct TaghaTBC));
}

void TBCGen_Init(struct TaghaTBC *restrict const pTBC)
{
	if( !pTBC )
		return;
	
	*pTBC = (struct TaghaTBC){0};
}
