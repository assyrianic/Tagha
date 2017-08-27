#!/bin/bash
cd "$(dirname "$0")"
clang-3.5	-std=c11 hashmap.c taghaapi.c tagha.c hostapp.c -o TaghaVM_Clang
gcc			-std=c11 hashmap.c taghaapi.c tagha.c hostapp.c -o TaghaVM_GCC
#-S for asm output, -g for debug, -Os for optimization by size, -Ofast		-pg profiling
# -Wall

#./TaghaVM_GCC "test bytecode/test_recursion.tbc"
#gprof TaghaVM_GCC gmon.out > Tagha_profile.txt
