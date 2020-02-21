CC = gcc
CFLAGS = -Wextra -Wall -std=c99 -s -O2
TESTFLAGS = -Wextra -Wall -std=c99 -g -O2
PROFFLAGS = -Wextra -Wall -std=c99 -pg -O2


taghatest:
	$(CC) $(CFLAGS) test_hostapp.c -L. -ltagha -o taghatest

debug:
	$(CC) $(TESTFLAGS) test_hostapp.c -L. -ltagha -o taghatest

clean:
	$(RM) *.o
