# Tagha VM

![Tagha Logo by Noxabellus and modified by Khanno](https://images-wixmp-ed30a86b8c4ca887773594c2.wixmp.com/intermediary/f/efc8ece3-f4a3-4477-8ebb-cb9595fb9e58/dcx0vh7-7c8a2027-14e9-48a9-a0e9-638260f44433.png/v1/fill/w_400,h_462,strp/tagha_virtual_machine_logo_by_assyrianic_dcx0vh7-fullview.png)

## Introduction

**Tagha** is a minimal, fast, memory-safe, self-contained register-based virtual machine && runtime environment designed as an alternative to a C dynamic loading plugin systems with intent to giving binary portability to C code!

### Rationale:

+ 1. give C (and by extension C++) binary portability.
+ 2. be fast.
+ 3. be open-source.
+ 4. have a runtime environment without dependencies (beyond libc of course)
+ 5. be portable and embeddable for any program and platform.
+ 6. be small and minimal.
+ 7. be memory safe.


### Features

* Self-contained, meaning that everything the code base needs is packaged together and there's no dependencies except for C standard lib obviously.
* Has its own, open source implementation of libc.
* Register-based virtual machine that handles immediate values, register, and memory operations.
* Supports 1, 2, 4, and 8 byte operations.
* 22 **general purpose registers** + 3 reserved-use (stack pointers and instruction pointer) registers.
* Floats and doubles are supported (can be compiled without).
* Uses computed gotos (the ones that use a void\*) which is 20%-25% faster than a switch {[citation](http://eli.thegreenplace.net/2012/07/12/computed-goto-for-efficient-dispatch-tables)}.
* Tagha is 64-bit as the registers and memory addresses are 64-bit. (will run slower on 32-bit OSes and significantly slower on 32-bit systems.)
* Embeddable.
* Tagha allocates what is needed and does no garbage collecting.
* Scripts can call host-defined functions (Native Interface).
* Host can give arguments and call script functions and retrieve return values.
* Host can bind its own global variables to script-side global variables by name (the script-side global variable must be a pointer).
* Integer & float arithmetic, (un)conditional jumps, comparison operations, and stack and memory manipulations.
* Function call and return opcodes automatically execute function prologues and epilogues.
* Little-endian format (only).
* Small. The runtime environment static library is <100kb.
* Tagha's entire code is **less than 1k lines of code**!
* Speed, tagha is very fast for a virtual machine that does not use a JIT.
* Memory safe, tagha can sandbox scripts that have safemode enabled.


## Usage

```c
#include <stdio.h>
#include <stdlib.h>
#include "tagha.h"

int main(int argc, char *argv[])
{
	/* make our script instance. */
	struct TaghaModule *script = tagha_module_new_from_file("my_tbc_script.tbc");
	
	/* call 'main' with no command-line arguments. */
	tagha_module_run(script, 0, NULL);
	
	/* clean up script, pointer will be set to NULL. */
	tagha_module_free(&script);
}
```

## Contributing

To submit a patch, first file an issue and/or present a pull request.

## Help

If you need help or have any question, you can reach on the `#taghavm` channel through discord (https://discord.gg/2NKFgPS)
Simply drop a message or your question and you'll be reached in no time!

## Installation

### Requirements

C99 compiler and libc implementation with stdlib.h, stdio.h, and stddef.h. Everything the code base requires is there.

### Installation

To embed tagha into your application, you must first build tagha as either a static or shared library.

create a directory with the repo file and run `make tagha` which will create a static AND shared library of tagha.

Once you've built tagha as a library, include "tagha.h" into your C or C++ application.

If you need help in embedding, check out the C tutorial on embedding in the documentation folder.


To compile `.tasm` scripts to `.tbc` executables, you'll need to build the Tagha Assembler! Run `make tagha_asm` which will build the tagha assembler executable named `tagha_asm`.

### How to create TBC Scripts with Tagha ASM.

Scripts can be created by taking a tasm script and supplying it as a command-line argument to tagha assembler. Here's an example:

```sh 
./tasm 'script.tasm'
```

If there are no errors reported, a `.tbc` file with the same filename as the tasm script will be produced. Now that you have a usable tbc script, you can run it from your C or C++ application.

To use the tbc scripts, embed Tagha into your C or C++ application and direct your application to the file directly or a special directory for tbc scripts.

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

### Testing
If you wish to build and test the tagha code base, compile `test_hostapp.c` with either the shared or static tagha library, compile the tagha assembler and compile the testing .tasm scripts in `tagha_testcode`, and then run the .tbc scripts.

## Credits

* Khanno Hanna - main developer of Tagha.
* Id Software - developers of Quake 3 Virtual Machine, which inspired Tagha's creation.
* Noxabellus - helping out with development, design, logo designer, & accidentally giving me ideas.
* Animu/Megumazing - helping out with development & design.

## Contact

I can be contacted through the discord link (https://discord.gg/2NKFgPS).

## License

This project is licensed under MIT License.


## FAQ
* Q: _**why not just pick a scripting language?**_
* A: You're right. Any developer could simply choose an existing scripting language and its implementation, but not all developers want to use a scripting language and they could have various reasons like performance, syntax, maybe the runtime is too bloated. Secondly, not all developers might know the language or are comfortable with it. Perhaps for the sake of consistency with the code base, they want the entire code to be in one language. After all, to be able to utilize the scripting language, you'd need to learn it as well as learning the API of the host app.

* Q: _**Why implement TaghaVM in C and not C++?**_
* A: The design choices I gave to TaghaVM was to be minimal, fast, and with little-to-no dependencies except for a few C standard library functions. To achieve this, I needed to use C which allowed me to manipulate memory as fast and seamless as possible. I'm aware C++ allows me to manipulate memory but it's not without trouble.

* Q: _**Can TaghaVM be used to implement any language?**_
* A: Yes but not perfectly. If we take Lua's example, Lua values are entirely pointers to a tagged union type in which the types are either a float value, string, or table/hashmap. Since most of TaghaVMs registers are general-purpose (can hold/use memory locations), they can hold/use the Lua values themselves but Lua's high level opcodes would have to be broken up into lower level operations since Tagha is a low-level VM that operates upon the byte sizes of the data, regardless of their actual types. This may possibly result in worse performance than just running Lua's code on its respective VM.

* Q: _**Will you implement a JIT in the future?**_
* A: Maybe.
