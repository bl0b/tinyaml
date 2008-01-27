data
	0
	0
	0
end

asm
	RTC_init
	RTC_start

#	push 0			# lowest prio
#	newThread @_nop
#	setmem 2
	push 99
	newThread @_th
	setmem 1
	jmp @_main

_nop:
	nop jmp @_nop

_th:
	enter 2
_loop:
	_RTC_nextTask
	push "task !\n" print 1
	setmem -1	# function
	setmem -2	# date
	push "Tâche " getmem -1 push "\ à la date " getmem -2 push "\n" print 5

	getmem -2 getmem -1 call

	getmem 0
	SNZ
	jmp @_loop
	push "th a terminé.\n" print 1
	ret 0

_stop:
	push 1
	setmem 0
	ret 0

_main:
	push "\ton attend 10 secondes\n"
	print 1
	push 10.0
	RTC_wait
	push "\tterminé.\n"
	dynFunNew @_stop
	RTC_getDate add 5.0
	RTC_sched
	push "\ton attend 7 secondes\n"
	print 1
	push 7.0
	RTC_wait
	push "\tterminé.\n"
	RTC_term
end
