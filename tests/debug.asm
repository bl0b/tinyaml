#require "tests/struc.melang"
#require "melang_struc.wc"
require "tests/struc.routines"

#data 0 0 "\\t" "\\n" end
glob
	wibble = 0
	wobble = 0
	tab = "\\t"
	ret = "\\n"
	count = 0
end

asm
_start:
	+$count [[
		push "Compteur à 1.\n" print 1
		ret 0
	][
		+$count inc -$count
	]]

	+$count [
		push "maintenant, le compteur est à 1.\n" print 1
	]
	
#	push "0123456789"
#	push 2			# start offset, included
#	push 5			# end offset, excluded
#	substr toS		# toS converts from object to string, without check (FIXME)
#	push "\\n"
#	print 2
    nop
        strucNew foobar {
                foo:  asm dynFunNew @_dump_struc_cls $wibble dynFunAddClosure end
                bar:  asm dynFunNew @_dump_struc_cls $wobble dynFunAddClosure end
                baz:push 3.3
        } -$wibble
        strucNew foobar {
                foo:  push 7
                bar:  push "\tbla\t"
                baz:push 3.3
        } -$wobble

    enter 1
        +$wibble setmem -1
        envGet &dump_struc
        call
    leave 1

    +$wibble+(foobar.foo) call
    +$wibble+(foobar.bar) call
    +$wobble
        +$wobble+(foobar.foo) +$wobble+(foobar.baz) mul
    -(foobar.bar)
    +$wibble+(foobar.bar) call
    ret 0

_dump_struc_cls:
        getClosure 0
        push " : "
    print 2

    enter 1
            getClosure 0 getmem
        setmem -1
            envGet &dump_struc
        call
    leave 1

    ret 0

end



