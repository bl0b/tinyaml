# This code will be "inlined" in place of the include statement in the main file.

# This means the labels we define here are accessible from the main file.

# New opcodes here :
# ret <int>
#	pop that many values from data stack and return from function call.
#

asm
	jmp @_skip_inc
_hello_label:
	push "Hello, include world !\n"
	print 1
	ret 0

_skip_inc:
end

