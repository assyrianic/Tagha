# Tagha Virtual Machine
"tagha" is Aramaic for "crown".
Tagha is a minimal yet complex stack-based virtual machine, written in C, designed to run C. Why is it named "crown"? Because C is king in the programming world :P

## Purpose:
to be an embeddable C-as-scripting-language scripting engine for C or C++ programs. The plan is to ~~create or~~ use a C compiler's backend to generate and run bytecode on this vm.

## Why?:
so far, the only C interpreters (that I currently know of) are **CINT**, **PicoC**, **TCC [1]** and **Ch**:
- The problems with CINT is that it's old, clunky to use, outdated, and deprecated.
- PicoC is good but its problem is that it uses old-school interpreting (just runs literal code) instead of compiling to bytecode and running much faster.
- The problem with Ch is that, though it's embeddable and updated, it's proprietary and it's unknown how it interprets code; as the usual problem with proprietary code, you don't know what code it could contain and there's no telling what security issues that code could have.
- **[1]** - Tiny C Compiler, it can compile and run scripts for testing but it cannot be embedded. The scripting action is more or less a **great** way to test your program! libtcc only acts as a JIT IIRC.

The goal for TaghaVM is to...
+ 1. be a new and modernly optimized piece of software for C-as-a-scripting-language.
+ 2. have the clear execution speed advantage bytecode interpreters have over traditional interpreters.
+ 3. be open-source and open for anyone who wants to improve this 1st portion of what would become a scripting engine.

### Features
* float (float32, not 64-bit float) support by converting integer to a float by its bits. For example, "5.0f" is '0x40a00000' or '1084227584' in terms of its bits if it were an int.
I usually use hexadecimals but decimal numbers work just as good. The giant numbers are then transformed into floats via type-punning by a single union singleton.
* uses computed gotos (the ones that use a void\*) which is 20%-25% faster than a switch [citation needed for this one].
* "CPU" is "32-bit" as the word size is uint32. Numerical memory addresses are 32-bits as well.
* memory manipulation where loading copies the top of the stack into any memory address and storing pops off the top of the stack into any memory address.
* has integer and float arithmetic, (un)conditional jumps, comparisons, and stack and memory manipulations. By default, the arithmetic is always 4 bytes because in C, integers are always promoted to the largest width.
* ~~call stack for functions. Supports function calls from function calls! (unless you overflow the call stack...)~~ Call stack has been merged into the overall Stack. When you make a call, a new stack frame is created. You can use the loadsp/storesp opcodes to load/store function argument/parameter data. Returning, of course, destroys the stack frame.
* It's Turing Complete! (lol)
* VM is little-endian.

### V2 Instruction Set.
 - nop - does nothing.
 
 - pushl - push 4-byte data to top of stack (TOS).
 - pushs - push 2-byte data to TOS.
 - pushb - push a byte to TOS.
 - pushsp - push current stack pointer index to TOS.
 - puship - push current instruction pointer index to TOS.
 - pushbp - pushes the frame pointer to TOS.
 
 - popl - pops 4 bytes from TOS.
 - pops - pops 2 bytes from TOS.
 - popb - pops byte from TOS.
 - popsp - pops 4 bytes from TOS and sets that as the current stack pointer.
 - popip - pops 4 bytes from TOS and sets that as the current instruction pointer.
 - popbp - pops 4 bytes from TOS as the new frame pointer.
 
 - wrtl - writes 4 bytes to a memory address.
 - wrts - writes 2 bytes to a memory address.
 - wrtb - writes a byte to a memory address.
 
 - storel - pops 4 bytes from TOS and stores to a memory address.
 - stores - pops 2 bytes from TOS and stores to a memory address.
 - storeb - pop a byte from TOS and stores to a memory address.
 
 - storela - pops 4 bytes from TOS, uses that as memory address, then pops another 4 bytes and stores into the address.
 - storesa - pops 4 bytes from tos as memory address, pops another 2 bytes into that address.
 - storeba - pops 4 bytes from tos as mem address, pops a byte into that address.
 
 - storespl - pops a stack address, then pops 4 bytes from TOS into said address in the stack.
 - storesps - pops stack addr, then pops 2 bytes from TOS into said address in the stack.
 - storespb - pop stack addr, then pops a byte from TOS into said address in the stack.
 
 - loadl - puts 4 bytes from memory into TOS.
 - loads - puts 2 bytes from memory into TOS.
 - loadb - puts a byte from memory into TOS.
 
 - loadla - pops 4 bytes as a memory address, pushes the 4 bytes of data from the memory address to TOS.
 - loadsa - pops 4 bytes as memory address, pushes 2 bytes of data from address to TOS.
 - loadba - pops 4 bytes as memory address, pushes byte from address to TOS.
 
 - loadspl - pushes 4 bytes of data from data stack address to TOS.
 - loadsps - pushes 2 bytes of data from data stack addr to TOS.
 - loadspb - pushes byte of data from data stk addr to TOS.
 
 - copyl - copies the first 4 bytes of the TOS.
 - copys - copies the first 2 bytes of the TOS.
 - copyb - copies the first byte on the TOS.
 
 - addl - signed 4-byte integer addition.
 - uaddl - unsigned 4-byte int addition.
 - addf - float addition.
 
 - subl - signed 4-byte int subtraction
 - usubl - unsigned int subtract.
 - subf - float subtract.
 
 - mull - signed int multiplication.
 - umull - unsigned int mult.
 - mulf - float mult.
 
 - divl - signed int division. divide by zero causes immediate halt of vm.
 - udivl - unsigned int division. divide by zero causes immediate halt of vm.
 - divf - float division. divide by zero causes immediate halt of vm.
 
 - modl - signed int modulo math.
 - umodl - unsigned int modulo.
 
 - andl - bitwise AND operation.
 - orl - bitwise OR op.
 - xorl - bitwise XOR.
 - notl - bitwise NOT.
 - shl - bit shift left.
 - shr - bit shift right.
 
 - incl - increment 4-bytes of data by 1.
 - incf - increments 4-byte float by 1.0.
 - decl - decrement 4-bytes of data by 1.
 - decf - decrement 4-byte float by 1.0.
 - negl - negates 4-byte integer.
 - negf - negates 4-byte float.
 
 - ltl - signed int less than comparison.
 - ultl - unsigned int less than.
 - ltf - float less than compare.
 
 - gtl - signed int greater than.
 - ugtl - unsigned int greater than.
 - gtf - float greater than.
 
 - cmpl - signed int equality comparison.
 - ucmpl - unsigned int equality compare.
 - cmpf - float equality comparison.
 
 - leql - signed int less than or equal comparison.
 - uleql - unsigned int lt or equal compare.
 - leqf - float less than or equal.
 
 - geql - signed greater than or equal comparison.
 - ugeql - unsigned greater than or equal compare.
 - geqf - float greater than or equal.
 
 - jmp - jumps to a different instruction address.
 - jzl - jump if zero, checks if the first 4 bytes of TOS is zero and jumps to the desired address.
 - jnzl - checks if first 4 bytes of TOS is not zero then jumps to desired address if not zero.
 
 - call - jumps to an address of code and returns to original address if a `ret` opcode is executed.
 - calls - pops 4 bytes off TOS and jumps that code address. works similar to `call` but uses stack.
 - calla - pops 4 bytes off TOS as a memory address, retrives 4-byte datum from address, and jumps to the datum as a code address. works similar to `calls` but uses a function address stored in memory.
 
 - ret - returns to an original instruction address after using `call` opcode for subroutines/procedures.
 - reset - halts execution of program and refreshes VM data to 0.
 - halt - stops all execution.

## TODO list
- [x] ~~add call + ret instructions to support procedures.~~
- [x] ~~test call + ret for recursive functions.~~
- [x] ~~implementing a call stack and memory addressing means we would need a form of buffer overflow protection.~~
- [x] ~~expand opcodes to take various sizes of data and sources. What I mean is make a push and pop for a byte, word (2 bytes), dword (4 bytes), and qword (8 bytes).~~
- [ ] add `q` or 'quad' opcodes for 64-bit control.
- [x] ~~add a form of referencing and dereferencing for memory and stack.~~
- [ ] add API for host applications to embed this VM.
- [x] replace stack and ~~callstack~~ with an actual pointer.
- [ ] create format for libraries and headers.
- [ ] orient stack to be little endian (push the least significant byte first, leaving the 1st byte as top of stack byte data)

## End Goals list
- [ ] complete, seamless embeddability to C (and by extension C++) programs. As smooth as how Angelscript binds to C++.
- [ ] compatibility of as much of the C standards possible, including C11.
- [ ] implement a compiler that generates the bytecode for this VM.
- [ ] 64-bit version availability.
- [ ] Windows compatibility/availability.
- [ ] Ultimate Goal: VM can bootstrap itself! (compiled to TaghaVM bytecode and ran by TaghaVM itself).
