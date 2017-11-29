
#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include "tagha.hpp"

/*
 * Same as hostapp.c but using C++ code to test portability and compatibility.
*/

/* void print_helloworld(void); */
static void native_print_helloworld(TaghaScript_ *script, Param_t params[], Param_t *retval, const uint32_t argc, TaghaVM_ *env)
{
	puts("native_print_helloworld :: hello world from bytecode!\n");
}

/* void test_ptr(struct player *p); */
static void native_test_ptr(TaghaScript_ *script, Param_t params[], Param_t *retval, const uint32_t argc, TaghaVM_ *env)
{
	struct Player {
		float		speed;
		uint32_t	health;
		uint32_t	ammo;
	} *player=nullptr;
	
	// get first arg which is the virtual address to our data.
	player = reinterpret_cast< struct Player* >(params[0].Ptr);
	if( !player )
		return;
	
	// debug print to see if our data is accurate.
	printf("native_test_ptr :: ammo: %" PRIu32 " | health: %" PRIu32 " | speed: %f\n", player->ammo, player->health, player->speed);
	player=nullptr;
}

/* void getglobal(void); */
static void native_getglobal(TaghaScript_ *script, Param_t params[], Param_t *retval, const uint32_t argc, TaghaVM_ *env)
{
	int *p = (int *)script->get_global_by_name("i");
	if( !p )
		return;
	
	printf("native_getglobal :: i == %i\n", *p);
}



int main(int argc, char **argv)
{
	if( !argv[1] ) {
		printf("[TaghaVM Usage]: '%s' '.tbc file' \n", argv[0]);
		return 1;
	}
	TaghaVM_ *VM = new TaghaVM_();
	NativeInfo_ host_natives[] = {
		{"test", native_test_ptr},
		{"printHW", native_print_helloworld},
		{"getglobal", native_getglobal},
		{NULL, NULL}
	};
	VM->register_natives(host_natives);
	VM->load_libc_natives();
	VM->load_self_natives();
	VM->load_script_by_name(argv[1]);
	VM->exec(nullptr);
	
	/*
	// tested with test_3d_vecs.tbc
	float vect[3]={ 10.f, 15.f, 20.f };
	Taghascript Script = Taghascript(VM->get_script());
	Script.push_value((Val_t){ .Pointer=vect });
	VM->call_script_func("vec_invert");
	printf("vect[3]=={ %f , %f, %f }\n", vect[0], vect[1], vect[2]);
	*/
	
	VM->del();
	delete VM;
}
