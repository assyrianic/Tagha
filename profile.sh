#!/bin/bash
cd "$(dirname "$0")"

#gcc -Wall -Wextra -pg -O3 -funroll-loops -finline-functions -fno-math-errno -fexpensive-optimizations -std=c99 tagha_api.c hostapp.c -o taghavm

clang-6.0		-Wextra -Wall -std=c99 -g -O3 -funroll-loops -finline-functions -ffast-math -fexpensive-optimizations -c	tagha_api.c #-fno-guess-branch-probability -fomit-frame-pointer
ar			cr libtagha.a	tagha_api.o

#gcc		-Wextra -Wall -std=c99 -pg -O2 -funroll-loops -finline-functions -ffast-math -fexpensive-optimizations -S -masm=intel tagha_api.c

#gcc		-Wextra -Wall -std=c99 -Os -S -masm=intel tagha_api.c

rm	tagha_api.o

clang-6.0 -Wextra -Wall -std=c99 -g -O3 test_hostapp.c -L. -ltagha -o taghavm_hosttest


./taghavm_hosttest "test_fib.tbc"
gprof taghavm_hosttest gmon.out > Tagha_profile.txt
rm gmon.out
