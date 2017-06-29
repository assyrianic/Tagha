# C Virtual Machine
minimal but complex stack-based virtual machine, written in C.

### Features
* float64 (double, not 32bit float) support by converting integer to a float by its bits. For example, "5.0" double is '0x4014000000000000' or '4617315517961601024' in terms of its bits if it were a long.
I usually use hexadecimals but decimal numbers work just as good. The giant numbers are then transformed into doubles by typecasting into double\* and dereferencing.
* uses computed gotos (the ones that use a void\*) which is 20%-25% faster than a switch [citation needed for this one].
* "CPU" is "64-bit" as the entire stack and memory is uint64_t. I will likely change this to uint8_t so it simulates real memory better.
* memory manipulation where loading copies the top of the stack into any memory address and storing pops off the top of the stack into any memory address.
* has integer and float arithmetic, conditional and unconditional jumps, comparisons, and stack and register manipulations,

### Instruction Set.
 - nop - does nothing.
 - push - push unsigned long onto the stack.
 - pop - decrements stack pointer
 - add, fadd - arithmetic addition, int and float supported
-  sub, fsub - arithmetic subraction
-  mul, fmul - arithmetic multiplication
-  idiv, fdiv - arithmetic division (division with 0 is excepted by skipped operation and restoring stack, I should probably change that but restoring stack prevents an underflow.)
-  mod - modulo arithmetic
-  jmp - unconditional jump
-  lt - less than comparison, pop two items off stack and pushes result
 - gt - greater than comparison, same as 'lt'
 - cmp - equality comparison, checks if two popped-off-stack values are exactly the same, pushes result
-  jnz, jz - jump if not zero and jump if zero for conditional branching.
 - inc, dec - increment and decrement by 1 respectively, haven't added float support on this one!
-  shl, shr - bit shift left and shift right, pops two numbers off stack and does bit shift op
-  cpy - cpy takes the item pointed to by the stack pointer, makes a copy, and pushes that onto the top of stack.
-  swap - swap takes the two top most items off the stack and swaps their position.
-  load - load a register value to the top of the stack
-  store - pops a value off the stack and stores it into a register
-  halt - stops all execution.

## TODO list
- [ ] add a callstack, call + ret instructions to support procedures.
- [ ] \(thinking about it) make an assembler or make compiler that generates binary.
- [x] add bitwise AND, OR, XOR, NOT operations
- [ ] add memory addressing so we can support pointers (achieved saving stack pointer index to stack I believe?).
- [ ] add memory dereferencing (can't have memory addressing without dereferencing can we?)
- [ ] implementing a call stack and memory addressing means we would need a form of buffer overflow protection.
- [ ] group all globals into a single struct
