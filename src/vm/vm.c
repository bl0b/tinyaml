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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "vm.h"
#include "_impl.h"
#include "text_seg.h"
#include "opcode_dict.h"
#include "program.h"

#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <assert.h>

#include "bml_ops.h"
#include "object.h"

/* hidden feature in tinyap */
ast_node_t ast_unserialize(const char*);
/* hidden serialized ml grammar */
extern const char* ml_core_grammar;
extern const char* ml_core_lib;

vm_t vm_new() {
	vm_t ret = (vm_t)malloc(sizeof(struct _vm_t));
	tinyap_init();
	ret->result=NULL;
	ret->parser = tinyap_new();
	tinyap_set_grammar_ast(ret->parser,ast_unserialize(ml_core_grammar));
	opcode_dict_init(&ret->opcodes);
	ret->current_node=NULL;
	gstack_init(&ret->cn_stack,sizeof(wast_t));
	ret->threads_count=0;
	ret->current_thread=NULL;
	ret->timeslice=100;
	dynarray_init(&ret->compile_vectors.by_index);
	init_hashtab(&ret->compile_vectors.by_text, (hash_func) hash_str, (compare_func) strcmp);
	dynarray_set(&ret->compile_vectors.by_index,1,0);
	dynarray_set(&ret->compile_vectors.by_index,0,0);
	slist_init(&ret->all_programs);
	dlist_init(&ret->ready_threads);
	dlist_init(&ret->running_threads);
	dlist_init(&ret->yielded_threads);
	dlist_init(&ret->gc_pending);
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

void htab_free_dict(htab_entry_t);


void vm_del(vm_t ret) {
	/*printf("delete vm\n");*/
	dlist_forward(&ret->ready_threads,thread_t,thread_delete);
	dlist_forward(&ret->running_threads,thread_t,thread_delete);
	dlist_forward(&ret->yielded_threads,thread_t,thread_delete);
	dynarray_deinit(&ret->compile_vectors.by_index,NULL);
	clean_hashtab(&ret->compile_vectors.by_text,htab_free_dict);
	dlist_flush(&ret->ready_threads);
	dlist_flush(&ret->running_threads);
	dlist_flush(&ret->yielded_threads);
	tinyap_delete(ret->parser);
	opcode_dict_deinit(&ret->opcodes);
	#define prg_free(_x) program_free(ret,_x)
	slist_forward(&ret->all_programs,program_t,prg_free);
	slist_flush(&ret->all_programs);
	#undef prg_free
	#define obj_free(_x) vm_obj_free(ret,_x)
	dlist_forward(&ret->gc_pending,void*,obj_free);
	#undef obj_free
	dlist_flush(&ret->gc_pending);
	gstack_deinit(&ret->cn_stack,NULL);
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






word_t opcode_arg_serialize(program_t p, opcode_arg_t arg_type, word_t arg) {
	switch(arg_type) {
	case OpcodeArgString:
		arg = text_seg_text_to_index(&p->strings,(const char*)arg);
		break;
	default:;
	};
	return arg;
}


word_t opcode_arg_unserialize(program_t p, opcode_arg_t arg_type, word_t arg) {
	switch(arg_type) {
	case OpcodeArgString:
		arg = (word_t) text_seg_find_by_index(&p->strings,arg);
		break;
	default:;
	};
	return arg;
}


#define PROG_FILE_MAGIC "BMLP"
#define ENDIAN_TEST 0x01000000


program_t vm_unserialize_program(vm_t vm, reader_t r) {
	program_t p=program_new();
	opcode_dict_t od;
	const char*str;
	int i;
	word_t endian;
	word_t tot;
	word_t op;
	word_t wc;
	word_t arg;

	str = read_string(r);
	if(strcmp(str,"BML_PRG")) {
		return NULL;
	}
	endian = read_word(r);
	/* FIXME : byte reversal is not yet implemented */
	assert(ENDIAN_TEST==endian);

	od = opcode_dict_new();
	opcode_dict_unserialize(od,r,vm->dl_handle);

	text_seg_unserialize(&p->strings,r);

	str = read_string(r);
	assert(!strcmp(str,"CODE---"));
	tot = read_word(r);
	dynarray_reserve(&p->code,tot+2);
	for(i=0;i<tot;i+=2) {
		wc = read_word(r);
		op = (word_t) opcode_stub_by_code(od, wc);
		arg = opcode_arg_unserialize(p, WC_GET_ARGTYPE(wc), read_word(r));
		program_write_code(p,op,arg);
	}
	opcode_dict_free(od);
	wc = read_word(r);
	assert(wc==0xFFFFFFFF);
	str = read_string(r);
	assert(!strcmp(str,"DATA---"));
	wc = read_word(r);
	dynarray_reserve(&p->data,wc+2);
	for(i=0;i<wc;i+=2) {
		p->data.data[i] = read_word(r);
		p->data.data[i+1] = opcode_arg_unserialize(p, p->data.data[i], read_word(r));
	}
	p->data.size=wc;
	return p;
}



opcode_dict_t opcode_dict_optimize(vm_t vm, program_t prog) {
	opcode_dict_t glob = vm_get_dict(vm);
	opcode_dict_t od = opcode_dict_new();
	const char* name;
	opcode_arg_t arg_type;
	opcode_stub_t stub;
	int i;
	for(i=0;i<prog->code.size;i+=2) {
		stub = (opcode_stub_t)prog->code.data[i];
		name = opcode_name_by_stub(glob,stub);
		arg_type = WC_GET_ARGTYPE(opcode_code_by_stub(glob,stub));
		opcode_dict_add(od, arg_type, name, stub);
	}
	return od;
}




vm_t vm_serialize_program(vm_t vm, program_t p, writer_t w) {
	int i;
	word_t op;
	word_t arg;
	/* optimize opcode dictionary */
	opcode_dict_t odopt = opcode_dict_optimize(vm,p);
	/* write header */
	write_string(w,"BML_PRG");
	write_word(w,ENDIAN_TEST);
	/* write dict */
	opcode_dict_serialize(odopt,w);
	/* write text segment */
	text_seg_serialize(&p->strings,w);
	/* write code segment */
	write_string(w,"CODE---");
	/* write code segment size */
	write_word(w,dynarray_size(&p->code));
	for(i=0;i<dynarray_size(&p->code);i+=2) {
		printf("%8.8lX %8.8lX  ",dynarray_get(&p->code,i),dynarray_get(&p->code,i+1));
		if(i%8==6) printf("\n");
	}
	printf("\n");
	/* write serialized word code */
	for(i=0;i<dynarray_size(&p->code);i+=2) {
		op = opcode_code_by_stub(odopt, (opcode_stub_t)dynarray_get(&p->code,i));
		arg = opcode_arg_serialize(p, WC_GET_ARGTYPE(op), dynarray_get(&p->code,i+1));
		write_word(w,op);
		write_word(w,arg);
	}
	opcode_dict_free(odopt);
	write_word(w,0xFFFFFFFF);
	write_string(w,"DATA---");
	write_word(w,dynarray_size(&p->data));
	for(i=0;i<dynarray_size(&p->data);i+=2) {
		op = dynarray_get(&p->data,i);
		arg = opcode_arg_serialize(p, op, dynarray_get(&p->data,i+1));
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
		slist_insert_tail(&vm->all_programs,p);
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
		slist_insert_tail(&vm->all_programs,p);
	} else {
		fprintf(stderr,tinyap_get_error(vm->parser));
	}
	return p;
}



vm_t vm_run_program_bg(vm_t vm, program_t p, word_t ip, word_t prio) {
	vm->engine->_run_async(vm->engine, p, ip, prio);
	return vm;
}


vm_t vm_run_program_fg(vm_t vm, program_t p, word_t ip, word_t prio) {
	vm->engine->_run_sync(vm->engine, p, ip, prio);
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
	if(!vm->current_thread) {
		return NULL;
	}
	return node_value(thread_t,vm->current_thread);
}

program_t _VM_CALL vm_get_CS(vm_t vm) {
	if(!vm->current_thread) {
		return NULL;
	}
	return node_value(thread_t,vm->current_thread)->program;
}

word_t _VM_CALL vm_get_IP(vm_t vm) {
	if(!vm->current_thread) {
		return 0;
	}
	return node_value(thread_t,vm->current_thread)->IP;
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
	vm_data_t e;
	generic_stack_t stack = &node_value(thread_t,vm->current_thread)->data_stack;
	int sz = gstack_size(stack);
	int i;

	printf("sz = %i\n",sz);
	for(i=0;i<sz;i+=1) {
		e = (vm_data_t ) gpeek(vm_data_t ,stack,-i);
		printf("#%i : %4.4X %8.8lX\n",i,e->type, e->data);
	}
}

vm_t vm_push_data(vm_t vm, vm_data_type_t type, word_t value) {
	struct _data_stack_entry_t e;
	generic_stack_t stack = &node_value(thread_t,vm->current_thread)->data_stack;
	/*printf("vm push data : %lu %lu\n",type,value);*/
	e.type = type;
	e.data = value;
	if(type==DataObject) {
		vm_obj_ref(vm,(void*)value);
	}
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
	vm_data_t top = gpeek( vm_data_t , stack, rel_ofs );
	*type = (vm_data_type_t) top->type;
	*value = top->data;
	return vm;
}

vm_t vm_poke_data(vm_t vm, vm_data_type_t type, word_t value) {
	generic_stack_t stack = &node_value(thread_t,vm->current_thread)->data_stack;
	vm_data_t top = gpeek( vm_data_t , stack, 0 );
	if(top->type==DataObject) {
		vm_obj_deref(vm,(void*)top->data);
	}
	top->type = type;
	top->data = value;
	if(top->type==DataObject) {
		vm_obj_ref(vm,(void*)top->data);
	}
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


vm_data_t _VM_CALL _vm_pop(vm_t vm) {
	generic_stack_t stack = &node_value(thread_t,vm->current_thread)->data_stack;
	vm_data_t top = _gpop(stack);
	if(top->type==DataObject) {
		vm_obj_deref(vm,(void*)top->data);
	}
	return top;
}

vm_t _VM_CALL vm_collect(vm_t vm, vm_obj_t o) {
	/*printf("vm collect %p\n",o);*/
	dlist_insert_head(&vm->gc_pending,o);
	return vm;
}

vm_t _VM_CALL vm_uncollect(vm_t vm, vm_obj_t o) {
	dlist_node_t dn;
	/*printf("vm uncollect %p\n",o);*/
	if(!vm->gc_pending.head) {
		/*printf("   => failed.\n");*/
		return vm;
	}
	dn = vm->gc_pending.head;
	while(dn) {
		if(dn->value==(word_t)o) {
			dlist_remove(&vm->gc_pending,dn);
			return vm;
		}
		dn = dn->next;
	}
	/*printf("   => failed.\n");*/
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





thread_state_t _VM_CALL vm_exec_cycle(vm_t vm, thread_t t) {
	register word_t*array;
/*
	if(t->IP>=t->program->code.size) {
		t->state=ThreadDying;
		return;
	}
*/
	/* fetch */
	/*program_fetch(t->program, t->IP, (word_t*)&op, &arg);*/
	array = t->program->code.data+t->IP;

	/*printf("EXEC %p:%lX\n",t->program,t->IP);*/

	/* decode */
	((opcode_stub_t) *array) ( vm, *(array+1) );

	/* iterate */
	if(t->jmp_ofs) {
		/* it's a jump */
		/* assume call stack is already filled */
		t->program = t->jmp_seg;
		t->IP=t->jmp_ofs;
		t->jmp_ofs=0;
	} else if(t->IP==t->program->code.size-2) {
		/* we are at the end of the segment, thread should die. */
		t->state=ThreadDying;
	} else {
		/* hop to next instruction */
		t->IP+=2;
	}
	return t->state;
}





typedef thread_t (*_VM_CALL _sched_method_t) (vm_t);



thread_t _VM_CALL _sched_idle(vm_t vm) {
	return NULL;
}



thread_t _VM_CALL _sched_mono(vm_t vm) {
	register thread_t current=NULL;
	switch((word_t)vm->current_thread) {
	case 0:
		vm->current_thread=vm->ready_threads.head;
		vm->ready_threads.head=vm->ready_threads.head->next;
		if(!vm->ready_threads.head) {
			vm->ready_threads.tail=NULL;
		}
		vm->running_threads.head=vm->current_thread;
		vm->running_threads.tail=vm->current_thread;
		current = node_value(thread_t,vm->current_thread);
		current->state=ThreadRunning;
		return current;
	default:
		return node_value(thread_t,vm->current_thread);
	};
/*	if(vm->current_thread) {
		return node_value(thread_t,vm->current_thread);
	} else if(vm->ready_threads.head) {
		thread_t current=NULL;
		vm->current_thread=vm->ready_threads.head;
		vm->ready_threads.head=vm->ready_threads.head->next;
		if(!vm->ready_threads.head) {
			vm->ready_threads.tail=NULL;
		}
		vm->running_threads.head=vm->current_thread;
		vm->running_threads.tail=vm->current_thread;
		current = node_value(thread_t,vm->current_thread);
		current->state=ThreadRunning;
		return current;
	}
	return NULL;*/
}



thread_t _VM_CALL _sched_rr(vm_t vm) {
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



_sched_method_t schedulers[SchedulerAlgoMax] = {
	_sched_idle,
	_sched_mono,
	_sched_rr,
};

/* if has no thread, do nothing.
 * if has one thread, execute it.
 * if has many threads, time-slice round-robin them.
 */
vm_t vm_schedule_cycle(vm_t vm) {
	thread_t current = schedulers[vm->scheduler](vm);

/*	printf("CURRENT THREAD %p\n",current);
	printf("CURRENT PROGRAM %p\n",current->program);
	if(current) {
*/
	switch(current?vm_exec_cycle(vm,current):ThreadDying+1) {
	case ThreadDying:
		/*vm_dump_data_stack(vm);*/
		thread_delete(node_value(thread_t,vm->current_thread));
		dlist_remove(&vm->running_threads,vm->current_thread);
		vm->current_thread=NULL;
		vm->threads_count-=1;
	default:
		vm->cycles+=1;
		return vm;
	};
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

