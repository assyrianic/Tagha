CC = gcc

CFLAGS = -Wall -Wextra -std=c99 -s -O2
CPPFLAGS = -Wall -Wextra -std=c++11 -s -O2

TESTFLAGS = -Wall -Wextra -std=c99 -g -O2

DEPS = tagha.h
#LIBS = -ldl -lm

SRCS = tagha_api.c
SRCScpp = tagha_api_cpp.cpp

OBJS = $(SRCS:.c=.o)
OBJScpp = $(SRCS:.cpp=.o)


tagha_static:
	$(CC) $(CFLAGS) -c $(SRCS)
	ar cr libtagha.a $(OBJS)

tagha_shared:
	$(CC) $(CFLAGS) -fPIC -c $(SRCS)
	$(CC) -shared -o libtagha.so $(OBJS)

test:
	$(CC) $(TESTFLAGS) $(SRCS) tagha_testcode/test_hostapp.c -o tagha_testappc

tagha_cpp_static:
	$(CC) $(CPPFLAGS) -c $(SRCScpp) -L. -ltagha
	ar cr libtaghacpp.a $(OBJScpp)

clean:
	$(RM) *.o
