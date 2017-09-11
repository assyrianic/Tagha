#!/bin/bash
cd "$(dirname "$0")"
clang-3.5	-std=c11 -Os -fPIC -c vector.c hashmap.c taghaapi.c tagha.c
clang-3.5	-shared -Wl,-soname,libtagha_clang.so.1 -o libtagha_clang.so.1.0.0 vector.o hashmap.o taghaapi.o tagha.o

gcc			-std=c11 -Os -fPIC -c vector.c hashmap.c taghaapi.c tagha.c
gcc			-shared -Wl,-soname,libtagha_gcc.so.1 -o libtagha_gcc.so.1.0.0 vector.o hashmap.o taghaapi.o tagha.o

rm	vector.o hashmap.o taghaapi.o tagha.o
