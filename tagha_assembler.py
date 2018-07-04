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

Alaf=0
Beth=1
Veth=2
Gamal=3
Ghamal=4
Dalath=5
Dhalath=6
Heh=7
Waw=8
Zain=9
Heth=10
Teth=11
Yodh=12
Kaf=13
Khaf=14
Lamad=15
Meem=16
Noon=17
Semkath=18
_Eh=19
Peh=20
Feh=21
Sade=22
Qof=23
Reesh=24
Sheen=25
Taw=26
Thaw=27
Stk = 28;
Base = 29;
Instr = 30;
RegSize = 31;

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
	
	def add_one_op_instr(self, opcode:int, addrmode:int, operand:int, offset=None) -> int:
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
	
	def add_syscall(self, argcount:int, addrmode:int, operand:int, offset=None) -> int:
		if self.Native:
			return;
		
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

def is_whitespace(c:str) -> bool:
	return( c == ' ' or c == '\t' or c == '\r' or c == '\v' or c == '\f' or c == '\n' );

if __name__ == "__main__":
	code = "";
	with open(sys.argv[1], 'r') as file:
		code = file.read();
	
	srcindex = 0;
	srclen = len(code);
	while srcindex < srclen:
		if is_whitespace(code[srcindex]):
			srcindex += 1;
			continue;


