 # Contents:
 * [Baby Steps](https://github.com/assyrianic/Tagha-Virtual-Machine/wiki/Embedding-Tagha-to-your-Application!-(C)#baby-steps)
 * [Exporting Host Functionality](https://github.com/assyrianic/Tagha-Virtual-Machine/wiki/Embedding-Tagha-to-your-Application!-(C)#exporting-host-functionality-to-scripts)
 * [Script - Native Communication](https://github.com/assyrianic/Tagha-Virtual-Machine/wiki/Embedding-Tagha-to-your-Application!-(C)#script--native-communication)
 * [Script - Host Messaging](https://github.com/assyrianic/Tagha/wiki/Embedding-Tagha-to-your-Application!-(C)#script--host-messaging)
 * ['va_list' Natives](https://github.com/assyrianic/Tagha-Virtual-Machine/wiki/Embedding-Tagha-to-your-Application!-(C)#va_list-arguments)

# Intro
Thank you for choosing Tagha as your runtime environment/scripting engine! Tagha has a wide range of API functions to help you get as much control as possible to data to and from scripts. Let's get started!

# Baby Steps

Let's first learn how to embed Tagha with this small, minimal as possible example of a host application:
```c
#include <stdio.h>
#include <stdlib.h>
#include "tagha.h"

int main(int argc, char *argv[const restrict static 1])
{
	/* create our script instance. */
	struct TaghaModule script = tagha_module_create_from_file("script.tbc");
	
	/* call 'main' with no params. */
	tagha_module_run(&script, 0, NULL);
	
	/* clean up script. */
	tagha_module_clear(&script);
}
```

As you can see, all you do is instantiate, execute, then free! If you need to know what API is wholly available to use, check out the [API Reference](https://github.com/assyrianic/Tagha-Virtual-Machine/wiki/Tagha-API-Reference).

## Running
After you've compiled the example code above, your app will run whatever script you have loaded onto Tagha's system.

# Exporting Host Functionality to Scripts
What good is embedding a runtime system if your scripts cannot use your host application's code? A staple feature of any scripting language is "hooking/binding" a function from the host to scripts. In Tagha, similar to Java, Python, and Pawn, hook functions are called **Natives**. Natives must follow a specific function prototype to properly use. Here is that prototype:
```c
union TaghaVal native_func_name(struct TaghaModule *ctxt, size_t args, const union TaghaVal params[]);
```
A little hard to work with at first but here's some explanation:

* `ctxt` is a pointer to the module that's calling the native function.
* `args` is the amount of arguments given to the native function when it's called.
* `params` is an `args`-sized array that contains each individual argument - the first argument starts at 0.

If you're wondering how returning data works, returning only requires you to pick the data type you want to return. To be more specific, natives return data through raw unions. here's an example:
```c
/* float give_hundred(void); */
static union TaghaVal
native_give_hundred(struct TaghaModule *const ctxt, const size_t args, const union TaghaVal params[const static 1])
{
	return (union TaghaVal){ .float32 = 100.f };
}
```
As you can see in the example above, the script-side prototype of the function takes no arguments but returns a float. To return a float, we use a compound initializer and initialize the `float32` member to our data returns a floating point value of "100.f" to our script's main register (`alaf`). One thing needs to be said: if your native takes no parameters and you try to use the `params` argument, **this is undefined behavior**, so don't do that unless you know absolutely what you're doing...

## How to Register Natives
Of course, how could any script use your natives if the runtime system doesn't know they exist? Another staple of scripting languages is the process of registering your hook functions. Here's how to register natives for Tagha using both the minimal embedding example AND the native example!
```c
/* file: 'myapp.c' */
#include <stdio.h>
#include <stdlib.h>
#include "tagha.h"

/* float give_hundred(void); */
static union TaghaVal
native_give_hundred(struct TaghaModule *const ctxt, const size_t args, const union TaghaVal params[const static 1])
{
	return (union TaghaVal){ .float32 = 100.f };
}

int main(int argc, char **argv)
{
	/* create our script instance. */
	struct TaghaModule script = tagha_module_create_from_file("script.tbc");
	
	/* before execution, register our natives! */
	const struct TaghaNative myapp_natives[] = {
		{"give_hundred", native_give_hundred},
		{NULL, NULL}
	};
	tagha_module_register_natives(&script, myapp_natives);
	
	/* call 'main' with no params. */
	tagha_module_run(&script, 0, NULL);
	
	/* clean up script. */
	tagha_module_clear(&script);
}
```
As you can see in the example, to properly register your host application's natives, you bind them to a string literal which will be used as its script-side name and wrap it to an array, so "`give_hundred`" will be the function name that scripts must use to invoke "`native_give_hundred`" itself. the end of the registration array must **always** be `NULL` so the registrar function can know when to stop iterating through the array.

## Script <==> Native Communication
In the last segment, we had a single native return a 4-byte float value of '100.f' but realistic natives from a host application wouldn't always be that simple. Many natives would probably return different values or pointers, some will even take arguments whether copy by value or by reference.

### Passing Structs by Reference
Let's create a native that returns nothing but takes a specific struct pointer! For example, assume the function prototype `void print_player_info(struct Player *);`
```c
struct Player {
	float speed;
	uint32_t health, ammo;
};

/* void print_player_info(struct Player *p); */
static union TaghaVal
native_print_player_info(struct TaghaModule *const ctxt, const size_t args, const union TaghaVal params[const static 1])
{
	/* get first arg which is the address to our data.
	 * cast the void* to a struct Player*, done implicitly in C.
	 */
	struct Player *const player = params[0].uintptr;
	if( player != NULL ) {
		printf("native_print_player_info :: ammo: %" PRIu32 " | health: %" PRIu32 " | speed: %f\n", player->ammo, player->health, player->speed);
	}
	return (union TaghaVal){0};
}
```
In the example above, we use a kind of video game-like data as an example and print out its data. It should be noted that the module relies on the ordering of the members when executing as the module's VM is little endian and only works on little endian systems.

### Returning Structs
So in the prior tutorial, we talked about how a native would use a struct reference but what if you wanted to create a native that actually *returns* a struct? This is also possible but it's tricky in that it *assumes* compiler optimizations!

Typically when a C program returns a struct, the function returning the struct and the code that uses it is optimized into a void function that takes a struct pointer as a hidden 1st parameter!

This **ONLY** applies if the struct is larger than 8 bytes, but if the struct is smaller than 8 bytes then you can get away with returning the struct directly from the native. To directly return a struct that's 8 bytes or smaller, the best strategy in this case is to use a `union` that typepuns between your struct type and `union TaghaVal`. Here's an example:
```c
struct Vec2D {
	float x,y;
};

/* struct Vec2D vec2d_create(float x, float y); */
static union TaghaVal
native_vec2d_create(struct TaghaModule *const ctxt, const size_t args, const union TaghaVal params[const static 1])
{
	const struct Vec2D v = { params[0].float32, params[1].float32 };
	const union {
		const struct Vec2D v;
		const union TaghaVal t;
	} pun = {v};
	return pun.t;
}
```


### Returning Pointers.
There are some cases where you will need to return pointers that are used by your host application such as dynamically allocated objects OR retrieving struct object pointers; assuming we're doing the latter, let's reuse our example from above but we'll modify the prototype to not only take in a struct Player pointer but to return a pointer to the unsigned int `health` struct member.
```c
struct Player {
	float speed;
	uint32_t health, ammo;
};
/* uint32_t *ip(struct Player *p); */
static union TaghaVal
native_ip(struct TaghaModule *const ctxt, const size_t args, const union TaghaVal params[const static 1])
{
	struct Player *const player = params[0].uintptr;
	
	/* return the address of the 4-byte int member "health" value. */
	return (player != NULL) ? (union TaghaVal){.uintptr = &player->health} : (union TaghaVal){.uintptr = NULL};
}
```

### Returning Memory Allocated pointers!
Now for some **advanced** pointer action! You're probably thinking, what if the pointer I need comes from the host side and I need to store it to the script side? Simple.

By using `malloc` and making a native around it, I can demonstrate to you how to return a pointer from a Host C application to script code.
```c
/* void *malloc(size_t size); */
static union TaghaVal
native_malloc(struct TaghaModule *const ctxt, const size_t args, const union TaghaVal params[const static 1])
{
	/* size_t is 8 bytes on 64-bit systems */
	return (union TaghaVal){ .uintptr = malloc(params[0].uint64) };
}
```
All we have to do is set the `uintptr` member of our return compound initializer and voila! If `malloc` returns a NULL pointer, the result will be NULL as well. Now we have our allocated pointer in our system. What if we wanted to use it?

Unfortunately, script's cannot use it directly because the pointer was allocated from C's native runtime and attempting to dereference the pointer will trigger a runtime memory access violation. The best (and safest) way to use a memory allocated data is through natives that manipulate the allocated pointers themselves.

A good and last example would be `free` implemented as a native which takes a pointer to de-allocate:
```c
/* void free(void *ptr); */
static union TaghaVal
native_free(struct TaghaModule *const ctxt, const size_t args, const union TaghaVal params[const static 1])
{
	free(( void* )params[0].uintptr);
	return (union TaghaVal){0};
}
```
As you can see, we first check if the address for the pointer is a valid one and then free it. This saves Tagha extra work by putting the pointer addressing effort on C's runtime instead of Tagha's runtime.


## Script <==> Host Messaging

### Calling Functions and Retrieving Global Vars by Name
Sometimes, there are cases when your host application needs to call a script defined function instead of a C/C++ function. There's only one way to call a function and it's by name.

Calling a function by name uses a simple API method, here's the example:
```c
void call_script_event(struct TaghaModule *const ctxt)
{
	tagha_module_call(ctxt, "my_function", 0, NULL, NULL);
}
```
In the example above, "my_function" is assumed to take no arguments.

Here's an example showing WITH passing arguments AND retrieving a return value. First thing required is that we push the values we want to pass to the function. Always push the last argument first and the first argument last:
```c
uint32_t factorialOfTen(struct TaghaModule *const ctxt)
{
	/* factorial takes a single, unsigned 4-byte int.
	 * push a single argument (hence first arg is 1).
	 * union TaghaVal argument must be an array but a pointer can be used as array.
	 */
	union TaghaVal retval = {0};
	tagha_module_call(ctxt, "factorial", 1, &(union TaghaVal){.uint32=10}, &retval);
	return retval.uint32;
}
```
From the example above, we call a recursive function called "factorial" and pass a single argument to it. In order to be able to push ANY type of data, we have to pass our data as an array of `union TaghaVal`.

After passing data and running the code, our result is in the accumulator register, ready to be popped which is always returned as `union TaghaVal` so we can retrieve any type of data from it, in our example, it's an unsigned 32-bit int. If you need to know what types `union TaghaVal` let's you type-pun, please look at Tagha's API reference.

Calling a script function is a relatively cheap process **BUT, you (the programmer) are responsible for making sure you pushed the correct data in the correct order**.

### Retrieving a Script-side Global Variable
There are certain cases where we have data that needs to be tracked globally, especially data that is tracked globally in our tagha scripts. For retrieving a script-side global variable, we require one more API method:
```c
void reset_script_globals(struct TaghaModule *const ctxt)
{
	int *const restrict windows_open = tagha_module_get_var(ctxt, "g_pwindows_open");
	if( windows_open != NULL )
		*windows_open = 0;
	
	int (*const restrict windows_colors)[4] = tagha_module_get_var(ctxt, "g_windows_colors");
	if( windows_colors != NULL )
		memset(&(*windows_colors)[0], 0, sizeof *windows_colors);
}
```
In this example, we get the data of a global variable that is assumed to named `g_pwindows_open`. `g_pwindows_open` is supposed to be compiled in the global data table as a 4 byte integer. `tagha_module_get_var` always returns a `void*` to the script's area of memory that the global's data resides in, so it's necessary null-check the given pointer in the event the script doesn't actually have the global var. Just like any global, you can directly modify `g_pwindows_open` from its pointer to whatever value you wish.


### "va_list" Arguments

Using `va_list` implementations are not hard to use for the Tagha Runtime until you attempt to use a native that takes a `va_list` argument. Implementation `va_list` for natives thankfully isn't difficult either.

To build a working `va_list` implementation, you only require two members: a pointer to the stack area and an unsigned integer that tracks the arguments in the stack area.

The area itself will be used as an array of the pushed arguments.

For our example function that uses a va_list from bytecode as an argument, we will make a native that calculates the average of a variadic amount of integer numbers given!

```c
/* int32_t va_int32_averages(va_list args); */
union TaghaVal
native_va_int32_averages(struct TaghaModule *const ctxt, const size_t args, const union TaghaVal params[const static 1])
{
	const struct {
		union TaghaVal area;
		uint64_t args;
	} *const valist = params[0].uintptr;
	
	int32_t res = 0;
	const union TaghaVal *const restrict ints = valist->area.uintptr;
	for( size_t i=0; i<valist->args; i++ )
		res += ints[i].int32;
	return (union TaghaVal){ .int32 = res / valist->args };
}
```

Here's the example step by step:
The first param will contain a pointer to Tagha's implementation of a `va_list` struct.
the struct has two fields: `args` & `area`.
`area` is a pointer-array (as type `union TaghaVal`) that stores the arguments of the `va_list`.
`args` is an integer (of type `uint64_t`) that stores the size of the pointer-array of `area`.

With the `area`, we dereference and retrieve `int32_t` values from it and add it with our `res` variable.
Finally we get the average calculation by dividing `res` with the `args` member of our `va_list` struct implementation.
