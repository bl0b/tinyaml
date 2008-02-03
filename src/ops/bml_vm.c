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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "vm.h"
#include "_impl.h"
#include <stdio.h>
#include <string.h>

#include <math.h>
#include "fastmath.h"
#include "opcode_chain.h"
#include "object.h"

/*! \addtogroup vcop_data
 * @{
 */
void _VM_CALL vm_op_clone(vm_t vm, word_t unused) {
	vm_data_t d = _vm_pop(vm);
	assert(d->type&DataManagedObjectFlag);
	vm_push_data(vm, d->type, (word_t)vm_obj_clone_obj(vm,PTR_TO_OBJ(d->data)));
}

void _VM_CALL vm_op_push_Int(vm_t vm, word_t data) {
	vm_push_data(vm, DataInt, data);
}

void _VM_CALL vm_op_push_Float(vm_t vm, word_t data) {
	vm_push_data(vm, DataFloat, data);
}

void _VM_CALL vm_op_push_String(vm_t vm, word_t data) {
	vm_push_data(vm, DataString, data);
}

void _VM_CALL vm_op_push_Opcode(vm_t vm, word_t data) {
	vm_push_data(vm, DataInt, data);
}

void _VM_CALL vm_op_pop(vm_t vm, word_t unused) {
	vm_pop_data(vm,1);
}

void _VM_CALL vm_op_pop_Int(vm_t vm, word_t data) {
	vm_pop_data(vm,data);
}

void _VM_CALL vm_op_dup_Int(vm_t vm, int data) {
	vm_data_type_t a;
	word_t b;
	vm_peek_data(vm,data,&a,&b);
	vm_push_data(vm, a, b);
}


void _VM_CALL vm_op_enter_Int(vm_t vm, word_t size) {
	thread_t t=vm->current_thread;
	gstack_grow(&t->locals_stack,size);
}

void _VM_CALL vm_op_leave_Int(vm_t vm, word_t size) {
	thread_t t=vm->current_thread;
	vm_data_t local;
	long i,min=-size;
	for(i=0;i>min;i-=1) {
		local = _gpeek(&t->locals_stack,i);	/* -1 becomes 0 */
		if(local->type&DataManagedObjectFlag) {
			vm_obj_deref_ptr(vm,(void*)local->data);
			local->type=DataInt;
			local->data=0;
		}
	}
	gstack_shrink(&t->locals_stack,size);
}



/*@}*/


/*! \addtogroup vm_core_ops
 * @{
 */
void _VM_CALL vm_op_nop(vm_t vm, word_t unused) {
	/* mimics /bin/true's behaviour */
}


void _VM_CALL vm_op_print_Int(vm_t vm, int n) {
	vm_data_type_t dt;
	_IFC tmp;
	int k=1-n;
	while(k<=0) {
		vm_peek_data(vm,k,&dt,(word_t*)&tmp);
		switch(dt) {
		case DataInt:
			printf("%li", tmp.i);
			break;
		case DataFloat:
			printf("%lf", tmp.f);
			break;
		case DataString:
			printf("%s", (const char*) tmp.i);
			break;
		case DataObjStr:
			printf("[ObjStr  \"%s\"]",(const char*)tmp.i);
			break;
		case DataObjSymTab:
			printf("[SymTab  %p]",(void*)tmp.i);
			break;
		case DataObjMutex:
			printf("[Mutex  %p]",(void*)tmp.i);
			break;
		case DataObjThread:
			printf("[Thread  %p]",(void*)tmp.i);
			break;
		case DataObjArray:
			printf("[Array  %p]",(void*)tmp.i);
			break;
		case DataObjEnv:
			printf("[Map  %p]",(void*)tmp.i);
			break;
		case DataObjStack:
			printf("[Stack  %p]",(void*)tmp.i);
			break;
		case DataObjFun:
			printf("[Function  %p]",(void*)tmp.i);
			break;
		case DataObjVObj:
			printf("[V-Obj  %p]",(void*)tmp.i);
			break;
		case DataManagedObjectFlag:
			printf("[Undefined Object ! %p]", (opcode_stub_t*)tmp.i);
			break;
		case DataTypeMax:
		default:;
			printf("[Erroneous data %X %lX]", dt, tmp.i);
		};
		k+=1;
	}
	fflush(stdout);
	vm_pop_data(vm,n);
}
/*@}*/


/*! \addtogroup vcop_ctrl
 * @{
 */
void _VM_CALL vm_op_SNZ(vm_t vm, int data) {
	vm_data_type_t a;
	word_t b;
	vm_peek_data(vm,data,&a,&b);
	vm_pop_data(vm,1);
	if(b) {
		vm->current_thread->IP+=2;
	}
}

void _VM_CALL vm_op_SZ(vm_t vm, int data) {
	vm_data_type_t a;
	word_t b;
	vm_peek_data(vm,data,&a,&b);
	vm_pop_data(vm,1);
	if(!b) {
		vm->current_thread->IP+=2;
	}
}

/*
 * Jumps
 */

void _VM_CALL vm_op_jmp_Label(vm_t vm, word_t data) {
	thread_t t=vm->current_thread;
	t->jmp_ofs=t->IP+data;
}
/*@}*/

/*
 * Call stack
 */

/*! \addtogroup vcop_df
 * @{
 */
void _VM_CALL vm_op_call(vm_t vm, word_t unused) {
	vm_data_t d = _vm_pop(vm);
	thread_t t=vm->current_thread;
	vm_dyn_func_t fun = (vm_dyn_func_t) d->data;
	assert(d->type&DataManagedObjectFlag);
	if(fun->closure) { 
		vm_push_caller(vm, t->program, t->IP, 1);
		gpush(&t->closures_stack,&fun->closure);
		/*printf("pushed closure %p\n",fun->closure);*/
	} else {
		vm_push_caller(vm, t->program, t->IP, 0);
	}
	t->jmp_seg=fun->cs;
	t->jmp_ofs=fun->ip;
}

void _VM_CALL vm_op_call_vc(vm_t vm, word_t unused) {
	vm_data_t d;
	vm_dyn_func_t fun;
	dynarray_t da;
	thread_t t=vm->current_thread;
	/* pop dynfun */
	d = _vm_pop(vm);
	assert(d->type==DataObjFun);
	fun = (vm_dyn_func_t) d->data;
	assert(fun->closure==NULL);
	/* pop closure */
	d = _vm_pop(vm);
	assert(d->type==DataObjArray||d->type==DataObjVObj);
	da = (dynarray_t) d->data;
	/* perform call */
	vm_push_caller(vm,t->program, t->IP, 1);
	gpush(&t->closures_stack,&da);
	t->jmp_seg=fun->cs;
	t->jmp_ofs=fun->ip;
}

/*@}*/

/*! \addtogroup vcop_ctrl
 * @{
 */
void _VM_CALL vm_op_call_Label(vm_t vm, word_t data) {
	thread_t t=vm->current_thread;
	vm_push_caller(vm, t->program, t->IP, 0);
	t->jmp_ofs=t->IP+data;
}

/*void _VM_CALL vm_op_lcall_Label(vm_t vm, word_t data) {*/
	/*thread_t t=vm->current_thread;*/
	/*vm_data_type_t a;*/
	/*word_t b;*/
	/*vm_peek_data(vm,0,&a,&b);*/
	/*vm_push_caller(vm, t->program, t->IP, 0);*/
	/*t->jmp_seg=(program_t)b;*/
	/* FIXME : can resolve_label() set up correct data value ? */
	/*t->jmp_ofs=data;*/
/*}*/



void _VM_CALL vm_op_retval_Int(vm_t vm, word_t n) {
	program_t cs;
	word_t ip;
	vm_data_type_t dt;
	word_t d;
	thread_t t=vm->current_thread;
	if(t->call_stack.sp!=(word_t)-1) {
		vm_peek_data(vm,0,&dt,&d);
		vm_pop_data(vm,n);
		vm_poke_data(vm,dt,d);
		vm_peek_caller(vm,&cs,&ip);
		vm_pop_caller(vm,1);
		t->jmp_seg=cs;
		t->jmp_ofs=ip+2;
	} else {
		/*t->state=ThreadDying;*/
		thread_set_state(vm,t,ThreadDying);
	}
}


void _VM_CALL vm_op_ret_Int(vm_t vm, word_t n) {
	program_t cs;
	word_t ip;
	thread_t t=vm->current_thread;
	if(t->call_stack.sp!=(word_t)-1) {
		vm_pop_data(vm,n);
		vm_peek_caller(vm,&cs,&ip);
		vm_pop_caller(vm,1);
		t->jmp_seg=cs;
		t->jmp_ofs=ip+2;
	} else {
		/*t->state=ThreadDying;*/
		thread_set_state(vm,t,ThreadDying);
	}
}


void _VM_CALL vm_op_instCatcher_Label(vm_t vm, long rel_ofs) {
	vm_push_catcher(vm,vm->current_thread->program,vm->current_thread->IP+rel_ofs);
}

void _VM_CALL vm_op_uninstCatcher_Label(vm_t vm, long rel_ofs) {
	(void)_gpop(&vm->current_thread->catch_stack);
	vm->current_thread->jmp_seg=vm->current_thread->program;
	vm->current_thread->jmp_ofs=vm->current_thread->IP+rel_ofs;
}

void _VM_CALL vm_op_throw(vm_t vm, word_t unused) {
	call_stack_entry_t cse;
	vm_data_t e;
	if((long)vm->current_thread->data_stack.sp>=0) {
		e = _vm_pop(vm);
	} else {
		e = (struct _data_stack_entry_t[]){{ DataString, (word_t)"Global failure"}};
	}
	if(e->type&&DataManagedObjectFlag) {
		vm_obj_ref_ptr(vm,(void*)e->data);
	}
	vm->exception = e;
	if((long)vm->current_thread->catch_stack.sp>=0) {
		cse = _gpop(&vm->current_thread->catch_stack);
		vm->current_thread->jmp_seg=cse->cs;
		vm->current_thread->jmp_ofs=cse->ip;
		vm->current_thread->call_stack.sp = cse->has_closure;
		vm_push_data(vm,e->type,e->data);
	} else {
		vm_fatal("Uncaught exception");
	}
}

/*@}*/



void _VM_CALL vm_op_getmem_Int(vm_t vm, int n);


/*! \addtogroup vcop_thrd
 * @{
 */
void _VM_CALL vm_op_newThread_Label(vm_t vm, word_t rel_ofs) {
	vm_data_t d = _vm_pop(vm);
	thread_t t=vm->current_thread;
	word_t ofs = t->IP+rel_ofs;
	assert(d->type==DataInt);
	t = vm_add_thread(vm, t->program, ofs, d->data, 0);
	/*printf("new thread has handle %p\n",t);*/
	vm_push_data(vm,DataObjThread,(word_t)t);
}

void _VM_CALL vm_op_getPid(vm_t vm, word_t unused) {
	vm_push_data(vm,DataObjThread,(word_t) vm->current_thread);
}


void _VM_CALL vm_op_newMtx(vm_t vm, word_t unused) {
	word_t handle = (word_t) vm_mutex_new();
	/*printf("push new mutex %lx\n",handle);*/
	vm_push_data(vm, DataObjMutex, handle);
}


void _VM_CALL vm_op_lockMtx_Int(vm_t vm, long memcell) {
	vm_data_t d;
	thread_t t = vm->current_thread;
	mutex_t m;
	vm_op_getmem_Int(vm,memcell);
	d = _vm_pop(vm);
	assert(d->type==DataObjMutex);
	m = (mutex_t)d->data;
	/*assert_ptr_is_obj(m);*/
	assert(_is_a_ptr(m,DataObjMutex));
	mutex_lock(vm,m,t);
}


void _VM_CALL vm_op_unlockMtx_Int(vm_t vm, long memcell) {
	vm_data_t d;
	thread_t t = vm->current_thread;
	mutex_t m;
	vm_op_getmem_Int(vm,memcell);
	d = _vm_pop(vm);
	assert(d->type==DataObjMutex);
	m = (mutex_t)d->data;
	/*assert_ptr_is_obj(m);*/
	assert(_is_a_ptr(m,DataObjMutex));
	mutex_unlock(vm,m,t);
}



void _VM_CALL vm_op_lockMtx(vm_t vm, word_t unused) {
	vm_data_t d;
	thread_t t = vm->current_thread;
	if(!t->pending_lock) {
		d = _vm_pop(vm);
		assert(d->type==DataObjMutex);
		t->pending_lock = (mutex_t)d->data;
		assert(_is_a_ptr(t->pending_lock,DataObjMutex));
		/*assert_ptr_is_obj(t->pending_lock);*/
	}
	if(mutex_lock(vm,t->pending_lock,t)) {
		t->pending_lock=NULL;
	}
}


void _VM_CALL vm_op_unlockMtx(vm_t vm, word_t unused) {
	vm_data_t d;
	thread_t t = vm->current_thread;
	mutex_t m;
	d = _vm_pop(vm);
	m = (mutex_t)d->data;
	assert(_is_a_ptr(m,DataObjMutex));
	/*assert_ptr_is_obj(m);*/
	mutex_unlock(vm,m,t);
}

/*! \brief dirty hack : compute thread_t address from thread->join_mutex address. requires a local thread_t variable. */
#define join_lock_to_thread(_t,_m) ((thread_t) (((char*)(_m)) - ( ((char*)&(_t)->join_mutex) - ((char*)(_t)) ) ))

void _VM_CALL vm_op_joinThread(vm_t vm, word_t unused) {
	vm_data_t d;
	thread_t t = vm->current_thread,joinee;
	if(!t->pending_lock) {
		d=_vm_pop(vm);
		assert(d->type==DataObjThread);
		joinee = (thread_t)d->data;
		assert(_is_a_ptr(joinee,DataObjThread));
		assert(((thread_t)joinee->sched_data.value) == joinee /* check that it's really a thread */);
		t->pending_lock = &joinee->join_mutex;
		vm_obj_ref_ptr(vm,joinee);
	} else {
		joinee = join_lock_to_thread(t,t->pending_lock);
		/*printf("pending_lock %p => thread should be %p\n",t->pending_lock,joinee);*/
	}
	if(mutex_lock(vm,t->pending_lock,t)) {
		mutex_unlock(vm,t->pending_lock,t);
		t->pending_lock=NULL;
		vm_obj_deref_ptr(vm,joinee);
	}
}


void _VM_CALL vm_op_killThread(vm_t vm, word_t unused) {
	vm_data_t d = _vm_pop(vm);
	thread_t victim = (thread_t)d->data;
	assert(_is_a_ptr(victim,DataObjThread));
	assert(((thread_t)victim->sched_data.value) == victim /* check that it's really a thread */);
	vm_obj_deref_ptr(vm,victim);
	vm_kill_thread(vm,victim);
	/*thread_set_state(vm,victim,ThreadDying);*/
}



void _VM_CALL vm_op_yield(vm_t vm, word_t unused) {
	/* skip yield when thread resumes */
	/*vm->current_thread->IP+=2;*/
	/*thread_set_state(vm,vm->current_thread,ThreadReady);*/
	vm->current_thread->remaining=0;
	/*printf("YIELD %p\n",vm->current_thread);*/
}
/*@}*/


/*! \addtogroup vcop_df
 * @{
 */

void _VM_CALL vm_op_dynFunNew_Label(vm_t vm, word_t rel_ofs) {
	vm_dyn_func_t handle = vm_dyn_fun_new();
	handle->cs = vm->current_thread->program;
	handle->ip = vm->current_thread->IP+rel_ofs;
	/*printf("push new mutex %lx\n",handle);*/
	vm_push_data(vm, DataObjFun, (word_t) handle);
}


void _VM_CALL vm_op_dynFunAddClosure(vm_t vm, word_t unused) {
	word_t index;
	vm_data_t dc = _vm_pop(vm);
	/* FIXME : changes pops to peeks wherever it fits and change asm compiler code accordingly */
	vm_data_t df = _vm_peek(vm);
	vm_dyn_func_t f=(vm_dyn_func_t) df->data;
	word_t data=0;
	assert(_is_a_ptr(f,DataObjFun));
	if(dc->type&DataManagedObjectFlag) {
		data = (word_t) vm_obj_clone_obj(vm,PTR_TO_OBJ(dc->data));
	} else {
		data = dc->data;
	}
	if(!f->closure) {
		f->closure = vm_array_new();
		vm_obj_ref_ptr(vm,f->closure);
	}
	index = f->closure->size;
	dynarray_set(f->closure,f->closure->size,dc->type);
	dynarray_set(f->closure,f->closure->size,data);
	/*printf("dynFunAddClosure(%li) : %li,%8.8lX\n",index>>1,f->closure->data[index],f->closure->data[index+1]);*/
}

/*@}*/

