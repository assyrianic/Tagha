#pragma once

#include "../tagha/tagha.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef TAGHA_LIBC_FUNCS
#	define TAGHA_LIBC_FUNCS
bool tagha_module_load_stdio_natives(struct TaghaModule *module);
bool tagha_module_load_ctype_natives(struct TaghaModule *module);
bool tagha_module_load_stdlib_natives(struct TaghaModule *module);
bool tagha_module_load_string_natives(struct TaghaModule *module);
bool tagha_module_load_time_natives(struct TaghaModule *module);
bool tagha_module_load_module_natives(struct TaghaModule *module);
#endif

#ifdef __cplusplus
}
#endif
