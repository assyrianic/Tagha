# Introduction

TaghaVM is a little-endian, 64-bit register-based VM. Tagha has 32, 64-bit general purpose registers, named after the Syriac alphabet, with 3 additional special purpose registers.

Tagha also uses a stack for establishing call frames, creating local variables, and for holding additional function arguments. Stack operations moves the stack pointer by 8 bytes.

# VM Registers

## General Purpose Registers

### alaf | 0
Accumulator register (return values go here).

### beth | 1
### gamal | 2
### dalath | 3
### heh | 4
### waw | 5
### zain | 6
### heth | 7
### teth | 8
### yodh | 9
### kaf | 10
### lamadh | 11
### meem | 12
### noon | 13

### semkath | 14 | arg0
In bytecode & native functions, contains the 1st param value.

### _eh | 15 | arg1
In bytecode & native functions, contains the 2nd param value.

### peh | 16 | arg2
In bytecode & native functions, contains the 3rd param value.

### sadhe | 17 | arg3
In bytecode & native functions, contains the 4th param value.

### qof | 18 | arg4
In bytecode & native functions, contains the 5th param value.

### reesh | 19 | arg5
In bytecode & native functions, contains the 6th param value.

### sheen | 20 | arg6
In bytecode & native functions, contains the 7th param value.

### taw | 21 | arg7
In bytecode & native functions, contains the 8th param value.

### veth | 22 | arg8
In bytecode & native functions, contains the 9th param value.

### ghamal | 23 | arg9
In bytecode & native functions, contains the 10th param value.

### dhalath | 24 | arg10
In bytecode & native functions, contains the 11th param value.

### khaf | 25 | arg11
In bytecode & native functions, contains the 12th param value.

### feh | 26 | arg12
In bytecode & native functions, contains the 13th param value.

### thaw | 27 | arg13
In bytecode & native functions, contains the 14th param value.

### zeth | 28 | arg14
In bytecode & native functions, contains the 15th param value.

### dadeh | 29 | arg15
In bytecode & native functions, contains the 16th param value.

## Special Purpose Registers

### sp | 30
Stack Pointer.

### bp | 31
Base Pointer aka stack frame pointer.
Can be repurposed as an extra register but its original value MUST be restored.


# VM Opcodes
The listed opcodes are in numerical order (HALT is 0x0, PUSH is 0x01, ...)

## HALT

### Description
Stops execution of a script/function and returns register `alaf`'s value as `int32_t`.


## PUSH

### Description
pushes a register's contents to the stack.


## POP

### Description
pops a value from the stack into a register.


## LDVAR

### Description
loads the address of a global variable into a register.


## LDADDR

### Description
loads an address from the local call frame.


## LDFUNC

### Description
loads a function pointer to a register.


## MOVI

### Description
copies an 8 byte immediate value to a register.


## MOV

### Description
copies a source register's contents to a destination register.


## LD1

### Description
loads a byte value from a memory address (added with a signed 4-byte offset) into a register.


## LD2

### Description
loads a short (2 byte) value from a memory address (added with a signed 4-byte offset) into a register.


## LD4

### Description
loads a long (4 byte) value from a memory address (added with a signed 4-byte offset) into a register.


## LD8

### Description
loads a long long (8 byte) value from a memory address (added with a signed 4-byte offset) into a register.


## LDS1

### Description
loads a signed byte value from a memory address (added with a signed 4-byte offset) into a register.


## LDS2

### Description
loads a signed short (2 byte) value from a memory address (added with a signed 4-byte offset) into a register.


## LDS4

### Description
loads a signed long (4 byte) value from a memory address (added with a signed 4-byte offset) into a register.


## ST1

### Description
stores a byte value from a register into a memory address (added with a signed 4-byte offset).


## ST2

### Description
stores a short (2 byte) value from a register into a memory address (added with a signed 4-byte offset).


## ST4

### Description
stores a long (4 byte) value from a register into a memory address (added with a signed 4-byte offset).


## ST8

### Description
stores a long long (8 byte) value from a register into a memory address (added with a signed 4-byte offset).


## ADD

### Description
Adds the integer value of a source register to a destination register.


## SUB

### Description
Subtracts the integer value of a source register to a destination register.


## MUL

### Description
multiplies the integer value of a source register to a destination register.


## DIVI

### Description
divides the integer value of a source register to a destination register.


## MOD

### Description
modulos the integer value of a source register to a destination register.


## BIT_AND

### Description
peforms bitwise AND of the integer value of a source register to a destination register.
Assembler can use `and` & `bit_and` naming of opcode.


## BIT_OR

### Description
peforms bitwise OR of the integer value of a source register to a destination register.
Assembler can use `or` & `bit_or` naming of opcode.


## BIT_XOR

### Description
peforms bitwise XOR of the integer value of a source register to a destination register.
Assembler can use `xor` & `bit_xor` naming of opcode.


## BIT_NOT

### Description
peforms bitwise NOT to a register.
Assembler can use `not` & `bit_not` naming of opcode.


## SHL

### Description
peforms logical leftward bit shift of the integer value of a source register to a destination register.


## SHR

### Description
peforms logical rightward bit shift of the integer value of a source register to a destination register.


## SHAL

### Description
peforms arithmetic leftward bit shift of the integer value of a source register to a destination register.


## SHAR

### Description
peforms arithmetic rightward bit shift of the integer value of a source register to a destination register.


## NEG

### Description
negates the integer value of a register.


## ILT

### Description
signed LESS-THAN comparison between two registers.


## ILE

### Description
signed LESS-EQUAL comparison between two registers.


## ULT

### Description
unsigned LESS-THAN comparison between two registers.


## ULE

### Description
unsigned LESS-EQUAL comparison between two registers.


## CMP

### Description
EQUALITY comparison between two registers.


## JMP

### Description
local instruction jump using a signed 8 byte immediate value.


## JZ

### Description
**J**ump if **Z**ero by a comparison result using a signed 8 byte immediate value.


## JNZ

### Description
**J**ump if **N**ot **Z**ero by a comparison result using a signed 8 byte immediate value.


## CALL

### Description
jumps to a function using an unsigned 8 byte immediate value as an index.
can also call a native function.
automatically performs a function prologue.


## CALLR

### Description
jumps to a function using an item pointer.
can also call a native function.
automatically performs a function prologue.


## RET

### Description
returns to a previous stack frame and instruction address.
automatically performs function epilogue.


## NOP

### Description
Does Jack and Sh!%.


## F32TOF64

### Description
converts a register's float32 value to float64 value. Does nothing if floats or doubles values aren't defined for the tagha registers.


## F64TOF32

### Description
converts a register's float64 value to float32 value. Does nothing if floats or doubles values aren't defined for the tagha registers or floating point support isn't used at all.


## ITOF64

### Description
converts a register's integer value to a float64, does nothing if doubles values aren't defined for registers or floating point support isn't used at all.


## ITOF32

### Description
converts a register's integer value to a float32, does nothing if floats values aren't defined for registers or floating point support isn't used at all.


## F64TOI

### Description
converts a register's float64 value to an int, does nothing if floats values aren't defined for registers or floating point support isn't used at all.


## F32TOI

### Description
converts a register's float32 value to a int, does nothing if floats values aren't defined for registers or floating point support isn't used at all.


## ADDF

### Description
same as ADD but for floating point values. Math is done for the largest data width that's enabled (meaning if both doubles and floats are used, ADDF will perform on doubles, regardless whether floats are defined or not).


## SUBF

### Description
same as SUB but for floating point values. Math is done for the largest data width that's enabled (meaning if both doubles and floats are used, SUBF will perform on doubles, regardless whether floats are defined or not).


## MULF

### Description
same as MUL but for floating point values. Math is done for the largest data width that's enabled (meaning if both doubles and floats are used, MULF will perform on doubles, regardless whether floats are defined or not).


## DIVF

### Description
same as DIVI but for floating point values. Math is done for the largest data width that's enabled (meaning if both doubles and floats are used, DIVF will perform on doubles, regardless whether floats are defined or not).


## NEGF

### Description
same as NEG but for floating point values. Math is done for the largest data width that's enabled (meaning if both doubles and floats are used, NEGF will perform on doubles, regardless whether floats are defined or not).


## LTF

### Description
same as ILT but for floating point values. Math is done for the largest data width that's enabled (meaning if both doubles and floats are used, LTF will perform on doubles, regardless whether floats are defined or not).


## LEF

### Description
same as ILE but for floating point values. Math is done for the largest data width that's enabled (meaning if both doubles and floats are used, LEF will perform on doubles, regardless whether floats are defined or not).


# VM Instruction Encoding

All opcodes take up a single byte and additional bytes depending on whether they operate on registers, immediate values, or doing memory operations.

Here's a crude ASCII art of the instruction encodings for Tagha opcodes order by byte size:
---------------------------------------------------------------------
* | byte: opcode                                                    | 1  byte
* | byte: opcode | byte: register id                                | 2  bytes
* | byte: opcode | byte: dest reg | byte: src reg                   | 3  bytes
* | byte: opcode | byte: dest reg | byte: src reg | 4 bytes: offset | 7  bytes
* | byte: opcode | 8 bytes: imm (immediate) value                   | 9  bytes
* | byte: opcode | byte: register id | 8 bytes: imm value           | 10 bytes
---------------------------------------------------------------------

# Calling Convention

Tagha's Calling convention is consistent for both bytecode and native C or (C++) functions.

Arguments 1 to 16 are stored in registers `semkath` to `dadeh`.

More than 16 params requires passing a va_list. Only 16 OR LESS can be passed by registers.
