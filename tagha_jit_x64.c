#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <limits.h>
#include "tagha.h"

#define TAGHA_PAGE_SIZE    4096

#if defined(_WIN32) || defined(_WIN64)
#  ifndef OS_WINDOWS
#    define OS_WINDOWS 1
#  endif
#elif defined(unix) || defined(__unix) || defined(__unix__) || defined(__linux__) || defined(linux) || defined(__linux) || defined(__FreeBSD__)
#  ifndef OS_LINUX_UNIX
#    define OS_LINUX_UNIX 1
#  endif
#elif defined(__ANDROID__)
#  ifndef OS_ANDROID
#    define OS_ANDROID 1
#  endif
#else
#  ifndef OS_MAC
#    define OS_MAC 1
#  endif
#endif


struct JITFunc;
typedef struct JITFunc *jitfunc_ptr;

struct JITFunc {
	uint8_t Code[TAGHA_PAGE_SIZE - sizeof(size_t)];
	size_t Len;
};

jitfunc_ptr jitfunc_create(void)
{
# ifdef OS_WINDOWS
	return VirtualAlloc(NULL, TAGHA_PAGE_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
# else
	// giving undefined symbol for MAP_ANON and ANONYMOUS.
	return mmap(NULL, TAGHA_PAGE_SIZE, PROT_READ | PROT_WRITE, 0x20 | MAP_PRIVATE, -1, 0);
# endif
}

void jitfunc_protect(jitfunc_ptr jit)
{
	if( !jit )
		return;
# ifdef OS_WINDOWS
	DWORD i;
	VirtualProtect(jit, sizeof *jit, PAGE_EXECUTE_READ, &i);
# else
	mprotect(jit, TAGHA_PAGE_SIZE, PROT_READ | PROT_EXEC);
# endif
}

static void jitfunc_unprotect(jitfunc_ptr jit)
{
	if( !jit )
		return;
# ifdef OS_WINDOWS
	DWORD i;
	VirtualProtect(jit, sizeof *jit, PAGE_READWRITE, &i);
# else
	mprotect(jit, TAGHA_PAGE_SIZE, PROT_READ | PROT_WRITE);
# endif
}

void jitfunc_free(jitfunc_ptr *jitref)
{
	if( !jitref || !*jitref )
		return;
# ifdef OS_WINDOWS
	VirtualFree(*jitref, 0, MEM_RELEASE);
# else
	munmap(*jitref, TAGHA_PAGE_SIZE);
# endif
	*jitref = NULL;
}

void jitfunc_insert(jitfunc_ptr jit, const size_t bytes, const uint64_t instr)
{
	if( !jit )
		return;
	
	memcpy(jit->Code + jit->Len, &instr, bytes);
	jit->Len += bytes;
}

void jitfunc_imm(jitfunc_ptr jit, uint64_t imm)
{
	if( !jit )
		return;
	
	/* 8 byte imm values that have the first 4 bytes set to max are encoded as 4 bytes. */
	if( imm>>32 == UINT32_MAX ) {
		memcpy(jit->Code + jit->Len, &imm, sizeof(uint32_t));
		jit->Len += sizeof(uint32_t);
	} else {
		memcpy(jit->Code + jit->Len, &imm, sizeof imm);
		jit->Len += immlen;
	}
}

static void *jitfunc_givefunc(jitfunc_ptr jit)
{
	return jit->Code;
}

static void jitfunc_clear(jitfunc_ptr jit)
{
	memset(jit->Code, 0, jit->Len);
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


//#include <unistd.h>	// sleep() func

int32_t Tagha_JITExec(struct Tagha *const restrict vm)
{
	if( !vm )
		return -1;
	
	union TaghaPtr pc = {vm->regInstr.PtrUInt8};
	jitfunc_ptr jit = jitfunc_create();
	
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
		// movabs r11
		jitfunc_insert(jit, 2, 0x49BB); jitfunc_imm(jit, *pc.PtrUInt64++);
		// push r11
		jitfunc_insert(jit, 2, 0x4153);
		DISPATCH();
	}
	/* push a register's contents. */
	exec_push: { /* char: opcode | char: regid */
		switch( *pc.PtrUInt8++ ) {
			case regStk:
				jitfunc_insert(jit, 1, 0x54); // push rsp
				DISPATCH();
			case regBase:
				jitfunc_insert(jit, 1, 0x55); // push rbp
				DISPATCH();
			case regSemkath:
				jitfunc_insert(jit, 2, 0x4153); // push r11
				DISPATCH();
			case reg_Eh:
				jitfunc_insert(jit, 2, 0x4154); // push r12
				DISPATCH();
			case regPeh:
				jitfunc_insert(jit, 2, 0x4155); // push r13
				DISPATCH();
			case regSadhe:
				jitfunc_insert(jit, 2, 0x4156); // push r14
				DISPATCH();
			default:
				jitfunc_insert(jit, 2, 0x4157); // push r15
				DISPATCH();
		}
	}
	
	/* pops a value from the stack into a register then reduces stack by 8 bytes. */
	exec_pop: { /* char: opcode | char: regid */
		switch( *pc.PtrUInt8++ ) {
			case regStk:
				jitfunc_insert(jit, 1, 0x5c); // pop rsp
				DISPATCH();
			case regBase:
				jitfunc_insert(jit, 1, 0x5d); // pop rbp
				DISPATCH();
			case regSemkath:
				jitfunc_insert(jit, 2, 0x415B); // pop r11
				DISPATCH();
			case reg_Eh:
				jitfunc_insert(jit, 2, 0x415C); // pop r12
				DISPATCH();
			case regPeh:
				jitfunc_insert(jit, 2, 0x415D); // pop r13
				DISPATCH();
			case regSadhe:
				jitfunc_insert(jit, 2, 0x415E); // pop r14
				DISPATCH();
			default:
				jitfunc_insert(jit, 2, 0x415F); // pop r15
				DISPATCH();
		}
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
		switch( *pc.PtrUInt8++ ) {
			case regStk:
				jitfunc_insert(jit, 2, 0x48BC); // mov rsp, <imm64>
				break;
			case regBase:
				jitfunc_insert(jit, 2, 0x48BD); // mov rbp, <imm64>
				break;
			case regSemkath:
				jitfunc_insert(jit, 2, 0x49BB); // mov r11, <imm64>
				break;
			case reg_Eh:
				jitfunc_insert(jit, 2, 0x49BC); // mov r12, <imm64>
				break;
			case regPeh:
				jitfunc_insert(jit, 2, 0x49BD); // mov r13, <imm64>
				break;
			case regSadhe:
				jitfunc_insert(jit, 2, 0x49BE); // mov r14, <imm64>
				break;
			default:
				jitfunc_insert(jit, 2, 0x49BF); // mov r15, <imm64>
		}
		jitfunc_imm(jit, *pc.PtrUInt64++);
		DISPATCH();
	}
	exec_mov: { /* char: opcode | char: dest reg | char: src reg */
		pc.PtrUInt16++;
		jitfunc_insert(jit, 3, 0x4D89E3); // mov r11, r12
		DISPATCH();
	}
	
	exec_ld1: { /* char: opcode | char: dest reg | char: src reg | i32: offset */
		const uint16_t regids = *pc.PtrUInt16++;
		const union TaghaPtr mem = {vm->Regs[regids >> 8].PtrUInt8 + *pc.PtrInt32++};
		// do a memcheck here.
		if( vm->SafeMode && (mem.PtrUInt8 < vm->DataBase || mem.PtrUInt8 >= vm->Footer) ) {
			vm->Error = ErrBadPtr;
			jitfunc_free(&jit);
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
			jitfunc_free(&jit);
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
			jitfunc_free(&jit);
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
			jitfunc_free(&jit);
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
			jitfunc_free(&jit);
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
			jitfunc_free(&jit);
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
			jitfunc_free(&jit);
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
			jitfunc_free(&jit);
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
		jitfunc_protect(jit);
		void (*jitfunc)(void) = jitfunc_givefunc(jit);
		(*jitfunc)();
		jitfunc_clear(jit);
		jitfunc_unprotect(jit);
		
		const int64_t offset = *pc.PtrInt64++;
		pc.PtrUInt8 += offset;
		DISPATCH();
	}
	exec_jz: { /* char: opcode | i64: offset */
		jitfunc_protect(jit);
		void (*jitfunc)(void) = jitfunc_givefunc(jit);
		(*jitfunc)();
		jitfunc_clear(jit);
		jitfunc_unprotect(jit);
		
		const int64_t offset = *pc.PtrInt64++;
		//if( !vm->CondFlag )
		//	pc.PtrUInt8 += offset;
		//pc.PtrUInt8 = ( !vm->CondFlag ) ? pc.PtrUInt8 + offset : pc.PtrUInt8;
		pc.PtrUInt8 += ( !vm->CondFlag ) ? offset : 0;
		DISPATCH();
	}
	exec_jnz: { /* char: opcode | i64: offset */
		jitfunc_protect(jit);
		void (*jitfunc)(void) = jitfunc_givefunc(jit);
		(*jitfunc)();
		jitfunc_clear(jit);
		jitfunc_unprotect(jit);
		
		const int64_t offset = *pc.PtrInt64++;
		//if( vm->CondFlag )
		//	pc.PtrUInt8 += offset;
		//pc.PtrUInt8 = ( vm->CondFlag ) ? pc.PtrUInt8 + offset : pc.PtrUInt8;
		pc.PtrUInt8 += ( vm->CondFlag ) ? offset : 0;
		DISPATCH();
	}
	exec_call: { /* char: opcode | i64: offset */
		jitfunc_protect(jit);
		void (*jitfunc)(void) = jitfunc_givefunc(jit);
		(*jitfunc)();
		jitfunc_clear(jit);
		jitfunc_unprotect(jit);
		
		/* The restrict type qualifier is an indication to the compiler that,
		 * if the memory addressed by the restrict-qualified pointer is modified,
		 * no other pointer will access that same memory.
		 * Since we're pushing the restrict-qualified pointer's memory that it points to,
		 * This is NOT undefined behavior because it's not aliasing access of the instruction stream.
		 */
		const uint64_t index = *pc.PtrUInt64++;
		(--vm->regStk.PtrSelf)->Ptr = pc.Ptr;	/* push rip */
		*--vm->regStk.PtrSelf = vm->regBase;	/* push rbp */
		vm->regBase = vm->regStk;	/* mov rbp, rsp */
		pc.PtrUInt8 = GetFunctionOffsetByIndex(vm->Header, index - 1);
		DISPATCH();
	}
	exec_callr: { /* char: opcode | char: regid */
		jitfunc_protect(jit);
		void (*jitfunc)(void) = jitfunc_givefunc(jit);
		(*jitfunc)();
		jitfunc_clear(jit);
		jitfunc_unprotect(jit);
		
		const uint8_t regid = *pc.PtrUInt8++;
		(--vm->regStk.PtrSelf)->Ptr = pc.Ptr;	/* push rip */
		*--vm->regStk.PtrSelf = vm->regBase;	/* push rbp */
		vm->regBase = vm->regStk;	/* mov rbp, rsp */
		pc.PtrUInt8 = GetFunctionOffsetByIndex(vm->Header, (vm->Regs[regid].UInt64 - 1));
		DISPATCH();
	}
	exec_ret: { /* char: opcode */
		jitfunc_protect(jit);
		void (*jitfunc)(void) = jitfunc_givefunc(jit);
		(*jitfunc)();
		jitfunc_clear(jit);
		jitfunc_unprotect(jit);
		
		vm->regStk = vm->regBase; /* mov rsp, rbp */
		vm->regBase = *vm->regStk.PtrSelf++; /* pop rbp */
		pc.Ptr = (*vm->regStk.PtrSelf++).Ptr; /* pop rip */
		if( !pc.Ptr ) {
			jitfunc_free(&jit);
			return vm->regAlaf.Int32;
		}
		else { DISPATCH(); }
	}
	
	exec_syscall: { /* char: opcode | i64: index */
		jitfunc_protect(jit);
		void (*jitfunc)(void) = jitfunc_givefunc(jit);
		(*jitfunc)();
		jitfunc_clear(jit);
		jitfunc_unprotect(jit);
		
		TaghaNative *const nativeref = GetNativeByIndex(vm->Header, (-1 - *pc.PtrInt64++));
		if( !nativeref ) {
			vm->Error = ErrMissingNative;
			jitfunc_free(&jit);
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
			
			if( vm->Error != ErrNone ) {
				jitfunc_free(&jit);
				return vm->Error;
			}
			else { DISPATCH(); }
		}
	}
	exec_syscallr: { /* char: opcode | char: reg id */
		jitfunc_protect(jit);
		void (*jitfunc)(void) = jitfunc_givefunc(jit);
		(*jitfunc)();
		jitfunc_clear(jit);
		jitfunc_unprotect(jit);
		
		const uint8_t regid = *pc.PtrUInt8++;
		TaghaNative *const nativeref = GetNativeByIndex(vm->Header, (-1 - vm->Regs[regid].Int64));
		if( !nativeref ) {
			vm->Error = ErrMissingNative;
			jitfunc_free(&jit);
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
			
			if( vm->Error != ErrNone ) {
				jitfunc_free(&jit);
				return vm->Error;
			}
			else { DISPATCH(); }
		}
	}
	
#ifdef FLOATING_POINT_OPS
	exec_flt2dbl: { /* treat as no op if one float type is defined but not the other. */
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
	jitfunc_free(&jit);
	return vm->regAlaf.Int32;
}
/************************************************/
