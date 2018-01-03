#!/bin/bash
cd "$(dirname "$0")"

#gcc -g -fPIC -Wall -Werror -Wextra -pedantic *.c -shared -o liball.so

clang-3.5	-std=c99 -fPIC -Os tagha_libc.c -shared -o libtagha_libc_clang.so
gcc			-std=c99 -fPIC -Os tagha_libc.c -shared -o libtagha_libc_gcc.so

