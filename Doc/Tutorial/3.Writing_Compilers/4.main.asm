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
#                          Part III : Writing Compilers                         #
#                             4. A main loop for fun                            #
#                                                                               #
#   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #
#################################################################################

# Not much in this file, we just aggregate everything in order.
include "2.language.grammar"
include "3.compiler.asm"

loadlib IO
require "symasm.wc"

glob
	expr = ""
end

asm
_read_lp:
	push "> " print 1					# print the prompt
	stdin _funpack 'S' -$expr				# read one line
	+$expr push "quit\n" strcmp [				# if user didn't enter "quit", then
		+$expr compileStringToThread 50 joinThread	# compile expr and wait til execution ended
		jmp @_read_lp					# and do it again
	]
end

