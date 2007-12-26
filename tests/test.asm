asm
#emulate a loop
        push 5			# push the counter

loop:
        push "Hello, world.\n"	# do something
        print 1
        dec			# decrement counter
        dup 0			# 
        SF			# compare to 0 (pop stack top)
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
	SF jmp @fibo_loop

        jmp @_end
#

fibo:				# stack : n,
        # compare to 1
        dup 0 dec		# stack : n, n-1
	ST jmp @fibo_ret_1	# stack : n
        # compare to 2
        dup 0 dec dec		# stack : n, n-2
	ST jmp @fibo_ret_1	# stack : n
        # prepare to sum
        dec dup 0		# stack : n-1, n-1
	call @fibo		# stack : n-1, fibo(n-1)
        dup -1 dec		# stack : n-1, fibo(n-1), n-2
	call @fibo		# stack : n-1, fibo(n-1), fibo(n-2),
        add			# stack : n-1, fibo(n-1)+fibo(n-2),

        ret 1			# stack : fibo(n-1)+fibo(n-2),
fibo_ret_1:
	push 1			# stack : n, 1
	ret 1			# stack : 1

#
_end:nop
end

