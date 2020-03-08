# Introduction
The Tagha Runtime Environment, though I wish to have an official compiler for it, is not meant to be tied down to a single compiler. Similar to how C and C++ have many compilers for them. Tagha is only meant to be a minimal runtime that has the bare bones environment necessary to run C programs.

To be a worthy C runtime environment, giving Tagha runtime speed is a hard requirement and has the following specifications needed to accommodate it.

# Tagha Required C Specifications

* `double` and `long double` should always be 8 bytes in data size.

* `float` and `double` as defined by the IEEE.

* Tagha's push and pop operations manipulates the stack by 8 bytes.

* `long`, `size_t`, and `long long` should be 8 bytes in size.

* all binary math operations assume both operands are the same size.

* Bytecode & Native Function Calling Convention is **only** through registers, calling convention uses registers `rsemkath` to `rdadeh` aka `rarg0` to `rarg15`. `rsemkath` will contain the 1st argument up to `rdadeh` which will contain the 16th argument.
	* *If there are more than 16 params, optimize into a va_list*.

* for C++ and other OOP-based languages, the `this` pointer must be placed in `rsemkath` aka `rarg0`.

* Return values always go in the `ralaf` (`r0`) register. This includes return values for natives. If the return data is larger than 64-bits, then optimize the function and/or native to use a hidden pointer parameter.

* `argc` and `argv` are implementation-defined for Tagha, so `main` could have any type of parameters as necessary to script devs.

* if `argv` contains pointers that are not owned by the script, then **the VM _will_ throw a runtime exception if the bytecode dereferences them**. Keep this in mind when passing objects to the scripts. A workaround for this is to allocate from the script's own runtime heap, copy data to it, and pass that to `main`.


# Tagha Script File Format

* ------------------------------ start of header ------------------------------
* 4 bytes: magic verifier ==> TAGHA_MAGIC_VERIFIER
* 4 bytes: stack size, stack size needed for the code.
* 4 bytes: mem region size.
* 4 bytes: flags
* ------------------------------ end of header ------------------------------
* .functions table
* 4 bytes: amount of funcs
* n bytes: func table
*     4 bytes: entry size.
*     4 bytes: 0 if bytecode func, 1 if it's a native, other flags.
*     4 bytes: string size + '\0' of func string
*     4 bytes: instr len, 8 if native.
*     n bytes: func string
*     if bytecode func: n bytes - instructions
* 
* .globalvars table
* 4 bytes: amount of global vars
* n bytes: global vars table
*     4 bytes: entry size.
*     4 bytes: flags
*     4 bytes: string size + '\0' of global var string
*     4 bytes: byte size, 8 if ptr.
*     n bytes: global var string
*     n bytes: data. All 0 if not initialized in script code.
* 
* .mem region - taken control by the memory pool as both a stack and heap.
