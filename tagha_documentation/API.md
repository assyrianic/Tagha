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
pointer to 32-bit float value. exists only if *__TAGHA_FLOAT32_DEFINED* is defined.

### PtrDouble
pointer to 64-bit float value. exists only if *__TAGHA_FLOAT64_DEFINED* is defined.

### Ptr
pointer to void type value.

### PtrSelf
pointer to union TaghaVal value.


## struct NativeInfo

### Name
constant C string (pointer to const char) of the name of the native function.

### NativeCFunc
pointer to native C function (C++ interface has it's own implementation).
The native must have the signature: `void (*)(struct Tagha *vm, union TaghaVal *ret, size_t args, union TaghaVal params[]);`

# Functions/Methods

## Tagha_New
```c
struct Tagha *Tagha_New(void *script);
```

### Description
Allocates a `struct Tagha` pointer from heap and initializes it with a script pointer.

### Parameters
`script` - pointer to raw script data.

### Return Value
pointer to a newly allocated `struct Tagha` pointer, return `NULL` if an error occured or heap is exhausted.


## Tagha_NewNatives
```c
struct Tagha *Tagha_NewNatives(void *script, const struct NativeInfo natives[]);
```

### Description
Allocates a `struct Tagha` pointer from heap and initializes it with a script pointer and a constant array of natives for registration.

### Parameters
`script` - pointer to raw script data.
`natives` - constant array of natives to register.

### Return Value
pointer to a newly allocated `struct Tagha` pointer, return `NULL` if an error occured or heap is exhausted.


## Tagha_Free
```c
void Tagha_Free(struct Tagha **tagharef);
```

### Description
Deallocates an allocated `struct Tagha` pointer and sets the pointer to `NULL`. Does **NOT** deallocate the script memory initialized to it; you need to use `Tagha_GetRawScriptPtr` to free it.

### Parameters
`tagharef` - reference to a `struct Tagha` pointer.

### Return Value
None.


## Tagha_Init
```c
void Tagha_Init(struct Tagha *vm, void *script);
```

### Description
Initializes a script unto a pointer to `struct Tagha` and prepares the script data.

### Parameters
`vm` - pointer to a `struct Tagha` instance.
`script` - pointer to raw script data.

### Return Value
None.


## Tagha_InitNatives
```c
void Tagha_InitNatives(struct Tagha *vm, void *script, const struct NativeInfo natives[]);
```

### Description
Initializes a script unto a pointer to `struct Tagha` and prepares the script data, then registers the natives array.

### Parameters
`vm` - pointer to a `struct Tagha` instance.
`script` - pointer to raw script data.
`natives` - constant array of natives to register.

### Return Value
None.


## Tagha_PrintVMState
```c
void Tagha_PrintVMState(const struct Tagha *vm);
```

### Description
Prints the registers and condition flag of a Tagha VM instance.

### Parameters
`vm` - pointer to a `struct Tagha` instance.

### Return Value
None.


## Tagha_GetError
```c
const char *Tagha_GetError(const struct Tagha *vm);
```

### Description
Returns a constant string of an error message to check what error had occurred. When an error or exception occurs in the VM, a return value of `-1` is returned and the VMs error field is set.

### Parameters
`vm` - pointer to a `struct Tagha` instance.

### Return Value
const char C string error message. Returns `NULL` if the VM instance was `NULL`.


## Tagha_RegisterNatives
```c
bool Tagha_RegisterNatives(struct Tagha *vm, const struct NativeInfo natives[]);
```

### Description
Registers the native C functions to the VMs script for data communication between C code and the script's bytecode.

### Parameters
`vm` - pointer to a `struct Tagha` instance.
`natives` - array of C natives to register.

### Return Value
true or false if the operation was successful or not.


## Tagha_GetGlobalVarByName
```c
void *Tagha_GetGlobalVarByName(struct Tagha *vm, const char name[]);
```

### Description
Returns a pointer to a script-defined global variable.
If the global variable is defined a pointer, then the returning pointer will be a pointer to that pointer.

### Parameters
`vm` - pointer to a `struct Tagha` instance.
`name` - string name of the global variable to retrieve.

### Return Value
pointer to the global variable, `NULL` if the variable doesn't exist, the VM doesn't have script data/memory, or VM instance was `NULL`.


## Tagha_GetRawScriptPtr
```c
void *Tagha_GetRawScriptPtr(const struct Tagha *vm);
```

### Description
Returns a pointer to a VMs script memory.

### Parameters
`vm` - pointer to a `struct Tagha` instance.

### Return Value
pointer to the script datum, `NULL` if no script was initialized to it or VM instance was `NULL`.


## Tagha_CallFunc
```c
int32_t Tagha_CallFunc(struct Tagha *vm, const char name[], size_t args, union TaghaVal params[]);
```

### Description
Manually calls a script function from C.

### Parameters
`vm` - pointer to a `struct Tagha` instance.
`name` - name of script function to invoke.
`args` - amount of arguments to pass.
`params` - function params to be passed, as an array of `union TaghaVal`.

### Return Value
returns a status int32 value, returns `-1` if an error occurred. Use `Tagha_GetReturnValue` to get the proper return value as required from the function.


## Tagha_GetReturnValue
```c
union TaghaVal Tagha_GetReturnValue(const struct Tagha *vm);
```

### Description
retrieves the return value from a script bytecode function invocation as a type of `union TaghaVal` for easier type coercion.

### Parameters
`vm` - pointer to a `struct Tagha` instance.

### Return Value
value of type `union TaghaVal`, returns `0` as `union TaghaVal` if the VM instance is `NULL`.


## Tagha_RunScript
```c
int32_t Tagha_RunScript(struct Tagha *vm, int32_t argc, char *argv[]);
```

### Description
Executes a script by calling its main function.

### Parameters
`vm` - pointer to a `struct Tagha` instance.
`argc` - amount of strings given.
`argv` - array of C string ptrs to give to the script, the C string MUST be modifiable as per the C language standard.

### Return Value
returns a status int32 value, returns `-1` if an error occurred.


## Tagha_ThrowError
```c
void Tagha_ThrowError(int32_t err);
```

### Description
Allows a developer to manually throw a VM exception. This is especially needed if an error occurs within a native C or C++ function call and the script needs to stop for whatever reason.

### Parameters
`err` - value that's higher than 0, can be either a user defined error or a Tagha Error enum value.

### Return Value
None.


# C++ Interface

# Datatypes

## struct CNativeInfo

### Name
constant C-style string (pointer to const char) of the name of the native function.

### NativeFunc
pointer to native C++ function.
The native must have the signature: `void (*)(class CTagha *, union TaghaVal *, size_t, union TaghaVal []);`


## Class CTagha
Inherits from `struct Tagha` C interface.

## CTagha Methods


## CTagha::CTagha
```c++
CTagha::CTagha(void *script);
```

### Description
Constructor, functions similar to `Tagha_Init`.

### Parameters
`script` - pointer to raw script data.

### Return Value
None.


## CTagha::CTagha
```c++
CTagha::CTagha(void *script, const struct CNativeInfo natives[]);
```

### Description
Overloaded constructor, functions similar to `Tagha_InitNatives`.

### Parameters
`script` - pointer to raw script data.
`natives` - array of CNativeInfo values.

### Return Value
None.


## CTagha::RegisterNatives
```c++
bool CTagha::RegisterNatives(const struct CNativeInfo natives[]);
```

### Description
registers an array of C++ natives, similar to `Tagha_RegisterNatives`.

### Parameters
`natives` - array of CNativeInfo values.

### Return Value
true or false if operation was successful.


## CTagha::GetGlobalVarByName
```c++
void *CTagha::GetGlobalVarByName(const char name[]);
```

### Description
Same as `Tagha_GetGlobalVarByName` but in a C++ class setting.

### Parameters
`name` - string name of global variable to retrieve.

### Return Value
returns pointer to global variable. `NULL` if an issue occurred.


## CTagha::CallFunc
```c++
int32_t CTagha::CallFunc(const char name[], size_t args, union TaghaVal params[]);
```

### Description
Same as `Tagha_CallFunc` but in a C++ class setting.

### Parameters
`name` - name of script function to invoke.
`args` - amount of arguments to pass.
`params` - function params to be passed, as an array of `union TaghaVal`.

### Return Value
returns a status int32 value, returns `-1` if an error occurred. Use `CTagha::GetReturnValue` to get the proper return value as required from the function.


## CTagha::GetReturnValue
```c++
union TaghaVal CTagha::GetReturnValue();
```

### Description
Same as `Tagha_GetReturnValue` but in a C++ class setting.

### Parameters
None.

### Return Value
value of type `union TaghaVal`, returns `0` as `union TaghaVal` if issue occurred.


## CTagha::RunScript
```c++
int32_t CTagha::RunScript(int32_t argc, char *argv[]);
```

### Description
Same as `Tagha_RunScript` but in a C++ class setting.

### Parameters
`argc` - amount of strings given.
`argv` - array of C string ptrs to give to the script, the C string MUST be modifiable as per the C language standard.

### Return Value
returns a status int32 value, returns `-1` if an error occurred.


## CTagha::GetError
```c++
const char *CTagha::GetError();
```

### Description
Same as `Tagha_GetError` but in a C++ class setting.

### Parameters
None.

### Return Value
const char C string error message.


## CTagha::PrintVMState
```c++
void CTagha::PrintVMState();
```

### Description
Same as `Tagha_PrintVMState` but in a C++ class setting.

### Parameters
None.

### Return Value
None.


## CTagha::GetRawScriptPtr
```c++
void *CTagha::GetRawScriptPtr();
```

### Description
Same as `Tagha_GetRawScriptPtr` but in a C++ class setting.

### Parameters
None.

### Return Value
pointer to the script datum, `NULL` if no script was initialized to it.


## CTagha::ThrowError
```c++
void CTagha::ThrowError(int32_t err);
```

### Description
Same as `Tagha_ThrowError` but in a C++ class setting.

### Parameters
`err` - user defined error or using tagha error enum values.

### Return Value
None.
