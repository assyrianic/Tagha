#!/usr/bin/python3

import sys;
import ctypes;
import struct;

# Leave the references empty and build rest of bytecode
# Fill in references afterwards when you know their positions.

def enum(*sequential, **named) -> object:
	enums = dict(zip(sequential, range(len(sequential))), **named);
	return type('Enum', (), enums);


opcodes = enum('halt', 'push', 'pop', 'neg', 'inc', 'dec', 'bnot', 'jmp', 'jz', 'jnz', 'call', 'ret', 'callnat', 'movr', 'movm', 'lea', 'addr', 'addm', 'uaddr', 'uaddm', 'subr', 'subm', 'usubr', 'usubm', 'mulr', 'mulm', 'umulr', 'umulm', 'divr', 'divm', 'udivr', 'udivm', 'modr', 'modm', 'umodr', 'umodm', 'shrr', 'shrm', 'shlr', 'shlm', 'andr', 'andm', 'orr', 'orm', 'xorr', 'xorm', 'ltr', 'ltm', 'ultr', 'ultm', 'gtr', 'gtm', 'ugtr', 'ugtm', 'cmpr', 'cmpm', 'ucmpr', 'ucmpm', 'neqr', 'neqm', 'uneqr', 'uneqm', 'reset', 'int2float', 'int2dbl', 'float2dbl', 'dbl2float','faddr', 'faddm', 'fsubr', 'fsubm', 'fmulr', 'fmulm', 'fdivr', 'fdivm', 'fneg', 'fltr', 'fltm', 'fgtr', 'fgtm', 'fcmpr', 'fcmpm', 'fneqr', 'fneqm', 'nop');

Immediate	= 1;
Register	= 2;
RegIndirect	= 4;
IPRelative	= 8;
Byte		= 16;
TwoBytes	= 32;
FourBytes	= 64;
EightBytes	= 128;

ras=0;
rbs=1;
rcs=2;
rds=3;
res=4;
rfs=5;
rgs=6;
rhs=7;
ris=8;
rjs=9;
rks=10;
rsp=11;
rbp=12;
rip=13;
regsize=14;

def wrt_hdr(f, stacksize:int, datasize:int) -> None:
	f.write(0xC0DE.to_bytes(2, byteorder='little'));
	f.write(stacksize.to_bytes(4, byteorder='little'));
	f.write(datasize.to_bytes(4, byteorder='little'));

def wrt_hdr_natives(f, *natives) -> None:
	i = 0;
	numnatives = len(natives);
	f.write(numnatives.to_bytes(4, byteorder='little'));
	while i<numnatives:
		f.write((len(natives[i])+1).to_bytes(4, byteorder='little'));
		f.write(natives[i].encode('utf-8'));
		f.write(0x0.to_bytes(1, byteorder='little'));
		i += 1;

def wrt_hdr_funcs(f, *funcs) -> None:
	i = 0;
	numfuncs = len(funcs) // 2;
	f.write(numfuncs.to_bytes(4, byteorder='little'));
	while i<numfuncs*2:
		strsize = len(funcs[i])+1;
		f.write(strsize.to_bytes(4, byteorder='little'));
		f.write(funcs[i].encode('utf-8'));
		f.write(0x0.to_bytes(1, byteorder='little'));
		i += 1;
		
		f.write(funcs[i].to_bytes(4, byteorder='little'));
		i += 1;

def wrt_hdr_globals(f, *lGlobals) -> None:
	i = 0;
	numglobals = len(lGlobals) // 3;
	f.write(numglobals.to_bytes(4, byteorder='little'));
	
	while i<numglobals*3:
		strsize = len(lGlobals[i])+1;
		f.write(strsize.to_bytes(4, byteorder='little'));
		f.write(lGlobals[i].encode('utf-8'));
		f.write(0x0.to_bytes(1, byteorder='little'));
		i += 1;
		
		# write the .data address of this global var.
		f.write(lGlobals[i].to_bytes(4, byteorder='little'));
		i += 1;
		
		# write how many bytes this data takes up.
		bytecount = lGlobals[i];
		f.write(lGlobals[i].to_bytes(4, byteorder='little'));
		i += 1;
	

def float32_to_int(val:float) -> int:
	ba = bytearray(struct.pack("f", val));
	i = int.from_bytes(ba, byteorder='little');
	return i;

def float64_to_int(val:float) -> int:
	ba = bytearray(struct.pack("d", val));
	i = int.from_bytes(ba, byteorder='little');
	return i;

def wrt_global_values(f, *lstValues) -> None:
	i = 0;
	while i<len(lstValues):
		if type(lstValues[i]) is str:
			f.write(lstValues[i].encode('utf-8'));
			f.write(0x0.to_bytes(1, byteorder='little'));
		elif type(lstValues[i]) is int:
			val = lstValues[i];
			i += 1;
			f.write(val.to_bytes(lstValues[i], byteorder='little'));
		elif type(lstValues[i]) is float:
			fval = lstValues[i];
			i += 1;
			if lstValues[i] == 4:
				f.write(float32_to_int(fval).to_bytes(4, byteorder='little'));
			else:
				f.write(float64_to_int(fval).to_bytes(8, byteorder='little'));
		i += 1;

def wrt_hdr_flags(f, modes=3) -> None:
	# 1 for safemode, 2 for debugmode, 3 for both.
	f.write(modes.to_bytes(1, byteorder='little'));


def wrt_non_op_code(f, opcode:int, addrmode:int) -> int:
	f.write(opcode.to_bytes(1, byteorder='little'));
	f.write(addrmode.to_bytes(1, byteorder='little'));
	return 2;

def wrt_one_op_code(f, opcode:int, addrmode:int, operand:int, offset=None) -> int:
	InstrAddr = 0;
	f.write(opcode.to_bytes(1, byteorder='little'));
	f.write(addrmode.to_bytes(1, byteorder='little'));
	InstrAddr += 2;
	if operand != None:
		f.write(operand.to_bytes(8, byteorder='little'));
		InstrAddr += 8;
	if offset != None:
		ba = bytearray(struct.pack("i", offset));
		i = int.from_bytes(ba, byteorder='little');
		f.write(i.to_bytes(4, byteorder='little'));
		InstrAddr += 4;
	return InstrAddr;

def wrt_two_op_code(f, opcode:int, addrmode:int, operand1:int, operand2:int, offset=None) -> int:
	InstrAddr = 0;
	f.write(opcode.to_bytes(1, byteorder='little'));
	f.write(addrmode.to_bytes(1, byteorder='little'));
	InstrAddr += 2;
	if operand1 != None:
		f.write(operand1.to_bytes(8, byteorder='little'));
		InstrAddr += 8;
	if operand2 != None:
		f.write(operand2.to_bytes(8, byteorder='little'));
		InstrAddr += 8;
	if offset != None:
		ba = bytearray(struct.pack("i", offset));
		i = int.from_bytes(ba, byteorder='little');
		f.write(i.to_bytes(4, byteorder='little'));
		InstrAddr += 4;
	return InstrAddr;

def wrt_callnat(f, addrmode:int, argcount:int, operand:int, offset=None) -> int:
	InstrAddr = 0;
	f.write(opcodes.callnat.to_bytes(1, byteorder='little'));
	f.write(addrmode.to_bytes(1, byteorder='little'));
	InstrAddr += 2;
	if operand != None:
		f.write(operand.to_bytes(8, byteorder='little'));
		InstrAddr += 8;
	if offset != None:
		ba = bytearray(struct.pack("i", offset));
		i = int.from_bytes(ba, byteorder='little');
		f.write(i.to_bytes(4, byteorder='little'));
		InstrAddr += 4;
	f.write(argcount.to_bytes(4, byteorder='little'));
	InstrAddr += 4;
	return InstrAddr;

'''
unsigned i = 0x0a0b0c0d;
int main()
{
	return 0;
}
'''
with open('test_endian.tbc', 'wb+') as tbc:
	wrt_hdr(tbc, 128, 4);
	wrt_hdr_natives(tbc);
	wrt_hdr_funcs(tbc, 'main', 0);
	wrt_hdr_globals(tbc, 'i', 0, 4);
	wrt_hdr_flags(tbc);
	wrt_global_values(tbc, 0x0a0b0c0d, 4);
# main:
	# movr ras, 0
	# ret
	wrt_two_op_code(tbc, opcodes.movr, Immediate, ras, 0);
	wrt_non_op_code(tbc, opcodes.ret, 0);


'''
int main()
{
	float f = 2.f;
	f += 4.f;
	return 0;
}
'''
with open('test_floatops.tbc', 'wb+') as tbc:
	wrt_hdr(tbc, 128, 0);
	wrt_hdr_natives(tbc);
	wrt_hdr_funcs(tbc, 'main', 0);
	wrt_hdr_globals(tbc);
	wrt_hdr_flags(tbc);
	wrt_global_values(tbc);
	
# main:
	# mov rhs, 2.f
	wrt_two_op_code(tbc, opcodes.movr, Immediate, rhs, float32_to_int(2.0));
	# mov rhs, 4.f
	wrt_two_op_code(tbc, opcodes.movr, Immediate, rfs, float32_to_int(4.0));
	
	# extend rhs and rfs to double.
	# all register-based arithmetic uses 64-bit ints/floats.
	# to use smaller sizes, you have to use pointers.
	wrt_one_op_code(tbc, opcodes.float2dbl, Register, rhs);
	wrt_one_op_code(tbc, opcodes.float2dbl, Register, rfs);
	
	# fadd rfs, rhs
	wrt_two_op_code(tbc, opcodes.faddr, Register, rfs, rhs);
	
	# truncate rfs to float.
	wrt_one_op_code(tbc, opcodes.dbl2float, Register, rfs);
	
	# movr ras, 0
	# ret
	wrt_two_op_code(tbc, opcodes.movr, Immediate, ras, 0);
	wrt_non_op_code(tbc, opcodes.ret, 0);


'''
int main()
{
	int i = 5;
	int *p = &i;
	*p = 127;
}
'''
with open('test_pointers.tbc', 'wb+') as tbc:
	wrt_hdr(tbc, 128, 0);
	wrt_hdr_natives(tbc);
	wrt_hdr_funcs(tbc, 'main', 0);
	wrt_hdr_globals(tbc);
	wrt_hdr_flags(tbc);
	wrt_global_values(tbc);
	
# main:
# int i = 5;
	# sub rsp, 16 | subq 16, %rsp
	wrt_two_op_code(tbc, opcodes.usubr, Immediate, rsp, 16);
	
	# mov DWORD PTR [rbp-12], 5 | movl 5, -12(%rbp)
	wrt_two_op_code(tbc, opcodes.movm, RegIndirect|Immediate|FourBytes, rbp, 5, -12); # 12 is offset.
	
# int *p = &i;
	# lea ras, [rbp-12] | leaq -12(%rbp), %ras
	wrt_two_op_code(tbc, opcodes.lea, RegIndirect, ras, rbp, -12);
	
	# mov QWORD PTR [rbp-8], ras | movq %ras, -8(%rbp)
	wrt_two_op_code(tbc, opcodes.movm, RegIndirect|Register|EightBytes, rbp, ras, -8);
	
# *p = 127;
	# mov ras, QWORD PTR [rbp-8] | movq -8(%rbp), %ras
	wrt_two_op_code(tbc, opcodes.movr, RegIndirect, ras, rbp, -8);
	
	# mov DWORD PTR [ras], 127 | movl #127, (%ras)
	wrt_two_op_code(tbc, opcodes.movm, RegIndirect|Immediate|FourBytes, ras, 127, 0);
	
	# mov ras, 0 | movq #0, %ras
	# ret
	wrt_two_op_code(tbc, opcodes.movr, Immediate, ras, 0);
	wrt_non_op_code(tbc, opcodes.ret, 0);


'''
int main()
{
	puts("hello world\n");
}
'''
with open('test_puts_helloworld.tbc', 'wb+') as tbc:
	wrt_hdr(tbc, 0, len('hello world\n')+1);
	wrt_hdr_natives(tbc, 'puts');
	wrt_hdr_funcs(tbc, 'main', 0);
	wrt_hdr_globals(tbc, 'str00001', 0, len('hello world\n')+1);
	wrt_hdr_flags(tbc);
	wrt_global_values(tbc, 'hello world\n');
	
# main:
	
# puts("hello world\n");
	# sub rsp, 16 | subq 16, %rsp
	wrt_two_op_code(tbc, opcodes.usubr, Immediate, rsp, 16);
	
	# lea ras, [rbp-127] ;load the offset of the string to ras
	wrt_two_op_code(tbc, opcodes.lea, RegIndirect, rbs, rip, 44);
	
	# mov QWORD PTR [rbp-16], ras
	# "push" the address in 'ras' to stack.
	# calling natives can only use values from the stack.
	# values from the stack are ALWAYS popped for natives.
	#wrt_two_op_code(tbc, opcodes.movm, RegIndirect|Register|EightBytes, rbp, rbs, -16);
	wrt_one_op_code(tbc, opcodes.push, Register, rbs);
	
	wrt_callnat(tbc, Immediate, 1, 0);
	
	# movr ras, 0
	# ret
	wrt_two_op_code(tbc, opcodes.movr, Immediate, ras, 0);
	wrt_non_op_code(tbc, opcodes.ret, 0);



'''
uint32_t factorial(const uint32_t i)
{
	if( i<=1 )
		return 1;
	return i * factorial(i-1);
}
'''
with open('test_factorial.tbc', 'wb+') as tbc:
	wrt_hdr(tbc, 512, 0);
	wrt_hdr_natives(tbc);
	wrt_hdr_funcs(tbc, 'factorial', 0);
	wrt_hdr_globals(tbc);
	wrt_hdr_flags(tbc);
	wrt_global_values(tbc);
	
# factorial:	# CHANGE TO USE STACK AND OTHER REGISTERS.
	# sub rsp, 16
	wrt_two_op_code(tbc, opcodes.usubr, Immediate, rsp, 16); #0-17
	
# if( i<=1 )
	# cmp DWORD PTR [rbp+16], 1
	wrt_two_op_code(tbc, opcodes.ucmpm, RegIndirect|Immediate|FourBytes, rbp, 1, 16); #18-39
	# jz offset:70
	wrt_one_op_code(tbc, opcodes.jnz, Immediate, 70); #40-49
	
# return 1;
	# mov ras, 1
	wrt_two_op_code(tbc, opcodes.movr, Immediate, ras, 1); #50-67
	# ret
	wrt_non_op_code(tbc, opcodes.ret, 0); #68-69
	
# return i * factorial(i-1);
	# mov ras, DWORD PTR [rbp+16]
	wrt_two_op_code(tbc, opcodes.movr, RegIndirect|FourBytes, ras, rbp, 16); #70
	
	# sub ras, 1
	wrt_two_op_code(tbc, opcodes.usubr, Immediate, ras, 1);
	
	# push ras
	wrt_one_op_code(tbc, opcodes.push, Register, ras);
	
	# call factorial
	wrt_one_op_code(tbc, opcodes.call, Immediate, 0);
	
	# mul ras, DWORD PTR [rbp+16], ret
	wrt_two_op_code(tbc, opcodes.umulr, RegIndirect|FourBytes, ras, rbp, 16);
	wrt_non_op_code(tbc, opcodes.ret, 0);


'''
struct Player {
	float speed;
	unsigned health, ammo;
};

void main(void)
{
	struct Player pl = { 300.f, 100, 50 };
	test(&pl);
}
'''
with open('test_native.tbc', 'wb+') as tbc:
	wrt_hdr(tbc, 128, 0);
	wrt_hdr_natives(tbc, 'test');
	wrt_hdr_funcs(tbc, 'main', 0);
	wrt_hdr_globals(tbc);
	wrt_hdr_flags(tbc);
	wrt_global_values(tbc);
	
# main:
	# sub rsp, 16
	wrt_two_op_code(tbc, opcodes.usubr, Immediate, rsp, 16);
	
	# mov dword ptr [rbp-4], 50
	# mov dword ptr [rbp-8], 100
	# mov dword ptr [rbp-12], 300.f
	wrt_two_op_code(tbc, opcodes.movm, RegIndirect|Immediate|FourBytes, rbp, 50, -4);
	wrt_two_op_code(tbc, opcodes.movm, RegIndirect|Immediate|FourBytes, rbp, 100, -8);
	wrt_two_op_code(tbc, opcodes.movm, RegIndirect|Immediate|FourBytes, rbp, float32_to_int(300.0), -12);
	
	# lea ras, [rbp-12]
	wrt_two_op_code(tbc, opcodes.lea, RegIndirect, ras, rbp, -12);
	# push ras
	wrt_one_op_code(tbc, opcodes.push, Register, ras);
	# callnat test
	wrt_callnat(tbc, Immediate, 1, 0);
	
	wrt_two_op_code(tbc, opcodes.movr, Immediate, ras, 0);
	wrt_non_op_code(tbc, opcodes.ret, 0);


'''
void main(void)
{
	struct Player pl = { 300.f, 100, 50 };
	void (*native_test)(struct Player *) = &test;
	(*native_test)(&pl);
}
'''
with open('test_native_funcptr.tbc', 'wb+') as tbc:
	wrt_hdr(tbc, 128, 0);
	wrt_hdr_natives(tbc, 'test');
	wrt_hdr_funcs(tbc, 'main', 0);
	wrt_hdr_globals(tbc);
	wrt_hdr_flags(tbc);
	wrt_global_values(tbc);
	
# main:
	# sub rsp, 32
	wrt_two_op_code(tbc, opcodes.usubr, Immediate, rsp, 32);
	
	# mov dword ptr [rbp-4], 50
	# mov dword ptr [rbp-8], 100
	# mov dword ptr [rbp-12], 300.f
	wrt_two_op_code(tbc, opcodes.movm, RegIndirect|Immediate|FourBytes, rbp, 50, -4);
	wrt_two_op_code(tbc, opcodes.movm, RegIndirect|Immediate|FourBytes, rbp, 100, -8);
	wrt_two_op_code(tbc, opcodes.movm, RegIndirect|Immediate|FourBytes, rbp, float32_to_int(300.0), -12);
	wrt_two_op_code(tbc, opcodes.movm, RegIndirect|Immediate|EightBytes, rbp, 0, -20);
	
	# lea ras, [rbp-12]
	wrt_two_op_code(tbc, opcodes.lea, RegIndirect, ras, rbp, -12);
	# push ras
	wrt_one_op_code(tbc, opcodes.push, Register, ras);
	# mov ras, 0
	wrt_two_op_code(tbc, opcodes.movr, Immediate, ras, 0);
	
	# callnat ras
	wrt_callnat(tbc, Register, 1, ras);
	
	wrt_two_op_code(tbc, opcodes.movr, Immediate, ras, 0);
	wrt_non_op_code(tbc, opcodes.ret, 0);


'''
int main()
{
	getglobal();
}
'''
with open('test_loadgbl.tbc', 'wb+') as tbc:
	wrt_hdr(tbc, 128, 4);
	wrt_hdr_natives(tbc, 'getglobal');
	wrt_hdr_funcs(tbc, 'main', 0);
	wrt_hdr_globals(tbc, 'i', 0, 4);
	wrt_hdr_flags(tbc);
	wrt_global_values(tbc, 4294967196, 4);
	
# main:
	wrt_callnat(tbc, Immediate, 0, 0);
	wrt_two_op_code(tbc, opcodes.movr, Immediate, ras, 0);
	wrt_non_op_code(tbc, opcodes.ret, 0);



'''
test a game-like type of vector calculation.
void vec_invert(float v[3])
{
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
}
int main()
{
	float v[3] = { 2.f, 3.f, 4.f };
	vec_invert(v);
	return 0;
}
'''
with open('test_3d_vecs.tbc', 'wb+') as tbc:
	wrt_hdr(tbc, 128, 0);
	wrt_hdr_natives(tbc);
	wrt_hdr_funcs(tbc, 'main', 0, 'VecInvert', 146);
	wrt_hdr_globals(tbc);
	wrt_hdr_flags(tbc);
	wrt_global_values(tbc);
	
# main:
	# sub rsp, 16
	wrt_two_op_code(tbc, opcodes.usubr, Immediate, rsp, 16); #12-29
	
	wrt_two_op_code(tbc, opcodes.movm, RegIndirect|Immediate|FourBytes, rbp, float32_to_int(2.0), -16); # 30-51
	wrt_two_op_code(tbc, opcodes.movm, RegIndirect|Immediate|FourBytes, rbp, float32_to_int(3.0), -12); # 52-73
	wrt_two_op_code(tbc, opcodes.movm, RegIndirect|Immediate|FourBytes, rbp, float32_to_int(4.0), -8); #74-95
	
	# lea ras, [rbp-16]
	wrt_two_op_code(tbc, opcodes.lea, RegIndirect, ras, rbp, -16); #96-117
	# push ras
	wrt_one_op_code(tbc, opcodes.push, Register, ras); #118-127
	
	wrt_one_op_code(tbc, opcodes.call, Immediate, 146); #128-137
	
	wrt_two_op_code(tbc, opcodes.movr, Immediate, ras, 0); #138-155
	wrt_non_op_code(tbc, opcodes.ret, 0); #156-157
	
# VecInvert:
	# v[0] = -v[0];
	
	# load the vector's pointer off rbp to ras
	wrt_two_op_code(tbc, opcodes.movr, RegIndirect|EightBytes, ras, rbp, 16);
	# load the address in ras to rfs
	wrt_two_op_code(tbc, opcodes.movr, RegIndirect|FourBytes, rfs, ras, 0);
	
	wrt_one_op_code(tbc, opcodes.float2dbl, Register, rfs);
	wrt_one_op_code(tbc, opcodes.fneg, Register, rfs);
	wrt_one_op_code(tbc, opcodes.dbl2float, Register, rfs);
	wrt_two_op_code(tbc, opcodes.movm, RegIndirect|Register|FourBytes, ras, rfs, 0);
	
	# v[1] = -v[1];
	wrt_two_op_code(tbc, opcodes.movr, RegIndirect|FourBytes, rfs, ras, 4);
	
	wrt_one_op_code(tbc, opcodes.float2dbl, Register, rfs);
	wrt_one_op_code(tbc, opcodes.fneg, Register, rfs);
	wrt_one_op_code(tbc, opcodes.dbl2float, Register, rfs);
	wrt_two_op_code(tbc, opcodes.movm, RegIndirect|Register|FourBytes, ras, rfs, 4);
	
	# v[2] = -v[2];
	wrt_two_op_code(tbc, opcodes.movr, RegIndirect|FourBytes, rfs, ras, 8);
	
	wrt_one_op_code(tbc, opcodes.float2dbl, Register, rfs);
	wrt_one_op_code(tbc, opcodes.fneg, Register, rfs);
	wrt_one_op_code(tbc, opcodes.dbl2float, Register, rfs);
	wrt_two_op_code(tbc, opcodes.movm, RegIndirect|Register|FourBytes, ras, rfs, 8);
	
	wrt_non_op_code(tbc, opcodes.ret, 0);




'''
Purpose: test out stdin.

int main()
{
	char string[256];
	puts("Please enter a long string: ");
	fgets(string, 256, stdin);
}
'''
with open('test_stdin.tbc', 'wb+') as tbc:
	wrt_hdr(tbc, 512, 8+len('Please enter a long string: ')+1);
	wrt_hdr_natives(tbc, 'puts', 'fgets');
	wrt_hdr_funcs(tbc, 'main', 0);
	wrt_hdr_globals(tbc,
			'stdin',	0, 8,
			'str00001',	8, len('Please enter a long string: ')+1
	);
	wrt_hdr_flags(tbc);
	wrt_global_values(tbc, 0, 8, 'Please enter a long string: ');
	pcaddr = 0;
# main:
	# char string[256];
	pcaddr += wrt_two_op_code(tbc, opcodes.usubr, Immediate, rsp, 256);
	
	# puts("Please enter a long string: ");
	pcaddr += wrt_two_op_code(tbc, opcodes.lea, RegIndirect, rbs, rip, 144); # 40[rip]
	pcaddr += wrt_one_op_code(tbc, opcodes.push, Register, rbs);
	pcaddr += wrt_callnat(tbc, Immediate, 1, 0);
	# fgets(string, 256, stdin);
	# stdin
	pcaddr += wrt_two_op_code(tbc, opcodes.lea, RegIndirect, rbs, rip, 90); # 86[rip]
	pcaddr += wrt_one_op_code(tbc, opcodes.push, RegIndirect, rbs, 0);
	
	# 256
	pcaddr += wrt_one_op_code(tbc, opcodes.push, Immediate, 256);
	
	# string - lea ras, [rbp-256]
	pcaddr += wrt_two_op_code(tbc, opcodes.lea, RegIndirect, ras, rbp, -256);
	
	pcaddr += wrt_one_op_code(tbc, opcodes.push, Register, ras);
	pcaddr += wrt_callnat(tbc, Immediate, 3, 1);
	
	pcaddr += wrt_two_op_code(tbc, opcodes.movr, Immediate, ras, 0);
	pcaddr += wrt_non_op_code(tbc, opcodes.ret, 0);



'''
Purpose: test self natives for retrieving data script to script.
We need for script's to be able to retrieve data from one another.


struct Tagha;
typedef struct Tagha	Tagha;

void Script_BuildFromFile(Tagha *restrict script, const char *filename);
void Script_Free(Tagha *script);
void Script_CallFunc(Tagha *restrict script, const char *restrict strFunc);
void *Script_GetGlobalByName(const Tagha *restrict script, const char *restrict str);
void Script_PushValue(Tagha *script, const CValue value);
CValue Script_PopValue(Tagha *script);

Tagha *myself;	// myself refers to the script running this code.

int main()
{
	Tagha *t = Script_New();
	if( !t )
		return 0;
	
	Script_BuildFromFile(t, "test_factorial.tbc");
	Script_PushValue(t, (CValue){ .UInt32=7 });
	Script_CallFunc(t, "factorial");
	CValue val = Script_PopValue(t);
	printf("%u\n", val.UInt32);
	
	Script_Free(t), t = NULL;
	return 0;
}
'''
'''
with open('test_interplugin_com.tbc', 'wb+') as tbc:
	wrt_hdr(tbc, 256,
		8+len('%u\n')+1+len('test_factorial.tbc')+1+len('factorial')+1
	);
	wrt_hdr_natives(tbc,
		'Script_New',
		'Script_BuildFromFile',
		'Script_Free',
		'Script_CallFunc',
		'Script_GetGlobalByName',
		'Script_PushValue',
		'Script_PopValue',
		'printf',
	);
	wrt_hdr_funcs(tbc, 'main', 0);
	wrt_hdr_globals(tbc,
		'myself', 0, 8,
		'strFORMAT', 8, len('%u\n')+1,
		'strFILENAME', 12, len('test_factorial.tbc')+1,
		'strFUNCNAME', 38, len('factorial')+1
	);
	wrt_hdr_flags(tbc);
	wrt_global_values(tbc, 0, 8, '%u\n', 'test_factorial.tbc', 'factorial');
	
# main:
	# struct Tagha *t = calloc(1, sizeof(struct Tagha));
	# struct Tagha *t = Script_BuildFromFile("test_factorial.tbc");
	wrt_two_op_code(tbc, opcodes.usubr, Immediate, rsp, 16); #12-29
	
	wrt_two_op_code(tbc, opcodes.lea, RegIndirect, rbs, rip, -243); #30-51
	wrt_one_op_code(tbc, opcodes.push, Register, rbs); #52-61
	# returned ptr is in ras register.
	wrt_callnat(tbc, Immediate, 1, 0); #62-75
	wrt_two_op_code(tbc, opcodes.movm, RegIndirect|Register|EightBytes, rbp, ras, -16); #76-97
	
	# if( !t )
	wrt_two_op_code(tbc, opcodes.ucmpm, RegIndirect|Immediate|EightBytes, rbp, 0, -16); #98-119
	# jump if t is not 0
	wrt_one_op_code(tbc, opcodes.jnz, Immediate, 138); #120-129
	
	# return 0;
	wrt_two_op_code(tbc, opcodes.movr, Immediate, ras, 0); #130-147
	wrt_non_op_code(tbc, opcodes.ret, 0); #148-149
	
	# Script_PushValue(t, (CValue){ .UInt32=7 });
	wrt_one_op_code(tbc, opcodes.push, Immediate, 7); #150
	wrt_one_op_code(tbc, opcodes.push, RegIndirect, rbp, -16);
	wrt_callnat(tbc, Immediate, 2, 4);
	
	# Script_CallFunc(t, "factorial");
	wrt_two_op_code(tbc, opcodes.lea, RegIndirect, rbs, rbp, -217);
	
	wrt_one_op_code(tbc, opcodes.push, Register, rbs);
	wrt_one_op_code(tbc, opcodes.push, RegIndirect, rbp, -16);
	wrt_callnat(tbc, Immediate, 2, 2);
	
	# CValue val = Script_PopValue(t);
	wrt_one_op_code(tbc, opcodes.push, RegIndirect, rbp, -16);
	wrt_callnat(tbc, Immediate, 1, 5);
	
	# printf("%u\n", val.UInt32);
	wrt_one_op_code(tbc, opcodes.push, Register, ras);
	wrt_two_op_code(tbc, opcodes.lea, RegIndirect, rbs, rbp, -247);
	wrt_one_op_code(tbc, opcodes.push, Register, rbs);
	wrt_callnat(tbc, Immediate, 2, 6);
	
	# Script_Free(t), t=NULL;
	wrt_one_op_code(tbc, opcodes.push, RegIndirect, rbp, -16);
	wrt_callnat(tbc, Immediate, 1, 1);
	wrt_two_op_code(tbc, opcodes.movm, RegIndirect|Immediate|EightBytes, rbp, 0, -16);
	
	# return 0;
	#wrt_two_op_code(tbc, opcodes.movr, Immediate, ras, 0); #93-155
	wrt_non_op_code(tbc, opcodes.ret, 0); #156-157
'''



'''
Purpose: test main's function parameters like argv.

int main(int argc, char *argv[])
{
	printf("%s\n", argv[0]);
	return 0;
}
'''
with open('test_main_args.tbc', 'wb+') as tbc:
	wrt_hdr(tbc, 128, len('%s ==\n')+1);
	wrt_hdr_natives(tbc, 'printf');
	wrt_hdr_funcs(tbc, 'main', 0);
	wrt_hdr_globals(tbc,
		'strFORMAT', 0, len('%s ==\n')+1
	);
	wrt_hdr_flags(tbc);
	wrt_global_values(tbc, '%s ==\n');
	
# main:
	#wrt_two_op_code(tbc, opcodes.usubr, Immediate, rsp, 16);
	pcaddr = 0;
	# load address of argv, then add 1 and dereference it
	pcaddr += wrt_two_op_code(tbc, opcodes.movr, RegIndirect|EightBytes, rbs, rbp, 24);
	pcaddr += wrt_two_op_code(tbc, opcodes.movr, RegIndirect|EightBytes, rbs, rbs, 8);
	pcaddr += wrt_one_op_code(tbc, opcodes.push, Register, rbs);
	
	# load our string literal
	pcaddr += wrt_two_op_code(tbc, opcodes.lea, RegIndirect, rcs, rip, 26); # 76[rip]
	pcaddr += wrt_one_op_code(tbc, opcodes.push, Register, rcs);
	# printf("%s\n", argv[1]);
	pcaddr += wrt_callnat(tbc, Immediate, 2, 0);
	
	#wrt_two_op_code(tbc, opcodes.movr, Immediate, ras, 0);
	pcaddr += wrt_non_op_code(tbc, opcodes.ret, 0); #102
