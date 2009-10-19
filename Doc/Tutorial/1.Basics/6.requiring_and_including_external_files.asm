# Here we demonstrate the use of require and include statements

# Many things are explained in the included and required files, so please
# read them thoroughly before reading this file further :
# extras/6.include_example.asm
# extras/6.require_example.asm


# New syntax here :
# include <string>
#	include this file "inline". Files are searched in the current directory
#	if a path is not provided.
# require <string>
#	load and execute this file before compiling the rest of the text.
#	File can either be a compiled binary or source text.
#	If a path is not provided, required files are first searched in the site
#	directory (typically /usr/share/tinyaml or /usr/share/local/tinyaml)
#	then in the current directory.

# New opcodes here :
# call <label>
#	local function call. The current instruction address is backuped and
#	execution jumps to the given address.
# envGet <env var>
#	push value of given environment variable onto the data stack.
#

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


# Example output :
#
# $ tinyaml 6.requiring_and_including_external_files.asm
# We first call the label defined in the included file.
# Hello, include world !
# 
# Now we get the greeting string from the environment of the VM.
# Hello, world
