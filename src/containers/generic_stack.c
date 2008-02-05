/* TinyaML
 * Copyright (C) 2007 Damien Leroux
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
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

void gstack_deinit(generic_stack_t s, void (*callback)(void*)) {
	if(callback) {
		while(s->sp!=-1) {
			callback(_gpop(s));
		}
	}
	if(s->stack) {
		free(s->stack);
	}
}


word_t gstack_size(generic_stack_t s) {
/*	vm_printf("stack state : %lu tokens, (size %lu, token_size %lu)\n",1+s->sp,s->sz,s->token_size);
*/	return s->sp+1;
}

/* this one will be used only with locals stack */
void gstack_grow(generic_stack_t s, word_t count) {
	assert(count<16384);
	s->sp+=count;
	s->tok_sp += s->token_size * count;
	if(s->sz <= s->tok_sp) {
		s->stack = (word_t*) realloc(s->stack, (s->sz+16384)*s->token_size);
		memset(s->stack+s->sz*s->token_size, 0, 16384*s->token_size);
		s->sz+=16384;
	}
}

/* this one will be used only with locals stack */
void gstack_shrink(generic_stack_t s, word_t count) {
	assert(((long) s->sp) >= ((long) count-1));
	s->sp-=count;
	s->tok_sp -= s->token_size * count;
}

void gpush(generic_stack_t s, void* w) {
/*	vm_printf("gpush\n");
*/	s->sp += 1;
	s->tok_sp += s->token_size;
/*	for(i=0;i<s->token_size;i+=sizeof(word_t)) {
		vm_printf("%8.8lX ",*(((word_t*)w)+i));
	}
	vm_printf("\n");
*/	if(s->sz <= s->tok_sp) {
		s->sz+=1024;
		s->stack = realloc(s->stack, s->sz*s->token_size);
		memset(s->stack+s->tok_sp+s->token_size,0,(s->sz-s->sp-1)*s->token_size);
	}

	memmove(s->stack+s->tok_sp,w,s->token_size);
/*	vm_printf("stack state : %lu tokens, (size %lu, token_size %lu)\n",1+s->sp,s->sz,s->token_size);
	//s->stack[s->sp] = w;
*/}

void* _gpop(generic_stack_t s) {
	void* ret = s->stack+s->tok_sp;
/*	vm_printf("gpop\n");
*/	assert(s->sp!=(word_t)-1);
	s->sp -= 1;
	s->tok_sp -= s->token_size;
/*	vm_printf("stack state : %lu tokens, (size %lu, token_size %lu)\n",1+s->sp,s->sz,s->token_size);
*/	return ret;
}

void* _gpeek(generic_stack_t s, int rel_ofs) {
	assert(s->sp>=(word_t)-rel_ofs);
/*	vm_printf("gpeek\n");
	vm_printf("stack state : %lu tokens, (size %lu, token_size %lu)\n",1+s->sp,s->sz,s->token_size);
*/	return s->stack+s->tok_sp+(rel_ofs*s->token_size);
}

void free_gstack(generic_stack_t s) {
/*	vm_printf("free_stack\n");
*/	if(s->stack) {
		free(s->stack);
	}
	free(s);
}


