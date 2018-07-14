#!/bin/bash
cd "$(dirname "$0")"
./build_libCDSC_static.sh
gcc -Wall -Wextra -O2 -std=c99 -g tagha_assembler.c -L. -lDataStructColl -o tasm
