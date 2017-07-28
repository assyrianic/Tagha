#!/bin/bash
cd "$(dirname "$0")"
clang-3.5	-g -std=c11 -Os vm.c -o CrownVM_Clang
gcc		-g -std=c11 -Os vm.c -o CrownVM_GCC
#-S for asm output, -g for debug, -Os for optimization by size		-pg profiling
# -Wall

#./CrownVM_GCC
#gprof CrownVM_GCC gmon.out > CVM_profile.txt
