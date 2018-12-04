# Introduction
Tagha itself, though I wish to have an official compiler for it, is not meant to be tied down to a single compiler. Similar to how C and C++ have many compilers for them. Tagha is only meant to be a minimal runtime that has the bare bones environment necessary to run C code and to use such C code in an abstracted way.

To be a worthy abstracted C runtime environment, giving Tagha runtime speed is a hard requirement and has the following specifications needed to accommodate it.

# Tagha specification

* `double` and `long double` should always be 8 bytes in data size.
* `float` as defined by the IEEE.
* Tagha's push and pop operations manipulates the stack by 8 bytes.
* Local function data should be aligned by a 16 byte boundary.
* `long`, `size_t`, and `long long` should be 8 bytes in data size.
* all binary math operations assume both operands are the same size.
* Bytecode Function calling convention is through the registers first and then stack, calling convention uses registers `rsemkath` to `rtaw` for the first 8 arguments. `semkath` will contain the 1st argument up to `rtaw` which will contain the 8th argument, remaining params will be dumped to the stack.
* for C++, the `this` pointer will be placed in `rsemkath`.
* Function return values always return in the `ralaf` register (`ralaf` is the accumulator though it's general purpose). This includes return values for natives. If the return data is larger than 64-bits, then optimize the function to take a hidden pointer obviously.
* For consistency with natives, all native functions returning a struct scalar must be optimized to pass a hidden pointer instead if the struct scalar is not small enough to fit in a register.
* In order to accommodate for natives, Tagha requires, if possible, that explicitly `extern`'d functions must emit native-oriented syscall opcodes.
Example:
```c
extern int puts(const char *);
```

* Calling convention for exported C natives requires that if a native takes 8 or less parameters, then registers `semkath` to `taw` will contain the first 8 arguments.
* If the exported native requires more than 8 parameters, then ALL parameters must be dumped to the stack with the arguments pushed from right to left (cdecl convention).
* `argc` and `argv` are implemented in scripts but `env` variable is not implemented (I see no reason to implement as of currently).

# Tagha Script File Format

 * ------------------------------ start of header ------------------------------
 * 2 bytes: magic verifier ==> 0xC0DE
 * 4 bytes: stack size, stack size needed for the code
 * 1 byte: flags
 * ------------------------------ end of header ------------------------------
 * .functions table
 * 4 bytes: amount of funcs
 * n bytes: func table
 *     1 byte: 0 if bytecode func, 1 if it's a native, other flags.
 *     4 bytes: string size + '\0' of func string
 *     4 bytes: instr len, 8 if native.
 *     n bytes: func string
 *     if bytecode func: n bytes - instructions
 *     else: 8 bytes: native address (0 at first, will be filled in during runtime)
 * 
 * .globalvars table
 * 4 bytes: amount of global vars
 * n bytes: global vars table
 *     1 byte: flags
 *     4 bytes: string size + '\0' of global var string
 *     4 bytes: byte size, 8 if ptr.
 *     n bytes: global var string
 *     if bytecode var: n bytes: data. All 0 if not initialized in script code.
 *     else: 8 bytes: var address (0 at first, filled in during runtime)
