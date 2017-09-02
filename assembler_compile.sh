#!/bin/bash
cd "$(dirname "$0")"

gcc -g -std=c11 -Ofast assembler.c -o tagha_bytecode_gen
#-S for asm output
