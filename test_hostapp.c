
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <time.h>
#include "tagha.h"

struct Player {
	float		speed;
	uint32_t	health, ammo;
};

/* lldiv_t lldiv(long long int numer, long long int denom); */
void Native_lldiv(struct Tagha *sys, union Value *retval, const size_t args, union Value params[static args])
{
	(void)sys;
	(void)retval; // makes the compiler stop bitching.
	lldiv_t *val = params[0].Ptr;
	*val = lldiv(params[1].Int64, params[2].Int64);
}

/* int puts(const char *str); */
void Native_puts(struct Tagha *sys, union Value *retval, const size_t args, union Value params[static args])
{
	(void)sys;
	const char *p = params[0].Ptr;
	if( !p ) {
		retval->Int32 = -1;
		return;
	}
	retval->Int32 = puts(p);
}

/* char *fgets(char *buffer, int num, FILE *stream); */
void Native_fgets(struct Tagha *sys, union Value *retval, const size_t args, union Value params[static args])
{
	(void)sys;
	char *buf = params[0].Ptr;
	if( !buf ) {
		retval->Ptr = NULL;
		return;
	}
	FILE *stream = params[2].Ptr;
	if( !stream ) {
		retval->Ptr = NULL;
		return;
	}
	retval->Ptr = fgets(buf, params[1].Int32, stream);
}

/* size_t strlen(const char *s); */
void Native_strlen(struct Tagha *const restrict sys, union Value *const restrict retval, const size_t args, union Value params[static args])
{
	(void)sys; (void)args;
	const char *s = params[0].Ptr;
	for( ; *s ; s++ );
	retval->UInt64 = (s - (const char *)params[0].Ptr);
}

static size_t GetFileSize(FILE *const restrict file)
{
	int64_t size = 0L;
	if( !file )
		return size;
	
	if( !fseek(file, 0, SEEK_END) ) {
		size = ftell(file);
		if( size == -1 )
			return 0L;
		rewind(file);
	}
	return (size_t)size;
}

int main(int argc, char *argv[static argc])
{
	if( !argv[1] ) {
		printf("[TaghaVM Usage]: '%s' '.tbc filepath' \n", argv[0]);
		return 1;
	}
	
	FILE *script = fopen(argv[1], "rb");
	if( !script )
		return 1;
	
	const size_t filesize = GetFileSize(script);
	uint8_t process[filesize];
	const size_t val = fread(process, sizeof(uint8_t), filesize, script);
	(void)val;
	fclose(script), script=NULL;
	
	const struct NativeInfo host_natives[] = {
		{"puts", Native_puts},
		{"fgets", Native_fgets},
		{"strlen", Native_strlen},
		{NULL, NULL}
	};
	
	struct Tagha vm;
	Tagha_InitN(&vm, process, host_natives);
	
	struct Player player = (struct Player){0};
	// GetGlobalVarByName returns a pointer to the data.
	// if the data itself is a pointer, then you gotta use a pointer-pointer.
	struct Player **pp = Tagha_GetGlobalVarByName(&vm, "g_pPlayer");
	if( pp )
		*pp = &player;
	
	char i[] = "hello from main argv!";
	char *arguments[] = {i, NULL};
	clock_t start = clock();
	int32_t result = Tagha_RunScript(&vm, 1, arguments);
	//int32_t result = Tagha_CallFunc(&vm, "factorial", 1, &(union Value){.UInt64 = 5});
	if( pp )
		printf("player.speed: '%f' | player.health: '%u' | player.ammo: '%u'\n", player.speed, player.health, player.ammo);
	printf("result?: '%i' | profile time: '%f'\n", result, (clock()-start)/(double)CLOCKS_PER_SEC);
}
