# C Virtual Machine
minimal but complex stack-based virtual machine, written in C.
## Purpose:
to be an embeddable C-as-scripting-language scripting engine for C or C++ programs. The plan is to create or use a C compiler frontend to generate bytecode and run the bytecode on this vm.
## Why?:
so far, the only C interpreters (that I currently know of) are **CINT**, **PicoC**, **TCC\*** and **Ch**:
- The problems with CINT is that it's old, clunky to use, outdated, and deprecated.
- PicoC is good but its problem is that it uses old-school interpreting (just runs literal code) instead of compiling to bytecode and running much faster.
- The problem with Ch is that, though it's embeddable and updated, it's proprietary and it's unknown how it interprets code; as the usual problem with proprietary code, you don't know what code it could contain and there's no telling what security issues that code could have.
- \* TCC - Tiny C Compiler, it can compile and run scripts for testing but it cannot be embedded. The scripting action is more or less a **great** way to test your program! libtcc only acts as a JIT IIRC.

The goal for this VM is to...
+ 1. be a new and modernly optimized piece of software for C-as-a-scripting-language.
+ 2. have the clear execution speed advantage bytecode interpreters have over traditional interpreters.
+ 3. be open-source and open for anyone who wants to improve this 1st portion of what would become a scripting engine.

### Features
* float (float32, not 64-bit float) support by converting integer to a float by its bits. For example, "5.0f" is '0x40a00000' or '1084227584' in terms of its bits if it were an int.
I usually use hexadecimals but decimal numbers work just as good. The giant numbers are then transformed into floats via type-punning by a single union singleton.
* uses computed gotos (the ones that use a void\*) which is 20%-25% faster than a switch [citation needed for this one].
* "CPU" is "32-bit" as the word size is uint32_t. Numerical memory addresses are 4 bytes.
* memory manipulation where loading copies the top of the stack into any memory address and storing pops off the top of the stack into any memory address.
* has integer and float arithmetic, (un)conditional jumps, comparisons, and stack and memory manipulations. By default, the arithmetic is always 4 bytes because in C, integers are always promoted to the largest width.
* call stack for functions. Supports function calls from function calls! (unless you overflow the call stack...)
* It's Turing Complete! (lol)

### V2 Instruction Set.
 - nop - does nothing.
 
 - pushl - push 4-byte data to top of stack (TOS).
 - pushs - push 2-byte data to TOS.
 - pushb - push a byte to TOS.
 
 - popl - pops 4 bytes from TOS.
 - pops - pops 2 bytes from TOS.
 - popb - pops byte from TOS.
 
 - wrtl - writes 4 bytes to a memory address.
 - wrts - writes 2 bytes to a memory address.
 - wrtb - writes a byte to a memory address.
 
 - storel - pops 4 bytes from TOS and stores to a memory address.
 - stores - pops 2 bytes from TOS and stores to a memory address.
 - storeb - pop a byte from TOS and stores to a memory address.
 
 - loadl - puts 4 bytes from memory into TOS.
 - loads - puts 2 bytes from memory into TOS.
 - loadb - puts a byte from memory into TOS.
 
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
 - incl - increment 1.
 - decl - decrement 1.
 
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
 - jzs - checks if first 2 bytes of TOS is zero and jumps if zero.
 - jzb - checks if first byte on TOS is zero and jumps if zero.
 - jnzl - checks if first 4 bytes of TOS is not zero then jumps to desired address if not zero.
 - jnzs - checks if first 2 bytes of TOS is not zero then jumps to desired address if not zero.
 - jnzb - checks if first byte of TOS is not zero then jumps to desired address if not zero.
 
 - call - jumps to an address of code and returns to original address if a `ret` opcode is executed.
 - ret - returns to an original instruction address after using `call` opcode for subroutines/procedures.
-  halt - stops all execution.

## TODO list
- [x] add call + ret instructions to support procedures.
- [ ] test call + ret for recursive functions.
- [ ] implementing a call stack and memory addressing means we would need a form of buffer overflow protection.
- [x] expand opcodes to take various sizes of data and sources. What I mean is make a push and pop for a byte, word (2 bytes), dword (4 bytes), and qword (8 bytes).
- [ ] add more opcodes for the arithmetic and comparisons so we can do packed vector mathematics easier.
