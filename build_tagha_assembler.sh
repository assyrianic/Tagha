#!/bin/bash
cd "$(dirname "$0")"

gcc -Wall -Wextra -O2 -std=c99 -s libharbol/stringobj.c libharbol/vector.c libharbol/hashmap.c libharbol/bytebuffer.c libharbol/linkmap.c tagha_assembler/tagha_assembler.c -o tagha_asm
