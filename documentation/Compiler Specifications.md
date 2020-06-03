# Introduction
The Tagha Runtime Environment, though I wish to have an official compiler for it, is not meant to be tied down to a single compiler. Similar to how C and C++ have many compilers for them. Tagha is only meant to be a minimal runtime that has the bare bones environment necessary to run C programs.

To be a worthy C runtime environment, giving Tagha runtime speed is a hard requirement and has the following specifications needed to accommodate it.

# Tagha Required C Specifications

* `double` and `long double` must be 8 bytes in data size.

* `float` and `double` are as defined by the IEEE.

* Tagha's push and pop operations manipulates the stack by 8 bytes.

* `long`, `size_t`, and `long long` should be 8 bytes in size.

* all binary math operations assume both operands are the same size. 

* Bytecode & Native Function Calling Convention is **only** through registers, calling convention uses registers `rsemkath` to `rdadeh` aka `rarg0` to `rarg15`. `rsemkath` will contain the 1st argument up to `rdadeh` which will contain the 16th argument.
	* *If there are more than 16 params, optimize into a va_list/array*.

* for C++ and other OOP-based languages, the `this` pointer must be placed in `rsemkath` aka `rarg0`.

* Return values always go in the `ralaf` (`r0`) register. This includes return values for natives. If the return data is larger than 64-bits, then optimize the function and/or native to use a hidden pointer parameter.
	* If the return value is to be used by other natives, it is permissable to use the array aliases of the union type used by Tagha's registers and also use multiple registers to store the same data. I.E using `ralaf` (`r0`) and `rbeth` (`r1`) to store four 4-byte floating point numbers.

* `argc` and `argv` are implementation-defined for Tagha, so `main` could have any type of parameters as necessary to script devs.

* if `argv` contains pointers that are not owned by the script's memory allocator, then **the VM _will_ throw a runtime exception if the bytecode dereferences them**. Keep this in mind when passing objects to the scripts. A workaround for this is to allocate from the script's own runtime heap, copy data to it, and pass that to `main`.
