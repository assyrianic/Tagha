# Tagha VM

## Introduction

**TaghaVM** is a minimal, fast, memory-safe, self-contained register-based virtual machine && runtime environment designed as an alternative to a C dynamic loading plugin systems with intent to giving binary portability to C code!

### Rationale:

You might be thinking, *why not just pick a scripting language*? You're correct. One could simply choose a scripting language and I even advise using a scripting language **but** not all developers want to use a scripting language nor require one and they could have various reasons such as performance, syntax, maybe bloated runtimes.

Besides a scripting system, the other option would be to use a native shared library plugin system. What's the problem with shared libraries? They're not binary portable unless you make a build system for every platform you plan to ship your software for and hand out new binaries for every patch and update.


The Rationale for Tagha is...

+ 1. give C (and by extension C++) binary portability.
+ 2. be fast.
+ 3. be open-source.
+ 4. have a runtime environment without dependencies (beyond libc of course)
+ 5. be portable and embeddable for any program and platform.
+ 6. be small and minimal.
+ 7. be memory safe.


### Features

* self-contained.
* has its own, open source implementation of libc.
* register-based virtual machine with 3 different addressing modes to tackle any kind of operation.
* 22 **general purpose registers** + 3 reserved-use (stack pointers and instruction pointer) registers.
* floats and doubles are supported (can be compiled without).
* uses computed gotos (the ones that use a void\*) which is 20%-25% faster than a switch {[citation](http://eli.thegreenplace.net/2012/07/12/computed-goto-for-efficient-dispatch-tables)}.
* Tagha is "64-bit" as the registers and memory addresses are 64-bit. (will run slower on 32-bit systems/OSes).
* Embeddable.
* scripts can call host-defined functions (Native Interface).
* host can give arguments and call script functions and retrieve return values.
* host can bind its own global variables to script-side global variables by name (the script-side global variable must be a pointer).
* integer & float arithmetic, (un)conditional jumps, comparison operations, and stack and memory manipulations.
* function call and return opcodes automatically execute function prologues and epilogues.
* little-endian format (only).
* small. The runtime environment static library is only <50kb.
* Tagha is not natively threaded, this is by design, this is so any developer can thread Tagha in anyway they wish whether by having a single VM instance run multiple scripts in a multi-threaded way OR use an array of Tagha VM instances each running their own scripts in a threaded manner.
* Speed, tagha is very fast for a virtual machine that does not use a JIT.
* Memory safe, tagha can sandbox scripts that have safemode enabled.


## Usage

```c
#include <stdio.h>
#include <stdlib.h>
#include "tagha.h"

int main(int argc, char **argv)
{
	struct Tagha vm;

	// assume "LoadScriptFromFile" is a real function.
	Tagha\_Init(&vm, LoadScriptFromFile("my\_tbc\_script.tbc"));

	// Execute our script!
	Tagha_RunScript(&vm, 0, NULL);
}
```

## Contributing

To submit a patch, first file an issue and/or present a pull request.

## Help

If you need help or have any question, you can reach on the #tagha channel through discord (https://discord.gg/2NKFgPS)
Simply drop a message or your question and you'll be reached in no time!

## Installation

### Requirements

C99 compiler and libc implementation with stdlib.h, stdio.h, and stddef.h.

### Installation

To embed tagha into your application, you must first build tagha as either a static or shared library.
You can build a static library of tagha by executing one of the build shell scripts that's part of the tagha repo.

Once you've built tagha as a library, include "tagha.h" into your C or C++ application.

If you need help in embedding, check out the [Tagha Wiki](https://github.com/assyrianic/Tagha-Virtual-Machine/wiki/Embedding-Tagha-to-your-Application!-(C)). A [C++ tutorial](https://github.com/assyrianic/Tagha-Virtual-Machine/wiki/Embedding-Tagha-to-your-Application!-(C-Plus-Plus)) is also available if needed!


To compile `.tasm` scripts to `.tbc` executables, you'll need to build the Tagha Assembler.

The TASM Assembler has a single software dependency by using my [C Data Structure Collection](https://github.com/assyrianic/C-Data-Structure-Collection) to accomodate data structures like the symbol tables, etc.

* run the `build_tagha_toolchain.sh` script which will build the C data structure collection library and link it to a build of the Tagha Assembler.
* you should have an executable called `tasm`

### How to create TBC Scripts with TASM.
* Once you've created a tasm script, run `./tasm 'my_tasm_source.tasm'`
* if there's no errors reported, a `.tbc` file with the same filename as the tasm script will be generated.
* to use the TBC scripts, embed Tagha into your C or C++ application and direct your application to the file directly or a special directory for tbc scripts.

### Configuration

Tagha can be configured in consideration with floating point support.
If you wish to completely remove floating point support, go into "tagha.h" and comment out these two macros:
```c
#define __TAGHA_FLOAT32_DEFINED // allow tagha to use 32-bit floats
#define __TAGHA_FLOAT64_DEFINED // allow tagha to use 64-bit floats
```

If you require one of these specific float types, you can comment out the other.
If you need to re-enable floating point support for all types, simply uncomment the defines.

Changing the header file requires that you recompile tagha for the change to take effect.

## Credits

Khanno Hanna - main developer of Tagha.
Id Software - developers of Quake 3 Virtual Machine, which inspired Tagha's creation.

## Contact

I can be contacted through the discord link above.

## License

This project is licensed under GPL License v3.
