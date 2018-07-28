#!/bin/bash
cd "$(dirname "$0")"
./C-Data-Structure-Collection/build_libCDSC_static.sh
mv C-Data-Structure-Collection/libDataStructColl.a -t "$(dirname "$0")"
gcc -Wall -Wextra -O2 -std=c99 -s tagha_assembler.c -L. -lDataStructColl -o tasm
#gcc -Wall -Wextra -g -O2 -std=c99 -s tagha_llvmir2tasm.c -L. -lDataStructColl -o llvm2tagha
