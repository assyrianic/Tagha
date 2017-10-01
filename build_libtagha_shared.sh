#!/bin/bash
cd "$(dirname "$0")"
clang-3.5	-std=c11 -O3 -fPIC -c vector.c hashmap.c taghaapi.c tagha.c tagha_libc.c
clang-3.5	-shared -Wl,-soname,libtagha_clang.so.1 -o libtagha_clang.so.1.0.0 vector.o hashmap.o taghaapi.o tagha.o tagha_libc.o

gcc			-std=c11 -O3 -fPIC -c vector.c hashmap.c taghaapi.c tagha.c tagha_libc.c
gcc			-shared -Wl,-soname,libtagha_gcc.so.1 -o libtagha_gcc.so.1.0.0 vector.o hashmap.o taghaapi.o tagha.o tagha_libc.o

rm	vector.o hashmap.o taghaapi.o tagha.o tagha_libc.o
