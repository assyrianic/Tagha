# C interface

# Datatypes

## Union TaghaVal (Typedef'd as "UTaghaVal")

### boolean
boolean value.

### int8
signed char value.

### int8a
signed char[8] value.

### int16
signed short value.

### int16a
signed short[4] value.

### int32
signed int value.

### int32a
signed int[2] value.

### int64
signed long long value.

### uint8
unsigned char value.

### uint8a
unsigned char[8] value.

### uint16
unsigned short value.

### uint16a
unsigned short[4] value.

### uint32
unsigned int value.

### uint32a
unsigned int[2] value.

### uint64
unsigned long long value.

### size
size_t value.

### ssize
ssize_t value.

### uintptr
uintptr_t value.

### inptr
inptr_t value.

### float32
32-bit float value. exists only if `TAGHA_FLOAT32_DEFINED` is defined.

### float32a
32-bit float[2] value. exists only if `TAGHA_FLOAT32_DEFINED` is defined.

### float64
64-bit float value. exists only if `TAGHA_FLOAT64_DEFINED` is defined.


## struct TaghaNative (Typedef'd as "STaghaNative")

### name
constant C string (const char *) of the name of the native function.

### cfunc
pointer to native C function.
The native must have the signature: `union TaghaVal (*)(struct TaghaModule *ctxt, size_t args, const union TaghaVal params[]);`


## enum TaghaErrCode

### tagha_err_instr_oob
integer code that defines an out of bounds error. Value of `-1`.

### tagha_err_none
integer code that defines execution was successful. Value of `0`.

### tagha_err_bad_ptr
integer code that defines either a `NULL` or invalid pointer dereference attempt. Value of `1`.

### tagha_err_no_func
integer code that defines a missing/NULL bytecode function. Value of `2`.

### tagha_err_no_cfunc
integer code that defines a missing/unresolved/NULL native function. Value of `3`.

### tagha_err_bad_script
integer code that defines an invalid script during loading. Value of `4`.

### tagha_err_stk_size
integer code that defines an invalid stack size given. Value of `5`.

### tagha_err_stk_overflow
integer code that defines a stack overflow. Value of `6`.


# Functions/Methods

## tagha_module_new_from_file
```c
struct TaghaModule *tagha_module_new_from_file(const char filename[]);
```

### Description
Allocates a `struct TaghaModule` pointer from a script file.

### Parameters
* `filename` - filename string of the script to load.

### Return Value
pointer to a newly allocated `struct TaghaModule` pointer, returns `NULL` if an error occured or problems reading the script.


## tagha_module_new_from_buffer
```c
struct TaghaModule *tagha_module_new_from_buffer(uint8_t buffer[]);
```

### Description
Allocates a `struct TaghaModule` pointer from an existing data buffer.

### Parameters
* `buffer` - pointer to raw script data.

### Return Value
pointer to a newly allocated `struct TaghaModule` pointer, return `NULL` if an error occured or problem reading the buffer data.


## tagha_module_free
```c
bool tagha_module_free(struct TaghaModule **module_ref);
```

### Description
Deallocates a `struct TaghaModule` pointer's data, deallocates the module pointer itself, and sets the pointer to `NULL`.

### Parameters
* `module_ref` - reference to a `struct TaghaModule` pointer.

### Return Value
boolean value whether the deallocation was successful or not.


## tagha_module_create_from_file
```c
struct TaghaModule tagha_module_create_from_file(const char filename[]);
```

### Description
builds a module from a file.

### Parameters
* `filename` - filename string of the script to load.

### Return Value
usable module structure if building the module from the file was a success, empty module structure otherwise.


## tagha_module_create_from_buffer
```c
struct TaghaModule tagha_module_create_from_buffer(uint8_t buffer[]);
```

### Description
builds a module from an existing data buffer.

### Parameters
* `buffer` - pointer to raw script data.

### Return Value
usable module structure if building the module from buffer was a success, empty module structure otherwise.


## tagha_module_clear
```c
bool tagha_module_clear(struct TaghaModule *module);
```

### Description
Deallocates a `struct TaghaModule`'s stored script data. The module pointer itself is NOT freed. Do NOT use this if you're using a buffer made from scratch.

### Parameters
* `module` - pointer to a `struct TaghaModule` object.

### Return Value
boolean value whether the deallocation was successful or not.


## tagha_module_print_vm_state
```c
void tagha_module_print_vm_state(const struct TaghaModule *module, bool hex);
```

### Description
Prints the registers and condition flag of a Tagha module object.

### Parameters
* `module` - pointer to a `struct TaghaModule` object.
* `hex` - option boolean to print general-purpose register data as hexadecimal instead of decimal.

### Return Value
None.


## tagha_module_get_error
```c
const char *tagha_module_get_error(const struct TaghaModule *module);
```

### Description
Returns a constant string of an error message to check what error had occurred. When an error or exception occurs in the VM portion of a module, a return value of `-1` is returned and the module's error field is set.

### Parameters
* `module` - pointer to a `struct TaghaModule` object.

### Return Value
constant C string (const char *) error message. Returns `NULL` if the module object was `NULL`.


## tagha_module_register_natives
```c
bool tagha_module_register_natives(struct TaghaModule *module, const struct TaghaNative natives[]);
```

### Description
Registers the native C functions to a module for data communication between C code and the script's bytecode.

### Parameters
* `module` - pointer to a `struct TaghaModule` object.
* `natives` - array of `struct TaghaNative`'s to register.

### Return Value
true or false if the operation was successful or not.


## tagha_module_register_ptr
```c
bool tagha_module_register_ptr(struct TaghaModule *module, const char varname[], void *ptr);
```

### Description
Registers a pointer to a script's global pointer variable by name. Example - registering 'stdin' standard input FILE *:
```c
tagha_module_register_ptr(module, "stdin", stdin);
```
Will crash the program if the variable name given is not a pointer on the script's side.

### Parameters
* `module` - pointer to a `struct TaghaModule` object.
* `varname` - string name of the global ptr variable to register.
* `ptr` - ptr value to register.

### Return Value
true or false if the operation was successful or not.


## tagha_module_get_var
```c
void *tagha_module_get_var(struct TaghaModule *module, const char name[]);
```

### Description
Returns a pointer to a script-defined global variable.
If the global variable is defined as a pointer in the script, then the returning pointer will be a pointer to that pointer.

### Parameters
* `module` - pointer to a `struct TaghaModule` object.
* `name` - string name of the global variable to retrieve.

### Return Value
pointer to the global variable, `NULL` if the variable doesn't exist or the module doesn't have script data/memory.


## tagha_module_get_flags
```c
uint32_t tagha_module_get_flags(const struct TaghaModule *module);
```

### Description
gets a script's flags.

### Parameters
* `module` - pointer to a `struct TaghaModule` object.

### Return Value
a `uint8_t` of the script's flags


## tagha_module_call
```c
int32_t tagha_module_call(struct TaghaModule *module, const char name[], size_t args, union TaghaVal params[], union TaghaVal *ret_val);
```

### Description
Manually calls a script function from C by name.

### Parameters
* `module` - pointer to a `struct TaghaModule` object.
* `name` - name of script function to invoke.
* `args` - amount of arguments to pass.
* `params` - function params to be passed, as an array of `union TaghaVal`.
* `ret_val` - pointer to `union TaghaVal` for use as a return value buffer.

### Return Value
returns a status `int32_t` value, returns `-1` if an error occurred. Use `ret_val` to get a proper return value.


## tagha_module_invoke
```c
int32_t tagha_module_invoke(struct TaghaModule *module, int64_t func_index, size_t args, union TaghaVal params[], union TaghaVal *ret_val);
```

### Description
Manually calls a script function from C by function index. Designed to be used for natives that take a function pointer from bytecode.

### Parameters
* `module` - pointer to a `struct TaghaModule` object.
* `func_index` - index of script function to invoke.
* `args` - amount of arguments to pass.
* `params` - function params to be passed, as an array of `union TaghaVal`.
* `ret_val` - pointer to `union TaghaVal` for use as a return value buffer.

### Return Value
returns a status `int32_t` value, returns `-1` if an error occurred. Use `ret_val` to get a proper return value.


## tagha_module_run
```c
int32_t tagha_module_run(struct TaghaModule *module, size_t argc, union TaghaVal argv[]);
```

### Description
Executes a script by calling its main function.

### Parameters
* `module` - pointer to a `struct TaghaModule` object.
* `argc` - length of the `argv` array.
* `argv` - array of `union TaghaVal`s of length `argc`.

### Return Value
returns a status `int32_t` value, returns `-1` if an error occurred.


## tagha_module_throw_error
```c
void tagha_module_throw_error(struct TaghaModule *module, int32_t err);
```

### Description
Allows a developer to manually throw a VM runtime exception. Only use within a native C or C++ function call that the script needs to stop running.

### Parameters
* `module` - pointer to a `struct TaghaModule` object.
* `err` - value that's higher or lower than 0, can be either a user defined error or an `enum TaghaErrCode` value.

### Return Value
None.
