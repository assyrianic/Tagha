#include "tagha.h"

CTagha::CTagha(void *script)
{
	Tagha_Init((struct Tagha *)this, script);
}

CTagha::CTagha(void *script, struct CNativeInfo natives[])
{
	Tagha_Init((struct Tagha *)this, script);
	Tagha_RegisterNatives((struct Tagha *)this, (struct NativeInfo *)natives);
}
CTagha::~CTagha()
{
	Tagha_Del((struct Tagha *)this);
}

bool CTagha::RegisterNatives(struct CNativeInfo natives[])
{
	return Tagha_RegisterNatives((struct Tagha *)this, (struct NativeInfo *)natives);
}
void *CTagha::GetGlobalVarByName(const char *varname)
{
	return Tagha_GetGlobalVarByName((struct Tagha *)this, varname);
}

int32_t CTagha::CallFunc(const char *funcname, const size_t args, union Value params[])
{
	return Tagha_CallFunc((struct Tagha *)this, funcname, args, params);
}

union Value CTagha::GetReturnValue()
{
	return Tagha_GetReturnValue((struct Tagha *)this);
}

int32_t CTagha::RunScript(int32_t argc, char *argv[])
{
	return Tagha_RunScript((struct Tagha *)this, argc, argv);
}

/////////////////////////////////////////////////////////////////////////////////
