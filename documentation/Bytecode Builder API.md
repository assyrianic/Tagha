# C interface

# Functions/Methods

## tagha_bc_op
```c
size_t tagha_bc_op(struct HarbolByteBuf *const tbc, const enum TaghaInstrSet op)
```

### Description
writes an opcode that takes no operands.

### Parameters
* `tbc` - pointer to a `struct HarbolByteBuf` object (byte buffer). Can be `NULL` to return the amount of bytes the opcode takes up.
* `op` - opcode value.

### Return Value
amount of bytes taken up by the opcode.


## tagha_bc_op_imm
```c
size_t tagha_bc_op_imm(struct HarbolByteBuf *const tbc, const enum TaghaInstrSet op, const uint64_t imm)
```

### Description
writes an opcode that takes an immediate value operand.

### Parameters
* `tbc` - pointer to a `struct HarbolByteBuf` object (byte buffer). Can be `NULL` to return the amount of bytes the opcode takes up.
* `op` - opcode value.
* `imm` - immediate value to be encoded.

### Return Value
amount of bytes taken up by the opcode.


## tagha_bc_op_reg
```c
size_t tagha_bc_op_reg(struct HarbolByteBuf *const tbc, const enum TaghaInstrSet op, const enum TaghaRegID reg)
```

### Description
writes an opcode that takes a single register operand.

### Parameters
* `tbc` - pointer to a `struct HarbolByteBuf` object (byte buffer). Can be `NULL` to return the amount of bytes the opcode takes up.
* `op` - opcode value.
* `reg` - register ID to be encoded.

### Return Value
amount of bytes taken up by the opcode.


## tagha_bc_op_reg_reg
```c
size_t tagha_bc_op_reg_reg(struct HarbolByteBuf *const tbc, const enum TaghaInstrSet op, const enum TaghaRegID dest, const enum TaghaRegID src)
```

### Description
writes an opcode that takes a two register operands.

### Parameters
* `tbc` - pointer to a `struct HarbolByteBuf` object (byte buffer). Can be `NULL` to return the amount of bytes the opcode takes up.
* `op` - opcode value.
* `dest` - destination register ID to be encoded.
* `src` - source register ID to be encoded.

### Return Value
amount of bytes taken up by the opcode.


## tagha_bc_op_reg_imm
```c
size_t tagha_bc_op_reg_imm(struct HarbolByteBuf *const tbc, const enum TaghaInstrSet op, const enum TaghaRegID reg, const uint64_t imm)
```

### Description
writes an opcode that takes a register and immediate value operand.

### Parameters
* `tbc` - pointer to a `struct HarbolByteBuf` object (byte buffer). Can be `NULL` to return the amount of bytes the opcode takes up.
* `op` - opcode value.
* `reg` - destination register ID to be encoded.
* `imm` - source immediate value to be encoded.

### Return Value
amount of bytes taken up by the opcode.


## tagha_bc_op_reg_mem
```c
size_t tagha_bc_op_reg_mem(struct HarbolByteBuf *const tbc, const enum TaghaInstrSet op, const enum TaghaRegID dest, const enum TaghaRegID src, const int32_t offset)
```

### Description
writes an opcode that takes two registers and an offset operand.

### Parameters
* `tbc` - pointer to a `struct HarbolByteBuf` object (byte buffer). Can be `NULL` to return the amount of bytes the opcode takes up.
* `op` - opcode value.
* `dest` - destination register ID to be encoded.
* `src` - source register ID to be encoded.
* `offset` - 4-byte offset to be encoded.

### Return Value
amount of bytes taken up by the opcode.
