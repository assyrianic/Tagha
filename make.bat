@echo off

set CC=clang-cl
set CFLAGS=-Xclang -std=c99 -Xclang -O2 -D_CRT_SECURE_NO_WARNINGS

set SRCS=tagha_api.c

set OBJS=%SRCS:.c=.obj%
set LIBNAME=libtagha

set ACTION=%1

if [%1] == [] set ACTION=tagha
goto %ACTION%

:tagha
       %CC% %CFLAGS% -c libharbol/stringobj.c libharbol/vector.c libharbol/hashmap.c libharbol/mempool.c libharbol/linkmap.c %SRCS%

       llvm-ar cr %LIBNAME%.lib stringobj.obj vector.obj hashmap.obj mempool.obj linkmap.obj %OBJS%

       %CC% %CFLAGS% /LD stringobj.obj vector.obj hashmap.obj mempool.obj linkmap.obj %OBJS% -o%LIBNAME%

       goto END

:tagha_asm
       %CC% %CFLAGS% libharbol/stringobj.c libharbol/vector.c libharbol/hashmap.c libharbol/bytebuffer.c libharbol/linkmap.c tagha_assembler/tagha_assembler.c -otagha_asm

       goto END

:tagha_libc
	%CC% %CFLAGS% -c ^
       libharbol/stringobj.c libharbol/vector.c libharbol/hashmap.c libharbol/mempool.c libharbol/linkmap.c ^
       tagha_libc/tagha_ctype.c tagha_libc/tagha_stdio.c tagha_libc/tagha_stdlib.c tagha_libc/tagha_string.c tagha_libc/tagha_time.c tagha_libc/tagha_module.c
	
	llvm-ar cr %LIBNAME%_libc.lib stringobj.obj vector.obj hashmap.obj mempool.obj linkmap.obj tagha_ctype.obj tagha_stdio.obj tagha_stdlib.obj tagha_string.obj tagha_time.obj tagha_module.obj
	
	%CC% %CFLAGS% /LD stringobj.obj vector.obj hashmap.obj mempool.obj linkmap.obj tagha_ctype.obj tagha_stdio.obj tagha_stdlib.obj tagha_string.obj tagha_time.obj tagha_module.obj %LIBNAME%.lib -o%LIBNAME%_libc

       goto END

:testapp
	%CC% %CFLAGS% test_hostapp.c %LIBNAME%.lib %LIBNAME%_libc.lib -otagha_testapp

       goto END

:clean
       del *.obj

       goto END

:END
       echo %ACTION% is done