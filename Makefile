CC = gcc
CFLAGS = -Wextra -Wall -std=c99 -s -O2
TESTFLAGS = -Wextra -Wall -std=c99 -g -O2

SRCS = tagha_api.c

OBJS = $(SRCS:.c=.o)
LIBNAME = libtagha

tagha:
	$(CC) $(CFLAGS) -c libharbol/stringobj.c libharbol/vector.c libharbol/hashmap.c libharbol/mempool.c libharbol/linkmap.c $(SRCS)
	ar cr $(LIBNAME).a stringobj.o vector.o hashmap.o mempool.o linkmap.o $(OBJS)
	$(CC) -shared stringobj.o vector.o hashmap.o mempool.o linkmap.o $(OBJS) -o $(LIBNAME).so

tagha_asm:
	$(CC) $(CFLAGS) libharbol/stringobj.c libharbol/vector.c libharbol/hashmap.c libharbol/bytebuffer.c libharbol/linkmap.c tagha_assembler/tagha_assembler.c -o tagha_asm

tagha_libc:
	$(CC) $(CFLAGS) -c libharbol/stringobj.c libharbol/vector.c libharbol/hashmap.c libharbol/mempool.c libharbol/linkmap.c tagha_libc/tagha_ctype.c tagha_libc/tagha_stdio.c tagha_libc/tagha_stdlib.c tagha_libc/tagha_string.c tagha_libc/tagha_time.c -L. -ltagha
	
	ar	cr libtagha_libc.a stringobj.o vector.o hashmap.o mempool.o linkmap.o tagha_ctype.o tagha_stdio.o tagha_stdlib.o tagha_string.o  tagha_time.o
	
	$(CC) -shared stringobj.o vector.o hashmap.o mempool.o linkmap.o tagha_ctype.o tagha_stdio.o tagha_stdlib.o tagha_string.o  tagha_time.o -o libtagha_libc.so

testapp:
	$(CC) $(CFLAGS) test_hostapp.c -L. -ltagha -ltagha_libc -o tagha_testapp

clean:
	$(RM) *.o
