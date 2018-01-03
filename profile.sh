#!/bin/bash
cd "$(dirname "$0")"

gcc			-std=c99 -pg -Os -c		ds.c tagha_api.c tagha_exec.c
ar			cr libtaghagcc.a	ds.o tagha_api.o tagha_exec.o
gcc			-Os -pg hostapp.c -L. -ltaghagcc -o TaghaGCC

rm	ds.o tagha_api.o tagha_exec.o

#./TaghaGCC "test_infloop.tbc"
./TaghaGCC "test_infloop.tbc"
gprof TaghaGCC gmon.out > TaghaGCC_profile.txt

rm gmon.out

#./TaghaGCCg++ "test_recursion.tbc"
#gprof TaghaGCCg++ gmon.out > TaghaGCCC++_profile.txt
