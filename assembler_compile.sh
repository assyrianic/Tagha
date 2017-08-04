#!/bin/bash
cd "$(dirname "$0")"

gcc -std=c11 -Ofast assembler.c -o bit
#-S for asm output
