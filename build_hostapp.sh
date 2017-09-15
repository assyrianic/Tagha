#!/bin/bash
cd "$(dirname "$0")"
clang-3.5	hostapp.c -L. -ltaghaclang -o TaghaClang
gcc			hostapp.c -L. -ltaghagcc -o TaghaGCC

clang-3.5	hostapp_sharedlib.c -o TaghaSharedClang -ldl
gcc			hostapp_sharedlib.c -o TaghaSharedGCC -ldl

clang-3.5	-std=c++11 hostcppapp.cpp -L. -ltaghaclang -o TaghaClangC++
gcc			-std=c++11 hostcppapp.cpp -L. -ltaghagcc -o TaghaGCCC++
#-S for asm output, -g for debug, -Os for optimization by size, -Ofast		-pg profiling
# -Wall

#./TaghaGCC "test bytecode/test_recursion.tbc"
#gprof TaghaGCC gmon.out > Tagha_profile.txt
