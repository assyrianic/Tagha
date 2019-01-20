@echo off

set CC=clang
set CFLAGS=-Wextra -Wall -std=c99 -s -O2
set TESTFLAGS=-Wextra -Wall -std=c99 -g -O2

set SRCS=tagha_api.c

set OBJS=%SRCS:.c=.o%
set LIBNAME=libtagha

set ACTION=%1

if [%1] == [] set ACTION=tagha
goto %ACTION%

:tagha
       %CC% %CFLAGS% -c libharbol/stringobj.c libharbol/vector.c libharbol/hashmap.c libharbol/mempool.c libharbol/linkmap.c %SRCS%
       llvm-ar cr %LIBNAME%.lib stringobj.o vector.o hashmap.o mempool.o linkmap.o %OBJS%
       %CC% -shared stringobj.o vector.o hashmap.o mempool.o linkmap.o %OBJS% -o %LIBNAME%.dll
       goto END

:tagha_asm
       %CC% %CFLAGS% libharbol/stringobj.c libharbol/vector.c libharbol/hashmap.c libharbol/bytebuffer.c libharbol/linkmap.c tagha_assembler/tagha_assembler.c -o tagha_asm.exe
       goto END

:clean
       del *.o
       goto END

:END
echo %ACTION% is done