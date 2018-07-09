#!/bin/bash
cd "$(dirname "$0")"
gcc			-Wextra -Wall -std=c99 -g -Os -c	tagha_api.c
ar			cr libtagha.a	tagha_api.o

#gcc			-Wextra -Wall -std=c99 -Os -S -masm=intel tagha_api.c

rm	tagha_api.o


gcc			-Wextra -Wall -std=c++11 -O3 -c	tagha_api_cpp.cpp -L. -ltagha
ar			cr libtaghacpp.a	tagha_api_cpp.o

rm	tagha_api_cpp.o
