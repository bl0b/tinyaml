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

thread_t thread_new(word_t prio, program_t p, word_t ip);
void thread_init(thread_t,word_t prio, program_t p, word_t ip);
void thread_delete(vm_t,thread_t);
void thread_deinit(vm_t vm, thread_t t);

void thread_set_state(vm_t, thread_t, thread_state_t);


mutex_t mutex_new();
void mutex_init(mutex_t);
void mutex_delete(vm_t,mutex_t);
void mutex_deinit(mutex_t);

long mutex_lock(vm_t, mutex_t, thread_t);
long mutex_unlock(vm_t, mutex_t, thread_t);

vm_blocker_t blocker_new();
void blocker_free(vm_blocker_t);
void blocker_suspend(vm_t,vm_blocker_t,thread_t);
void blocker_resume(vm_t,vm_blocker_t);


#endif

