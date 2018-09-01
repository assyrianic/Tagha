#!/bin/bash
cd "$(dirname "$0")"

gcc		-Wextra -Wall -std=c99 -s -O3 -funroll-loops -finline-functions -ffast-math -fexpensive-optimizations -c	tagha_api.c #-fno-guess-branch-probability -fomit-frame-pointer

ar			cr libtagha.a	tagha_api.o

gcc -Wextra -Wall -std=c99 -s -O3 test_hostapp.c -L. -ltagha -o taghavmgcc_hosttest



clang-6.0		-Wextra -Wall -std=c99 -s -O3 -funroll-loops -finline-functions -ffast-math -fexpensive-optimizations -c	tagha_api.c #-fno-guess-branch-probability -fomit-frame-pointer

ar			cr libtagha.a	tagha_api.o

clang-6.0 -Wextra -Wall -std=c99 -s -O3 test_hostapp.c -L. -ltagha -o taghavmclang_hosttest

#gcc		-Wextra -Wall -std=c99 -s -O2 -funroll-loops -finline-functions -ffast-math -fexpensive-optimizations -S -masm=intel tagha_api.c


rm	tagha_api.o

#test_fib.tbc
#test_native_number
#test_global
./taghavmgcc_hosttest "test_fib.tbc"
gprof taghavmgcc_hosttest gmon.out > Tagha_profile.txt
rm gmon.out

./taghavmclang_hosttest "test_fib.tbc"
gprof taghavmclang_hosttest gmon.out > Tagha_profile.txt
rm gmon.out
