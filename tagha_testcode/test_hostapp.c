
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "tagha.h"

struct Player {
	float		speed;
	uint32_t	health, ammo;
};

// lldiv_t lldiv(long long int numer, long long int denom);
void Native_lldiv(struct Tagha *sys, union TaghaVal *restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)sys; (void)retval; // makes the compiler stop bitching.
	lldiv_t *val = params[0].Ptr;
	*val = lldiv(params[1].Int64, params[2].Int64);
}

// int puts(const char *str);
void Native_puts(struct Tagha *const restrict sys, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)sys;
	const char *restrict p = params[0].Ptr;
	retval->Int32 = p != NULL ? puts(p) : -1;
}

// char *fgets(char *buffer, int num, FILE *stream);
void Native_fgets(struct Tagha *const restrict sys, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)sys;
	char *restrict buf = params[0].Ptr;
	FILE *restrict stream = params[2].Ptr;
	if( !buf || !stream ) {
		return;
	}
	retval->Ptr = fgets(buf, params[1].Int32, stream);
}

// size_t strlen(const char *s);
void Native_strlen(struct Tagha *const restrict sys, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)sys; (void)args;
	const char *restrict s = params[0].Ptr;
	for( ; *s ; s++ );
	retval->UInt64 = (s - (const char *)params[0].Ptr);
}

void Native_AddOne(struct Tagha *const sys, union TaghaVal *const restrict retval, const size_t args, union TaghaVal params[restrict static args])
{
	(void)sys; (void)args;
	retval->Int32 = params[0].Int32 + 1;
}

struct Tagha *MakeScriptByName(const char *restrict filename, const struct NativeInfo natives[restrict])
{
	FILE *script = fopen(filename, "rb");
	if( !script )
		return NULL;
	
	fseek(script, 0, SEEK_END);
	const int64_t filesize = ftell(script);
	if( filesize <= -1 ) {
		fclose(script);
		return NULL;
	}
	rewind(script);
	
	uint8_t *restrict process = calloc(filesize, sizeof *process);
	if( fread(process, sizeof *process, filesize, script) != (size_t)filesize ) {
		free(process), process=NULL;
		fclose(script), script=NULL;
		return NULL;
	} else {
		fclose(script), script=NULL;
		return Tagha_NewNatives(process, natives);
	}
}


#include "tagha_libc/libtagha.c"

int main(const int argc, char *argv[restrict static argc+1])
{
	if( !argv[1] ) {
		printf("[TaghaVM Usage]: '%s' '.tbc filepath' \n", argv[0]);
		return 1;
	}
	
	const struct NativeInfo host_natives[] = {
		{"puts", Native_puts},
		{"fgets", Native_fgets},
		{"strlen", Native_strlen},
		{"AddOne", Native_AddOne},
		{NULL, NULL}
	};
	
	struct Tagha *vm = MakeScriptByName(argv[1], host_natives);
	if( vm ) {
		Tagha_LoadlibTaghaNatives(vm); // from tagha_libc/libtagha.c
		
		struct Player player = (struct Player){0};
		// GetGlobalVarByName returns a pointer to the data.
		// if the data itself is a pointer, then you gotta use a pointer-pointer.
		struct Player **pp = Tagha_GetGlobalVarByName(vm, "g_pPlayer");
		if( pp )
			*pp = &player;
		
		char argv1[] = "hello from main argv!";
		char *arguments[] = {argv1, NULL};
		const clock_t start = clock();
		//int32_t result = Tagha_CallFunc(vm, "factorial", 1, &(union TaghaVal){.UInt64 = 5});
		const int32_t result = Tagha_RunScript(vm, 1, arguments);
		if( pp )
			printf("player.speed: '%f' | player.health: '%u' | player.ammo: '%u'\n", player.speed, player.health, player.ammo);
		Tagha_PrintVMState(vm);
		printf("Tagha Error ?: '%s'\n", Tagha_GetError(vm));
		printf("result?: '%i' | profile time: '%f'\n", result, (clock()-start)/(double)CLOCKS_PER_SEC);
		void *restrict mem = Tagha_GetRawScriptPtr(vm);
		free(mem), mem=NULL;
		Tagha_Free(&vm);
	}
}
