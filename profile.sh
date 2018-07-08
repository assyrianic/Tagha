#!/bin/bash
cd "$(dirname "$0")"

gcc -Wall -Wextra -Os -pg -std=c99 tagha_api.c hostapp.c -L. -lDataStructColl -o taghavm

./taghavm "test_factorial.tbc"
gprof taghavm gmon.out > Tagha_profile.txt

rm gmon.out
