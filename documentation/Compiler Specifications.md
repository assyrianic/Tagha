# Introduction
The Tagha Runtime Environment, though I wish to have an official compiler for it, is not meant to be tied down to a single compiler. Similar to how C and C++ have many compilers for them. Tagha is only meant to be a minimal runtime that has the bare bones environment necessary to run C programs.

To be a worthy C runtime environment, giving Tagha runtime speed is a hard requirement and has the following specifications needed to accommodate it.

# Tagha Requirement Specification

* `double` and `long double` should always be 8 bytes in data size.

* `float` and `double` as defined by the IEEE.

* Tagha's push and pop operations manipulates the stack by 8 bytes.

* `long`, `size_t`, and `long long` should be 8 bytes in size.

* all binary math operations assume both operands are the same size.

* Bytecode & Native Function Calling Convention is **only** through registers, calling convention uses registers `rsemkath` to `rdadeh` aka `rarg0` to `rarg15`. `rsemkath` will contain the 1st argument up to `rdadeh` which will contain the 16th argument.
	* *If there are more than 16 params, optimize into a va_list*.

* for C++ and other OOP-based languages, the `this` pointer must be placed in `rsemkath` aka `rarg0`.

* Return values always go in the `ralaf` (`r0`) register. This includes return values for natives. If the return data is larger than 64-bits, then optimize the function and/or native to use a hidden pointer parameter.

* `argc` and `argv` are implemented in scripts but `env` variable is not implemented.

* `main` MAY be able to allowed to give whatever parameters the developers embedding tagha want to give to script devs. Just not pointers.


# Tagha Script File Format

 * ------------------------------ start of header ------------------------------
 * 2 bytes: magic verifier ==> 0xC0DE
 * 4 bytes: stack size, stack size needed for the code.
 * 4 bytes: mem region size.
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
 * 
 * .mem region - taken control by the memory pool as both a stack and heap.
