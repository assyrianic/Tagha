# Tagha Virtual Machine
"tagha" is Aramaic for "crown".
Tagha is a **WIP** minimal yet complex stack-based virtual machine and scripting engine, written in C, designed to run C compiled as tagha scripts. Why is it named "crown"? Because C is king in the programming world :P

[See the Wiki for API references, Opcodes reference, Tutorials, and more](https://github.com/assyrianic/Tagha-Virtual-Machine/wiki)


## TODO list
- [x] add `q` or 'quad' opcodes for 64-bit data.
- [x] add API for host applications to embed this VM.
- [x] figure out natives system.
- [x] create format for libraries and headers.
- [ ] have host apps able to call functions on script side? (would require header changes put entry point and function name as a whole function table).
- [ ] figure out how to share types between host and scripts.
- [ ] figure out system of communication between Tagha C scripts.
- [ ] upgrade SIMD opcodes to use 32 bytes instead of 16 bytes (allows us to do math with four 4-byte data at once).

## End Goals list
- [ ] complete, seamless embeddability to C (and by extension C++) programs. As smooth as how Angelscript binds to C++.
- [ ] compatibility of as much of the C standards possible, including C11.
- [ ] implement a compiler or compiler backend that generates bytecode for this VM. Possibility: Make GCC output Tagha bytecode using a custom GCC Backend but will take alot of effort. Maybe use Clang/LLVM backend to output Tagha bytecode?
- [ ] Windows compatibility/availability.
- [ ] Ultimate Goal: VM can bootstrap itself! (compiled to TaghaVM bytecode and ran by TaghaVM itself).
