#!/bin/bash
cd "$(dirname "$0")"
clang-3.5	-std=c11 -Os vm.c -o KYVM_Clang
gcc		-std=c11 -Os vm.c -o KYVM_GCC
#-S for asm output, -g for debug, -Os for optimization by size		-pg profiling
# -Wall

#./KYVM_GCC
#gprof KYVM_GCC gmon.out > CVM_profile.txt
