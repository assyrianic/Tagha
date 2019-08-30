# Introduction

TaghaVM is a little-endian, 64-bit register-based VM. Tagha has 22, 64-bit general purpose registers, named after the Syriac alphabet, with 3 additional special purpose registers.

Tagha also uses a stack for establishing call frames, creating local variables, and for holding function arguments to functions that require more than 8 parameters. Stack operations moves the stack pointer by 8 bytes.

# VM Registers

## General Purpose Registers

### alaf
Accumulator register (return values go here).

### beth
### gamal
### dalath
### heh
### waw
### zain
### heth
### teth
### yodh
### kaf
### lamadh
### meem
### noon

### semkath
In both bytecode and native functions, contains the 1st param value.

### _eh
In both bytecode and native functions, contains the 2nd param value.

### peh
In both bytecode and native functions, contains the 3rd param value.

### sadhe
In both bytecode and native functions, contains the 4th param value.

### qof
In both bytecode and native functions, contains the 5th param value.

### reesh
In both bytecode and native functions, contains the 6th param value.

### sheen
In both bytecode and native functions, contains the 7th param value.

### taw
In both bytecode and native functions, contains the 8th param value.

## Special Purpose Registers

### rsp
Stack Pointer.

### rbp
Base Pointer aka stack frame pointer.
Can be repurposed as an extra register but its original value MUST be restored.


# VM Opcodes
The listed opcodes are in numerical order (HALT is 0x0, PUSHI is 0x01, ...)

## HALT

### Description
Stops execution of a script/function and returns register `alaf`'s value as `int32_t`.


## PUSHI

### Description
pushes an 8-byte immediate value to the stack.


## PUSH

### Description
pushes a register's contents to the stack.


## POP

### Description
pops a value from the stack into a register.


## LOADGLOBAL

### Description
loads the address of a global variable into a register.
Assembler can use `loadvar` & `loadglobal` naming of opcode.


## LOADADDR

### Description
loads an address from the local call frame.


## LOADFUNC

### Description
loads a function index to a register.


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
peforms leftward bit shift of the integer value of a source register to a destination register.


## SHR

### Description
peforms rightward bit shift of the integer value of a source register to a destination register.


## INC

### Description
increments the integer value of a register.


## DEC

### Description
decrements the integer value of a register.


## NEG

### Description
negates the integer value of a register.


## ILT

### Description
signed LESS-THAN comparison between two registers.


## ILE

### Description
signed LESS-EQUAL comparison between two registers.


## IGT

### Description
signed GREATER-THAN comparison between two registers.


## IGE

### Description
signed GREATER-EQUAL comparison between two registers.


## ULT

### Description
unsigned LESS-THAN comparison between two registers.


## ULE

### Description
unsigned LESS-EQUAL comparison between two registers.


## UGT

### Description
unsigned GREATER-THAN comparison between two registers.


## UGE

### Description
unsigned GREATER-EQUAL comparison between two registers.


## CMP

### Description
EQUALITY comparison between two registers.


## NEQ

### Description
NOT-EQUAL comparison between two registers.


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
jumps to a function using a register as an index.
can also call a native function.
automatically performs a function prologue.


## RET

### Description
returns to a previous stack frame and instruction address.
automatically performs function epilogue.


## NOP

### Description
Does Jack and Sh!%.


## FLT2DBL

### Description
converts a register's float32 value to float64 value. Does nothing if floats or doubles values aren't defined for the tagha registers.
Assembler can use `f2d` & `f4tof8` naming of opcode.


## DBL2FLT

### Description
converts a register's float64 value to float32 value. Does nothing if floats or doubles values aren't defined for the tagha registers or floating point support isn't used at all.
Assembler can use `d2f` & `f8tof4` naming of opcode.


## INT2DBL

### Description
converts a register's integer value to a float64, does nothing if doubles values aren't defined for registers or floating point support isn't used at all.
Assembler can use `i2d` & `itof8` naming of opcode.


## INT2FLT

### Description
converts a register's integer value to a float32, does nothing if floats values aren't defined for registers or floating point support isn't used at all.
Assembler can use `i2f` & `itof4` naming of opcode.


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


## INCF

### Description
same as INC but for floating point values. Math is done for the largest data width that's enabled (meaning if both doubles and floats are used, INCF will perform on doubles, regardless whether floats are defined or not).


## DECF

### Description
same as DEC but for floating point values. Math is done for the largest data width that's enabled (meaning if both doubles and floats are used, DECF will perform on doubles, regardless whether floats are defined or not).


## NEGF

### Description
same as NEG but for floating point values. Math is done for the largest data width that's enabled (meaning if both doubles and floats are used, NEGF will perform on doubles, regardless whether floats are defined or not).


## LTF

### Description
same as ILT but for floating point values. Math is done for the largest data width that's enabled (meaning if both doubles and floats are used, LTF will perform on doubles, regardless whether floats are defined or not).


## LEF

### Description
same as ILE but for floating point values. Math is done for the largest data width that's enabled (meaning if both doubles and floats are used, LEF will perform on doubles, regardless whether floats are defined or not).


## GTF

### Description
same as IGT but for floating point values. Math is done for the largest data width that's enabled (meaning if both doubles and floats are used, GTF will perform on doubles, regardless whether floats are defined or not).


## GEF

### Description
same as IGE but for floating point values. Math is done for the largest data width that's enabled (meaning if both doubles and floats are used, GEF will perform on doubles, regardless whether floats are defined or not).


## CMPF

### Description
same as CMP but for floating point values. Math is done for the largest data width that's enabled (meaning if both doubles and floats are used, CMPF will perform on doubles, regardless whether floats are defined or not).


## NEQF

### Description
same as NEQ but for floating point values. Math is done for the largest data width that's enabled (meaning if both doubles and floats are used, NEQF will perform on doubles, regardless whether floats are defined or not).


# VM Instruction Encoding

All opcodes take up a single byte and additional bytes depending on whether they operate on registers, immediate values, or doing memory operations.

Here's a crude ASCII art of the instruction encodings for Tagha opcodes:
---------------------------------------------------------------------
* | byte: opcode                                                    |
* | byte: opcode | byte: register id                                |
* | byte: opcode | byte: dest reg | byte: src reg                   |
* | byte: opcode | 8 bytes: imm (immediate) value                   |
* | byte: opcode | byte: register id | 8 bytes: imm value           |
* | byte: opcode | byte: dest reg | byte: src reg | 4 bytes: offset |
---------------------------------------------------------------------

# Calling Convention

Tagha's Calling convention is consistent for both bytecode and native C or (C++) functions.

Arguments 1 to 8 are stored in registers `semkath` to `taw`.

More than 8 params requires ALL parameters be pushed to the stack. Only 8 OR LESS can be passed by registers.
