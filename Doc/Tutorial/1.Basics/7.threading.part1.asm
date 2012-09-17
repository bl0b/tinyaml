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
#                                Part I : Basics                                #
#                               7. Threading (1/3)                              #
#                                                                               #
#   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #
#################################################################################

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

# New opcodes here :
# newThread <label>
# newThread
#	pops the priority level (int) from stack and creates a new
#	thread. If the instruction is given a label argument, thread
#	execution will start there. Otherwise, a function object is
#	popped from stack BEFORE the priority value is popped. The function
#	object is then called without argument and its return value is
#	ignored.
# getPid
#	returns the Thread object (PID is not proper here. This name is
#	kept for historical reasons).
# yield
#	tell the scheduler to skip to next thread in queue.
# joinThread
#	pop thread object from stack and wait till it finishes.
# killThread
#	pop thread object from stack and kill it at once.
# crit_begin
#	starts a critical section (thread can NOT be interrupted
#	while inside a critical section).
# crit_end
#	ends the critical section.
# _set_timeslice <int>
# _set_timeslice
#	change the timeslice value (any integer > 0). If an
#	integer argument is not provided, the value is popped from
#	the data stack.
# _get_timeslice
#	pushes the timeslice value onto the data stack.
#
# Also, basic mutex synchronization instructions :
# newMtx
#	create a new mutex object and push it onto the data stack.
# lockMtx
#	lock a mutex object. If an int parameter is given, it is
#	interpreted as a memory address (as in getmem), otherwise the
#	mutex object is popped from the data stack.
#	This instruction may yield the thread until the mutex is free for
#	locking.
# unlockMtx
#	unlock a mutex object. As in lockMtx, an int parameter may be given.
#	This instruction may yield the thread if a higher priority thread is
#	waiting to lock the mutex.
#
# New opcode unrelated to threads :
# eq
#	pops two values from the data stack, pushes 1 if they are equal, 0
#	otherwise.
# inc
#	increments the top of the data stack (expected to be an int).

# This tutorial is rather long, so it is divided into three parts :
# - In part 1 (this file), we demonstrate creating threads, joining them (i.e.
#   waiting for a thread to finish), and yielding threads to synchronize their
#   execution.
# - In part 2, we demonstrate the effects of changing the timeslice value, as
#   well as race conditions that can occur when threads are accessing and
#   modifying unsynchronized resources. We also introduce critical sections to
#   ensure that some instructions sequences can't be interrupted by the
#   scheduler.
# - In part 3, we modify the example from part 2 to achieve synchronized access
#   and modification of a resource.


data
	0	# thread 1
	0	# thread 2
end

asm
	push "\nTimeslice value is " _get_timeslice push " cycles per thread.\n\n" print 3

	push "Demonstrating yielding threads to achieve synchronization.\n" print 1

	push 50 newThread @_thread1_func setmem 0
	push 50 newThread @_thread1_func setmem 1

	push "Thread1 & Thread1 have not yet started.\nNow we yield the main thread.\n" print 1
	yield

	getmem 0 joinThread
	push "Thread1 #1 is done !\n" print 1
	getmem 1 joinThread
	push "Thread1 #2 is done !\n" print 1

	ret 0						# the code after the ret instruction will not be
							# executed in the main thread. It is another way
							# to write :
							# jmp @_start
							# [write some functions]
							# _start: [main code]

# Define some utility functions

_thread1_func:
	enter 1
	push 0 setmem -1				# initialize a counter in a local variable
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

end


# Example command and output :
#
# $ tinyaml 7.threading.part1.asm
# 
# Timeslice value is 100 cycles per thread.
# 
# Demonstrating yielding threads to achieve synchronization.
# Thread1 & Thread1 have not yet started.
# Now we yield the main thread.
# In thread [Thread  0x93d31c8] : 0
# In thread [Thread  0x93d32e8] : 0
# In thread [Thread  0x93d31c8] : 1
# In thread [Thread  0x93d32e8] : 1
# In thread [Thread  0x93d31c8] : 2
# In thread [Thread  0x93d32e8] : 2
# Thread1 #1 is done !
# Thread1 #2 is done !

# NOTE : the values displayed when print'ing a Thread object are pointers in
#        memory. They may differ at each execution.

