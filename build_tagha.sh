#!/bin/bash
cd "$(dirname "$0")"
gcc -Wall -Wextra -Os -g -std=c99 tagha_api.c hostapp.c -L. -lDataStructColl -o taghavm
