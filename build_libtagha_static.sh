#!/bin/bash
cd "$(dirname "$0")"
clang-3.5	-std=c11 -Os -c 	vector.c hashmap.c taghaapi.c tagha.c
ar			cr libtaghaclang.a	vector.o hashmap.o taghaapi.o tagha.o

gcc			-std=c11 -Os -c		vector.c hashmap.c taghaapi.c tagha.c
ar			cr libtaghagcc.a	vector.o hashmap.o taghaapi.o tagha.o

rm	vector.o hashmap.o taghaapi.o tagha.o
