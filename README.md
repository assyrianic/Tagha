# Tagha

![Tagha Logo by Noxabellus and modified by Khanno](https://images-wixmp-ed30a86b8c4ca887773594c2.wixmp.com/intermediary/f/efc8ece3-f4a3-4477-8ebb-cb9595fb9e58/dcx0vh7-7c8a2027-14e9-48a9-a0e9-638260f44433.png/v1/fill/w_400,h_462,strp/tagha_virtual_machine_logo_by_assyrianic_dcx0vh7-fullview.png)


[![star this repo](http://githubbadges.com/star.svg?user=assyrianic&repo=tagha&style=plastic)](https://github.com/assyrianic/tagha)
[![forks repo](http://githubbadges.com/fork.svg?user=assyrianic&repo=tagha&style=plastic)](https://github.com/assyrianic/tagha)

## Introduction

**Tagha** is a minimal, fast, memory-safe, self-contained, register-based virtual machine runtime environment designed to execute C code as bytecode scripts.

### Rationale:

+ 1. give C binary portability.
+ 2. be fast.
+ 3. be open-source.
+ 4. have a runtime environment without dependencies (beyond libc of course)
+ 5. be portable and embeddable for any program and platform.
+ 6. be small and minimal. ![file size](https://img.shields.io/github/repo-size/assyrianic/tagha.svg?style=plastic)
+ 7. be memory safe.
+ 8. be easy for compilers to target.


### Features

* Tagha is 64-bit as registers & memory addresses are 64-bit. (will run significantly slower (~25%) on 32-bit OS's and drastically slower (~4x-5x) on 32-bit systems.)
* Self-contained, everything the codebase needs is packaged together and there's no dependencies except for some C standard library API.
* Tagha codebase has its own, open source implementation of libc for scripts to use (INCOMPLETE).
* Register-based virtual machine that handles immediate values, register, and memory operations.
* Supports 1, 2, 4, and 8 byte operations.
* Utilize up to 256 **general purpose registers** _per_ function!.
* Floats and doubles are supported (can be compiled without and also can be compiled with only one or the other).
* Uses computed gotos (ones that use a `void*`) which is 20%-25% faster than using a switch+loop construct {[citation](http://eli.thegreenplace.net/2012/07/12/computed-goto-for-efficient-dispatch-tables)}.
* Embeddable and easy to embed.
* Tagha allocates only what is needed and performs no garbage collection during runtime.
* Scripts can call host-defined functions (Native Interface).
* Host Applications can call script functions and retrieve return values from the script function invocation.
* Scripts (by exporting Tagha's own API) can manually load other scripts as dynamic libraries.
* VM runs as little-endian format (only).
* Small. The entire runtime as a static library is less than 50kb.
* Tagha's entire VM code is **about 1k lines of code**!
* Speed, Tagha is very fast for a virtual machine that does not use a JIT -- Check out the documentation for profiling results.
* Memory safe, Tagha sandboxes scripts by blocking memory operations that were not allocated by the script's own memory allocator.
* Tagha Assembler - transforms human readable bytecode into binary bytecode.
* Tagha Bytecode Builder - header-only encoder functions to help with lower level bytecode creation.
* Tagha Module Builder - header-only library that helps create a full-fledged Tagha Module script.


## Usage

```c
#include "tagha.h"

int main(int argc, char *argv[])
{
	/// make our script instance.
	struct TaghaModule *script = tagha_module_new_from_file("my_tbc_script.tbc");
	
	/// call 'main' with no command-line arguments.
	const int32_t result = tagha_module_run(script, 0, NULL);
	
	/// do something with 'result'.
	
	/// clean up script. sets 'script' to NULL.
	tagha_module_free(&script);
}
```

## Contributing

To submit a patch, file an issue and/or hit up a pull request.

## Help

If you need help or have any question, simply file an issue with **\[HELP\]** in the title.

## Installation

### Requirements

C99 compliant compiler and libc implementation with the following headers available:
* stdlib.h
* stdio.h
* stdbool.h
* inttypes.h
* string.h
* stdarg.h
* limits.h
* float.h
It is preferable that you compile using GCC or Clang/LLVM.

### Installation

To embed Tagha into your application, you must build Tagha as either a static or shared library.

Create a directory with the repo, change directory to the `tagha` folder and run `make` which will create a static library of Tagha. To create a shared library version, run `make shared`. To clean up the object files, run `make clean`.

If for any reason, you need to create a debug version, run `make debug` to create a debug version of the static library and `make debug_shared` for a debug version of the shared library.

Once you've built Tagha as a library, include "tagha.h" into your C or C++ application.

If you need help in embedding, check out the C tutorial on embedding in the documentation.


To compile `.tasm` scripts to `.tbc` executables, you'll need to build the Tagha Assembler! Change directory into the `assembler` directory and run `make` which will build the Tagha assembler executable (named `tagha_assembler`).

### How to create TBC Scripts with Tagha ASM.

Scripts can be created by supplying a `.tasm` file as a command-line argument to the tagha assembler. Here's an example:

```sh
./tagha_assembler 'script.tasm'
```

If there are no errors reported, a `.tbc` binary, with the same filename as the script, will be produced. Now that you have a usable tbc script, you can run it from your C or C++ application.

To execute tbc scripts, embed Tagha into your C or C++ application (or build the example host application) and direct your application to the file directly or a special directory just for tbc scripts.


### Using the Tagha Disassembler.
Also as part of the Tagha Toolchain, there includes a disassembler which functions exactly the same as the Tagha Assembler. The only difference between the assembler and the disassembler is you feed a `.tbc` script to the disassembler and it spits out a `.tasm` file.

The `.tasm` file can be re-assembled by the assembler but requires fixing jump labels if any.


### Configuration

Tagha can be configured in consideration with floating point support.
If you wish to completely remove floating point support, go into "tagha.h" and comment out these two macros:
```c
#define TAGHA_FLOAT32_DEFINED    /// allow tagha to use 32-bit floats
#define TAGHA_FLOAT64_DEFINED    /// allow tagha to use 64-bit floats
```

If you require one of these specific float types, you can comment out the other.
If you need to re-enable floating point support for all types, simply uncomment the defines.

Note: Changing the header file requires that you recompile the Tagha library for the changes to take effect on the runtime.

### Testing
If you wish to build and test the Tagha code base, compile `test_driver.c` with either the shared or static Tagha library, link with Tagha's libc implementation, compile the Tagha assembler and compile the testing .tasm scripts in the `test_asm` folder, and run the generated .tbc scripts.

## Credits

* Khanno Hanna - main developer of Tagha.
* Id Software - developers of Quake 3 Virtual Machine, which inspired Tagha's creation.
* Noxabellus - helping out with development, design, logo designer, & accidentally giving me ideas.
* Animu/Megumazing - helping out with development & design.

## License
[![License](https://img.shields.io/github/license/assyrianic/tagha.svg?label=License&style=plastic)](https://github.com/assyrianic/tagha)

This project is licensed under MIT License.


## FAQ
* Q: _**Why not just pick a scripting language?**_
* A: You're right. Any developer could simply choose an existing scripting language and its implementation, but not all developers want to use a scripting language and they could have various reasons like performance, syntax, maybe the runtime is too bloated. Secondly, not all developers might know the language, are comfortable with it, or don't preffer it. Perhaps for the sake of consistency with the code base, they want the entire code to be in one language. After all, to be able to utilize the scripting language, you'd need to learn it as well as learning the exposed API of the host app. My point is, there's a myriad of reasons to choose (or not to choose) a scripting language.

* Q: _**Ok, then why not use a dynamic linking/shared library module/plugin system?**_
* A: Same answer as before, any developer could choose such a system over a scripting language as well. The benefits of this is having the native speed of the application's implementation language while still being extendable/modifiable. However the drawbacks to a shared library plugin system is that you need to build the plugins for every architecture and OS for the shared plugins to run properly. On Windows OS' this isn't as big a deal but Linux ABIs also use the OS as a factor. Thus, having portable programs isn't easy to implement with using a plugin system without taking ABI in account.

* Q: _**Then why use Tagha at all?**_
* A: You should use Tagha if you want a bytecode runtime environment that is fast, minimal, very small memory footprint, completely self-contained within a single (static or shared) library, open source, and permissive in licensing with absolutely no strings attached.

* Q: _**Why implement Tagha in C and not C++?**_
* A: The design choices for Tagha was to be minimal, fast, and with little-to-no dependencies except for a few C standard library functions. To achieve this, I needed to use C which allowed me to manipulate memory as fast and seamless as possible. I'm aware C++ allows me to manipulate memory but it's not without trouble.

* Q: _**Can Tagha be used to implement any language?**_
* A: In theory yes; in practice, yes but not perfectly. If we take Lua's example, Lua values are entirely pointers to a tagged union type in which the types are either a float value, string, or table/hashmap. Tagha is designed as a runtime environment for C code that is compiled to bytecode, not Lua. Lua as a language can be supported but features like tables have to be translated into more lower level operations or implemented as natives. Since Tagha has the bare minimum features to be a C runtime, that can be adapted to other languages although it would require more effort.

* Q: _**Will you implement a JIT in the future?**_
* A: Maybe. I will likely not implement a JIT but I could make a compromise by adding JIT compiling support. If I were to seriously consider implementing a JIT, I'd likely use the MIR JIT Compiler since it's also in C and is planned as a standalone library, easy to use JIT compilation library.
