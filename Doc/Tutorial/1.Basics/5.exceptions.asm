# Here we demonstrate the use of exceptions and try/catch blocs.

# New opcodes are :
# - instCatcher <label> : push the catch bloc at label onto the catch blocs stack. Any exception throw will redirect to most recently pushed catch bloc.
# - uninstCatcher <label> : pops the last pushed catch bloc and jump to label.
# - throw : pops value from top of data stack and throws it as an exception. Data can be of any type.
# - getException : push exception value onto data stack.
#
# When an exception is received by a catch bloc, this bloc is popped from catch blocs stack, so it is not used twice.


asm

_try:
	instCatcher @_catch1
	instCatcher @_catch2
	push "Before throw\n" print 1
	push 23
	throw
	push "After throw\n" print 1			# this code will never be executed
	uninstCatcher @_end				# if there were no exception thrown before, we would skip directly to the _end label

_catch1:
	push "Catch 1\nException #" getException push "\n" print 3
	jmp @_end

_catch2:
	push "Catch 2\nException #" getException push "\n" print 3
	# propagate to enclosing catch
	getException
	inc						# This time we'll throw the exception <24>
	throw


_end:
	push "Done.\n" print 1				# this code will never be executed
end


# Example output :
#
# $ tinyaml 5.exceptions.asm
# Before throw
# Catch 2
# Exception #23
# Catch 1
# Exception #24
# Done.
