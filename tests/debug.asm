#require "tests/struc.melang"
#require "melang_struc.wc"
require "tests/struc.routines"

data 0 0 "\\t" "\\n" end

asm
_start:
    nop
        strucNew foobar {
                foo:  asm dynFunNew @_dump_struc_cls push 0 dynFunAddClosure end
                bar:  asm dynFunNew @_dump_struc_cls push 1 dynFunAddClosure end
                baz:push 3.3
        } setmem 0
        strucNew foobar {
                foo:  push 7
                bar:  push "\tbla\t"
                baz:push 3.3
        } setmem 1

    enter 1
        getmem 0 setmem -1
        envGet &dump_struc
        call
    leave 1

    getmem 0+foobar.bar call
        getmem 1
            getmem 1 +foobar.foo getmem 1 +foobar.baz mul
        -foobar.bar
    getmem 0+foobar.bar call
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



