# Here we will demonstrate the use of labels and jumps

asm
start:				# label is simply defined by identifier followed by colon
	jmp @label1		# reads as "jump at label1"

	push "never executed.\n" print 1

label1:
	push "at label1 !\n" print 1
	push 1
	SZ			# this opcode means "skip next instruction if top of stack is 0" (in short, skip-zero). It pops the value from the stack.
	jmp @label2		# since we pushed 1 onto the stack, this opcode will not be skipped.
	jmp @label3		# so this one will never be executed.

label2:
	push "at label2 !\n" print 1
	push 1
	SNZ			# this opcode means "skip next instruction if top of stack is NOT 0" (in short, skip-non-zero). It also pops the value from the stack.
	jmp @label2		# so we are not going to label2 again, this is skipped.

	jmp@label3		# not pretty, but space is optional.

label3:
	push "at label3 !\n" print 1
	jmp @finish

finish:
	push "Finished !\n" print 1


_end_of_code_segment:		# there may not be any opcode after a label. This is legal.
end

