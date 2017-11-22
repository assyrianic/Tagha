#!/bin/bash
cd "$(dirname "$0")"
gcc		-std=c99 -g -Os main.c -o taghavm_test
