# Introduction

Tagha is a little-endian, 64-bit stack & register based VM. Tagha employs both an operand stack and a callstack. Tagha utilizes 3 registers: `sp`, `cp`, and `lr`.

`sp` - is the Operand Stack Pointer.
`cp` - is the Call Stack Pointer.
`lr` - is the Link Register.

through the opcodes and the operand stack, the stack pointer can access 256 registers at any one time.

# VM Opcodes

## halt

### Description
Stops execution of a script/function and returns register `alaf`'s value as `int32_t`.


## nop

### Description
does jack and snap.


## alloc

### Description
reduces the op-stack pointer by n * 8 bytes to create room for data.


## redux

### Description
increases the op-stack pointer by n * 8 bytes to destroy data. Inverse of `alloc`.


## movi

### Description
copies an 8 byte immediate value to a register.


## mov

### Description
copies a source register's contents to a destination register.


## lra

### Description
"Load Register Address", gets the address of the op-stack pointer, adds an unsigned offset multiplied by 8 (word size of Tagha) to it, and puts the resulting address in a register.


## lea

### Description
"Load Effective Address", performs address calculation of a source register and signed offset to a destination register.


## ldvar

### Description
loads the address of a global variable into a register.


## ldfn

### Description
loads a function pointer to a register.


## ld1

### Description
loads a byte value from a memory address (added with a signed 2-byte offset) into a register.


## ld2

### Description
loads a short (2 byte) value from a memory address (added with a signed 2-byte offset) into a register.


## ld4

### Description
loads a long (4 byte) value from a memory address (added with a signed 2-byte offset) into a register.


## ld8

### Description
loads a long long (8 byte) value from a memory address (added with a signed 2-byte offset) into a register.


## ldu1

### Description
loads an unsigned byte value from a memory address (added with a signed 2-byte offset) into a register.


## ldu2

### Description
loads an unsigned short (2 byte) value from a memory address (added with a signed 2-byte offset) into a register.


## ldu4

### Description
loads an unsigned long (4 byte) value from a memory address (added with a signed 2-byte offset) into a register.


## st1

### Description
stores a byte value from a register into a memory address (added with a signed 2-byte offset).


## st2

### Description
stores a short (2 byte) value from a register into a memory address (added with a signed 2-byte offset).


## st4

### Description
stores a long (4 byte) value from a register into a memory address (added with a signed 2-byte offset).


## st8

### Description
stores a long long (8 byte) value from a register into a memory address (added with a signed 2-byte offset).


## add

### Description
Adds the integer value of a source register to a destination register.


## sub

### Description
Subtracts the integer value of a source register to a destination register.


## mul

### Description
multiplies the integer value of a source register to a destination register.


## idiv

### Description
divides the integer value of a source register to a destination register.


## mod

### Description
modulos the integer value of a source register to a destination register.


## neg

### Description
negates the integer value of a register.


## fadd

### Description
same as `add` but for floating point values. Math is done for the largest data width that's enabled (meaning if both doubles and floats are used, `fadd` will perform on doubles, regardless whether floats are defined or not).


## fsub

### Description
same as `sub` but for floating point values. Math is done for the largest data width that's enabled (meaning if both doubles and floats are used, `fsub` will perform on doubles, regardless whether floats are defined or not).


## fmul

### Description
same as `mul` but for floating point values. Math is done for the largest data width that's enabled (meaning if both doubles and floats are used, `fmul` will perform on doubles, regardless whether floats are defined or not).


## fdiv

### Description
same as `idiv` but for floating point values. Math is done for the largest data width that's enabled (meaning if both doubles and floats are used, `fdiv` will perform on doubles, regardless whether floats are defined or not).


## fneg

### Description
same as `neg` but for floating point values. Math is done for the largest data width that's enabled (meaning if both doubles and floats are used, `fneg` will perform on doubles, regardless whether floats are defined or not).


## bit_and

### Description
peforms bitwise AND of the integer value of a source register to a destination register.
Assembler can use `and` & `bit_and` naming of opcode.


## bit_or

### Description
peforms bitwise OR of the integer value of a source register to a destination register.
Assembler can use `or` & `bit_or` naming of opcode.


## bit_xor

### Description
peforms bitwise XOR of the integer value of a source register to a destination register.
Assembler can use `xor` & `bit_xor` naming of opcode.


## shl

### Description
peforms logical leftward bit shift of the integer value of a source register to a destination register.


## shr

### Description
peforms logical rightward bit shift of the integer value of a source register to a destination register.


## shar

### Description
peforms arithmetic rightward bit shift of the integer value of a source register to a destination register.


## bit_not

### Description
peforms bitwise NOT to a register.
Assembler can use `not` & `bit_not` naming of opcode.


## ilt

### Description
signed LESS-THAN comparison between two registers.


## ile

### Description
signed LESS-EQUAL comparison between two registers.


## ult

### Description
unsigned LESS-THAN comparison between two registers.


## ule

### Description
unsigned LESS-EQUAL comparison between two registers.


## cmp

### Description
EQUALITY comparison between two registers.


## flt

### Description
same as `ilt` but for floating point values. Math is done for the largest data width that's enabled (meaning if both doubles and floats are used, `flt` will perform on doubles, regardless whether floats are defined or not).


## fle

### Description
same as `ile` but for floating point values. Math is done for the largest data width that's enabled (meaning if both doubles and floats are used, `fle` will perform on doubles, regardless whether floats are defined or not).


## setc

### Description
stores the conditional flag to a register.


## f32tof64

### Description
converts a register's float32 value to float64 value. Does nothing if floats or doubles values aren't defined for tagha registers or floating point support isn't used at all.


## f64tof32

### Description
converts a register's float64 value to float32 value. Does nothing if floats or doubles values aren't defined for tagha registers or floating point support isn't used at all.


## itof64

### Description
converts a register's integer value to a float64, does nothing if doubles values aren't defined for registers or floating point support isn't used at all.


## itof32

### Description
converts a register's integer value to a float32, does nothing if floats values aren't defined for registers or floating point support isn't used at all.


## f64toi

### Description
converts a register's float64 value to an int, does nothing if floats values aren't defined for registers or floating point support isn't used at all.


## f32toi

### Description
converts a register's float32 value to a int, does nothing if floats values aren't defined for registers or floating point support isn't used at all.


## jmp

### Description
local instruction jump using a signed 8 byte immediate value.


## jz

### Description
**J**ump if **Z**ero by a comparison result using a signed 8 byte immediate value.


## jnz

### Description
**J**ump if **N**ot **Z**ero by a comparison result using a signed 8 byte immediate value.


## pushlr

### Description
pushes the link register to the call stack. Necessary if a function calls another.


## poplr

### Description
pops a return address from the call stack to the link register.


## call

### Description
jumps to a function using an unsigned 2 byte value as an index.
can also call a native function.


## callr

### Description
jumps to a function using a function pointer.
can also call a native function.


## ret

### Description
returns to a previous stack frame and instruction address.


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

Calling Convention is that return values should go or end up into `r0` which is the top of stack.
If the function returns nothing (`void`), `r0` may be used as pleased by the compiler.

If the function calls another (bytecode) function, it's REQUIRED to push the link register and pop it back at the end of the function's context.
Please be aware if the function requires additional registers so that the old top of stack for use as return data is kept track of.
If you give an argument that lives in r0 and the subsequent function requests 5 additional registers, then the 1st argument will be `r5` at least until the amount of registers in use has been reduced.
