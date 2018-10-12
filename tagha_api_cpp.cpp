#include "tagha.h"

CTagha::CTagha(void *script)
{
	Tagha_Init((struct Tagha *)this, script);
}

CTagha::CTagha(void *script, const struct CNativeInfo natives[])
{
	Tagha_Init((struct Tagha *)this, script);
	Tagha_RegisterNatives((struct Tagha *)this, (const struct NativeInfo *)natives);
}

CTagha::~CTagha()
{
	
}

bool CTagha::RegisterNatives(const struct CNativeInfo natives[])
{
	return Tagha_RegisterNatives((struct Tagha *)this, (const struct NativeInfo *)natives);
}

void *CTagha::GetGlobalVarByName(const char varname[])
{
	return Tagha_GetGlobalVarByName((struct Tagha *)this, varname);
}

int32_t CTagha::CallFunc(const char funcname[], const size_t args, union TaghaVal params[])
{
	return Tagha_CallFunc((struct Tagha *)this, funcname, args, params);
}

union TaghaVal CTagha::GetReturnValue()
{
	return Tagha_GetReturnValue((struct Tagha *)this);
}

int32_t CTagha::RunScript(int32_t argc, char *argv[])
{
	return Tagha_RunScript((struct Tagha *)this, argc, argv);
}

const char *CTagha::GetError()
{
	return Tagha_GetError((struct Tagha *)this);
}

void CTagha::PrintVMState()
{
	Tagha_PrintVMState((struct Tagha *)this);
}

void *CTagha::GetRawScriptPtr()
{
	return Tagha_GetRawScriptPtr((struct Tagha *)this);
}

/////////////////////////////////////////////////////////////////////////////////
