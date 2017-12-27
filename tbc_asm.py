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
	
	# truncate rfs to float.
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
	
# main:
	
# puts("hello world\n");
	# sub rsp, 16 | subq 16, %rsp
	wrt_two_op_code(code, opcodes.usubr, Immediate, rsp, 16);
	
	# lea ras, [rbp-127] ;load the offset of the string to ras
	wrt_two_op_code(code, opcodes.lea, RegIndirect, rbs, rip, 44);
	
	# mov QWORD PTR [rbp-16], ras
	# "push" the address in 'ras' to stack.
	# calling natives can only use values from the stack.
	# values from the stack are ALWAYS popped for natives.
	#wrt_two_op_code(tbc, opcodes.movm, RegIndirect|Register|EightBytes, rbp, rbs, -16);
	wrt_one_op_code(code, opcodes.push, Register, rbs);
	
	wrt_callnat(code, Immediate, 1, 0);
	
	# movr ras, 0
	# ret
	wrt_two_op_code(code, opcodes.movr, Immediate, ras, 0);
	wrt_non_op_code(code, opcodes.ret, 0);
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
	
# factorial:	# CHANGE TO USE STACK AND OTHER REGISTERS.
	# sub rsp, 16
	wrt_two_op_code(code, opcodes.usubr, Immediate, rsp, 16); #0-17
	
# if( i<=1 )
	# cmp DWORD PTR [rbp+16], 1
	wrt_two_op_code(code, opcodes.ucmpm, RegIndirect|Immediate|FourBytes, rbp, 1, 16); #18-39
	# jz offset:70
	wrt_one_op_code(code, opcodes.jnz, Immediate, 70); #40-49
	
# return 1;
	# mov ras, 1
	wrt_two_op_code(code, opcodes.movr, Immediate, ras, 1); #50-67
	# ret
	wrt_non_op_code(code, opcodes.ret, 0); #68-69
	
# return i * factorial(i-1);
	# mov ras, DWORD PTR [rbp+16]
	wrt_two_op_code(code, opcodes.movr, RegIndirect|FourBytes, ras, rbp, 16); #70
	
	# sub ras, 1
	wrt_two_op_code(code, opcodes.usubr, Immediate, ras, 1);
	
	# push ras
	wrt_one_op_code(code, opcodes.push, Register, ras);
	
	# call factorial
	wrt_one_op_code(code, opcodes.call, Immediate, 0);
	
	# mul ras, DWORD PTR [rbp+16], ret
	wrt_two_op_code(code, opcodes.umulr, RegIndirect|FourBytes, ras, rbp, 16);
	wrt_non_op_code(code, opcodes.ret, 0);
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
	
	# lea ras, [rbp-12]
	wrt_two_op_code(code, opcodes.lea, RegIndirect, ras, rbp, -12);
	# push ras
	wrt_one_op_code(code, opcodes.push, Register, ras);
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
	wrt_two_op_code(code, opcodes.movm, RegIndirect|Immediate|EightBytes, rbp, 0, -20);
	
	# lea ras, [rbp-12]
	wrt_two_op_code(code, opcodes.lea, RegIndirect, ras, rbp, -12);
	# push ras
	wrt_one_op_code(code, opcodes.push, Register, ras);
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
	code = bytearray();
	wrt_hdr(code, 128, 0);
	wrt_hdr_natives(code);
	wrt_hdr_funcs(code, 'main', 0, 'VecInvert', 146);
	wrt_hdr_globals(code);
	wrt_global_values(code);
	
# main:
	# sub rsp, 16
	wrt_two_op_code(code, opcodes.usubr, Immediate, rsp, 16); #12-29
	
	wrt_two_op_code(code, opcodes.movm, RegIndirect|Immediate|FourBytes, rbp, float32_to_int(2.0), -16); # 30-51
	wrt_two_op_code(code, opcodes.movm, RegIndirect|Immediate|FourBytes, rbp, float32_to_int(3.0), -12); # 52-73
	wrt_two_op_code(code, opcodes.movm, RegIndirect|Immediate|FourBytes, rbp, float32_to_int(4.0), -8); #74-95
	
	# lea ras, [rbp-16]
	wrt_two_op_code(code, opcodes.lea, RegIndirect, ras, rbp, -16); #96-117
	# push ras
	wrt_one_op_code(code, opcodes.push, Register, ras); #118-127
	
	wrt_one_op_code(code, opcodes.call, Immediate, 146); #128-137
	
	wrt_two_op_code(code, opcodes.movr, Immediate, ras, 0); #138-155
	wrt_non_op_code(code, opcodes.ret, 0); #156-157
	
# VecInvert:
	# v[0] = -v[0];
	
	# load the vector's pointer off rbp to ras
	wrt_two_op_code(code, opcodes.movr, RegIndirect|EightBytes, ras, rbp, 16);
	# load the address in ras to rfs
	wrt_two_op_code(code, opcodes.movr, RegIndirect|FourBytes, rfs, ras, 0);
	
	wrt_one_op_code(code, opcodes.float2dbl, Register, rfs);
	wrt_one_op_code(code, opcodes.fneg, Register, rfs);
	wrt_one_op_code(code, opcodes.dbl2float, Register, rfs);
	wrt_two_op_code(code, opcodes.movm, RegIndirect|Register|FourBytes, ras, rfs, 0);
	
	# v[1] = -v[1];
	wrt_two_op_code(code, opcodes.movr, RegIndirect|FourBytes, rfs, ras, 4);
	
	wrt_one_op_code(code, opcodes.float2dbl, Register, rfs);
	wrt_one_op_code(code, opcodes.fneg, Register, rfs);
	wrt_one_op_code(code, opcodes.dbl2float, Register, rfs);
	wrt_two_op_code(code, opcodes.movm, RegIndirect|Register|FourBytes, ras, rfs, 4);
	
	# v[2] = -v[2];
	wrt_two_op_code(code, opcodes.movr, RegIndirect|FourBytes, rfs, ras, 8);
	
	wrt_one_op_code(code, opcodes.float2dbl, Register, rfs);
	wrt_one_op_code(code, opcodes.fneg, Register, rfs);
	wrt_one_op_code(code, opcodes.dbl2float, Register, rfs);
	wrt_two_op_code(code, opcodes.movm, RegIndirect|Register|FourBytes, ras, rfs, 8);
	
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
	pcaddr += wrt_two_op_code(code, opcodes.lea, RegIndirect, rbs, rip, 144); # 40[rip]
	pcaddr += wrt_one_op_code(code, opcodes.push, Register, rbs);
	pcaddr += wrt_callnat(code, Immediate, 1, 0);
	# fgets(string, 256, stdin);
	# stdin
	pcaddr += wrt_two_op_code(code, opcodes.lea, RegIndirect, rbs, rip, 90); # 86[rip]
	pcaddr += wrt_one_op_code(code, opcodes.push, RegIndirect, rbs, 0);
	
	# 256
	pcaddr += wrt_one_op_code(code, opcodes.push, Immediate, 256);
	
	# string - lea ras, [rbp-256]
	pcaddr += wrt_two_op_code(code, opcodes.lea, RegIndirect, ras, rbp, -256);
	
	pcaddr += wrt_one_op_code(code, opcodes.push, Register, ras);
	pcaddr += wrt_callnat(code, Immediate, 3, 1);
	
	pcaddr += wrt_two_op_code(code, opcodes.movr, Immediate, ras, 0);
	pcaddr += wrt_non_op_code(code, opcodes.ret, 0);
	tbc.write(code);



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
	code = bytearray();
	wrt_hdr(code, 256,
		8+len('%u\n')+1+len('test_factorial.tbc')+1+len('factorial')+1
	);
	wrt_hdr_natives(code,
		'Script_New',
		'Script_BuildFromFile',
		'Script_Free',
		'Script_CallFunc',
		'Script_GetGlobalByName',
		'Script_PushValue',
		'Script_PopValue',
		'printf',
	);
	wrt_hdr_funcs(code, 'main', 0);
	wrt_hdr_globals(code,
		'myself', 0,
		'strFORMAT', 8,
		'strFILENAME', 12,
		'strFUNCNAME', 38
	);
	wrt_global_values(code, 0, 8, '%u\n', 'test_factorial.tbc', 'factorial');
	
# main:
	# struct Tagha *t = calloc(1, sizeof(struct Tagha));
	# struct Tagha *t = Script_BuildFromFile("test_factorial.tbc");
	wrt_two_op_code(code, opcodes.usubr, Immediate, rsp, 16); #12-29
	
	wrt_two_op_code(code, opcodes.lea, RegIndirect, rbs, rip, -243); #30-51
	wrt_one_op_code(code, opcodes.push, Register, rbs); #52-61
	# returned ptr is in ras register.
	wrt_callnat(code, Immediate, 1, 0); #62-75
	wrt_two_op_code(code, opcodes.movm, RegIndirect|Register|EightBytes, rbp, ras, -16); #76-97
	
	# if( !t )
	wrt_two_op_code(code, opcodes.ucmpm, RegIndirect|Immediate|EightBytes, rbp, 0, -16); #98-119
	# jump if t is not 0
	wrt_one_op_code(code, opcodes.jnz, Immediate, 138); #120-129
	
	# return 0;
	wrt_two_op_code(code, opcodes.movr, Immediate, ras, 0); #130-147
	wrt_non_op_code(code, opcodes.ret, 0); #148-149
	
	# Script_PushValue(t, (CValue){ .UInt32=7 });
	wrt_one_op_code(code, opcodes.push, Immediate, 7); #150
	wrt_one_op_code(code, opcodes.push, RegIndirect, rbp, -16);
	wrt_callnat(code, Immediate, 2, 4);
	
	# Script_CallFunc(t, "factorial");
	wrt_two_op_code(code, opcodes.lea, RegIndirect, rbs, rbp, -217);
	
	wrt_one_op_code(code, opcodes.push, Register, rbs);
	wrt_one_op_code(code, opcodes.push, RegIndirect, rbp, -16);
	wrt_callnat(code, Immediate, 2, 2);
	
	# CValue val = Script_PopValue(t);
	wrt_one_op_code(code, opcodes.push, RegIndirect, rbp, -16);
	wrt_callnat(code, Immediate, 1, 5);
	
	# printf("%u\n", val.UInt32);
	wrt_one_op_code(code, opcodes.push, Register, ras);
	wrt_two_op_code(code, opcodes.lea, RegIndirect, rbs, rbp, -247);
	wrt_one_op_code(code, opcodes.push, Register, rbs);
	wrt_callnat(code, Immediate, 2, 6);
	
	# Script_Free(t), t=NULL;
	wrt_one_op_code(code, opcodes.push, RegIndirect, rbp, -16);
	wrt_callnat(code, Immediate, 1, 1);
	wrt_two_op_code(code, opcodes.movm, RegIndirect|Immediate|EightBytes, rbp, 0, -16);
	
	# return 0;
	#wrt_two_op_code(code, opcodes.movr, Immediate, ras, 0); #93-155
	wrt_non_op_code(code, opcodes.ret, 0); #156-157
	tbc.write(code);
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
	code = bytearray();
	wrt_hdr(code, 128, len('%s ==\n')+1);
	wrt_hdr_natives(code, 'printf');
	wrt_hdr_funcs(code, 'main', 0);
	wrt_hdr_globals(code, 'strFORMAT', 0);
	wrt_global_values(code, '%s ==\n');
	
# main:
	#wrt_two_op_code(code, opcodes.usubr, Immediate, rsp, 16);
	pcaddr = 0;
	# load address of argv, then add 1 and dereference it
	pcaddr += wrt_two_op_code(code, opcodes.movr, RegIndirect|EightBytes, rbs, rbp, 24);
	pcaddr += wrt_two_op_code(code, opcodes.movr, RegIndirect|EightBytes, rbs, rbs, 8);
	pcaddr += wrt_one_op_code(code, opcodes.push, Register, rbs);
	
	# load our string literal
	pcaddr += wrt_two_op_code(code, opcodes.lea, RegIndirect, rcs, rip, 26); # 76[rip]
	pcaddr += wrt_one_op_code(code, opcodes.push, Register, rcs);
	# printf("%s\n", argv[1]);
	pcaddr += wrt_callnat(code, Immediate, 2, 0);
	
	#wrt_two_op_code(code, opcodes.movr, Immediate, ras, 0);
	pcaddr += wrt_non_op_code(code, opcodes.ret, 0); #102
	tbc.write(code);

