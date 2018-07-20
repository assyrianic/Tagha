#!/bin/bash
cd "$(dirname "$0")"
clang-3.5 -Wall -Wextra -std=c99 -O2 test_hostapp.c -L. -ltagha -o taghavm_hosttest
clang-3.5 -Wall -Wextra -std=c++11 -O2 test_hostapp.cpp -L. -ltaghacpp -ltagha -o taghavm_hosttest_cpp
#clang-3.5 -Wall -Wextra -std=c99 -O2 test_hostapp.c -L. -ltaghaclang -o taghavmclang
#clang-3.5 -S -emit-llvm tagha_api.c
