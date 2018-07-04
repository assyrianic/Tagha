#!/bin/bash
cd "$(dirname "$0")"
gcc -Wall -Wextra -O3 -std=c99 -g tagha_assembler.c -L. -ltagha -lDataStructColl -o tasm
