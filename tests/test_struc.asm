struc foobar {
	foo bar baz
}

data 0 0 0 end

asm
	strucNew foobar { bar:push 1 baz:push 10 foo:push 20 } setmem 1
	getmem 1 +foobar.foo
	push "\t"
	getmem 1 +foobar.bar
	push "\t"
	getmem 1 +foobar.baz
	push "\n"
	print 6
end

