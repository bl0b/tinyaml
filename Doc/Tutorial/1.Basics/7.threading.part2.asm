# Here we demonstrate multi-threading in tinyaml

# This part focuses on the effects of the timeslice value and demonstrates a
# race condition.
# The example output is commented to stress out the race condition.

# All *_data functions define a critical section so their execution is atomic
# to the scheduler, but for the final ret instruction.

# New opcodes here :
# _get_timeslice
#	Push current timeslice value (number of instructions a thread will execute
#	before execution switches to the next running thread) onto the data stack.
# _set_timeslice
#	Set the timeslice value with popping an integer from the data stack.

data
	0	# thread 1
	0	# thread 2
	0	# mutex	: this one will only be used in the 3rd part. We keep it here so the code
		# won't change much.
	0	# data
	3	# timeslice value
end

asm
	_set_timeslice 3			# We start with a very small value, threads will switch fast.
	jmp @_start

# Define some utility functions

_reset_data:
	# A little exception to the aforementioned rule, this function will only be called by the main thread
	# when no other thread is running. We don't need to ensure that its execution is atomic.
	push 0 setmem 3
	ret 0

_fetch_data:
	# Push shared data value onto data stack
	crit_begin
	getmem 3
	push "In thread " getPid push ", read " dup -3 push '\n' print 5
	crit_end
	ret 0

_set_data:
	# Pop value from data stack and write into shared data
	crit_begin
	push "In thread " getPid push ", write " dup -3 push '\n' print 5
	setmem 3
	crit_end
	ret 0

_test_data:
	# Push result of "shared data value is >= 3" onto data stack
	crit_begin
	getmem 3 push 3 supEq
	crit_end
	ret 0

_thread2_no_mutex_func:
	# This function accesses some data, increases its value, and writes it back.
	# Each of these operations are atomic (*), but threads may switch execution in between
	# at any moment.
	#
	# (*) : atomic but for the final ret instruction, which is harmless but creates one more potential
	#       switch point.

	# We first yield the thread to make sure the other thread starts before we do anything
	yield
_t2nm_loop:
	call @_fetch_data
	inc
	call @_set_data
	call @_test_data SNZ jmp@_t2nm_loop
	ret 0


_start:
	push "\nTimeslice has been set to " _get_timeslice push " cycles per thread.\n\n" print 3

	push "Demonstrating a race condition :\n\n" print 1

	call @_reset_data

	push 60 newThread @_thread2_no_mutex_func setmem 0
	push 60 newThread @_thread2_no_mutex_func setmem 1

	yield

	getmem 1 joinThread
	getmem 0 joinThread

	# Now, increase the timeslice and re-run the test.
	getmem 4
	push 2
	mul
	inc
	setmem 4
	getmem 4 push 31 infEq SNZ jmp @_done
	getmem 4 _set_timeslice
	jmp @_start
_done:
end


# Example command and output :
#
# $ tinyaml 7.threading.part2.asm
# 
# Timeslice has been set to 3 cycles per thread.
# 
# Demonstrating a race condition :
# 
# In thread [Thread  0x9a327a8], read 0			# Notice how the threads do exactly the same work TWICE.
# In thread [Thread  0x9a324d0], read 0			# They are supposed to cooperate in increasing the data value.
# In thread [Thread  0x9a327a8], write 1		# They also should NOT be able to read the value before it is
# In thread [Thread  0x9a324d0], write 1		# written back by the other thread.
# In thread [Thread  0x9a327a8], read 1
# In thread [Thread  0x9a324d0], read 1
# In thread [Thread  0x9a327a8], write 2
# In thread [Thread  0x9a324d0], write 2
# In thread [Thread  0x9a327a8], read 2
# In thread [Thread  0x9a324d0], read 2
# In thread [Thread  0x9a327a8], write 3
# In thread [Thread  0x9a324d0], write 3
# 
# Timeslice has been set to 7 cycles per thread.
# 
# Demonstrating a race condition :
# 
# In thread [Thread  0x9a32628], read 0
# In thread [Thread  0x9a37ff8], read 0
# In thread [Thread  0x9a32628], write 1
# In thread [Thread  0x9a37ff8], write 1
# In thread [Thread  0x9a32628], read 1
# In thread [Thread  0x9a37ff8], read 1
# In thread [Thread  0x9a32628], write 2
# In thread [Thread  0x9a37ff8], write 2
# In thread [Thread  0x9a32628], read 2
# In thread [Thread  0x9a37ff8], read 2
# In thread [Thread  0x9a32628], write 3
# In thread [Thread  0x9a37ff8], write 3
# 
# Timeslice has been set to 15 cycles per thread.
# 
# Demonstrating a race condition :
# 
# In thread [Thread  0x9a378b8], read 0			# Here the first passes appear to be synchronous, but...
# In thread [Thread  0x9a378b8], write 1
# In thread [Thread  0x9a379b8], read 1
# In thread [Thread  0x9a379b8], write 2
# In thread [Thread  0x9a378b8], read 2			# Here we fail again.
# In thread [Thread  0x9a379b8], read 2
# In thread [Thread  0x9a378b8], write 3
# In thread [Thread  0x9a379b8], write 3
# 
# Timeslice has been set to 31 cycles per thread.
# 
# Demonstrating a race condition :
# 
# In thread [Thread  0x9a31bf0], read 0			# This is how the process should happen, so one may think
# In thread [Thread  0x9a31bf0], write 1		# the code is OK. However it's only out of luck. May the
# In thread [Thread  0x9a31cf0], read 1			# timeslice value change again, and we'll fail again.
# In thread [Thread  0x9a31cf0], write 2
# In thread [Thread  0x9a31bf0], read 2
# In thread [Thread  0x9a31bf0], write 3
# In thread [Thread  0x9a31cf0], read 3
# In thread [Thread  0x9a31cf0], write 4
# 

