# C interface

# Datatypes

## Union TaghaVal

### b00l
bool value.

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

### intptr
intptr_t value.

### float32
32-bit float value. exists only if `TAGHA_FLOAT32_DEFINED` is defined.

### float32a
32-bit float[2] value. exists only if `TAGHA_FLOAT32_DEFINED` is defined.

### float64
64-bit float value. exists only if `TAGHA_FLOAT64_DEFINED` is defined.


## struct TaghaNative

### name
constant C string (const char*) of the name of the native function.

### cfunc
pointer to native C function.
The native must have the signature: `union TaghaVal (*)(struct TaghaModule *ctxt, const union TaghaVal params[]);`


## enum TaghaErrCode

### TaghaErrNone
integer code that defines execution was successful.

### TaghaErrOpcodeOOB
integer code that defines an out of bounds instruction error (only used during debugging).

### TaghaErrBadPtr
integer code that defines either a `NULL` or invalid pointer dereference attempt.

### TaghaErrBadFunc
integer code that defines a missing/NULL bytecode function.

### TaghaErrBadNative
integer code that defines a missing/unresolved/NULL native function.

### TaghaErrOpStackOF
integer code that defines a stack overflow.

### TaghaErrBadNative
integer code that defines a bad external call, whether the function owner is nil, the function wasn't linked, or the data was nil.


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


## tagha_module_clear
```c
bool tagha_module_clear(struct TaghaModule *module);
```

### Description
Deallocates a `struct TaghaModule` pointer's data.

### Parameters
* `module` - pointer to a `struct TaghaModule` object.

### Return Value
bool value whether the deallocation was successful or not.


## tagha_module_free
```c
bool tagha_module_free(struct TaghaModule **module_ref);
```

### Description
Deallocates a `struct TaghaModule` pointer's data, deallocates the module pointer itself, and sets the pointer to `NULL`.

### Parameters
* `module_ref` - reference to a `struct TaghaModule` pointer.

### Return Value
bool value whether the deallocation was successful or not.



## tagha_module_get_err
```c
const char *tagha_module_get_err(const struct TaghaModule *module);
```

### Description
Returns a constant string of an error message to check what error had occurred. When an error or exception occurs in the VM portion of a module, a return value of `-1` is returned and the module's error field is set.

### Parameters
* `module` - pointer to a `struct TaghaModule` object.

### Return Value
constant C string (const char *) error message. Never returns `NULL`.


## tagha_module_link_natives
```c
void tagha_module_link_natives(struct TaghaModule *module, const struct TaghaNative natives[]);
```

### Description
Links the native C functions to a module for data communication between C code and the script's bytecode.

### Parameters
* `module` - pointer to a `struct TaghaModule` object.
* `natives` - array of `struct TaghaNative`'s to register.

### Return Value
None.


## tagha_module_link_ptr
```c
bool tagha_module_link_ptr(struct TaghaModule *module, const char name[], uintptr_t ptr);
```

### Description
Registers a pointer to a script's global pointer variable by name. Example - registering 'stdin' standard input FILE*:
```c
tagha_module_link_ptr(module, "stdin", ( uintptr_t )stdin);
```
Will crash the program if the variable name given is not a pointer on the script's side.

### Parameters
* `module` - pointer to a `struct TaghaModule` object.
* `name` - string name of the global ptr variable to register.
* `ptr` - uintptr_t value to link.

### Return Value
true or false if the operation was successful or not.


## tagha_module_get_var
```c
void *tagha_module_get_var(const struct TaghaModule *module, const char name[]);
```

### Description
Returns a pointer to a script-defined global variable.
If the global variable is defined as a pointer in the script, then the returning pointer will be a pointer to that pointer.

### Parameters
* `module` - pointer to a `const struct TaghaModule` object.
* `name` - string name of the global variable to retrieve.

### Return Value
pointer to the global variable, `NULL` if the variable doesn't exist or the module doesn't have script data/memory.


## tagha_module_get_func
```c
TaghaFunc tagha_module_get_func(const struct TaghaModule *module, const char name[]);
```

### Description
Returns a pointer to a script-defined function.

### Parameters
* `module` - pointer to a `const struct TaghaModule` object.
* `name` - string name of the function to retrieve.

### Return Value
returns a `TaghaFunc` object, NULL if error occurred.


## tagha_module_get_flags
```c
uint32_t tagha_module_get_flags(const struct TaghaModule *module);
```

### Description
gets a script's flags.

### Parameters
* `module` - pointer to a `struct TaghaModule` object.

### Return Value
a `uint32_t` of the script's flags.


## tagha_module_heap_alloc
```c
uintptr_t tagha_module_heap_alloc(struct TaghaModule *module, size_t size);
```

### Description
allocates memory from the script's heap.

### Parameters
* `module` - pointer to a `struct TaghaModule` object.
* `size` - how many bytes to allocate.

### Return Value
a pointer allocated from a module's runtime casted to `uintptr_t`, `NIL` if script heap is exhausted.


## tagha_module_heap_free
```c
bool tagha_module_heap_free(struct TaghaModule *module, uintptr_t ptr);
```

### Description
returns memory back to a script's heap.

### Parameters
* `module` - pointer to a `struct TaghaModule` object.
* `ptr` - `uintptr_t` of a pointer that was allocated from the script's heap.

### Return Value
true if operation was successful, false otherwise.


## tagha_module_call
```c
bool tagha_module_call(struct TaghaModule *module, const char name[], size_t args, const union TaghaVal params[], union TaghaVal *retval);
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
true if successful AND no errors occurred, false otherwise.


## tagha_module_invoke
```c
bool tagha_module_invoke(struct TaghaModule *module, TaghaFunc func, size_t args, const union TaghaVal params[], union TaghaVal *retval);
```

### Description
Manually calls a script function from C by function pointer.
Designed to be used for natives that use a function pointer from bytecode as a parameter.

### Parameters
* `module` - pointer to a `struct TaghaModule` object.
* `func` - `TaghaFunc` object.
* `args` - amount of arguments to pass.
* `params` - function params to be passed, as an array of `union TaghaVal`.
* `ret_val` - pointer to `union TaghaVal` for use as a return value buffer.

### Return Value
true if successful AND no errors occurred, false otherwise.


## tagha_module_run
```c
int tagha_module_run(struct TaghaModule *module, size_t argc, const union TaghaVal argv[]);
```

### Description
Executes a script by calling its main function.

### Parameters
* `module` - pointer to a `struct TaghaModule` object.
* `argc` - length of the `argv` array.
* `argv` - array of `union TaghaVal`s of length `argc`.

### Return Value
true if successful AND no errors occurred, false otherwise as an `int`.


## tagha_module_throw_err
```c
void tagha_module_throw_err(struct TaghaModule *module, int32_t err);
```

### Description
Allows a developer to manually throw a VM runtime exception. Only use within a native C or C++ function call that the script needs to stop running.

### Parameters
* `module` - pointer to a `struct TaghaModule` object.
* `err` - value that's higher or lower than 0, can be either a user defined error or an `enum TaghaErrCode` value.

### Return Value
None.


## tagha_module_link_module
```c
void tagha_module_link_module(struct TaghaModule *module, const struct TaghaModule *lib);
```

### Description
Resolves unlinked functions in `module` from `lib`. (NOTE: `lib` could possibly have its own unresolved linkage.)
Two modules _can_ link to functions from one another and a "lib" module can have its own unresolved links. This API function is a helper to resolve unlinked functions that are in `module`.

### Parameters
* `module` - pointer to a `struct TaghaModule` object.
* `lib` - const pointer to a `struct TaghaModule` object.

### Return Value
None.