#!/bin/bash
cd "$(dirname "$0")"
gcc -Wall -Wextra -std=c99 -O2 hostapp.c -L. -ltagha -o taghavm_hosttest
g++ -Wall -Wextra -O2 -std=c++11 hostapp.cpp -L. -ltaghacpp -ltagha -o taghavm_hosttest_cpp
#clang-3.5 -Wall -Wextra -std=c99 -O2 hostapp.c -L. -ltaghaclang -o taghavmclang
#clang-3.5 -S -emit-llvm tagha_api.c
