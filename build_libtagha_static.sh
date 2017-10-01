#!/bin/bash
cd "$(dirname "$0")"
clang-3.5	-std=c99 -O3 -c 	ds.c taghaapi.c tagha.c tagha_libc.c
ar			cr libtaghaclang.a	ds.o taghaapi.o tagha.o tagha_libc.o

gcc			-std=c99 -O3 -c		ds.c taghaapi.c tagha.c tagha_libc.c
ar			cr libtaghagcc.a	ds.o taghaapi.o tagha.o tagha_libc.o

rm	ds.o taghaapi.o tagha.o tagha_libc.o
