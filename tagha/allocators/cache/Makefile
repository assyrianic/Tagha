CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -s -O2
TFLAGS = -Wall -Wextra -pedantic -std=c99 -g -O2

SRCS = cache.c
OBJS = $(SRCS:.c=.o)

harbol_cache:
	$(CC) $(CFLAGS) -c $(SRCS)

debug:
	$(CC) $(TFLAGS) -c $(SRCS)

clean:
	$(RM) *.o
