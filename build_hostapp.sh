#!/bin/bash
cd "$(dirname "$0")"
clang-3.5	hostapp.c -L. -ltagha -o TaghaClang
gcc			hostapp.c -L. -ltagha -o TaghaGCC
clang-3.5	hostapp_sharedlib.c -o TaghaSharedClang -ldl
gcc			hostapp_sharedlib.c -o TaghaSharedGCC -ldl
#-S for asm output, -g for debug, -Os for optimization by size, -Ofast		-pg profiling
# -Wall

#./TaghaGCC "test bytecode/test_recursion.tbc"
#gprof TaghaGCC gmon.out > Tagha_profile.txt
