#!/bin/bash
cd "$(dirname "$0")"
gcc			-Wextra -Wall -std=c99 -Os -c	tagha_api.c -L. -lDataStructColl
ar			cr libtaghagcc.a	tagha_api.o

rm	tagha_api.o
