#include "vm.h"
#include "fastmath.h"
#include "timer.h"
#include "_impl.h"
#include "object.h"

static int RTC_is_init=0;

struct _generic_stack_t rtc_pending_threads;

void _VM_CALL vm_op_RTC_init(vm_t vm, word_t unused) {
	if(RTC_is_init) return;
	timer_init();
	gstack_init(&rtc_pending_threads,sizeof(thread_t));
	RTC_is_init=1;
}

void _VM_CALL vm_op_RTC_term(vm_t vm, word_t unused) {
	if(!RTC_is_init) return;
	timer_terminate();
	RTC_is_init=0;
}




void _VM_CALL vm_op_RTC_start(vm_t vm, word_t unused) {
	timer_start();
}

void _VM_CALL vm_op_RTC_stop(vm_t vm, word_t unused) {
	timer_stop();
}



void _VM_CALL vm_op_RTC_getDate(vm_t vm, word_t unused) {
	vm_push_data(vm, DataFloat, timer_get_seconds());
}




void _VM_CALL vm_op_RTC_getBeat(vm_t vm, word_t unused) {
	vm_push_data(vm, DataFloat, timer_get_date());
}


void _VM_CALL vm_op_RTC_setBeat_Float(vm_t vm, float date) {
	timer_set_date(date);
}

void _VM_CALL vm_op_RTC_setBeat(vm_t vm, word_t unused) {
	_IFC conv;
	vm_data_t d = _vm_pop(vm);
	assert(d->type==DataFloat);
	conv.i=d->data;
	timer_set_date(conv.f);
}




void _VM_CALL vm_op_RTC_getTempo(vm_t vm, word_t unused) {
	vm_push_data(vm, DataFloat, timer_get_tempo());
}


void _VM_CALL vm_op_RTC_setTempo_Float(vm_t vm, float tempo) {
	timer_set_tempo(tempo);
}

void _VM_CALL vm_op_RTC_setTempo(vm_t vm, word_t unused) {
	_IFC conv;
	vm_data_t d = _vm_pop(vm);
	assert(d->type==DataFloat);
	conv.i=d->data;
	timer_set_tempo(conv.f);
}



void _VM_CALL vm_op_RTC_getRes(vm_t vm, word_t unused) {
	vm_push_data(vm, DataFloat, timer_get_resolution());
}


void _VM_CALL vm_op_RTC_setRes_Float(vm_t vm, float resolution) {
	timer_set_resolution(resolution);
}

void _VM_CALL vm_op_RTC_setRes(vm_t vm, word_t unused) {
	_IFC conv;
	vm_data_t d = _vm_pop(vm);
	assert(d->type==DataFloat);
	conv.i=d->data;
	timer_set_resolution(conv.f);
}





void* resume_thread(float date, thread_t t) {
	/*printf("RESUME THREAD %p\n",t);*/
	thread_set_state(_glob_vm,t,ThreadReady);
	return NULL;
}



void _VM_CALL vm_op_RTC_wait(vm_t vm, word_t unused) {
	_IFC conv;
	vm_data_t d = _vm_pop(vm);
	assert(d->type==DataFloat);
	conv.i=d->data;
	/*printf("SUSPEND THREAD %p\n",vm->current_thread);*/
	timer_scheduleTask(conv.f,(void*(*)(float,void*))resume_thread,vm->current_thread);
	thread_set_state(vm,vm->current_thread,ThreadBlocked);
}



#define TASK_FIFO_SZ 4096
#define TASK_FIFO_MASK (TASK_FIFO_SZ-1)

vm_dyn_func_t fifo_f[4096];
float fifo_d[4096];
word_t rd=0,wr=0;

void fifo_wr(float d,vm_dyn_func_t df) {
	thread_t t;
	fifo_f[wr]=df;
	fifo_d[wr]=d;
	while(gstack_size(&rtc_pending_threads)) {
		t = *(thread_t*)_gpop(&rtc_pending_threads);
		/*printf("task arrived ! resuming thread %p\n",t);*/
		t->IP-=2;
		thread_set_state(_glob_vm,t,ThreadReady);
	}
	wr=(wr+1)&TASK_FIFO_MASK;
}

int fifo_rd(vm_dyn_func_t*df,float*d) {
	if(rd==wr) {
		return 0;
	}
	*df = fifo_f[rd];
	*d = fifo_d[rd];
	rd=(rd+1)&TASK_FIFO_MASK;
	return 1;
}


void* sched_task(float date, vm_dyn_func_t df) {
	fifo_wr(date,df);
	return NULL;
}


void _VM_CALL vm_op_RTC_sched(vm_t vm, word_t unused) {
	_IFC conv;
	vm_data_t d = _vm_pop(vm);
	vm_data_t f = _vm_pop(vm);
	assert(d->type==DataFloat);
	conv.i=d->data;
	assert(f->type==DataObjFun);
	vm_obj_ref_ptr(vm,(void*)f->data);
	timer_scheduleTask(conv.f,sched_task,f->data);
}




void _VM_CALL vm_op__RTC_nextTask(vm_t vm, word_t unused) {
	vm_dyn_func_t df;
	float d;
	if(fifo_rd(&df,&d)) {
		vm_push_data(vm,DataFloat,d);
		vm_push_data(vm,DataObjFun,df);
		vm_obj_deref_ptr(vm,df);
	} else {
		/*printf("blocking thread %p until a task arrives\n",vm->current_thread);*/
		gpush(&rtc_pending_threads,&vm->current_thread);
		thread_set_state(vm,vm->current_thread,ThreadBlocked);
	}
}



