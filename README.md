# PLEASE USE THE V2 BRANCH, MASTER IS DEPRECATED. This is here to show the original, original code that led to the development of V2 branch.
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
* float64 (double, not 32bit float) support by converting integer to a float by its bits. For example, "5.0" double is '0x4014000000000000' or '4617315517961601024' in terms of its bits if it were a long.
I usually use hexadecimals but decimal numbers work just as good. The giant numbers are then transformed into doubles by typecasting into double\* and dereferencing.
* uses computed gotos (the ones that use a void\*) which is 20%-25% faster than a switch [citation needed for this one].
* "CPU" is "64-bit" as the entire stack and memory is uint64_t. I will likely change this to uint8_t so it simulates real memory better.
* memory manipulation where loading copies the top of the stack into any memory address and storing pops off the top of the stack into any memory address.
* has integer and float arithmetic, (un)conditional jumps, comparisons, and stack and memory manipulations.
* call stack for functions. Supports function calls from function calls! (unless you overflow the call stack...)
* It's Turing Complete! (lol)

### Instruction Set.
 - nop - does nothing.
 - push - push unsigned long onto the stack.
 - pop - decrements stack pointer
 - pushsp - pushes the stack pointer to top of stack
 - popsp - pops data at top of stack into stack pointer (lets you set or save where top of stack is)
 - add, fadd - arithmetic addition, int and float supported
-  sub, fsub - arithmetic subraction
-  mul, fmul - arithmetic multiplication
-  idiv, fdiv - arithmetic division (division with 0 is excepted by skipped operation and restoring stack, I should probably change that but restoring stack prevents an underflow.)
-  mod - modulo arithmetic
-  jmp - unconditional jump
-  lt - less than comparison, pop two items off stack and pushes result
 - gt - greater than comparison, functions same as 'lt'
 - cmp - equality comparison, checks if two popped-off-stack values are exactly the same, pushes result
-  jnz, jz - jump if not zero and jump if zero for conditional branching.
 - inc, dec - increment and decrement by 1 respectively, haven't added float support on this one!
-  shl, shr - bit shift left and shift right, pops two numbers off stack and does bit shift op
- and, or, xor, not - bitwise &, |, ^, and ~ operations.
-  cpy - cpy takes the item pointed to by the stack pointer, makes a copy, and pushes that onto the top of stack.
-  swap - swap takes the two top most items off the stack and swaps their position.
-  load - load a memory value to the top of the stack, pretty much push but with memory.
-  store - pops a value off the stack and stores it into a memory address.
- call - jumps to a section of code and saves the original instruction address to jump back to.
- ret - jumps back to the code that is after the `call` and call address argument.
-  halt - stops all execution.

## TODO list
- [x] add a callstack, call + ret instructions to support procedures.
- [ ] test call + ret for recursive functions.
- [ ] \(thinking about it) make an assembler or make compiler that generates binary.
- [ ] \(after implementing assembler) have opcodes encode into bytes for more compact executables.
- [ ] add memory addressing so we can support pointers (achieved saving stack pointer index to stack I believe?).
- [ ] add memory dereferencing (can't have memory addressing without dereferencing can we?)
- [ ] implementing a call stack and memory addressing means we would need a form of buffer overflow protection.
- [x] group all globals into a single struct
- [ ] segment ~~data stack and~~ memory into uint8_ts
- [ ] expand opcodes to take various sizes of data and sources. What I mean is make a push and pop for a byte, word (2 bytes), dword (4 bytes), and qword (8 bytes).
