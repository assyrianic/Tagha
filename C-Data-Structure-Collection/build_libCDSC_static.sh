#!/bin/bash
cd "$(dirname "$0")"
gcc	-Wall -Wextra -std=c99 -g -O2 -c 	stringobj.c vector.c hashmap.c unilist.c bilist.c bytebuffer.c tuple.c heap.c graph.c tree.c pluginarch.c linkmap.c

ar	cr libDataStructColl.a		stringobj.o vector.o hashmap.o unilist.o bilist.o bytebuffer.o tuple.o heap.o graph.o tree.o pluginarch.o linkmap.o

rm	stringobj.o vector.o hashmap.o unilist.o bilist.o bytebuffer.o tuple.o heap.o graph.o tree.o pluginarch.o linkmap.o
