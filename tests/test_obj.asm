require "symasm.wc"

glob
	o = 0
	c = 0
end

asm
	jmp@_start

_fnu :
	push " args\n" print 2  # discard argc
	push 23
	ret 0

_start:
	_vcls_new -$c
	_vobj_new -$o
	+$o +$c _vobj_scls
	+$o push " " +$o _vobj_gcls push '\n' print 4
	+$c push "inc" envGet &OpcodeNoArg dynFunNew@_fnu _vcls_soo
	push "fnu => " +$o inc push '\n' print 3
end

