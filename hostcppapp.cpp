
#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include "tagha.hpp"

/*
 * Same as hostapp.c but using C++ code to test portability and compatibility.
*/

/* void print_helloworld(void); */
static void native_print_helloworld(Script_t *script, Param_t params[], Param_t *retval, const uint32_t argc, TaghaVM_t *env)
{
	puts("native_print_helloworld :: hello world from bytecode!\n");
}

/* void test_ptr(struct player *p); */
static void native_test_ptr(Script_t *script, Param_t params[], Param_t *retval, const uint32_t argc, TaghaVM_t *env)
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
static void native_getglobal(Script_t *script, Param_t params[], Param_t *retval, const uint32_t argc, TaghaVM_t *env)
{
	TaghaScriptCPP Script = TaghaScriptCPP(script);
	int *p = (int *)Script.get_global_by_name("i");
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
	TaghaVMCPP *VM = new TaghaVMCPP();
	NativeInfo_t host_natives[] = {
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
	TaghaScriptCPP Script = TaghaScriptCPP(VM->get_script());
	Script.push_value((Val_t){ .Pointer=vect });
	VM->call_script_func("vec_invert");
	printf("vect[3]=={ %f , %f, %f }\n", vect[0], vect[1], vect[2]);
	*/
	
	VM->del();
	delete VM;
}
