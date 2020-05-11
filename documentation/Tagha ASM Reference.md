# Intro
This article teaches about the TASM assembly language that's compiled by the TASM Assembler into a working .tbc script that Tagha executes.

# Comments
In TASM, only single line comments exist. the `;` or `#` both work as comments.

# Directives
In TASM asm, the assembly directives start with a dollar sign `$`. Here's a break down of all the TASM directives.

#### stacksize
TBC scripts, by default, are allocated a stack size of 128. The stack is aligned by 8 bytes so 128 becomes 1024.
the `$stacksize` directive allows a programmer (or generating compiler) to change the default stack size given to scripts.
The directive only takes one argument which is either a decimal, `0x` hexadecimal, `0b` binary, or `0#` octal argument.

Example tasm code usage:
```asm
$stacksize 10 ; sets the stack size to 10 * 8 bytes (decimal)
$stacksize 0x10 ; sets the stack size to 16 * 8 bytes (hexadecimal)
$stacksize 010 ; sets the stack size to 8 * 8 bytes (octal)
$stacksize 0b10 ; sets the stack size to 2 * 8 bytes (binary)
```

#### heapsize
TBC scripts utilize a memory allocator which stores the stack and data tables. After allocating the stack and data tables, what's left of the allocator can be used internally as heap data.

The `$heapsize` directive allows a programmer (or generating compiler) to leave extra heap memory for scripts to use.
The directive only takes one argument which is either a decimal, `0x` hexadecimal, `0b` binary, or `0#` octal argument.

Example tasm code usage:
```asm
$heapsize 10 ; sets the heap size to 10 bytes (decimal)
$heapsize 0x10 ; sets the heap size to 16 bytes (hexadecimal)
$heapsize 010 ; sets the heap size to 8 bytes (octal)
$heapsize 0b10 ; sets the heap size to 2 bytes (binary)
```

#### global
Since global variables in a tbc script are designed to be accessible by the host application, global variable require to be named and defined through tasm code.

the `$global` directive has different arguments depending on the type of global variable.

For strings (anonymous or named), the arguments are...
 * arg 1 - the name of the string.
 * arg 2 - the byte count of the string including the null terminator.
 * arg 3 -  the actual string data. Example tasm code usage of defining a string.
```asm
$global str1, 13, "hello world\n" ; escape chars are handled.
```

It should be noted here that the string length is automatically deduced by the assembler, the byte count argument is only necessary to be consistent. As long as the byte count argument is above 0, the string length will be computed.
So the above example could also be:

```asm
$global str1, 1, "hello world\n" ; escape chars are handled.
```


For every other variable, there's 2 kinds of possible arguments:
Like the string version, you must have a name for the variable as well as the byte size of the variable.
for the 3rd argument (the actual data). You have two (limited) options.
* the data for a global var must have each data count listed by size and the data and the count must be equal to the bytesize of the global var.
* if you're making a large global variable (struct or array or something) and you only want its data to be zero, you simply put a `0` as the 3rd argument without requiring to give a size.

```asm
$global i, 12,    0
$global n, 4,     byte 0, byte 0, byte 0, byte 0x40
$global m, 4,     half 0, half 0x4000 ; same as what n does, including data
$global c, 4,     long 0x40000000 ; same as n does, including data
$global ptr, 8,   word 0 ; 'word' is redundant since we're only zeroing.
```

#### native
the `$native` directive exists to expose the C or C++ native to the script. The directive only takes one argument which is the name of the native you want to expose but with a "function" syntax (having the % percent sign as the first character to denote that it's indeed a function). Here's an example tasm code that exposes `malloc` from `stdlib.h` to a script:
```asm
$native %malloc
```

#### extern
the `$extern` directive exists to do dynamic linking between scripts. The directive takes one argument which is the name of the module we link to and the function name. The module and function names are separated by the at-sign '@'.
Here's an example of how it's used.:
```asm
$extern %module_name@function_to_link
```

# Code Syntax
### Functions
In TASM Assembly, operations can _only_ exist within function blocks. Function names must **ALWAYS** start with a `%` percent sign character so that we can separate the name from jump labels. After defining the function name, you must follow the name with a colon `:` and a beginning curly brace `{`. Once a function's code is finished, you must end the block with, you guessed it, an ending curly brace `}`.

Let's create an example by defining a function and call it `main`! Since `main` is a function, we must give it a `ret` opcode so that the virtual machine can destroy the call frame after `main` is done calling!
```asm
; create "main" and its code block!
%main: {
    ret
}
```
It's necessary at this point to bring up that the rest of this article assumes that you've read the [Tagha Instruction Set article](https://github.com/assyrianic/Tagha-Virtual-Machine/wiki/Tagha's-Instruction-Set-Opcodes)!

Alright, we've created our `main` function but it doesn't do jack. Well let's make it do jack.
Before we start writing code, let me introduce you to a few more information beginning with the available registers in Tagha.

### Registers
Registers are special locations in a CPU that store data. All registers in Tagha are 64-bit and can be used for both integer, floating point, and memory based operations. Registers are vital is much of tagha's operations as they're useful in many optimization cases.

**Note**: all the register names start with an `r` to denote that it's a register! The Register names are all derived from the Syriac alphabet.

* **_ralaf_** - general-purpose/accumulator register, meaning that return values from functions & natives go here.
* **_rbeth_**
* **_rgamal_**
* **_rdalath_**
* **_rheh_**
* **_rwaw_**
* **_rzain_**
* **_rheth_**
* **_rteth_**
* **_ryodh_**
* **_rkaf_**
* **_rlamadh_**
* **_rmeem_**
* **_rnoon_**
* **_rsemkath_** - in a function/native call, holds the 1st parameter.
* **_r_eh_** - in a function/native call, holds the 2nd parameter.
* **_rpeh_** - in a function/native call, holds the 3rd parameter.
* **_rsadhe_** - in a function/native call, holds the 4th parameter.
* **_rqof_** - in a function/native call, holds the 5th parameter.
* **_rreesh_** - in a function/native call, holds the 6th parameter.
* **_rsheen_** - in a function/native call, holds the 7th parameter.
* **_rtaw_** - in a function/native call, holds the 8th parameter.
* **_rveth_** - in a function/native call, holds the 9th parameter.
* **_rghamal_** - in a function/native call, holds the 10th parameter.
* **_rdhalath_** - in a function/native call, holds the 11th parameter.
* **_rkhaf_** - in a function/native call, holds the 12th parameter.
* **_rfeh_** - in a function/native call, holds the 13th parameter.
* **_rthaw_** - in a function/native call, holds the 14th parameter.
* **_rzeth_** - in a function/native call, holds the 15th parameter.
* **_rdadeh_** - in a function/native call, holds the 16th parameter.
* **_rsp -_** stack pointer, it's wise **NOT** to directly change the value of this register or you'll cause crashes.
* **_rbp_** - base pointer aka the call frame pointer. Useful for referencing local function data.

### Immediate/Constant Values
One of the most common ways to input data is through constants or immediate values. Immediate values are always encoded as 8 bytes for the sake of convenience. Just like the `$stacksize` or `$heapsize` directive, an immediate value can be numbered as a decimal, hexadecimal, binary, or octal value!

### Register Indirection / Register-Memory
As mentioned in the registers portion, registers data can be used as memory addresses.
Dereferencing a register as a memory address has the following format:
```asm
[register_name +/- constant_offset]
```

Register-Memory operations also gives you the ability to add (or subtract) an offset to the address you're dereferencing. The offset is calculated in terms of bytes (again Tagha is byte-addressable).

Here's an example where we assume `alaf` contains a memory address and we want to dereference it as an int16:
```asm
st2 [ralaf + 0] ; store 2 bytes
```

Conveniently, you don't need the zero since it will not change the address given
```asm
st2 [ralaf] ; store 2 bytes
```
but you do need to do arithmetic for any number larger than 0:
```asm
st2 [ralaf+2]
```
which is necessary for cases such as arrays or structs.

### Putting It All Together
Now that we've gotten the 3 data operations covered, we can go back to our `main` example. Let's create an example where we do something like `1 + 2`!

```asm
; create "main" and its code block!
%main: {
    movi ralaf, 2     ; set alaf's value to 2
    movi rbeth, 1     ; set beth's value to 1
    add  ralaf, rbeth ; add beth to alaf, modifying alaf. alaf is now "3"
    ret
}
```

In the example above, we show how we can work directly with immediate values and registers. How about we use the same example with register-memory?

```asm
; create "main" and its code block!
%main: {
    movi ralaf, 2        ; set alaf's value to 2
    movi rbeth, 1
    st1  [rbp-1], rbeth  ; set a byte value within main's stack frame.
    ld1  rgamal, [rbp-1] ; load byte to register gamal.
    add  ralaf, rgamal   ; add the byte value to alaf, modifying alaf. alaf is now "3"
    ret
}
```
As you see in the example, we dereference a byte address in main's stack frame and set a value of 1 in that address; we then add the value of that address with the value contained in `alaf`.

We could also `add` the values the other way around!
```asm
; create "main" and its code block!
%main: {
    movi ralaf, 2       ; set alaf's value to 2
    movi rbeth, 1
    add  ralaf, rbeth   ; add alaf to the byte value, 
    st1  [rbp-1], ralaf ; modifying the address location. the address now contains "3"
    ret
}
```
Since `alaf` and many of the other registers are used to store temporary values, persistent data like locals are usually stored in the stack.

### Jump Labels
So far the only type of label you've been exposed to is the `%function` label but that's only for defining functions.
One of the most prized and valuable constructs in any programming language is the control flow controls like `if-else` statements, `switch`, and `while` or `for` loops.

In order to implement these control flow tools, we need the ability to label areas of flow!

In TASM assembly, jump labels are defined similar to function labels but instead of using a `%` percent sign, jump labels use a `.` dot sign!

Here's an example:
```asm
%main: {
.exit:
    ret
}
```
the jump label so far is pretty useless, all it does is point to where the `ret` opcode is.
Let's take all that we've learned and create real, usable code!

Let's create a `main` function in our script so the script can have a direct entry point and we'll have a secondary function called factorial:

```asm
%main: {
    ret
}

%factorial: {
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

So now let's implement that in our TASM assembly language!
```asm
%factorial: {
    movi    rbeth, 1
    st4     [rbp-4], rsemkath   ; store 1st param to local frame.
    
; if( i<=1 )
    cmp     rsemkath, rbeth     ; n==1?
    jz      .eval            ; return 1;
    
; return 1;
    mov     ralaf, rbeth     ; set return value to 1.
    ret
    
.eval:
; return i * factorial(i-1);
    ld4     ralaf, [rbp-4]   ; load 1st param that was stored to frame.
    sub     ralaf, rbeth     ; subtract by 1
    mov     rsemkath, ralaf  ; set semkath to new value in alaf.
    call    %factorial       ; recursive call
    ld4     rgamal, [rbp-4]  ; load 1st param again but to gamal.
    mul     ralaf, rgamal    ; multiply return value from recursive call with gamal.
    ret
}
```

Alright, we have a defined and working function in tasm asm, how can we call it within our script's asm code?
This is where we talk about Calling Convention.

Tagha's calling convention requires that 16 or less arguments be put in registers `semkath` to `dadeh` with `dadeh` holding the 16th parameter. More than 16 parameters requiring using a va_list (using the stack as an array). All return values that are 8 bytes or less must be returned in register `alaf`, otherwise the function must be optimized to take a hidden pointer parameter.

To actually give `factorial` arguments and call it from tasm asm code, we must set the `semkath` register to a numeric value and then call `factorial`, here's an example.
```asm
%main: {
    movi    rarg0, 5   ; `rarg0` is an alias for `rsemkath`.
    call    %factorial
    ret
}
```
when `factorial` is finished execution and its final stack frame has been popped, the final return value will be in `alaf`. The factorial of 5 is 120, so register `alaf` data will contain the 32-bit integer value of 120. Now concerning the C standard, `main` should be returning 0, so `alaf` must be set to 0 after the call to `factorial` is finished.
