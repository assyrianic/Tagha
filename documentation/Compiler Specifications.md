# Introduction
The Tagha Runtime Environment is not meant to be tied down to a single compiler. Similar to how C and C++ have many compilers for them. Tagha is only meant to be a minimal runtime that has the bare bones environment necessary to run bytecode compiled from C code.


# Tagha Required C Specifications

* `double` and `long double` should always be 8 bytes in data size.

* `float` and `double` are as defined by the IEEE.

* `long`, `size_t`, and `long long` should be 8 bytes in size.

* all binary math operations assume both operands are the same size.

* `argc` and `argv` are implementation-defined for Tagha, so `main` could have any type of parameters as necessary to script devs.

* if `argv` contains pointers that are not owned by the script's memory allocator, then **the VM _will_ throw a runtime exception if the bytecode dereferences them**. Keep this in mind when passing objects to the scripts. A workaround for this is to allocate from the script's own runtime heap, copy data to it, and pass that to `main`.