#!/bin/bash
cd "$(dirname "$0")"
clang-3.5	-std=c11 -Os -fPIC -c tagha_libc.c
clang-3.5	-shared -Wl,-soname, tagha_libc.so.1 -o libtagha_libc_clang.so.1.0.0 tagha_libc.o

gcc			-std=c11 -Os -fPIC -c tagha_libc.c
gcc			-shared -Wl,-soname, tagha_libc.so.1 -o libtagha_libc_gcc.so.1.0.0 tagha_libc.o

