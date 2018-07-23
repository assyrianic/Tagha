#!/bin/bash
cd "$(dirname "$0")"
gcc -Wall -Wextra -std=c99 -g -O3 test_hostapp.c -L. -ltagha -o taghavm_hosttest
g++ -Wall -Wextra -std=c++11 -s -O3 test_hostapp.cpp -L. -ltaghacpp -ltagha -o taghavm_hosttest_cpp
#clang-6.0 -Wall -Wextra -std=c99 -O2 test_hostapp.c -L. -ltaghaclang -o taghavmclang
#clang-6.0 -S -emit-llvm tagha_api.c
