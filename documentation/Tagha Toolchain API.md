# Intro
This article teaches how to utilize the Tagha toolchain API that helps in creating, debugging, & analyzing Tagha modules.


# Header-Only Files

## instr_gen.h
This header is for streamlining the instruction generation process.

The instruction generator function has the following signature:
```c
size_t tagha_instr_gen(struct HarbolByteBuf *tbc, enum TaghaInstrSet op, ...);
```

`tagha_instr_gen` returns a `size_t` integer indicating the amount of bytes written.

The `tbc` "tagha bytecode" argument can optionally be `NULL` so as to just return the amount of bytes that would've been written.

Arguments associated with `op` "opcode" will not be read/consumed if `tbc` is `NULL`, thus it's safe to simply pass a `NULL` argument and the opcode like so:
```c
const size_t call_bytes  = tagha_instr_gen(NULL, call);
const size_t redux_bytes = tagha_instr_gen(NULL, redux, 10); /// 10 is passed but won't be processed.
```

If `tbc` is _NOT_ `NULL`, then you MUST supply arguments for the opcode to process.


## module_gen.h
This header is for simplifying the module generation process.

The module generation process (using this header) starts by creating a module generator object:
```c
struct TaghaModGen modgen = tagha_mod_gen_create();
```

There are three API functions that fill in the module generator:
```c
void tagha_mod_gen_write_header(struct TaghaModGen *mod, uint32_t opstacksize, uint32_t callstacksize, uint32_t heapsize, uint32_t flags);

void tagha_mod_gen_write_func(struct TaghaModGen *mod, uint32_t flags, const char name[], const struct HarbolByteBuf *bytecode);

void tagha_mod_gen_write_var(struct TaghaModGen *mod, uint32_t flags, const char name[], const struct HarbolByteBuf *datum);
```

These 3 functions can be used at anytime after creating the module generator but not after finalization (will talk about this later).

Writing the module header is simple. You simplify fill in how large the two stacks will be, how large the module's heap will be, and the kind of runtime flags you want to give it!

For writing global variables and functions, you will need to use the byte buffer found in the Harbol library. Functions take a bytebuffer filled with bytecode (typically filled in by the `tagha_instr_gen` function we mentioned at the start) while global variables take a bytebuffer of their data, even if it's all 0.

Once you've filled the module generator object, you simply need to finalize the module by using one of 3 API functions:

```c
bool tagha_mod_gen_create_file(struct TaghaModGen *mod, const char filename[]);
struct HarbolByteBuf tagha_mod_gen_buffer(struct TaghaModGen *mod);
uint8_t *tagha_mod_gen_raw(struct TaghaModGen *mod);
```

NOTE: Finalizing the module WILL clear out the module generator object's data so keep this in mind.

`tagha_mod_gen_create_file` is good if you're compiling/assembling a Tagha Module to a file, simply supply a name for the module file binary.
`tagha_mod_gen_buffer` is good if you're disassembling or analyzing the generated module binary.
`tagha_mod_gen_raw` is good if you're dynamically creating a Tagha Module during runtime.

## module_info.h
This header is for debugging purposes by unwinding the runtime stacks `tagha_module_print_callstack` and `tagha_module_print_opstack` or emitting the module's header information `tagha_module_print_header`.

The following functions are:
```c
void tagha_module_print_header(const struct TaghaModule *mod, FILE *stream);
void tagha_module_print_opstack(const struct TaghaModule *mod, FILE *stream);
void tagha_module_print_callstack(const struct TaghaModule *mod, FILE *stream);
```