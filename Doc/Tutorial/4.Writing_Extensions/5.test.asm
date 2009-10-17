# Load the extension library

require "symasm.wc"
loadlib Tutorial

glob ofs=-1 value=0 end

asm
	getEnvSymOfsAndValue &TutoTest
	-$value
	-$ofs

	push "TutoTest @"
	+$ofs
	push " = "
	+$value
	push '\n'
	print 5
end


# Example command and output :
# $ tinyaml 5.test.asm 
# Hello, opcode world !
# fibo(0) = 1
# fibo(1) = 1
# fibo(2) = 2
# fibo(3) = 3
# fibo(4) = 5
# fibo(5) = 8
# fibo(6) = 13
# fibo(7) = 21
# fibo(8) = 34
# fibo(9) = 55
# Hello, map item here
# TutoTest @16 = Hi, symofs !

