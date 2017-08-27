# Tagha Virtual Machine
"tagha" is Aramaic for "crown".
Tagha is a **WIP** minimal yet complex stack-based virtual machine and scripting engine, written in C, designed to run compiled C scripts. Why is it named "crown"? Because C is king in the programming world :P

[See the Wiki for API references, Opcodes reference, Tutorials, and more](https://github.com/assyrianic/Tagha-Virtual-Machine/wiki)

## Purpose:
To be an embeddable scripting engine for C or C++ programs that uses unmodified C as a compiled scripting language. Though C is fairly low level to be used for scripting, Tagha is to be a good alternative to running a C plugin architecture via dynamically loaded .so/.dll's, especially with the advantage of being able to run C code without having to recompile the .so/.dll for different OSs. The plan is to use a C compiler's backend to generate and run bytecode on this VM.

## Why?:
Reason 1: so far, the only C interpreters (that I currently know of) are **CINT**, **PicoC**, **TCC [1]** and **Ch**:
- The problems with CINT is that it's old, clunky to use, outdated, and deprecated.
- PicoC is good but its problem is that it uses old-school interpreting (just runs literal code) instead of compiling to bytecode which would allow it to execute faster.
- The problem with Ch is that, though it's embeddable and updated, it's proprietary and it's unknown how it interprets code; as the usual problem with proprietary code, you don't know what code it could contain and there's no telling what security issues Ch could possibly have; not to mention that proprietary code shuts out enthusiastic individuals or groups from contributing to the software.
- **[1]** - Tiny C Compiler, it can compile and run scripts for testing but it can't be embedded the same way a scripting system can be. The scripting action is more or less a **great** way to test your program.

Reason 2: I've always wanted to create a useful, open-source piece of software for many to use and one of my particular interests happens to be computer languages, so why not make a scripting engine?

Reason 3: To learn from making a scripting engine from scratch. If I polish Tagha to a good amount, hopefully it'll be used in education to teach about virtual machines, scripting engines, or embedding for educational use!

The goal for TaghaVM is to...
+ 1. be a new and modernly optimized piece of software.
+ 2. have the clear execution speed advantage bytecode interpreters have over traditional interpreters.
+ 3. be open-source and open for anyone who wants to improve this 1st portion of what would become a scripting engine.
+ 4. be an example of how to do a minimal scripting system.

### Features
* floats and doubles supported by converting integers to floating point data by its bits. For example, "5.0f" is '0x40a00000' or '1084227584' in terms of its bits if it were a 4-byte int.
I usually use hexadecimals but decimal numbers work just as good. The giant numbers are then transformed into floating point via type-punning by a single union singleton.
* uses computed gotos (the ones that use a void\*) which is 20%-25% faster than a switch [[citation]](http://eli.thegreenplace.net/2012/07/12/computed-goto-for-efficient-dispatch-tables).
* "CPU" is "32-bit" as the word size is uint32. Numerical memory and stack addresses are 32-bits as well.
* Memory manipulation where loading data pops the top of stack into any memory address and storing pops off the top of stack into any memory address.
* has integer and float arithmetic, (un)conditional jumps, comparisons, and stack and memory manipulations. By default, the arithmetic is always 4 bytes because in C, integers are always promoted to the largest width.
* ~~call stack for functions. Supports function calls from function calls! (unless you overflow the call stack...)~~ Call stack has been merged into the overall Stack. When you do a function call, a new stack frame is created. You can use the loadsp/storesp opcodes to load/store function argument/parameter data. Returning destroys the stack frame as usual.
* It's Turing Complete! (lol)
* VM is little-endian.

## TODO list
- [x] add `q` or 'quad' opcodes for 64-bit data ~~AND make VM completely 64-bit (32-bit will NOT be available)~~.
- [x] add API for host applications to embed this VM.
- [x] figure out natives system.
- [x] create format for libraries and headers.
- [ ] figure out how to share types between host and scripts.
- [ ] figure out system of communication between Tagha C scripts.

## End Goals list
- [ ] complete, seamless embeddability to C (and by extension C++) programs. As smooth as how Angelscript binds to C++.
- [ ] compatibility of as much of the C standards possible, including C11.
- [ ] implement a compiler or compiler backend that generates bytecode for this VM. Possibility: Make GCC output Tagha bytecode using a custom GCC Backend but will take alot of effort. Maybe use Clang/LLVM backend to output Tagha bytecode?
- [ ] Windows compatibility/availability.
- [ ] Ultimate Goal: VM can bootstrap itself! (compiled to TaghaVM bytecode and ran by TaghaVM itself).
