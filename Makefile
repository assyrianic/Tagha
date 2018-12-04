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

clean:
	$(RM) *.o
