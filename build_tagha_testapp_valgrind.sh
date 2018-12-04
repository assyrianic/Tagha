#!/bin/bash
cd "$(dirname "$0")"

gcc -Wextra -Wall -std=c99 -g -O2 -c libharbol/stringobj.c libharbol/vector.c libharbol/hashmap.c libharbol/mempool.c libharbol/linkmap.c tagha_api.c

ar cr libtagha.a stringobj.o vector.o hashmap.o mempool.o linkmap.o tagha_api.o

gcc -Wextra -Wall -std=c99 -g -O2 test_hostapp.c -L. -ltagha -o taghavmgcc_hosttest

rm *.o

valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes -v --main-stacksize=16000000 ./taghavmgcc_hosttest test_factorial.tbc
