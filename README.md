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
* uses computed gotos (the ones that use a void\*) which is 20%-25% faster than a switch [[citation]](http://eli.thegreenplace.net/2012/07/12/computed-goto-for-efficient-dispatch-tables).
* "CPU" is "32-bit" as the word size is uint32. Numerical memory addresses are 32-bits as well.
* memory manipulation where loading copies the top of the stack into any memory address and storing pops off the top of the stack into any memory address.
* has integer and float arithmetic, (un)conditional jumps, comparisons, and stack and memory manipulations. By default, the arithmetic is always 4 bytes because in C, integers are always promoted to the largest width.
* ~~call stack for functions. Supports function calls from function calls! (unless you overflow the call stack...)~~ Call stack has been merged into the overall Stack. When you do a function call, a new stack frame is created. You can use the loadsp/storesp opcodes to load/store function argument/parameter data. Returning destroys the stack frame as usual.
* It's Turing Complete! (lol)
* VM is little-endian.
* 

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
