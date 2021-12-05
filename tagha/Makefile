CC = gcc
#CC = clang-9
CFLAGS = -Wextra -Wall -Wrestrict -std=c99 -static -s  -O2 -mtune=native -march=native
TFLAGS = -Wextra -Wall -Wrestrict -std=c99 -g  -O2 -mtune=native -march=native
PFlAGS = -Wextra -Wall -Wrestrict -std=c99 -pg -O2 -mtune=native -march=native

# -static

SRCS = allocators/region/region.c allocators/mempool/mempool.c tagha.c
OBJS = region.o mempool.o tagha.o

LIBNAME = libtagha

tagha:
	$(CC) $(CFLAGS) -c $(SRCS)
	$(AR) cr $(LIBNAME).a $(OBJS)

shared:
	$(CC) $(CFLAGS) -shared $(SRCS) -o $(LIBNAME).so

debug:
	$(CC) $(TFLAGS) -c $(SRCS)
	$(AR) cr $(LIBNAME).a $(OBJS)

debug_shared:
	$(CC) $(TFLAGS) -shared $(SRCS) -o $(LIBNAME).so

profile:
	$(CC) $(PFlAGS) -c $(SRCS)
	$(AR) cr $(LIBNAME).a $(OBJS)

disasm:
	$(CC) -O2 -masm=intel -S $(SRCS)
#	llvm-gcc -emit-llvm -S $(SRCS)
# -fverbose-asm

clean:
	$(RM) *.o
