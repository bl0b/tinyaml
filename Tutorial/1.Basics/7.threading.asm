# Here we demonstrate multi-threading in tinyaml

# Each compiled program runs in a virtual thread in the VM. The VM does NOT
# create system threads, it has its own virtual threading model.

# Thread scheduling is based on time slices (typically 100 instructions per
# slice). Some instructions may block the current thread and resume execution
# of the next thread in queue : blocking instructions such as I/O, yield,
# lock/unlock mutex...

# There is no clear distinction between processes and threads in tinyaml (in
# particular there is no fork instruction in tinyaml).

# Each program has its own code segment and data segment. A program running in
# its thread can be seen as a process.

# Each thread has its own stacks (call, data, catch) and registers (registers
# will be covered later on).

# The important thing is that many threads running different parts of the same
# program will share its code and data segments.

# As the execution flow can jump out of a program through the use of dynamic
# functions (once exported to the environment, a dynamic function can be called
# from anywhere), the notion of data sharing between threads is not so clear in
# tinyaml, and one should stick to the axiom "code from THIS program can access
# data from THIS program".

# Each thread has an associated priority level that typically ranges from 0 to
# 99. When the scheduler has to choose between two threads to schedule after a
# time slice or a yield, it will choose the highest priority first. High
# numbers mean high priority. The default thread priority is 50.

# Opcodes covered here :
# - newThread : pops the priority level (int) from stack and creates a new
#               thread. If the instruction is given a label argument, thread
#               execution will start there. Otherwise, pops a function object
#               from stack and calls it (no argument given, return value will
#               be ignored).
# - getPid : returns the Thread object (PID is not proper here. This name is
#            kept for historical reasons).
# - yield : tell the scheduler to skip to next thread in queue.
# - joinThread : pop thread object from stack and wait till it finishes.
# - killThread : pop thread object from stack and kill it at once.
# - crit_begin : starts a critical section (thread can NOT be interrupted).
# - crit_end : ends the critical section.
# - _set_timeslice : change the timeslice value (any integer > 0). If an
#                    integer argument is not provided, the value is popped from
#                    the data stack.
# - _get_timeslice : pushes the timeslice value onto the data stack.
#
# Also, basic mutex synchronization instructions :
# - newMtx : create a new mutex object and push it onto the data stack.
# - lockMtx : lock a mutex object. If an int parameter is given, it is
#             interpreted as a memory address (as in getmem), otherwise the
#             mutex object is popped from the data stack.
#             This instruction may yield the thread until the mutex is free for
#             locking.
# - unlockMtx : unlock a mutex object. As in lockMtx, an int parameter may be
#               given.
#               This instruction may yield the thread if a higher priority
#               thread is waiting to lock the mutex.

data
	0	# thread 1
	0	# thread 2
	0	# mutex
	0	# data
	3	# timeslice value
end

asm
	_set_timeslice 3
	jmp @_start

# Define some utility functions

_lock:
	getmem 2 lockMtx
	push "Lock...   " getPid push "\n" print 3
	ret 0
_unlock:
	push "Unlock... " getPid push "\n" print 3
	getmem 2 unlockMtx
	ret 0

_reset_data:
	push 0 setmem 3
	ret 0

_fetch_data:
	crit_begin
	getmem 3
	push "In thread " getPid push ", read " dup -3 push '\n' print 5
	crit_end
	ret 0

_set_data:
	crit_begin
	push "In thread " getPid push ", write " dup -3 push '\n' print 5
	setmem 3
	crit_end
	ret 0

_test_data:
	crit_begin
	getmem 3 push 3 supEq
	crit_end
	ret 0

_print_data:
	crit_begin
	push "In thread " getPid push " ; data = " getmem 3 push '\n' print 5
	crit_end
	ret 0

_thread1_func:
	enter 1
	push 0 setmem -1	# initialize a counter
_thread1_loop:
	getmem -1 push 3 eq SZ jmp@_thread1_done	# while counter<5 {
	push "In thread "
	getPid
	push " : "
	getmem -1
	push '\n'
	print 5						#	print("In thread [PID] : [counter]\n")
	yield						#	# be nice and let other thread do its work.
	getmem -1 inc setmem -1				#	counter+=1
	jmp @_thread1_loop				# }
_thread1_done:
	leave 1
	ret 0

_thread2_no_mutex_func:
_t2nm_loop:
	call @_fetch_data
	inc
	call @_set_data
	call @_test_data SNZ jmp@_t2nm_loop
	ret 0


_thread2_mutex_func:
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

	# Initialize the mutex
	newMtx
	setmem 2

	push "Demonstrating yielding threads to achieve synchronization.\n" print 1

	push 50 newThread @_thread1_func setmem 0
	push 50 newThread @_thread1_func setmem 1

	push "Thread1 & Thread1 have not yet started.\nNow we yield the main thread.\n" print 1
	yield

	getmem 0 joinThread
	push "Thread1 #1 is done !\n" print 1
	getmem 1 joinThread
	push "Thread1 #2 is done !\n" print 1

	push "\nDemonstrating a race condition :\n\n" print 1

	push 50 newThread @_thread2_no_mutex_func setmem 0
	push 50 newThread @_thread2_no_mutex_func setmem 1

	yield

	getmem 1 joinThread
	getmem 0 joinThread

	call @_reset_data

	push "\nDemonstrating mutex :\n\n" print 1

	push 50 newThread @_thread2_mutex_func setmem 0
	push 50 newThread @_thread2_mutex_func setmem 1

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

