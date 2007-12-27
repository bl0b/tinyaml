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

#include "vm.h"
#include "_impl.h"
#include "text_seg.h"
#include "opcode_dict.h"
#include "program.h"

#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#include "bml_ops.h"

/* hidden feature in tinyap */
ast_node_t ast_unserialize(const char*);
/* hidden serialized ml grammar */
extern const char* ml_core_grammar;
extern const char* ml_core_lib;

vm_t vm_new() {
	vm_t ret = (vm_t)malloc(sizeof(struct _vm_t));
	tinyap_init();
	ret->parser = tinyap_new();
	tinyap_set_grammar_ast(ret->parser,ast_unserialize(ml_core_grammar));
	opcode_dict_init(&ret->opcodes);
	ret->threads_count=0;
	ret->current_thread=NULL;
	ret->timeslice=100;
	dlist_init(&ret->ready_threads);
	dlist_init(&ret->running_threads);
	dlist_init(&ret->yielded_threads);
	ret->engine=NULL;

	ret->dl_handle=NULL;

	ret->cycles=0;

	/* fill opcodes dictionary with basic library */
	vm_compile_buffer(ret, ml_core_lib);
	/* nops are hardcoded due to a limitation of tinyap */
	vm_add_opcode(ret,"nop",OpcodeNoArg, vm_op_nop);
	vm_add_opcode(ret,"nop",OpcodeArgString, vm_op_nop);
	vm_add_opcode(ret,"nop",OpcodeArgLabel, vm_op_nop);
	vm_add_opcode(ret,"nop",OpcodeArgOpcode, vm_op_nop);
	vm_add_opcode(ret,"nop",OpcodeArgInt, vm_op_nop);
	vm_add_opcode(ret,"nop",OpcodeArgFloat, vm_op_nop);

	return ret;
}

void vm_del(vm_t ret) {
	printf("delete vm\n");
	dlist_forward(&ret->ready_threads,thread_t,thread_delete);
	dlist_forward(&ret->running_threads,thread_t,thread_delete);
	dlist_forward(&ret->yielded_threads,thread_t,thread_delete);
	dlist_flush(&ret->ready_threads);
	dlist_flush(&ret->running_threads);
	dlist_flush(&ret->yielded_threads);
	tinyap_delete(ret->parser);
	opcode_dict_deinit(&ret->opcodes);
	free(ret);
}


int comp_prio(dlist_node_t a, dlist_node_t b) {
	return (int)(node_value(thread_t,b)->prio-node_value(thread_t,a)->prio);
}


vm_t vm_set_lib_file(vm_t vm, const char*fname) {
	vm->dl_handle = dlopen(fname, RTLD_LAZY);
	return vm;
}

vm_t vm_add_opcode(vm_t vm, const char*name, opcode_arg_t arg_type, opcode_stub_t stub) {
	opcode_dict_add(&vm->opcodes,arg_type,name,stub);
	return vm;
}

opcode_dict_t vm_get_dict(vm_t vm) {
	return &vm->opcodes;
}


#define PROG_FILE_MAGIC "BMLP"
#define ENDIAN_TEST 0x01000000


program_t vm_unserialize_program(vm_t vm, reader_t r) {
	program_t p=NULL;
	return p;
}

word_t opcode_arg_serialize(program_t p, opcode_arg_t arg_type, word_t arg) {
	switch(arg_type) {
	case OpcodeArgString:
		arg = text_seg_text_to_index(&p->strings,(const char*)arg);
		break;
	default:;
	};
	return arg;
}

vm_t vm_serialize_program(vm_t vm, program_t p, writer_t w) {
	int i;
	word_t op;
	word_t arg;
	/* write header */
	write_string(w,"BML_PRG");
	write_word(w,ENDIAN_TEST);
	/* write dict */
	opcode_dict_serialize(vm_get_dict(vm),w);
	/* write text segment */
	text_seg_serialize(&p->strings,w);
	/* write code segment */
	write_string(w,"CODE---");
	/* write code segment size */
	write_word(w,dynarray_size(&p->code));
	for(i=0;i<dynarray_size(&p->code);i+=2) {
		printf("%8.8lX %8.8lX   ",dynarray_get(&p->code,i),dynarray_get(&p->code,i+1));
		if(i%8==6) printf("\n");
	}
	printf("\n");
	/* write serialized word code */
	for(i=0;i<dynarray_size(&p->code);i+=2) {
		op = opcode_code_by_stub(vm_get_dict(vm), (opcode_stub_t)dynarray_get(&p->code,i));
		arg = opcode_arg_serialize(p, WC_GET_ARGTYPE(op), dynarray_get(&p->code,i+1));
		write_word(w,op);
		write_word(w,arg);
	}
	return vm;
}



program_t compile_wast(wast_t, vm_t);
void wa_del(wast_t w);

program_t vm_compile_file(vm_t vm, const char* fname) {
	program_t p=NULL;
	tinyap_set_source_file(vm->parser,fname);
	tinyap_parse(vm->parser);

	if(tinyap_parsed_ok(vm->parser)&&tinyap_get_output(vm->parser)) {
		wast_t wa = tinyap_make_wast( tinyap_list_get_element( tinyap_get_output(vm->parser), 0) );
		p = compile_wast(wa, vm);
		wa_del(wa);
	} else {
		fprintf(stderr,"parse error at %i:%i\n%s",tinyap_get_error_row(vm->parser),tinyap_get_error_col(vm->parser),tinyap_get_error(vm->parser));
	}
	return p;
}


program_t vm_compile_buffer(vm_t vm, const char* buffer) {
	program_t p=NULL;
	tinyap_set_source_buffer(vm->parser,buffer,strlen(buffer));
	tinyap_parse(vm->parser);

	if(tinyap_parsed_ok(vm->parser)&&tinyap_get_output(vm->parser)) {
		wast_t wa = tinyap_make_wast( tinyap_list_get_element( tinyap_get_output(vm->parser), 0) );
		p = compile_wast(wa, vm);
		wa_del(wa);
	} else {
		fprintf(stderr,tinyap_get_error(vm->parser));
	}
	return p;
}



vm_t vm_run_program(vm_t vm, program_t p, word_t prio) {
	vm->engine->_run(vm->engine,p, prio);
	return vm;
}


vm_t vm_add_thread(vm_t vm, program_t p, word_t ip, word_t prio) {
	thread_t t;
	dlist_node_t dn;
	if(!p) {
		return vm;
	}
	t = thread_new(prio,p,ip);
	/* FIXME : this should go into thread_new() */
	t->jmp_seg=NULL;
	t->jmp_ofs=0;
	vm->threads_count += 1;
	if(vm->threads_count==1) {
		vm->scheduler = SchedulerMonoThread;
	} else if(vm->threads_count==1) {
		vm->scheduler = SchedulerRoundRobin;
	}

	dn = (dlist_node_t) malloc(sizeof(struct _dlist_node_t));
	dn->value=(word_t)t;

	dlist_insert_sorted(&vm->ready_threads,dn,comp_prio);

	return vm;
}


thread_t vm_get_thread(vm_t vm, word_t index) {
	return NULL;
}

word_t vm_get_current_thread_index(vm_t vm) {
	return 0;
}

thread_t vm_get_current_thread(vm_t vm) {
	return node_value(thread_t,vm->current_thread);
}


vm_t vm_kill_thread(vm_t vm, thread_t t) {
	vm->threads_count -= 1;
	if(vm->threads_count==1) {
		vm->scheduler = SchedulerMonoThread;
	} else if(vm->threads_count==0) {
		vm->scheduler = SchedulerIdle;
	}

	return vm;
}


void vm_dump_data_stack(vm_t vm) {
	struct _data_stack_entry_t* e;
	generic_stack_t stack = &node_value(thread_t,vm->current_thread)->data_stack;
	int sz = gstack_size(stack);
	int i;

	printf("sz = %i\n",sz);
	for(i=0;i<sz;i+=1) {
		e = (struct _data_stack_entry_t*) gpeek(struct _data_stack_entry_t*,stack,-i);
		printf("#%i : %4.4X %8.8lX\n",i,e->type, e->data);
	}
}

vm_t vm_push_data(vm_t vm, vm_data_type_t type, word_t value) {
	struct _data_stack_entry_t e;
	generic_stack_t stack = &node_value(thread_t,vm->current_thread)->data_stack;
	/*printf("vm push data : %lu %lu\n",type,value);*/
	e.type = type;
	e.data = value;
	gpush( stack, &e );
	/*vm_dump_data_stack(vm);*/
	return vm;
}

vm_t vm_push_caller(vm_t vm, program_t seg, word_t ofs) {
	struct _call_stack_entry_t e;
	generic_stack_t stack = &node_value(thread_t,vm->current_thread)->call_stack;
	e.cs = seg;
	e.ip = ofs;
	gpush( stack, &e );
	return vm;
}

vm_t vm_push_catcher(vm_t vm, program_t seg, word_t ofs) {
	struct _call_stack_entry_t e;
	generic_stack_t stack = &node_value(thread_t,vm->current_thread)->catch_stack;
	e.cs = seg;
	e.ip = ofs;
	gpush( stack, &e );
	return vm;
}


vm_t vm_peek_data(vm_t vm, int rel_ofs, vm_data_type_t* type, word_t* value) {
	generic_stack_t stack = &node_value(thread_t,vm->current_thread)->data_stack;
	struct _data_stack_entry_t* top = gpeek( struct _data_stack_entry_t*, stack, rel_ofs );
	*type = (vm_data_type_t) top->type;
	*value = top->data;
	return vm;
}

vm_t vm_poke_data(vm_t vm, vm_data_type_t type, word_t value) {
	generic_stack_t stack = &node_value(thread_t,vm->current_thread)->data_stack;
	struct _data_stack_entry_t* top = gpeek( struct _data_stack_entry_t*, stack, 0 );
	top->type = type;
	top->data = value;
	return vm;
}

vm_t vm_peek_caller(vm_t vm, program_t* cs, word_t* ip) {
	generic_stack_t stack = &node_value(thread_t,vm->current_thread)->call_stack;
	struct _call_stack_entry_t* top = gpeek( struct _call_stack_entry_t*, stack, 0 );
	*cs = top->cs;
	*ip = top->ip;
	return vm;
}

vm_t vm_peek_catcher(vm_t vm, program_t* cs, word_t* ip) {
	generic_stack_t stack = &node_value(thread_t,vm->current_thread)->catch_stack;
	struct _call_stack_entry_t* top = gpeek( struct _call_stack_entry_t*, stack, 0 );
	*cs = top->cs;
	*ip = top->ip;
	return vm;
}


vm_t vm_pop_data(vm_t vm, word_t count) {
	generic_stack_t stack = &node_value(thread_t,vm->current_thread)->data_stack;
	while(count>0) {
		_gpop(stack);
		count-=1;
	}
	return vm;
}

vm_t vm_pop_caller(vm_t vm, word_t count) {
	generic_stack_t stack = &node_value(thread_t,vm->current_thread)->call_stack;
	while(count>0) {
		_gpop(stack);
		count-=1;
	}
	return vm;
}

vm_t vm_pop_catcher(vm_t vm, word_t count) {
	generic_stack_t stack = &node_value(thread_t,vm->current_thread)->catch_stack;
	while(count>0) {
		_gpop(stack);
		count-=1;
	}
	return vm;
}





void vm_exec_cycle(vm_t vm, thread_t t) {
	opcode_stub_t op;
	word_t arg;
	word_t*array;
/*
	if(t->IP>=t->program->code.size) {
		t->state=ThreadDying;
		return;
	}
*/
	/* fetch */
	/*program_fetch(t->program, t->IP, (word_t*)&op, &arg);*/
	array = t->program->code.data+t->IP;
	op = (opcode_stub_t) *array;
	arg = *(array+1);
	/* decode */
	op(vm,arg);
	/* iterate */
	if(t->jmp_seg) {
		/* it's a (long) call */
		/* assume call stack is already filled */
		t->program = (program_t)t->jmp_seg;
		t->IP=t->jmp_ofs;
		t->jmp_seg=NULL;
		t->jmp_ofs=0;
	} else if(t->jmp_ofs) {
		/* it's a (short) jump or call */
		/* assume call stack is already filled */
		t->IP=t->jmp_ofs;
		t->jmp_ofs=0;
	} else if(t->IP==t->program->code.size-2) {
		/* we are at the end of the segment, thread should die. */
		t->state=ThreadDying;
	} else {
		/* hop to next instruction */
		t->IP+=2;
	}
}




thread_t vm_select_thread(vm_t vm) {
	thread_t current;
	if(vm->current_thread) {
		current = node_value(thread_t,vm->current_thread);
		/* quick pass if current meets conditions */
		if(current->state==ThreadRunning&&current->remaining!=0&&current->remaining<vm->timeslice) {
			current->remaining -= 1;
			return current;
		}
	}

	if(vm->current_thread&&vm->current_thread->next) {
		vm->current_thread = vm->current_thread->next;
	} else if(vm->running_threads.head) {
		vm->current_thread = vm->running_threads.head;
	}

	/* solve the conflict between next ready and next running threads, if any */
	if(vm->ready_threads.head) {
		dlist_node_t a = vm->current_thread;
		dlist_node_t b = vm->ready_threads.head;
		thread_t tb = node_value(thread_t,b);
		if((!a)||node_value(thread_t,a)->prio < tb->prio) {
			/* thread is no more ready */
			vm->ready_threads.head = vm->ready_threads.head->next;
			tb->state=ThreadRunning;
			dlist_insert_sorted(&vm->running_threads,b,comp_prio);
			tb->remaining = vm->timeslice;
			vm->current_thread = b;
		}
	}

	if(vm->current_thread) {
		return node_value(thread_t,vm->current_thread);
	} else {
		return NULL;
	}
}



/* if has no thread, do nothing.
 * if has one thread, execute it.
 * if has many threads, time-slice round-robin them.
 */
vm_t vm_schedule_cycle(vm_t vm) {
	thread_t current;

	switch(vm->scheduler) {
	case SchedulerRoundRobin:
		current = vm_select_thread(vm);
		break;
	case SchedulerMonoThread:
		if((!vm->current_thread)&&vm->ready_threads.head) {
			vm->current_thread=vm->ready_threads.head;
			vm->ready_threads.head=vm->ready_threads.head->next;
			if(!vm->ready_threads.head) {
				vm->ready_threads.tail=NULL;
			}
			vm->running_threads.head=vm->current_thread;
			vm->running_threads.tail=vm->current_thread;
			node_value(thread_t,vm->current_thread)->state=ThreadRunning;
		}
		if(!vm->current_thread) {
			return vm;
		}
		current = node_value(thread_t,vm->current_thread);
		break;
	default:;
		return vm;
	};
/*	printf("CURRENT THREAD %p\n",current);
	printf("CURRENT PROGRAM %p\n",current->program);
*/	vm_exec_cycle(vm,current);
	if(current->state==ThreadDying) {
		vm_dump_data_stack(vm);
		thread_delete(node_value(thread_t,vm->current_thread));
		dlist_remove(&vm->running_threads,vm->current_thread);
		vm->current_thread=NULL;
		vm->threads_count-=1;
	}
	vm->cycles+=1;
	return vm;
}

vm_t vm_set_engine(vm_t vm, vm_engine_t e) {
	if(vm->engine) {
		/* deinit */
	}
	vm->engine=e;
	e->vm=vm;
	e->_init(e);
	return vm;
}

