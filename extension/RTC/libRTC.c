#include "vm.h"
#include "fastmath.h"
#include "thread.h"
#include "timer.h"
#include "_impl.h"
#include "object.h"
#include "rtc_alloc.h"

static long RTC_is_init=0;

/*struct _generic_stack_t rtc_pending_threads;*/

vm_blocker_t rtc_nextTask;

void _VM_CALL vm_op_RTC_init(vm_t vm, word_t unused) {
	if(RTC_is_init) return;
	init_alloc();
	timer_init();
	/*gstack_init(&rtc_pending_threads,sizeof(thread_t));*/
	rtc_nextTask = blocker_new();
	RTC_is_init=1;
}

void _VM_CALL vm_op_RTC_term(vm_t vm, word_t unused) {
	if(!RTC_is_init) return;
	timer_terminate();
	term_alloc();
	/*gstack_deinit(&rtc_pending_threads,NULL);*/
	blocker_free(rtc_nextTask);
	RTC_is_init=0;
}




void _VM_CALL vm_op_RTC_start(vm_t vm, word_t unused) {
	timer_start();
}

void _VM_CALL vm_op_RTC_stop(vm_t vm, word_t unused) {
	timer_stop();
}



void _VM_CALL vm_op_RTC_getDate(vm_t vm, word_t unused) {
	_IFC conv;
	conv.f=timer_get_seconds();
	vm_push_data(vm, DataFloat, conv.i);
}




void _VM_CALL vm_op_RTC_getBeat(vm_t vm, word_t unused) {
	_IFC conv;
	conv.f=timer_get_date();
	vm_push_data(vm, DataFloat, conv.i);
}


void _VM_CALL vm_op_RTC_setBeat_Float(vm_t vm, float_t date) {
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
	_IFC conv;
	conv.f = timer_get_tempo();
	vm_push_data(vm, DataFloat, conv.i);
}


void _VM_CALL vm_op_RTC_setTempo_Float(vm_t vm, float_t tempo) {
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
	_IFC conv;
	conv.f = timer_get_resolution();
	vm_push_data(vm, DataFloat, conv.i);
}


void _VM_CALL vm_op_RTC_setRes_Float(vm_t vm, float_t resolution) {
	timer_set_resolution(resolution);
}

void _VM_CALL vm_op_RTC_setRes(vm_t vm, word_t unused) {
	_IFC conv;
	vm_data_t d = _vm_pop(vm);
	assert(d->type==DataFloat);
	conv.i=d->data;
	timer_set_resolution(conv.f);
}





void* resume_thread(float_t date, thread_t t) {
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
	timer_scheduleTask(conv.f,(void*(*)(float_t,void*))resume_thread,vm->current_thread);
	thread_set_state(vm,vm->current_thread,ThreadBlocked);
}



#define TASK_FIFO_SZ 4096
#define TASK_FIFO_MASK (TASK_FIFO_SZ-1)

vm_dyn_func_t fifo_f[4096];
float_t fifo_d[4096];
word_t rd=0,wr=0;

void fifo_wr(float_t d,vm_dyn_func_t df) {
	thread_t t;
	fifo_f[wr]=df;
	fifo_d[wr]=d;
	/*while(gstack_size(&rtc_pending_threads)) {*/
		/*t = *(thread_t*)_gpop(&rtc_pending_threads);*/
		/*printf("task arrived ! resuming thread %p\n",t);*/
		/*t->IP-=2;*/
		/*thread_set_state(_glob_vm,t,ThreadReady);*/
	/*}*/
	blocker_resume(_glob_vm,rtc_nextTask);
	wr=(wr+1)&TASK_FIFO_MASK;
}

long fifo_rd(vm_dyn_func_t*df,float_t*d) {
	if(rd==wr) {
		return 0;
	}
	*df = fifo_f[rd];
	*d = fifo_d[rd];
	rd=(rd+1)&TASK_FIFO_MASK;
	return 1;
}


void* sched_task(float_t date, vm_dyn_func_t df) {
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
	_IFC conv;
	if(fifo_rd(&df,&conv.f)) {
		vm_push_data(vm,DataFloat,conv.i);
		/*vm_push_data(vm,DataInt,1);*/
		vm_push_data(vm,DataObjFun,df);
		vm_obj_deref_ptr(vm,df);
	} else {
		/*printf("blocking thread %p until a task arrives\n",vm->current_thread);*/
		/*gpush(&rtc_pending_threads,&vm->current_thread);*/
		/*thread_set_state(vm,vm->current_thread,ThreadBlocked);*/
		blocker_suspend(vm,rtc_nextTask,vm->current_thread);
	}
}



