loadlib MessageQueue

data 0 0 0 0 end

asm
	msgQueueNew setmem 0
	push 80 newThread @_th setmem 1

	#call @_nops
	yield
	getmem 0 push "   Hello, world." msgQueueWrite
	#call @_nops
	yield
	getmem 0 push 23 msgQueueWrite
	#call @_nops
	yield
	getmem 0 arrayNew msgQueueWrite
	#call @_nops
	yield

	#getmem 1 killThread
	jmp @_end

_nops: enter 1
	push 1000 setmem -1
_nops_lp:
	getmem -1 dec setmem -1
	getmem -1 SZ jmp @_nops_lp
	leave 1
	ret 0
	
_th:	getmem 0 msgQueueReaderNew setmem 2
_lp:	getmem 2 msgQueueRead setmem 3
	push "A lu " getmem 3 push "\n" print 3
	jmp @_lp

_end:
	push 0 setmem 0
	push 0 setmem 1
	push 0 setmem 2
	push 0 setmem 3
	ret 0
end

