#!/bin/bash
cd "$(dirname "$0")"
clang-3.8	-std=c99 -Os -c 	ds.c taghavm_api.c taghascript_api.c tagha_exec.c tagha_libc.c
ar			cr libtaghaclang.a	ds.o taghavm_api.o taghascript_api.o tagha_exec.o tagha_libc.o

gcc			-std=c99 -Os -c		ds.c taghavm_api.c taghascript_api.c tagha_exec.c tagha_libc.c
ar			cr libtaghagcc.a	ds.o taghavm_api.o taghascript_api.o tagha_exec.o tagha_libc.o

rm	ds.o taghavm_api.o taghascript_api.o tagha_exec.o tagha_libc.o
