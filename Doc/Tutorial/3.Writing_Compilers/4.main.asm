# Not much in this file, we just aggregate everything in order.
include "2.language.grammar"
include "3.compiler.asm"

loadlib IO
require "symasm.wc"

glob
	expr = ""
end

asm
_read_lp:
	push "> " print 1					# print the prompt
	stdin _funpack 'S' -$expr				# read one line
	+$expr push "quit\n" strcmp [				# if user didn't enter "quit", then
		+$expr compileStringToThread 50 joinThread	# compile expr and wait til execution ended
		jmp @_read_lp					# and do it again
	]
end

