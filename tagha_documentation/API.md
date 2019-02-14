# C interface

# Datatypes

## Union TaghaVal
### Bool
boolean value.

### Int8
signed char value.

### Int16
signed short value.

### Int32
signed long value.

### Int64
signed long long value.

### UInt8
unsigned char value.

### UInt16
unsigned short value.

### UInt32
unsigned long value.

### UInt64
unsigned long long value.

### SizeInt
size_t value.

### UIntPtr
uintptr_t value.

### IntPtr
intptr_t value.

### Float
32-bit float value. exists only if `__TAGHA_FLOAT32_DEFINED` is defined.

### Double
64-bit float value. exists only if `__TAGHA_FLOAT64_DEFINED` is defined.

### PtrBool
pointer to bool value.

### PtrInt8
pointer to signed char value.

### PtrInt16
pointer to signed short value.

### PtrInt32
pointer to signed long value.

### PtrInt64
pointer to signed long long value.

### PtrUInt8
pointer to unsigned char value.

### PtrUInt16
pointer to unsigned short value.

### PtrUInt32
pointer to unsigned long value.

### PtrUInt64
pointer to unsigned long long value.

### PtrSizeInt
pointer to size_t value.

### PtrFloat
pointer to 32-bit float value. exists only if `__TAGHA_FLOAT32_DEFINED` is defined.

### PtrDouble
pointer to 64-bit float value. exists only if `__TAGHA_FLOAT64_DEFINED` is defined.

### Ptr
pointer to void type value.

### PtrCStr
pointer to a C string (const char *)

### PtrSelf
pointer to union TaghaVal value.


## struct TaghaNative

### Name
constant C string (const char *) of the name of the native function.

### NativeCFunc
pointer to native C function (C++ interface has it's own implementation).
The native must have the signature: `void (*)(struct TaghaModule *ctxt, union TaghaVal *ret, size_t args, union TaghaVal params[]);`


## enum TaghaErrCode

### ErrInstrBounds
integer code that defines an out of bounds error. Value of `-1`.

### ErrNone
integer code that defines execution was successful. Value of `0`.

### ErrBadPtr
integer code that defines either a `NULL` or invalid pointer dereference attempt. Value of `1`.

### ErrMissingFunc
integer code that defines a missing/NULL bytecode function. Value of `2`.

### ErrMissingNative
integer code that defines a missing/unresolved/NULL native function. Value of `3`.

### ErrInvalidScript
integer code that defines an invalid script during loading. Value of `4`.

### ErrStackSize
integer code that defines an invalid stack size given. Value of `5`.

### ErrStackOver
integer code that defines a stack overflow. Value of `6`.


## struct Tagha_va_list

### Area
member of `union TaghaVal` that is given a pointer of `union TaghaVal` to an array of arguments.

### Args
member of `uint64_t`, stores the size of the array that is referenced by `Area`.


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
Deallocates a `struct TaghaModule` pointer's data, deallocates the pointer itself, and then sets the pointer to `NULL`.

### Parameters
* `module_ref` - reference to a `struct TaghaModule` pointer.

### Return Value
boolean value whether the deallocation was successful or not.


## tagha_module_from_file
```c
bool tagha_module_from_file(struct TaghaModule *module, const char filename[]);
```

### Description
builds a module, from an existing pointer, from a file.

### Parameters
* `module` - pointer to a `struct TaghaModule` instance.
* `filename` - filename string of the script to load.

### Return Value
True if building the module from the file was a success, false otherwise.


## tagha_module_from_buffer
```c
bool tagha_module_from_buffer(struct TaghaModule *module, uint8_t buffer[]);
```

### Description
builds a module, from an existing pointer, from an existing data buffer.

### Parameters
* `module` - pointer to a `struct TaghaModule` instance.
* `buffer` - pointer to raw script data.

### Return Value
True if building the module from buffer was a success, false otherwise.


## tagha_module_del
```c
bool tagha_module_del(struct TaghaModule *module);
```

### Description
Deallocates a `struct TaghaModule` pointer's data and zeroes the data. The module pointer itself is NOT freed.

### Parameters
* `module` - pointer to a `struct TaghaModule` instance.

### Return Value
boolean value whether the deallocation was successful or not.


## tagha_module_print_vm_state
```c
void tagha_module_print_vm_state(const struct TaghaModule *module);
```

### Description
Prints the registers and condition flag of a Tagha module instance.

### Parameters
* `module` - pointer to a `struct TaghaModule` instance.

### Return Value
None.


## tagha_module_get_error
```c
const char *tagha_module_get_error(const struct TaghaModule *module);
```

### Description
Returns a constant string of an error message to check what error had occurred. When an error or exception occurs in the VM portion of a module, a return value of `-1` is returned and the module's error field is set.

### Parameters
* `module` - pointer to a `struct TaghaModule` instance.

### Return Value
constant C string (const char *) error message. Returns `NULL` if the module instance was `NULL`.


## tagha_module_register_natives
```c
bool tagha_module_register_natives(struct TaghaModule *module, const struct TaghaNative natives[]);
```

### Description
Registers the native C functions to a module for data communication between C code and the script's bytecode.

### Parameters
* `module` - pointer to a `struct TaghaModule` instance.
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
* `module` - pointer to a `struct TaghaModule` instance.
* `varname` - string name of the global ptr variable to register.
* `ptr` - ptr value to register.

### Return Value
true or false if the operation was successful or not.


## tagha_module_get_globalvar_by_name
```c
void *tagha_module_get_globalvar_by_name(struct TaghaModule *module, const char name[]);
```

### Description
Returns a pointer to a script-defined global variable.
If the global variable is defined as a pointer in the script, then the returning pointer will be a pointer to that pointer.

### Parameters
* `module` - pointer to a `struct TaghaModule` instance.
* `name` - string name of the global variable to retrieve.

### Return Value
pointer to the global variable, `NULL` if the variable doesn't exist, the module doesn't have script data/memory, or module instance was `NULL`.


## tagha_module_call
```c
int32_t tagha_module_call(struct TaghaModule *module, const char funcname[], size_t args, union TaghaVal params[], union TaghaVal *return_val);
```

### Description
Manually calls a script function from C by name.

### Parameters
* `module` - pointer to a `struct TaghaModule` instance.
* `name` - name of script function to invoke.
* `args` - amount of arguments to pass.
* `params` - function params to be passed, as an array of `union TaghaVal`.
* `return_val` - pointer to `union TaghaVal` for use as a return value buffer.

### Return Value
returns a status `int32_t` value, returns `-1` if an error occurred. Use `return_val` to get a proper return value.


## tagha_module_invoke
```c
int32_t tagha_module_invoke(struct TaghaModule *module, int64_t func_index, size_t args, union TaghaVal params[], union TaghaVal *return_val);
```

### Description
Manually calls a script function from C by function index. Designed to be used for natives that take a function pointer from bytecode.

### Parameters
* `module` - pointer to a `struct TaghaModule` instance.
* `func_index` - index of script function to invoke.
* `args` - amount of arguments to pass.
* `params` - function params to be passed, as an array of `union TaghaVal`.
* `return_val` - pointer to `union TaghaVal` for use as a return value buffer.

### Return Value
returns a status `int32_t` value, returns `-1` if an error occurred. Use `return_val` to get a proper return value.


## tagha_module_run
```c
int32_t tagha_module_run(struct TaghaModule *module, int32_t argc, char *argv[]);
```

### Description
Executes a script by calling its main function.

### Parameters
* `module` - pointer to a `struct TaghaModule` instance.
* `argc` - amount of strings given.
* `argv` - array of C string ptrs to give to the script, the C string MUST be modifiable as per the C language standard.

### Return Value
returns a status `int32_t` value, returns `-1` if an error occurred.


## tagha_module_throw_error
```c
void tagha_module_throw_error(struct TaghaModule *module, int32_t err);
```

### Description
Allows a developer to manually throw a VM runtime exception. Only use within a native C or C++ function call and that the script needs to stop running.

### Parameters
* `module` - pointer to a `struct TaghaModule` instance.
* `err` - value that's higher or lower than 0, can be either a user defined error or an `enum TaghaErrCode` value.

### Return Value
None.


## tagha_module_force_safemode
```c
void tagha_module_force_safemode(struct TaghaModule *module);
```

### Description
Forces a module to run in safemode.

### Parameters
* `module` - pointer to a `struct TaghaModule` instance.

### Return Value
None.

