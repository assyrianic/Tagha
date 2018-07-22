#!/bin/bash
cd "$(dirname "$0")"
gcc -Wall -Wextra -O2 -std=c99 -s tagha_assembler.c -L. -lDataStructColl -o tasm
