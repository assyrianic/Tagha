CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -s -O2
TFLAGS = -Wall -Wextra -pedantic -std=c99 -g -O2

SRCS = objpool.c
OBJS = $(SRCS:.c=.o)

harbol_objpool:
	$(CC) $(CFLAGS) -c $(SRCS)

debug:
	$(CC) $(TFLAGS) -c $(SRCS)

test:
	$(CC) $(TFLAGS) $(SRCS) test_objpool.c -o harbol_objpool_test

clean:
	$(RM) *.o
	$(RM) harbol_objpool_test
