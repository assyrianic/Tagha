#!/usr/bin/python3

import sys;
import ctypes;
import struct;

def enum(*sequential, **named) -> object:
	enums = dict(zip(sequential, range(len(sequential))), **named);
	return type('Enum', (), enums);

opcodes = enum('halt', 'pushq', 'pushl', 'pushs', 'pushb', 'pushsp', 'puship', 'pushbp', 'pushaddr', 'pushspadd', 'pushspsub', 'pushbpadd', 'pushbpsub', 'pushipadd', 'pushipsub', 'popq', 'popl', 'pops', 'popb', 'popsp', 'popip', 'popbp', 'storespq', 'storespl', 'storesps', 'storespb', 'loadspq', 'loadspl', 'loadsps', 'loadspb', 'copyq', 'copyl', 'copys', 'copyb', 'addq', 'uaddq', 'addl', 'uaddl', 'addf', 'subq', 'usubq', 'subl', 'usubl', 'subf', 'mulq', 'umulq', 'mull', 'umull', 'mulf', 'divq', 'udivq', 'divl', 'udivl', 'divf', 'modq', 'umodq', 'modl', 'umodl', 'addf64', 'subf64', 'mulf64', 'divf64', 'andl', 'orl', 'xorl', 'notl', 'shll', 'shrl', 'andq', 'orq', 'xorq', 'notq', 'shlq', 'shrq', 'incq', 'incl', 'incf', 'decq', 'decl', 'decf', 'negq', 'negl', 'negf', 'incf64', 'decf64', 'negf64', 'ltq', 'ltl', 'ultq', 'ultl', 'ltf', 'gtq', 'gtl', 'ugtq', 'ugtl', 'gtf', 'cmpq', 'cmpl', 'ucmpq', 'ucmpl', 'compf', 'leqq', 'uleqq', 'leql', 'uleql', 'leqf', 'geqq', 'ugeqq', 'geql', 'ugeql', 'geqf', 'ltf64', 'gtf64', 'cmpf64', 'leqf64', 'geqf64', 'neqq', 'uneqq', 'neql', 'uneql', 'neqf', 'neqf64', 'jmp', 'jzq', 'jnzq', 'jzl', 'jnzl', 'call', 'calls', 'ret', 'retx', 'reset', 'pushnataddr', 'callnat', 'callnats', 'nop');

g_dictLabels = {};


def is_potential_identifier(c:str) -> bool:
	return( (c >= 'a' and c <= 'z')
		or (c >= 'A' and c <= 'Z')
		or c == '_'
		or (c >= '0' and c <= '9')
		or c >= chr(255) );

def is_alphabetic(c:str) -> bool:
	return( (c >= 'a' and c <= 'z')
		or (c >= 'A' and c <= 'Z')
		or c == '_'
		or c >= chr(255) );

def is_whitespace(c:str) -> bool:
	return( c == '\t' or c == '\r' or c == '\v' or c == '\f' or c == '\n' );
	
def is_hex(c:str) -> bool:
	return( (c >= 'a' and c <= 'f') or (c >= 'A' and c <= 'F') or (c >= '0' and c <= '9') );

def is_octal(c:str) -> bool:
	return( c >= '0' and c <= '7' );

def is_numeric(c:str) -> bool:
	return( c >= '0' and c <= '9' );


def prep_file(filename:str) -> list:
	lstLines=[];
	with open(filename, 'r+') as objFile:
		strTok = "";
		for line in objFile.readlines():
			for char in line:
				if char==';':	# remove comments
					break;
				strTok += char;
			
			lstLines.append(strTok);
			strTok = "";
		
	return lstLines;


def asmlify(lines:list) -> list:
	iAddr=0;
	# first pass: resolve the label references.
	for line in lines:
		print(line);


def wrt_hdr(f, memsize:int) -> None:
	f.write(0xC0DE.to_bytes(2, byteorder='little'));
	f.write(memsize.to_bytes(4, byteorder='little'));

def wrt_hdr_natives(f, *natives) -> None:
	i = 0;
	numnatives = len(natives);
	f.write(numnatives.to_bytes(4, byteorder='little'));
	while i<numnatives:
		#f.write(natives[i].to_bytes(4, byteorder='little'));
		#print((len(natives[i])+1).to_bytes(4, byteorder='little'));
		f.write((len(natives[i])+1).to_bytes(4, byteorder='little'));
		f.write(natives[i].encode('utf-8'));
		f.write(0x0.to_bytes(1, byteorder='little'));
		i += 1;

def wrt_hdr_funcs(f, *funcs) -> None:
	i = 0;
	numfuncs = len(funcs) // 3;
	f.write(numfuncs.to_bytes(4, byteorder='little'));
	while i<numfuncs*3:
		strsize = len(funcs[i])+1;
		f.write(strsize.to_bytes(4, byteorder='little'));
		f.write(funcs[i].encode('utf-8'));
		f.write(0x0.to_bytes(1, byteorder='little'));
		i += 1;
		
		f.write(funcs[i].to_bytes(4, byteorder='little'));
		i += 1;
		
		f.write(funcs[i].to_bytes(4, byteorder='little'));
		i += 1;

def wrt_hdr_globals(f, *lGlobals) -> None:
	i = 0;
	numglobals = len(lGlobals) // 4;
	f.write(numglobals.to_bytes(4, byteorder='little'));
	
	while i<numglobals*4:
		strsize = len(lGlobals[i])+1;
		f.write(strsize.to_bytes(4, byteorder='little'));
		f.write(lGlobals[i].encode('utf-8'));
		f.write(0x0.to_bytes(1, byteorder='little'));
		i += 1;
		
		# write the stack address of this global var.
		f.write(lGlobals[i].to_bytes(4, byteorder='little'));
		i += 1;
		
		# write how many bytes this data takes up.
		bytecount = lGlobals[i];
		f.write(lGlobals[i].to_bytes(4, byteorder='little'));
		i += 1;
		
		# write the actual data
		if type(lGlobals[i]) == float:
			if bytecount==4:
				ba = bytearray(struct.pack("f", lGlobals[i]));
				n = int.from_bytes(ba, byteorder='little');
				f.write(n.to_bytes(bytecount, byteorder='little'));
			elif bytecount==8:
				ba = bytearray(struct.pack("d", lGlobals[i]));
				n = int.from_bytes(ba, byteorder='little');
				f.write(n.to_bytes(bytecount, byteorder='little'));
		elif type(lGlobals[i])==str:
			for x in lGlobals[i]:
				f.write(ord(x).to_bytes(1, byteorder='little'));
			f.write(0x0.to_bytes(1, byteorder='little'));
		else:
			f.write(lGlobals[i].to_bytes(bytecount, byteorder='little'));
		i += 1;

def wrt_hdr_footer(f, entry=0, modes=3) -> None:
	f.write(entry.to_bytes(8, byteorder='little'));
	# 1 for safemode, 2 for debugmode, 3 for both.
	f.write(modes.to_bytes(1, byteorder='little'));

def wrt_opcode(f, opcode:int) -> None:
	f.write(opcode.to_bytes(1, byteorder='little'));

def wrt_pushq(f, val:any) -> None:
	f.write(opcodes.pushq.to_bytes(1, byteorder='little'));
	if type(val) == float:
		ba = bytearray(struct.pack("d", val));
		i = int.from_bytes(ba, byteorder='little');
		f.write(i.to_bytes(8, byteorder='little'));
	else:
		f.write(val.to_bytes(8, byteorder='little'));

def wrt_pushl(f, val:any) -> None:
	f.write(opcodes.pushl.to_bytes(1, byteorder='little'));
	if type(val) == float:
		ba = bytearray(struct.pack("f", val));
		i = int.from_bytes(ba, byteorder='little');
		f.write(i.to_bytes(4, byteorder='little'));
	else:
		f.write(val.to_bytes(4, byteorder='little'));
	
def wrt_push_smaller(f, size:int, val:int) -> None:
	if size==2:
		f.write(opcodes.pushs.to_bytes(1, byteorder='little'));
		f.write(val.to_bytes(2, byteorder='little'));
	else:
		f.write(opcodes.pushb.to_bytes(1, byteorder='little'));
		f.write(val.to_bytes(1, byteorder='little'));

def wrt_1op_4byte(f, opcode:int, val:int) -> None:
	f.write(opcode.to_bytes(1, byteorder='little'));
	f.write(val.to_bytes(4, byteorder='little'));
	
def wrt_1op_8byte(f, opcode:int, val:int) -> None:
	f.write(opcode.to_bytes(1, byteorder='little'));
	f.write(val.to_bytes(8, byteorder='little'));

def wrt_callnat(f, index:int, bytespushed:int, argcount:int) -> None:
	f.write(opcodes.callnat.to_bytes(1, byteorder='little'));
	f.write(index.to_bytes(4, byteorder='little'));
	f.write(bytespushed.to_bytes(4, byteorder='little'));
	f.write(argcount.to_bytes(4, byteorder='little'));

def wrt_callnats(f, bytespushed:int, argcount:int) -> None:
	f.write(opcodes.callnats.to_bytes(1, byteorder='little'));
	f.write(bytespushed.to_bytes(4, byteorder='little'));
	f.write(argcount.to_bytes(4, byteorder='little'));


'''
unsigned i = 0x0a0b0c0d;
int main()
{
	return 0;
}
'''
with open('endian_test1.tbc', 'wb+') as tbc:
	wrt_hdr(tbc, 64);
	wrt_hdr_natives(tbc);
	wrt_hdr_funcs(tbc, 'main', 0, 10);
	wrt_hdr_globals(tbc, 'i', 63-4, 4, 0x0a0b0c0d);
	wrt_hdr_footer(tbc, entry=0);
	
	wrt_1op_8byte(tbc, opcodes.call, 10);
	wrt_opcode(tbc, opcodes.halt);
	
	wrt_pushl(tbc, 0);
	wrt_1op_4byte(tbc, opcodes.retx, 4);

with open('float_test.tbc', 'wb+') as tbc:
	wrt_hdr(tbc, 64);
	wrt_hdr_natives(tbc);
	wrt_hdr_funcs(tbc);
	wrt_hdr_globals(tbc, 'flTen', 63-4, 4, 10.0, 'flTwo', 63-8, 4, 2.0);
	wrt_hdr_footer(tbc, entry=0);
	
	wrt_opcode(tbc, opcodes.addf);
	wrt_opcode(tbc, opcodes.halt);

'''
int main()
{
	puts("hello world\n");
}
'''
with open('hello_world.tbc', 'wb+') as tbc:
	wrt_hdr(tbc, 64);
	wrt_hdr_natives(tbc, 'puts');
	wrt_hdr_funcs(tbc, 'main', 0, 10);
	wrt_hdr_globals(tbc, 'str00001', 63-(len('hello world\n')+1), len('hello world\n')+1, 'hello world\n');
	wrt_hdr_footer(tbc, entry=0);
	
	# stack starts at 31, pushing 13 chars, left with 18.
	wrt_1op_8byte(tbc, opcodes.call, 10);
	wrt_opcode(tbc, opcodes.halt);
	
	wrt_1op_8byte(tbc, opcodes.pushaddr, 63-(len('hello world\n')+1));
	wrt_callnat(tbc, 0, 8, 1);
	wrt_pushl(tbc, 0);
	wrt_1op_4byte(tbc, opcodes.retx, 4);

with open('pointers.tbc', 'wb+') as tbc:
	wrt_hdr(tbc, 64);
	wrt_hdr_natives(tbc);
	wrt_hdr_funcs(tbc, 'main', 0, 10);
	wrt_hdr_globals(tbc, 't0', 63-4, 4, 50000);
	wrt_hdr_footer(tbc, entry=0);
	
	# 687 in hex
	wrt_pushl(tbc, 687);
	wrt_1op_8byte(tbc, opcodes.pushaddr, 63-4);
	wrt_opcode(tbc, opcodes.storespl);
	wrt_opcode(tbc, opcodes.halt);

with open('test_func_call.tbc', 'wb+') as tbc:
	wrt_hdr(tbc, 64);
	wrt_hdr_natives(tbc);
	wrt_hdr_funcs(tbc, "func", 2, 22);
	wrt_hdr_globals(tbc);
	wrt_hdr_footer(tbc, entry=0);
	
	wrt_pushl(tbc, 500);
	wrt_pushl(tbc, 2);
	wrt_1op_8byte(tbc, opcodes.call, 22);
	wrt_opcode(tbc, opcodes.popl);
	wrt_opcode(tbc, opcodes.popl);
	
	wrt_opcode(tbc, opcodes.halt);
	wrt_1op_8byte(tbc, opcodes.pushaddr, 55);
	wrt_opcode(tbc, opcodes.loadspl);
	wrt_1op_8byte(tbc, opcodes.pushaddr, 59);
	wrt_opcode(tbc, opcodes.loadspl);
	wrt_opcode(tbc, opcodes.addl);
	wrt_opcode(tbc, opcodes.ret);

with open('test_call_opcodes.tbc', 'wb+') as tbc:
	wrt_hdr(tbc, 64);
	wrt_hdr_natives(tbc);
	wrt_hdr_funcs(tbc, 'func1', 0, 20, 'func2', 0, 26);
	wrt_hdr_globals(tbc);
	wrt_hdr_footer(tbc, entry=0);
	
	wrt_1op_8byte(tbc, opcodes.call, 20);
	wrt_pushq(tbc, 26);
	wrt_opcode(tbc, opcodes.calls);
	wrt_opcode(tbc, opcodes.halt);
	
	wrt_pushl(tbc, 0x0a0b0c0d);
	wrt_opcode(tbc, opcodes.ret);
	
	wrt_pushl(tbc, 0xffff);
	wrt_opcode(tbc, opcodes.ret);

with open('test_retx_func.tbc', 'wb+') as tbc:
	wrt_hdr(tbc, 64);
	wrt_hdr_natives(tbc);
	wrt_hdr_funcs(tbc, 'f', 1, 15);
	wrt_hdr_globals(tbc);
	wrt_hdr_footer(tbc, entry=0);
	
	wrt_pushl(tbc, 9);
	wrt_1op_8byte(tbc, opcodes.call, 15);
	wrt_opcode(tbc, opcodes.halt);
	
	wrt_pushq(tbc, 16);
	wrt_opcode(tbc, opcodes.pushbpadd);
	wrt_opcode(tbc, opcodes.loadspl);
	wrt_pushl(tbc, 6);
	wrt_opcode(tbc, opcodes.addl);
	wrt_1op_4byte(tbc, opcodes.retx, 4);

with open('test_recursion.tbc', 'wb+') as tbc:
	wrt_hdr(tbc, 0xffffff);
	wrt_hdr_natives(tbc);
	wrt_hdr_funcs(tbc, 'recursive', 0, 10);
	wrt_hdr_globals(tbc);
	wrt_hdr_footer(tbc, entry=0, modes=3);
	
	wrt_1op_8byte(tbc, opcodes.call, 10);
	wrt_opcode(tbc, opcodes.halt);
	wrt_1op_8byte(tbc, opcodes.call, 10);

with open('test_factorial_recurs.tbc', 'wb+') as tbc:
	wrt_hdr(tbc, 255);
	wrt_hdr_natives(tbc);
	wrt_hdr_funcs(tbc, 'factorial', 1, 15);
	wrt_hdr_globals(tbc);
	wrt_hdr_footer(tbc, entry=0);
	#unsigned int factorial(unsigned int i) {
	#	if( i<=1 )
	#		return 1;
	#	return i * factorial( i-1 );
	#}
	
	wrt_pushl(tbc, 7);
	wrt_1op_8byte(tbc, opcodes.call, 15);
	wrt_opcode(tbc, opcodes.halt);
	
	# load i
	wrt_pushq(tbc, 16);
	wrt_opcode(tbc, opcodes.pushbpadd);
	wrt_opcode(tbc, opcodes.loadspl);
	# load 1
	wrt_pushl(tbc, 1);
	# i <= 1 ?
	wrt_opcode(tbc, opcodes.uleql);
	wrt_1op_8byte(tbc, opcodes.jzl, 51);
	wrt_pushl(tbc, 1);
	wrt_1op_4byte(tbc, opcodes.retx, 4);
	
	wrt_pushq(tbc, 16);
	wrt_opcode(tbc, opcodes.pushbpadd);
	wrt_opcode(tbc, opcodes.loadspl);
	wrt_pushl(tbc, 1);
	# i-1
	wrt_opcode(tbc, opcodes.usubl);
	# factorial( i-1 );
	wrt_1op_8byte(tbc, opcodes.call, 15);
	
	# load i
	wrt_pushq(tbc, 16);
	wrt_opcode(tbc, opcodes.pushbpadd);
	wrt_opcode(tbc, opcodes.loadspl);
	# i * result of factorial( i-1 );
	wrt_opcode(tbc, opcodes.umull);
	wrt_1op_4byte(tbc, opcodes.retx, 4);

with open('test_native.tbc', 'wb+') as tbc:
	wrt_hdr(tbc, 64);
	wrt_hdr_natives(tbc, 'test');
	wrt_hdr_funcs(tbc);
	wrt_hdr_globals(tbc);
	wrt_hdr_footer(tbc, entry=0);
	
	wrt_pushl(tbc, 50);
	wrt_pushl(tbc, 100);
	wrt_pushl(tbc, 300.0);
	wrt_pushq(tbc, 12);
	wrt_opcode(tbc, opcodes.pushbpsub);
	wrt_callnat(tbc, 0, 4, 1);
	wrt_opcode(tbc, opcodes.halt);

with open('test_local_native_funcptr.tbc', 'wb+') as tbc:
	wrt_hdr(tbc, 64);
	wrt_hdr_natives(tbc, 'test');
	wrt_hdr_funcs(tbc);
	wrt_hdr_globals(tbc);
	wrt_hdr_footer(tbc, entry=0);
	
	wrt_pushl(tbc, 50);
	wrt_pushl(tbc, 100);
	wrt_pushl(tbc, 300.0);
	wrt_pushq(tbc, 12);
	wrt_opcode(tbc, opcodes.pushbpsub);
	wrt_1op_4byte(tbc, opcodes.pushnataddr, 0);
	wrt_callnats(tbc, 8, 1);
	wrt_opcode(tbc, opcodes.halt);

with open('test_multiple_natives.tbc', 'wb+') as tbc:
	wrt_hdr(tbc, 64);
	wrt_hdr_natives(tbc, 'test', 'printHW');
	wrt_hdr_funcs(tbc);
	wrt_hdr_globals(tbc);
	wrt_hdr_footer(tbc, entry=0);
	
	wrt_pushl(tbc, 50);
	wrt_pushl(tbc, 100);
	wrt_pushl(tbc, 300.0);
	wrt_pushq(tbc, 12);
	wrt_opcode(tbc, opcodes.pushbpsub);
	wrt_callnat(tbc, 1, 0, 0);
	wrt_callnat(tbc, 0, 8, 1);
	wrt_opcode(tbc, opcodes.halt);

with open('test_int2chr.tbc', 'wb+') as tbc:
	wrt_hdr(tbc, 64);
	wrt_hdr_natives(tbc);
	wrt_hdr_funcs(tbc, "main", 0, 10);
	wrt_hdr_globals(tbc);
	wrt_hdr_footer(tbc, entry=0);
	
	wrt_1op_8byte(tbc, opcodes.call, 10);
	wrt_opcode(tbc, opcodes.halt);
	
	wrt_pushl(tbc, 0x052A);
	wrt_pushq(tbc, 4);
	wrt_opcode(tbc, opcodes.pushbpsub);
	wrt_opcode(tbc, opcodes.loadspb);
	wrt_opcode(tbc, opcodes.ret);


with open('test_printf.tbc', 'wb+') as tbc:
	wrt_hdr(tbc, 64);
	wrt_hdr_natives(tbc, "printf");
	wrt_hdr_funcs(tbc, 'main', 0, 10);
	wrt_hdr_globals(tbc, 'str00001', 63-(len('\nnum==%i %f\n')+1), len('\nnum==%i %f\n')+1, '\nnum==%i %f\n');
	wrt_hdr_footer(tbc, entry=0);
	
	wrt_1op_8byte(tbc, opcodes.call, 10);
	wrt_opcode(tbc, opcodes.halt);
	
	wrt_pushq(tbc, 300.0);
	wrt_pushl(tbc, 280);
	wrt_1op_8byte(tbc, opcodes.pushaddr, 63-(len('\nnum==%i %f\n')+1));
	wrt_callnat(tbc, 0, 20, 3);
	wrt_opcode(tbc, opcodes.ret);

with open('test_fopen.tbc', 'wb+') as tbc:
	wrt_hdr(tbc, 64);
	wrt_hdr_natives(tbc, "fopen", "fclose");
	wrt_hdr_funcs(tbc, 'main', 0, 10);
	wrt_hdr_globals(tbc,
		'str00001', 63-(len('./endian_test1.tbc')+1), len('./endian_test1.tbc')+1, './endian_test1.tbc',
		'str00002', 44-(len('rb')+1), len('rb')+1, 'rb'
		);
	wrt_hdr_footer(tbc, entry=0);
	
	wrt_1op_8byte(tbc, opcodes.call, 10);
	wrt_opcode(tbc, opcodes.halt);
	
	wrt_1op_8byte(tbc, opcodes.pushaddr, 44-(len('rb')+1));
	wrt_1op_8byte(tbc, opcodes.pushaddr, 63-(len('./endian_test1.tbc')+1));
	wrt_callnat(tbc, 0, 16, 2);
	wrt_callnat(tbc, 1, 8, 1);
	wrt_opcode(tbc, opcodes.ret);

with open('test_malloc.tbc', 'wb+') as tbc:
	wrt_hdr(tbc, 64);
	wrt_hdr_natives(tbc, "malloc", "free");
	wrt_hdr_funcs(tbc, 'main', 0, 10);
	wrt_hdr_globals(tbc);
	wrt_hdr_footer(tbc, entry=0);
	
	wrt_1op_8byte(tbc, opcodes.call, 10);
	wrt_opcode(tbc, opcodes.halt);
	
	wrt_pushq(tbc, 4);
	wrt_callnat(tbc, 0, 8, 1);
	wrt_callnat(tbc, 1, 8, 1);
	wrt_opcode(tbc, opcodes.ret);



''' FIRST MAJOR PROGRAM THAT RESEMBLES ACTUAL C
int i;
int f(void) {
	return i;
}
float e;

int main(void)
{
	int l = 5;
	printf( "%i\n", f()+l );
	return 0;
}
'''
with open('test_globalvars.tbc', 'wb+') as tbc:
	wrt_hdr(tbc, 64);
	wrt_hdr_natives(tbc, 'printf');
	wrt_hdr_funcs(tbc, 'main', 0, 25, 'f', 0, 0);
	wrt_hdr_globals(tbc, 'i', 63-4, 4, 0, 'e', 63-8, 4, 0, 'str00001', 63-12, len('%i\n')+1, '%i\n');
	wrt_hdr_footer(tbc, entry=15);
	
	wrt_1op_8byte(tbc, opcodes.pushaddr, 63-4); #0-8	push 'i''s address
	wrt_opcode(tbc, opcodes.loadspl); #9 load 'i' by address to TOS.
	wrt_1op_4byte(tbc, opcodes.retx, 4); #10-14		return 'i'
	
	wrt_1op_8byte(tbc, opcodes.call, 25); #15-23	call main
	wrt_opcode(tbc, opcodes.halt); #24	exit main
	
	wrt_pushl(tbc, 5); #25-29	int l=5;
	wrt_1op_8byte(tbc, opcodes.call, 0); #30-38		call 'f'
	wrt_opcode(tbc, opcodes.addl); # f() + l
	wrt_1op_8byte(tbc, opcodes.pushaddr, 63-12);	# load string literal
	wrt_callnat(tbc, 0, 12, 2);	# call printf
	wrt_pushl(tbc, 0);
	wrt_1op_4byte(tbc, opcodes.retx, 4);	# return 0;



'''
void f() {
}

int main()
{
	void (*z)(void);
	callfunc(z);
	return 0;
}
'''
with open('test_funcptr_native.tbc', 'wb+') as tbc:
	wrt_hdr(tbc, 64);
	wrt_hdr_natives(tbc, 'callfunc');
	wrt_hdr_funcs(tbc, 'main', 0, 20, 'f', 1, 0);
	wrt_hdr_globals(tbc);
	wrt_hdr_footer(tbc, entry=10);
	
	wrt_pushl(tbc, 0); #0-4	push 'f''s func address
	wrt_1op_4byte(tbc, opcodes.retx, 4); #5-9		return 0
	
	wrt_1op_8byte(tbc, opcodes.call, 20); #10-18	call main
	wrt_opcode(tbc, opcodes.halt); #19	exit main
	
	wrt_pushq(tbc, 0); #20
	wrt_callnat(tbc, 0, 8, 1);
	wrt_pushl(tbc, 0);
	wrt_1op_4byte(tbc, opcodes.retx, 4);	# return 0;

'''
int i;
int f(void) {
	return i;
}
float e;

int main(void)
{
	int l = 5;
	printf( "%i\n", f()+l );
	return 0;
}
'''
with open('test_loadgbl.tbc', 'wb+') as tbc:
	wrt_hdr(tbc, 64);
	wrt_hdr_natives(tbc, 'getglobal');
	wrt_hdr_funcs(tbc, 'main', 0, 6);
	wrt_hdr_globals(tbc, 'i', 63-4, 4, 4294967196);
	wrt_hdr_footer(tbc, entry=0);
	
	wrt_1op_8byte(tbc, opcodes.call, 10); #0-8	call main
	wrt_opcode(tbc, opcodes.halt); #9	exit main
	
	#wrt_pushl(tbc, 16-4);	# load string literal
	wrt_callnat(tbc, 0, 0, 0);	# call 'getglobal'
	wrt_pushl(tbc, 0);
	wrt_1op_4byte(tbc, opcodes.retx, 4);	# return 0;
	
'''
void f() {
}

int main()
{
	void (*z)(void);
	callfunc(z);
	return 0;
}
'''
with open('test_call_func_by_name.tbc', 'wb+') as tbc:
	wrt_hdr(tbc, 64);
	wrt_hdr_natives(tbc, 'callfuncname');
	wrt_hdr_funcs(tbc, 'main', 0, 20, 'f', 1, 0);
	wrt_hdr_globals(tbc, 'str00001', 63-(len('f')+1), len('f')+1, 'f');
	wrt_hdr_footer(tbc, entry=10);
	
	wrt_pushl(tbc, 0); #0-4	push 'f''s func address
	wrt_1op_4byte(tbc, opcodes.retx, 4); #5-9		return 0
	
	wrt_1op_8byte(tbc, opcodes.call, 20); #10-18	call main
	wrt_opcode(tbc, opcodes.halt); #19	exit main
	
	wrt_1op_8byte(tbc, opcodes.pushaddr, 63-(len('f')+1));
	wrt_callnat(tbc, 0, 8, 1);
	wrt_pushl(tbc, 0);
	wrt_1op_4byte(tbc, opcodes.retx, 4);	# return 0;

'''
test GCC-style assembler generated code.
main:
        pushq   %rbp
        movq    %rsp, %rbp
        movl    $5, -4(%rbp)
        movb    $-1, -5(%rbp)
        movl    $0, %eax
        popq    %rbp
        ret
'''
with open('test_gcc_style_asm.tbc', 'wb+') as tbc:
	wrt_hdr(tbc, 64);
	wrt_hdr_natives(tbc);
	wrt_hdr_funcs(tbc, 'main', 0, 10);
	wrt_hdr_globals(tbc);
	wrt_hdr_footer(tbc, entry=0);
	
	wrt_1op_8byte(tbc, opcodes.call, 10); #0-8	call main
	wrt_opcode(tbc, opcodes.halt); #9	exit main
	
	wrt_pushq(tbc, 5);	# reserve stack space for local vars
	wrt_opcode(tbc, opcodes.pushspsub);
	wrt_opcode(tbc, opcodes.popsp);
	
	wrt_pushl(tbc, 5);
	wrt_pushq(tbc, 4);
	wrt_opcode(tbc, opcodes.pushbpsub);
	wrt_opcode(tbc, opcodes.storespl);
	
	wrt_push_smaller(tbc, 1, 255);
	wrt_pushq(tbc, 5);
	wrt_opcode(tbc, opcodes.pushbpsub);
	wrt_opcode(tbc, opcodes.storespb);
	
	wrt_pushl(tbc, 0);
	wrt_1op_4byte(tbc, opcodes.retx, 4);	# return 0;







