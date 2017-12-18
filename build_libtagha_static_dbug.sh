#!/bin/bash
cd "$(dirname "$0")"
clang-3.5	-std=c99 -g -Os -c 	ds.c tagha_api.c tagha_exec.c tagha_libc.c
ar			cr libtaghaclang.a	ds.o tagha_api.o tagha_exec.o tagha_libc.o

gcc			-std=c99 -g -Os -c		ds.c tagha_api.c tagha_exec.c tagha_libc.c
ar			cr libtaghagcc.a	ds.o tagha_api.o tagha_exec.o tagha_libc.o

rm	ds.o tagha_api.o tagha_exec.o tagha_libc.o
