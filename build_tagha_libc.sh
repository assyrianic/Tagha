#!/bin/bash
cd "$(dirname "$0")"

gcc -Wall -Wextra -O2 -std=c99 -s -c libharbol/stringobj.c libharbol/vector.c libharbol/hashmap.c libharbol/mempool.c libharbol/linkmap.c tagha_libc/tagha_ctype.c tagha_libc/tagha_stdio.c tagha_libc/tagha_stdlib.c tagha_libc/tagha_string.c tagha_libc/tagha_time.c -L. -ltagha

ar		cr libtagha_libc.a stringobj.o vector.o hashmap.o mempool.o linkmap.o tagha_ctype.o tagha_stdio.o tagha_stdlib.o tagha_string.o  tagha_time.o

rm *.o
