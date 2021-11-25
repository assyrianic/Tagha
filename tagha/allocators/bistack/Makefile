CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -s -O2
TFLAGS = -Wall -Wextra -pedantic -std=c99 -g -O2

SRCS = bistack.c
OBJS = $(SRCS:.c=.o)

harbol_bistack:
	$(CC) $(CFLAGS) -c $(SRCS)

debug:
	$(CC) $(TFLAGS) -c $(SRCS)

test:
	$(CC) $(TFLAGS) $(SRCS) test_bistack.c -o harbol_bistack_test

clean:
	$(RM) *.o
	$(RM) harbol_bistack_test
