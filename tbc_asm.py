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

opcodes = enum('halt', 'push', 'pop', 'lea', 'mov', 'movgbl', 'add', 'sub', 'mul', 'divi', 'mod', 'andb', 'orb', 'xorb', 'notb', 'shl', 'shr', 'inc', 'dec', 'neg', 'lt', 'gt', 'cmp', 'neq', 'jmp', 'jz', 'jnz', 'call', 'syscall', 'ret', 'flt2dbl', 'dbl2flt', 'addf', 'subf', 'mulf', 'divf', 'incf', 'decf', 'negf', 'ltf', 'gtf', 'cmpf', 'neqf', 'nop');

Immediate	= 1;
Register	= 2;
RegIndirect	= 4;
Unsign		= 8;
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
	
	def add_two_op_instr(self, opcode:int, first_addrmode:int, sec_addrmode:int, first_oper:int, sec_operand:int, offset=None) -> None:
		if self.Native:
			return;
		
		self.Instrs += opcode.to_bytes(1, byteorder='little'); # write opcode
		self.Instrs += first_addrmode.to_bytes(1, byteorder='little'); # write 1st addrmode
		self.Instrs += sec_addrmode.to_bytes(1, byteorder='little'); # write 2nd addrmode
		self.Instrs += first_oper.to_bytes(1, byteorder='little'); # write 1st register operand value.
		
		if sec_addrmode & Immediate:
			self.Instrs += sec_operand.to_bytes(8, byteorder='little');
		else:
			self.Instrs += sec_operand.to_bytes(1, byteorder='little');
		if offset != None:
			ba = bytearray(struct.pack("i", offset));
			i = int.from_bytes(ba, byteorder='little');
			self.Instrs += i.to_bytes(4, byteorder='little');
	
	def add_one_op_instr(self, opcode:int, addrmode:int, operand:int, offset=None) -> None:
		if self.Native:
			return;
		
		self.Instrs += opcode.to_bytes(1, byteorder='little'); # write opcode
		self.Instrs += addrmode.to_bytes(1, byteorder='little'); # write addrmode
		
		if addrmode & Immediate:
			self.Instrs += operand.to_bytes(8, byteorder='little');
		else:
			self.Instrs += operand.to_bytes(1, byteorder='little');
		if offset != None:
			ba = bytearray(struct.pack("i", offset));
			i = int.from_bytes(ba, byteorder='little');
			self.Instrs += i.to_bytes(4, byteorder='little');
	
	def add_op_instr(self, opcode:int, addrmode=0) -> None:
		if self.Native:
			return;
		
		self.Instrs += opcode.to_bytes(1, byteorder='little'); # write opcode
		self.Instrs += addrmode.to_bytes(1, byteorder='little'); # write addrmode
	
	def add_syscall(self, argcount:int, addrmode:int, operand:int, offset=None) -> None:
		if self.Native:
			return;
		
		self.Instrs += opcodes.syscall.to_bytes(1, byteorder='little'); # write syscall opcode
		self.Instrs += addrmode.to_bytes(1, byteorder='little'); # write addrmode
		self.Instrs += argcount.to_bytes(4, byteorder='little');
		
		if addrmode & Immediate:
			self.Instrs += operand.to_bytes(8, byteorder='little');
		else:
			self.Instrs += operand.to_bytes(1, byteorder='little');
		if offset != None:
			ba = bytearray(struct.pack("i", offset));
			i = int.from_bytes(ba, byteorder='little');
			self.Instrs += i.to_bytes(4, byteorder='little');

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


def write_header(f:bytearray, stacksize:int, flags=0, magic=0xC0DE) -> None:
	f += magic.to_bytes(2, byteorder='little');
	f += stacksize.to_bytes(4, byteorder='little');
	f += flags.to_bytes(1, byteorder='little');

def header_write_funcs(f:bytearray, funcs:dict) -> int:
	i = 0;
	if len(funcs) == 0:
		return 0;
	
	numfuncs = len(funcs);
	f += numfuncs.to_bytes(4, byteorder='little');
	for key, val in funcs.items():
		f += val.Native.to_bytes(1, byteorder='little');
		f += (len(key)+1).to_bytes(4, byteorder='little');
		if val.Native:
			f += 0x8.to_bytes(4, byteorder='little');
		else:
			f += (len(val.Instrs)).to_bytes(4, byteorder='little');
			print("instr len {%d}" % len(val.Instrs));
		f += key.encode('utf-8');
		f += 0x0.to_bytes(1, byteorder='little');
		
		if val.Native:
			f += 0x0.to_bytes(8, byteorder='little');
		else:
			for byte in val.Instrs:
				f.append(byte);
		i += 1;
	return i;

def header_write_globalvars(f:bytearray, globalvars:dict) -> int:
	i = 0;
	if globalvars == None or len(globalvars) == 0:
		f += 0x0.to_bytes(4, byteorder='little');
		return 0;
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
		i += 1;
	return i;

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
	dictVars = collections.OrderedDict();
	code = bytearray();
	write_header(code, 128);
	dictFuncs["main"] = CFuncInfo();
# main:
	# mov regAlaf, 550
	dictFuncs["main"].add_two_op_instr(opcodes.mov, Register, Immediate, regAlaf, 550);
	
	# ret
	dictFuncs["main"].add_op_instr(opcodes.ret);
	
	dictVars["i"] = CGlobalVarInfo(4, 0x0a0b0c0d.to_bytes(4, byteorder='little'));
	header_write_funcs(code, dictFuncs);
	header_write_globalvars(code, dictVars);
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
	code = bytearray();
	write_header(code, 128);
	dictFuncs["main"] = CFuncInfo();
# main:
	# mov regAlaf, 2.f
	dictFuncs["main"].add_two_op_instr(opcodes.mov, Register, Immediate, regAlaf, float32_to_int(2.0));
	
	# addf regAlaf, 4.f
	dictFuncs["main"].add_two_op_instr(opcodes.addf, Register, Immediate|FourBytes, regAlaf, float32_to_int(8.5));
	
	# ret
	dictFuncs["main"].add_op_instr(opcodes.ret);
	header_write_funcs(code, dictFuncs);
	header_write_globalvars(code, None);
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
	code = bytearray();
	write_header(code, 128);
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
	header_write_funcs(code, dictFuncs);
	header_write_globalvars(code, None);
	tbc.write(code);


'''
int main()
{
	puts("hello world\n");
}
'''
with open('test_puts_helloworld.tbc', 'wb+') as tbc:
	dictFuncs = collections.OrderedDict();
	dictVars = collections.OrderedDict();
	code = bytearray();
	write_header(code, 128);
	dictFuncs["main"] = CFuncInfo();
	dictFuncs["puts"] = CFuncInfo(isnative=True);
	
# main:
# puts("hello world\n");
	# movgbl	rps, "hello world\n"[global_table]	; load the address of the string to rps
	dictFuncs["main"].add_two_op_instr(opcodes.movgbl, Register, Immediate, regPeh, 0);
	
	# callnat 'puts', 1
	dictFuncs["main"].add_syscall(1, Immediate, 1);
	
	# ret
	dictFuncs["main"].add_op_instr(opcodes.ret);
	
	ba = bytearray();
	for c in b"hello world\n":
		ba += c.to_bytes(1, byteorder='little');
	ba += 0x0.to_bytes(1, byteorder='little');
	dictVars["str0"] = CGlobalVarInfo(len("hello world\n")+1, ba);
	
	header_write_funcs(code, dictFuncs);
	header_write_globalvars(code, dictVars);
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
	code = bytearray();
	write_header(code, 128, 0, 0xD11);
	factorial = CFuncInfo();
	
# factorial:
	# sub rsp, 16
	factorial.add_two_op_instr(opcodes.sub, Register, Immediate, regStk, 16);
	# mov DWORD PTR [rbp-4], rhs
	factorial.add_two_op_instr(opcodes.mov, RegIndirect|FourBytes, Register, regBase, regPeh, -4);
# if( i<=1 )
	# cmp DWORD PTR [rbp-4], 1
	factorial.add_two_op_instr(opcodes.cmp, RegIndirect|FourBytes, Immediate, regBase, 1, -4);
	# jz [rip+14]
	factorial.add_one_op_instr(opcodes.jz, Immediate, 14);
	
# return 1;
	# mov ras, 1
	factorial.add_two_op_instr(opcodes.mov, Register, Immediate, regAlaf, 1);
	# ret
	factorial.add_op_instr(opcodes.ret);
	
# return i * factorial(i-1);
	# mov ras, DWORD PTR [rbp-4]
	factorial.add_two_op_instr(opcodes.mov, Register, RegIndirect|FourBytes, regAlaf, regBase, -4);
	
	# sub ras, 1
	factorial.add_two_op_instr(opcodes.sub, Register, Immediate, regAlaf, 1);
	
	# mov rhs, ras
	factorial.add_two_op_instr(opcodes.mov, Register, Register, regPeh, regAlaf);
	
	# call factorial
	factorial.add_one_op_instr(opcodes.call, Immediate, 0);
	
	# mul ras, DWORD PTR [rbp-4]
	factorial.add_two_op_instr(opcodes.mul, Register, RegIndirect|FourBytes, regAlaf, regBase, -4);
	# ret
	factorial.add_op_instr(opcodes.ret);
	
	dictFuncs["factorial"] = factorial;
	header_write_funcs(code, dictFuncs);
	header_write_globalvars(code, None);
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
	dictFuncs = collections.OrderedDict();
	code = bytearray();
	write_header(code, 128);
	VecInvert = CFuncInfo();
	main = CFuncInfo();
	
# main:
	# sub rsp, 32
	main.add_two_op_instr(opcodes.sub, Register, Immediate, regStk, 32);
	
	# lea rhs, [rbp-24]
	main.add_two_op_instr(opcodes.lea, Register, RegIndirect, regPeh, regBase, -24);
	
	# mov DWORD PTR [rbp-24], 0x40000000
	# mov DWORD PTR [rbp-20], 0x40400000
	# mov DWORD PTR [rbp-16], 0x40800000
	main.add_two_op_instr(opcodes.mov, RegIndirect|FourBytes, Immediate, regBase, float32_to_int(2.0), -24);
	main.add_two_op_instr(opcodes.mov, RegIndirect|FourBytes, Immediate, regBase, float32_to_int(3.0), -20);
	main.add_two_op_instr(opcodes.mov, RegIndirect|FourBytes, Immediate, regBase, float32_to_int(4.0), -16);
	
	# call VecInvert
	main.add_one_op_instr(opcodes.call, Immediate, 1);
	
	main.add_two_op_instr(opcodes.mov, Register, Immediate, regAlaf, 0);
	main.add_op_instr(opcodes.ret);
	dictFuncs["main"] = main;
	
# VecInvert:
	# v[0] = -v[0];
	# mov rfs, DWORD PTR [rhs]
	VecInvert.add_two_op_instr(opcodes.mov, Register, RegIndirect|FourBytes, regFeh, regPeh, 0);
	
	# negf rfs
	VecInvert.add_one_op_instr(opcodes.negf, Register|FourBytes, regFeh);
	
	# mov DWORD PTR [rhs], rfs
	VecInvert.add_two_op_instr(opcodes.mov, RegIndirect|FourBytes, Register, regPeh, regFeh, 0);
	
	# v[1] = -v[1];
	# mov rcs, DWORD PTR [rhs+4]
	VecInvert.add_two_op_instr(opcodes.mov, Register, RegIndirect|FourBytes, regSade, regPeh, 4);
	
	# negf rcs
	VecInvert.add_one_op_instr(opcodes.negf, Register|FourBytes, regSade);
	
	# mov DWORD PTR [rhs+4], rcs
	VecInvert.add_two_op_instr(opcodes.mov, RegIndirect|FourBytes, Register, regPeh, regSade, 4);
	
	# v[2] = -v[2];
	# mov rqs, DWORD PTR [rhs+8]
	VecInvert.add_two_op_instr(opcodes.mov, Register, RegIndirect|FourBytes, regQof, regPeh, 8);
	
	# negf rqs
	VecInvert.add_one_op_instr(opcodes.negf, Register|FourBytes, regQof);
	
	# mov DWORD PTR [rhs+8], rqs
	VecInvert.add_two_op_instr(opcodes.mov, RegIndirect|FourBytes, Register, regPeh, regQof, 8);
	VecInvert.add_op_instr(opcodes.ret);
	
	dictFuncs["VecInvert"] = VecInvert;
	
	header_write_funcs(code, dictFuncs);
	header_write_globalvars(code, None);
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
	dictFuncs = collections.OrderedDict();
	dictVars = collections.OrderedDict();
	code = bytearray();
	write_header(code, 128);
	dictFuncs["main"] = CFuncInfo();
	dictFuncs["puts"] = CFuncInfo(True);
	dictFuncs["fgets"] = CFuncInfo(True);
	
# main:
	# char string[256];
	# sub rsp, 256
	dictFuncs["main"].add_two_op_instr(opcodes.sub, Register, Immediate, regStk, 256);
	
	# puts("Please enter a long string: ");
	# movgbl	rps, [global_table+0]	; load the address of the string to rps
	dictFuncs["main"].add_two_op_instr(opcodes.movgbl, Register, Immediate, regPeh, 0);
	
	# callnat 'puts', 1
	dictFuncs["main"].add_syscall(1, Immediate, 1);
	
 # fgets(string, 256, stdin);
	# movgbl rcs 1
	dictFuncs["main"].add_two_op_instr(opcodes.movgbl, Register, Immediate, regSade, 1);
	
	# mov rcs, [rcs]	;loading a global loads only its address, we need the actual ptr value of stdin.
	dictFuncs["main"].add_two_op_instr(opcodes.mov, Register, RegIndirect|EightBytes, regSade, regSade, 0);
	
	# mov rfs, 256;
	dictFuncs["main"].add_two_op_instr(opcodes.mov, Register, Immediate, regFeh, 256);
	
	# lea rps, [rbp-256];
	dictFuncs["main"].add_two_op_instr(opcodes.lea, Register, RegIndirect|EightBytes, regPeh, regBase, -256);
	# syscall fgets
	dictFuncs["main"].add_syscall(3, Immediate, 2);
	
	dictFuncs["main"].add_op_instr(opcodes.ret);
	
	ba = bytearray();
	for c in b"Please enter a long string: ":
		ba += c.to_bytes(1, byteorder='little');
	ba += 0x0.to_bytes(1, byteorder='little');
	dictVars["str0"] = CGlobalVarInfo(len("Please enter a long string: ")+1, ba);
	dictVars["stdin"] = CGlobalVarInfo(8, None, True);
	
	header_write_funcs(code, dictFuncs);
	header_write_globalvars(code, dictVars);
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
	dictFuncs = collections.OrderedDict();
	dictVars = collections.OrderedDict();
	code = bytearray();
	write_header(code, 128);
	main = CFuncInfo();
	
	# movgbl rcs, 0
	# mov rcs, [rcs]
	main.add_two_op_instr(opcodes.movgbl, Register, Immediate, regSade, 0);
	main.add_two_op_instr(opcodes.mov, Register, RegIndirect|EightBytes, regSade, regSade, 0);
	
	# mov DWORD PTR [rcs], 300.f
	# mov DWORD PTR [rcs], 100
	# mov DWORD PTR [rcs], 32
	main.add_two_op_instr(opcodes.mov, RegIndirect|FourBytes, Immediate, regSade, float32_to_int(300.0), 0);
	main.add_two_op_instr(opcodes.mov, RegIndirect|FourBytes, Immediate, regSade, 100, 4);
	main.add_two_op_instr(opcodes.mov, RegIndirect|FourBytes, Immediate, regSade, 32, 8);
	
	main.add_op_instr(opcodes.ret);
	
	dictFuncs["main"] = main;
	dictVars["g_pPlayer"] = CGlobalVarInfo(8, None, True);
	
	header_write_funcs(code, dictFuncs);
	header_write_globalvars(code, dictVars);
	tbc.write(code);


'''
// Purpose: test main's function parameters like argv.

int main(int argc, char *argv[])
{
	puts(argv[1]);
}
'''
with open('test_main_args.tbc', 'wb+') as tbc:
	dictFuncs = collections.OrderedDict();
	code = bytearray();
	write_header(code, 128);
	main = CFuncInfo();
	
# main:
	# mov	rbs, rps	; save value of rps which holds 'argc'
	main.add_two_op_instr(opcodes.mov, Register, Register, regBeth, regPeh);
	
	# *argv++;
	# mov rps, rfs
	main.add_two_op_instr(opcodes.mov, Register, RegIndirect|EightBytes, regPeh, regFeh, 0);
	
	# puts(argv[1]);
	main.add_syscall(1, Immediate, 1);
	
	main.add_op_instr(opcodes.ret);
	
	dictFuncs["main"] = main;
	dictFuncs["puts"] = CFuncInfo(isnative=True);
	
	header_write_funcs(code, dictFuncs);
	header_write_globalvars(code, None);
	tbc.write(code);
