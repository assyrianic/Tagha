# Introduction

Tagha is a little-endian, 64-bit stack-register based VM. Tagha employs an operand stack and a callstack. Tagha utilizes 4 registers: `osp`, `ofp`, `csp`, and `lr`.

`osp` - **O**perand **S**tack **P**ointer.
`ofp` - **O**perand **F**rame **P**ointer.
`csp` - **C**all **S**tack **P**ointer.
`lr`  - **L**ink **R**egister.

Through the opcodes & operand stack, the instruction set can allocate up to 256 registers per `alloc` opcode, access up to 256 live registers per register-based opcodes, and address up to 32,000+ register addresses.

# VM Opcodes

## halt
Stops execution of a script/function.


## nop
does jack.


## alloc
reduces the op-stack pointer by n * 8 bytes to create room for data.


## redux
increases the op-stack pointer by n * 8 bytes to destroy data. Inverse of `alloc`.


## movi
copies an 8 byte immediate value to a register.


## mov
copies a source register's contents to a destination register.


## lra
"Load Register Address", gets the address of the op-stack pointer, adds an unsigned 2-byte offset multiplied by 8 (word size of Tagha) to it, and puts the resulting address in a register.


## lea
"Load Effective Address", performs address calculation of a source register and signed 2-byte offset to a destination register.


## ldvar
loads the address of a global variable into a register.


## ldfn
loads a function pointer to a register.


## ld1
loads a byte value from a memory address (added with a signed 2-byte offset) into a register.


## ld2
loads a short (2 byte) value from a memory address (added with a signed 2-byte offset) into a register.


## ld4
loads a long (4 byte) value from a memory address (added with a signed 2-byte offset) into a register.


## ld8
loads a long long (8 byte) value from a memory address (added with a signed 2-byte offset) into a register.


## ldu1
loads an unsigned byte value from a memory address (added with a signed 2-byte offset) into a register.


## ldu2
loads an unsigned short (2 byte) value from a memory address (added with a signed 2-byte offset) into a register.


## ldu4
loads an unsigned long (4 byte) value from a memory address (added with a signed 2-byte offset) into a register.


## st1
stores a byte value from a register into a memory address (added with a signed 2-byte offset).


## st2
stores a short (2 byte) value from a register into a memory address (added with a signed 2-byte offset).


## st4
stores a long (4 byte) value from a register into a memory address (added with a signed 2-byte offset).


## st8
stores a long long (8 byte) value from a register into a memory address (added with a signed 2-byte offset).


## add
Adds the integer value of a source register to a destination register.


## sub
Subtracts the integer value of a source register to a destination register.


## mul
multiplies the integer value of a source register to a destination register.


## idiv
divides the integer value of a source register to a destination register.


## mod
modulos the integer value of a source register to a destination register.


## neg
negates the integer value of a register.


## fadd
same as `add` but for floating point values. Math is done for the largest data width that's enabled (meaning if both doubles and floats are used, `fadd` will perform on doubles, regardless whether floats are defined or not).


## fsub
same as `sub` but for floating point values. Math is done for the largest data width that's enabled (meaning if both doubles and floats are used, `fsub` will perform on doubles, regardless whether floats are defined or not).


## fmul
same as `mul` but for floating point values. Math is done for the largest data width that's enabled (meaning if both doubles and floats are used, `fmul` will perform on doubles, regardless whether floats are defined or not).


## fdiv
same as `idiv` but for floating point values. Math is done for the largest data width that's enabled (meaning if both doubles and floats are used, `fdiv` will perform on doubles, regardless whether floats are defined or not).


## fneg
same as `neg` but for floating point values. Math is done for the largest data width that's enabled (meaning if both doubles and floats are used, `fneg` will perform on doubles, regardless whether floats are defined or not).


## _and
peforms bitwise AND of the integer value of a source register to a destination register.
Assembler can use `and` & `_and` naming of opcode.


## _or
peforms bitwise OR of the integer value of a source register to a destination register.
Assembler can use `or` & `_or` naming of opcode.


## _xor
peforms bitwise XOR of the integer value of a source register to a destination register.
Assembler can use `xor` & `_xor` naming of opcode.


## sll
peforms logical leftward bit shift of the integer value of a source register to a destination register.


## srl
peforms logical rightward bit shift of the integer value of a source register to a destination register.


## sra
peforms arithmetic rightward bit shift of the integer value of a source register to a destination register.


## _not
peforms bitwise NOT to a register.
Assembler can use `not` & `_not` naming of opcode.


## ilt
signed LESS-THAN comparison between two registers.


## ile
signed LESS-EQUAL comparison between two registers.


## ult
unsigned LESS-THAN comparison between two registers.


## ule
unsigned LESS-EQUAL comparison between two registers.


## cmp
EQUALITY comparison between two registers.


## flt
same as `ilt` but for floating point values. Math is done for the largest data width that's enabled (meaning if both doubles and floats are used, `flt` will perform on doubles, regardless whether floats are defined or not).


## fle
same as `ile` but for floating point values. Math is done for the largest data width that's enabled (meaning if both doubles and floats are used, `fle` will perform on doubles, regardless whether floats are defined or not).


## f32tof64
converts a register's float32 value to float64 value. Does nothing if floats or doubles values aren't defined for tagha registers or floating point support isn't used at all.


## f64tof32
converts a register's float64 value to float32 value. Does nothing if floats or doubles values aren't defined for tagha registers or floating point support isn't used at all.


## itof64
converts a register's integer value to a float64, does nothing if doubles values aren't defined for registers or floating point support isn't used at all.


## itof32
converts a register's integer value to a float32, does nothing if floats values aren't defined for registers or floating point support isn't used at all.


## f64toi
converts a register's float64 value to an int, does nothing if floats values aren't defined for registers or floating point support isn't used at all.


## f32toi
converts a register's float32 value to a int, does nothing if floats values aren't defined for registers or floating point support isn't used at all.


## jmp
local instruction jump using a signed 4 byte immediate value.


## jz
**J**ump if **Z**ero by a comparison result using a signed 4 byte immediate value.


## jnz
**J**ump if **N**ot **Z**ero by a comparison result using a signed 4 byte immediate value.


## pushlr
pushes the link register to the call stack. Necessary if a function calls another.


## poplr
pops a return address from the call stack to the link register.


## call
jumps to a function using an unsigned 2 byte value as an index.
can also call a native function.


## callr
jumps to a function using a function pointer.
can also call a native function pointer.


## ret
loads the value of the link register to the program counter and resumes execution.


### Vector Opcodes
Vectors are basically packed registers that can be larger than 8 bytes but can be any width multiplied by an element size. For any vector opcode that takes a source and destination register, the destination register must be the same vector size or larger than the source register. In terms of technical operation, a vector register is basically an array.

When using a register as a vector, any element size less than word (64-bits) must be packed. For float-based vector operations, both `float32_t` and `float64_t` MUST BE DEFINED AND USEABLE, otherwise it's entirely a nop.


## setvinfo
sets the operational width and element length of vectors that are used for the vector opcodes. Valid element sizes are `byte`, `half`, `long`, and `word`. For float-based vector operations, only `long` and `word` are valid. If the element size is invalid, `word` size is used.

## vmov
copies a source register's contents as a vector (element size multiplied by vector width) to a destination register, destination registers is also used as a vector.

## vadd
same as `add` but source + destination registers are vectors.

## vsub
same as `sub` but source + destination registers are vectors.

## vmul
same as `mul` but source + destination registers are vectors.

## vdiv
same as `div` but source + destination registers are vectors.

## vmod
same as `mod` but source + destination registers are vectors.

## vneg
same as `neg` but source register is a vector.

## vfadd
Same as `vadd` but with floats.

## vfsub
Same as `vsub` but with floats.

## vfmul
Same as `vmul` but with floats.

## vfdiv
Same as `vdiv` but with floats.

## vfneg
Same as `vneg` but with floats.

## vand
same as `_and` but source + destination registers are vectors.

## vor
same as `_or` but source + destination registers are vectors.

## vxor
same as `_xor` but source + destination registers are vectors.

## vsll
same as `sll` but source + destination registers are vectors.

## vsrl
same as `srl` but source + destination registers are vectors.

## vsra
same as `sra` but source + destination registers are vectors.

## vnot
same as `_not` but source + destination registers are vectors.

## vcmp
same as `cmp` but source + destination registers are vectors.

## vilt
same as `ilt` but source + destination registers are vectors.

## vile
same as `ile` but source + destination registers are vectors.

## vult
same as `ult` but source + destination registers are vectors. Not a "deus vult" joke.

## vule
same as `ule` but source + destination registers are vectors.

## vflt
same as `flt` but source + destination registers are vectors.

## vfle
same as `fle` but source + destination registers are vectors.


### Super Instructions
These are instructions that are a combination of other, often used instructions.

## restore
combination of `poplr` and `ret` opcodes.

## leave
combination of `poplr`, `redux`, and `ret` opcodes.

## remit
combination of `redux` and `ret` opcodes.

## enter
combination of `alloc` and `pushlr` opcodes.


# VM Instruction Encoding

All opcodes take up a single byte and additional bytes depending on whether they operate on registers, immediate values, or doing memory operations.

Here's a crude ASCII art of the instruction encodings for Tagha opcodes by byte size:
---------------------------------------------------------------------
* | byte: opcode                                                    | 1  byte
* | byte: opcode | byte: register id / argument                     | 2  bytes
* | byte: opcode | byte: dest reg | byte: src reg                   | 3  bytes
* | byte: opcode | byte: dest reg | 2 bytes: imm                    | 4  bytes
* | byte: opcode | byte: dest reg | byte: src reg | 2 bytes: offset | 5  bytes
* | byte: opcode | 4 bytes: imm (immediate) value                   | 5  bytes
* | byte: opcode | byte: register id | 8 bytes: imm value           | 10 bytes
---------------------------------------------------------------------


# Calling Convention

### Call Setup
If a bytecode function calls another bytecode function (even itself) and that bytecode function is not tail-call optimized, then it's REQUIRED to preserve the link register (using `pushlr`) and restore it (using `poplr`) at the end of the function's context. External function calls ALWAYS preserves and restores the link register so `pushlr` and `poplr` is not necessary for external calls.

### Arguments
All arguments must be placed in registers from `r1` to `rN` as needed for the amount of arguments.
It's suggested that `r0` stores the number of arguments (or perhaps total byte size sum of all arguments) but not required.

In a different calling convention (called "Clobber Call"), a bytecode function may use `r0` to hold the first argument and clobber `r0` with the final return result.

For `va_list`, it's required to use a register to store a pointer that will point to two values, a pointer to the array of arguments and a number of the arguments.

Here's an example in pseudo-ASM:
```asm
r6 = 10
r5 = 15
r4 = 20

r3 = 3
r2 = &r4
r1 = &r2
```

In a native function, `r1` will be used as like `union TaghaVal[2]` where `[0]` will hold the array of arguments and `[1]` holds the argument count.

### Return Values
All functions, bytecode & native, must allocate one extra register as `r0` (top of stack) will be the return value register. `r0` may be used as pleased by the compiler if the function returns nothing (`void`).

Native functions clobber `r0` regardless as all native wrappers must return data, even if the C(++) function returns `void`.

`r0` may optionally hold a copy of any of the other arguments passed to the function for simplifying code size.

### Tips
Realistically, though Tagha allows functions to allocate & reduce the amount of available registers, it's also permissible to allocate a large amount of registers and use those same registers throughout the entire program.
