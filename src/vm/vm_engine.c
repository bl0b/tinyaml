#include "vm.h"
#include "fastmath.h"
#include "_impl.h"
#include "vm_engine.h"
#include "object.h"
#include "program.h"
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>


void _VM_CALL put_stdout(const char*s) {
	fputs(s,stdout);
}

void _VM_CALL put_stderr(const char*s) {
	fputs(s,stderr);
}


void lookup_label_and_ofs(program_t cs, word_t ip, const char** label, word_t* ofs) {
	struct _label_tab_t* l = &cs->labels;
	word_t i=1;
	if(l->offsets.size==0) {
		*label = NULL;
		*ofs = ip;
		return;
	}
	ip+=2;
	while(i<(l->offsets.size-1) && ip>=l->offsets.data[i+1]) { i+=1; }
	if(i>=l->offsets.size) {
		*ofs = ip;
		*label = "";
	} else {
		*ofs = ip - l->offsets.data[i];
		*label = (const char*)l->labels.by_index.data[i];
	}
}


static void data_stack_renderer(vm_data_t d) {
	_IFC conv;
	/*int i;*/
	/*const unsigned char*s;*/
	switch(d->type) {
	case DataInt:
		vm_printerrf("Int     %li",d->data);
		break;
	case DataFloat:
		conv.i = d->data;
		vm_printerrf("Float   %f",conv.f);
		break;
	case DataString:
		vm_printerrf("String  \"%s\"",(const char*)d->data);
		break;
	case DataObjStr:
		vm_printerrf("ObjStr  \"%s\"",(const char*)d->data);
		break;
	case DataObjSymTab:
		vm_printerrf("SymTab  %p",(void*)d->data);
		break;
	case DataObjMutex:
		vm_printerrf("Mutex  %p",(void*)d->data);
		break;
	case DataObjThread:
		vm_printerrf("Thread  %p",(void*)d->data);
		break;
	case DataObjArray:
		vm_printerrf("Array  %p",(void*)d->data);
		break;
	case DataObjEnv:
		vm_printerrf("Map  %p",(void*)d->data);
		break;
	case DataObjStack:
		vm_printerrf("Stack  %p",(void*)d->data);
		break;
	case DataObjFun:
		vm_printerrf("Function  %p",(void*)d->data);
		break;
	case DataObjVObj:
		vm_printerrf("V-Obj  %p",(void*)d->data);
		break;
	case DataManagedObjectFlag:
		vm_printerrf("Undefined Object ! %p",(void*)d->data);
		break;
	default:
		vm_printerrf( "Unknown (%u, 0x%lX)",d->type,d->data);
	};
}

static void closure_stack_renderer(dynarray_t* da) {
	vm_data_t tab = (vm_data_t) (*da)->data;
	word_t i;
	vm_printerrf("[%lu]",(*da)->size);
	for(i=0;i<(*da)->size;i+=1) {
		vm_printerrf("\n\t\t%li : ",i);
		data_stack_renderer(&(tab[i]));
	}
}

static void call_stack_renderer(struct _call_stack_entry_t* cse) {
	word_t ofs;
	const char*label;
	lookup_label_and_ofs(cse->cs,cse->ip,&label,&ofs);
	if(label) {
		vm_printerrf("%s:%s+%lu", program_get_source(cse->cs), label, ofs);
	} else {
		vm_printerrf("%s:%lu", program_get_source(cse->cs), cse->ip);
	}
}


static void render_stack(generic_stack_t s, word_t sz, const char*prefix, void(*renderer)(void*)) {
	long counter = 0;
	long stop = -sz;
	while(counter>stop) {
		vm_printerrf("%s%li : ",prefix,-counter);
		renderer(s->stack+s->tok_sp+(counter*s->token_size));
		/*renderer(_gpeek(s,counter));*/
		fputc('\n',stderr);
		counter-=1;
	}
}



void _VM_CALL thread_failed(vm_t vm, thread_t t) {
	word_t ofs;
	const char*label;
	const char* disasm = program_disassemble(vm,t->program,t->IP);
	lookup_label_and_ofs(t->program,t->IP,&label,&ofs);
	vm_printerrf( "Thread :\t%p\n",t);
	if(label) {
		vm_printerrf("CS:IP : \t%s:%lXh (%s+%lXh) # %s\n", program_get_source(t->program), t->IP, label, ofs, disasm);
	} else {
		vm_printerrf("CS:IP : \t%s:%lXh # %s\n", program_get_source(t->program), t->IP, disasm);
	}
	free((char*)disasm);
	vm_printerrf("\nCall stack :\t[%lu]\n", gstack_size(&t->call_stack));
	render_stack(&t->call_stack, gstack_size(&t->call_stack), "\t", (void(*)(void*)) call_stack_renderer);
	vm_printerrf("\nClosure stack :\t[%lu]\n", gstack_size(&t->data_stack));
	render_stack(&t->closures_stack, gstack_size(&t->closures_stack), "\t", (void(*)(void*)) closure_stack_renderer);
	vm_printerrf("\nData stack :\t[%lu]\n", gstack_size(&t->data_stack));
	render_stack(&t->data_stack, t->data_sp_backup, "\t", (void(*)(void*)) data_stack_renderer);
	vm_printerrf("\nLocals stack :\t[%lu]\n", gstack_size(&t->locals_stack));
	render_stack(&t->locals_stack, gstack_size(&t->locals_stack), "\t", (void(*)(void*)) data_stack_renderer);
	vm_printerrf("\nCatch stack :\t[%lu]\n", gstack_size(&t->catch_stack));
}









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
		vm_push_caller(e->vm, t->program, t->IP, 0, NULL);
		s_sz = t->call_stack.sp-1;
		t->program=p;
		t->IP=ip;
		while(e->vm->threads_count&&t->call_stack.sp!=s_sz) {
			vm_schedule_cycle(e->vm);
		}
		if(e->vm->threads_count) {
			/*vm_printf("done with sub thread\n");*/
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
	/*vm_printf("ENGINE START\n");*/
	e->is_running=1;
	while(e->is_running&&e->_.vm->scheduler!=SchedulerIdle) {
		vm_schedule_cycle(e->_.vm);
	}
	/*vm_printf("ENGINE STOP\n");*/
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
	vm_obj_ref_ptr(e->_.vm,t);
	/*vm_printf("\n\n### STARTING FG THREAD ###\n\n");*/
	/*while(t->state!=ThreadZombie);*/
	/*vm_printf("\n\n### FG THREAD ZOMBIFIED ###\n\n");*/
	while(pthread_mutex_trylock(&e->fg_mutex)!=EBUSY) {
		/*fputc('.',stdout); fflush(stdout);*/
		pthread_mutex_unlock(&e->fg_mutex);
	}
	/*fputc('\n',stdout);*/
	pthread_mutex_lock(&e->fg_mutex);
	pthread_mutex_unlock(&e->fg_mutex);
	vm_obj_deref_ptr(e->_.vm,t);
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
	thread_failed,
	NULL,
	put_stdout,
	put_stderr,
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
	thread_failed,
	NULL,
	put_stdout,
	put_stderr,
	NULL
},
	(pthread_t)0,
	PTHREAD_MUTEX_INITIALIZER,
	PTHREAD_MUTEX_INITIALIZER,
	0
}};


