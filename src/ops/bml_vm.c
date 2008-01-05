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

void _VM_CALL vm_op_nop(vm_t vm, word_t unused) {
	/* mimics /bin/true's behaviour */
}




/* FIXME : have print take an Int and pop&print as many values */
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
		case DataObject:
			printf("[Object %p]", (opcode_stub_t*)tmp.i);
			break;
		};
		k+=1;
	}
	vm_pop_data(vm,n);
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

void _VM_CALL vm_op_SNZ(vm_t vm, int data) {
	vm_data_type_t a;
	word_t b;
	vm_peek_data(vm,data,&a,&b);
	vm_pop_data(vm,1);
	if(b) {
		node_value(thread_t,vm->current_thread)->IP+=2;
	}
}

void _VM_CALL vm_op_SZ(vm_t vm, int data) {
	vm_data_type_t a;
	word_t b;
	vm_peek_data(vm,data,&a,&b);
	vm_pop_data(vm,1);
	if(!b) {
		node_value(thread_t,vm->current_thread)->IP+=2;
	}
}

/*
 * Jumps
 */

void _VM_CALL vm_op_jmp_Label(vm_t vm, word_t data) {
	thread_t t=node_value(thread_t,vm->current_thread);
	t->jmp_ofs=t->IP+data;
}

/*
 * Call stack
 */

void _VM_CALL vm_op_call_Label(vm_t vm, word_t data) {
	thread_t t=node_value(thread_t,vm->current_thread);
	vm_push_caller(vm, t->program, t->IP);
	t->jmp_ofs=t->IP+data;
}

void _VM_CALL vm_op_lcall_Label(vm_t vm, word_t data) {
	thread_t t=node_value(thread_t,vm->current_thread);
	vm_data_type_t a;
	word_t b;
	vm_peek_data(vm,0,&a,&b);
	vm_push_caller(vm, t->program, t->IP);
	t->jmp_seg=(program_t)b;
	/* FIXME : can resolve_label() set up correct data value ? */
	t->jmp_ofs=data;
}


void _VM_CALL vm_op_enter_Int(vm_t vm, word_t size) {
	thread_t t=node_value(thread_t,vm->current_thread);
	gstack_grow(&t->locals_stack,size);
}

void _VM_CALL vm_op_leave_Int(vm_t vm, word_t size) {
	thread_t t=node_value(thread_t,vm->current_thread);
	gstack_shrink(&t->locals_stack,size);
}




void _VM_CALL vm_op_getmem_Int(vm_t vm, int n) {
	thread_t t=node_value(thread_t,vm->current_thread);
	vm_data_t var;
	if(n<0) {
		vm_data_t local = _gpeek(&t->locals_stack,1+n);	/* -1 becomes 0 */
		vm_push_data(vm,local->type,local->data);
	} else {
		var = (vm_data_t ) (t->program->data.data+(n<<1));
		vm_push_data(vm,var->type,var->data);
	}
}




void _VM_CALL vm_op_setmem_Int(vm_t vm, int n) {
	thread_t t=node_value(thread_t,vm->current_thread);
	vm_data_t top = _vm_pop(vm);
	vm_data_t var=NULL;
	if(n<0) {
		assert(t->locals_stack.sp>=-1-n);
		var = gpeek( vm_data_t , &t->locals_stack, 1+n );
	} else {
		n<<=1;
		/*printf("setmem at %lu of %lu/%lu\n",n,t->program->data.size,t->program->data.reserved);*/
		assert(t->program->data.reserved>n);
		if(n>t->program->data.size) {
			t->program->data.size = n+2;
		}
		var = (vm_data_t ) (t->program->data.data+n);
	}
	if(var->type==DataObject) {
		vm_obj_deref(vm,(void*)var->data);
	}
	var->type=top->type;
	var->data=top->data;
	if(top->type==DataObject) {
		vm_obj_ref(vm,(void*)top->data);
	}
}


void _VM_CALL vm_op_setmem(vm_t vm, int n) {
	vm_data_t top = _vm_pop(vm);
	if(top->type!=DataInt) {
		return;
	}
	vm_op_setmem_Int(vm, (int)top->data);
}


void _VM_CALL vm_op_getmem(vm_t vm, int n) {
	vm_data_t top = _vm_pop(vm);
	if(top->type!=DataInt) {
		return;
	}
	vm_op_getmem_Int(vm, (int)top->data);
}



void _VM_CALL vm_op_retval_Int(vm_t vm, word_t n) {
	program_t cs;
	word_t ip;
	vm_data_type_t dt;
	word_t d;
	thread_t t=node_value(thread_t,vm->current_thread);
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
	thread_t t=node_value(thread_t,vm->current_thread);
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


void _VM_CALL vm_op_strcmp(vm_t vm, word_t unused) {
	vm_data_t s2 = _vm_pop(vm);
	vm_data_t s1 = _vm_pop(vm);
	assert(s1->type==DataString);
	assert(s2->type==DataString);
	vm_push_data(vm,DataInt,strcmp((const char*)s1->data, (const char*)s2->data));
}



void _VM_CALL vm_op_toI(vm_t vm, word_t unused) {
	_IFC conv;
	vm_data_t d = _vm_pop(vm);
	switch(d->type) {
	case DataInt:
		vm_push_data(vm,DataInt,d->data);
		break;
	case DataFloat:
		conv.i=d->data;
		vm_push_data(vm,DataInt,f2i(conv.f));
		break;
	case DataString:
		/*printf("convert \"%s\" to int\n",(const char*)d->data);*/
		vm_push_data(vm,DataInt,atoi((const char*)d->data));
		break;
	default:
		printf("[VM:WRN] can't convert to int.\n");
		vm_push_data(vm,DataInt,0);
	};
}


void _VM_CALL vm_op_toF(vm_t vm, word_t unused) {
	_IFC conv;
	vm_data_t d = _vm_pop(vm);
	switch(d->type) {
	case DataInt:
		vm_push_data(vm,DataFloat,i2f(d->data));
		break;
	case DataFloat:
		vm_push_data(vm,DataFloat,d->data);
		break;
	case DataString:
		conv.f = atof((const char*)d->data);
		vm_push_data(vm,DataFloat,conv.i);
		break;
	default:
		vm_push_data(vm,DataFloat,0);
	};
}


void _VM_CALL vm_op_toS(vm_t vm, word_t unused) {
	static char buf[40];
	vm_data_t d = _vm_pop(vm);
	_IFC conv;
	char* str;
	switch(d->type) {
	case DataInt:
		sprintf(buf,"%li",(long)d->data);
		str=vm_string_new(buf);
		vm_push_data(vm,DataString,(word_t)str);
		break;
	case DataFloat:
		conv.i=d->data;
		sprintf(buf,"%f",conv.f);
		str=vm_string_new(buf);
		vm_push_data(vm,DataString,(word_t)str);
		break;
	case DataString:
		vm_push_data(vm,DataString,d->data);
		break;
	default:
		vm_push_data(vm,DataString,0);
	};
}



void _VM_CALL vm_op_newThread_Label(vm_t vm, word_t rel_ofs) {
	vm_data_t d = _vm_pop(vm);
	thread_t t=node_value(thread_t,vm->current_thread);
	word_t ofs = t->IP+rel_ofs;
	assert(d->type==DataInt);
	t = vm_add_thread(vm,t->program, ofs, d->data);
	vm_push_data(vm,DataInt,(word_t)t);
}

void _VM_CALL vm_op_getPid(vm_t vm, word_t unused) {
	vm_push_data(vm,DataInt,(word_t) vm->current_thread->value);
}


void _VM_CALL vm_op_newMtx(vm_t vm, word_t unused) {
	word_t handle = (word_t) vm_mutex_new();
	/*printf("push new mutex %lx\n",handle);*/
	vm_push_data(vm, DataObject, handle);
}


void _VM_CALL vm_op_lockMtx_Int(vm_t vm, long memcell) {
	vm_data_t d;
	thread_t t = node_value(thread_t,vm->current_thread);
	mutex_t m;
	vm_op_getmem_Int(vm,memcell);
	d = _vm_pop(vm);
	assert(d->type==DataObject);
	m = (mutex_t)d->data;
	assert_ptr_is_obj(m);
	mutex_lock(vm,m,t);
}


void _VM_CALL vm_op_unlockMtx_Int(vm_t vm, long memcell) {
	vm_data_t d;
	thread_t t = node_value(thread_t,vm->current_thread);
	mutex_t m;
	vm_op_getmem_Int(vm,memcell);
	d = _vm_pop(vm);
	assert(d->type==DataObject);
	m = (mutex_t)d->data;
	assert_ptr_is_obj(m);
	mutex_unlock(vm,m,t);
}



void _VM_CALL vm_op_lockMtx(vm_t vm, word_t unused) {
	vm_data_t d;
	thread_t t = node_value(thread_t,vm->current_thread);
	if(!t->pending_lock) {
		d = _vm_pop(vm);
		assert(d->type==DataObject);
		t->pending_lock = (mutex_t)d->data;
		assert_ptr_is_obj(t->pending_lock);
	}
	if(mutex_lock(vm,t->pending_lock,t)) {
		t->pending_lock=NULL;
	}
}


void _VM_CALL vm_op_unlockMtx(vm_t vm, word_t unused) {
	vm_data_t d;
	thread_t t = node_value(thread_t,vm->current_thread);
	mutex_t m;
	d = _vm_pop(vm);
	assert(d->type==DataObject);
	m = (mutex_t)d->data;
	assert_ptr_is_obj(m);
	mutex_unlock(vm,m,t);
}


void _VM_CALL vm_op_joinThread(vm_t vm, word_t unused) {
	vm_data_t d;
	thread_t t = node_value(thread_t,vm->current_thread),joinee;
	if(!t->pending_lock) {
		d=_vm_pop(vm);
		assert(d->type==DataInt);
		joinee = (thread_t)d->data;
		assert(((thread_t)joinee->sched_data.value) == joinee /* check that it's really a thread */);
		t->pending_lock = &joinee->join_mutex;
	}
	if(mutex_lock(vm,t->pending_lock,t)) {
		t->pending_lock=NULL;
	}
}


void _VM_CALL vm_op_killThread(vm_t vm, word_t unused) {
	vm_data_t d = _vm_pop(vm);
	thread_t victim = (thread_t)d->data;
	assert(d->type==DataInt);
	assert(((thread_t)victim->sched_data.value) == victim /* check that it's really a thread */);
	vm_kill_thread(vm,victim);
	/*thread_set_state(vm,victim,ThreadDying);*/
}





