
#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include "tagha.hpp"

/*
 * Same as hostapp.c but using C++ code to test portability and compatibility.
*/

/* void print_helloworld(void); */
static void Native_PrintHelloWorld(Tagha_ *pSys, CValue params[], CValue *const pRetval, const uint32_t argc)
{
	puts("Native_PrintHelloWorld :: hello world from bytecode!\n");
}

/* void test(struct player *p); */
static void Native_Test(Tagha_ *pSys, CValue params[], CValue *const pRetval, const uint32_t argc)
{
	struct Player {
		float		speed;
		uint32_t	health;
		uint32_t	ammo;
	} *player=nullptr;
	
	// get first arg which is the virtual address to our data.
	player = reinterpret_cast< Player* >(params[0].Ptr);
	if( !player )
		return;
	
	// debug print to see if our data is accurate.
	printf("Native_Test :: ammo: %" PRIu32 " | health: %" PRIu32 " | speed: %f\n", player->ammo, player->health, player->speed);
	player=nullptr;
}

/* void getglobal(void); */
static void Native_TestGlobal(Tagha_ *pSys, CValue params[], CValue *const pRetval, const uint32_t argc)
{
	int *p = (int *)pSys->GetGlobalByName("i");
	if( !p )
		return;
	
	printf("Native_TestGlobal :: i == %i\n", *p);
}



int main(int argc, char *argv[])
{
	if( !argv[1] ) {
		printf("[Tagha Usage]: '%s' '.tbc file' \n", argv[0]);
		return 1;
	}
	Tagha_ *tagha = new Tagha_();
	NativeInfo_ tagha_host_natives[] = {
		{"test", Native_Test},
		{"printHW", Native_PrintHelloWorld},
		{"getglobal", Native_TestGlobal},
		{nullptr, nullptr}
	};
	tagha->RegisterNatives(tagha_host_natives);
	tagha->LoadScriptByName(argv[1]);
	
	char *args[] = {
		argv[1],
		(char *)"lektus",
		nullptr
	};
	tagha->SetCmdArgs(args);
	tagha->RunScript();
	
	/* // tested with test_3d_vecs.tbc
	float vect[3]={ 10.f, 15.f, 20.f };
	CValue vals[] = { (CValue){ .Ptr=vect } };
	tagha->PushValues(1, vals);
	tagha->CallFunc("VecInvert");
	printf("vect[3]=={ %f , %f, %f }\n", vect[0], vect[1], vect[2]);
	*/
	
	delete tagha;
	tagha=nullptr;
}
