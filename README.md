# Tagha Virtual Machine
"tagha" is Aramaic for "crown".
Tagha is a minimal yet complex register-based virtual machine and scripting engine environment, written in C, designed to run C compiled into tagha scripts. Why is it named "crown"? Because C is king in the programming world :P

[See the Wiki for API references, Opcodes reference, Tutorials, and more](https://github.com/assyrianic/Tagha-Virtual-Machine/wiki)

## (Current) Features:
* Scripts can call Host functions (called natives).
* Host can call script functions, with pushing and retrieving data!
* Host can retrieve script global variables.
* Host can bind its own global variables as a pointer to a script global variable.
* Very small memory usage: executable itself is 50-70kb.
* No dependencies except libc!
* easy to use, seamless C++ wrapper.

## Motivation/Inspiration
What motivated and inspired me to create Tagha was...
* the QuakeC scripting language - inspired in the idea of using C as-is as a scripting language.
* the Pawn scripting language - a C-like (really B-like) scripting language with no real type system but very small enough to fit on embedded systems.
* Dynamically Loaded Libraries - the idea of an alternative to dynamically loaded libs.
* Virtual Machines - the idea of emulating a computer system and a programming language on top of virtual system made me set out to do similar.

## Installation/Building
download or git clone the project. The build scripts in the project give you the option to build Tagha as a static or shared library.

* **For a shared library**, edit `build_libtagha_shared.sh` as needed and execute. The build script will cleanup any \*.o files left behind.
* **For a static library**, edit `build_libtagha_static.sh` as needed and execute. The build script will cleanup any \*.o files left behind as well.
* **If you want to try out the bytecode executables using the example host app sources**, edit `build_hostapp.sh` as needed and execute.
* **To Generate Tagha Bytecode**, execute `tbc_asm.py` (requires python v3+) but careful as python assembler will generate the bytecode executables _in the same directory where it is executed_.

## Pull Requests
Pull requests are always welcome!
