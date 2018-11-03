#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tagha.h"

static void			*GetFunctionOffsetByName(uint8_t *, const char *);
static void			*GetFunctionOffsetByIndex(uint8_t *, size_t);
static TaghaNative		*GetNativeByIndex(uint8_t *, size_t);

static void			*GetVariableOffsetByName(uint8_t *, const char *);
static void			*GetVariableOffsetByIndex(uint8_t *, size_t);
//static bool			VerifyTaghaScript(uint8_t *);

static void PrepModule(uint8_t *const module)
{
	union TaghaPtr reader = {module};
	reader.PtrUInt8 += 10;
	const uint32_t vartable_offset = *reader.PtrUInt32;
	
	reader.PtrUInt8 = module + vartable_offset;
	const uint32_t globalvars = *reader.PtrUInt32++;
	for( uint32_t i=0 ; i<globalvars ; i++ ) {
		reader.PtrUInt8++;
		const uint64_t sizes = *reader.PtrUInt64++;
		const uint32_t cstrlen = sizes & 0xffFFffFF;
		const uint32_t datalen = sizes >> 32;
		if( !strcmp("stdin", reader.PtrCStr) ) {
			FILE **fileptr = (FILE **)(reader.PtrUInt8 + cstrlen);
			*fileptr = stdin;
		}
		else if( !strcmp("stdout", reader.PtrCStr) ) {
			FILE **fileptr = (FILE **)(reader.PtrUInt8 + cstrlen);
			*fileptr = stdout;
		}
		else if( !strcmp("stderr", reader.PtrCStr) ) {
			FILE **fileptr = (FILE **)(reader.PtrUInt8 + cstrlen);
			*fileptr = stderr;
		}
		reader.PtrUInt8 += (cstrlen + datalen);
	}
}


struct Tagha *Tagha_New(void *restrict script)
{
	struct Tagha *restrict vm = calloc(1, sizeof *vm);
	Tagha_Init(vm, script);
	return vm;
}

struct Tagha *Tagha_NewNatives(void *restrict script, const struct NativeInfo natives[restrict])
{
	struct Tagha *restrict vm = Tagha_New(script);
	Tagha_RegisterNatives(vm, natives);
	return vm;
}

void Tagha_Free(struct Tagha **vmref)
{
	if( !vmref || !*vmref )
		return;
	free(*vmref), *vmref = NULL;
}

void Tagha_Init(struct Tagha *const restrict vm, void *script)
{
	if( !vm || !script )
		return;
	
	*vm = (struct Tagha){0};
	PrepModule(script);
	vm->Header = script;
	vm->SafeMode = vm->Header[18];
}

void Tagha_InitNatives(struct Tagha *const restrict vm, void *restrict script, const struct NativeInfo natives[restrict])
{
	Tagha_Init(vm, script);
	Tagha_RegisterNatives(vm, natives);
}



void Tagha_PrintVMState(const struct Tagha *const vm)
{
	if( !vm )
		return;
	
	printf("=== Tagha VM State Info ===\n\nPrinting Registers:\nregister alaf: '%" PRIu64 "'\nregister beth: '%" PRIu64 "'\nregister gamal: '%" PRIu64 "'\nregister dalath: '%" PRIu64 "'\nregister heh: '%" PRIu64 "'\nregister waw: '%" PRIu64 "'\nregister zain: '%" PRIu64 "'\nregister heth: '%" PRIu64 "'\nregister teth: '%" PRIu64 "'\nregister yodh: '%" PRIu64 "'\nregister kaf: '%" PRIu64 "'\nregister lamadh: '%" PRIu64 "'\nregister meem: '%" PRIu64 "'\nregister noon: '%" PRIu64 "'\nregister semkath: '%" PRIu64 "'\nregister eh: '%" PRIu64 "'\nregister peh: '%" PRIu64 "'\nregister sadhe: '%" PRIu64 "'\nregister qof: '%" PRIu64 "'\nregister reesh: '%" PRIu64 "'\nregister sheen: '%" PRIu64 "'\nregister taw: '%" PRIu64 "'\nregister stack pointer: '%p'\nregister base pointer: '%p'\nregister instruction pointer: '%p'\n\nPrinting Condition Flag: %u\n=== End Tagha VM State Info ===\n",
	vm->regAlaf.UInt64,
	vm->regBeth.UInt64,
	vm->regGamal.UInt64,
	vm->regDalath.UInt64,
	vm->regHeh.UInt64,
	vm->regWaw.UInt64,
	vm->regZain.UInt64,
	vm->regHeth.UInt64,
	vm->regTeth.UInt64,
	vm->regYodh.UInt64,
	vm->regKaf.UInt64,
	vm->regLamadh.UInt64,
	vm->regMeem.UInt64,
	vm->regNoon.UInt64,
	vm->regSemkath.UInt64,
	vm->reg_Eh.UInt64,
	vm->regPeh.UInt64,
	vm->regSadhe.UInt64,
	vm->regQof.UInt64,
	vm->regReesh.UInt64,
	vm->regSheen.UInt64,
	vm->regTaw.UInt64,
	vm->regStk.Ptr,
	vm->regBase.Ptr,
	vm->regInstr.Ptr,
	vm->CondFlag);
}

bool Tagha_RegisterNatives(struct Tagha *const restrict vm, const struct NativeInfo natives[])
{
	if( !vm || !vm->Header || !natives )
		return false;
	
	for( const struct NativeInfo *restrict n=natives ; n->NativeCFunc && n->Name ; n++ ) {
		TaghaNative **nativeref = GetFunctionOffsetByName(vm->Header, n->Name);
		if( nativeref )
			*nativeref = n->NativeCFunc;
	}
	return true;
}

static void *GetFunctionOffsetByName(uint8_t *const hdr, const char *restrict funcname)
{
	union TaghaPtr reader = {hdr + sizeof(struct TaghaHeader)};
	const uint32_t funcs = *reader.PtrUInt32++;
	for( uint32_t i=0 ; i<funcs ; i++ ) {
		reader.PtrUInt8++;
		const uint64_t sizes = *reader.PtrUInt64++;
		const uint32_t cstrlen = sizes & 0xffFFffFF;
		const uint32_t datalen = sizes >> 32;
		if( !strcmp(funcname, reader.PtrCStr) )
			return reader.PtrUInt8 + cstrlen;
		else reader.PtrUInt8 += (cstrlen + datalen);
	}
	return NULL;
}

static void *GetFunctionOffsetByIndex(uint8_t *const hdr, const size_t index)
{
	union TaghaPtr reader = {hdr + sizeof(struct TaghaHeader)};
	const uint32_t funcs = *reader.PtrUInt32++;
	if( index >= funcs )
		return NULL;
	
	for( uint32_t i=0 ; i<funcs ; i++ ) {
		reader.PtrUInt8++;
		const uint64_t sizes = *reader.PtrUInt64++;
		const uint32_t cstrlen = sizes & 0xffFFffFF;
		const uint32_t datalen = sizes >> 32;
		if( i==index )
			return reader.PtrUInt8 + cstrlen;
		else reader.PtrUInt8 += (cstrlen + datalen);
	}
	return NULL;
}

static TaghaNative *GetNativeByIndex(uint8_t *const hdr, const size_t index)
{
	union TaghaPtr reader = {hdr + sizeof(struct TaghaHeader)};
	const uint32_t funcs = *reader.PtrUInt32++;
	if( index >= funcs )
		return NULL;
	
	for( uint32_t i=0 ; i<funcs ; i++ ) {
		reader.PtrUInt8++;
		const uint64_t sizes = *reader.PtrUInt64++;
		const uint32_t cstrlen = sizes & 0xffFFffFF;
		const uint32_t datalen = sizes >> 32;
		if( i==index )
			return *(TaghaNative **)(reader.PtrUInt8 + cstrlen);
		else reader.PtrUInt8 += (cstrlen + datalen);
	}
	return NULL;
}

static void *GetVariableOffsetByName(uint8_t *const module, const char *restrict varname)
{
	union TaghaPtr reader = {module};
	reader.PtrUInt8 += 10;
	const uint32_t vartable_offset = *reader.PtrUInt32;
	
	reader.PtrUInt8 = module + vartable_offset;
	const uint32_t globalvars = *reader.PtrUInt32++;
	for( uint32_t i=0 ; i<globalvars ; i++ ) {
		reader.PtrUInt8++;
		const uint64_t sizes = *reader.PtrUInt64++;
		const uint32_t cstrlen = sizes & 0xffFFffFF;
		const uint32_t datalen = sizes >> 32;
		if( !strcmp(varname, reader.PtrCStr) )
			return reader.PtrUInt8 + cstrlen;
		else reader.PtrUInt8 += (cstrlen + datalen);
	}
	return NULL;
}

static void *GetVariableOffsetByIndex(uint8_t *const module, const size_t index)
{
	union TaghaPtr reader = {module};
	reader.PtrUInt8 += 10;
	const uint32_t vartable_offset = *reader.PtrUInt32;
	
	reader.PtrUInt8 = module + vartable_offset;
	const uint32_t globalvars = *reader.PtrUInt32++;
	if( index >= globalvars )
		return NULL;
	
	for( uint32_t i=0 ; i<globalvars ; i++ ) {
		reader.PtrUInt8++;
		const uint64_t sizes = *reader.PtrUInt64++;
		const uint32_t cstrlen = sizes & 0xffFFffFF;
		const uint32_t datalen = sizes >> 32;
		if( i==index )
			return reader.PtrUInt8 + cstrlen;
		else reader.PtrUInt8 += (cstrlen + datalen);
	}
	return NULL;
}

/*
#include <sys/mman.h>
void *Tagha_JITCompileFunc(const size_t len, uint8_t code[restrict static len])
{
	const uint8_t map_anon = 0x20; // giving undefined symbol for MAP_ANON and ANONYMOUS.
	uint8_t *mem = mmap(NULL, len, PROT_READ | PROT_WRITE, map_anon | MAP_PRIVATE, -1, 0);
	if( !mem )
		return NULL;
	
	memcpy(mem, code, len);
	mprotect(mem, len, PROT_READ | PROT_EXEC);
	return mem;
}

munmap(mem, len);
*/

//#include <unistd.h>	// sleep() func

int32_t Tagha_Exec(struct Tagha *const restrict vm)
{
	if( !vm )
		return -1;
	
	union TaghaPtr pc = {vm->regInstr.PtrUInt8};
	
#define X(x) #x ,
	/* for debugging purposes. */
	//const char *const restrict opcode2str[] = { INSTR_SET };
#undef X
	
#define X(x) &&exec_##x ,
	/* our instruction dispatch table. */
	static const void *const restrict dispatch[] = { INSTR_SET };
#undef X
#undef INSTR_SET
	
	#define oldDISPATCH() \
		const uint8_t instr = *pc.PtrUInt8++; \
		\
		if( instr>nop ) { \
			vm->Error = ErrInstrBounds; \
			return -1; \
		} \
		\
		/*usleep(100);*/ \
		/*printf("dispatching to '%s'\n", opcode2str[instr]);*/ \
		/*Tagha_PrintVMState(vm);*/ \
		goto *dispatch[instr]
	
	#define DISPATCH() goto *dispatch[*pc.PtrUInt8++]
	
	exec_nop: {
		DISPATCH();
	}
	/* push immediate value. */
	exec_pushi: { /* char: opcode | i64: imm */
		*--vm->regStk.PtrSelf = *pc.PtrVal++;
		DISPATCH();
	}
	/* push a register's contents. */
	exec_push: { /* char: opcode | char: regid */
		*--vm->regStk.PtrSelf = vm->Regs[*pc.PtrUInt8++];
		DISPATCH();
	}
	
	/* pops a value from the stack into a register then reduces stack by 8 bytes. */
	exec_pop: { /* char: opcode | char: regid */
		vm->Regs[*pc.PtrUInt8++] = *vm->regStk.PtrSelf++;
		DISPATCH();
	}
	
	exec_loadglobal: { /* char: opcode | char: regid | u64: index */
		const uint8_t regid = *pc.PtrUInt8++;
		vm->Regs[regid].Ptr = GetVariableOffsetByIndex(vm->Header, *pc.PtrUInt64++);
		DISPATCH();
	}
	/* loads a function index which could be a native */
	exec_loadfunc: { /* char: opcode | char: regid | i64: index */
		const uint8_t regid = *pc.PtrUInt8++;
		//vm->Regs[regid].Int64 = *pc.PtrInt64++;
		memcpy(&vm->Regs[regid].Int64, pc.PtrInt64++, sizeof(int64_t));
		DISPATCH();
	}
	exec_loadaddr: { /* char: opcode | char: regid1 | char: regid2 | i32: offset */
		const uint16_t regids = *pc.PtrUInt16++;
		vm->Regs[regids & 0xff].PtrUInt8 = vm->Regs[regids >> 8].PtrUInt8 + *pc.PtrInt32++;
		DISPATCH();
	}
	
	exec_movi: { /* char: opcode | char: regid | i64: imm */
		const uint8_t regid = *pc.PtrUInt8++;
		//vm->Regs[regid] = *pc.PtrVal++;
		memcpy(&vm->Regs[regid], pc.PtrVal++, sizeof(union TaghaVal));
		DISPATCH();
	}
	exec_mov: { /* char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.PtrUInt16++;
		//vm->Regs[regids & 0xff] = vm->Regs[regids >> 8];
		memcpy(&vm->Regs[regids & 0xff], &vm->Regs[regids >> 8], sizeof(union TaghaVal));
		DISPATCH();
	}
	
	exec_ld1: { /* char: opcode | char: dest reg | char: src reg | i32: offset */
		const uint16_t regids = *pc.PtrUInt16++;
		const union TaghaPtr mem = {vm->Regs[regids >> 8].PtrUInt8 + *pc.PtrInt32++};
		// do a memcheck here.
		if( vm->SafeMode && (mem.PtrUInt8 < vm->DataBase || mem.PtrUInt8 >= vm->Footer) ) {
			vm->Error = ErrBadPtr;
			return -1;
		}
		vm->Regs[regids & 0xff].UInt64 = (uint64_t) *mem.PtrUInt8;
		DISPATCH();
	}
	exec_ld2: { /* char: opcode | char: dest reg | char: src reg | i32: offset */
		const uint16_t regids = *pc.PtrUInt16++;
		const union TaghaPtr mem = {vm->Regs[regids >> 8].PtrUInt8 + *pc.PtrInt32++};
		// do a memcheck here.
		if( vm->SafeMode && (mem.PtrUInt8 < vm->DataBase || mem.PtrUInt8+1 >= vm->Footer) ) {
			vm->Error = ErrBadPtr;
			return -1;
		}
		vm->Regs[regids & 0xff].UInt64 = (uint64_t) *mem.PtrUInt16;
		DISPATCH();
	}
	exec_ld4: { /* char: opcode | char: dest reg | char: src reg | i32: offset */
		const uint16_t regids = *pc.PtrUInt16++;
		const union TaghaPtr mem = {vm->Regs[regids >> 8].PtrUInt8 + *pc.PtrInt32++};
		// do a memcheck here.
		if( vm->SafeMode && (mem.PtrUInt8 < vm->DataBase || mem.PtrUInt8+3 >= vm->Footer) ) {
			vm->Error = ErrBadPtr;
			return -1;
		}
		vm->Regs[regids & 0xff].UInt64 = (uint64_t) *mem.PtrUInt32;
		DISPATCH();
	}
	exec_ld8: { /* char: opcode | char: dest reg | char: src reg | i32: offset */
		const uint16_t regids = *pc.PtrUInt16++;
		const union TaghaPtr mem = {vm->Regs[regids >> 8].PtrUInt8 + *pc.PtrInt32++};
		// do a memcheck here.
		if( vm->SafeMode && (mem.PtrUInt8 < vm->DataBase || mem.PtrUInt8+7 >= vm->Footer) ) {
			vm->Error = ErrBadPtr;
			return -1;
		}
		vm->Regs[regids & 0xff].UInt64 = *mem.PtrUInt64;
		DISPATCH();
	}
	
	exec_st1: { /* char: opcode | char: dest reg | char: src reg | i32: offset */
		const uint16_t regids = *pc.PtrUInt16++;
		const union TaghaPtr mem = {vm->Regs[regids & 0xff].PtrUInt8 + *pc.PtrInt32++};
		// do a memcheck here.
		if( vm->SafeMode && (mem.PtrUInt8 < vm->DataBase || mem.PtrUInt8 >= vm->Footer) ) {
			vm->Error = ErrBadPtr;
			return -1;
		}
		memcpy(mem.Ptr, &vm->Regs[regids >> 8], sizeof(uint8_t));
		//*mem.PtrUInt8 = vm->Regs[regids >> 8].UInt8;
		DISPATCH();
	}
	exec_st2: { /* char: opcode | char: dest reg | char: src reg | i32: offset */
		const uint16_t regids = *pc.PtrUInt16++;
		const union TaghaPtr mem = {vm->Regs[regids & 0xff].PtrUInt8 + *pc.PtrInt32++};
		// do a memcheck here.
		if( vm->SafeMode && (mem.PtrUInt8 < vm->DataBase || mem.PtrUInt8+1 >= vm->Footer) ) {
			vm->Error = ErrBadPtr;
			return -1;
		}
		memcpy(mem.Ptr, &vm->Regs[regids >> 8], sizeof(uint16_t));
		//*mem.PtrUInt16 = vm->Regs[regids >> 8].UInt16;
		DISPATCH();
	}
	exec_st4: { /* char: opcode | char: dest reg | char: src reg | i32: offset */
		const uint16_t regids = *pc.PtrUInt16++;
		const union TaghaPtr mem = {vm->Regs[regids & 0xff].PtrUInt8 + *pc.PtrInt32++};
		// do a memcheck here.
		if( vm->SafeMode && (mem.PtrUInt8 < vm->DataBase || mem.PtrUInt8+3 >= vm->Footer) ) {
			vm->Error = ErrBadPtr;
			return -1;
		}
		memcpy(mem.Ptr, &vm->Regs[regids >> 8], sizeof(uint32_t));
		//*mem.PtrUInt32 = vm->Regs[regids >> 8].UInt32;
		DISPATCH();
	}
	exec_st8: { /* char: opcode | char: dest reg | char: src reg | i32: offset */
		const uint16_t regids = *pc.PtrUInt16++;
		const union TaghaPtr mem = {vm->Regs[regids & 0xff].PtrUInt8 + *pc.PtrInt32++};
		// do a memcheck here.
		if( vm->SafeMode && (mem.PtrUInt8 < vm->DataBase || mem.PtrUInt8+7 >= vm->Footer) ) {
			vm->Error = ErrBadPtr;
			return -1;
		}
		memcpy(mem.Ptr, &vm->Regs[regids >> 8], sizeof(uint64_t));
		//*mem.PtrUInt64 = vm->Regs[regids >> 8].UInt64;
		DISPATCH();
	}
	
	exec_add: { /* char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.PtrUInt16++;
		vm->Regs[regids & 0xff].Int64 += vm->Regs[regids >> 8].Int64;
		DISPATCH();
	}
	exec_sub: { /* char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.PtrUInt16++;
		vm->Regs[regids & 0xff].Int64 -= vm->Regs[regids >> 8].Int64;
		DISPATCH();
	}
	exec_mul: { /* char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.PtrUInt16++;
		vm->Regs[regids & 0xff].Int64 *= vm->Regs[regids >> 8].Int64;
		DISPATCH();
	}
	exec_divi: { /* char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.PtrUInt16++;
		vm->Regs[regids & 0xff].Int64 /= vm->Regs[regids >> 8].Int64;
		DISPATCH();
	}
	exec_mod: { /* char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.PtrUInt16++;
		vm->Regs[regids & 0xff].Int64 %= vm->Regs[regids >> 8].Int64;
		DISPATCH();
	}
	exec_bit_and: { /* char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.PtrUInt16++;
		vm->Regs[regids & 0xff].UInt64 &= vm->Regs[regids >> 8].UInt64;
		DISPATCH();
	}
	exec_bit_or: { /* char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.PtrUInt16++;
		vm->Regs[regids & 0xff].UInt64 |= vm->Regs[regids >> 8].UInt64;
		DISPATCH();
	}
	exec_bit_xor: { /* char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.PtrUInt16++;
		vm->Regs[regids & 0xff].UInt64 ^= vm->Regs[regids >> 8].UInt64;
		DISPATCH();
	}
	exec_shl: { /* char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.PtrUInt16++;
		vm->Regs[regids & 0xff].UInt64 <<= vm->Regs[regids >> 8].UInt64;
		DISPATCH();
	}
	exec_shr: { /* char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.PtrUInt16++;
		vm->Regs[regids & 0xff].UInt64 >>= vm->Regs[regids >> 8].UInt64;
		DISPATCH();
	}
	exec_bit_not: { /* char: opcode | char: regid */
		const uint8_t regid = *pc.PtrUInt8++;
		vm->Regs[regid].UInt64 = ~vm->Regs[regid].UInt64;
		DISPATCH();
	}
	exec_inc: { /* char: opcode | char: regid */
		const uint8_t regid = *pc.PtrUInt8++;
		++vm->Regs[regid].UInt64;
		DISPATCH();
	}
	exec_dec: { /* char: opcode | char: regid */
		const uint8_t regid = *pc.PtrUInt8++;
		--vm->Regs[regid].UInt64;
		DISPATCH();
	}
	exec_neg: { /* char: opcode | char: regid */
		const uint8_t regid = *pc.PtrUInt8++;
		vm->Regs[regid].Int64 = -vm->Regs[regid].Int64;
		DISPATCH();
	}
	
	exec_ilt: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.PtrUInt16++;
		vm->CondFlag = vm->Regs[regids & 0xff].Int64 < vm->Regs[regids >> 8].Int64;
		DISPATCH();
	}
	exec_ile: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.PtrUInt16++;
		vm->CondFlag = vm->Regs[regids & 0xff].Int64 <= vm->Regs[regids >> 8].Int64;
		DISPATCH();
	}
	
	exec_igt: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.PtrUInt16++;
		vm->CondFlag = vm->Regs[regids & 0xff].Int64 > vm->Regs[regids >> 8].Int64;
		DISPATCH();
	}
	exec_ige: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.PtrUInt16++;
		vm->CondFlag = vm->Regs[regids & 0xff].Int64 >= vm->Regs[regids >> 8].Int64;
		DISPATCH();
	}
	
	exec_ult: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.PtrUInt16++;
		vm->CondFlag = vm->Regs[regids & 0xff].UInt64 < vm->Regs[regids >> 8].UInt64;
		DISPATCH();
	}
	exec_ule: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.PtrUInt16++;
		vm->CondFlag = vm->Regs[regids & 0xff].UInt64 <= vm->Regs[regids >> 8].UInt64;
		DISPATCH();
	}
	
	exec_ugt: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.PtrUInt16++;
		vm->CondFlag = vm->Regs[regids & 0xff].UInt64 > vm->Regs[regids >> 8].UInt64;
		DISPATCH();
	}
	exec_uge: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.PtrUInt16++;
		vm->CondFlag = vm->Regs[regids & 0xff].UInt64 >= vm->Regs[regids >> 8].UInt64;
		DISPATCH();
	}
	
	exec_cmp: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.PtrUInt16++;
		vm->CondFlag = vm->Regs[regids & 0xff].UInt64 == vm->Regs[regids >> 8].UInt64;
		DISPATCH();
	}
	exec_neq: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.PtrUInt16++;
		vm->CondFlag = vm->Regs[regids & 0xff].UInt64 != vm->Regs[regids >> 8].UInt64;
		DISPATCH();
	}
	exec_jmp: { /* char: opcode | i64: offset */
		const int64_t offset = *pc.PtrInt64++;
		pc.PtrUInt8 += offset;
		DISPATCH();
	}
	exec_jz: { /* char: opcode | i64: offset */
		const int64_t offset = *pc.PtrInt64++;
		//if( !vm->CondFlag )
		//	pc.PtrUInt8 += offset;
		pc.PtrUInt8 += ( !vm->CondFlag ) ? offset : 0;
		DISPATCH();
	}
	exec_jnz: { /* char: opcode | i64: offset */
		const int64_t offset = *pc.PtrInt64++;
		//if( vm->CondFlag )
		//	pc.PtrUInt8 += offset;
		pc.PtrUInt8 += ( vm->CondFlag ) ? offset : 0;
		DISPATCH();
	}
	exec_call: { /* char: opcode | i64: offset */
		/* The restrict type qualifier is an indication to the compiler that,
		 * if the memory addressed by the restrict-qualified pointer is modified,
		 * no other pointer will access that same memory.
		 * Since we're pushing the restrict-qualified pointer's memory that it points to,
		 * This is NOT undefined behavior because it's not aliasing access of the instruction stream.
		 */
		const int64_t index = *pc.PtrInt64++;
		(--vm->regStk.PtrSelf)->Ptr = pc.Ptr;	/* push rip */
		*--vm->regStk.PtrSelf = vm->regBase;	/* push rbp */
		vm->regBase = vm->regStk;	/* mov rbp, rsp */
		pc.PtrUInt8 = GetFunctionOffsetByIndex(vm->Header, index - 1);
		DISPATCH();
	}
	exec_callr: { /* char: opcode | char: regid */
		const uint8_t regid = *pc.PtrUInt8++;
		(--vm->regStk.PtrSelf)->Ptr = pc.Ptr;	/* push rip */
		*--vm->regStk.PtrSelf = vm->regBase;	/* push rbp */
		vm->regBase = vm->regStk;	/* mov rbp, rsp */
		pc.PtrUInt8 = GetFunctionOffsetByIndex(vm->Header, (vm->Regs[regid].Int64 - 1));
		DISPATCH();
	}
	exec_ret: { /* char: opcode */
		vm->regStk = vm->regBase; /* mov rsp, rbp */
		vm->regBase = *vm->regStk.PtrSelf++; /* pop rbp */
		pc.Ptr = (*vm->regStk.PtrSelf++).Ptr; /* pop rip */
		if( !pc.Ptr )
			return vm->regAlaf.Int32;
		else { DISPATCH(); }
	}
	
	exec_syscall: { /* char: opcode | i64: index */
		TaghaNative *const nativeref = GetNativeByIndex(vm->Header, (-1 - *pc.PtrInt64++));
		if( !nativeref ) {
			vm->Error = ErrMissingNative;
			goto *dispatch[halt];
		} else {
			const uint8_t reg_param_initial = regSemkath;
			const uint8_t reg_params = regTaw - regSemkath + 1;
			union TaghaVal retval = (union TaghaVal){0};
			
			/* save stack space by using the registers for passing arguments. */
			/* the other registers can then be used for other data operations. */
			( vm->regAlaf.SizeInt <= reg_params ) ?
				(*nativeref)(vm, &retval, vm->regAlaf.SizeInt, vm->Regs+reg_param_initial) :
				/* if the native call has more than a certain num of params, get all params from stack. */
				((*nativeref)(vm, &retval, vm->regAlaf.SizeInt, vm->regStk.PtrSelf), vm->regStk.PtrSelf += vm->regAlaf.SizeInt);
			memcpy(&vm->regAlaf, &retval, sizeof retval);
			
			if( vm->Error != ErrNone )
				return vm->Error;
			else { DISPATCH(); }
		}
	}
	exec_syscallr: { /* char: opcode | char: reg id */
		const uint8_t regid = *pc.PtrUInt8++;
		TaghaNative *const nativeref = GetNativeByIndex(vm->Header, (-1 - vm->Regs[regid].Int64));
		if( !nativeref ) {
			vm->Error = ErrMissingNative;
			goto *dispatch[halt];
		} else {
			const uint8_t reg_param_initial = regSemkath;
			const uint8_t reg_params = regTaw - regSemkath + 1;
			union TaghaVal retval = (union TaghaVal){0};
			
			/* save stack space by using the registers for passing arguments. */
			/* the other registers can then be used for other data operations. */
			( vm->regAlaf.SizeInt <= reg_params ) ?
				(*nativeref)(vm, &retval, vm->regAlaf.SizeInt, vm->Regs+reg_param_initial) :
				/* if the native call has more than a certain num of params, get all params from stack. */
				((*nativeref)(vm, &retval, vm->regAlaf.SizeInt, vm->regStk.PtrSelf), vm->regStk.PtrSelf += vm->regAlaf.SizeInt);
			memcpy(&vm->regAlaf, &retval, sizeof retval);
			
			if( vm->Error != ErrNone )
				return vm->Error;
			else { DISPATCH(); }
		}
	}
	
#ifdef FLOATING_POINT_OPS
	exec_flt2dbl: { /* treat as no op if one float is defined but not the other. */
	 /* char: opcode | char: reg id */
		const uint8_t regid = *pc.PtrUInt8++;
	#if defined(__TAGHA_FLOAT32_DEFINED) && defined(__TAGHA_FLOAT64_DEFINED)
		const float f = vm->Regs[regid].Float;
		vm->Regs[regid].Double = (double)f;
	#endif
		DISPATCH();
	}
	exec_dbl2flt: { /* char: opcode | char: reg id */
		const uint8_t regid = *pc.PtrUInt8++;
	#if defined(__TAGHA_FLOAT32_DEFINED) && defined(__TAGHA_FLOAT64_DEFINED)
		const double d = vm->Regs[regid].Double;
		vm->Regs[regid].Int64 = 0;
		vm->Regs[regid].Float = (float)d;
	#endif
		DISPATCH();
	}
	exec_int2dbl: { /* char: opcode | char: reg id */
		const uint8_t regid = *pc.PtrUInt8++;
	#ifdef __TAGHA_FLOAT64_DEFINED
		const int64_t i = vm->Regs[regid].Int64;
		vm->Regs[regid].Double = (double)i;
	#endif
		DISPATCH();
	}
	exec_int2flt: { /* char: opcode | char: reg id */
		const uint8_t regid = *pc.PtrUInt8++;
	#ifdef __TAGHA_FLOAT32_DEFINED
		const int64_t i = vm->Regs[regid].Int64;
		vm->Regs[regid].Int64 = 0;
		vm->Regs[regid].Float = (float)i;
	#endif
		DISPATCH();
	}
	
	exec_addf: { /* char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.PtrUInt16++;
	#if defined(__TAGHA_FLOAT64_DEFINED) /* if doubles are defined, regardless whether float is or not */
		vm->Regs[regids & 0xff].Double += vm->Regs[regids >> 8].Double;
	#elif defined(__TAGHA_FLOAT32_DEFINED)
		vm->Regs[regids & 0xff].Float += vm->Regs[regids >> 8].Float;
	#endif
		DISPATCH();
	}
	exec_subf: { /* char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.PtrUInt16++;
	#if defined(__TAGHA_FLOAT64_DEFINED)
		vm->Regs[regids & 0xff].Double -= vm->Regs[regids >> 8].Double;
	#elif defined(__TAGHA_FLOAT32_DEFINED)
		vm->Regs[regids & 0xff].Float -= vm->Regs[regids >> 8].Float;
	#endif
		DISPATCH();
	}
	exec_mulf: { /* char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.PtrUInt16++;
	#if defined(__TAGHA_FLOAT64_DEFINED)
		vm->Regs[regids & 0xff].Double *= vm->Regs[regids >> 8].Double;
	#elif defined(__TAGHA_FLOAT32_DEFINED)
		vm->Regs[regids & 0xff].Float *= vm->Regs[regids >> 8].Float;
	#endif
		DISPATCH();
	}
	exec_divf: { /* char: opcode | char: dest reg | char: src reg */
		const uint16_t regids = *pc.PtrUInt16++;
	#if defined(__TAGHA_FLOAT64_DEFINED)
		vm->Regs[regids & 0xff].Double /= vm->Regs[regids >> 8].Double;
	#elif defined(__TAGHA_FLOAT32_DEFINED)
		vm->Regs[regids & 0xff].Float /= vm->Regs[regids >> 8].Float;
	#endif
		DISPATCH();
	}
	
	exec_ltf: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.PtrUInt16++;
	#if defined(__TAGHA_FLOAT64_DEFINED)
		vm->CondFlag = vm->Regs[regids & 0xff].Double < vm->Regs[regids >> 8].Double;
	#elif defined(__TAGHA_FLOAT32_DEFINED)
		vm->CondFlag = vm->Regs[regids & 0xff].Float < vm->Regs[regids >> 8].Float;
	#endif
		DISPATCH();
	}
	exec_lef: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.PtrUInt16++;
	#if defined(__TAGHA_FLOAT64_DEFINED)
		vm->CondFlag = vm->Regs[regids & 0xff].Double <= vm->Regs[regids >> 8].Double;
	#elif defined(__TAGHA_FLOAT32_DEFINED)
		vm->CondFlag = vm->Regs[regids & 0xff].Float <= vm->Regs[regids >> 8].Float;
	#endif
		DISPATCH();
	}
	
	exec_gtf: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.PtrUInt16++;
	#if defined(__TAGHA_FLOAT64_DEFINED)
		vm->CondFlag = vm->Regs[regids & 0xff].Double > vm->Regs[regids >> 8].Double;
	#elif defined(__TAGHA_FLOAT32_DEFINED)
		vm->CondFlag = vm->Regs[regids & 0xff].Float > vm->Regs[regids >> 8].Float;
	#endif
		DISPATCH();
	}
	exec_gef: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.PtrUInt16++;
	#if defined(__TAGHA_FLOAT64_DEFINED)
		vm->CondFlag = vm->Regs[regids & 0xff].Double >= vm->Regs[regids >> 8].Double;
	#elif defined(__TAGHA_FLOAT32_DEFINED)
		vm->CondFlag = vm->Regs[regids & 0xff].Float >= vm->Regs[regids >> 8].Float;
	#endif
		DISPATCH();
	}
	
	exec_cmpf: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.PtrUInt16++;
	#if defined(__TAGHA_FLOAT64_DEFINED)
		vm->CondFlag = vm->Regs[regids & 0xff].Double == vm->Regs[regids >> 8].Double;
	#elif defined(__TAGHA_FLOAT32_DEFINED)
		vm->CondFlag = vm->Regs[regids & 0xff].Float == vm->Regs[regids >> 8].Float;
	#endif
		DISPATCH();
	}
	exec_neqf: { /* char: opcode | char: reg 1 | char: reg 2 */
		const uint16_t regids = *pc.PtrUInt16++;
	#if defined(__TAGHA_FLOAT64_DEFINED)
		vm->CondFlag = vm->Regs[regids & 0xff].Double != vm->Regs[regids >> 8].Double;
	#elif defined(__TAGHA_FLOAT32_DEFINED)
		vm->CondFlag = vm->Regs[regids & 0xff].Float != vm->Regs[regids >> 8].Float;
	#endif
		DISPATCH();
	}
	exec_incf: { /* char: opcode | char: regid */
		const uint8_t regid = *pc.PtrUInt8++;
	#if defined(__TAGHA_FLOAT64_DEFINED)
		++vm->Regs[regid].Double;
	#elif defined(__TAGHA_FLOAT32_DEFINED)
		++vm->Regs[regid].Float;
	#endif
		DISPATCH();
	}
	exec_decf: { /* char: opcode | char: regid */
		const uint8_t regid = *pc.PtrUInt8++;
	#if defined(__TAGHA_FLOAT64_DEFINED)
		--vm->Regs[regid].Double;
	#elif defined(__TAGHA_FLOAT32_DEFINED)
		--vm->Regs[regid].Float;
	#endif
		DISPATCH();
	}
	exec_negf: { /* char: opcode | char: regid */
		const uint8_t regid = *pc.PtrUInt8++;
	#if defined(__TAGHA_FLOAT64_DEFINED)
		vm->Regs[regid].Double = -vm->Regs[regid].Double;
	#elif defined(__TAGHA_FLOAT32_DEFINED)
		vm->Regs[regid].Float = -vm->Regs[regid].Float;
	#endif
		DISPATCH();
	}
#endif
	exec_halt:
	//Tagha_PrintVMState(vm);
	return vm->regAlaf.Int32;
}

int32_t Tagha_RunScript(struct Tagha *const restrict vm, const int32_t argc, char *argv[restrict static argc+1])
{
	if( !vm || !vm->Header )
		return -1;
	
	union TaghaPtr reader = {vm->Header};
	if( *reader.PtrUInt16 != 0xC0DE ) {
		vm->Error = ErrInvalidScript;
		return -1;
	}
	reader.PtrUInt16++;
	
	/* push argc, argv to registers. */
	union TaghaVal MainArgs[argc+1];
	MainArgs[argc].Ptr = NULL;
	if( argv )
		for( int32_t i=0 ; i<argc ; i++ )
			MainArgs[i].Ptr = argv[i];
	vm->reg_Eh.Ptr = MainArgs;
	vm->regSemkath.Int32 = argc;
	
	/* check out stack size and align it by the size of union TaghaVal. */
	const uint32_t stacksize = *reader.PtrUInt32++;
	if( !stacksize ) {
		vm->Error = ErrStackSize;
		return -1;
	}
	
	reader.PtrUInt32++;
	vm->DataBase = (vm->Header + *reader.PtrUInt32++);
	vm->DataBase += sizeof(int32_t);
	
	union TaghaVal *Stack = (union TaghaVal *)(vm->Header + *reader.PtrUInt32);
	vm->regStk.PtrSelf = vm->regBase.PtrSelf = Stack + stacksize - 1;
	vm->Footer = (uint8_t *)(Stack + stacksize);
	
	vm->Error = ErrNone;
	(--vm->regStk.PtrSelf)->Int64 = 0LL;	/* push NULL return address. */
	*--vm->regStk.PtrSelf = vm->regBase; /* push rbp */
	vm->regBase = vm->regStk; /* mov rbp, rsp */
	vm->regInstr.Ptr = GetFunctionOffsetByName(vm->Header, "main");
	if( !vm->regInstr.Ptr ) {
		vm->Error = ErrMissingFunc;
		return -1;
	}
	else return Tagha_Exec(vm);
}

int32_t Tagha_CallFunc(struct Tagha *const restrict vm, const char *restrict funcname, const size_t args, union TaghaVal values[static args])
{
	if( !vm || !vm->Header || !funcname || !values )
		return -1;
	
	union TaghaPtr reader = {vm->Header};
	if( *reader.PtrUInt16 != 0xC0DE ) {
		vm->Error = ErrInvalidScript;
		return -1;
	}
	reader.PtrUInt16++;
	
	/* check out stack size && align it by the size of union TaghaVal. */
	const uint32_t stacksize = *reader.PtrUInt32++;
	if( !stacksize ) {
		vm->Error = ErrStackSize;
		return -1;
	}
	
	reader.PtrUInt32++;
	vm->DataBase = (vm->Header + *reader.PtrUInt32++);
	vm->DataBase += sizeof(int32_t);
	
	union TaghaVal *Stack = (union TaghaVal *)(vm->Header + *reader.PtrUInt32);
	vm->regStk.PtrSelf = vm->regBase.PtrSelf = Stack + stacksize - 1;
	vm->Footer = (uint8_t *)(Stack + stacksize);
	
	/* remember that arguments must be passed right to left.
	 we have enough args to fit in registers. */
	const uint8_t reg_param_initial = regSemkath;
	const uint8_t reg_params = regTaw - regSemkath + 1;
	const size_t bytecount = sizeof(union TaghaVal) * args;
	
	/* save stack space by using the registers for passing arguments. */
	/* the other registers can be used for other data ops. */
	if( args <= reg_params ) {
		memcpy(vm->Regs+reg_param_initial, values, bytecount);
	}
	/* if the function has more than a certain num of params, push from both registers && stack. */
	else {
		if( vm->regStk.PtrSelf - (args-reg_params) < Stack ) {
			vm->Error = ErrStackOver;
			return -1;
		}
		memcpy(vm->Regs+reg_param_initial, values, sizeof(union TaghaVal) * reg_params);
		memcpy(vm->regStk.PtrSelf, values+reg_params, sizeof(union TaghaVal) * (args-reg_params));
		vm->regStk.PtrSelf -= (args-reg_params);
	}
	vm->Error = ErrNone;
	(--vm->regStk.PtrSelf)->Int64 = 0LL;	/* push NULL return address. */
	*--vm->regStk.PtrSelf = vm->regBase; /* push rbp */
	vm->regBase = vm->regStk; /* mov rbp, rsp */
	vm->regInstr.Ptr = GetFunctionOffsetByName(vm->Header, funcname);
	if( !vm->regInstr.Ptr ) {
		vm->Error = ErrMissingFunc;
		return -1;
	}
	else return Tagha_Exec(vm);
}

union TaghaVal Tagha_GetReturnValue(const struct Tagha *const vm)
{
	return vm ? vm->regAlaf : (union TaghaVal){0};
}

void *Tagha_GetGlobalVarByName(struct Tagha *const restrict vm, const char *restrict varname)
{
	return !vm || !vm->Header || !varname ? NULL : GetVariableOffsetByName(vm->Header, varname);
}

const char *Tagha_GetError(const struct Tagha *const restrict vm)
{
	if( !vm )
		return "Null VM Pointer";
	
	switch( vm->Error ) {
		case ErrInstrBounds: return "Out of Bound Instruction";
		case ErrNone: return "None";
		case ErrBadPtr: return "Null/Invalid Pointer";
		case ErrMissingFunc: return "Missing Function";
		case ErrInvalidScript: return "Null/Invalid Script";
		case ErrStackSize: return "Bad Stack Size";
		case ErrMissingNative: return "Missing Native";
		case ErrStackOver: return "Stack Overflow";
		default: return "Unknown Error";
	}
}

void *Tagha_GetRawScriptPtr(const struct Tagha *const restrict vm)
{
	return !vm ? NULL : vm->Header;
}

void Tagha_ThrowError(struct Tagha *const vm, const int32_t err)
{
	if( !vm || !err )
		return;
	vm->Error = err;
}
/************************************************/
