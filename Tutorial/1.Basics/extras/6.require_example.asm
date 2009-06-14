# This code will be executed BEFORE the text following the require statement is
# PARSED (and thus compiled).

# This is very useful in many cases :
# - the required file may define new language features (and the associated
#   compiler),
# - the required file may define ENVIRONMENT VARIABLES (which we haven't covered
#   yet, this is the occasion),
# - the required file may do anything that has to be done before the main file
#   is parsed.

# Here we will demonstrate the definition of environment variables, and their
# use is demonstrated in the main file (6.requiring_and_including_files.asm).

# The new opcodes to handle environment variables are :
# - envAdd : create/set an environment variable.
#            use : push value, push symbolic_name, envAdd.
#                  push value, envAdd "symbolic_name".
# - envGet : get the value of the given environment variable.
#            use : envGet &symbolic_name.
#            The variable must exist prior to compiling the file containing this
#            instruction !
# - envSet : set the value of the given environment variable.
#            use : push value, envSet &symbolic name.
#            The variable must exist prior to compiling the file containing this
#            instruction !
# - envLookup : gets the index of a symbolic name in the environment.
#               can be used to check that a variable exists. Returns -1 if there
#               is no such variable.
#               use : push "symbolic_name" envLookup


# Environment variables are named, VM-wide variables. They can be accessed by
# name (hashtable => slower) or by index (array => faster). They are always
# described by name in the source code. Access by index is handled by the
# compiler and name resolution is performed each time a program is loaded slash
# compiled.

# So, for instance, the following code will fail to compile :
# asm
#	push "Hello, world\n"
#	push "my_greeting"
#	envAdd
#	envGet &my_greeting		# failure happens here : the value is
#					# undefined at compile-time.
#	print 1
# end

# Here we split this sequence in two, the envGet part being compiled AFTER in
# the main file.

asm
	push "Hello, world\n"
	push "my_greeting"
	envAdd
end

