#!/bin/bash
cd "$(dirname "$0")"

gcc	-Wall -Wextra -std=c99 -g -O2 -c 	C-Data-Structure-Collection/stringobj.c C-Data-Structure-Collection/vector.c C-Data-Structure-Collection/hashmap.c C-Data-Structure-Collection/unilist.c C-Data-Structure-Collection/bilist.c C-Data-Structure-Collection/bytebuffer.c C-Data-Structure-Collection/tuple.c C-Data-Structure-Collection/mempool.c C-Data-Structure-Collection/graph.c C-Data-Structure-Collection/tree.c C-Data-Structure-Collection/linkmap.c

ar	cr libDataStructColl.a		stringobj.o vector.o hashmap.o unilist.o bilist.o bytebuffer.o tuple.o mempool.o graph.o tree.o linkmap.o

rm	stringobj.o vector.o hashmap.o unilist.o bilist.o bytebuffer.o tuple.o mempool.o graph.o tree.o linkmap.o

gcc -Wall -Wextra -O2 -std=c99 -s tagha_assembler.c -L. -lDataStructColl -o tasm
