#include "_impl.h"
#include "vm.h"
#include "thread.h"

#include <stdio.h>

thread_t thread_new(word_t prio, program_t p, word_t ip) {
	thread_t ret = (thread_t)malloc(sizeof(struct _thread_t));
	printf("NEW THREAD %p\n",ret);
	gstack_init(&ret->data_stack,sizeof(word_t));
	gstack_init(&ret->call_stack,sizeof(struct _call_stack_entry_t));
	gstack_init(&ret->catch_stack,sizeof(struct _call_stack_entry_t));
	ret->program = p;
	ret->IP = ip;
	ret->prio = prio;
	ret->state = ThreadReady;
	ret->remaining = 0;
	printf("\tPROGRAM %p\n",ret->program);
	return ret;
}

void thread_delete(thread_t t) {
}

