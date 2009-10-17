#!/usr/bin/env python

import os

sections = {
	'A':'arith_math_boolean_and_bin',
	'C':'compiler',
	'F':'flow_control_and_threading',
	'D':'data_handling'
}

def raw2dic(opcode_descr) :
	sec, oc = opcode_descr.strip().split(' ')
	return (oc, sec)

os.system(r'grep ^\\\w Core.txt > opcodes_list')

opcodes = dict(map(raw2dic, open('opcodes_list').readlines()))

print opcodes

sec_files = {
	'A':open('.opcode_sec_A', 'w'),
	'C':open('.opcode_sec_C', 'w'),
	'F':open('.opcode_sec_F', 'w'),
	'D':open('.opcode_sec_D', 'w')
}


for oc in opcodes.keys() :
	print >> sec_files[opcodes[oc]], '^%s %s'%(opcodes[oc], oc)

for sec in sections.keys() :
	fname = sec_files[sec].name
	sec_files[sec].close()
	os.system("grep -A 3 -f %s Core.txt | grep -v ^-- | sed 's/^[ACFD] //' > Sections/%s.txt"%(sec_files[sec].name, sections[sec]))

