 # Contents:
 * [Baby Steps](https://github.com/assyrianic/Tagha-Virtual-Machine/wiki/Embedding-Tagha-to-your-Application!-(C)#baby-steps)
 * [Exporting Host Functionality](https://github.com/assyrianic/Tagha-Virtual-Machine/wiki/Embedding-Tagha-to-your-Application!-(C)#exporting-host-functionality-to-scripts)
 * [Script - Native Communication](https://github.com/assyrianic/Tagha-Virtual-Machine/wiki/Embedding-Tagha-to-your-Application!-(C)#script--native-communication)
 * [Script - Host Sharing](https://github.com/assyrianic/Tagha-Virtual-Machine/wiki/Embedding-Tagha-to-your-Application!-(C)#script--host-sharing)
 * [Custom Main Arguments](https://github.com/assyrianic/Tagha-Virtual-Machine/wiki/Embedding-Tagha-to-your-Application!-(C)#giving-main-custom-arguments)

# Intro
Thank you for choosing Tagha as your runtime environment/scripting engine! Tagha has a wide range of API functions to help you get as much control as possible to data to and from scripts. Let's get started!

# Baby Steps

Let's first learn how to embed Tagha with this small, minimal as possible example of a host application:
```c
#include <stdio.h>
#include <stdlib.h>
#include "tagha.h"

int main(int argc, char *argv[])
{
	/* make our script instance. */
	struct TaghaModule *script = tagha_module_new_from_file("script.tbc");
	
	/* call 'main' with no command-line arguments. */
	tagha_module_run(script, 0, NULL);
	
	/* clean up script, pointer will be set to NULL. */
	tagha_module_free(&script);
}
```

As you can see, all you do is instantiate, execute, then free! If you need to know what API is wholly available to use, check out the [API Reference](https://github.com/assyrianic/Tagha-Virtual-Machine/wiki/Tagha-API-Reference).

## Running
After you've compiled the example code above, your app will run whatever script you have loaded onto Tagha's system.

# Exporting Host Functionality to Scripts
What good is embedding a scripting system if your scripts cannot use your host application's code? A staple feature of any scripting language is "hooking/binding" a function from the host to scripts. In Tagha, similar to Java and Pawn, hook functions are called **Natives**. Natives must follow a specific function prototype to properly use. Here is that prototype:
```c
void native_func_name(struct TaghaModule *ctxt, union TaghaVal *restrict retval, const size_t args, union TaghaVal params[restrict static args]);
```
A little hard to work with at first but here's some explanation: `ctxt` is a pointer to the module that's calling the native function, `params` is an `args`-sized array that contains each argument, the first argument starts at 0, `retval` is a pointer for returning data back to the script, and `args` is the amount of arguments given to the native function when it's called.

If you're wondering how would `retval` return data, returning only requires you to pick the data type you want to return, here's an example:
```c
/* float give_hundred(void); */
static void
native_give_hundred(struct TaghaModule *ctxt, union TaghaVal *restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	retval->Float = 100.f;
}
```
As you can see in the example above, the script-side prototype of the function takes no arguments but returns a float. To return a float, dereferencing `retval` and setting the `Float` member to our data returns a floating point value of "100.f" to our script's accumulator register (`Alaf`). One thing needs to be said: if your native takes no parameters and you try to use the `params` argument, **this is undefined behavior**, so don't do that unless you know absolutely what you're doing...

## How to Register Natives
Of course, how could any script use your natives if Tagha's runtime system doesn't know they exist? Another staple of scripting languages is the process of registering your hook functions. Here's how to register natives for Tagha using both the minimal embedding example AND the native example!
```c
/* file: 'myapp.c' */
#include <stdio.h>
#include <stdlib.h>
#include "tagha.h"

/* float give_hundred(void); */
static void
native_give_hundred(struct TaghaModule *const ctxt, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	retval->Float = 100.f;
}

int main(int argc, char **argv)
{
	/* make our script instance. */
	struct TaghaModule *script = tagha_module_new_from_file("script.tbc");

	/* before execution, register our natives! */
	const struct TaghaNative myapp_natives[] = {
		{"give_hundred", native_give_hundred},
		{NULL, NULL}
	};
	tagha_module_register_natives(script, myapp_natives);

	/* call 'main' with no command-line arguments. */
	tagha_module_run(script, 0, NULL);
	
	/* clean up script, pointer will be set to NULL. */
	tagha_module_free(&script);
}
```
As you can see in the example, to properly register your host application's natives, you bind them to a string literal which will be used as its script-side name and wrap it to an array, so "`give_hundred`" will be the function name that scripts must use to invoke "`native_give_hundred`" itself. the end of the registration array must **always** be `NULL` so the registrar function can know when to stop iterating through the array.

## Script <==> Native Communication
In the last segment, we had a single native return a 4-byte float value of '100.f' but realistic natives from a host application wouldn't always be that simple. Many natives would probably return different values or pointers, some will even take arguments whether copy by value or by reference.

### Passing Structs by Reference
Let's create a native that returns nothing but takes a specific struct pointer! For example, assume the function prototype `void f(struct Player *);`
```c
struct Player {
	float	speed;
	uint32_t health;
	uint32_t ammo;
};
/* void f(struct player *p); */
static void native_print_player_info(struct TaghaModule *const ctxt, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	/* get first arg which is the address to our data.
	 * cast the void * to a struct Player *, done implicitly in C.
	 */
	struct Player *player = params[0].Ptr;
	if( !player )
		return;
	
	printf("native_print_player_info :: ammo: %" PRIu32 " | health: %" PRIu32 " | speed: %f\n", player->ammo, player->health, player->speed);
}
```
In the example above, we use a kind of video game-like data as an example and print out its data. It should be noted that the module relies on the ordering of the members when executing as the module's VM is little endian and only works on little endian systems.


### Returning Pointers.
There are some cases where you will need to return pointers that are used by your host application such as dynamically allocated objects OR retrieving struct object pointers; assuming we're doing the latter, let's reuse our example from above but we'll modify the prototype to not only take in a struct Player pointer but to return a pointer to the unsigned int `health` struct member.
```c
struct Player {
	float	speed;
	uint32_t health;
	uint32_t ammo;
};
/* int *ip(struct player *p); */
static void native_ip
(struct TaghaModule *const ctxt, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	struct Player *player = params[0].Ptr;
	if( !player )
		return;

	/* return the address of the 4-byte int member "health" value. */
	retval->Ptr = &player->health;
}
```

### Returning Memory Allocated pointers!
Now for some **advanced** pointer action! You're probably thinking, what if the pointer I need comes from the host side and I need to store it to the script side? Easy to mess up but still simple!

By using `malloc` and making a native around it, I can demonstrate to you how to return a pointer from a Host C application to script code.
```c
/* void *malloc(size_t size); */
static void native_malloc
(struct TaghaModule *const ctxt, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	/* size_t is 8 bytes on 64-bit systems */
	retval->Ptr = malloc(params[0].UInt64);
}
```
All we have to do is set the `Ptr` member of `retval` and voila! If `malloc` returns a NULL pointer, the result will be NULL as well. Now we have our allocated pointer in our system. What if we wanted to use it?

Unfortunately, script's cannot use it directly in a clean way because the pointer was allocated from C's runtime, unless the pointer's data is defined on the script side such in the case of structs and unions but many times you'll likely return an allocated object to internal data used by the host app. The best (and safest) way to use a memory allocated data is through natives that manipulate the allocated pointers themselves.

A good and last example would be `free` implemented as a native which takes a pointer to de-allocate:
```c
/* void free(void *ptr); */
static void native_print_player_inforee
(struct TaghaModule *const ctxt, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	void *ptr = params[0].Ptr;
	free(ptr), ptr=NULL;
}
```
As you can see, we first check if the address for the pointer is a valid one and then free it. This saves Tagha extra work by putting the pointer addressing effort on C's runtime instead of Tagha's runtime.


## Script <==> Host Sharing

### Calling Functions and Retrieving Global vars by Name
Sometimes, there are cases when your host application needs to call a script defined function instead of a C/C++ function. There's only one way to call a function and it's by name.

Calling a function by name uses a simple API method, here's the example:
```c
void CallScriptEvent(struct TaghaModule *const ctxt)
{
	tagha_module_call(ctxt, "MyFunction", 0, NULL, NULL);
}
```
In the example above, "MyFunction" is assumed to take no arguments.

Here's an example showing WITH passing arguments AND retrieving a return value. First thing required is that we push the values we want to pass to the function. Always push the last argument first and the first argument last:
```c
uint32_t factorialOfTen(struct TaghaModule *const ctxt)
{
	/* factorial takes a single, unsigned 4-byte int.
	 * push a single argument (hence first arg is 1).
	 * union TaghaVal argument must be an array but a pointer can be used as array.
	 */
	union TaghaVal retval = {0};
	tagha_module_call(vm, "factorial", 1, &(union TaghaVal){.UInt32=10}, &retval);
	return retval.UInt32;
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
	int *restrict windows_open = tagha_module_get_globalvar_by_name(ctxt, "g_pwindows_open");
	if( !windows_open )
		return;
	*windows_open = 0;

	int *restrict windows_colors = tagha_module_get_globalvar_by_name(ctxt, "g_windows_colors");
	windows_colors[0] = 0; windows_colors[1] = 0; windows_colors[2] = 0; windows_colors[3] = 0;
}
```
In this example, we get the data of a global variable that is assumed to named `g_pwindows_open`. `g_pwindows_open` is supposed to be compiled in the global data table as a 4 byte integer. `tagha_module_get_globalvar_by_name` always returns a `void*` to the script's area of memory that the global's data resides in, so it's necessary to cast and null-check the returned pointer in the event the script doesn't actually have the global var. Just like any global, you can directly modify `g_pwindows_open` from its pointer to whatever value you wish.


### Sending command arguments to Scripts
Tagha cannot bear the standard of being a runtime environment without one important feature: Command-line arguments.
Tagha internally manages a vector of command line argument strings.

Scripts themselves can directly use `argc` and `argv` but arguments MUST be set up for each script when calling `main`. Here is an example:

```c
int main()
{
	/* make our script instance. */
	struct TaghaModule *script = tagha_module_new_from_file("script.tbc");
	
	/* Execute our script! */
	char force[] = "--force";
	char *args[] = {
		argv[1], /* set string as our script name + dir. */
		force, /* pass custom argument. */
		NULL /* end the array with a NULL ptr. */
	};
	tagha_module_run(script, 2, args);
	
	/* clean up script, pointer will be set to NULL. */
	tagha_module_free(&script);
}
```
As you see in the example, we create an array of strings which will be passed to `tagha_module_run`. `tagha_module_run` will copy the contents of the strings and resize the `argv` vector as necessary. Note that you MUST end the array with `NULL` just like with registering natives. This is *required* as part of the C standards.

After setting up the arguments, your script can now utilize `argv` and `argc`:
```c
/* script-side */
int main(int argc, char *argv[])
{
	/* prints '--force'. */
	puts(argv[1]);
}
```

### Giving 'main' custom arguments
Being able to pass strings to a string is fine and all but some devs would disagree and would rather pass something more useful to their scripts devs. Not a problem, `main` itself can be given custom arguments!

To give `main` custom arguments, we need to use `tagha_module_call` and manually call `main` while giving our own arguments.

Let's say we have a small GTK-based application and we want the end user to be able to modify the window and button actions (within reason of course). Our `main` function would probably look like this:
```c
int main(GtkWindow *w, const size_t numbuttons, GtkWidget *buttons[static numbuttons])
{
	...
}
```

Achieving this is no different than simply calling another function by name. We make an array of `union TaghaVal` and fill it with our arguments.
**NOTE: If an argument you want to pass is an array of pointers and the pointer size on your system is less than 64-bit, you MUST use another array of `union TaghaVal` to store those pointers or else the array indexing will be corrupted.**

Here's a full example of how to create our custom main with the arguments above on our host side.
```c
int32_t run_custom_main(struct TaghaModule *const ctxt, GtkWindow *const w, const size_t numbuttons, GtkWidget *const buttons[static numbuttons])
{
	union TaghaVal main_args[3] = { {0},{0},{0} };
	main_args[0].Ptr = w;
	main_args[1].SizeInt = numbuttons;
	if( sizeof(intptr_t)<sizeof(int64_t) ) {
		/* if pointer sizes are less than 64-bit, we pad out the ptr array with 'union TaghaVal'.
		 */
		union TaghaVal ptr_padder[numbuttons]; memset(ptr_padder, 0, sizeof(union TaghVal) * numbuttons);
		for( size_t i=0 ; i<numbuttons ; i++ )
			ptr_padder[i].Ptr = buttons[i];
		main_args[2].Ptr = ptr_padder;
		return tagha_module_call(ctxt, "main", 3, main_args, NULL);
	} else {
		main_args[2].Ptr = buttons;
		return tagha_module_call(ctxt, "main", 3, main_args, NULL);
	}
}
```
