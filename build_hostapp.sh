#!/bin/bash
cd "$(dirname "$0")"
clang-3.5	-Os hostapp.c -L. -ltaghaclang -lm -o TaghaClang
gcc			-Os hostapp.c -L. -ltaghagcc -lm -o TaghaGCC

clang-3.5	-std=c++11 -Os tagha_cpp.cpp hostcppapp.cpp -L. -ltaghaclang -lm -o TaghaClang++
g++			-std=c++11 -Os tagha_cpp.cpp hostcppapp.cpp -L. -ltaghagcc -lm -o TaghaGCCg++
#-S for asm output, -g for debug, -Os for optimization by size, -Ofast		-pg profiling
# -Wall

#./TaghaGCC "test bytecode/test_recursion.tbc"
#gprof TaghaGCC gmon.out > Tagha_profile.txt
