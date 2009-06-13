# Here we define and use global and local data


data					# this keyword starts a global data definition bloc.
					# all data defined here goes straigth in a data segment, starting at offset 0.
					# there is ONE data segment per program, so all data...end blocs will append to this segment.
	42				# an integer at offset 0
	"Hello, data !"			# a string at offset 1
	123.45				# a float at offset 2
	'\n'				# a character at offset 3
end


asm
	getmem 1			# getmem pushes data at given offset. Offsets >=0 mean "take data from this offset in data segment".
	getmem 3
	print 2

	getmem 0			# here we demonstrate simple data modification.
	push 23
	add				# every binary operator works the same : push left operand, push right operand, operator pops both and pushed result.
	setmem 0			# setmem pops data from top of stack and puts it at the given offset. Offsets work the same as with getmem.
					# exactly as with getmem.
	push "42+23="
	getmem 0
	push '\n'
	print 3				# display the result
end



# Now for local data. It's mainly useful when defining functions, but still can be used anywhere. And we haven't covered function calls yet.

asm
	enter 2				# reserve 2 data slots in local data stack.
	getmem 2
	setmem -1			# Offsets < 0 mean "take/put data from/into local data stack at offset (1-given_offset)".
					# i.e. we put 123.45 in the first of the two reserved local data slots.
	getmem -1
	push 100
	mul				# add mul sub div all handle mixes of int/float nicely. The result will be int if both operands
					# are ints, or float otherwise.
	setmem -2			# now we have 12345. stored in the second local variable.

	getmem -1
	push "*100="
	getmem -2
	getmem 3
	print 4				# display.

	leave 2				# free the 2 data slots.
end
