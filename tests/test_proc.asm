require "procasm.wc"

func pouet(a, b)
	+$a push " " +$b push "\n" print 4
	ret 0
endfunc

func var(n)
	local arg, counter {
		push "n @" $n push '\n' print 3
		push "arg @" $arg push '\n' print 3
		push "counter @" $counter push '\n' print 3
		local test_inner {
			push "Inner frame :\n" print 1
			push "n @" $n push '\n' print 3
			push "arg @" $arg push '\n' print 3
			push "counter @" $counter push '\n' print 3
			push "test_inner @" $test_inner push '\n' print 3
		}
		push 0 -$counter
	_v_lp:	+$counter inc +$n inf [
			-$arg
			push "arg #" +$counter push " : " +$arg push "\n" print 5
			+$counter inc -$counter
			jmp @_v_lp
		]
	}
	# returns last argument
endfunc

func hop()
	push "hello"
	push "world"
endfunc

asm
	local a, b {
		push "hello" -$a
	}
	local x, y {
		local z, t {
			push "hello" -$t
			push "hello" -$x
			%pouet(+$x, %var(push 3, push "A", push "B", push "world"))
			%pouet(%hop())
		}
	}
end

