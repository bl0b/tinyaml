#include "vm.h"
#include "_impl.h"
#include "vm_engine.h"
#include "object.h"
#include <pthread.h>

void e_init(vm_engine_t e) {}
void e_deinit(vm_engine_t e) {}
void e_kill(vm_engine_t e) {}
void e_run(vm_engine_t e, program_t p, word_t ip, word_t prio) {
	if(e->vm->current_thread) {
		thread_t t = e->vm->current_thread;
		/* save some thread state */
		program_t jmp_seg = t->jmp_seg;
		word_t jmp_ofs = t->jmp_ofs;
		word_t IP = t->IP;
		program_t program = t->program;
		/* hack new thread state */
		word_t s_sz;
		t->jmp_ofs=0;
		t->jmp_seg=p;
		vm_push_caller(e->vm,t->program,t->IP);
		s_sz = t->call_stack.sp-1;
		t->program=p;
		t->IP=ip;
		while(t->call_stack.sp!=s_sz) {
			vm_schedule_cycle(e->vm);
		}
		/*printf("done with sub thread\n");*/
		/* restore old state */
		t->program=program;
		t->IP=IP;
		t->jmp_seg=jmp_seg;
		t->jmp_ofs=jmp_ofs;
	} else {
		vm_add_thread(e->vm,p,ip,prio);
		while(e->vm->threads_count) {
			vm_schedule_cycle(e->vm);
		}
	}
}





struct _thread_engine_t {
	struct _vm_engine_t _;
	pthread_t thread;
	pthread_mutex_t mutex;
	int is_running;
};

void* th_th_run(struct _thread_engine_t* e) {
	printf("ENGINE START\n");
	while(e->_.vm->scheduler!=SchedulerIdle) {
		vm_schedule_cycle(e->_.vm);
	}
	printf("ENGINE STOP\n");
	return NULL;
}

void th_init(struct _thread_engine_t* e) {
	e->mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	pthread_attr_t pa;
	pthread_attr_init(&pa);
	pthread_attr_setdetachstate(&pa,PTHREAD_CREATE_DETACHED);
	pthread_attr_setschedpolicy(&pa,SCHED_RR);
	e->is_running=1;
	pthread_create(&e->thread,&pa, (void*(*)(void*))th_th_run,e);
}
void th_deinit(struct _thread_engine_t* e) {
	e->is_running=0;
	/*vm->scheduler = SchedulerIdle;*/
	/*while(!e->is_running);*/
}

void th_kill(struct _thread_engine_t* e) {
	/*e->is_running=0;*/
	/*while(!e->is_running);*/
}

void th_run_fg(struct _thread_engine_t* e, program_t p, word_t ip, word_t prio) {
	thread_t t = vm_add_thread(e->_.vm,p,ip,prio);
	vm_obj_ref(e->_.vm,t);
	/*printf("\n\n### STARTING FG THREAD ###\n\n");*/
	while(t->state!=ThreadZombie);
	/*printf("\n\n### FG THREAD ZOMBIFIED ###\n\n");*/
	vm_obj_deref(e->_.vm,t);
}

void th_run_bg(struct _thread_engine_t* e, program_t p, word_t ip, word_t prio) {
		vm_add_thread(e->_.vm,p,ip,prio);
}


#define decl_engine(_i,_d,_rf,_rb,_k) (struct _vm_engine_t[]){{ _i, _d, _rf, _rb, _k }}

const vm_engine_t stub_engine = decl_engine(e_init, e_deinit, e_run, e_run, e_kill);
const vm_engine_t thread_engine = (vm_engine_t) (struct _thread_engine_t[])
{{{
	(vm_engine_func_t)th_init,
	(vm_engine_func_t)th_deinit,
	(void (*)(vm_engine_t, program_t, word_t, word_t))th_run_fg,
	(void (*)(vm_engine_t, program_t, word_t, word_t))th_run_bg,
	(vm_engine_func_t)th_kill,
	NULL
},
	(pthread_t)0,
	PTHREAD_MUTEX_INITIALIZER,
	0
}};


