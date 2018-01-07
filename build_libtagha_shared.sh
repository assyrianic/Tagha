#!/bin/bash
cd "$(dirname "$0")"
clang-3.5	-Wall -std=c11 -Os -fPIC -c ds.c tagha_api.c tagha_exec.c
clang-3.5	-shared -Wl,-soname,libtagha_clang.so.1 -o libtagha_clang.so.1.0.0 ds.o tagha_api.o tagha_exec.o

gcc			-Wall -std=c11 -Os -fPIC -c ds.c tagha_api.c tagha_exec.c
gcc			-shared -Wl,-soname,libtagha_gcc.so.1 -o libtagha_gcc.so.1.0.0 ds.o tagha_api.o tagha_exec.o

rm	ds.o tagha_api.o tagha_exec.o
