CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -s -O2
TESTFLAGS = -Wall -Wextra -std=c99 -g -O2
DEPS = harbol.h
#LIBS = -ldl -lm
SRCS = stringobj.c vector.c hashmap.c unilist.c bilist.c bytebuffer.c tuple.c mempool.c graph.c tree.c linkmap.c variant.c cfg.c
OBJS = $(SRCS:.c=.o)

harbol:
	$(CC) $(CFLAGS) -c $(SRCS)
	ar cr libharbol.a $(OBJS)

harbol_shared:
	$(CC) $(CFLAGS) -fPIC -c $(SRCS)
	$(CC) -shared -o libharbol.so $(OBJS)

test:
	$(CC) $(TESTFLAGS) $(SRCS) test_suite.c -o harbol_testprogram

clean:
	$(RM) *.o
