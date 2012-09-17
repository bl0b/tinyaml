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
#                                2. Compiler code                               #
#                                                                               #
#   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #
#################################################################################

require "metasm.wc"

# The basic syntax for the compiler part is :
# compile <node_name> <code_bloc>
# Where node_name is the name of the rule that produced the node,
# and <code_bloc> any code bloc. The only thing is that the code bloc MUST
# set the compiler state at some point (typically at the end of the bloc),
# the state being one of :
# - Down : "enter" this node, and process (visit) each child in sequence.
# - Next : compilation went OK, go to next sibling node OR to the next sibling
#          of the parent node if no next sibling OR terminate compilation
#          successfully if no next sibling of the parent node (or no parent
#          node at all).
# - Error : as it says. Terminate compilation with an error.
# - Done : terminate compilation successfully without further processing.
# - Up : jump back to next sibling of the parent node if any, without visiting
#        any next sibling of the current node.

compile ex_expr_list
asm
	# To compile all the elements of a node, we just have to tell the compiler to
	# enter the node with using compileStateDown
	compileStateDown
end

compile ex_expr
asm
	compileStateDown
end

compile ex_number
asm
	<<
		# Those big "quotes" delimit code that is to be written in the program
		# currently compiled. This syntax is provided by the metasm layer.
		# Here, we push a number (integer), the value of which is given by the
		# first string inside the current node.
		# So we get this string (astGetChildString 0), convert it to an integer
		# (toI), and give it as the argument to the "push" instruction we want
		# to write in the program being compiled.
		# The prefix `i' before the parentheses tells the meta-assembly layer
		# that the "push" instruction has an integer argument.
		# The defined prefixes are :
		# i : integer
		# f : float
		# s : string
		# e : environment symbol
		# c : character
		# l : label
		# To define a dynamically-generated label in the code, use the following
		# syntax :
		# (...code to push a string...):
		#
		# For instance :
		# << (push "Kwak"): jmp@Kwak >> would loop forever,
		# just as :
		# << (push "Kwak"): jmp l(push "Kwak") >> would do,
		# or :
		# << Kwak: jmp @Kwak >> which is exactly the same, but not dynamically-generated at all.

		push i(astGetChildString 0 toI)
	>>
	compileStateNext
end


compile ex_div
asm
	# We tell the compiler to recursively compile the first child of the current node
	astCompileChild 0
	# Then the second
	astCompileChild 1
	# Then we can write the specific opcode
	<< div >>
	compileStateNext
end


compile ex_mul
asm
	astCompileChild 0
	astCompileChild 1
	<< mul >>
	compileStateNext
end


compile ex_sub
asm
	astCompileChild 0
	astCompileChild 1
	<< sub >>
	compileStateNext
end


compile ex_add
asm
	astCompileChild 0
	astCompileChild 1
	<< add >>
	compileStateNext
end


compile ex_minus
asm
	<< push 0 >>
	astCompileChild 0
	<< sub >>
	compileStateNext
end


compile ex_expr_print
asm
	# We compile the expression
	astCompileChild 0
	# Which will result in a single value pushed onto the data stack,
	# so we print it, as well as a carriage return to keep the display clean.
	<< push "\n" print 2 >>
	compileStateNext
end

