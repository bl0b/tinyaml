# Here we demonstrate the use of symbolic assembly layer.

# This layer enables defining and using symbolic names for global and local data,
# as well as If..Then..Endif and If..Then..Else..Endif constructs.
# It also enables definition and use of data structures.

# First, we require the compiled language layer file.
# Since it's not found in the current directory, tinyaml will go search for it in the site directory (typically, /usr/share/tinyaml)
require "symasm.wc"

# After this require statement, we are ready to use the features defined in symasm.wc

glob				# This new keyword starts a global data definition bloc, with symbolic names.
				# Notice that the first offset for symbolic variables is 1, the offset 0 is reserved
				# (offset 0 can be thought as the NULL value).
	foobar = 0		# <name> = <initial value>
	my_struc_var = 0
end

# Given a symbolic name, we can :
# - access its value with prefixing the name with +$
# - set its value (popped from the top of the data stack) with prefixing wth -$
# - get its offset in data segment or locals stack with prefixing with $
#
# +$ and -$ have been chosen from the data stack point of view :
# - to access a value, we push it onto the data stack
# - to set a value, we pop it from the data stack top

asm
	push "Hello, world\n"
	-$foobar

	+$foobar
	print 1

	push "the symbol foobar is at offset " $foobar push "\n" print 3
end


struc Wibble {			# the keyword struc starts a data structure definition (it is followed by the symbolic name
				# of this structure type, and the field declarations are enclosed between braces).
	field_1			# Since the assembly language is not typed, we only define
	field_2			# the symbolic name of each field here.
}


asm
	# We create a new structure instance
	# To initialize a field, we can either :
	# - write a single opcode that will push ONE data that will be used as the initial value
	# - use an asm..end bloc that will push ONE data to the same effect
	strucNew Wibble {
		field_1 : asm push 23 push 19 add end		# field_1=42
		field_2 : push "Kwak"				# field_2="Kwak"
	}
	# the new structure instance is on top of data stack,
	# so we can store it into a variable
	-$my_struc_var

	# The structure instance in memory is merely an array.

	# To access a field in a structure instance, we must first push the instance, then ask for the given field.
	# Since there is no static typing, we have to mention the structure type name so the field can be fetched.

	# Field access and setting uses the same prefixing convention as variable data accessing and setting (with the + and -).
	# So we use :
	# +$struc_instance +(STRUC_NAME.FIELD_NAME) to access a field,
	# +$struc_instance (code to push single data) -(STRUC_NAME.FIELD_NAME) to set a field.

	push "Duck says "
	+$my_struc_var +(Wibble.field_2)
	push '\n'
	print 3

	push 1 -$foobar

	# Now we demonstrate the If..Then..Endif and If..Then..Else..Endif blocs

	# The two constructs are :
	# - [ <sub asm bloc : THEN> ]
	#   the THEN bloc will be executed only if value on top of stack is NOT ZERO (the condition value is always popped)
	# - [[ <sub asm bloc : THEN> ][ <sub asm bloc : ELSE> ]]
	#   the THEN bloc will be executed only if value on top of stack is NOT ZERO, otherwise the ELSE bloc will be executed
	#   (the condition value is also always popped)

	# If foobar!=0 Then print(my_struc_var.field_1) Endif

	+$foobar [
		+$my_struc_var +(Wibble.field_1)
		push '\n'
		print 2
	]

	nop

	# If foobar==0 Then print("Foobar is 0 !") Else print("Foobar=", foobar) Endif

	+$foobar push 0 eq [[
		push "Foobar is 0 !\n"
		print 1
	][
		push "Foobar="
		+$foobar
		push "\n"
		print 3
	]]


	# Last thing to demonstrate here is the definition of local symbols.
	# Local symbols have a restricted scope and lifespan. Define them using the `local' keyword followed by a list of symbols.
	# The code where they can be used is enclosed between braces. They are allocated on the locals stack and automatically freed
	# at the end of the code bloc.
	local a, b, c {
		push "In a local context.\n" print 1
		push "local symbol a has offset " $a push "\n" print 3
		push "local symbol b has offset " $b push "\n" print 3
		push "local symbol c has offset " $c push "\n" print 3
		# the `local' statement is reentrant.
		local d {
			push "In a local context inside the first one.\n" print 1
			push "local symbol a has offset " $a push "\n" print 3
			push "local symbol b has offset " $b push "\n" print 3
			push "local symbol c has offset " $c push "\n" print 3
			push "local symbol d has offset " $d push "\n" print 3
		}
	}

end


# Example command and output :
#
# $ tinyaml Tutorial/2.Layers/1.symbolic.asm
# Hello, world
# the symbol foobar is at offset 1
# Duck says Kwak
# 42
# Foobar=1
# In a local context.
# local symbol a has offset -1
# local symbol b has offset -2
# local symbol c has offset -3
# In a local context inside the first one.
# local symbol a has offset -2
# local symbol b has offset -3
# local symbol c has offset -4
# local symbol d has offset -1

