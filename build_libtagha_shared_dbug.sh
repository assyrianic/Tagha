#!/bin/bash
cd "$(dirname "$0")"
clang-3.5	-std=c11 -g -Os -fPIC -c ds.c tagha_api.c tagha_exec.c tagha_libc.c
clang-3.5	-shared -Wl,-soname,libtagha_clang.so.1 -o libtagha_clang.so.1.0.0 ds.o tagha_api.o tagha_exec.o tagha_libc.o

gcc			-std=c11 -g -Os -fPIC -c ds.c tagha_api.c tagha_exec.c tagha_libc.c
gcc			-shared -Wl,-soname,libtagha_gcc.so.1 -o libtagha_gcc.so.1.0.0 ds.o tagha_api.o tagha_exec.o tagha_libc.o

rm	ds.o tagha_api.o tagha_exec.o tagha_libc.o
