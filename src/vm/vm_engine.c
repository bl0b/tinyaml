#include "vm.h"
#include "_impl.h"
#include "vm_engine.h"
#include "object.h"
#include <pthread.h>
#include <signal.h>
#include <errno.h>

void _VM_CALL e_stub(vm_engine_t e) {}

void _VM_CALL e_run(vm_engine_t e, program_t p, word_t ip, word_t prio) {
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
		vm_push_caller(e->vm,t->program,t->IP,0);
		s_sz = t->call_stack.sp-1;
		t->program=p;
		t->IP=ip;
		while(e->vm->threads_count&&t->call_stack.sp!=s_sz) {
			vm_schedule_cycle(e->vm);
		}
		if(e->vm->threads_count) {
			/*printf("done with sub thread\n");*/
			/* restore old state */
			t->program=program;
			t->IP=IP;
			t->jmp_seg=jmp_seg;
			t->jmp_ofs=jmp_ofs;
		}
	} else {
		vm_add_thread(e->vm,p,ip,prio,0);
		while(e->vm->threads_count) {
			vm_schedule_cycle(e->vm);
		}
	}
}





struct _thread_engine_t {
	struct _vm_engine_t _;
	pthread_t thread;
	pthread_mutex_t mutex;
	pthread_mutex_t fg_mutex;
	volatile int is_running;
};

void _VM_CALL th_cli_lock(struct _thread_engine_t* e) {
	if(pthread_self()==e->thread) {
		return;
	}
	pthread_mutex_lock(&e->mutex);
}

void _VM_CALL th_cli_unlock(struct _thread_engine_t* e) {
	if(pthread_self()==e->thread) {
		return;
	}
	pthread_mutex_unlock(&e->mutex);
}

void _VM_CALL th_vm_lock(struct _thread_engine_t* e) {
	/*if(pthread_self()!=e->thread) {*/
		/*return;*/
	/*}*/
	pthread_mutex_lock(&e->mutex);
}

void _VM_CALL th_vm_unlock(struct _thread_engine_t* e) {
	/*if(pthread_self()!=e->thread) {*/
		/*return;*/
	/*}*/
	pthread_mutex_unlock(&e->mutex);
}

void* th_th_run(struct _thread_engine_t* e) {
	/*printf("ENGINE START\n");*/
	e->is_running=1;
	while(e->is_running&&e->_.vm->scheduler!=SchedulerIdle) {
		vm_schedule_cycle(e->_.vm);
	}
	/*printf("ENGINE STOP\n");*/
	return NULL;
}

void _VM_CALL th_init(struct _thread_engine_t* e) {
	e->mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	pthread_attr_t pa;
	pthread_attr_init(&pa);
	/*pthread_attr_setdetachstate(&pa,PTHREAD_CREATE_DETACHED);*/
	pthread_attr_setdetachstate(&pa,PTHREAD_CREATE_JOINABLE);
	pthread_attr_setschedpolicy(&pa,SCHED_RR);
	pthread_create(&e->thread,&pa, (void*(*)(void*))th_th_run,e);
}

void _VM_CALL th_deinit(struct _thread_engine_t* e) {
	/*e->is_running=0;*/
	/* handmade join */
	/*while(e->is_running!=-1);*/
	if(e->thread) {
		void*ret;
		e->is_running=0;
		if(e->thread!=pthread_self()) {
			pthread_join(e->thread,&ret);
			e->thread=0;
		}
	}
	pthread_mutex_unlock(&e->mutex);
	/*pthread_mutex_unlock(&e->fg_mutex);*/
}

void _VM_CALL th_kill(struct _thread_engine_t* e) {
	e->is_running=0;
	/*while(!e->is_running);*/
	if(e->thread!=pthread_self()) {
		void*ret;
		/*pthread_kill(e->thread,SIGHUP);*/
		pthread_join(e->thread,&ret);
		e->thread=0;
	} else {
		e->thread=0;
		pthread_exit(NULL);
	}
}

void _VM_CALL th_fg_start_cb(struct _thread_engine_t* e) {
	/*fputs("th_fg_start_cb\n",stdout);*/
	pthread_mutex_lock(&e->fg_mutex);
}

void _VM_CALL th_fg_done_cb(struct _thread_engine_t* e) {
	/*fputs("th_fg_done_cb\n",stdout);*/
	pthread_mutex_unlock(&e->fg_mutex);
}

void _VM_CALL th_run_fg(struct _thread_engine_t* e, program_t p, word_t ip, word_t prio) {
	volatile thread_t t;
	t = vm_add_thread(e->_.vm,p,ip,prio,1);
	vm_obj_ref(e->_.vm,t);
	/*printf("\n\n### STARTING FG THREAD ###\n\n");*/
	/*while(t->state!=ThreadZombie);*/
	/*printf("\n\n### FG THREAD ZOMBIFIED ###\n\n");*/
	while(pthread_mutex_trylock(&e->fg_mutex)!=EBUSY) {
		/*fputc('.',stdout); fflush(stdout);*/
		pthread_mutex_unlock(&e->fg_mutex);
	}
	/*fputc('\n',stdout);*/
	pthread_mutex_lock(&e->fg_mutex);
	pthread_mutex_unlock(&e->fg_mutex);
	vm_obj_deref(e->_.vm,t);
}

void _VM_CALL th_run_bg(struct _thread_engine_t* e, program_t p, word_t ip, word_t prio) {
		vm_add_thread(e->_.vm,p,ip,prio,0);
}


#define decl_engine(_i,_d,_rf,_rb,_k) 

/* synchronous engine. init lock unlock and deinit are pointless, and kill can't happen. */
const vm_engine_t stub_engine = (struct _vm_engine_t[])
{{
	e_stub,
	e_stub,
	e_run,
	e_stub,
	e_stub,
	e_run,
	e_stub,
	e_stub,
	e_stub,
	e_stub,
	e_stub,
	NULL
}};

const vm_engine_t thread_engine = (vm_engine_t) (struct _thread_engine_t[])
{{{
	(vm_engine_func_t)th_init,
	(vm_engine_func_t)th_deinit,
	(void (*_VM_CALL)(vm_engine_t, program_t, word_t, word_t))th_run_fg,
	(vm_engine_func_t)th_fg_start_cb,
	(vm_engine_func_t)th_fg_done_cb,
	(void (*_VM_CALL)(vm_engine_t, program_t, word_t, word_t))th_run_bg,
	(vm_engine_func_t)th_kill,
	(vm_engine_func_t)th_cli_lock,
	(vm_engine_func_t)th_cli_unlock,
	(vm_engine_func_t)th_vm_lock,
	(vm_engine_func_t)th_vm_unlock,
	NULL
},
	(pthread_t)0,
	PTHREAD_MUTEX_INITIALIZER,
	PTHREAD_MUTEX_INITIALIZER,
	0
}};


