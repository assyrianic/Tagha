
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>
#include "vm.h"


/*	here's the deal ok? make an opcode for each and erry n-bytes!
 * 'q' = int64
 * 'l' - int32
 * 's' - int16
 * 'b' - byte | push and pop do not take bytes
 * 'f' - float32
 * 'df' - float64
 * 'a' - address
*/

// this vm is designed to run C programs. Vastly, if not all C expressions are int32, uint32 if bigger than int
// this is why the arithmetic and bit operations are all int32 sized.
// there's 2 byte and single byte memory storage for the sake of dealing with structs and unions.
// expressions are int or uint then truncated to a variable's byte-width.
#define INSTR_SET	\
	X(halt) \
	X(pushl) X(pushs) X(pushb) X(pushsp) X(puship) \
	X(popl) X(pops) X(popb) X(popsp) X(popip) \
	X(wrtl) X(wrts) X(wrtb) \
	X(storel) X(stores) X(storeb) \
	X(storela) X(storesa) X(storeba) \
	X(loadl) X(loads) X(loadb) \
	X(loadla) X(loadsa) X(loadba) \
	X(copyl) X(copys) X(copyb) \
	X(addl) X(uaddl) X(addf) \
	X(subl) X(usubl) X(subf) \
	X(mull) X(umull) X(mulf) \
	X(divl) X(udivl) X(divf) \
	X(modl) X(umodl) \
	X(andl) X(orl) X(xorl) \
	X(notl) X(shll) X(shrl) \
	X(incl) X(decl) X(negl) \
	X(ltl) X(ultl) X(ltf) \
	X(gtl) X(ugtl) X(gtf) \
	X(cmpl) X(ucmpl) X(compf) \
	X(leql) X(uleql) X(leqf) \
	X(geql) X(ugeql) X(geqf) \
	X(jmp) X(jzl) X(jnzl) \
	X(call) X(ret) X(reset) \
	X(nop) \

#define X(x) x,
enum InstrSet { INSTR_SET };
#undef X

void vm_init(CVM_t *restrict vm)
{
	if( !vm )
		return;
	
	vm->pbMemory = NULL;
	vm->pbDataStack = NULL;
	
	uint i;
	for( i=0 ; i<CALLSTK_SIZE ; i++ )
		vm->bCallstack[i] = 0;
	
	vm->ip = 0;
	vm->sp = 0;
	vm->callsp = 0;
	vm->pInstrStream = NULL;
}

void vm_load_code(CVM_t *restrict vm, uchar *restrict program)
{
	if( !vm )
		return;
	
	vm->pbDataStack = calloc(STK_SIZE, sizeof(uchar)); //&(uchar[STK_SIZE]){0};
	assert(vm->pbDataStack);
	vm->pbMemory = calloc(MEM_SIZE, sizeof(uchar)); //&(uchar[MEM_SIZE]){0};
	assert(vm->pbMemory);
	
	vm->pInstrStream = program;
}

void vm_free(CVM_t *vm)
{
	if( !vm )
		return;
	
	if( vm->pbDataStack )
		free(vm->pbDataStack);
	vm->pbDataStack = NULL;
	
	if( vm->pbMemory )
		free(vm->pbMemory);
	vm->pbMemory = NULL;
	vm_init(vm);
}


void vm_reset(CVM_t *vm)
{
	if( !vm )
		return;
	uint i;
	for( i=0 ; i<MEM_SIZE ; i++ )
		vm->pbMemory[i] = 0;
	for( i=0 ; i<STK_SIZE ; i++ )
		vm->pbDataStack[i] = 0;
	for( i=0 ; i<CALLSTK_SIZE ; i++ )
		vm->bCallstack[i] = 0;
	vm->ip = 0;
	vm->sp = 0;
	vm->callsp = 0;
}

// returns index of func ptr
int vm_register_func(CVM_t *restrict vm, NativeInfo *arrNatives)
{
	if( !vm || !arrNatives )
		return -1;
	
	return 0;
}

uint vm_pop_word(CVM_t *vm)
{
	if( !vm )
		return 0;
#ifdef SAFEMODE
	if( (vm->sp-4) >= STK_SIZE ) {	// we're subtracting, did we integer underflow?
		printf("vm_pop_word reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-4);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.c[3] = vm->pbDataStack[vm->sp--];
	conv.c[2] = vm->pbDataStack[vm->sp--];
	conv.c[1] = vm->pbDataStack[vm->sp--];
	conv.c[0] = vm->pbDataStack[vm->sp--];
	return conv.ui;
}

ushort vm_pop_short(CVM_t *vm)
{
	if( !vm )
		return 0;
#ifdef SAFEMODE
	if( (vm->sp-2) >= STK_SIZE ) {	// we're subtracting, did we integer underflow?
		printf("vm_pop_short reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-2);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.c[1] = vm->pbDataStack[vm->sp--];
	conv.c[0] = vm->pbDataStack[vm->sp--];
	return conv.us;
}
uchar vm_pop_byte(CVM_t *vm)
{
	if( !vm )
		return 0;
#ifdef SAFEMODE
	if( (vm->sp-1) >= STK_SIZE ) {	// we're subtracting, did we integer underflow?
		printf("vm_pop_byte reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-1);
		exit(1);
	}
#endif
	return vm->pbDataStack[vm->sp--];
}

float vm_pop_float32(CVM_t *vm)
{
	if( !vm )
		return 0;
#ifdef SAFEMODE
	if( (vm->sp-4) >= STK_SIZE ) {	// we're subtracting, did we integer underflow?
		printf("vm_pop_float32 reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-4);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.c[3] = vm->pbDataStack[vm->sp--];
	conv.c[2] = vm->pbDataStack[vm->sp--];
	conv.c[1] = vm->pbDataStack[vm->sp--];
	conv.c[0] = vm->pbDataStack[vm->sp--];
	return conv.f;
}

void vm_push_word(CVM_t *restrict vm, const uint val)
{
	if( !vm )
		return;
#ifdef SAFEMODE
	if( (vm->sp+4) >= STK_SIZE ) {
		printf("vm_push_word reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp+4);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.ui = val;
	vm->pbDataStack[++vm->sp] = conv.c[0];
	vm->pbDataStack[++vm->sp] = conv.c[1];
	vm->pbDataStack[++vm->sp] = conv.c[2];
	vm->pbDataStack[++vm->sp] = conv.c[3];
}

void vm_push_short(CVM_t *restrict vm, const ushort val)
{
	if( !vm )
		return;
#ifdef SAFEMODE
	if( (vm->sp+2) >= STK_SIZE ) {
		printf("vm_push_short reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp+2);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.us = val;
	vm->pbDataStack[++vm->sp] = conv.c[0];
	vm->pbDataStack[++vm->sp] = conv.c[1];
}

void vm_push_byte(CVM_t *restrict vm, const uchar val)
{
	if( !vm )
		return;
#ifdef SAFEMODE
	if( (vm->sp+1) >= STK_SIZE ) {
		printf("vm_push_byte reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp+1);
		exit(1);
	}
#endif
	vm->pbDataStack[++vm->sp] = val;
}

void vm_push_float(CVM_t *restrict vm, const float val)
{
	if( !vm )
		return;
#ifdef SAFEMODE
	if( (vm->sp+4) >= STK_SIZE ) {
		printf("vm_push_float reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp+4);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.f = val;
	vm->pbDataStack[++vm->sp] = conv.c[0];
	vm->pbDataStack[++vm->sp] = conv.c[1];
	vm->pbDataStack[++vm->sp] = conv.c[2];
	vm->pbDataStack[++vm->sp] = conv.c[3];
}

void vm_write_word(CVM_t *restrict vm, const uint val, const uint address)
{
	if( !vm )
		return;
#ifdef SAFEMODE
	if( address > MEM_SIZE-4 ) {
		printf("vm_write_word reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, address);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.ui = val;
	vm->pbMemory[address] = conv.c[0];
	vm->pbMemory[address+1] = conv.c[1];
	vm->pbMemory[address+2] = conv.c[2];
	vm->pbMemory[address+3] = conv.c[3];
}

void vm_write_short(CVM_t *restrict vm, const ushort val, const uint address)
{
	if( !vm )
		return;
#ifdef SAFEMODE
	if( address > MEM_SIZE-2 ) {
		printf("vm_write_short reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, address);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.us = val;
	vm->pbMemory[address] = conv.c[0];
	vm->pbMemory[address+1] = conv.c[1];
}

void vm_write_byte(CVM_t *restrict vm, const uchar val, const uint address)
{
	if( !vm )
		return;
#ifdef SAFEMODE
	if( address >= MEM_SIZE ) {
		printf("vm_write_byte reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, address);
		exit(1);
	}
#endif
	vm->pbMemory[address] = val;
}

void vm_write_float(CVM_t *restrict vm, const float val, const uint address)
{
	if( !vm )
		return;
#ifdef SAFEMODE
	if( address > MEM_SIZE-4 ) {
		printf("vm_write_float reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, address);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.f = val;
	vm->pbMemory[address] = conv.c[0];
	vm->pbMemory[address+1] = conv.c[1];
	vm->pbMemory[address+2] = conv.c[2];
	vm->pbMemory[address+3] = conv.c[3];
}

void vm_write_bytearray(CVM_t *restrict vm, uchar *restrict val, const uint size, const uint address)
{
	if( !vm )
		return;
	uint addr = address;
	uint i = 0;
	while( i<size ) {
#ifdef SAFEMODE
		if( addr >= MEM_SIZE-i ) {
			printf("vm_write_array reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, addr);
			exit(1);
		}
#endif
		vm->pbMemory[addr++] = val[i++];
	}
}

uint vm_read_word(CVM_t *restrict vm, const uint address)
{
	if( !vm )
		return 0;
#ifdef SAFEMODE
	if( address > MEM_SIZE-4 ) {
		printf("vm_read_word reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, address);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.c[0] = vm->pbMemory[address];
	conv.c[1] = vm->pbMemory[address+1];
	conv.c[2] = vm->pbMemory[address+2];
	conv.c[3] = vm->pbMemory[address+3];
	return conv.ui;
}

ushort vm_read_short(CVM_t *restrict vm, const uint address)
{
	if( !vm )
		return 0;
#ifdef SAFEMODE
	if( address > MEM_SIZE-2 ) {
		printf("vm_read_short reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, address);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.c[0] = vm->pbMemory[address];
	conv.c[1] = vm->pbMemory[address+1];
	return conv.f;
}

uchar vm_read_byte(CVM_t *restrict vm, const uint address)
{
	if( !vm )
		return 0;
#ifdef SAFEMODE
	if( address >= MEM_SIZE ) {
		printf("vm_read_byte reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, address);
		exit(1);
	}
#endif
	return vm->pbMemory[address];
}

float vm_read_float(CVM_t *restrict vm, const uint address)
{
	if( !vm )
		return 0;
#ifdef SAFEMODE
	if( address > MEM_SIZE-4 ) {
		printf("vm_read_float reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, address);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.c[0] = vm->pbMemory[address];
	conv.c[1] = vm->pbMemory[address+1];
	conv.c[2] = vm->pbMemory[address+2];
	conv.c[3] = vm->pbMemory[address+3];
	return conv.f;
}

void vm_read_bytearray(CVM_t *restrict vm, uchar *restrict buffer, const uint size, const uint address)
{
	if( !vm )
		return;
	uint addr = address;
	uint i = 0;
	while( i<size ) {
#ifdef SAFEMODE
		if( addr >= MEM_SIZE-i ) {
			printf("vm_read_array reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, addr);
			exit(1);
		}
#endif
		buffer[i++] = vm->pbMemory[addr++];
	}
}


void vm_debug_print_memory(const CVM_t *vm)
{
	if( !vm )
		return;
	if( !vm->pbMemory )
		return;
	
	printf("DEBUG ...---===---... Printing Memory...\n");
	uint i;
	for( i=0 ; i<MEM_SIZE ; i++ )
		if( vm->pbMemory[i] )
			printf("Memory Index: 0x%x | data: %" PRIu32 "\n", i, vm->pbMemory[i]);
	printf("\n");
}
void vm_debug_print_stack(const CVM_t *vm)
{
	if( !vm )
		return;
	if( !vm->pbDataStack )
		return;
	
	printf("DEBUG ...---===---... Printing Stack...\n");
	uint i;
	for( i=0 ; i<STK_SIZE ; i++ )
		if( vm->pbDataStack[i] )
			printf("Stack Index: 0x%x | data: %" PRIu32 "\n", i, vm->pbDataStack[i]);
	printf("\n");
}
void vm_debug_print_callstack(const CVM_t *vm)
{
	if( !vm )
		return;
	printf("DEBUG ...---===---... Printing Call Stack...\n");
	uint i;
	for( i=0 ; i<CALLSTK_SIZE ; i++ )
		if( vm->bCallstack[i] )
			printf("Call Stack Index: 0x%x | data: %" PRIu32 "\n", i, vm->bCallstack[i]);
	printf("\n");
}
void vm_debug_print_ptrs(const CVM_t *vm)
{
	if( !vm )
		return;
	
	printf("DEBUG ...---===---... Printing Pointers...\n");
	printf("Instruction Pointer: %" PRIu32 "\
			\nStack Pointer: %" PRIu32 "\
			\nCall Stack Pointer: %" PRIu32 ""
			/*\nCall Stack Frame Pointer: %" PRIu32 "\n"*/, vm->ip, vm->sp, vm->callsp/*, vm->bp*/);
	printf("\n");
}



static inline uint vm_get_imm4(CVM_t *restrict vm)
{
	if( !vm )
		return 0;
	union conv_union conv;
	conv.c[0] = vm->pInstrStream[++vm->ip];
	conv.c[1] = vm->pInstrStream[++vm->ip];
	conv.c[2] = vm->pInstrStream[++vm->ip];
	conv.c[3] = vm->pInstrStream[++vm->ip];
	return conv.ui;
}

static inline ushort vm_get_imm2(CVM_t *restrict vm)
{
	if( !vm )
		return 0;
	union conv_union conv;
	conv.c[0] = vm->pInstrStream[++vm->ip];
	conv.c[1] = vm->pInstrStream[++vm->ip];
	return conv.us;
}

static inline uint _vm_pop_word(CVM_t *vm)
{
	if( !vm )
		return 0;
#ifdef SAFEMODE
	if( (vm->sp-4) >= STK_SIZE ) {	// we're subtracting, did we integer underflow?
		printf("vm_pop_word reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-4);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.c[3] = vm->pbDataStack[vm->sp--];
	conv.c[2] = vm->pbDataStack[vm->sp--];
	conv.c[1] = vm->pbDataStack[vm->sp--];
	conv.c[0] = vm->pbDataStack[vm->sp--];
	return conv.ui;
}

static inline float _vm_pop_float32(CVM_t *vm)
{
	if( !vm )
		return 0;
#ifdef SAFEMODE
	if( (vm->sp-4) >= STK_SIZE ) {	// we're subtracting, did we integer underflow?
		printf("vm_pop_float32 reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-4);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.c[3] = vm->pbDataStack[vm->sp--];
	conv.c[2] = vm->pbDataStack[vm->sp--];
	conv.c[1] = vm->pbDataStack[vm->sp--];
	conv.c[0] = vm->pbDataStack[vm->sp--];
	return conv.f;
}

static inline ushort _vm_pop_short(CVM_t *vm)
{
	if( !vm )
		return 0;
#ifdef SAFEMODE
	if( (vm->sp-2) >= STK_SIZE ) {	// we're subtracting, did we integer underflow?
		printf("vm_pop_word reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-2);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.c[1] = vm->pbDataStack[vm->sp--];
	conv.c[0] = vm->pbDataStack[vm->sp--];
	return conv.us;
}

static inline uchar _vm_pop_byte(CVM_t *vm)
{
	if( !vm )
		return 0;
#ifdef SAFEMODE
	if( (vm->sp-1) >= STK_SIZE ) {	// we're subtracting, did we integer underflow?
		printf("vm_pop_word reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-1);
		exit(1);
	}
#endif
	return vm->pbDataStack[vm->sp--];
}


static inline void _vm_push_word(CVM_t *restrict vm, const uint val)
{
	if( !vm )
		return;
#ifdef SAFEMODE
	if( (vm->sp+4) >= STK_SIZE ) {
		printf("vm_push_word reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp+4);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.ui = val;
	vm->pbDataStack[++vm->sp] = conv.c[0];
	vm->pbDataStack[++vm->sp] = conv.c[1];
	vm->pbDataStack[++vm->sp] = conv.c[2];
	vm->pbDataStack[++vm->sp] = conv.c[3];
}
static inline void _vm_push_float(CVM_t *restrict vm, const float val)
{
	if( !vm )
		return;
#ifdef SAFEMODE
	if( (vm->sp+4) >= STK_SIZE ) {
		printf("vm_push_float reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp+4);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.f = val;
	vm->pbDataStack[++vm->sp] = conv.c[0];
	vm->pbDataStack[++vm->sp] = conv.c[1];
	vm->pbDataStack[++vm->sp] = conv.c[2];
	vm->pbDataStack[++vm->sp] = conv.c[3];
}

void _vm_push_short(CVM_t *restrict vm, const ushort val)
{
	if( !vm )
		return;
#ifdef SAFEMODE
	if( (vm->sp+2) >= STK_SIZE ) {
		printf("vm_push_short reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp+2);
		exit(1);
	}
#endif
	union conv_union conv;
	conv.us = val;
	vm->pbDataStack[++vm->sp] = conv.c[0];
	vm->pbDataStack[++vm->sp] = conv.c[1];
}

static inline void _vm_push_byte(CVM_t *restrict vm, const uchar val)
{
	if( !vm )
		return;
#ifdef SAFEMODE
	if( (vm->sp+1) >= STK_SIZE ) {
		printf("vm_push_byte reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp+1);
		exit(1);
	}
#endif
	vm->pbDataStack[++vm->sp] = val;
}


//#include <unistd.h>	// sleep() func
void vm_exec(CVM_t *restrict vm)
{
	if( !vm )
		return;
	else if( !vm->pInstrStream )
		return;
	
	union conv_union conv;
	uint b,a;
	float fb,fa;
	ushort sb,sa;
	uchar cb,ca;
	
#define X(x) #x ,
	const char *opcode2str[] = { INSTR_SET };
#undef X

#define X(x) &&exec_##x ,
	static const void *dispatch[] = { INSTR_SET };
#undef X
#undef INSTR_SET

	if( vm->pInstrStream[vm->ip] > nop) {
		printf("illegal instruction exception! instruction == \'%" PRIu32 "\' @ %" PRIu32 "\n", vm->pInstrStream[vm->ip], vm->ip);
		goto *dispatch[halt];
		return;
	}
	//printf( "current instruction == \"%s\" @ ip == %" PRIu32 "\n", opcode2str[vm->pInstrStream[vm->ip]], vm->ip );
#ifdef _UNISTD_H
	#define DISPATCH()	sleep(1); goto *dispatch[ vm->pInstrStream[++vm->ip] ]
#else
	#define DISPATCH()	goto *dispatch[ vm->pInstrStream[++vm->ip] ]
#endif
	goto *dispatch[ vm->pInstrStream[vm->ip] ];

exec_nop:;
	DISPATCH();
	
exec_halt:;
	printf("===================== vm done\n\n");
	return;

// opcodes for longs
exec_pushl:;	// push 4 bytes onto the stack
	conv.ui = vm_get_imm4(vm);
	_vm_push_word(vm, conv.ui);
	printf("pushl: pushed %" PRIu32 "\n", conv.ui);
	DISPATCH();
	
exec_pushs:;	// push 2 bytes onto the stack
	conv.us = vm_get_imm2(vm);
#ifdef SAFEMODE
	if( vm->sp+2 >= STK_SIZE ) {
		printf("exec_pushs reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp+2);
		goto *dispatch[halt];
	}
#endif
	//conv.c[0] = code[++vm->ip];
	//conv.c[1] = code[++vm->ip];
	vm->pbDataStack[++vm->sp] = conv.c[0];
	vm->pbDataStack[++vm->sp] = conv.c[1];
	printf("pushs: pushed %" PRIu32 "\n", conv.us);
	DISPATCH();
	
exec_pushb:;	// push a byte onto the stack
#ifdef SAFEMODE
	if( vm->sp+1 >= STK_SIZE ) {
		printf("exec_pushb reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp+1);
		goto *dispatch[halt];
	}
#endif
	vm->pbDataStack[++vm->sp] = vm->pInstrStream[++vm->ip];
	printf("pushb: pushed %" PRIu32 "\n", vm->pbDataStack[vm->sp]);
	DISPATCH();
	
exec_pushsp:;	// push sp onto the stack, uses 4 bytes since 'sp' is uint32
	conv.ui = vm->sp;
	_vm_push_word(vm, conv.ui);
	printf("pushsp: pushed sp index: %" PRIu32 "\n", conv.ui);
	DISPATCH();
	
exec_puship:;
	conv.ui = vm->ip;
	_vm_push_word(vm, conv.ui);
	printf("puship: pushed ip index: %" PRIu32 "\n", conv.ui);
	DISPATCH();
	
exec_popl:;		// pop 4 bytes to eventually be overwritten
#ifdef SAFEMODE
	if( vm->sp-4 >= STK_SIZE ) {
		printf("exec_popl reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-4);
		goto *dispatch[halt];
	}
#endif
	vm->sp -= 4;
	printf("popl\n");
	DISPATCH();
	
exec_pops:;		// pop 2 bytes
#ifdef SAFEMODE
	if( vm->sp-2 >= STK_SIZE ) {
		printf("exec_pops reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-2);
		goto *dispatch[halt];
	}
#endif
	vm->sp -= 2;
	printf("pops\n");
	DISPATCH();
	
exec_popb:;		// pop a byte
#ifdef SAFEMODE
	if( vm->sp-1 >= STK_SIZE ) {
		printf("exec_popb reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-1);
		goto *dispatch[halt];
	}
#endif
	--vm->sp;
	printf("popb\n");
	DISPATCH();

exec_popsp:;
#ifdef SAFEMODE
	if( vm->sp-1 >= STK_SIZE ) {
		printf("exec_popsp reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-1);
		goto *dispatch[halt];
	}
#endif
	vm->sp = vm->pbDataStack[vm->sp];
	printf("popsp: sp is now %" PRIu32 " bytes.\n", vm->sp);
	DISPATCH();

exec_popip:;
#ifdef SAFEMODE
	if( vm->sp-1 >= STK_SIZE ) {
		printf("exec_popip reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-1);
		goto *dispatch[halt];
	}
#endif
	vm->ip = vm->pbDataStack[vm->sp--];
	printf("popip: ip is now %" PRIu32 " bytes.\n", vm->ip);
	goto *dispatch[ vm->pInstrStream[vm->ip] ];
	
exec_wrtl:;	// writes an int to memory, First operand is the memory address as 4 byte number, second is the int of data.
	a = vm_get_imm4(vm);
#ifdef SAFEMODE
	if( a > MEM_SIZE-4 ) {
		printf("exec_wrtl reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, a+3);
		goto *dispatch[halt];
	}
#endif
	vm->pbMemory[a] = vm->pInstrStream[++vm->ip];
	vm->pbMemory[a+1] = vm->pInstrStream[++vm->ip];
	vm->pbMemory[a+2] = vm->pInstrStream[++vm->ip];
	vm->pbMemory[a+3] = vm->pInstrStream[++vm->ip];
	conv.c[0] = vm->pbMemory[a];
	conv.c[1] = vm->pbMemory[a+1];
	conv.c[2] = vm->pbMemory[a+2];
	conv.c[3] = vm->pbMemory[a+3];
	printf("wrote int data - %" PRIu32 " @ address 0x%x\n", conv.ui, a);
	DISPATCH();
	
exec_wrts:;	// writes a short to memory. First operand is the memory address as 4 byte number, second is the short of data.
	a = vm_get_imm4(vm);
#ifdef SAFEMODE
	if( a > MEM_SIZE-2 ) {
		printf("exec_wrts reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, a);
		goto *dispatch[halt];
	}
#endif
	vm->pbMemory[a] = vm->pInstrStream[++vm->ip];
	vm->pbMemory[a+1] = vm->pInstrStream[++vm->ip];
	conv.c[0] = vm->pbMemory[a];
	conv.c[1] = vm->pbMemory[a+1];
	printf("wrote short data - %" PRIu32 " @ address 0x%x\n", conv.us, a);
	DISPATCH();
	
exec_wrtb:;	// writes a byte to memory. First operand is the memory address as 32-bit number, second is the byte of data.
	conv.ui = vm_get_imm4(vm);
#ifdef SAFEMODE
	if( conv.ui >= MEM_SIZE ) {
		printf("exec_wrtb reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, conv.ui);
		goto *dispatch[halt];
	}
#endif
	vm->pbMemory[conv.ui] = vm->pInstrStream[++vm->ip];
	printf("wrote byte data - %" PRIu32 " @ address 0x%x\n", vm->pbMemory[conv.ui], conv.ui);
	DISPATCH();
	
exec_storel:;	// pops 4-byte value off stack and into a memory address.
	a = vm_get_imm4(vm);
#ifdef SAFEMODE
	if( a >= MEM_SIZE-4 ) {
		printf("exec_storel reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, a);
		goto *dispatch[halt];
	}
	else if( vm->sp-4 >= STK_SIZE ) {
		printf("exec_storel reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-4);
		goto *dispatch[halt];
	}
#endif
	vm->pbMemory[a+3] = vm->pbDataStack[vm->sp--];
	vm->pbMemory[a+2] = vm->pbDataStack[vm->sp--];
	vm->pbMemory[a+1] = vm->pbDataStack[vm->sp--];
	vm->pbMemory[a] = vm->pbDataStack[vm->sp--];
	conv.c[0] = vm->pbMemory[a];
	conv.c[1] = vm->pbMemory[a+1];
	conv.c[2] = vm->pbMemory[a+2];
	conv.c[3] = vm->pbMemory[a+3];
	printf("stored int data - %" PRIu32 " @ address 0x%x\n", conv.ui, a);
	DISPATCH();
	
exec_stores:;	// pops 2-byte value off stack and into a memory address.
	a = vm_get_imm4(vm);
#ifdef SAFEMODE
	if( a > MEM_SIZE-2 ) {
		printf("exec_stores reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, a);
		goto *dispatch[halt];
	}
	else if( vm->sp-2 >= STK_SIZE ) {
		printf("exec_stores reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-2);
		goto *dispatch[halt];
	}
#endif
	vm->pbMemory[a+1] = vm->pbDataStack[vm->sp--];
	vm->pbMemory[a] = vm->pbDataStack[vm->sp--];
	conv.c[0] = vm->pbMemory[a];
	conv.c[1] = vm->pbMemory[a+1];
	printf("stored short data - %" PRIu32 " @ address 0x%x\n", conv.us, a);
	DISPATCH();
	
exec_storeb:;	// pops byte value off stack and into a memory address.
	a = vm_get_imm4(vm);
#ifdef SAFEMODE
	if( a >= MEM_SIZE ) {
		printf("exec_storeb reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, a);
		goto *dispatch[halt];
	}
	else if( vm->sp-1 >= STK_SIZE ) {
		printf("exec_storeb reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-1);
		goto *dispatch[halt];
	}
#endif
	vm->pbMemory[a] = vm->pbDataStack[vm->sp--];
	conv.c[0] = vm->pbMemory[a];
	printf("stored byte data - %" PRIu32 " @ address 0x%x\n", conv.c[0], a);
	DISPATCH();

/*
 * pushl <value to store>
 * loadl <ptr address>
 * storela
*/
exec_storela:;
	a = _vm_pop_word(vm);
#ifdef SAFEMODE
	if( a > MEM_SIZE-4 ) {
		printf("exec_storela reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, a);
		goto *dispatch[halt];
	}
	else if( vm->sp-4 >= STK_SIZE ) {
		printf("exec_storela reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-4);
		goto *dispatch[halt];
	}
#endif
	vm->pbMemory[a+3] = vm->pbDataStack[vm->sp--];
	vm->pbMemory[a+2] = vm->pbDataStack[vm->sp--];
	vm->pbMemory[a+1] = vm->pbDataStack[vm->sp--];
	vm->pbMemory[a] = vm->pbDataStack[vm->sp--];
	conv.c[0] = vm->pbMemory[a];
	conv.c[1] = vm->pbMemory[a+1];
	conv.c[2] = vm->pbMemory[a+2];
	conv.c[3] = vm->pbMemory[a+3];
	printf("stored 4 byte data - %" PRIu32 " from pointer address 0x%x\n", conv.ui, a);
	DISPATCH();
	
exec_storesa:;
	a = _vm_pop_word(vm);
#ifdef SAFEMODE
	if( a > MEM_SIZE-2 ) {
		printf("exec_storesa reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, a);
		goto *dispatch[halt];
	}
	else if( vm->sp-2 >= STK_SIZE ) {
		printf("exec_storesa reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-2);
		goto *dispatch[halt];
	}
#endif
	vm->pbMemory[a+1] = vm->pbDataStack[vm->sp--];
	vm->pbMemory[a] = vm->pbDataStack[vm->sp--];
	conv.c[0] = vm->pbMemory[a];
	conv.c[1] = vm->pbMemory[a+1];
	printf("stored 2 byte data - %" PRIu32 " from pointer address 0x%x\n", conv.us, a);
	DISPATCH();
	
exec_storeba:;
	a = _vm_pop_word(vm);
#ifdef SAFEMODE
	if( a >= MEM_SIZE ) {
		printf("exec_storeba reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, a);
		goto *dispatch[halt];
	}
	else if( vm->sp-1 >= STK_SIZE ) {
		printf("exec_storeba reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-1);
		goto *dispatch[halt];
	}
#endif
	vm->pbMemory[a] = vm->pbDataStack[vm->sp--];
	conv.c[0] = vm->pbMemory[a];
	printf("stored byte - %" PRIu32 " from pointer address 0x%x\n", conv.c[0], a);
	DISPATCH();
	
exec_loadl:;
	a = vm_get_imm4(vm);
#ifdef SAFEMODE
	if( a > MEM_SIZE-4 ) {
		printf("exec_loadl reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, a);
		goto *dispatch[halt];
	}
	else if( vm->sp+4 >= STK_SIZE ) {
		printf("exec_loadl reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp+4);
		goto *dispatch[halt];
	}
#endif
	vm->pbDataStack[++vm->sp] = vm->pbMemory[a];
	vm->pbDataStack[++vm->sp] = vm->pbMemory[a+1];
	vm->pbDataStack[++vm->sp] = vm->pbMemory[a+2];
	vm->pbDataStack[++vm->sp] = vm->pbMemory[a+3];
	conv.c[0] = vm->pbMemory[a];
	conv.c[1] = vm->pbMemory[a+1];
	conv.c[2] = vm->pbMemory[a+2];
	conv.c[3] = vm->pbMemory[a+3];
	printf("loaded int data to T.O.S. - %" PRIu32 " from address 0x%x\n", conv.ui, a);
	DISPATCH();
	
exec_loads:;
	a = vm_get_imm4(vm);
#ifdef SAFEMODE
	if( a > MEM_SIZE-2 ) {
		printf("exec_loads reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, a);
		goto *dispatch[halt];
	}
	else if( vm->sp+2 >= STK_SIZE ) {
		printf("exec_loads reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp+2);
		goto *dispatch[halt];
	}
#endif
	vm->pbDataStack[++vm->sp] = vm->pbMemory[a];
	vm->pbDataStack[++vm->sp] = vm->pbMemory[a+1];
	conv.c[0] = vm->pbMemory[a];
	conv.c[1] = vm->pbMemory[a+1];
	printf("loaded short data to T.O.S. - %" PRIu32 " from address 0x%x\n", conv.us, a);
	DISPATCH();
	
exec_loadb:;
	a = vm_get_imm4(vm);
#ifdef SAFEMODE
	if( a >= MEM_SIZE ) {
		printf("exec_loadb reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, a);
		goto *dispatch[halt];
	}
	else if( vm->sp+1 >= STK_SIZE ) {
		printf("exec_loadb reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp+1);
		goto *dispatch[halt];
	}
#endif
	vm->pbDataStack[++vm->sp] = vm->pbMemory[a];
	printf("loaded byte data to T.O.S. - %" PRIu32 " from address 0x%x\n", vm->pbDataStack[vm->sp], a);
	DISPATCH();

exec_loadla:;
	a = _vm_pop_word(vm);
#ifdef SAFEMODE
	if( a > MEM_SIZE-4 ) {
		printf("exec_loadla reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, a);
		goto *dispatch[halt];
	}
	else if( vm->sp+4 >= STK_SIZE ) {
		printf("exec_loadla reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp+4);
		goto *dispatch[halt];
	}
#endif
	vm->pbDataStack[++vm->sp] = vm->pbMemory[a];
	vm->pbDataStack[++vm->sp] = vm->pbMemory[a+1];
	vm->pbDataStack[++vm->sp] = vm->pbMemory[a+2];
	vm->pbDataStack[++vm->sp] = vm->pbMemory[a+3];
	conv.c[0] = vm->pbMemory[a];
	conv.c[1] = vm->pbMemory[a+1];
	conv.c[2] = vm->pbMemory[a+2];
	conv.c[3] = vm->pbMemory[a+3];
	printf("loaded 4 byte data to T.O.S. - %" PRIu32 " from pointer address 0x%x\n", conv.ui, a);
	DISPATCH();
	
exec_loadsa:;
	a = _vm_pop_word(vm);
#ifdef SAFEMODE
	if( a > MEM_SIZE-2 ) {
		printf("exec_loadsa reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, a);
		goto *dispatch[halt];
	}
	else if( vm->sp+2 >= STK_SIZE ) {
		printf("exec_loadsa reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp+2);
		goto *dispatch[halt];
	}
#endif
	vm->pbDataStack[++vm->sp] = vm->pbMemory[a];
	vm->pbDataStack[++vm->sp] = vm->pbMemory[a+1];
	conv.c[0] = vm->pbMemory[a];
	conv.c[1] = vm->pbMemory[a+1];
	printf("loaded 2 byte data to T.O.S. - %" PRIu32 " from pointer address 0x%x\n", conv.us, a);
	DISPATCH();
	
exec_loadba:;
	a = _vm_pop_word(vm);
#ifdef SAFEMODE
	if( a > MEM_SIZE ) {
		printf("exec_loadba reported: Invalid Memory Access! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\nInvalid Memory Address: %" PRIu32 "\n", vm->ip, vm->sp, a);
		goto *dispatch[halt];
	}
	else if( vm->sp >= STK_SIZE ) {
		printf("exec_loadba reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp);
		goto *dispatch[halt];
	}
#endif
	vm->pbDataStack[++vm->sp] = vm->pbMemory[a];
	conv.c[0] = vm->pbMemory[a];
	printf("loaded byte data to T.O.S. - %" PRIu32 " from pointer address 0x%x\n", vm->pbDataStack[vm->sp], a);
	DISPATCH();
	
exec_copyl:;	// copy 4 bytes of top of stack and put as new top of stack.
#ifdef SAFEMODE
	if( vm->sp-3 >= STK_SIZE ) {
		printf("exec_copyl reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-3);
		goto *dispatch[halt];
	}
#endif
	conv.c[0] = vm->pbDataStack[vm->sp-3];
	conv.c[1] = vm->pbDataStack[vm->sp-2];
	conv.c[2] = vm->pbDataStack[vm->sp-1];
	conv.c[3] = vm->pbDataStack[vm->sp];
	printf("copied int data from T.O.S. - %" PRIu32 "\n", conv.ui);
	_vm_push_word(vm, conv.ui);
	DISPATCH();
	
exec_copys:;
#ifdef SAFEMODE
	if( vm->sp-1 >= STK_SIZE ) {
		printf("exec_copys reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-1);
		goto *dispatch[halt];
	}
#endif
	conv.c[0] = vm->pbDataStack[vm->sp-1];
	conv.c[1] = vm->pbDataStack[vm->sp];
	vm->pbDataStack[++vm->sp] = conv.c[0];
	vm->pbDataStack[++vm->sp] = conv.c[1];
	printf("copied short data from T.O.S. - %" PRIu32 "\n", conv.us);
	DISPATCH();
	
exec_copyb:;
#ifdef SAFEMODE
	if( vm->sp+1 >= STK_SIZE ) {
		printf("exec_copys reported: stack overflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp+1);
		goto *dispatch[halt];
	}
#endif
	conv.c[0] = vm->pbDataStack[vm->sp];
	vm->pbDataStack[++vm->sp] = conv.c[0];
	printf("copied byte data from T.O.S. - %" PRIu32 "\n", conv.c[0]);
	DISPATCH();
	
exec_addl:;		// pop 8 bytes, signed addition, and push 4 byte result to top of stack
	b = _vm_pop_word(vm);
	a = _vm_pop_word(vm);
	conv.i = (int)a + (int)b;
	printf("signed 4 byte addition result: %i == %i + %i\n", conv.i, a,b);
	_vm_push_word(vm, conv.ui);
	DISPATCH();
	
exec_uaddl:;	// In C, all integers in an expression are promoted to int32, if number is bigger then uint32 or int64
	b = _vm_pop_word(vm);
	a = _vm_pop_word(vm);
	conv.ui = a+b;
	printf("unsigned 4 byte addition result: %u == %u + %u\n", conv.ui, a,b);
	_vm_push_word(vm, conv.ui);
	DISPATCH();
	
exec_addf:;
	fb = _vm_pop_float32(vm);
	fa = _vm_pop_float32(vm);
	conv.f = fa+fb;
	printf("float addition result: %f == %f + %f\n", conv.f, fa,fb);
	_vm_push_float(vm, conv.f);
	DISPATCH();
	
exec_subl:;
	b = _vm_pop_word(vm);
	a = _vm_pop_word(vm);
	conv.i = (int)a - (int)b;
	printf("signed 4 byte subtraction result: %i == %i - %i\n", conv.ui, a,b);
	_vm_push_word(vm, conv.ui);
	DISPATCH();
	
exec_usubl:;
	b = _vm_pop_word(vm);
	a = _vm_pop_word(vm);
	conv.ui = a-b;
	printf("unsigned 4 byte subtraction result: %" PRIu32 " == %" PRIu32 " - %" PRIu32 "\n", conv.ui, a,b);
	_vm_push_word(vm, conv.ui);
	DISPATCH();
	
exec_subf:;
	fb = _vm_pop_float32(vm);
	fa = _vm_pop_float32(vm);
	conv.f = fa-fb;
	printf("float subtraction result: %f == %f - %f\n", conv.f, fa,fb);
	_vm_push_float(vm, conv.f);
	DISPATCH();
	
exec_mull:;
	b = _vm_pop_word(vm);
	a = _vm_pop_word(vm);
	conv.i = (int)a * (int)b;
	printf("signed 4 byte mult result: %i == %i * %i\n", conv.i, a,b);
	_vm_push_word(vm, conv.ui);
	DISPATCH();
	
exec_umull:;
	b = _vm_pop_word(vm);
	a = _vm_pop_word(vm);
	conv.ui = a*b;
	printf("unsigned 4 byte mult result: %" PRIu32 " == %" PRIu32 " * %" PRIu32 "\n", conv.ui, a,b);
	_vm_push_word(vm, conv.ui);
	DISPATCH();
	
exec_mulf:;
	fb = _vm_pop_float32(vm);
	fa = _vm_pop_float32(vm);
	conv.f = fa*fb;
	printf("float mul result: %f == %f * %f\n", conv.f, fa,fb);
	_vm_push_float(vm, conv.f);
	DISPATCH();
	
exec_divl:;
	b = _vm_pop_word(vm);
	if( !b ) {
		printf("divl: divide by 0 error.\n");
		goto *dispatch[halt];
	}
	a = _vm_pop_word(vm);
	conv.i = (int)a / (int)b;
	printf("signed 4 byte division result: %i == %i / %i\n", conv.i, a,b);
	_vm_push_word(vm, conv.ui);
	DISPATCH();
	
exec_udivl:;
	b = _vm_pop_word(vm);
	if( !b ) {
		printf("udivl: divide by 0 error.\n");
		goto *dispatch[halt];
	}
	a = _vm_pop_word(vm);
	conv.ui = a/b;
	printf("unsigned 4 byte division result: %" PRIu32 " == %" PRIu32 " / %" PRIu32 "\n", conv.ui, a,b);
	_vm_push_word(vm, conv.ui);
	DISPATCH();
	
exec_divf:;
	fb = _vm_pop_float32(vm);
	if( !fb ) {
		printf("divf: divide by 0.0 error.\n");
		goto *dispatch[halt];
	}
	fa = _vm_pop_float32(vm);
	conv.f = fa/fb;
	printf("float division result: %f == %f / %f\n", conv.f, fa,fb);
	_vm_push_float(vm, conv.f);
	DISPATCH();
	
exec_modl:;
	b = _vm_pop_word(vm);
	if( !b ) {
		printf("modl: divide by 0 error.\n");
		goto *dispatch[halt];
	}
	a = _vm_pop_word(vm);
	conv.i = (int)a % (int)b;
	printf("signed 4 byte modulo result: %i == %i %% %i\n", conv.i, a,b);
	_vm_push_word(vm, conv.ui);
	DISPATCH();
	
exec_umodl:;
	b = _vm_pop_word(vm);
	if( !b ) {
		printf("umodl: divide by 0 error.\n");
		goto *dispatch[halt];
	}
	a = _vm_pop_word(vm);
	conv.ui = a%b;
	printf("unsigned 4 byte modulo result: %" PRIu32 " == %" PRIu32 " %% %" PRIu32 "\n", conv.ui, a,b);
	_vm_push_word(vm, conv.ui);
	DISPATCH();
	
exec_andl:;
	b = _vm_pop_word(vm);
	a = _vm_pop_word(vm);
	conv.ui = a & b;
	printf("4 byte AND result: %" PRIu32 " == %u & %u\n", conv.ui, a,b);
	_vm_push_word(vm, conv.ui);
	DISPATCH();
	
exec_orl:;
	b = _vm_pop_word(vm);
	a = _vm_pop_word(vm);
	conv.ui = a | b;
	printf("4 byte OR result: %" PRIu32 " == %u | %u\n", conv.ui, a,b);
	_vm_push_word(vm, conv.ui);
	DISPATCH();
	
exec_xorl:;
	b = _vm_pop_word(vm);
	a = _vm_pop_word(vm);
	conv.ui = a ^ b;
	printf("4 byte XOR result: %" PRIu32 " == %u ^ %u\n", conv.ui, a,b);
	_vm_push_word(vm, conv.ui);
	DISPATCH();
	
exec_notl:;
	a = _vm_pop_word(vm);
	conv.ui = ~a;
	printf("4 byte NOT result: %" PRIu32 "\n", conv.ui);
	_vm_push_word(vm, conv.ui);
	DISPATCH();
	
exec_shll:;
	b = _vm_pop_word(vm);
	a = _vm_pop_word(vm);
	conv.ui = a << b;
	printf("4 byte Shift Left result: %" PRIu32 " == %u << %u\n", conv.ui, a,b);
	_vm_push_word(vm, conv.ui);
	DISPATCH();
	
exec_shrl:;
	b = _vm_pop_word(vm);
	a = _vm_pop_word(vm);
	conv.ui = a >> b;
	printf("4 byte Shift Right result: %" PRIu32 " == %u >> %u\n", conv.ui, a,b);
	_vm_push_word(vm, conv.ui);
	DISPATCH();
	
exec_incl:;
	a = _vm_pop_word(vm);
	conv.ui = ++a;
	printf("4 byte Increment result: %" PRIu32 "\n", conv.ui);
	_vm_push_word(vm, conv.ui);
	DISPATCH();
	
exec_decl:;
	a = _vm_pop_word(vm);
	conv.ui = --a;
	printf("4 byte Decrement result: %" PRIu32 "\n", conv.ui);
	_vm_push_word(vm, conv.ui);
	DISPATCH();

exec_negl:;
	a = _vm_pop_word(vm);
	conv.ui = -a;
	printf("4 byte Decrement result: %" PRIu32 "\n", conv.ui);
	_vm_push_word(vm, conv.ui);
	DISPATCH();

exec_ltl:;
	b = _vm_pop_word(vm);
	a = _vm_pop_word(vm);
	conv.ui = (int)a < (int)b;
	printf("4 byte Signed Less Than result: %" PRIu32 " == %i < %i\n", conv.ui, a,b);
	_vm_push_word(vm, conv.ui);
	DISPATCH();
	
exec_ultl:;
	b = _vm_pop_word(vm);
	a = _vm_pop_word(vm);
	conv.ui = a < b;
	printf("4 byte Unsigned Less Than result: %" PRIu32 " == %" PRIu32 " < %" PRIu32 "\n", conv.ui, a,b);
	_vm_push_word(vm, conv.ui);
	DISPATCH();
	
exec_ltf:;
	fb = _vm_pop_float32(vm);
	fa = _vm_pop_float32(vm);
	conv.ui = fa < fb;
	printf("4 byte Less Than Float result: %" PRIu32 " == %f < %f\n", conv.ui, fa,fb);
	_vm_push_float(vm, conv.f);
	DISPATCH();
	
exec_gtl:;
	b = _vm_pop_word(vm);
	a = _vm_pop_word(vm);
	conv.ui = (int)a > (int)b;
	printf("4 byte Signed Greater Than result: %" PRIu32 " == %i > %i\n", conv.ui, a,b);
	_vm_push_word(vm, conv.ui);
	DISPATCH();
	
exec_ugtl:;
	b = _vm_pop_word(vm);
	a = _vm_pop_word(vm);
	conv.ui = a > b;
	printf("4 byte Signed Greater Than result: %" PRIu32 " == %" PRIu32 " > %" PRIu32 "\n", conv.ui, a,b);
	_vm_push_word(vm, conv.ui);
	DISPATCH();
	
exec_gtf:;
	fb = _vm_pop_float32(vm);
	fa = _vm_pop_float32(vm);
	conv.ui = fa > fb;
	printf("4 byte Greater Than Float result: %" PRIu32 " == %f > %f\n", conv.ui, fa,fb);
	_vm_push_float(vm, conv.f);
	DISPATCH();
	
exec_cmpl:;
	b = _vm_pop_word(vm);
	a = _vm_pop_word(vm);
	conv.ui = (int)a == (int)b;
	printf("4 byte Signed Compare result: %" PRIu32 " == %i == %i\n", conv.ui, a,b);
	_vm_push_word(vm, conv.ui);
	DISPATCH();
	
exec_ucmpl:;
	b = _vm_pop_word(vm);
	a = _vm_pop_word(vm);
	conv.ui = a == b;
	printf("4 byte Unsigned Compare result: %" PRIu32 " == %" PRIu32 " == %" PRIu32 "\n", conv.ui, a,b);
	_vm_push_word(vm, conv.ui);
	DISPATCH();
	
exec_compf:;
	fb = _vm_pop_float32(vm);
	fa = _vm_pop_float32(vm);
	conv.ui = fa == fb;
	printf("4 byte Compare Float result: %" PRIu32 " == %f == %f\n", conv.ui, fa,fb);
	_vm_push_float(vm, conv.f);
	DISPATCH();
	
exec_leql:;
	b = _vm_pop_word(vm);
	a = _vm_pop_word(vm);
	conv.ui = (int)a <= (int)b;
	printf("4 byte Signed Less Equal result: %" PRIu32 " == %i <= %i\n", conv.ui, a,b);
	_vm_push_word(vm, conv.ui);
	DISPATCH();
	
exec_uleql:;
	b = _vm_pop_word(vm);
	a = _vm_pop_word(vm);
	conv.ui = a <= b;
	printf("4 byte Unsigned Less Equal result: %" PRIu32 " == %" PRIu32 " <= %" PRIu32 "\n", conv.ui, a,b);
	_vm_push_word(vm, conv.ui);
	DISPATCH();
	
exec_leqf:;
	fb = _vm_pop_float32(vm);
	fa = _vm_pop_float32(vm);
	conv.ui = fa <= fb;
	printf("4 byte Less Equal Float result: %" PRIu32 " == %f <= %f\n", conv.ui, fa, fb);
	_vm_push_float(vm, conv.f);
	DISPATCH();
	
exec_geql:;
	b = _vm_pop_word(vm);
	a = _vm_pop_word(vm);
	conv.ui = (int)a >= (int)b;
	printf("4 byte Signed Greater Equal result: %" PRIu32 " == %i >= %i\n", conv.ui, a,b);
	_vm_push_word(vm, conv.ui);
	DISPATCH();
	
exec_ugeql:;
	b = _vm_pop_word(vm);
	a = _vm_pop_word(vm);
	conv.ui = a >= b;
	printf("4 byte Unsigned Greater Equal result: %" PRIu32 " == %" PRIu32 " >= %" PRIu32 "\n", conv.ui, a,b);
	_vm_push_word(vm, conv.ui);
	DISPATCH();
	
exec_geqf:;
	fb = _vm_pop_float32(vm);
	fa = _vm_pop_float32(vm);
	conv.ui = fa >= fb;
	printf("4 byte Greater Equal Float result: %" PRIu32 " == %f >= %f\n", conv.ui, fa, fb);
	_vm_push_float(vm, conv.f);
	DISPATCH();
	
exec_jmp:;		// addresses are word sized bytes.
	conv.ui = vm_get_imm4(vm);
	vm->ip = conv.ui;
	printf("jmping to instruction address: %" PRIu32 "\n", vm->ip);
	goto *dispatch[ vm->pInstrStream[vm->ip] ];
	
exec_jzl:;		// check if the first 4 bytes on stack are zero, if yes then jump it.
#ifdef SAFEMODE
	if( vm->sp-3 >= STK_SIZE ) {
		printf("exec_jzl reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-3);
		goto *dispatch[halt];
	}
#endif
	conv.c[3] = vm->pbDataStack[vm->sp];
	conv.c[2] = vm->pbDataStack[vm->sp-1];
	conv.c[1] = vm->pbDataStack[vm->sp-2];
	conv.c[0] = vm->pbDataStack[vm->sp-3];
	a = conv.ui;
	conv.ui = vm_get_imm4(vm);
	vm->ip = (!a) ? conv.ui : vm->ip+1 ;
	printf("jzl'ing to instruction address: %" PRIu32 "\n", vm->ip);	//opcode2str[vm->pInstrStream[vm->ip]]
	goto *dispatch[ vm->pInstrStream[vm->ip] ];
	
exec_jnzl:;
#ifdef SAFEMODE
	if( vm->sp-3 >= STK_SIZE ) {
		printf("exec_jnzl reported: stack underflow! Current instruction address: %" PRIu32 " | Stack index: %" PRIu32 "\n", vm->ip, vm->sp-3);
		goto *dispatch[halt];
	}
#endif
	conv.c[3] = vm->pbDataStack[vm->sp];
	conv.c[2] = vm->pbDataStack[vm->sp-1];
	conv.c[1] = vm->pbDataStack[vm->sp-2];
	conv.c[0] = vm->pbDataStack[vm->sp-3];
	a = conv.ui;
	conv.ui = vm_get_imm4(vm);
	vm->ip = (a) ? conv.ui : vm->ip+1 ;
	printf("jnzl'ing to instruction address: %" PRIu32 "\n", vm->ip);
	goto *dispatch[ vm->pInstrStream[vm->ip] ];
	
exec_call:;
	conv.ui = vm_get_imm4(vm);
	printf("calling address: %" PRIu32 "\n", conv.ui);
#ifdef SAFEMODE
	if( vm->callsp+1 >= CALLSTK_SIZE ) {
		printf("exec_call reported: callstack overflow! Current instruction address: %" PRIu32 " | Call Stack index: %" PRIu32 "\n", vm->ip, vm->callsp+1);
		goto *dispatch[halt];
	}
#endif
	vm->bCallstack[++vm->callsp] = vm->ip+1;
	//vm->bp = vm->callsp;
	vm->ip = conv.ui;
	printf("call return addr: %" PRIu32 "\n", vm->bCallstack[vm->callsp]);
	goto *dispatch[ vm->pInstrStream[vm->ip] ];
	
exec_ret:;
	//vm->callsp = vm->bp;
	//printf("callsp set to bp, callsp == %" PRIu32 "\n", vm->callsp);
#ifdef SAFEMODE
	if( vm->callsp-1 >= CALLSTK_SIZE ) {
		printf("exec_ret reported: callstack underflow! Current instruction address: %" PRIu32 " | Call Stack index: %" PRIu32 "\n", vm->ip, vm->callsp-1);
		goto *dispatch[halt];
	}
#endif
	vm->ip = vm->bCallstack[vm->callsp--];
	//vm->bp = vm->callsp;
	printf("returning to address: %" PRIu32 "\n", vm->ip);
	goto *dispatch[ vm->pInstrStream[vm->ip] ];
	
exec_reset:;
	vm_reset(vm);
	return;
}


unsigned long long int get_file_size(FILE *pFile)
{
	unsigned long long int size = 0;
	if( !pFile )
		return size;
	
	if( !fseek(pFile, 0, SEEK_END) ) {
		size = (unsigned long long int)ftell(pFile);
		rewind(pFile);
	}
	return size;
}

int main(void)
{
	/*
	FILE *pFile = fopen("./myfile.original_lang_file_ext", "rb");
	if( !pFile )
		return 0;
	
	unsigned long long int size = get_file_size(pFile);
	uchar *program = malloc(sizeof(uchar)*size);
	fread(program, sizeof(uchar), size, pFile);
	fclose(pFile); pFile=NULL;
	*/
	bytecode test1 = {
		nop,
		//pushl, 255, 1, 0, 0x0,
		//pushs, 0xDD, 0xDD,
		//pushb, 0xAA,
		//pushb, 0xFF,
		wrtl, 0xff,0xff,0xff,0xff, 0,0,0,0,
		halt
	};
	bytecode float_test = {
		nop,
		//jmp, 17,0,0,0,
		// -16776961
		//pushl, 255,0,0,255,
		//pushl, 1,0,0,0,
		// -855637761
		pushl, 255,0,0,205,
		pushl, 255,255,0,0,
		uaddl, popl,
		// 10.f in 4 bytes form
		pushl, 0,0,32,65,
		jzl, 17,0,0,0,
		// 2.f in 4 bytes form
		pushl, 0,0,0,64,
		addf,	// 12.f
		//leqf,
		halt
	};
	bytecode nested_func_calls = {
		nop,
		call, 7,0,0,0,	// 1
		halt,			// 6
	// func1:
		pushl, 9,0,0,0,	// 7
		call, 18,0,0,0,	// 12
		ret,	// 17
	// func2:
		pushl, 5,0,0,0,	// 18
		pushl, 10,0,0,0,	// 23
		mull,	// 28
		mull,
		call, 36,0,0,0,	// 30
		ret,	
	// func3:
		pushl, 40,0,0,0,	// 36
		divl,	// 41
		ret,
	};
	
	// Fibonnaci sequence to test performance!
	/*	int fib(int n)
		{
			int a = 0;
			int b = 1;
			while (n-- > 1) {
				int t = a;
				a = b;
				b += t;
			}
			return b;
		}
	*/
	bytecode fibonacci = {
		nop, // calc fibonnaci number
		wrtl, 0,0,0,0, 30,0,0,0,	// write n to address 0
		call, 16,0,0,0,
		halt,
		// a = 0;
		wrtl, 4,0,0,0, 0,0,0,0,		// 16
		// b = 1;
		wrtl, 8,0,0,0, 1,0,0,0,		// 25
		// while( n-- > 1 ) {
		loadl, 0,0,0,0,		// load param n		// 34
		pushl, 1,0,0,0,		// push 1
		gtl,
		loadl, 0,0,0,0,		// load param n
		decl,				// decrement address 0
		storel, 0,0,0,0,	// store decrement result to memory address
		jzl, 103,0,0,0,		// jmp to storing b and returning.
		popl,
		// int t = a;
		loadl, 4,0,0,0,		// load a's value.
		storel, 12,0,0,0,	// store a's value into another address as 't'
		// a = b;
		loadl, 8,0,0,0,		// load b.
		storel, 4,0,0,0,	// store b's value into a's address.
		// b += t;
		loadl, 12,0,0,0,	// load t.
		loadl, 8,0,0,0,		// load b.
		uaddl,				// add b and t
		storel, 8,0,0,0,	// store addition value to b's address.
		jmp, 34,0,0,0,		// jmp back to start of loop.	// 98
		// }
		ret		// b has been fully 'mathemized' and is stored into memory for reuse.
	};
	
	bytecode hello_world = {
		nop,
		wrtb, 0,0,0,0,	72,		// H
		wrtb, 1,0,0,0,	101,	// e
		wrtb, 2,0,0,0,	108,	// l
		wrtb, 3,0,0,0,	108,	// l
		wrtb, 4,0,0,0,	111,	// o
		wrtb, 5,0,0,0,	32,		// space
		wrtb, 6,0,0,0,	87,		// W
		wrtb, 7,0,0,0,	111,	// o
		wrtb, 8,0,0,0,	114,	// r
		wrtb, 9,0,0,0,	108,	// l
		wrtb, 10,0,0,0,	100,	// d
		halt,
	};
	
	bytecode pointers = {
		nop,
		// The way you wuold store to a pointer would be something like...
		// pushl <value to store>
		// loadl <ptr address>
		// storela
		
		// push 170 to tos.
		pushl,	0xAA,0,0,0,
		// store 170 to address 255 as variable 'i'
		// int i = 170;
		storel,	0xff,0,0,0,
		
		// the way you would load from a pointer would be something like:
		// loadl <ptr address>
		// loadla
		
		// load address of 'i'
		// int *p = &i;
		// *p = 26;		// i == 26;
		pushl,	0x1a,0,0,0,
		pushl, 0xff,0,0,0,
		storela, // pops 4 byte address, then pops 4 byte data into memory address.
		halt
	};
	
	CVM_t *vm = &(CVM_t){ 0 };
	vm_init(vm);
	vm_load_code(vm, test1);				vm_exec(vm); vm_free(vm);
	vm_load_code(vm, float_test);			vm_exec(vm); vm_free(vm);
	vm_load_code(vm, nested_func_calls);	vm_exec(vm); vm_free(vm);
	vm_load_code(vm, fibonacci);			vm_exec(vm); vm_free(vm);
	vm_load_code(vm, pointers);				vm_exec(vm); vm_free(vm);

	vm_load_code(vm, hello_world); vm_exec(vm);
	uchar buffer[12] = {0};
	vm_read_bytearray(vm, buffer, 12, 0x0);
	printf("buffer == \'%s\'\n", buffer);

	//printf("instruction set amount == %u\n", nop);
	vm_debug_print_memory(vm);
	vm_debug_print_stack(vm);
	vm_free(vm);
	//vm_debug_print_callstack(p_vm);
	//vm_debug_print_ptrs(p_vm);
	//free(program); program=NULL;
	return 0;
}
