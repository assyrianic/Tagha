#!/bin/bash
cd "$(dirname "$0")"

#gcc -g -fPIC -Wall -Werror -Wextra -pedantic *.c -shared -o liball.so

clang-3.5	-Wall -std=c99 -fPIC -Os tagha_libc.c -shared -o libtagha_libc_clang.so
gcc			-Wall -std=c99 -fPIC -Os tagha_libc.c -shared -o libtagha_libc_gcc.so

