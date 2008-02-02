/* TinyaML
 * Copyright (C) 2007 Damien Leroux
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */


#ifndef _BML_THREAD_H_
#define _BML_THREAD_H_

/*! \addtogroup Threads
 * @{
 * \brief Implements an execution context for programs to execute. There is no real API for accessing and modifying
 * a given thread, since all standard interaction is done in the current thread \ref lolvl.
 *
 * <div style="float: right; border: solid black 1px; width=200px;" width="200px">
 * <div align="center"><em>Thread state transitions</em><hr></div><img src="thread_states.png" border="solid black 1px;"></div>
 */

/*! \addtogroup thread_t Thread
 * \brief API to handle thread objects.
 * @{
 */
/*! \brief create a new thread structure. */
thread_t thread_new(word_t prio, program_t p, word_t ip);
/*! \brief initialize a newly allocated thread structure. */
void thread_init(thread_t,word_t prio, program_t p, word_t ip);
/*! \brief free a thread's resources and structure. */
void thread_delete(vm_t,thread_t);
/*! \brief free a thread's resources. */
void thread_deinit(vm_t vm, thread_t t);

/*! \brief change thread state. */
void thread_set_state(vm_t, thread_t, thread_state_t);
/*@}*/

/*! \addtogroup mutex_t Mutex
 * \brief API to handle mutual exclusion devices.
 * @{
 */
mutex_t mutex_new();
void mutex_init(mutex_t);
void mutex_delete(vm_t,mutex_t);
void mutex_deinit(mutex_t);

long mutex_lock(vm_t, mutex_t, thread_t);
long mutex_unlock(vm_t, mutex_t, thread_t);

/*@}*/


/*! \addtogroup vm_blocker_t Blocker
 * \brief API to handle blocking calls in VM instructions. A thread can be suspended synchronously and resumed asynchronously.
 * @{
 */
vm_blocker_t blocker_new();
void blocker_free(vm_blocker_t);
void blocker_suspend(vm_t,vm_blocker_t,thread_t);
void blocker_resume(vm_t,vm_blocker_t);

/*@}*/

/*@}*/

#endif

