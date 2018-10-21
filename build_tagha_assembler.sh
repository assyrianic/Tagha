#!/bin/bash
cd "$(dirname "$0")"

gcc -Wall -Wextra -O2 -std=c99 -s C-Data-Structure-Collection/stringobj.c C-Data-Structure-Collection/vector.c C-Data-Structure-Collection/hashmap.c C-Data-Structure-Collection/bytebuffer.c C-Data-Structure-Collection/linkmap.c tagha_assembler/tagha_assembler.c -o tagha_asm
