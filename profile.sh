#!/bin/bash
cd "$(dirname "$0")"

# -fsanitize=address -fsanitize=undefined -lubsan -fstrict-aliasing
# -funroll-loops -finline-functions -ffast-math -fexpensive-optimizations


gcc		-Wextra -Wall -s -std=c99 -O2 -c libharbol/stringobj.c libharbol/vector.c libharbol/hashmap.c libharbol/mempool.c libharbol/linkmap.c tagha_api.c  #-fno-guess-branch-probability -fomit-frame-pointer

#gcc		-Wextra -Wall -std=c99 -s -O2 -S -masm=intel tagha_api.c

ar			cr libtagha.a	stringobj.o vector.o hashmap.o mempool.o linkmap.o tagha_api.o

gcc -Wextra -Wall -s -std=c99 -O2 test_hostapp.c -L. -ltagha -ltagha_libc -o taghavmgcc_hosttest



clang-6.0		-Wextra -Wall -s -std=c99 -O3 -c libharbol/stringobj.c libharbol/vector.c libharbol/hashmap.c libharbol/mempool.c libharbol/linkmap.c tagha_api.c #-fno-guess-branch-probability -fomit-frame-pointer -funroll-loops -finline-functions -ffast-math -fexpensive-optimizations

#clang-6.0	-Wextra -Wall -s -std=c99 -O1 -S -masm=intel tagha_api.c

ar			cr libtagha.a	stringobj.o vector.o hashmap.o mempool.o linkmap.o tagha_api.o

clang-6.0 -Wextra -Wall -s -std=c99 -O3 test_hostapp.c -L. -ltagha -ltagha_libc -o taghavmclang_hosttest

#gcc		-Wextra -Wall -std=c99 -s -O2 -funroll-loops -finline-functions -ffast-math -fexpensive-optimizations -S -masm=intel tagha_api.c


rm	*.o

./taghavmgcc_hosttest "test_fib.tbc"
gprof taghavmgcc_hosttest gmon.out > Tagha_profile_GCC.txt
rm gmon.out

./taghavmclang_hosttest "test_fib.tbc"
gprof taghavmclang_hosttest gmon.out > Tagha_profile_Clang.txt
rm gmon.out

