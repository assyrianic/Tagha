#!/bin/bash
cd "$(dirname "$0")"
valgrind --leak-check=full --show-leak-kinds=all ./Tagha_Clang endian_test1.tbc test_native.tbc test_multiple_natives.tbc test_local_native_funcptr.tbc
