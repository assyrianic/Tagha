#!/usr/bin/python3

import sys;
import ctypes;
import struct;
import collections;

# Leave the references empty and build rest of bytecode
# Fill in references afterwards when you know their positions.

def enum(*sequential, **named) -> object:
	enums = dict(zip(sequential, range(len(sequential))), **named);
	return type('Enum', (), enums);

opcodes = enum('halt', 'push', 'pop', 'lea', 'mov', 'add', 'sub', 'mul', 'divi', 'mod', 'andb', 'orb', 'xorb', 'notb', 'shl', 'shr', 'inc', 'dec', 'neg', 'lt', 'gt', 'cmp', 'neq', 'jmp', 'jz', 'jnz', 'call', 'syscall', 'ret', 'flt2dbl', 'dbl2flt', 'addf', 'subf', 'mulf', 'divf', 'incf', 'decf', 'negf', 'ltf', 'gtf', 'cmpf', 'neqf', 'nop');

Immediate	= 1;
Register	= 2;
RegIndirect	= 4;
Reserved	= 8;
Byte		= 16;
TwoBytes	= 32;
FourBytes	= 64;
EightBytes	= 128;

regAlaf=0
regBeth=1
regVeth=2
regGamal=3
regGhamal=4
regDalath=5
regDhalath=6
regHeh=7
regWaw=8
regZain=9
regHeth=10
regTeth=11
regYodh=12
regKaf=13
regKhaf=14
regLamad=15
regMeem=16
regNoon=17
regSemkath=18
reg_Eh=19
regPeh=20
regFeh=21
regSade=22
regQof=23
regReesh=24
regSheen=25
regTaw=26
regThaw=27
regStk = 28;
regBase = 29;
regInstr = 30;
regsize = 31;

class CFuncInfo:
	def __init__(self, isnative=False):
		self.Native = isnative;
		self.Instrs = bytearray();
	
	def add_two_op_instr(self, opcode:int, first_addrmode:int, sec_addrmode:int, first_oper:int, sec_operand:int, offset=None) -> int:
		if self.Native:
			return 0;
		n = 0;
		self.Instrs += opcode.to_bytes(1, byteorder='little'); # write opcode
		self.Instrs += first_addrmode.to_bytes(1, byteorder='little'); # write 1st addrmode
		self.Instrs += sec_addrmode.to_bytes(1, byteorder='little'); # write 2nd addrmode
		self.Instrs += first_oper.to_bytes(1, byteorder='little'); # write 1st register operand value.
		n += 4;
		if sec_addrmode & Immediate:
			self.Instrs += sec_operand.to_bytes(8, byteorder='little');
			n += 8;
		else:
			self.Instrs += sec_operand.to_bytes(1, byteorder='little');
			n += 1;
		if offset != None:
			ba = bytearray(struct.pack("i", offset));
			i = int.from_bytes(ba, byteorder='little');
			self.Instrs += i.to_bytes(4, byteorder='little');
			n += 4;
		return n;
	
	def add_one_op_instr(self, opcode:int, addrmode:int, operand:int, offset=None) -> None:
		if self.Native:
			return 0;
		
		n = 0;
		self.Instrs += opcode.to_bytes(1, byteorder='little'); # write opcode
		self.Instrs += addrmode.to_bytes(1, byteorder='little'); # write addrmode
		n += 2;
		
		if addrmode & Immediate:
			self.Instrs += operand.to_bytes(8, byteorder='little');
			n += 8;
		else:
			self.Instrs += operand.to_bytes(1, byteorder='little');
			n += 1;
		if offset != None:
			ba = bytearray(struct.pack("i", offset));
			i = int.from_bytes(ba, byteorder='little');
			self.Instrs += i.to_bytes(4, byteorder='little');
			n += 4;
		return n;
	
	def add_op_instr(self, opcode:int, addrmode=0) -> int:
		if self.Native:
			return 0;
		
		self.Instrs += opcode.to_bytes(1, byteorder='little'); # write opcode
		self.Instrs += addrmode.to_bytes(1, byteorder='little'); # write addrmode
		return 2;
	
	def add_syscall(self, argcount:int, addrmode:int, operand:int, offset=None) -> None:
		if self.Native:
			return 0;
		
		n = 0;
		self.Instrs += opcodes.syscall.to_bytes(1, byteorder='little'); # write syscall opcode
		self.Instrs += addrmode.to_bytes(1, byteorder='little'); # write addrmode
		self.Instrs += argcount.to_bytes(4, byteorder='little');
		n += 6;
		
		if addrmode & Immediate:
			self.Instrs += operand.to_bytes(8, byteorder='little');
			n += 8;
		else:
			self.Instrs += operand.to_bytes(1, byteorder='little');
			n += 1;
		if offset != None:
			ba = bytearray(struct.pack("i", offset));
			i = int.from_bytes(ba, byteorder='little');
			self.Instrs += i.to_bytes(4, byteorder='little');
			n += 4;
		return n;

class CGlobalVarInfo:
	def __init__(self, bytes:int, initval=None, isnative=False):
		self.Native = isnative;
		self.Bytesize = bytes;
		self.Val = bytearray();
		if initval == None:
			self.Val += 0x0.to_bytes(self.Bytesize, byteorder='little');
		else:
			for byte in initval:
				self.Val.append(byte);


def make_header(stacksize=64, flags=0, functable_bytes=0) -> object:
	f = bytearray();
	f += 0xC0DE.to_bytes(2, byteorder='little');
	f += stacksize.to_bytes(4, byteorder='little');
	f += flags.to_bytes(1, byteorder='little');
	f += functable_bytes.to_bytes(4, byteorder='little');
	return f;

def make_func_table(funcs:dict) -> object:
	f = bytearray();
	if len(funcs) == 0:
		return f;
	
	numfuncs = len(funcs);
	f += numfuncs.to_bytes(4, byteorder='little');
	for key, val in funcs.items():
		f += val.Native.to_bytes(1, byteorder='little');
		f += (len(key)+1).to_bytes(4, byteorder='little');
		if val.Native:
			f += 0x8.to_bytes(4, byteorder='little');
		else:
			f += (len(val.Instrs)).to_bytes(4, byteorder='little');
			print("instr len %d" % len(val.Instrs));
		f += key.encode('utf-8');
		f += 0x0.to_bytes(1, byteorder='little');
		
		if val.Native:
			f += 0x0.to_bytes(8, byteorder='little');
		else:
			for byte in val.Instrs:
				f.append(byte);
	return f;


def make_var_table(globalvars:dict) -> object:
	f = bytearray();
	if globalvars == None or len(globalvars) == 0:
		f += 0x0.to_bytes(4, byteorder='little');
		return f;
	else:
		numvars = len(globalvars);
		f += numvars.to_bytes(4, byteorder='little');
	
	for key, val in globalvars.items():
		f += val.Native.to_bytes(1, byteorder='little');
		f += (len(key)+1).to_bytes(4, byteorder='little');
		f += val.Bytesize.to_bytes(4, byteorder='little');
		
		f += key.encode('utf-8');
		f += 0x0.to_bytes(1, byteorder='little');
		
		if val.Native:
			f += 0x0.to_bytes(8, byteorder='little');
		else:
			for byte in val.Val:
				f.append(byte);
	return f;

def float32_to_int(val:float) -> int:
	ba = bytearray(struct.pack("f", val));
	ba += 0x0.to_bytes(4, byteorder='little');
	i = int.from_bytes(ba, byteorder='little');
	return i;

def float64_to_int(val:float) -> int:
	ba = bytearray(struct.pack("d", val));
	i = int.from_bytes(ba, byteorder='little');
	return i;


'''
uint32_t i = 0x0a0b0c0d;
int32_t main()
{
}
'''
with open('test_endian.tbc', 'wb+') as tbc:
	dictFuncs = collections.OrderedDict();
	dictFuncs["main"] = CFuncInfo();
# main:
	# mov regAlaf, 550
	dictFuncs["main"].add_two_op_instr(opcodes.mov, Register, Immediate, regAlaf, 550);
	
	# ret
	dictFuncs["main"].add_op_instr(opcodes.ret);
	
	dictVars = collections.OrderedDict();
	dictVars["i"] = CGlobalVarInfo(4, 0x0a0b0c0d.to_bytes(4, byteorder='little'));
	
	code = bytearray();
	functable = make_func_table(dictFuncs);
	vartable = make_var_table(dictVars);
	for i in make_header(128, 0, len(functable)):
		code.append(i);
	for i in functable:
		code.append(i);
	for i in vartable:
		code.append(i);
	tbc.write(code);


'''
int32_t main()
{
	float f = 2.f;
	f += 4.f;
}
'''
with open('test_float_ops.tbc', 'wb+') as tbc:
	dictFuncs = collections.OrderedDict();
	dictFuncs["main"] = CFuncInfo();
# main:
	# mov regAlaf, 2.f
	dictFuncs["main"].add_two_op_instr(opcodes.mov, Register, Immediate, regAlaf, float32_to_int(2.0));
	
	# addf regAlaf, 4.f
	dictFuncs["main"].add_two_op_instr(opcodes.addf, Register, Immediate|FourBytes, regAlaf, float32_to_int(8.5));
	
	# ret
	dictFuncs["main"].add_op_instr(opcodes.ret);
	
	
	code = bytearray();
	functable = make_func_table(dictFuncs);
	vartable = make_var_table(None);
	for i in make_header(128, 0, len(functable)):
		code.append(i);
	for i in functable:
		code.append(i);
	for i in vartable:
		code.append(i);
	tbc.write(code);


'''
int32_t main()
{
	int i = 5;
	int *p = &i;
	*p = 127;
}
'''
with open('test_ptrs.tbc', 'wb+') as tbc:
	dictFuncs = collections.OrderedDict();
	dictFuncs["main"] = CFuncInfo();
	
# main:
# int i = 5;
	# mov DWORD PTR [rbp-12], 5 | movl 5, -12(%rbp)
	dictFuncs["main"].add_two_op_instr(opcodes.mov, RegIndirect|FourBytes, Immediate, regBase, 5, -12);
	
# int *p = &i;
	# lea rbs, [rbp-12] | leaq -12(%rbp), %rbs
	dictFuncs["main"].add_two_op_instr(opcodes.lea, 0, 0, regBeth, regBase, -12);
	
	# mov QWORD PTR [rbp-8], rbs | movq %rbs, -8(%rbp)
	dictFuncs["main"].add_two_op_instr(opcodes.mov, RegIndirect|EightBytes, Register, regBase, regBeth, -8);
	
# *p = 127;
	# mov rbs, QWORD PTR [rbp-8] | movq -8(%rbp), %ras
	dictFuncs["main"].add_two_op_instr(opcodes.mov, Register, RegIndirect|EightBytes, regBeth, regBase, -8);
	
	# mov DWORD PTR [rbs], 333 | movl #333, (%rbs)
	dictFuncs["main"].add_two_op_instr(opcodes.mov, RegIndirect|FourBytes, Immediate, regBeth, 333, 0);
	
	# Register Alaf SHOULD have "333" as its value here.
	# mov ras, DWORD PTR [rbs] | movl %ras, (%rbs)
	dictFuncs["main"].add_two_op_instr(opcodes.mov, Register, RegIndirect|FourBytes, regAlaf, regBase, -12);
	
	# ret
	dictFuncs["main"].add_op_instr(opcodes.ret);
	
	code = bytearray();
	functable = make_func_table(dictFuncs);
	vartable = make_var_table(None);
	for i in make_header(128, 0, len(functable)):
		code.append(i);
	for i in functable:
		code.append(i);
	for i in vartable:
		code.append(i);
	tbc.write(code);


'''
int main()
{
	puts("hello world\n");
}
'''
with open('test_puts_helloworld.tbc', 'wb+') as tbc:
	dictFuncs = collections.OrderedDict();
	dictFuncs["main"] = CFuncInfo();
	dictFuncs["puts"] = CFuncInfo(isnative=True);
	
# main:
# puts("hello world\n");
	# lea	rps, [global_table+0]	; load the address of the string to rps
	dictFuncs["main"].add_two_op_instr(opcodes.lea, Immediate, Immediate, regPeh, 0);
	
	# syscall 'puts', 1
	# syscalls are negative
	dictFuncs["main"].add_syscall(1, Immediate, 0xfffffffffffffffe);
	
	# ret
	dictFuncs["main"].add_op_instr(opcodes.ret);
	
	ba = bytearray();
	for c in b"hello world\n":
		ba += c.to_bytes(1, byteorder='little');
	ba += 0x0.to_bytes(1, byteorder='little');
	
	dictVars = collections.OrderedDict();
	dictVars["str0"] = CGlobalVarInfo(len("hello world\n")+1, ba);
	
	code = bytearray();
	functable = make_func_table(dictFuncs);
	vartable = make_var_table(dictVars);
	for i in make_header(128, 0, len(functable)):
		code.append(i);
	for i in functable:
		code.append(i);
	for i in vartable:
		code.append(i);
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
	dictFuncs = collections.OrderedDict();
	factorial = CFuncInfo();
	pc = 0;
# factorial:
	# sub rsp, 16
	pc += factorial.add_two_op_instr(opcodes.sub, Register, Immediate, regStk, 16);
	# mov DWORD PTR [rbp-4], rps
	pc += factorial.add_two_op_instr(opcodes.mov, RegIndirect|FourBytes, Register, regBase, regPeh, -4);
# if( i<=1 )
	# cmp DWORD PTR [rbp-4], 1
	pc += factorial.add_two_op_instr(opcodes.cmp, RegIndirect|FourBytes, Immediate, regBase, 1, -4);
	# jz [rip+14]
	pc += factorial.add_one_op_instr(opcodes.jz, Immediate, 14);
	
# return 1;
	# mov ras, 1
	pc += factorial.add_two_op_instr(opcodes.mov, Register, Immediate, regAlaf, 1);
	# ret
	pc += factorial.add_op_instr(opcodes.ret);
	
# return i * factorial(i-1);
	# mov ras, DWORD PTR [rbp-4]
	pc += factorial.add_two_op_instr(opcodes.mov, Register, RegIndirect|FourBytes, regAlaf, regBase, -4);
	
	# sub ras, 1
	pc += factorial.add_two_op_instr(opcodes.sub, Register, Immediate, regAlaf, 1);
	
	# mov rps, ras
	pc += factorial.add_two_op_instr(opcodes.mov, Register, Register, regPeh, regAlaf);
	
	# call factorial
	factorial.add_one_op_instr(opcodes.call, Immediate, 0xffffffffffffff9f);
	
	# mul ras, DWORD PTR [rbp-4]
	factorial.add_two_op_instr(opcodes.mul, Register, RegIndirect|FourBytes, regAlaf, regBase, -4);
	# ret
	factorial.add_op_instr(opcodes.ret);
	
	dictFuncs["factorial"] = factorial;
	
	code = bytearray();
	functable = make_func_table(dictFuncs);
	vartable = make_var_table(None);
	for i in make_header(128, 0, len(functable)):
		code.append(i);
	for i in functable:
		code.append(i);
	for i in vartable:
		code.append(i);
	tbc.write(code);


'''
// test a game-like type of vector calculation.
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
	dictFuncs = collections.OrderedDict();
	VecInvert = CFuncInfo();
	main = CFuncInfo();
	
# main:
	# sub rsp, 32
	main.add_two_op_instr(opcodes.sub, Register, Immediate, regStk, 32);
	
	# lea rps, [rbp-24]
	main.add_two_op_instr(opcodes.lea, 0, 0, regPeh, regBase, -24);
	
	# mov DWORD PTR [rbp-24], 0x40000000
	# mov DWORD PTR [rbp-20], 0x40400000
	# mov DWORD PTR [rbp-16], 0x40800000
	main.add_two_op_instr(opcodes.mov, RegIndirect|FourBytes, Immediate, regBase, float32_to_int(2.0), -24);
	main.add_two_op_instr(opcodes.mov, RegIndirect|FourBytes, Immediate, regBase, float32_to_int(3.0), -20);
	main.add_two_op_instr(opcodes.mov, RegIndirect|FourBytes, Immediate, regBase, float32_to_int(4.0), -16);
	
	# call VecInvert
	main.add_one_op_instr(opcodes.call, Immediate, 14 + 9 + len("VecInvert") + 1);
	
	main.add_two_op_instr(opcodes.mov, Register, Immediate, regAlaf, 0);
	main.add_op_instr(opcodes.ret);
	dictFuncs["main"] = main;
	
# VecInvert:
	# v[0] = -v[0];
	# mov rfs, DWORD PTR [rps]
	VecInvert.add_two_op_instr(opcodes.mov, Register, RegIndirect|FourBytes, regFeh, regPeh, 0);
	
	# negf rfs
	VecInvert.add_one_op_instr(opcodes.negf, Register|FourBytes, regFeh);
	
	# mov DWORD PTR [rps], rfs
	VecInvert.add_two_op_instr(opcodes.mov, RegIndirect|FourBytes, Register, regPeh, regFeh, 0);
	
	# v[1] = -v[1];
	# mov rcs, DWORD PTR [rps+4]
	VecInvert.add_two_op_instr(opcodes.mov, Register, RegIndirect|FourBytes, regSade, regPeh, 4);
	
	# negf rcs
	VecInvert.add_one_op_instr(opcodes.negf, Register|FourBytes, regSade);
	
	# mov DWORD PTR [rps+4], rcs
	VecInvert.add_two_op_instr(opcodes.mov, RegIndirect|FourBytes, Register, regPeh, regSade, 4);
	
	# v[2] = -v[2];
	# mov rqs, DWORD PTR [rps+8]
	VecInvert.add_two_op_instr(opcodes.mov, Register, RegIndirect|FourBytes, regQof, regPeh, 8);
	
	# negf rqs
	VecInvert.add_one_op_instr(opcodes.negf, Register|FourBytes, regQof);
	
	# mov DWORD PTR [rps+8], rqs
	VecInvert.add_two_op_instr(opcodes.mov, RegIndirect|FourBytes, Register, regPeh, regQof, 8);
	VecInvert.add_op_instr(opcodes.ret);
	
	dictFuncs["VecInvert"] = VecInvert;
	
	code = bytearray();
	functable = make_func_table(dictFuncs);
	vartable = make_var_table(None);
	for i in make_header(128, 0, len(functable)):
		code.append(i);
	for i in functable:
		code.append(i);
	for i in vartable:
		code.append(i);
	tbc.write(code);


'''
// Purpose: test out stdin.

int main()
{
	char string[256];
	puts("Please enter a long string: ");
	fgets(string, 256, stdin);
}
'''
with open('test_stdin.tbc', 'wb+') as tbc:
	main = CFuncInfo();
	
# main:
	# char string[256];
	# sub rsp, 256
	main.add_two_op_instr(opcodes.sub, Register, Immediate, regStk, 256);
	
	# puts("Please enter a long string: ");
	# lea	rps, [global_table+0]	; load the address of the string to rps
	main.add_two_op_instr(opcodes.lea, Immediate, Immediate, regPeh, 0);
	
	# syscall puts, 1
	main.add_syscall(1, Immediate, 0xfffffffffffffffe);
	
 # fgets(string, 256, stdin);
	# lea rcs [global_table+1]
	main.add_two_op_instr(opcodes.lea, Immediate, Immediate, regSade, 1);
	
	# mov rcs, [rcs]	;loading a global loads only its address, we need the actual ptr value of stdin.
	main.add_two_op_instr(opcodes.mov, Register, RegIndirect|EightBytes, regSade, regSade, 0);
	
	# mov rfs, 256;
	main.add_two_op_instr(opcodes.mov, Register, Immediate, regFeh, 256);
	
	# lea rps, [rbp-256];
	main.add_two_op_instr(opcodes.lea, 0, 0, regPeh, regBase, -256);
	# syscall fgets, 3
	main.add_syscall(3, Immediate, 0xfffffffffffffffd);
	
	main.add_op_instr(opcodes.ret);
	
	dictFuncs = collections.OrderedDict();
	dictFuncs["main"] = main;
	dictFuncs["puts"] = CFuncInfo(True);
	dictFuncs["fgets"] = CFuncInfo(True);
	
	ba = bytearray();
	for c in b"Please enter a long string: ":
		ba += c.to_bytes(1, byteorder='little');
	ba += 0x0.to_bytes(1, byteorder='little');
	
	dictVars = collections.OrderedDict();
	dictVars["str0"] = CGlobalVarInfo(len("Please enter a long string: ")+1, ba);
	dictVars["stdin"] = CGlobalVarInfo(8, None, True);
	
	code = bytearray();
	functable = make_func_table(dictFuncs);
	vartable = make_var_table(dictVars);
	for i in make_header(128, 0, len(functable)):
		code.append(i);
	for i in functable:
		code.append(i);
	for i in vartable:
		code.append(i);
	tbc.write(code);


'''
// Purpose: test a host exported global variable

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
with open('test_exported_hostvar.tbc', 'wb+') as tbc:
	main = CFuncInfo();
	
	# lea rcs, [global_table+0]
	# mov rcs, [rcs]
	main.add_two_op_instr(opcodes.lea, Immediate, Immediate, regSade, 0);
	main.add_two_op_instr(opcodes.mov, Register, RegIndirect|EightBytes, regSade, regSade, 0);
	
	# mov DWORD PTR [rcs], 300.f
	# mov DWORD PTR [rcs], 100
	# mov DWORD PTR [rcs], 32
	main.add_two_op_instr(opcodes.mov, RegIndirect|FourBytes, Immediate, regSade, float32_to_int(300.0), 0);
	main.add_two_op_instr(opcodes.mov, RegIndirect|FourBytes, Immediate, regSade, 100, 4);
	main.add_two_op_instr(opcodes.mov, RegIndirect|FourBytes, Immediate, regSade, 32, 8);
	
	main.add_op_instr(opcodes.ret);
	
	dictFuncs = collections.OrderedDict();
	dictFuncs["main"] = main;
	
	dictVars = collections.OrderedDict();
	dictVars["g_pPlayer"] = CGlobalVarInfo(8, None, True);
	
	code = bytearray();
	functable = make_func_table(dictFuncs);
	vartable = make_var_table(dictVars);
	for i in make_header(128, 0, len(functable)):
		code.append(i);
	for i in functable:
		code.append(i);
	for i in vartable:
		code.append(i);
	tbc.write(code);


'''
// Purpose: test main's function parameters like argv.

int main(int argc, char *argv[])
{
	puts(argv[1]);
}
'''
with open('test_main_args.tbc', 'wb+') as tbc:
	main = CFuncInfo();
	
# main:
	# mov	rbs, rps	; save value of rps which holds 'argc'
	main.add_two_op_instr(opcodes.mov, Register, Register, regBeth, regPeh);
	
	# mov rps, rfs
	main.add_two_op_instr(opcodes.mov, Register, RegIndirect|EightBytes, regPeh, regFeh, 0);
	
	# puts(argv[1]);
	# syscall puts 1
	main.add_syscall(1, Immediate, 0xfffffffffffffffe);
	
	main.add_op_instr(opcodes.ret);
	
	dictFuncs = collections.OrderedDict();
	dictFuncs["main"] = main;
	dictFuncs["puts"] = CFuncInfo(isnative=True);
	
	code = bytearray();
	functable = make_func_table(dictFuncs);
	vartable = make_var_table(None);
	for i in make_header(128, 0, len(functable)):
		code.append(i);
	for i in functable:
		code.append(i);
	for i in vartable:
		code.append(i);
	tbc.write(code);


'''
// Purpose: test dynamic loading.

int main(int argc, char *argv[])
{
	void *module = tagha_dlopen("factorial.tbc");
	uint32_t (*factorial)() = tagha_dlsym(module, "factorial");
	return factorial(5);
}
'''

with open('test_dynamic_loading.tbc', 'wb+') as tbc:
	main = CFuncInfo();
	
# main:
  # void *module = tagha_dlopen("factorial.tbc");
	# lea rps [global_offset+0]
	main.add_two_op_instr(opcodes.lea, Immediate, Immediate, regPeh, 0);
	
	# syscall tagha_dlopen, 1
	main.add_syscall(1, Immediate, 0xfffffffffffffffe);
	
	# mov QWORD PTR [rbp-40], ras
	main.add_two_op_instr(opcodes.mov, RegIndirect|EightBytes, Register, regBase, regAlaf, -40);
	
  # uint32_t (*factorial)() = tagha_dlsym(module, "factorial");
	# mov ras, QWORD PTR [rbp-40]
	main.add_two_op_instr(opcodes.mov, Register, RegIndirect|EightBytes, regAlaf, regBase, -40);
	
	# lea rfs, [global_offset+1]
	main.add_two_op_instr(opcodes.lea, Immediate, Immediate, regFeh, 1);
	
	# mov rps, ras
	main.add_two_op_instr(opcodes.mov, Register, Register, regPeh, regAlaf);
	
	# syscall tagha_dlsym, 2
	main.add_syscall(2, Immediate, 0xfffffffffffffffd);
	
	# mov QWORD PTR [rbp-8], ras
	main.add_two_op_instr(opcodes.mov, RegIndirect|EightBytes, Register, regBase, regAlaf, -8);
	
	# mov rbs, QWORD PTR [rbp-8]
	main.add_two_op_instr(opcodes.mov, Register, RegIndirect|EightBytes, regBeth, regBase, -8);
	
	# mov rps, 5
	main.add_two_op_instr(opcodes.mov, Register, Immediate, regPeh, 5);
	
	# call rbs		; call factorial
	main.add_one_op_instr(opcodes.call, Register, regBeth);
	
	# lea rps, QWORD PTR [rbp-40]
	main.add_two_op_instr(opcodes.lea, 0, 0, regPeh, regBase, -40);
	
	# syscall tagha_dlclose, 1
	main.add_syscall(1, Immediate, 0xfffffffffffffffc);
	
	main.add_op_instr(opcodes.ret);
	
	dictFuncs = collections.OrderedDict();
	dictFuncs["main"] = main;
	dictFuncs["tagha_dlopen"] = CFuncInfo(isnative=True);
	dictFuncs["tagha_dlsym"] = CFuncInfo(isnative=True);
	dictFuncs["tagha_dlclose"] = CFuncInfo(isnative=True);
	
	
	code = bytearray();
	functable = make_func_table(dictFuncs);
	
	ba = bytearray();
	for c in b"./test_factorial.tbc":
		ba += c.to_bytes(1, byteorder='little');
	ba += 0x0.to_bytes(1, byteorder='little');
	
	dictVars = collections.OrderedDict();
	dictVars["modulename"] = CGlobalVarInfo(len("./test_factorial.tbc")+1, ba);
	
	baa = bytearray();
	for c in b"factorial":
		baa += c.to_bytes(1, byteorder='little');
	baa += 0x0.to_bytes(1, byteorder='little');
	dictVars["funcname"] = CGlobalVarInfo(len("factorial")+1, baa);
	
	vartable = make_var_table(dictVars);
	for i in make_header(128, 0, len(functable)):
		code.append(i);
	for i in functable:
		code.append(i);
	for i in vartable:
		code.append(i);
	tbc.write(code);

