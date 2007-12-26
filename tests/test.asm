asm
        push 5			# push the counter

loop:
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
fibo_loop:
	push "Fibo("
	dup -1
	push ") = "
        dup -1
        call @fibo
	push "\n"
        print 5
	inc
	dup 0
	sub 30
	SZ jmp @fibo_loop

        jmp @_end
#

fibo:				# stack : n,
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
	setLocal 0		# stack : n-1
        dup 0 dec		# stack : n-1, n-2
	call @fibo		# stack : n-1, fibo(n-2),
	setLocal 1		# stack : n-1,

	#...

	getLocal 1		# stack : n-1, fibo(n-2),
	getLocal 0		# stack : n-1, fibo(n-1),
        add			# stack : n-1, fibo(n-1)+fibo(n-2),

	leave 2
        retval 1			# stack : fibo(n-1)+fibo(n-2),
fibo_ret_1:
	push 1			# stack : n, 1
	retval 1			# stack : 1

#
_end:nop
end

