#!/bin/bash
cd "$(dirname "$0")"
clang-3.5	-std=c11 -Os -c 	ds.c taghaapi.c tagha.c
ar			cr libtaghaclang.a	ds.o taghaapi.o tagha.o

gcc			-std=c11 -Os -c		ds.c taghaapi.c tagha.c
ar			cr libtaghagcc.a	ds.o taghaapi.o tagha.o

rm	ds.o taghaapi.o tagha.o
