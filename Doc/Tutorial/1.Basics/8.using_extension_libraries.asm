# Here we explain and demonstrate the use of extension libraries.

# Extension libraries define new sets of instructions. Currently, there are
# three extensions defined :
# - IO :
#     implements file and TCP/UDP socket access. Also provides code to quickly
#     write threaded multi-client TCP servers.
# - RTC :
#     Real-Time Clock driver. Allows for scheduling tasks, and provides a
#     precise and real-time date. It includes the notion of tempo (BPM), to
#     enable musical-oriented uses. Clock resolution can be set from roughly
#     10 ms (1/0.128 ms) to 0.1 ms (1/8.192 ms).
# - MessageQueue :
#     Defines message queues (single writer, multiple readers) to achieve
#     synchronous inter-thread communication. Features immediate writes and
#     blocking reads.

# The statement loadlib [lib_name] loads and initializes the given library
# *BEFORE* the code that follows is parsed and compiled, pretty much the
# same way as the require statement.
#
# Note that the argument to loadlib is NOT a string, but a symbol indeed. This
# is because tinyaml will form filenames after this argument :
# if argument is LIBNAME and the libraries are installed in SITE_LIB_DIR,
# tinyaml will use these two filenames :
# - SITE_LIB_DIR/LIBNAME.tinyalib :
#        a tinyaml source which defines the new instructions and the driver
#        code.
# - SITE_LIB_DIR/libtinyaml_LIBNAME.so :
#        the shared library containing the corresponding executable code.
#
# This is covered in more detail in section 4, Writing extensions.

# New syntax here :
# loadlib LIBNAME
#	loads the library called LIBNAME.

# New opcodes here :
# msgQueueNew
# msgQueueReaderNew
# msgQueueWrite
# msgQueueRead
#	see Doc/OpcodeReference/extensions/MessageQueue.txt.
# arrayNew
#	create a new array object and push it onto the data stack.


loadlib MessageQueue						# We want to use the MQ extension.


data
	0							# The message queue
	0							# A thread which will read the messages
	0							# The message queue reader
	0							# The thread will store the read data here
end

asm
	msgQueueNew setmem 0					# Init the message queue
	push 80 newThread @_th setmem 1				# Create a new thread. We give it high priority
								# so it will preempt execution when a new message
								# arrives.
	yield							# ensure the thread starts
	getmem 0 push "   Hello, world." msgQueueWrite
	#yield							# The reader thread has a priority >= to the default priority (50),
								# so it will preempt execution. 
	getmem 0 push 23 msgQueueWrite
	getmem 0 arrayNew msgQueueWrite

	jmp @_end

	
_th:	getmem 0 msgQueueReaderNew setmem 2			# Init the message queue reader
_lp:	getmem 2 msgQueueRead setmem 3				# Read a new message. This will block the thread until
	push "Have read " getmem 3 push "\n" print 3		# a message arrives. Then print it.
	jmp @_lp						# We infinitely loop.

_end:
	getmem 1 killThread					# Kill the reader thread.
	push 0 setmem 0						# Give the garbage collector some work to do.
	push 0 setmem 1						# There is no instruction to explicitly delete
	push 0 setmem 2						# data.
	push 0 setmem 3
	ret 0
end

