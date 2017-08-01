# currently on hold, use 'v2' branch

# Crown Virtual Machine
Crown is minimal yet complex stack-based virtual machine, written in C, designed to run C. Why is it named 'Crown'? Because C is king in the programming world :P Crown runs compiled C bytecode as scripts. Scripts are sandboxed with their own memory and stacks.

## Purpose:
to be an embeddable C-as-scripting-language scripting engine for C or C++ programs. The plan is to ~~create or~~ use a C compiler frontend to generate and run bytecode on this vm.

## Why?:
so far, the only C interpreters (that I currently know of) are **CINT**, **PicoC**, **TCC [1]** and **Ch**:
- The problems with CINT is that it's old, clunky to use, outdated, and deprecated.
- PicoC is good but its problem is that it uses old-school interpreting (just runs literal code) instead of compiling to bytecode and running much faster.
- The problem with Ch is that, though it's embeddable and updated, it's proprietary and it's unknown how it interprets code; as the usual problem with proprietary code, you don't know what code it could contain and there's no telling what security issues that code could have.
- **[1]** - Tiny C Compiler, it can compile and run scripts for testing but it cannot be embedded. The scripting action is more or less a **great** way to test your program! libtcc only acts as a JIT IIRC.

The goal for this VM is to...
+ 1. be a new and modernly optimized piece of software for C-as-a-scripting-language.
+ 2. have the clear execution speed advantage bytecode interpreters have over traditional interpreters.
+ 3. be open-source and open for anyone who wants to improve this 1st portion of what would become a scripting engine.

### Features
* float (float32, not 64-bit float) support by converting integer to a float by its bits. For example, "5.0f" is '0x40a00000' or '1084227584' in terms of its bits if it were an int.
I usually use hexadecimals but decimal numbers work just as good. The giant numbers are then transformed into floats via type-punning by a single union singleton.
* uses computed gotos (the ones that use a void\*) which is 20%-25% faster than a switch [citation needed for this one].
* "CPU" is "64-bit" as the word size is 64 bits. Numerical memory addresses are 64-bits as well.
* memory manipulation where loading copies the top of the stack into any memory address and storing pops off the top of the stack into any memory address.
* has integer and float arithmetic, (un)conditional jumps, comparisons, and stack and memory manipulations. By default, the arithmetic is always 4 bytes because in C, integers are always promoted to the largest width.
* call stack for functions. Supports function calls from function calls! (unless you overflow the call stack...)
* It's Turing Complete! (lol)
* VM is little-endian.

### Crown's Instruction Set.
 - nop - does nothing.
 
 - pushq - push 8-byte data to Top of Stack (TOS).
 - pushl - push 4-byte data to TOS.
 
 - halt - stops all execution.

## TODO list
- [ ] add call + ret instructions to support procedures.
- [ ] add API for host applications to embed this VM.
- [ ] design or think up better Data, BSS, and Code format for bytecode binaries.
- [ ] create format for libraries and headers.

## End Goals list
- [ ] complete, seamless embeddability to C (and by extension C++) programs. As smooth as how Angelscript binds to C++.
- [ ] be as compatible to the C standards as possible, including C11.
- [ ] implement a compiler that generates the bytecode for this VM.
- [ ] 64-bit version availability.
- [ ] Windows compatibility/availability.
- [ ] VM-Interpreter can host itself from bytecode!
