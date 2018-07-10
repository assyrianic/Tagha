#!/bin/bash
cd "$(dirname "$0")"
gcc -Wall -Wextra -std=c99 -O2 hostapp.c -L. -ltaghagcc -o taghavmgcc
gcc -Wall -Wextra -std=c99 -O2 hostapp.c -L. -ltaghaclang -o taghavmclang
#g++ -Wall -Wextra -O2 -std=c++11 hostapp.cpp -L. -ltaghacpp -ltagha -o taghavmcpp
#clang-3.5 -S -emit-llvm tagha_api.c
