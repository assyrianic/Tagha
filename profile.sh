#!/bin/bash
cd "$(dirname "$0")"

gcc -Wall -Wextra -pg -O3 -funroll-loops -finline-functions -fno-math-errno -fexpensive-optimizations -std=c99 tagha_api.c hostapp.c -o taghavm

./taghavm "test_fib.tbc"
gprof taghavm gmon.out > Tagha_profile.txt

rm gmon.out
