# Here we demonstrate the functions and calls.

# There are two types of functions in tinyaml :
# - static functions, defined with a label.
# - dynamic functions, which are objects in memory (still defined from a label anyway).
#   These objects can enclose data for future use (this is of much use in the script layer which we'll see later on).

data
	0				# We'll use this slot to store a dynamic function
	0				# and a second one to demonstrate use of closures in dynamic functions
end


asm
	jmp @_skip			# skip function definitions


_my_hello:				# we'll use this label as a function with one argument
	push "Hello, "			# we want to print "Hello, <arg>\n"
	dup -1				# this opcode duplicates data at given offset in data stack at the top of stack. Offset must be negative.
	push "\n"
	print 3
	pop				# remove the argument from top of stack (print 3 has already removed everything else, remember ?)
	ret 0				# return from function

_my_hello2:				# we'll use this label as a function with one argument
	push "Hello, "			# we want to print "Hello, <arg>\n"
	swap 1				# this opcode exchanges data at top and given offset in data stack. Offset must be positive.
	push "\n"
	print 3
	ret 0				# return from function

_my_hello3:				# we'll use this label as a function with one argument
	push "Hello, "			# we want to print "Hello, <arg>\n"
	dup -1				# this opcode exchanges data at top and given offset in data stack.
	push "\n"
	print 3
	ret 1				# return from function with cleaning 1 value from stack, same effect as " pop ret 0 "


_my_hello_closure:
	getClosure 0
	swap 1
	push "\n"
	print 3
	ret 0

_my_generator:
	getClosure 0			# we have a counter
	inc				# we increment it
	setClosure 0			# store the result
	getClosure 0			# and return it
	ret 0				# when we demonstrate the use of exceptions and try/catch blocs, we can easily
					# define stop conditions for such a generator.

_skip:

	push "world !"			# push argument
	call @_my_hello			# call function

	push "foobar !"			# push argument
	call @_my_hello2		# call function

	push 42				# push argument
	call @_my_hello3		# call function

	# now we define a dynamic function
	dynFunNew @_my_hello
	setmem 0

	push "We have a "
	getmem 0
	push " at offset 0 in data segment\n"
	print 3

	push "DynFun !"
	getmem 0			# now we call the dynamic function
	call				# call without label argument expects a function object on top of data stack

	dynFunNew @_my_hello_closure
	push "Wibble, "			# this will be our "hello"
	dynFunAddClosure
	setmem 1
	
	push "Closure !"
	getmem 1
	call

	dynFunNew @_my_generator
	push 0 dynFunAddClosure
	setmem 0			# we can ditch the first function, we're not going to use it again.
					# NOTE : the function will be garbage-collected automatically since
					# it's not referenced anywhere else now.


# Now we build a while loop that will count to 10 using the generator.
# the symbolic asm layer will make writing such loops much easier.
loop :
	getmem 0 call			# get next value
	dup 0				# duplicate result so we can test it before using it
	push 10				# we want to stop after 10
	sup				# a binary comparison operator : pushes 1 if left operand is greater than right operand, otherwise pushes 0
	SZ				# if it's inf or eq, skip next instruction
	jmp @loop_end
	push "generator says "
	swap 1
	push '\n'
	print 3
	jmp @loop

loop_end :

end


# Example output :
#
# $ tinyaml 4.functions.asm
# Hello, world !
# Hello, foobar !
# Hello, 42
# We have a [Function  0x83a7a68] at offset 0 in data segment
# Hello, DynFun !
# Wibble, Closure !
# generator says 1
# generator says 2
# generator says 3
# generator says 4
# generator says 5
# generator says 6
# generator says 7
# generator says 8
# generator says 9
# generator says 10
