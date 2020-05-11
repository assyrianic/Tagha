# C interface

# Datatypes

## enum TaghaIRType (Typedef'd as "STaghaIRType")

### TIR_ADD

### TIR_SUB

### TIR_MUL

### TIR_DIV

### TIR_MOD

### TIR_NEG

### TIR_AND

### TIR_OR

### TIR_XOR

### TIR_NOT

### TIR_SHL

### TIR_SHR

### TIR_EQ

### TIR_LE

### TIR_LT

### TIR_PUSH

### TIR_POP

### TIR_MOV

### TIR_MOV_IMM

### TIR_RET

### TIR_CALL

### TIR_JMP

### TIR_BR

### TIR_LOAD1, TIR_LOAD2, TIR_LOAD4, TIR_LOAD8

### TIR_LOADBP

### TIR_LOADFN

### TIR_LOADVAR

### TIR_STORE1, TIR_STORE2, TIR_STORE4, TIR_STORE8

### TIR_NOP

### TIR_FADD

### TIR_FSUB

### TIR_FMUL

### TIR_FDIV

### TIR_FNEG

### TIR_F32_TO_F64

### TIR_F64_TO_F32

### TIR_INT_TO_F32

### TIR_INT_TO_F64

### TIR_F64_TO_INT

### TIR_F32_TO_INT

### TIR_FLE

### TIR_FLT



## struct TaghaIR (Typedef'd as "STaghaIR")

### ir
union variant of various structs, register arrays, and instruction-oriented data.

### op
integer of type `enum TaghaIRType` that marks what kind of data the `ir` field utilizes.

### usign
used in comparison operations only, checks if a binary expression is signed (when false) or unsigned (when true).


## struct TaghaIRBlock (Typedef'd as "STaghaIRBlock")

### ir
vector/dynamic array of `struct TaghaIR` IR data.

### offset
Used when generating bytecode from the IR Basic Block, represents the offset of the block's respective jump address for calculating jump information.


## struct TaghaIRFunc (Typedef'd as "STaghaIRFunc")

### blocks
vector/dynamic array that stores `struct TaghaIRBlock` basic blocks which in turn store IR.



# Functions/Methods

## tagha_ir_binop
```c
struct TaghaIR tagha_ir_binop(const enum TaghaIRType op, const enum TaghaRegID r1, const enum TaghaRegID r2, const bool usign)
```

### Description
creates an IR object that defines a binary operation.

### Parameters
* `op` - IR type of the operation.
* `r1` - register that will be operated & store the result.
* `r2` - register that holds the second operand.
* `usign` - if the binary operation expression is signed or unsigned.

### Return Value
returns a `struct TaghaIR` object that represents a binary operation.


## tagha_ir_unop
```c
struct TaghaIR tagha_ir_unop(const enum TaghaIRType op, const enum TaghaRegID r)
```

### Description
creates an IR object that defines a unary operation.

### Parameters
* `op` - IR type of the operation.
* `r` - register that will be operated & store the result.

### Return Value
returns a `struct TaghaIR` object that represents a unary operation


## tagha_ir_mov_imm
```c
struct TaghaIR tagha_ir_mov_imm(const enum TaghaRegID r, const uint64_t val)
```

### Description
creates an IR object that defines an immediate value being copied to a register.

### Parameters
* `r` - register name that value will set.
* `val` - unsigned 64-bit int value that `r` will be set to.

### Return Value
returns a `struct TaghaIR` object that represents an immediate value copy operation.


## tagha_ir_mov_imm_f32
```c
struct TaghaIR tagha_ir_mov_imm_f32(const enum TaghaRegID r, const float32_t val)
```

### Description
creates an IR object that defines an immediate value being copied to a register.

### Parameters
* `r` - register name that value will set.
* `val` - 32-bit float value that `r` will be set to.

### Return Value
returns a `struct TaghaIR` object that represents an immediate value copy operation.


## tagha_ir_mov_imm_f64
```c
struct TaghaIR tagha_ir_mov_imm_f64(const enum TaghaRegID r, const float64_t val)
```

### Description
creates an IR object that defines an immediate value being copied to a register.

### Parameters
* `r` - register name that value will set.
* `val` - 64-bit float value that `r` will be set to.

### Return Value
returns a `struct TaghaIR` object that represents an immediate value copy operation.


## tagha_ir_push
```c
struct TaghaIR tagha_ir_push(const enum TaghaRegID r)
```

### Description
creates an IR object that defines pushing a register's value to the stack.

### Parameters
* `r` - register of whose value that will be pushed to the stack.

### Return Value
returns a `struct TaghaIR` object that represents pushing a register's value to the stack.


## tagha_ir_pop
```c
struct TaghaIR tagha_ir_pop(const enum TaghaRegID r)
```

### Description
creates an IR object that defines pop a value off the stack into a register.

### Parameters
* `r` - register that will store the popped value.

### Return Value
returns a `struct TaghaIR` object that represents popping a value off the stack into a register.


## tagha_ir_load_var
```c
struct TaghaIR tagha_ir_load_var(const enum TaghaRegID r, const uint64_t global)
```

### Description
creates an IR object that defines loading a global variable's address to a register.

### Parameters
* `r` - register that will store the global variable address.
* `global` - global variable's index in a script's var table.

### Return Value
returns a `struct TaghaIR` object that represents loading a global variable's address to a register.


## tagha_ir_load_func
```c
struct TaghaIR tagha_ir_load_func(const enum TaghaRegID r, const int64_t func)
```

### Description
creates an IR object that defines loading a function address to a register.

### Parameters
* `r` - register that will store the function address.
* `func` - function's index in a script's var table.

### Return Value
returns a `struct TaghaIR` object that represents loading a function's address to a register.


## tagha_ir_load_bp
```c
struct TaghaIR tagha_ir_load_bp(const enum TaghaRegID r, const int32_t offset)
```

### Description
creates an IR object that defines loading a local stack address to a register.

### Parameters
* `r` - register that will store the local address.
* `offset` - offset from the base stack pointer.

### Return Value
returns a `struct TaghaIR` object that represents loading a local stack address to a register.


## tagha_ir_load
```c
struct TaghaIR tagha_ir_load(const enum TaghaRegID r1, const enum TaghaRegID r2, const int32_t offset, const size_t bytes)
```

### Description
creates an IR object that defines loading from an address to a register.

### Parameters
* `r1` - destination register.
* `r2` - source register.
* `offset` - offset to add with `r2`'s pointer address.
* `bytes` - how large of a dereference size, valid values are 1, 2, 4, and 8. Invalid values give a default load size of 1 byte.

### Return Value
returns a `struct TaghaIR` object that represents loading from an address to a register.


## tagha_ir_store
```c
struct TaghaIR tagha_ir_store(const enum TaghaRegID r1, const enum TaghaRegID r2, const int32_t offset, const size_t bytes)
```

### Description
creates an IR object that defines storing from a register to an address.

### Parameters
* `r1` - destination register.
* `r2` - source register.
* `offset` - offset to add with `r1`'s pointer address.
* `bytes` - how large of a dereference size, valid values are 1, 2, 4, and 8. Invalid values give a default store size of 1 byte.

### Return Value
returns a `struct TaghaIR` object that represents storing from a register to an address.


## tagha_ir_call
```c
struct TaghaIR tagha_ir_call(const int64_t funcid, const size_t arg_count, const bool native, const bool from_reg)
```

### Description
creates an IR object that defines calling a function.

### Parameters
* `funcid` - index of the function in the function table of a script, should be negative if native call, should be a register ID if calling from a register like a function pointer.
* `arg_count` - number of args given in the function call.
* `native` - if the function call is a call to a native.
* `from_reg` - if we're calling from a register, `funcid` must then be a register ID.

### Return Value
returns a `struct TaghaIR` object that represents a function call.


## tagha_ir_jmp
```c
struct TaghaIR tagha_ir_jmp(const size_t label)
```

### Description
creates an IR object that defines an unconditional jump.

### Parameters
* `label` - index of the basic block that _should_ exist in the Tagha IR function.

### Return Value
returns a `struct TaghaIR` object that represents an unconditional jump.


## tagha_ir_br
```c
struct TaghaIR tagha_ir_br(const size_t label, const bool iszero)
```

### Description
creates an IR object that defines a conditional jump.

### Parameters
* `label` - index of the basic block that _should_ exist in the Tagha IR function.
* `iszero` - true if doing a Jump If Zero, false if doing a Jump if Zero.

### Return Value
returns a `struct TaghaIR` object that represents a conditional jump.


## tagha_ir_ret
```c
struct TaghaIR tagha_ir_ret(void)
```

### Description
creates an IR object that defines a return operation.

### Parameters
None.

### Return Value
returns a `struct TaghaIR` object that represents a return operation.


## tagha_ir_nop
```c
struct TaghaIR tagha_ir_nop(void)
```

### Description
creates an IR object that defines a nop.

### Parameters
None.

### Return Value
returns a `struct TaghaIR` object that represents a nop.



## taghair_bb_create
```c
struct TaghaIRBlock taghair_bb_create(void)
```

### Description
creates an IR Basic Block.

### Parameters
None.

### Return Value
returns a `struct TaghaIRBlock` object.


## taghair_bb_clear
```c
bool taghair_bb_clear(struct TaghaIRBlock *const tbb)
```

### Description
cleans up the IR block data.

### Parameters
* `tbb` - pointer to a `struct TaghaIRBlock`.

### Return Value
true if successful, false otherwise.


## taghair_bb_add
```c
bool taghair_bb_add(struct TaghaIRBlock *const tbb, struct TaghaIR *const tir)
```

### Description
appends an IR instruction to the IR Basic Block.

### Parameters
* `tbb` - pointer to a `struct TaghaIRBlock`.
* `tir` - pointer to a `struct TaghaIR` to append.

### Return Value
true if successful, false otherwise.



## taghair_func_create
```c
struct TaghaIRFunc taghair_func_create(void)
```

### Description
creates an IR Function.

### Parameters
None.

### Return Value
returns a `struct TaghaIRFunc` object.


## taghair_func_add
```c
bool taghair_func_add(struct TaghaIRFunc *const tfn, struct TaghaIRBlock *const tbb)
```

### Description
appends an IR Basic Block to the IR Function.

### Parameters
* `tfn` - pointer to a `struct TaghaIRFunc`.
* `tbb` - pointer to a `struct TaghaIRBlock` to append.

### Return Value
true if successful, false otherwise.


## taghair_func_clear
```c
void taghair_func_clear(struct TaghaIRFunc *const tfn)
```

### Description
clears all IR Basic Blocks in the IR Function.

### Parameters
* `tfn` - pointer to a `struct TaghaIRFunc`.

### Return Value
None.


## taghair_func_to_bytecode
```c
struct HarbolByteBuf taghair_func_to_bytecode(const struct TaghaIRFunc *const tfn)
```

### Description
transforms an IR Function from IR instructions into actual bytecode.

### Parameters
* `tfn` - pointer to a `const struct TaghaIRFunc`.

### Return Value
Bytebuffer containing the function's bytecode instructions.


## taghair_func_to_tasm
```c
struct HarbolString taghair_func_to_tasm(const struct TaghaIRFunc *const tfn)
```

### Description
transforms an IR Function from IR instructions into human-readable bytecode.

### Parameters
* `tfn` - pointer to a `const struct TaghaIRFunc`.

### Return Value
string object of the IR function in human-readable bytecode form.
