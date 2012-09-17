# TinyaML
# Copyright (C) 2007 Damien Leroux
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#################################################################################
#   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #
#                                                                               #
#                                TINYAML TUTORIAL                               #
#                     Part IV : Writing extension libraries                     #
#                                 4. A test file                                #
#                                                                               #
#   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #
#################################################################################

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

