#!/bin/bash
cd "$(dirname "$0")"

#gcc -Wall -Wextra -pg -O3 -funroll-loops -finline-functions -fno-math-errno -fexpensive-optimizations -std=c99 tagha_api.c hostapp.c -o taghavm

gcc			-Wextra -Wall -std=c99 -pg -O3 -funroll-loops -finline-functions -ffast-math -fexpensive-optimizations -c	tagha_api.c #-fno-guess-branch-probability -fomit-frame-pointer
ar			cr libtaghagcc.a	tagha_api.o

#gcc		-Wextra -Wall -std=c99 -Os -S -masm=intel tagha_api.c

rm	tagha_api.o

gcc -Wall -Wextra -std=c99 -pg -O3 hostapp.c -L. -ltaghagcc -o taghavm


./taghavm "test_fib.tbc"
gprof taghavm gmon.out > Tagha_profile.txt

rm gmon.out
