
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
void Native_lldiv(class CTagha *sys, union TaghaVal *retval, const size_t args, union TaghaVal params[])
{
	(void)sys; (void)args; (void)retval; // makes the compiler stop bitching.
	lldiv_t *val = (lldiv_t *)params[0].Ptr;
	*val = lldiv(params[1].Int64, params[2].Int64);
}

/* int puts(const char *str); */
void Native_puts(class CTagha *sys, union TaghaVal *retval, const size_t args, union TaghaVal params[])
{
	(void)sys; (void)args;
	const char *p = (const char *)params[0].Ptr;
	if( !p ) {
		puts("Native_puts :: ERROR **** p is nullptr ****");
		retval->Int32 = -1;
		return;
	}
	retval->Int32 = puts(p);
}

/* char *fgets(char *buffer, int num, FILE *stream); */
void Native_fgets(class CTagha *sys, union TaghaVal *retval, const size_t args, union TaghaVal params[])
{
	(void)sys; (void)args;
	char *buf = (char *)params[0].Ptr;
	if( !buf ) {
		puts("buf is nullptr");
		return;
	}
	FILE *stream = (FILE *)params[2].Ptr;
	if( !stream ) {
		puts("stream is nullptr");
		return;
	}
	retval->Ptr = (void *)fgets(buf, params[1].Int32, stream);
}

/* size_t strlen(const char *s); */
void Native_strlen(class CTagha *sys, union TaghaVal *retval, const size_t args, union TaghaVal params[])
{
	(void)sys; (void)args;
	retval->UInt64 = strlen((const char *)params[0].Ptr);
}

void Native_AddOne(class CTagha *sys, union TaghaVal *retval, const size_t args, union TaghaVal params[])
{
	(void)sys; (void)args;
	retval->Int32 = params[0].Int32 + 1;
}


static size_t GetFileSize(FILE *const __restrict file)
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

int main(int argc, char *argv[])
{
	(void)argc;
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
	fclose(script), script=nullptr;
	
	struct CNativeInfo host_natives[] = {
		{"puts", Native_puts},
		{"fgets", Native_fgets},
		{"strlen", Native_strlen},
		{"AddOne", Native_AddOne},
		{nullptr, nullptr}
	};
	CTagha vm = CTagha(process, host_natives);
	
	struct Player player;
	player.speed = 0.f, player.health = 0, player.ammo = 0;
	struct Player **pp = (struct Player **)vm.GetGlobalVarByName("g_pPlayer");
	if( pp )
		*pp = &player;
	
	char i[] = "hello from main argv!";
	char *arguments[] = {i, nullptr};
	int32_t result = vm.RunScript(1, arguments);
	//union TaghaVal values[1]; values[0].UInt64 = 5;
	//int32_t result = vm.CallFunc("factorial", 1, values);
	if( pp )
		printf("player.speed: '%f' | player.health: '%u' | player.ammo: '%u'\n", player.speed, player.health, player.ammo);
	printf("result?: '%i'\n", result);
}
