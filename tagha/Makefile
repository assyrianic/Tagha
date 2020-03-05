CC = gcc
#CC = clang-9
CFLAGS = -Wextra -Wall -std=c99 -s -O2
TESTFLAGS = -Wextra -Wall -std=c99 -g -O2
PROFFLAGS = -Wextra -Wall -std=c99 -pg -O2

SRCS = allocators/mempool.c tagha.c
OBJS = mempool.o tagha.o

LIBNAME = libtagha

tagha:
	$(CC) $(CFLAGS) -c $(SRCS)
	$(AR) cr $(LIBNAME).a $(OBJS)

shared:
	$(CC) $(CFLAGS) -shared $(SRCS) -o $(LIBNAME).so

debug:
	$(CC) $(TESTFLAGS) -c $(SRCS)
	$(AR) cr $(LIBNAME).a $(OBJS)

debug_shared:
	$(CC) $(TESTFLAGS) -shared $(SRCS) -o $(LIBNAME).so

profile:
	$(CC) $(PROFFLAGS) -c $(SRCS)
	$(AR) cr $(LIBNAME).a $(OBJS)

disasm:
	$(CC) -O2 -fverbose-asm -masm=intel -S $(SRCS)
	#llvm-gcc -emit-llvm -S $(SRCS)

clean:
	$(RM) *.o
