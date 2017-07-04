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
* ~~float64 (double, not 32bit float) support by converting integer to a float by its bits. For example, "5.0" double is '0x4014000000000000' or '4617315517961601024' in terms of its bits if it were a long.
I usually use hexadecimals but decimal numbers work just as good. The giant numbers are then transformed into doubles by typecasting into double\* and dereferencing.~~
* uses computed gotos (the ones that use a void\*) which is 20%-25% faster than a switch [citation needed for this one].
* "CPU" is "32-bit" as the word size is uint32_t. Numerical memory addresses are 4 bytes.
* memory manipulation where loading copies the top of the stack into any memory address and storing pops off the top of the stack into any memory address.
* has integer and float arithmetic, (un)conditional jumps, comparisons, and stack and memory manipulations.
* call stack for functions. Supports function calls from function calls! (unless you overflow the call stack...)
* ~~It's Turing Complete! (lol)~~

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
-  halt - stops all execution.

## TODO list
- [ ] add call + ret instructions to support procedures.
- [ ] test call + ret for recursive functions.
- [ ] implementing a call stack and memory addressing means we would need a form of buffer overflow protection.
- [x] expand opcodes to take various sizes of data and sources. What I mean is make a push and pop for a byte, word (2 bytes), dword (4 bytes), and qword (8 bytes).
