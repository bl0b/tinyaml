require "procasm.wc"
loadlib IO

func _cli_cb(port, ip, file)
	+$file push '\t' +$ip ip2string push '\t' +$port push '\n' print 6
	push "Hello, world.\r\n" +$file _fpack 'S'
	+$file close
endfunc

asm	
	%_TCPServer(push 0, push 43210, +$_cli_cb, push 10)

	push 10000000
_lp:
	dec
	dup 0 push 0 sup SZ jmp@_lp
	pop
	push "Now closing server " dup -1 push "\n" print 3
	close
end
