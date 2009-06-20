# Here we demonstrate multi-threading in tinyaml

# This part modifies the example in part 2 to add synchronization by mutual
# exclusion around the read/modify/write instructions in the thread.

data
	0	# thread 1
	0	# thread 2
	0	# mutex
	0	# data
	3	# timeslice value
end

asm
	# Initialize the mutex
	newMtx
	setmem 2

	_set_timeslice 3
	jmp @_start

# Define some utility functions

_lock:
	# Lock the mutex
	crit_begin
	#getmem 2 lockMtx
	lockMtx 2					# same effect as above commented-out line, but in one cycle
	push "Lock...   " getPid push "\n" print 3
	crit_end
	ret 0

_unlock:
	# Unlock the mutex
	crit_begin
	push "Unlock... " getPid push "\n" print 3
	#getmem 2 unlockMtx
	unlockMtx 2					# same effect as above commented-out line, but in one cycle
	crit_end
	ret 0

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


_thread2_mutex_func:
	# This function accesses some data, increases its value, and writes it back.
	# The mutex guarantees that threads WON'T switch execution during the processing
	# of the shared data.

	# We first yield the thread to make sure the other thread starts before we do anything
	yield
_t2m_loop:
	call @_lock
	call @_fetch_data
	inc
	call @_set_data
	call @_test_data
	call @_unlock
	SNZ jmp@_t2m_loop
	ret 0


_start:
	push "\nTimeslice has been set to " _get_timeslice push " cycles per thread.\n\n" print 3

	call @_reset_data

	push "\nDemonstrating mutex :\n\n" print 1

	push 60 newThread @_thread2_mutex_func setmem 0
	push 60 newThread @_thread2_mutex_func setmem 1

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
# $ tinyaml 7.threading.part3.asm
# 
# Timeslice has been set to 3 cycles per thread.
# 
# 
# Demonstrating mutex :
# 
# Lock...   [Thread  0x9c0fc98]					# Now the two threads share equally the job of increasing the variable
# In thread [Thread  0x9c0fc98], read 0
# In thread [Thread  0x9c0fc98], write 1
# Unlock... [Thread  0x9c0fc98]
# Lock...   [Thread  0x9c0ff18]
# In thread [Thread  0x9c0ff18], read 1
# In thread [Thread  0x9c0ff18], write 2
# Unlock... [Thread  0x9c0ff18]
# Lock...   [Thread  0x9c0fc98]
# In thread [Thread  0x9c0fc98], read 2
# In thread [Thread  0x9c0fc98], write 3
# Unlock... [Thread  0x9c0fc98]					# Here the first thread ends because it detects that (variable>=3)
# Lock...   [Thread  0x9c0ff18]					# But the second thread was waiting to lock the mutex and won't
# In thread [Thread  0x9c0ff18], read 3				# notice the value of the variable until after it increments it (see code).
# In thread [Thread  0x9c0ff18], write 4
# Unlock... [Thread  0x9c0ff18]
# 
# Timeslice has been set to 7 cycles per thread.
# 
# 
# Demonstrating mutex :
# 
# Lock...   [Thread  0x9c10068]					# Notice how the two threads behave EXACTLY the same way whater timeslice is set.
# In thread [Thread  0x9c10068], read 0
# In thread [Thread  0x9c10068], write 1
# Unlock... [Thread  0x9c10068]
# Lock...   [Thread  0x9c0fdc8]
# In thread [Thread  0x9c0fdc8], read 1
# In thread [Thread  0x9c0fdc8], write 2
# Unlock... [Thread  0x9c0fdc8]
# Lock...   [Thread  0x9c10068]
# In thread [Thread  0x9c10068], read 2
# In thread [Thread  0x9c10068], write 3
# Unlock... [Thread  0x9c10068]
# Lock...   [Thread  0x9c0fdc8]
# In thread [Thread  0x9c0fdc8], read 3
# In thread [Thread  0x9c0fdc8], write 4
# Unlock... [Thread  0x9c0fdc8]
# 
# Timeslice has been set to 15 cycles per thread.
# 
# 
# Demonstrating mutex :
# 
# Lock...   [Thread  0x9c72140]
# In thread [Thread  0x9c72140], read 0
# In thread [Thread  0x9c72140], write 1
# Unlock... [Thread  0x9c72140]
# Lock...   [Thread  0x9c72240]
# In thread [Thread  0x9c72240], read 1
# In thread [Thread  0x9c72240], write 2
# Unlock... [Thread  0x9c72240]
# Lock...   [Thread  0x9c72140]
# In thread [Thread  0x9c72140], read 2
# In thread [Thread  0x9c72140], write 3
# Unlock... [Thread  0x9c72140]
# Lock...   [Thread  0x9c72240]
# In thread [Thread  0x9c72240], read 3
# In thread [Thread  0x9c72240], write 4
# Unlock... [Thread  0x9c72240]
# 
# Timeslice has been set to 31 cycles per thread.
# 
# 
# Demonstrating mutex :
# 
# Lock...   [Thread  0x9c13200]
# In thread [Thread  0x9c13200], read 0
# In thread [Thread  0x9c13200], write 1
# Unlock... [Thread  0x9c13200]
# Lock...   [Thread  0x9c13300]
# In thread [Thread  0x9c13300], read 1
# In thread [Thread  0x9c13300], write 2
# Unlock... [Thread  0x9c13300]
# Lock...   [Thread  0x9c13200]
# In thread [Thread  0x9c13200], read 2
# In thread [Thread  0x9c13200], write 3
# Unlock... [Thread  0x9c13200]
# Lock...   [Thread  0x9c13300]
# In thread [Thread  0x9c13300], read 3
# In thread [Thread  0x9c13300], write 4
# Unlock... [Thread  0x9c13300]
