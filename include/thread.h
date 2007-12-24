
#ifndef _BML_THREAD_H_
#define _BML_THREAD_H_

thread_t thread_new(word_t prio, program_t p, word_t ip);
void thread_delete(thread_t);

void thread_start(vm_t, thread_t);
void thread_yield(vm_t, thread_t);
void thread_resume(vm_t, thread_t);
void thread_stop(vm_t, thread_t);




#endif

