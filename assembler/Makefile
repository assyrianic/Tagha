CC = gcc
CFLAGS = -Wextra -Wall -std=c99 -static -s -O2 -flto
TFLAGS = -Wextra -Wall -std=c99 -g -O2 -flto

SRCS = libharbol/stringobj/stringobj.c
SRCS += libharbol/bytebuffer/bytebuffer.c
SRCS += libharbol/vector/vector.c
SRCS += libharbol/map/map.c
SRCS += libharbol/linkmap/linkmap.c
SRCS += tagha_assembler.c

tagha_assembler:
	$(CC) $(CFLAGS) $(SRCS) -o tagha_asm
	$(CC) $(CFLAGS) libharbol/stringobj/stringobj.c libharbol/vector/vector.c tagha_disassembler.c -o tagha_disasm

debug:
	$(CC) $(TFLAGS) $(SRCS) -o tagha_asm
	$(CC) $(TFLAGS) libharbol/stringobj/stringobj.c libharbol/vector/vector.c tagha_disassembler.c -o tagha_disasm

clean:
	$(RM) *.o
