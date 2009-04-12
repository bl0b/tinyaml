struc foobar {
        foo bar baz
}

data 0 0 0 end

asm
        strucNew foobar {
		bar: asm push 1 end
		baz: push 10
		foo:push 20
	} setmem 1
#        getmem 1 +foobar.foo
#        push "\t"
#        getmem 1 +foobar.bar
#        push "\t"
#        getmem 1 +foobar.baz
#        push "\n"
#        push 1
#        push 1
#        push 1
#        print 6
end

#asm
#	nop
#end

asm
	push 1 pop
	nop
end


