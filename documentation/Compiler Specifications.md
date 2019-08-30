# Introduction
The Tagha Runtime Environment, though I wish to have an official compiler for it, is not meant to be tied down to a single compiler. Similar to how C and C++ have many compilers for them. Tagha is only meant to be a minimal runtime that has the bare bones environment necessary to run C programs.

To be a worthy C runtime environment, giving Tagha runtime speed is a hard requirement and has the following specifications needed to accommodate it.

# Tagha Requirement Specification

* `double` and `long double` should always be 8 bytes in data size.

* `float` and `double` as defined by the IEEE.

* Tagha's push and pop operations manipulates the stack by 8 bytes.

* `long`, `size_t`, and `long long` should be 8 bytes in size.

* all binary math operations assume both operands are the same size.

* Bytecode & Native Function calling convention is either through the registers or the stack, calling convention uses registers `semkath` to `taw` for the first 8 arguments. `semkath` will contain the 1st argument up to `rtaw` which will contain the 8th argument.
	* *If there are more than 8 params, ALL params must be pushed to the stack* (cdecl convention).

* for C++ and other OOP-based languages, the `this` pointer should be placed in `rsemkath`.

* Function return values always return in the `ralaf` register. This includes return values for natives. If the return data is larger than 64-bits, then optimize the function to take a hidden pointer parameter.

* For consistency with natives, all native functions returning a struct scalar must be optimized to pass a hidden pointer instead if the struct scalar is not small enough to fit in a register.

* `argc` and `argv` are implemented in scripts but `env` variable is not implemented (I see no reason to implement as of current).

* `main` MAY be able to allowed to give whatever parameters the developers embedding tagha want to give to script devs.

* To see how to give `main` custom parameters, please read the C tutorial in the documentation.


# Tagha Script File Format

 * ------------------------------ start of header ------------------------------
 * 4 bytes: magic verifier ==> 0xC0DE
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
