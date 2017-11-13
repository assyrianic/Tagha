# Tagha Virtual Machine
"tagha" is Aramaic for "crown".
Tagha is a **WIP** minimal yet complex stack-based virtual machine and scripting engine, written in C, designed to run C compiled as tagha scripts. Why is it named "crown"? Because C is king in the programming world :P

[See the Wiki for API references, Opcodes reference, Tutorials, and more](https://github.com/assyrianic/Tagha-Virtual-Machine/wiki)

## (Current) Features:
* Scripts can call Host functions (natives).
* Host can call script functions, with pushing and retrieving data!
* Host can retrieve script global variables.
* Host can bind its own global variables as a pointer to a script global variable.
* Very small memory usage: around 60-80kb of memory.
* No dependencies except libc!

## Installation/Building
download or git clone the project. The build scripts in the project give you the option to build Tagha as a static or shared library.

* **For a shared library**, edit `build_libtagha_shared.sh` as needed and execute. The build script will cleanup any \*.o files left behind.
* **For a static library**, edit `build_libtagha_static.sh` as needed and execute. The build script will cleanup any \*.o files left behind as well.
* **If you want to try out the bytecode executables using the example host app sources**, edit `build_hostapp.sh` as needed and execute.
* **To Generate Tagha Bytecode**, execute `tbc_asm.py` (requires python v3+) but careful as python assembler will generate the bytecode executables _in the same directory where it is executed_.

## Pull Requests
Pull requests are always welcome and will be reviewed!
