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
#                                Part I : Basics                                #
#                                1. Hello, world.                               #
#                                                                               #
#   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #
#################################################################################


# This is a comment.
# Comments start with a hash sign and go to the end of the line.

# New syntax here :
# asm [...] end
#	declare an asm bloc. Each instruction can have an immediate argument
#	like the following :
#	- Int : any integer value
#	- Float : any floating-point representation, like 1.0, -153.2e10, etc...
#	- String : a double quote-delimited string, with standard C escapes
#	- Label : a "@" followed by a label name
#	- Environment variable : an "&" followed by a label name
#	- Single character : a single quote-delimited single character
#	There is no instruction separator.

# New opcodes here :
# - push <int|float|string> : push a value onto the data stack.
# - print <int> : fetch and display that many values from the data stack.
# - chr : convert int on top of stack into a character.
#


asm						# this keyword starts an assembly code bloc.
	push "Hello, world.\n"			# push litteral value onto stack. Could be an integer, a float, a single character, or a string.
	print 1					# print exactly one value.
end						# this keyword is pretty self-explanatory.

# Indenting and newlines are totally optionals.
asm push "Hello, one-line world.\n" print 1 end	# exactly the same effect as above.

# print N can be used to print complex messages
asm
	push 'H'				# push single character
	push "ello,"				# push string
	push 32					# push integer
	chr					# but we want it to be a character (a space)
	push "world ! "				# push string
	push 42					# integer...
	push ' '				# a space character
	push 2.3				# float !
	push '\n'				# and the final newline character
	print 8					# print everything (8 values)
end

# Example command and output :
#
# $ tinyaml 1.hello_world.asm
# Hello, world.
# Hello, one-line world.
# Hello, world ! 42 2.300000

