loadlib IO
require "symasm.wc"

glob
	count=0
	i=0
	f=0
	k=305419896
	pi_URL="http://www.exploratorium.edu/pi/Pi10-6.html"
	pi_host="www.exploratorium.edu"
	genese_URL="http://www.info-bible.org/lsg/01.Genese.html"
	genese_host="www.info-bible.org"
	tno_query="GET / HTTP/1.1\r\nHost: tirnan0g.org\r\n\r\n"
end

asm
	jmp @_skip
thread_1:
	+$i stderr fprint push '\t' stderr fprint
	+$i stderr fprint push '\t' stderr fprint
	+$i stderr fprint push '\t' stderr fprint
	+$i stderr fprint push '\t' stderr fprint
	+$i stderr fprint push '\t' stderr fprint
	+$i stderr fprint push '\t' stderr fprint
	+$i stderr fprint push '\t' stderr fprint
	+$i stderr fprint push '\n' stderr fprint
	+$i push 100 eq SNZ jmp@thread_1
	ret 0

thread_2:
	push 0 -$count
_lp_th2:
	+$count inc -$count
	#push 0 _lp2th2: inc dup 0 push 100 eq SNZ jmp@_lp2th2 pop
	+$count push 100 eq SNZ jmp@_lp_th2
	ret 0

test_write_to_file:
	push "toto.txt" push "w" fopen -$f
	push "Hello, world.\n" +$f fprint
	+$k +$f _fpack 'I'
	push '\n' +$f _fpack 'C'
	+$k +$f _fpack 'i'
	+$k toF +$f _fpack 'f'
	+$k toF +$f _fpack 'F'
	+$f close
	ret 0

test_read_from_file:
	push "toto.txt" fopen "r" -$f
	+$f _funpack 'S' push "\n" print 2
	+$f close
	ret 0

_skip:
	push ". is a " push "." ftype push '\n' print 3

	push "fprint to toto.txt...\n" print 1
	call @test_write_to_file

	push "fread from toto.txt...\n" print 1
	call @test_read_from_file

	nop

	push "www.tirnan0g.org" -$i
	+$i push " : " +$i string2ip -$i +$i ip2string push '\n' print 4
	+$i push 80 tcpopen -$f
	+$tno_query +$f _fpack 'S'
	+$f _funpack 's' print 1
	+$f close

	jmp@_skip_long_read_tests

	nop
	push "\n\nNow reading a looooooong string from URL " +$pi_URL push "\n(please be patient depending on the bandwidth)\n" print 3

	+$pi_host string2ip push 80 tcpopen -$f
	push "GET " +$pi_URL  push "\r\n" strcat strcat +$f _fpack 'S'
	+$f _funpack 's' -$i
	push "Have read " +$i strlen push " bytes.\n" print 3
	+$f close

	push "pi_decimals.txt" push "w" fopen -$f
	+$i +$f _fpack 'S'
	+$f close

	nop
	push "\n\nNow reading a looooooong string from URL " +$genese_URL push "\n(please be patient depending on the bandwidth)\n" print 3

	+$genese_host string2ip push 80 tcpopen -$f
	push "GET " +$genese_URL  push "\r\n" strcat strcat +$f _fpack 'S'
	+$f _funpack 's' -$i
	push "Have read " +$i strlen push " bytes.\n" print 3
	+$f close

	push "genese.txt" push "w" fopen -$f
	+$i +$f _fpack 'S'
	+$f close

	push "Hello, err world.\n" stderr fprint
	push "TEST err\n" stderr fprint
	push "Hello, out world.\n" stdout fprint
	push "TEST out\n" stdout fprint
	#ret 0
	push 0 -$i
	push 10 newThread @thread_2
	push 10 newThread @thread_1

_skip_long_read_tests:

	nop

	# test popen

	push "grep lo" popen "w" -$f
	push "Hello, world." +$f _fpack 'S'
	push '\n' +$f fprint
	push "filtered out\n" +$f _fpack 'S'
	push "filtered out\n" +$f _fpack 'S'
	push "Plop !\n" +$f _fpack 'S'
	+$f flush
	+$f close
	push 0 -$f


	push "." opendir -$f
	+$f readdir -$i
_lp_readdir:
	+$i strlen [
		push "dir entry : " +$i push "\n" print 3
		#push "--\\n" print 1
		+$f readdir -$i
		jmp@_lp_readdir
	]

	+$f close
	push 0 -$f

	%__IO__term()
end

