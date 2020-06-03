# C interface

# Datatypes

## struct TaghaScriptBuilder (Typedef'd as "STaghaScriptBuilder")

### hdr
structure (in tagha.h) that stores header information.

### datatbl
byte buffer that stores the global var table information.

### functbl
byte buffer that stores the function table information.


# Functions/Methods

## tagha_tbc_gen_create
```c
struct TaghaScriptBuilder tagha_tbc_gen_create(void);
```

### Description
creates a `struct TaghaScriptBuilder` object.

### Parameters
None.

### Return Value
`struct TaghaScriptBuilder` object.


## tagha_tbc_gen_write_header
```c
void tagha_tbc_gen_write_header(struct TaghaScriptBuilder *tbc, uint32_t stack_size, uint32_t heap_size, uint32_t mem_size, uint32_t flags);
```

### Description
writes all the header information necessary. the magic verifier is also automatically encoded.

### Parameters
* `tbc` - pointer to a `struct TaghaScriptBuilder` object.
* `stack_size` - how many stack cells the script will need.
* `heap_size` - how much heap memory to add to the script.
* `mem_size` - total byte size of all the memory the stack needs (including stack size).
* `flags` - script flags to encode.

### Return Value
None.


## tagha_tbc_gen_write_func
```c
void tagha_tbc_gen_write_func(struct TaghaScriptBuilder *tbc, uint32_t flags, const char name[], const struct HarbolByteBuf *bytecode);
```

### Description
writes a single function into the function table buffer, increments the amount of functions written.

### Parameters
* `tbc` - pointer to a `struct TaghaScriptBuilder` object.
* `flags` - flags for if the function being written is a native or extern.
* `name` - NULL terminated string name of the function.
* `bytecode` - compiled or built bytecode of the function, should be `NULL` if the function is a native.

### Return Value
None.


## tagha_tbc_gen_write_var
```c
void tagha_tbc_gen_write_var(struct TaghaScriptBuilder *tbc, uint32_t flags, const char name[], const struct HarbolByteBuf *datum);
```

### Description
writes a single var into the global var table buffer, increments the amount of vars written.

### Parameters
* `tbc` - pointer to a `struct TaghaScriptBuilder` object.
* `flags` - flags of the var.
* `name` - NULL terminated string name of the var.
* `datum` - encoded byte buffer data of the var.

### Return Value
None.


## tagha_tbc_gen_to_buffer
```c
struct HarbolByteBuf tagha_tbc_gen_to_buffer(struct TaghaScriptBuilder *tbc);
```

### Description
creates a direct byte buffer out of a `struct TaghaScriptBuilder` object.
This **WILL** clear the script builder data.

### Parameters
* `tbc` - pointer to a `struct TaghaScriptBuilder` object.

### Return Value
`struct HarbolByteBuf` object containing the entire script.


## tagha_tbc_gen_create_file
```c
bool tagha_tbc_gen_create_file(struct TaghaScriptBuilder *tbc, const char filename[]);
```

### Description
creates a Tagha Bytecode Script out of a `struct TaghaScriptBuilder` object.
This **WILL** clear the script builder data.

### Parameters
* `tbc` - pointer to a `struct TaghaScriptBuilder` object.
* `filename` - name of the file. Does NOT add the `.tbc` extension.

### Return Value
true if successful, false otherwise.
