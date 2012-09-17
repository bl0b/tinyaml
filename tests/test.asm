data
	0 rep 200		# 1
	"Hello, world !"	# 201
	"\n"			# 202
end

asm
	push 1
	push "\t"
	arrayNew
	dup -1
	dup -1
	push "\n"
	print 6
	ret 0


	enter 2			# counters
	push 500 setmem -2
:mega_loop
	push 2 setmem -1
	push 1
	setmem 1
	push 1
	setmem 2
	# fill fibonacci table
:fill_loop
	getmem -1 dec getmem
	getmem -1 sub 2 getmem
	add getmem -1 setmem
	getmem -1 inc setmem -1
	getmem -1 sub 200 SZ jmp @fill_loop

	# dump table

	push 1 setmem -1

	# redo filling many times
	getmem -2 dec setmem -2
	getmem -2 SZ jmp @mega_loop

:dump_loop
	yield
	push "Fibo("
	getmem -1
	push ") = "
	getmem -1 getmem
	push "\n"
        print 5
	getmem -1 inc setmem -1
	getmem -1 sub 200 SZ jmp @dump_loop

	leave 2
	jmp @_end


        push 5			# push the counter

:loop
        push "Hello, world.\n"	# do something
        print 1
        dec			# decrement counter
        dup 0			# 
        SZ			# compare to 0 (pop stack top)
        jmp @loop		# loop
        push "Goodbye.\n"	# counter is 0
        print 1
        pop
#
# test fibonacci
	push 1
:fibo_loop
	push "Fibo("
	dup -1
	push ") = "
        dup -1
        call @fibo
	push "\n"
        print 5
	inc
	dup 0
	sub 37
	SZ jmp @fibo_loop

        jmp @_end
#

:fibo				# stack : n,
        # compare to 1
        dup 0 dec		# stack : n, n-1
	SNZ jmp @fibo_ret_1	# stack : n
        # compare to 2
        dup 0 dec dec		# stack : n, n-2
	SNZ jmp @fibo_ret_1	# stack : n
        # prepare to sum
	enter 2
        dec
	dup 0			# stack : n-1, n-1
	call @fibo		# stack : n-1, fibo(n-1)
	setmem -1		# stack : n-1
        dup 0 dec		# stack : n-1, n-2
	call @fibo		# stack : n-1, fibo(n-2),
	setmem -2		# stack : n-1,

	#...

	getmem -2		# stack : n-1, fibo(n-2),
	getmem -1		# stack : n-1, fibo(n-2), fibo(n-1),
        add			# stack : n-1, fibo(n-1)+fibo(n-2),

	leave 2
        retval 1			# stack : fibo(n-1)+fibo(n-2),
:fibo_ret_1
	push 1			# stack : n, 1
	retval 1			# stack : 1

#
:_end
    nop
end

