#ifndef _STACK_H_
#define _STACK_H_

#include <sys/types.h>

typedef struct _stack_t* stack_t;

stack_t stack_new();
void stack_delete(stack_t);
void stack_push(stack_t,void*);
void* stack_pop(stack_t);
void* stack_peek(stack_t);
size_t stack_size(stack_t);

#endif

