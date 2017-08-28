#!/usr/bin/python3


def enum(*sequential, **named):
	enums = dict(zip(sequential, range(len(sequential))), **named);
	return type('Enum', (), enums);

opcodes = enum('halt', 'pushq', 'pushl', 'pushs', 'pushb', 'pushsp', 'puship', 'pushbp', 'pushspadd', 'pushspsub', 'pushbpadd', 'pushbpsub', 'popq', 'popl', 'pops', 'popb', 'popsp', 'popip', 'popbp', 'wrtq', 'wrtl', 'wrts', 'wrtb', 'storeq', 'storel', 'stores', 'storeb', 'storeqa', 'storela', 'storesa', 'storeba', 'storespq', 'storespl', 'storesps', 'storespb', 'loadq', 'loadl', 'loads', 'loadb', 'loadqa', 'loadla', 'loadsa', 'loadba', 'loadspq', 'loadspl', 'loadsps', 'loadspb', 'copyq', 'copyl', 'copys', 'copyb', 'addq', 'uaddq', 'addl', 'uaddl', 'addf', 'subq', 'usubq', 'subl', 'usubl', 'subf', 'mulq', 'umulq', 'mull', 'umull', 'mulf', 'divq', 'udivq', 'divl', 'udivl', 'divf', 'modq', 'umodq', 'modl', 'umodl', 'addf64', 'subf64', 'mulf64', 'divf64', 'andl', 'orl', 'xorl', 'notl', 'shll', 'shrl', 'andq', 'orq', 'xorq', 'notq', 'shlq', 'shrq', 'incq', 'incl', 'incf', 'decq', 'decl', 'decf', 'negq', 'negl', 'negf', 'incf64', 'decf64', 'negf64', 'ltq', 'ltl', 'ultq', 'ultl', 'ltf', 'gtq', 'gtl', 'ugtq', 'ugtl', 'gtf', 'cmpq', 'cmpl', 'ucmpq', 'ucmpl', 'compf', 'leqq', 'uleqq', 'leql', 'uleql', 'leqf', 'geqq', 'ugeqq', 'geql', 'ugeql', 'geqf', 'ltf64', 'gtf64', 'cmpf64', 'leqf64', 'geqf64', 'neqq', 'uneqq', 'neql', 'uneql', 'neqf', 'neqf64', 'jmp', 'jzq', 'jnzq', 'jzl', 'jnzl', 'call', 'calls', 'calla', 'ret', 'retx', 'reset', 'wrtnataddr', 'pushnataddr', 'callnat', 'callnats', 'callnata', 'mmxaddl', 'mmxuaddl', 'mmxaddf', 'mmxadds', 'mmxuadds', 'mmxaddb', 'mmxuaddb', 'mmxsubl', 'mmxusubl', 'mmxsubf', 'mmxsubs', 'mmxusubs', 'mmxsubb', 'mmxusubb', 'mmxmull', 'mmxumull', 'mmxmulf', 'mmxmuls', 'mmxumuls', 'mmxmulb', 'mmxumulb', 'mmxdivl', 'mmxudivl', 'mmxdivf', 'mmxdivs', 'mmxudivs', 'mmxdivb', 'mmxudivb', 'mmxmodl', 'mmxumodl', 'mmxmods', 'mmxumods', 'mmxmodb', 'mmxumodb', 'mmxandl', 'mmxands', 'mmxandb', 'mmxorl', 'mmxors', 'mmxorb', 'mmxxorl', 'mmxxors', 'mmxxorb', 'mmxnotl', 'mmxnots', 'mmxnotb', 'mmxshll', 'mmxshls', 'mmxshlb', 'mmxshrl', 'mmxshrs', 'mmxshrb', 'mmxincl', 'mmxincf', 'mmxincs', 'mmxincb', 'mmxdecl', 'mmxdecf', 'mmxdecs', 'mmxdecb', 'mmxnegl', 'mmxnegf', 'mmxnegs', 'mmxnegb', 'nop');


jmp_table = {
	"jmp": opcodes.jmp,
	"jzq": opcodes.jzq,
	"jnzq": opcodes.jnzq,
	"jzl": opcodes.jzl,
	"jnzl": opcodes.jnzl
};


def wrt_hdr(f, memsize:int, stksize:int):
	f.write(0xC0DE.to_bytes(2, byteorder='little'));
	f.write(memsize.to_bytes(4, byteorder='little'));
	f.write(stksize.to_bytes(4, byteorder='little'));

def wrt_hdr_natives(f, *natives):
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

def wrt_opcode(f, opcode:int):
	f.write(opcode.to_bytes(1, byteorder='big'));


with open('test_native.tbc', 'wb+') as tbc:
	wrt_hdr(tbc, 0, 16);
	wrt_hdr_natives(tbc, 'test');
	tbc.write(0x0.to_bytes(4, byteorder='little'));
	wrt_opcode(tbc, opcodes.pushl);
	tbc.write(0x32.to_bytes(4, byteorder='big'));
	wrt_opcode(tbc, opcodes.pushl);
	tbc.write(0x64.to_bytes(4, byteorder='big'));
	wrt_opcode(tbc, opcodes.pushl);
	tbc.write(0x43960000.to_bytes(4, byteorder='big'));
	wrt_opcode(tbc, opcodes.callnat);
	tbc.write(0x0.to_bytes(4, byteorder='big'));
	tbc.write(0xc.to_bytes(4, byteorder='big'));
	tbc.write(0x1.to_bytes(4, byteorder='big'));
	wrt_opcode(tbc, opcodes.halt);


with open('test_multiple_natives.tbc', 'wb+') as tbc:
	wrt_hdr(tbc, 0, 16);
	wrt_hdr_natives(tbc, 'test', 'printHW');
	tbc.write(0x0.to_bytes(4, byteorder='little'));
	wrt_opcode(tbc, opcodes.pushl);
	tbc.write(0x32.to_bytes(4, byteorder='big'));
	wrt_opcode(tbc, opcodes.pushl);
	tbc.write(0x64.to_bytes(4, byteorder='big'));
	wrt_opcode(tbc, opcodes.pushl);
	tbc.write(0x43960000.to_bytes(4, byteorder='big'));
	
	wrt_opcode(tbc, opcodes.callnat);
	tbc.write(0x1.to_bytes(4, byteorder='big'));
	tbc.write(0x0.to_bytes(4, byteorder='big'));
	tbc.write(0x0.to_bytes(4, byteorder='big'));
	
	wrt_opcode(tbc, opcodes.callnat);
	tbc.write(0x0.to_bytes(4, byteorder='big'));
	tbc.write(0xc.to_bytes(4, byteorder='big'));
	tbc.write(0x1.to_bytes(4, byteorder='big'));
	wrt_opcode(tbc, opcodes.halt);














