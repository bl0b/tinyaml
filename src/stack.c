#include "obj.h"
#include "stack.h"
#include <stdlib.h>
#include <string.h>


/****************************************************
 * STACK
 ***************************************************/

struct _stack_t {
	size_t size;
	size_t capacity;
	void** stack;
};

stack_t stack_new() {
	stack_t ret = (stack_t)malloc(sizeof(struct _stack_t));
	memset(ret,0,sizeof(struct _stack_t));
	return ret;
}

void stack_delete(stack_t s) {
	if(s->stack) {
		free(s->stack);
	}
	free(s);
}

void stack_push(stack_t s, void* data) {
	if(s->size==s->capacity) {
		s->capacity += REALLOC_GRANUL;
		s->stack = (void**) realloc(s->stack, s->capacity*sizeof(void*));
	}
	s->stack[s->size] = data;
	s->size += 1;
}

void* stack_pop(stack_t s) {
	if(!s->size) {
		return NULL;
	}
	s->size -= 1;
	return s->stack[s->size];
}

void* stack_peek(stack_t s) {
	if(!s->size) {
		return NULL;
	}
	return s->stack[s->size-1];
}

size_t stack_size(stack_t s) {
	return s->size;
}



