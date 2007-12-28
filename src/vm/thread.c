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

#include "_impl.h"
#include "vm.h"
#include "thread.h"

#include <stdio.h>

thread_t thread_new(word_t prio, program_t p, word_t ip) {
	thread_t ret = (thread_t)malloc(sizeof(struct _thread_t));
	/*printf("NEW THREAD %p\n",ret);*/
	gstack_init(&ret->locals_stack,sizeof(struct _data_stack_entry_t));
	gstack_init(&ret->data_stack,sizeof(struct _data_stack_entry_t));
	gstack_init(&ret->call_stack,sizeof(struct _call_stack_entry_t));
	gstack_init(&ret->catch_stack,sizeof(struct _call_stack_entry_t));
	ret->program = p;
	ret->jmp_seg = p;
	ret->jmp_ofs = 0;
	ret->IP = ip;
	ret->prio = prio;
	ret->state = ThreadReady;
	ret->remaining = 0;
	/*printf("\tPROGRAM %p\n",ret->program);*/
	return ret;
}

void thread_delete(thread_t t) {
	/*printf("\tdel thread\n");*/
	gstack_deinit(&t->locals_stack,NULL);
	gstack_deinit(&t->data_stack,NULL);
	gstack_deinit(&t->call_stack,NULL);
	gstack_deinit(&t->catch_stack,NULL);
	free(t);
}

