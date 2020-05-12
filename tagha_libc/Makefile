CC = gcc
CFLAGS = -Wextra -Wall -std=c99 -s -O2
TESTFLAGS = -Wextra -Wall -std=c99 -g -O2

SRCS = tagha_ctype.c
SRCS += tagha_stdio.c
SRCS += tagha_stdlib.c
SRCS += tagha_string.c
SRCS += tagha_time.c
SRCS += tagha_module.c
SRCS += tagha_simd.c
OBJS = $(SRCS:.c=.o)

LIBNAME = libtaghaclib

tagha_libc:
	$(CC) $(CFLAGS) -c $(SRCS)
	$(AR) cr $(LIBNAME).a $(OBJS)

shared:
	$(CC) $(CFLAGS) -shared $(SRCS) -o $(LIBNAME).so

debug:
	$(CC) $(TESTFLAGS) -c $(SRCS)
	$(AR) cr $(LIBNAME).a $(OBJS)

debug_shared:
	$(CC) $(TESTFLAGS) -shared $(SRCS) -o $(LIBNAME).so

clean:
	$(RM) *.o
