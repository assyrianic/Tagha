#!/bin/bash
cd "$(dirname "$0")"
clang-3.5	-std=c99 -O2 -c 	ds.c taghaapi.c tagha.c
ar			cr libtaghaclang.a	ds.o taghaapi.o tagha.o

gcc			-std=c99 -O2 -c		ds.c taghaapi.c tagha.c
ar			cr libtaghagcc.a	ds.o taghaapi.o tagha.o

rm	ds.o taghaapi.o tagha.o
