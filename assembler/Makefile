CC = gcc
CFLAGS = -Wextra -Wall -std=c99 -s -O2 -flto
TESTFLAGS = -Wextra -Wall -std=c99 -g -O2 -flto

SRCS = libharbol/stringobj/stringobj.c
SRCS += libharbol/bytebuffer/bytebuffer.c
SRCS += libharbol/vector/vector.c
SRCS += libharbol/map/map.c
SRCS += libharbol/linkmap/linkmap.c
SRCS += tagha_assembler.c

tagha_assembler:
	$(CC) $(CFLAGS) $(SRCS) -o tagha_asm

debug:
	$(CC) $(TESTFLAGS) $(SRCS) -o tagha_asm

clean:
	$(RM) *.o
