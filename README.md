# Tagha Virtual Machine
TaghaVM is a minimal yet complex register-based virtual machine && runtime environment designed as an alternative to a C dynamic loading plugin system as well as giving binary portability to C code!

Why? (See the Wiki for the full explanations)
Four reasons:
* 1. C interpreters exist but, in my opinion, they're not self-contained nor minimal with a focus on speed like Tagha is.
* 2. I wanted to learn how to make a virtual machine.
* 3. I wanted a way to allow C to be portable at the binary level so that C or C++ plugins could be shared without recompiliation.
* 4. Create a minimal and easy to use library for others to use in their projects.

# How to Build Tagha
Tagha's repo contains many build scripts. If you simply want a working static library and not have to fuss about, then edit or run `build_libtagha_static.sh`. You might need to edit it if you don't want a clang version or you wish to change the clang version.

Once you run the script, `libtagha.a` should be generated. Now you can do `#include "tagha.h"` and link `libtagha.a` to your application and you've successfully embedded Tagha.

To know how to effectively use the Tagha API, please read the embedding tutorial in the [Tagha Wiki](https://github.com/assyrianic/Tagha-Virtual-Machine/wiki/Embedding-Tagha-to-your-Application!-(C)). A [C++ tutorial](https://github.com/assyrianic/Tagha-Virtual-Machine/wiki/Embedding-Tagha-to-your-Application!-(C-Plus-Plus)) is also available if needed!

# How to Build Tagha TASM Assembler
The TASM Assembler has a single software dependency by using my [C Data Structure Collection](https://github.com/assyrianic/C-Data-Structure-Collection) to accomodate data structures like the symbol tables, etc.

* Build the data structure collection static library by running the build script
* copy the static library to the directory of the TASM Assembler source files.
* run the `build_tasm.sh` script

# How to create TBC Scripts with TASM.
* Once you've created a tasm script, run `./tasm 'my_tasm_source.tasm'
* if there's no errors reported, a `.tbc` file with the same name as your tasm script should be generated!

For more information, please check out the [Tagha Wiki](https://github.com/assyrianic/Tagha-Virtual-Machine/wiki).
