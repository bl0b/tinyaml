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

#include "_impl.h"
#include "vm.h"
#include "thread.h"
#include "object.h"

#include <stdio.h>
#include <string.h>

thread_t thread_new(word_t prio, program_t p, word_t ip) {
	thread_t ret = (thread_t)malloc(sizeof(struct _thread_t));
	/*vm_printf("NEW THREAD %p\n",ret);*/
	/*vm_printf("\tPROGRAM %p\n",ret->program);*/
	thread_init(ret,prio,p,ip);
	/* FIXME : deprecated */
	return NULL;
}

void thread_init(thread_t ret, word_t prio, program_t p, word_t ip) {
	gstack_init(&ret->closures_stack,sizeof(dynarray_t));
	gstack_init(&ret->locals_stack,sizeof(struct _data_stack_entry_t));
	gstack_init(&ret->data_stack,sizeof(struct _data_stack_entry_t));
	gstack_init(&ret->call_stack,sizeof(struct _call_stack_entry_t));
	gstack_init(&ret->catch_stack,sizeof(struct _call_stack_entry_t));
	mutex_init(&ret->join_mutex);
	ret->pending_lock = NULL;
	ret->program = p;
	ret->jmp_seg = p;
	ret->jmp_ofs = 0;
	ret->IP = ip;
	ret->prio = prio;
	ret->state = ThreadReady;
	ret->remaining = 0;
	ret->sched_data.next=NULL;
	ret->sched_data.prev=NULL;
	ret->sched_data.value=(word_t)ret;
}


void thread_deinit(vm_t vm, thread_t t) {
	/*vm_printf("thread deinit %p\n",t);*/
	if(t->state==ThreadZombie) {
		if(t->sched_data.prev) {
			t->sched_data.prev->next = t->sched_data.next;
		} else {
			vm->zombie_threads.head = t->sched_data.next;
		}
		if(t->sched_data.next) {
			t->sched_data.next->prev = t->sched_data.prev;
		} else {
			vm->zombie_threads.tail = t->sched_data.prev;
		}
	}
	mutex_deinit(&t->join_mutex);
	gstack_deinit(&t->closures_stack,NULL);
	/*assert(t->locals_stack.sp==(word_t)-1);*/
	gstack_deinit(&t->locals_stack,NULL);
	gstack_deinit(&t->data_stack,NULL);
	gstack_deinit(&t->call_stack,NULL);
	gstack_deinit(&t->catch_stack,NULL);
}



void thread_delete(vm_t vm, thread_t t) {
	/*vm_printf("\tdel thread\n");*/
	thread_deinit(vm,t);
	free(t);
}




int comp_prio(dlist_node_t a, dlist_node_t b);



#undef dnode_del
#define dnode_del(_n)
#undef _dnode_local_new
#define _dnode_local_new(_n)

#define tsts(_x) case _x: return #_x
const char* thread_state_to_str(thread_state_t ts) {
	switch(ts) {
	tsts(ThreadReady);
	tsts(ThreadBlocked);
	tsts(ThreadRunning);
	tsts(ThreadDying);
	tsts(ThreadZombie);
	tsts(ThreadStateMax);
	};
	return "???";
}

void thread_set_state(vm_t vm, thread_t t, thread_state_t state) {
	assert(t->sched_data.value==(word_t)t);
	/*assert(t->state!=state);*/
	/*if(t->state==state) {*/
		/*vm_printf("thread_set_state does nothing ; %s\n",thread_state_to_str(state));*/
	/*}*/
	/*vm_printf("thread_set_state %p : %s => %s\n",t,thread_state_to_str(t->state),thread_state_to_str(state));*/
	switch(t->state) {
	case ThreadBlocked:
		/*dlist_remove_no_free(&vm->yielded_threads,&t->sched_data);*/
		break;
	case ThreadReady:
		dlist_remove_no_free(&vm->ready_threads,&t->sched_data);
		break;
	case ThreadDying:
		break;
	case ThreadRunning:
		dlist_remove_no_free(&vm->running_threads,&t->sched_data);
		break;
	case ThreadStateMax:
		/* thread is just starting */
		if(t->_sync) {
			vm->engine->_fg_thread_start_cb(vm->engine);
		}
	default:;
	};
	t->state=state;
	switch(state) {
	case ThreadZombie:
		/*thread_delete(vm,t);*/
		dlist_insert_tail_node(&vm->zombie_threads,&t->sched_data);
		if(t->_sync) {
			vm->engine->_fg_thread_done_cb(vm->engine);
		}
		break;
	case ThreadReady:
		dlist_insert_sorted(&vm->ready_threads,&t->sched_data,comp_prio);
		break;
	case ThreadBlocked:
		/*dlist_insert_sorted(&vm->yielded_threads,&t->sched_data,comp_prio);*/
		break;
	case ThreadRunning:
		t->remaining=vm->timeslice;
		dlist_insert_sorted(&vm->running_threads,&t->sched_data,comp_prio);
		break;
	case ThreadDying:
		/*mutex_unlock(vm,&t->join_mutex,t);*/
		break;
	default:;
		
	};
}

mutex_t mutex_new() {
	mutex_t m = (mutex_t) malloc(sizeof(struct _mutex_t));
	mutex_init(m);
	return m;
}

void mutex_init(mutex_t m) {
	memset(m,0,sizeof(struct _mutex_t));
}

void mutex_delete(vm_t vm, mutex_t m) {
	/* should it kill pending threads ? */
	mutex_deinit(m);
	free(m);
}

void mutex_deinit(mutex_t m) {
/*
	dlist_node_t dn = m->pending.head;
	while(dn) {
		m->pending.head = dn->next;
		thread_delete(node_value(thread_t,dn));
		dn = m->pending.head;
	}
*/
}

long mutex_lock(vm_t vm, mutex_t m, thread_t t) {
	/*vm_printf("MUTEX LOCK :: thread %p attempts to lock mutex %p owned by %p\n",t,m,m->owner);*/
	if(m->owner==NULL) {
		/*vm_printf("mutex is not owned.\n");*/
		m->owner=t;
	}
	if(m->owner==t) {
		m->count+=1;
		/*vm_printf("mutex %p locked by thread %p (%li recursive locks)\n",m,t,m->count);*/
		return 1;
	} else {
		/*vm_printf("mutex lock : blocking thread %p\n",t);*/
		thread_set_state(vm, t, ThreadBlocked);
		dlist_insert_sorted(&m->pending,&t->sched_data,comp_prio);
		return 0;
	}
}

long mutex_unlock(vm_t vm, mutex_t m, thread_t t) {
	/*vm_printf("MUTEX UNLOCK :: thread %p attempts to unlock mutex %p owned by %p\n",t,m,m->owner);*/
	if(m->owner!=t) {
		vm_printerrf("[VM::ERR] : trying to unlock a mutex that is owned by another thread (%p).\n",m->owner);
	} else if(m->count>0) {
		m->count-=1;
	}
	if(!(m->owner&&m->count)) {
		dlist_node_t dn;
		m->owner=NULL;
		while(m->pending.head) {
			dn = m->pending.head;
			m->pending.head=dn->next;
			t = node_value(thread_t,dn);
			if(dn->next) {
				dn->next->prev=NULL;
			} else {
				m->pending.tail=NULL;
			}
			/*vm_printf("mutex unlock : unblocking thread %p\n",t);*/
			thread_set_state(vm, t, ThreadReady);
		}
	}
	return m->count;
}



vm_blocker_t blocker_new() {
	return new_gstack(sizeof(thread_t));
}


void bf_kt(thread_t*t) {
	if((*t)->state!=ThreadDying&&(*t)->state!=ThreadZombie) {
		/*vm_printf("blocker killing thread %p\n",*t);*/
		vm_kill_thread(_glob_vm,*t);
	}
	vm_obj_deref_ptr(_glob_vm,*t);
}

void blocker_free(vm_blocker_t b) {
	gstack_deinit(b,(void(*)(void*))bf_kt);
	free(b);
}

void blocker_suspend(vm_t vm,vm_blocker_t b,thread_t t) {
	gpush(b,&t);
	vm_obj_ref_ptr(vm,t);
	thread_set_state(vm,t,ThreadBlocked);
}

void blocker_resume(vm_t vm,vm_blocker_t b) {
	thread_t t;
	while(gstack_size(b)) {
		t = *(thread_t*)_gpop(b);
		/*vm_printf("task arrived ! resuming thread %p\n",t);*/
		t->IP-=2;
		vm_obj_deref_ptr(vm,t);
		thread_set_state(vm,t,ThreadReady);
	}
}




