# This is a comment.
# Comments start with a hash sign and go to the end of the line.

asm						# this keyword starts an assembly code bloc.
	push "Hello, world.\n"			# push litteral value onto stack. Could be an integer, a float, a single character, or a string.
	print 1					# print exactly one value.
end						# this keyword is pretty self-explanatory.

# Indenting and newlines are totally optionals.
asm push "Hello, one-line world.\n" print 1 end	# exactly the same effect as above.

# print N can be used to print complex messages
asm
	push 'H'				# push single character
	push "ello,"				# push string
	push 32					# push integer
	chr					# but we want it to be a character (a space)
	push "world ! "				# push string
	push 42					# integer...
	push ' '				# a space character
	push 2.3				# float !
	push '\n'				# and the final newline character
	print 8					# print everything (8 values)
end

