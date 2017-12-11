#include "tagha.hpp"


void TaghaScript_::Delete()
{
	TaghaScript_Free((struct TaghaScript *)this);
}

void TaghaScript_::PrintPtrs()
{
	TaghaScript_PrintPtrs((struct TaghaScript *)this);
}

void TaghaScript_::PrintMem()
{
	TaghaScript_PrintMem((struct TaghaScript *)this);
}

void TaghaScript_::PrintInstrs()
{
	TaghaScript_PrintInstrs((struct TaghaScript *)this);
}

void TaghaScript_::Reset()
{
	TaghaScript_Reset((struct TaghaScript *)this);
}

void *TaghaScript_::GetGlobalByName(const char *strGlobalName)
{
	return TaghaScript_GetGlobalByName((struct TaghaScript *)this, strGlobalName);
}

bool TaghaScript_::BindGlobalPtr(const char *strGlobalName, void *pVar)
{
	return TaghaScript_BindGlobalPtr((struct TaghaScript *)this, strGlobalName, pVar);
}

void TaghaScript_::PushValue(const CValue value)
{
	TaghaScript_PushValue((struct TaghaScript *)this, value);
}

CValue TaghaScript_::PopValue()
{
	return TaghaScript_PopValue((struct TaghaScript *)this);
}

uint32_t TaghaScript_::GetMemSize()
{
	return TaghaScript_GetMemSize((struct TaghaScript *)this);
}

uint32_t TaghaScript_::GetInstrSize()
{
	return TaghaScript_GetInstrSize((struct TaghaScript *)this);
}

uint32_t TaghaScript_::GetMaxInstrs()
{
	return TaghaScript_GetMaxInstrs((struct TaghaScript *)this);
}

uint32_t TaghaScript_::GetNativeCount()
{
	return TaghaScript_GetNativeCount((struct TaghaScript *)this);
}

uint32_t TaghaScript_::GetFuncCount()
{
	return TaghaScript_GetFuncCount((struct TaghaScript *)this);
}

uint32_t TaghaScript_::GetGlobalsCount()
{
	return TaghaScript_GetGlobalsCount((struct TaghaScript *)this);
}

bool TaghaScript_::IsSafemodeActive()
{
	return TaghaScript_IsSafemodeActive((struct TaghaScript *)this);
}

bool TaghaScript_::IsDebugActive()
{
	return TaghaScript_IsDebugActive((struct TaghaScript *)this);
}


TaghaVM_::TaghaVM_()
{
	Tagha_Init((struct TaghaVM *)this);
}

void TaghaVM_::Delete()
{
	Tagha_Free((struct TaghaVM *)this);
}

void TaghaVM_::LoadScriptByName(char *filename)
{
	Tagha_LoadScriptByName((struct TaghaVM *)this, filename);
}

bool TaghaVM_::RegisterNatives(NativeInfo_ arrNatives[])
{
	return Tagha_RegisterNatives((struct TaghaVM *)this, (struct NativeInfo *)arrNatives);
}

int32_t TaghaVM_::CallScriptFunc(const char *strFunc)
{
	return Tagha_CallScriptFunc((struct TaghaVM *)this, strFunc);
}

TaghaScript_ *TaghaVM_::GetScript()
{
	return (TaghaScript_ *)Tagha_GetScript((struct TaghaVM *)this);
}

void TaghaVM_::SetScript(TaghaScript_ *script)
{
	Tagha_SetScript((struct TaghaVM *)this, (struct TaghaScript *)script);
}

void TaghaVM_::Exec(int argc, CValue argv[])
{
	Tagha_Exec((struct TaghaVM *)this, argc, argv);
}

void TaghaVM_::LoadLibCNatives()
{
	Tagha_LoadLibCNatives((struct TaghaVM *)this);
}

void TaghaVM_::LoadSelfNatives()
{
	Tagha_LoadSelfNatives((struct TaghaVM *)this);
}


TaghaScript_ *TaghaScriptBuildFromFile(const char *filename)
{
	return (TaghaScript_ *)TaghaScript_BuildFromFile(filename);
}



