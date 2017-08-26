#!/bin/bash
cd "$(dirname "$0")"
clang-3.5	-std=c11 -Os hashmap.c vmapi.c vm.c hostapp.c -o TaghaVM_Clang
gcc			-std=c11 -Os hashmap.c vmapi.c vm.c hostapp.c -o TaghaVM_GCC
#-S for asm output, -g for debug, -Os for optimization by size		-pg profiling
# -Wall

#./TaghaVM_GCC
#gprof TaghaVM_GCC gmon.out > Tagha_profile.txt
