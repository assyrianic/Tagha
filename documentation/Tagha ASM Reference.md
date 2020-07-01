# Intro
This article teaches about the Tagha Assembly language that's compiled by the Tagha Assembler into a working .tbc script

# Comments
In Tagha Assembly, only single line comments exist. the `;` or `#` both work as comments.

# Directives
In Tagha Assembly, the assembly directives start with a dollar sign `$`. Here's a break down of all the Tagha Assembly directives.

#### opstack_size
TBC scripts, by default, are alloted an operand stack size of 4kb.
the `$opstack_size` directive allows a programmer (or generating compiler) to change the default operand stack size given to scripts.
The directive only takes one argument which is either a decimal, `0x` hexadecimal, `0b` binary, or `0#` octal argument.

Example Tagha Assembly code usage:
```asm
$opstack_size 10   ;; sets the stack size to 10 * 8 bytes (decimal)
$opstack_size 0x10 ;; sets the stack size to 16 * 8 bytes (hexadecimal)
$opstack_size 010  ;; sets the stack size to 8 * 8 bytes  (octal)
$opstack_size 0b10 ;; sets the stack size to 2 * 8 bytes  (binary)
```

#### callstack_size
Used the same way as `$opstack_size` but for a module's call stack size.
```asm
$callstack_size 10   ;; sets the stack size to 10 * 8 bytes (decimal)
$callstack_size 0x10 ;; sets the stack size to 16 * 8 bytes (hexadecimal)
$callstack_size 010  ;; sets the stack size to 8 * 8 bytes  (octal)
$callstack_size 0b10 ;; sets the stack size to 2 * 8 bytes  (binary)
```

#### heap_size
TBC scripts utilize a memory allocator which stores the stack and data tables. After allocating the stack and data tables, what's left of the allocator can be used internally as heap data.

The `$heap_size` directive allows a programmer (or generating compiler) to leave extra heap memory for scripts to use.
The directive only takes one argument which is either a decimal, `0x` hexadecimal, `0b` binary, or `0#` octal argument.

Example Tagha Assembly code usage:
```asm
$heap_size 10 ; sets the heap size to 10 bytes (decimal)
$heap_size 0x10 ; sets the heap size to 16 bytes (hexadecimal)
$heap_size 010 ; sets the heap size to 8 bytes (octal)
$heap_size 0b10 ; sets the heap size to 2 bytes (binary)
```

#### global
Since global variables in a tbc script are designed to be accessible by the host application, global variable require to be named and defined through Tagha Assembly code.

the `$global` directive has different arguments depending on the type of global variable.

For strings (anonymous or named), the arguments are...
 * arg 1 - the name of the string.
 * arg 2 -  the actual string data.

Example Tagha Assembly code usage of defining a string.
```asm
$global str1, "hello world\n"    ;; escape chars are handled.
```

For every other variable, there's 2 kinds of possible arguments:
Like the string version, you must have a name for the variable as well as the byte size of the variable.
for the 3rd argument (the actual data). You have two (limited) options.
* the data for a global var must have each data count listed by size and the data and the count must be equal to the bytesize of the global var.
* if you're making a large global variable (struct or array or something) and you only want its data to be zero, you simply put a `0` as the 3rd argument without requiring to give a size.

```asm
$global i,   12,  0
$global n,   4,   byte 0, byte 0, byte 0, byte 0x40
$global m,   4,   half 0, half 0x4000 ; same as what n does, including data
$global c,   4,   long 0x40000000 ; same as n does, including data
$global ptr, 8,   word 0 ; 'word' is redundant since we're only zeroing.
```

#### native
the `$native` directive exists to expose the C or C++ native to the script. The directive only takes one argument which is the name of the native you want to expose.

Here's an example Tagha Assembly code that exposes `malloc` from `stdlib.h` to a script:
```asm
$native malloc
```

#### extern
the `$extern` directive exists to do dynamic linking between scripts. The directive only takes one argument which is the name of the external function to link.

Here's an example of how it's used:
```asm
$extern function_name
```

# Code Syntax
### Functions
In Tagha Assembly, operations can _only_ exist within function blocks. After defining the function name, you must follow the name with a beginning curly brace `{`. Once a function's code is finished, you must end the block with, you guessed it, an ending curly brace `}`.

Let's create an example by defining a function and call it `main`! Since `main` is a function, we must give it a `ret` opcode so that the virtual machine can destroy the execution frame after `main` is done calling!

```asm
;; create "main" and its code block!
main: {   ;; : colon is optional here!
    ret
}
```

It's necessary at this point to bring up that the rest of this article assumes that you've read the [Tagha Instruction Set article](https://github.com/assyrianic/Tagha-Virtual-Machine/wiki/Tagha's-Instruction-Set-Opcodes)!

Alright, we've created our `main` function but it doesn't do jack. Now let's make it do jack.
Before we start writing code, let me introduce you to registers in Tagha.

### Registers
Registers are special locations in a CPU that store data. All registers in Tagha are 64-bit and can be used for both integer, floating point, & memory based operations. Registers are vital is much of Tagha's operations as they're useful in many optimization cases.

**Note**: all the register names start with an `r` to denote that it's a register!

Tagha allows you to use up to 256 registers in a given execution frame. All that is necessary to save register space is by allocating and reducing the amount of registers available in each function execution context.

### Immediate/Constant Values
One of the most common ways to input data is through constants or immediate values. Immediate values are always encoded as 8 bytes for the sake of convenience. Just like the `$opstack_size` or `$heap_size` directive, an immediate value can be numbered as a decimal, hexadecimal, binary, or octal value!

### Register Indirection / Register-Memory
As mentioned in the registers portion, registers data can be used as memory addresses.
Dereferencing a register as a memory address has the following format:
```asm
[register_name +/- constant_offset]
```

Register-Memory operations also gives you the ability to add (or subtract) an offset to the address you're dereferencing. The offset is calculated in terms of bytes (again Tagha is byte-addressable).

Here's an example where we assume `alaf` contains a memory address and we want to dereference it as an int16:
```asm
st2 [r0 + 0]   ;; store 2 bytes
```

Conveniently, you don't need the zero since it will not change the address given
```asm
st2 [r0]    ;; store 2 bytes
```
but you do need to do arithmetic for any number larger than 0:
```asm
st2 [r0+2]
```
which is necessary for cases such as arrays or structs.

### Putting It All Together
Now that we've gotten the 3 data operations covered, we can go back to our `main` example. Let's create an example where we do something like `1 + 2`!

```asm
;; create "main" and its code block!
main: {
    alloc  2         ;; allocate 2 registers
    movi   r0, 2     ;; set r0's value to 2
    movi   r1, 1     ;; set r1's value to 1
    add    r0, r1    ;; add r1 to r0, modifying r0. r0 is now "3"
    ret
}
```


### Jump Labels
So far the only type of label you've been exposed to is the `function` label but that's only for defining functions.
One of the most prized and valuable constructs in any programming language is the control flow controls like `if-else` statements, `switch`, and `while` or `for` loops.

In order to implement these control flow tools, we need the ability to label areas of flow!

In Tagha ASM, jump labels are defined using a `.` dot sign!

Here's an example:
```asm
main {
.exit
    ret
}
```

the jump label so far is pretty useless, all it does is point to where the `ret` opcode is.
Let's take all that we've learned and create real, usable code!

Let's create a `main` function in our script so the script can have a direct entry point and we'll have a secondary function called factorial:

```asm
main: {
    ret
}

factorial: {
    ret
}
```

So far both functions do nothing. We'll implement factorial as a function that takes a single int32 parameter and returns an int32 as well.

the implementation of the factorial looks like this in C and Python respectively:
```c
uint32_t factorial(const uint32_t i) {
  return i<=1 ? 1 : i * factorial(i-1);
}
```

```python
def factorial(i:int) -> int:
    if i<=1:
        return i
    return i * factorial(i-1)
```

So now let's implement that in our Tagha ASM language!

Now the important part here is that since `factorial` calls another function (itself), we must save the link register before calling another function and then restore it once the execution frame goes back to the original one by using `pushlr` and `poplr`.

This is where we talk about Calling convention.

Calling convention is that argument counts AND return values go into `r0` which is the top of stack.
If the function returns nothing (`void`), `r0` may be used whenever as pleased by the compiler.

If the function calls another (bytecode) function (since `factorial` calls itself), it's REQUIRED to push the link register (by using `pushlr`) and pop it back (by using `poplr`) at the end of the function's context.


```asm
factorial {
    pushlr            ;; preserve link register.
    alloc   3         ;; allot 3 registers for this function frame!
    mov     r0, r3    ;; r3 is previous call's r0!
    
;; if( i<=1 )
    movi    r1, 1
    ule     r0, r1
    jz      .L1
    
;; return 1;
    mov     r0, r1
    jmp     .L2
    
.L1
;; return i * factorial(i-1);
    mov     r2, r0    ;; int temp = i;
    sub     r2, r1    ;; temp -= 1;
    mov     r0, r2
    call    factorial ;; int res = factorial(temp);
    mul     r3, r0    ;; i * res;
    
.L2
    poplr             ;; restore link register.
    redux   3         ;; put back the 3 registers we allotted.
    
    ret
}
```

Alright, we have a defined and working function in Tagha Assembly, how can we call it within our script's Tagha ASM code?

To actually give `factorial` arguments and call it from Tagha Assembly code, we must set the `semkath` register to a numeric value and then call `factorial`, here's an example.
```asm
main {
    pushlr             ;; since we're gonna call factorial, preserve link register.
    alloc   1          ;; allocate 1 register.
    movi    r0, 5      ;; set that 1 reg to hold the argument of 5.
    call    factorial  ;; call factorial.
    poplr              ;; we've returned from 'factorial', restore old link register.
    ret
}
```

When `factorial` has finished execution, the final return value will be in `r0`. The factorial of 5 is 120, so register `r0` data will contain the 32-bit integer value of 120. Now concerning the C standard, `main` should be returning 0, so `r0` must be set to 0 after the call to `factorial` is finished.
