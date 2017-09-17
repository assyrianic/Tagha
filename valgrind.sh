#!/bin/bash
cd "$(dirname "$0")"
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes -v ./TaghaClang test_malloc.tbc test_fopen.tbc test_native.tbc test_multiple_natives.tbc test_local_native_funcptr.tbc
