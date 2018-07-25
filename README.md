# Tagha Virtual Machine
TaghaVM is a minimal, fast, self-contained, and complex register-based virtual machine && runtime environment designed as an alternative to a C dynamic loading plugin system as well as giving binary portability to C code!

## Rationale:
You might be thinking, why not just pick a scripting language? You're correct. One could simply choose a scripting language but not all developers want to use a scripting language and they could have various reasons like performance, syntax, maybe the runtime is too bloated.

Another idea you might think is that one could use a C interpreter and embed that but so far, the only C interpreters (that I currently know of) are **CINT**, **PicoC**, **TCC [1]**, **Cling** and **Ch**:
- The problems with CINT is that it's old, clunky to use, outdated, and deprecated.
- PicoC is ok but its problem is that it uses old-school interpreting (just runs literal code) instead of compiling to bytecode which would allow it to execute faster.
- Cling (developed by CERN ROOT) is very good in what it does but it creates a massive dependency called LLVM and Clang. Secondly, not all the features of Cling are sometimes necessary in a project.
- The problem with Ch is that, though it's embeddable and updated, it's proprietary and it's unknown how it interprets code; as the usual problem with proprietary code, you don't know what code it could contain and there's no telling what security issues Ch could possibly have; not to mention that proprietary code shuts out enthusiastic individuals or groups from contributing to the software. **UPDATE**: I read on the Ch's developers website that Ch also interprets source code directly, doesn't use a bytecode machine! Specifically, Ch uses an AST-walking interpreter that's combined with a JIT Compiler **[2]**, that'll speed up things alot but reduces portability significantly. **[3]**
- **[1]** - Tiny C Compiler, it can compile and run scripts for testing but it can't be embedded the same way a scripting system can be.
- **[2]** - [citation from Ch dev website](https://www.softintegration.com/support/faq/embed.html#bytecode)
- **[3]** - Embeddable Ch is **NOT** free, you must get a price quote for it and purchase it.

The Rationale for Tagha is...
+ 1. give C (and by extension C++) binary portability.
+ 2. have the clear execution speed advantage bytecode interpreters have over more traditional interpreters.
+ 3. be open-source.
+ 4. have a runtime environment without having dependencies (beyond libc of course)
+ 5. be portable and embeddable for any program and platform.
+ 6. be small and minimal.

# Features
* self-contained.
* has its own implementation of libc specially for TaghaVM.
* register-based virtual machine with 3 different addressing modes to tackle any kind of operation.
* 22 **general purpose registers** + 3 reserved-use (stack pointers and instruction pointer) registers.
* floats and doubles are supported (can be compiled without).
* uses computed gotos (the ones that use a void\*) which is 20%-25% faster than a switch {[citation](http://eli.thegreenplace.net/2012/07/12/computed-goto-for-efficient-dispatch-tables)}.
* Tagha is "64-bit" as the registers and memory addresses are 64-bit. (will run slower on 32-bit systems/OSes)
* Embeddable.
* scripts can call host-defined functions (Native Interface).
* host can give arguments and call script functions and retrieve return values.
* host can bind its own global variables to script-side global variables by name (the script-side global variable must be a pointer).
* integer & float arithmetic, (un)conditional jumps, comparison operations, and stack and memory manipulations.
* function call and return opcodes automatically execute function prologues and epilogues.
* little-endian format (only).
* small. The runtime environment static library is only ~30kb.
* Tagha is not natively threaded, this is by design, this is so any developer can thread Tagha in anyway they wish whether by having a single VM instance run multiple scripts in a multi-threaded way OR use an array of Tagha VM instances each running their own scripts in a threaded manner.
* Speed, tagha is very fast for a virtual machine that does not use a JIT.

# How to Build Tagha
Tagha's repo contains many build scripts. If you simply want a working static library and not have to fuss about...
* edit and/or run `build_libtagha_static.sh`. You might need to edit it if you don't want a clang version or you wish to change the clang version.
* `libtagha.a` should be generated.
* do `#include "tagha.h"` in your C or C++ application.
* link `libtagha.a` to your application and you've successfully embedded Tagha.

To know how to effectively use the Tagha API, please read the embedding tutorial in the [Tagha Wiki](https://github.com/assyrianic/Tagha-Virtual-Machine/wiki/Embedding-Tagha-to-your-Application!-(C)). A [C++ tutorial](https://github.com/assyrianic/Tagha-Virtual-Machine/wiki/Embedding-Tagha-to-your-Application!-(C-Plus-Plus)) is also available if needed!

# How to Build Tagha TASM Assembler
The TASM Assembler has a single software dependency by using my [C Data Structure Collection](https://github.com/assyrianic/C-Data-Structure-Collection) to accomodate data structures like the symbol tables, etc.

* Build the data structure collection static library by running the build script `build_libCDSC_static.sh` for the C data structure collection.
* copy the static library `.a` file to the directory of the TASM Assembler source files.
* run the `build_tasm.sh` script
* you should have an executable called `tasm`

# How to create TBC Scripts with TASM.
* Once you've created a tasm script, run `./tasm 'my_tasm_source.tasm'`
* if there's no errors reported, a `.tbc` file with the same filename as your tasm script should be generated.
* to use the TBC scripts, embed Tagha into your C or C++ application and direct your application to the file directly or a special directory for tbc scripts.
* Tagha is not natively threaded, this is by design, you can thread Tagha in anyway you wish whether by having a single VM instance run multiple scripts in a multi-threaded, managed way OR have an array of Tagha VM instances each running their own scripts in a threaded manner.

For more information, please check out the [Tagha Wiki](https://github.com/assyrianic/Tagha-Virtual-Machine/wiki).
