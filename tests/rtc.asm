loadlib RTC

data
	0
	0
	0
end

asm
	RTC_init
	RTC_start

	push 120.0
	RTC_setTempo

	push 0			# lowest prio
	dynFunNew @_nop newThread
	setmem 2
	push 99
	newThread @_th
	setmem 1
	jmp @_main

_nop:
	RTC_getDate push ":" RTC_getBeat push "\r" print 4
	getmem 0 SNZ jmp @_nop
	ret 0

_th:
	enter 2
_loop:
	_RTC_nextTask
	setmem -1	# function
	setmem -2	# date
	push "\nTâche " getmem -1 push " à la date " getmem -2 push "\n" print 5

	getmem -2 getmem -1 call

	getmem 0
	SNZ
	jmp @_loop
	push "\nth a terminé.\n" print 1
	ret 0

_stop:
	pop		# remove argc
	setmem 0	# store date
	push "\nSTOP ! Date=" getmem -1 push "\n" print 3
	push 1
	setmem 0
	ret 0

_main:
	push "                         on attend 2.5 secondes (5 battements)\r"
	print 1
	push 5.0
	RTC_wait
	push "\nterminé.\n"
	dynFunNew @_stop
	RTC_getBeat add 3.0
	RTC_sched
	push "\n                         on attend encore 2.5 secondes (5 battements)\r"
	print 1
	push 10.0
	RTC_wait
	
	push "\n"
	RTC_getDate
	push ":"
	RTC_getBeat
	push " : terminé.\n"
	print 5
	RTC_term
end
