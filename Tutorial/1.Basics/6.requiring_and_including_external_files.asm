# Here we demonstrate the use of require and include statements

# Many things are explained in the included and required files, so please
# read them thoroughly before reading this file further :
# extras/6.include_example.asm
# extras/6.require_example.asm


# Execute NOW, compile all text after this statement later.
require "extras/6.require_example.asm"

# Compile NOW. The effect of include is to inline the code here.
include "extras/6.include_example.asm"

asm
	push "We first call the label defined in the included file.\n" print 1
	call @_hello_label

	push '\n' print 1		# one empty line for clarity

	push "Now we get the greeting string from the environment of the VM.\n"
	envGet &my_greeting
	print 2
end
