CC = gcc
#CC = clang-9
CFLAGS = -Wextra -Wall -std=c99 -s -O2
TFLAGS = -Wextra -Wall -std=c99 -g -O2
PROFFLAGS = -Wextra -Wall -std=c99 -pg -O2
# -static

taghatest:
	$(CC) $(CFLAGS) test_hostapp.c -L. -ltagha -o taghatest

debug:
	$(CC) $(TFLAGS) test_hostapp.c -L. -ltagha -o taghatest

clean:
	$(RM) *.o
