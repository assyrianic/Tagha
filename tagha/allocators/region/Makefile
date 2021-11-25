CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -s -O2
TFLAGS = -Wall -Wextra -pedantic -std=c99 -g -O2

SRCS = region.c
OBJS = $(SRCS:.c=.o)

harbol_region:
	$(CC) $(CFLAGS) -c $(SRCS)

debug:
	$(CC) $(TFLAGS) -c $(SRCS)

test:
	$(CC) $(TFLAGS) $(SRCS) test_region.c -o harbol_region_test

clean:
	$(RM) *.o
	$(RM) harbol_region_test
