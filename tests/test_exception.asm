asm
_try:
	instCatcher @_catch
	# duplicate the catch block to handle exception twice
	instCatcher @_catch
	push "Avant throw\n" print 1
	push 23
	throw
	push "Après throw\n" print 1
	uninstCatcher @_end
_catch:
	push "Exception #" dup -1 push "\n" print 3
	# propagate to enclosing catch
	throw
_end:
	push "Fini.\n" print 1
end
