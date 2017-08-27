# Tagha Virtual Machine
"tagha" is Aramaic for "crown".
Tagha is a **WIP** minimal yet complex stack-based virtual machine and scripting engine, written in C, designed to run C compiled as tagha scripts. Why is it named "crown"? Because C is king in the programming world :P

[See the Wiki for API references, Opcodes reference, Tutorials, and more](https://github.com/assyrianic/Tagha-Virtual-Machine/wiki)


## TODO list
- [x] add `q` or 'quad' opcodes for 64-bit data ~~AND make VM completely 64-bit (32-bit will NOT be available)~~.
- [x] add API for host applications to embed this VM.
- [x] figure out natives system.
- [x] create format for libraries and headers.
- [ ] figure out how to share types between host and scripts.
- [ ] figure out system of communication between Tagha C scripts.

## End Goals list
- [ ] complete, seamless embeddability to C (and by extension C++) programs. As smooth as how Angelscript binds to C++.
- [ ] compatibility of as much of the C standards possible, including C11.
- [ ] implement a compiler or compiler backend that generates bytecode for this VM. Possibility: Make GCC output Tagha bytecode using a custom GCC Backend but will take alot of effort. Maybe use Clang/LLVM backend to output Tagha bytecode?
- [ ] Windows compatibility/availability.
- [ ] Ultimate Goal: VM can bootstrap itself! (compiled to TaghaVM bytecode and ran by TaghaVM itself).
