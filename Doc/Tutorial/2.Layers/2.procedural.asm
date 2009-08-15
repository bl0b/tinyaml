# Here we demonstrate the use of procedural assembly layer.

# In this layer, procedure and function are similar words and can be used
# interchangeably.

# This layer enables writing functions in a more coder-friendly manner.
# It also defines an `export' keyword to easily export functions to the VM
# environment, for use in other programs.

# A typical function declaration looks like (the ... just indicate that the
# list may go on) :
#
# func my_function ( arg1, arg2, ... )
#     ...function code...
# endfunc
#
# A `ret 0' instruction is automatically appended to the function code.
#
# To automatically export a function to the environment, just prepend the word
# `export' :
#
# export func my_public_function()
# endfunc
#
# To call a function, use :
#
# % FUNCTION_NAME ( ...code to push value..., ...code to push value..., ... )
#
# Return values are not specifically handled, they are just pushed onto the
# data stack.
#
# NOTE : there is no check for the number of defined arguments or the number of
# provided values.
#
# In a function call, arguments are processed from right to left. This allows
# for variable arguments handling.
#
# The function vararg_to_array demonstrates this feature.
#

require "procasm.wc"	# automatically require the symasm layer also

func hello(whom)
	push "Hello, "
	+$whom
	push " !\n"
	print 3
endfunc

func vararg_to_array(sup_args)
	# sup_args is the number of supplementary arguments supplied, which we will transform
	# into an array containing these arguments
	# Call example : %vararg_to_array(push "foo", push "bar", push 2, push 23, push 42)
	+$sup_args [
		arrayNew
		_fill_lp:
			swap 1				# top of stack after swap : array, last sup arg
			dup -1 arraySize		# set index to end of array
			arraySet
			+$sup_args dec -$sup_args	# decrement
			+$sup_args SZ jmp@_fill_lp	# loop until sup_args==0
	]
	# now we have array on top of stack, so just return.
endfunc

func vararg_example(arg1, arg2, sup_args)
	%vararg_to_array(+$sup_args)			# Transform the supplementary arguments into an array,
	-$sup_args					# and overwrite the local variable.
	push "have sup args as "			# Little display
	+$sup_args					#
	push ", containing "				#
	+$sup_args arraySize				#
	push " items.\n"				#
	print 5						#

	local i {					# We still can define local variables
		push 0 -$i				# i is a counter to iterate in the array
	_disp_lp:					# Guess what this loop does ?
		+$i +$sup_args arraySize inf [
			push "Sup arg #"		# Yet another little display
			+$i				#
			push " : "			#
			+$sup_args +$i arrayGet		#
			push '\n'			#
			print 5				#
			+$i inc -$i
			jmp@_disp_lp
		]
	}
endfunc


asm
	%hello(push "Foobar")
	%vararg_example(push "Foo", push "Bar", push 3, push 23, push 42, push "This is not a string.")
end


# Example command and output :
#
# $ tinyaml Tutorial/2.Layers/2.procedural.asm
# Hello, Foobar !
# have sup args as [Array  0x928cd00], containing 3 items.
# Sup arg #0 : 23
# Sup arg #1 : 42
# Sup arg #2 : This is not a string.
 
