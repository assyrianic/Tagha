#!/bin/bash
cd "$(dirname "$0")"
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes -v --main-stacksize=16000000 ./taghavm_hosttest test_global.tbc
