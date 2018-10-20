#!/bin/bash
cd "$(dirname "$0")"

# -fsanitize=address -fsanitize=undefined -fstrict-aliasing
# -funroll-loops -finline-functions -ffast-math -fexpensive-optimizations


gcc		-Wextra -Wall -std=c99 -s -O2 -c	tagha_api.c  #-fno-guess-branch-probability -fomit-frame-pointer
#gcc		-Wextra -Wall -std=c99 -s -O2 -S -masm=intel tagha_api.c

ar			cr libtagha.a	tagha_api.o

gcc -Wextra -Wall -std=c99 -s -O2 test_hostapp.c -L. -ltagha -o taghavmgcc_hosttest



clang-6.0		-Wextra -Wall -std=c99 -s -O3 -c	tagha_api.c #-fno-guess-branch-probability -fomit-frame-pointer -funroll-loops -finline-functions -ffast-math -fexpensive-optimizations

ar			cr libtagha.a	tagha_api.o

clang-6.0 -Wextra -Wall -std=c99 -s -O3 test_hostapp.c -L. -ltagha -o taghavmclang_hosttest

#gcc		-Wextra -Wall -std=c99 -s -O2 -funroll-loops -finline-functions -ffast-math -fexpensive-optimizations -S -masm=intel tagha_api.c


rm	tagha_api.o

./taghavmgcc_hosttest "test_fib.tbc"
gprof taghavmgcc_hosttest gmon.out > Tagha_profile_GCC.txt
rm gmon.out

./taghavmclang_hosttest "test_fib.tbc"
gprof taghavmclang_hosttest gmon.out > Tagha_profile_Clang.txt
rm gmon.out
