#!/bin/bash
cd "$(dirname "$0")"
gcc			-Wextra -Wall -std=c99 -Os -c	tagha_api.c -L. -lDataStructColl
ar			cr libtagha.a	tagha_api.o

rm	tagha_api.o


gcc			-Wextra -Wall -std=c++11 -Os -c	tagha_api_cpp.cpp -L. -ltagha
ar			cr libtaghacpp.a	tagha_api_cpp.o

rm	tagha_api_cpp.o
