
#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include "tagha.hpp"

/*
 * Same as hostapp.c but using C++ code to test portability and compatibility.
*/

/* void print_helloworld(void); */
static void native_print_helloworld(TaghaScript_ *pScript, CValue params[], CValue *pRetval, const uint32_t argc, TaghaVM_ *pEnv)
{
	puts("native_print_helloworld :: hello world from bytecode!\n");
}

/* void test_ptr(struct player *p); */
static void native_test_ptr(TaghaScript_ *pScript, CValue params[], CValue *pRetval, const uint32_t argc, TaghaVM_ *pEnv)
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
static void native_getglobal(TaghaScript_ *pScript, CValue params[], CValue *pRetval, const uint32_t argc, TaghaVM_ *pEnv)
{
	int *p = (int *)pScript->GetGlobalByName("i");
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
	TaghaVM_ *pVM = (TaghaVM_ *)calloc(1, sizeof(TaghaVM_));
	NativeInfo_ host_natives[] = {
		{"test", native_test_ptr},
		{"printHW", native_print_helloworld},
		{"getglobal", native_getglobal},
		{nullptr, nullptr}
	};
	pVM->RegisterNatives(host_natives);
	pVM->LoadLibCNatives();
	pVM->LoadSelfNatives();
	pVM->LoadScriptByName(argv[1]);
	
	int argcount = 3;
	CValue args[argcount];
	args[0].Str = argv[1],
	args[1].String = "kektus",
	args[2].Str = nullptr;
	pVM->Exec(argcount, args);
	
	/*
	// tested with test_3d_vecs.tbc
	float vect[3]={ 10.f, 15.f, 20.f };
	TaghaScript pScript = TaghaScript(VM->GetpScript());
	pScript.PushValue((CValue){ .Pointer=vect });
	VM->CallpScriptFunc("vec_invert");
	printf("vect[3]=={ %f , %f, %f }\n", vect[0], vect[1], vect[2]);
	*/
	
	pVM->Delete();
	free(pVM), pVM=nullptr;
}
