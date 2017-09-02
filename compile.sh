#!/bin/bash
cd "$(dirname "$0")"
clang-3.5	-g -std=c11 hashmap.c taghaapi.c tagha.c hostapp.c -o Tagha_Clang
gcc			-g -std=c11 hashmap.c taghaapi.c tagha.c hostapp.c -o Tagha_GCC
#-S for asm output, -g for debug, -Os for optimization by size, -Ofast		-pg profiling
# -Wall

#./TaghaVM_GCC "test bytecode/test_recursion.tbc"
#gprof TaghaVM_GCC gmon.out > Tagha_profile.txt
