#!/bin/bash
cd "$(dirname "$0")"
gcc -Wall -Wextra -O3 -std=c99 hostapp.c -L. -ltagha -lDataStructColl -o taghavm
g++ -Wall -Wextra -O3 -std=c++11 hostapp.cpp -L. -ltaghacpp -ltagha -lDataStructColl -o taghavmcpp
#clang-3.5 -S -emit-llvm tagha_api.c
