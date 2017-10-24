#!/bin/bash
cd "$(dirname "$0")"


gcc			-std=c99 -pg -Os -c		ds.c taghavm_api.c taghascript_api.c tagha_exec.c tagha_libc.c
ar			cr libtaghagcc.a	ds.o taghavm_api.o taghascript_api.o tagha_exec.o tagha_libc.o
gcc			-Os -pg hostapp.c -L. -ltaghagcc -o TaghaGCC

rm	ds.o taghavm_api.o taghascript_api.o tagha_exec.o tagha_libc.o libtaghagcc.a

#./TaghaGCC "test_recursion.tbc"
./TaghaGCC "test_factorial_recurs.tbc"
gprof TaghaGCC gmon.out > TaghaGCC_profile.txt

rm gmon.out

#./TaghaGCCg++ "test_recursion.tbc"
#gprof TaghaGCCg++ gmon.out > TaghaGCCC++_profile.txt
