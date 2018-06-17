#!/bin/bash
cd "$(dirname "$0")"
gcc -Wall -Wextra -O3 -g -std=c99 tagha_api.c hostapp.c -L. -lDataStructColl -o taghavm
#clang-3.5 -S -emit-llvm tagha_api.c
