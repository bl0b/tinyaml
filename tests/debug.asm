#require "tests/struc.melang"
#require "melang_struc.wc"
require "tests/struc.routines"

#data 0 0 "\\t" "\\n" end
glob
	wibble = 0
	wobble = 0
	tab = "\\t"
	ret = "\\n"
end

asm
_start:
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



