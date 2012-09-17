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
#                                   2. Labels                                   #
#                                                                               #
#   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #
#################################################################################

# Here we will demonstrate the use of labels and jumps

# New syntax here :
# @label
#	reference a label in an opcode argument
# label:
#	define a label in an asm..end bloc

# New opcodes here :
# - jmp <label> : push a value onto the data stack.
# - SZ : "Skip (next instruction if value on top of stack, expected to be an integer, is) Zero".
#        If top of stack is 0, equivalent to a jump AFTER the next instruction. Equivalent to NOP otherwise.
# - SNZ : "Skip (next instruction if value on top of stack, expected to be an integer, is) Not Zero".
#        If top of stack is 0, equivalent to NOP. Equivalent to a jump AFTER the next instruction otherwise.
# - chr : convert int on top of stack into a character.
#

asm
start:				# label is simply defined by identifier followed by colon
	jmp @label1		# reads as "jump at label1"

	push "never executed.\n" print 1

label1:
	push "at label1 !\n" print 1
	push 1
	SZ			# this opcode means "skip next instruction if top of stack is 0" (in short, skip-zero). It pops the value from the stack.
	jmp @label2		# since we pushed 1 onto the stack, this opcode will not be skipped.
	jmp @label3		# so this one will never be executed.

label2:
	push "at label2 !\n" print 1
	push 1
	SNZ			# this opcode means "skip next instruction if top of stack is NOT 0" (in short, skip-non-zero). It also pops the value from the stack.
	jmp @label2		# so we are not going to label2 again, this is skipped.

	jmp@label3		# not pretty, but space is optional.

label3:
	push "at label3 !\n" print 1
	jmp @finish

finish:
	push "Finished !\n" print 1


_end_of_code_segment:		# there may not be any opcode after a label. This is legal.
end


# Example output :
#
# $ tinyaml 2.labels.asm
# at label1 !
# at label2 !
# at label3 !
# Finished !
