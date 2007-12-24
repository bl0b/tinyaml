#include <stdlib.h>
#include <string.h>
#include "_impl.h"
#include "stack.h"

generic_stack_t new_gstack(word_t token_size) {
	generic_stack_t ret = (generic_stack_t) malloc(sizeof(struct _generic_stack_t));
	gstack_init(ret, token_size);
	return ret;
}

void gstack_init(generic_stack_t ret, word_t token_size) {
	ret->sz=0;
	ret->stack=NULL;
	ret->sp=(word_t)-1;
	ret->token_size=token_size;
	ret->tok_sp=(word_t)(-token_size);
	assert(ret->token_size<1024);
}

void gpush(generic_stack_t s, void* w) {
	s->sp += 1;
	s->tok_sp += s->token_size;
	if(s->sz <= s->tok_sp) {
		s->sz+=1024;
		s->stack = (word_t*) realloc(s->stack, s->sz*s->token_size);
	}

	memmove(s->stack+s->tok_sp,w,s->token_size);
	//s->stack[s->sp] = w;
}

void* _gpop(generic_stack_t s) {
	void* ret = s->stack+s->sp;
	assert(s->sp!=(word_t)-1);
	s->sp -= 1;
	s->tok_sp -= s->token_size;
	return ret;
}

void* _gpeek(generic_stack_t s) {
	assert(s->sp!=(word_t)-1);
	return s->stack+s->tok_sp;
}

void free_gstack(generic_stack_t s) {
//	printf("free_stack\n");
	if(s->stack) {
		free(s->stack);
	}
	free(s);
}


