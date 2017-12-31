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
#IPRelative	= 8;
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
rls=11;
rms=12;

rsp=13;
rbp=14;
rip=15;
regsize=16;


def wrt_hdr(f:bytearray, stacksize:int, datasize:int, modes=3) -> None:
	f += 0xC0DE.to_bytes(2, byteorder='little');
	f += stacksize.to_bytes(4, byteorder='little');
	f += datasize.to_bytes(4, byteorder='little');
	
	# 1 for safemode, 2 for debugmode, 3 for both.
	f += modes.to_bytes(1, byteorder='little');

def wrt_hdr_natives(f:bytearray, *natives) -> None:
	i = 0;
	numnatives = len(natives);
	f += numnatives.to_bytes(4, byteorder='little');
	while i<numnatives:
		f += (len(natives[i])+1).to_bytes(4, byteorder='little');
		f += natives[i].encode('utf-8');
		f += 0x0.to_bytes(1, byteorder='little');
		i += 1;

def wrt_hdr_funcs(f:bytearray, *funcs) -> None:
	i = 0;
	numfuncs = len(funcs) // 2;
	f += numfuncs.to_bytes(4, byteorder='little');
	while i<numfuncs*2:
		strsize = len(funcs[i])+1;
		f += strsize.to_bytes(4, byteorder='little');
		f += funcs[i].encode('utf-8');
		f += 0x0.to_bytes(1, byteorder='little');
		i += 1;
		
		f += funcs[i].to_bytes(4, byteorder='little');
		i += 1;

def wrt_hdr_globals(f:bytearray, *lGlobals) -> None:
	i = 0;
	numglobals = len(lGlobals) // 2;
	f += numglobals.to_bytes(4, byteorder='little');
	
	while i<numglobals*2:
		strsize = len(lGlobals[i])+1;
		f += strsize.to_bytes(4, byteorder='little');
		f += lGlobals[i].encode('utf-8');
		f += 0x0.to_bytes(1, byteorder='little');
		i += 1;
		
		# write the .data address offset of this global var.
		f += lGlobals[i].to_bytes(4, byteorder='little');
		i += 1;
	

def float32_to_int(val:float) -> int:
	ba = bytearray(struct.pack("f", val));
	i = int.from_bytes(ba, byteorder='little');
	return i;

def float64_to_int(val:float) -> int:
	ba = bytearray(struct.pack("d", val));
	i = int.from_bytes(ba, byteorder='little');
	return i;

def wrt_global_values(f:bytearray, *lstValues) -> None:
	i = 0;
	while i<len(lstValues):
		if type(lstValues[i]) is str:
			f += lstValues[i].encode('utf-8');
			f += 0x0.to_bytes(1, byteorder='little');
		elif type(lstValues[i]) is int:
			val = lstValues[i];
			i += 1;
			f += val.to_bytes(lstValues[i], byteorder='little');
		elif type(lstValues[i]) is float:
			fval = lstValues[i];
			i += 1;
			if lstValues[i] == 4:
				f += float32_to_int(fval).to_bytes(4, byteorder='little');
			else:
				f += float64_to_int(fval).to_bytes(8, byteorder='little');
		i += 1;


def wrt_non_op_code(f:bytearray, opcode:int, addrmode:int) -> int:
	f += opcode.to_bytes(1, byteorder='little');
	f += addrmode.to_bytes(1, byteorder='little');
	return 2;

def wrt_one_op_code(f:bytearray, opcode:int, addrmode:int, operand:int, offset=None) -> int:
	InstrAddr = 0;
	f += opcode.to_bytes(1, byteorder='little');
	f += addrmode.to_bytes(1, byteorder='little');
	InstrAddr += 2;
	if operand != None:
		f += operand.to_bytes(8, byteorder='little');
		InstrAddr += 8;
	if offset != None:
		ba = bytearray(struct.pack("i", offset));
		i = int.from_bytes(ba, byteorder='little');
		f += i.to_bytes(4, byteorder='little');
		InstrAddr += 4;
	return InstrAddr;

def wrt_two_op_code(f:bytearray, opcode:int, addrmode:int, operand1:int, operand2:int, offset=None) -> int:
	InstrAddr = 0;
	f += opcode.to_bytes(1, byteorder='little');
	f += addrmode.to_bytes(1, byteorder='little');
	InstrAddr += 2;
	if operand1 != None:
		f += operand1.to_bytes(8, byteorder='little');
		InstrAddr += 8;
	if operand2 != None:
		f += operand2.to_bytes(8, byteorder='little');
		InstrAddr += 8;
	if offset != None:
		ba = bytearray(struct.pack("i", offset));
		i = int.from_bytes(ba, byteorder='little');
		f += i.to_bytes(4, byteorder='little');
		InstrAddr += 4;
	return InstrAddr;

def wrt_callnat(f:bytearray, addrmode:int, argcount:int, operand:int, offset=None) -> int:
	InstrAddr = 0;
	f += opcodes.callnat.to_bytes(1, byteorder='little');
	f += addrmode.to_bytes(1, byteorder='little');
	InstrAddr += 2;
	if operand != None:
		f += operand.to_bytes(8, byteorder='little');
		InstrAddr += 8;
	if offset != None:
		ba = bytearray(struct.pack("i", offset));
		i = int.from_bytes(ba, byteorder='little');
		f += i.to_bytes(4, byteorder='little');
		InstrAddr += 4;
	f += argcount.to_bytes(4, byteorder='little');
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
	code = bytearray();
	wrt_hdr(code, 128, 4);
	wrt_hdr_natives(code);
	wrt_hdr_funcs(code, 'main', 0);
	wrt_hdr_globals(code, 'i', 0);
	wrt_global_values(code, 0x0a0b0c0d, 4);
# main:
	# movr ras, 0
	# ret
	wrt_two_op_code(code, opcodes.movr, Immediate, ras, 0);
	wrt_non_op_code(code, opcodes.ret, 0);
	tbc.write(code);


'''
int main()
{
	float f = 2.f;
	f += 4.f;
	return 0;
}
'''
with open('test_floatops.tbc', 'wb+') as tbc:
	code = bytearray();
	wrt_hdr(code, 128, 0);
	wrt_hdr_natives(code);
	wrt_hdr_funcs(code, 'main', 0);
	wrt_hdr_globals(code);
	wrt_global_values(code);
	
# main:
	# mov rhs, 2.f
	wrt_two_op_code(code, opcodes.movr, Immediate, rhs, float32_to_int(2.0));
	# mov rhs, 4.f
	wrt_two_op_code(code, opcodes.movr, Immediate, rfs, float32_to_int(4.0));
	
	# extend rhs and rfs to double.
	# all register-based arithmetic uses 64-bit ints/floats.
	# to use smaller sizes, you have to use pointers.
	wrt_one_op_code(code, opcodes.float2dbl, Register, rhs);
	wrt_one_op_code(code, opcodes.float2dbl, Register, rfs);
	
	# fadd rfs, rhs
	wrt_two_op_code(code, opcodes.faddr, Register, rfs, rhs);
	
	# truncate rfs to float32.
	wrt_one_op_code(code, opcodes.dbl2float, Register, rfs);
	
	# movr ras, 0
	# ret
	wrt_two_op_code(code, opcodes.movr, Immediate, ras, 0);
	wrt_non_op_code(code, opcodes.ret, 0);
	tbc.write(code);


'''
int main()
{
	int i = 5;
	int *p = &i;
	*p = 127;
}
'''
with open('test_pointers.tbc', 'wb+') as tbc:
	code = bytearray();
	wrt_hdr(code, 128, 0);
	wrt_hdr_natives(code);
	wrt_hdr_funcs(code, 'main', 0);
	wrt_hdr_globals(code);
	wrt_global_values(code);
	
# main:
# int i = 5;
	# sub rsp, 16 | subq 16, %rsp
	wrt_two_op_code(code, opcodes.usubr, Immediate, rsp, 16);
	
	# mov DWORD PTR [rbp-12], 5 | movl 5, -12(%rbp)
	wrt_two_op_code(code, opcodes.movm, RegIndirect|Immediate|FourBytes, rbp, 5, -12); # 12 is offset.
	
# int *p = &i;
	# lea ras, [rbp-12] | leaq -12(%rbp), %ras
	wrt_two_op_code(code, opcodes.lea, RegIndirect, ras, rbp, -12);
	
	# mov QWORD PTR [rbp-8], ras | movq %ras, -8(%rbp)
	wrt_two_op_code(code, opcodes.movm, RegIndirect|Register|EightBytes, rbp, ras, -8);
	
# *p = 127;
	# mov ras, QWORD PTR [rbp-8] | movq -8(%rbp), %ras
	wrt_two_op_code(code, opcodes.movr, RegIndirect, ras, rbp, -8);
	
	# mov DWORD PTR [ras], 127 | movl #127, (%ras)
	wrt_two_op_code(code, opcodes.movm, RegIndirect|Immediate|FourBytes, ras, 127, 0);
	
	# mov ras, 0 | movq #0, %ras
	# ret
	wrt_two_op_code(code, opcodes.movr, Immediate, ras, 0);
	wrt_non_op_code(code, opcodes.ret, 0);
	tbc.write(code);


'''
int main()
{
	puts("hello world\n");
}
'''
with open('test_puts_helloworld.tbc', 'wb+') as tbc:
	code = bytearray();
	wrt_hdr(code, 0, len('hello world\n')+1);
	wrt_hdr_natives(code, 'puts');
	wrt_hdr_funcs(code, 'main', 0);
	wrt_hdr_globals(code, 'str00001', 0);
	wrt_global_values(code, 'hello world\n');
	
	pcaddr = 0;
# main:
	
# puts("hello world\n");
	# sub rsp, 16 | subq 16, %rsp
	pcaddr += wrt_two_op_code(code, opcodes.usubr, Immediate, rsp, 16);
	
	# lea ras, [rbp-127] ;load the offset of the string to ras
	pcaddr += wrt_two_op_code(code, opcodes.lea, RegIndirect, rds, rip, 16);
	
	# callnat 'puts', 1
	pcaddr += wrt_callnat(code, Immediate, 1, 0);
	
	# ret
	pcaddr += wrt_non_op_code(code, opcodes.ret, 0);
	tbc.write(code);



'''
uint32_t factorial(const uint32_t i)
{
	if( i<=1 )
		return 1;
	return i * factorial(i-1);
}
'''
with open('test_factorial.tbc', 'wb+') as tbc:
	code = bytearray();
	wrt_hdr(code, 512, 0);
	wrt_hdr_natives(code);
	wrt_hdr_funcs(code, 'factorial', 0);
	wrt_hdr_globals(code);
	wrt_global_values(code);
	
# factorial:
	# sub rsp, 16
	wrt_two_op_code(code, opcodes.usubr, Immediate, rsp, 16); #0-17
	# mov DWORD PTR [rbp-4], rds
	wrt_two_op_code(code, opcodes.movm, RegIndirect|Register|FourBytes, rbp, rds, -4); #18-39
	
# if( i<=1 )
	# cmp DWORD PTR [rbp+16], 1
	wrt_two_op_code(code, opcodes.ucmpm, RegIndirect|Immediate|FourBytes, rbp, 1, -4); #40-61
	# jz offset 92
	wrt_one_op_code(code, opcodes.jnz, Immediate, 92); #62-71
	
# return 1;
	# mov ras, 1
	wrt_two_op_code(code, opcodes.movr, Immediate, ras, 1); #72-89
	# ret
	wrt_non_op_code(code, opcodes.ret, 0); #90-91
	
# return i * factorial(i-1);
	# mov ras, DWORD PTR [rbp-4]
	wrt_two_op_code(code, opcodes.movr, RegIndirect|FourBytes, ras, rbp, -4); #92-113
	
	# sub ras, 1
	wrt_two_op_code(code, opcodes.usubr, Immediate, ras, 1); #114-131
	
	# mov rds, ras
	wrt_two_op_code(code, opcodes.movr, Register, rds, ras); #132-149
	
	# call factorial
	wrt_one_op_code(code, opcodes.call, Immediate, 0); #150-159
	
	# mul ras, DWORD PTR [rbp-4]
	# ret
	wrt_two_op_code(code, opcodes.umulr, RegIndirect|FourBytes, ras, rbp, -4); #160-181
	wrt_non_op_code(code, opcodes.ret, 0); #182-183
	tbc.write(code);


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
	code = bytearray();
	wrt_hdr(code, 128, 0);
	wrt_hdr_natives(code, 'test');
	wrt_hdr_funcs(code, 'main', 0);
	wrt_hdr_globals(code);
	wrt_global_values(code);
	
# main:
	# sub rsp, 16
	wrt_two_op_code(code, opcodes.usubr, Immediate, rsp, 16);
	
	# mov dword ptr [rbp-4], 50
	# mov dword ptr [rbp-8], 100
	# mov dword ptr [rbp-12], 300.f
	wrt_two_op_code(code, opcodes.movm, RegIndirect|Immediate|FourBytes, rbp, 50, -4);
	wrt_two_op_code(code, opcodes.movm, RegIndirect|Immediate|FourBytes, rbp, 100, -8);
	wrt_two_op_code(code, opcodes.movm, RegIndirect|Immediate|FourBytes, rbp, float32_to_int(300.0), -12);
	
	# lea rds, [rbp-12]
	wrt_two_op_code(code, opcodes.lea, RegIndirect, rds, rbp, -12);
	# callnat test
	wrt_callnat(code, Immediate, 1, 0);
	
	wrt_two_op_code(code, opcodes.movr, Immediate, ras, 0);
	wrt_non_op_code(code, opcodes.ret, 0);
	tbc.write(code);


'''
void main(void)
{
	struct Player pl = { 300.f, 100, 50 };
	void (*native_test)(struct Player *) = &test;
	(*native_test)(&pl);
}
'''
with open('test_native_funcptr.tbc', 'wb+') as tbc:
	code = bytearray();
	wrt_hdr(code, 128, 0);
	wrt_hdr_natives(code, 'test');
	wrt_hdr_funcs(code, 'main', 0);
	wrt_hdr_globals(code);
	wrt_global_values(code);
	
# main:
	# sub rsp, 32
	wrt_two_op_code(code, opcodes.usubr, Immediate, rsp, 32);
	
	# mov dword ptr [rbp-4], 50
	# mov dword ptr [rbp-8], 100
	# mov dword ptr [rbp-12], 300.f
	wrt_two_op_code(code, opcodes.movm, RegIndirect|Immediate|FourBytes, rbp, 50, -4);
	wrt_two_op_code(code, opcodes.movm, RegIndirect|Immediate|FourBytes, rbp, 100, -8);
	wrt_two_op_code(code, opcodes.movm, RegIndirect|Immediate|FourBytes, rbp, float32_to_int(300.0), -12);
	
	# lea rds, [rbp-12]
	wrt_two_op_code(code, opcodes.lea, RegIndirect, rds, rbp, -12);
	# mov ras, 0
	wrt_two_op_code(code, opcodes.movr, Immediate, ras, 0);
	
	# callnat ras
	wrt_callnat(code, Register, 1, ras);
	
	wrt_two_op_code(code, opcodes.movr, Immediate, ras, 0);
	wrt_non_op_code(code, opcodes.ret, 0);
	tbc.write(code);


'''
int main()
{
	getglobal();
}
'''
with open('test_loadgbl.tbc', 'wb+') as tbc:
	code = bytearray();
	wrt_hdr(code, 128, 4);
	wrt_hdr_natives(code, 'getglobal');
	wrt_hdr_funcs(code, 'main', 0);
	wrt_hdr_globals(code, 'i', 0);
	wrt_global_values(code, 4294967196, 4);
	
# main:
	wrt_callnat(code, Immediate, 0, 0);
	wrt_two_op_code(code, opcodes.movr, Immediate, ras, 0);
	wrt_non_op_code(code, opcodes.ret, 0);
	tbc.write(code);



'''
test a game-like type of vector calculation.
void VecInvert(float v[3])
{
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
}
int main()
{
	float v[3] = { 2.f, 3.f, 4.f };
	VecInvert(v);
	return 0;
}
'''
with open('test_3d_vecs.tbc', 'wb+') as tbc:
	code = bytearray();
	wrt_hdr(code, 128, 0);
	wrt_hdr_natives(code);
	wrt_hdr_funcs(code, 'main', 0, 'VecInvert', 136);
	wrt_hdr_globals(code);
	wrt_global_values(code);
	
# main:
	# sub rsp, 16
	wrt_two_op_code(code, opcodes.usubr, Immediate, rsp, 16); #0-17
	
	# lea rds, [rsp+4]
	wrt_two_op_code(code, opcodes.lea, RegIndirect, rds, rsp, 4); #18-39
	
	# mov DWORD PTR [rsp+4], 0x40000000
	# mov DWORD PTR [rsp+8], 0x40400000
	# mov DWORD PTR [rsp+12], 0x40800000
	wrt_two_op_code(code, opcodes.movm, RegIndirect|Immediate|FourBytes, rsp, float32_to_int(2.0), 4); #40-61
	wrt_two_op_code(code, opcodes.movm, RegIndirect|Immediate|FourBytes, rsp, float32_to_int(3.0), 8); #62-83
	wrt_two_op_code(code, opcodes.movm, RegIndirect|Immediate|FourBytes, rsp, float32_to_int(4.0), 12); #84-105
	
	wrt_one_op_code(code, opcodes.call, Immediate, 136); #106-115
	
	wrt_two_op_code(code, opcodes.movr, Immediate, ras, 0); #116-133
	wrt_non_op_code(code, opcodes.ret, 0); #134-135
	
# VecInvert:
	# v[0] = -v[0];
	# mov rfs, dword ptr [rds]
	wrt_two_op_code(code, opcodes.movr, RegIndirect|FourBytes, rfs, rds, 0);
	
	wrt_one_op_code(code, opcodes.float2dbl, Register, rfs);
	wrt_one_op_code(code, opcodes.fneg, Register, rfs);
	wrt_one_op_code(code, opcodes.dbl2float, Register, rfs);
	# mov dword ptr [rds], rfs
	wrt_two_op_code(code, opcodes.movm, RegIndirect|Register|FourBytes, rds, rfs, 0);
	
	# v[1] = -v[1];
	# mov rfs, dword ptr [rds+4]
	wrt_two_op_code(code, opcodes.movr, RegIndirect|FourBytes, rhs, rds, 4);
	
	wrt_one_op_code(code, opcodes.float2dbl, Register, rhs);
	wrt_one_op_code(code, opcodes.fneg, Register, rhs);
	wrt_one_op_code(code, opcodes.dbl2float, Register, rhs);
	# mov dword ptr [rds+4], rfs
	wrt_two_op_code(code, opcodes.movm, RegIndirect|Register|FourBytes, rds, rhs, 4);
	
	# v[2] = -v[2];
	# mov rfs, dword ptr [rds+8]
	wrt_two_op_code(code, opcodes.movr, RegIndirect|FourBytes, rgs, rds, 8);
	
	wrt_one_op_code(code, opcodes.float2dbl, Register, rgs);
	wrt_one_op_code(code, opcodes.fneg, Register, rgs);
	wrt_one_op_code(code, opcodes.dbl2float, Register, rgs);
	# mov dword ptr [rds+8], rfs
	wrt_two_op_code(code, opcodes.movm, RegIndirect|Register|FourBytes, rds, rgs, 8);
	
	wrt_non_op_code(code, 255, 0);
	wrt_non_op_code(code, opcodes.ret, 0);
	tbc.write(code);




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
	code = bytearray();
	wrt_hdr(code, 512, 8+len('Please enter a long string: ')+1);
	wrt_hdr_natives(code, 'puts', 'fgets');
	wrt_hdr_funcs(code, 'main', 0);
	wrt_hdr_globals(code,
		'stdin',	0,
		'str00001',	8
	);
	wrt_global_values(code, 0, 8, 'Please enter a long string: ');
	pcaddr = 0;
# main:
	# char string[256];
	pcaddr += wrt_two_op_code(code, opcodes.usubr, Immediate, rsp, 256);
	
	# puts("Please enter a long string: ");
	pcaddr += wrt_two_op_code(code, opcodes.lea, RegIndirect, rds, rip, 118); # 118[rip]
	pcaddr += wrt_callnat(code, Immediate, 1, 0);
	# fgets(string, 256, stdin);
	# stdin
	pcaddr += wrt_two_op_code(code, opcodes.movr, RegIndirect|EightBytes, rfs, rip, 74); # 74[rip]
	
	# 256
	pcaddr += wrt_two_op_code(code, opcodes.movr, Immediate, res, 256);
	
	# string - lea rfs, [rbp-256]
	pcaddr += wrt_two_op_code(code, opcodes.lea, RegIndirect|EightBytes, rds, rbp, -256);
	
	#pcaddr += wrt_non_op_code(code, opcodes.nop, 0); pcaddr += wrt_non_op_code(code, opcodes.nop, 0); pcaddr += wrt_non_op_code(code, opcodes.nop, 0); pcaddr += wrt_non_op_code(code, opcodes.nop, 0); pcaddr += wrt_non_op_code(code, opcodes.nop, 0); pcaddr += wrt_non_op_code(code, opcodes.nop, 0); pcaddr += wrt_non_op_code(code, opcodes.nop, 0);
	
	pcaddr += wrt_callnat(code, Immediate, 3, 1);
	
	pcaddr += wrt_two_op_code(code, opcodes.movr, Immediate, ras, 0);
	pcaddr += wrt_non_op_code(code, opcodes.ret, 0);
	tbc.write(code);



'''
Purpose: test a host exported global variable

struct Player {
	float		speed;
	uint32_t	health;
	uint32_t	ammo;
};
struct Player *g_pPlayer;

int main()
{
	g_pPlayer->speed = 200.f;
	g_pPlayer->health = 99;
	g_pPlayer->ammo = 32;
}
'''

with open('test_exported_host_var.tbc', 'wb+') as tbc:
	code = bytearray();
	wrt_hdr(code, 256, 8);
	wrt_hdr_natives(code);
	wrt_hdr_funcs(code, 'main', 0);
	wrt_hdr_globals(code, 'g_pPlayer', 0);
	wrt_global_values(code, 0, 8);
	pcaddr = 0;
# main:
	pcaddr += wrt_two_op_code(code, opcodes.usubr, Immediate, rsp, 16); #0-18
	
	# g_pPlayer->speed
		# mov rds, qword ptr g_pPlayer[ip] ;54
		# mov dword ptr [rds], 300.f
	pcaddr += wrt_two_op_code(code, opcodes.movr, RegIndirect|EightBytes, rds, rip, 68); #40
	pcaddr += wrt_two_op_code(code, opcodes.movm, RegIndirect|FourBytes|Immediate, rds, float32_to_int(200.0), 0);
	# g_pPlayer->health
		# mov dword ptr [rds+4], 100
	pcaddr += wrt_two_op_code(code, opcodes.movm, RegIndirect|FourBytes|Immediate, rds, 99, 4);
	# g_pPlayer->ammo
		# mov dword ptr [rds+8], 51
	pcaddr += wrt_two_op_code(code, opcodes.movm, RegIndirect|FourBytes|Immediate, rds, 32, 8);
	# return 0;
	pcaddr += wrt_non_op_code(code, opcodes.ret, 0);
	tbc.write(code);




'''
Purpose: test main's function parameters like argv.

int main(int argc, char *argv[])
{
	printf("%s\n", argv[1]);
	return 0;
}
'''
with open('test_main_args.tbc', 'wb+') as tbc:
	code = bytearray();
	wrt_hdr(code, 128, 0);
	wrt_hdr_natives(code, 'puts');
	wrt_hdr_funcs(code, 'main', 0);
	wrt_hdr_globals(code);
	wrt_global_values(code);
	
# main:
	pcaddr = 0;
	pcaddr += wrt_two_op_code(code, opcodes.usubr, Immediate, rsp, 16);
	
	# mov rbs, rds ;save value of rds
	pcaddr += wrt_two_op_code(code, opcodes.movr, Register, rbs, rds);
	
	# *argv++;
	pcaddr += wrt_two_op_code(code, opcodes.movr, RegIndirect|EightBytes, rds, res, 8);
	
	# puts(argv[1]);
	pcaddr += wrt_callnat(code, Immediate, 1, 0);
	
	#wrt_two_op_code(code, opcodes.movr, Immediate, ras, 0);
	pcaddr += wrt_non_op_code(code, opcodes.ret, 0); #102
	tbc.write(code);

