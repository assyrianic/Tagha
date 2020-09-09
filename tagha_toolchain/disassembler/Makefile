CC = gcc
CFLAGS = -Wextra -Wall -Wrestrict -std=c99 -s -O2 -flto
TFLAGS = -Wextra -Wall -Wrestrict -std=c99 -g -O2 -flto
# -static
SRCS =  ../libharbol/stringobj/stringobj.c
SRCS += ../libharbol/bytebuffer/bytebuffer.c
SRCS += ../libharbol/vector/vector.c
SRCS += ../libharbol/map/map.c
SRCS += ../libharbol/linkmap/linkmap.c
SRCS += disassembler.c

tagha_disassembler:
	$(CC) $(CFLAGS) $(SRCS) -o tagha_disassembler

debug:
	$(CC) $(TFLAGS) $(SRCS) -o tagha_disassembler

clean:
	$(RM) *.o
