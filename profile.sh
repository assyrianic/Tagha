#!/bin/bash
cd "$(dirname "$0")"

gcc -Wall -Wextra -Os -pg -std=c99 tagha_api.c hostapp.c -o taghavm

./taghavm "test_fib.tbc"
gprof taghavm gmon.out > Tagha_profile.txt

rm gmon.out
