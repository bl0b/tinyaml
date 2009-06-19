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

#include "fastmath.h"
#include "vm.h"
#include "_impl.h"
#include "text_seg.h"
#include "opcode_dict.h"
#include "program.h"

#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <stdarg.h>
#include "vm_assert.h"

#include "bml_ops.h"
#include "object.h"
#include "vm_engine.h"

#include <setjmp.h>



/* hidden feature in tinyap */
ast_node_t ast_unserialize(const char*);
/* hidden serialized ml grammar */
extern const char* ml_core_grammar;
extern const char* ml_core_lib;

volatile vm_t _glob_vm = NULL;
volatile int _vm_trace = 0;

jmp_buf _glob_vm_jmpbuf;

const char* thread_state_to_str(thread_state_t ts);

program_t dynFun_exec = NULL;

const char* vm_find_innermost_file(vm_t vm) {
	int ofs=0;
	vm_data_t d;
	do {
		d = (vm_data_t) _gpeek(&vm->compinput_stack, 0);
	} while((ofs=ofs+1)<vm->compinput_stack.sp&&CompInFile!=(compinput_t)d->type);
	if(CompInFile!=(compinput_t)d->type) {
		return "buffer";
	}
	return (const char*)d->data;
}

void vm_print_compilation_source(vm_t vm, int ofs) {
	vm_data_t d = (vm_data_t) _gpeek(&vm->compinput_stack, ofs);
	compinput_t ci = (compinput_t) d->type;
	const char* buf = (const char*) d->data;
	switch(ci) {
	case CompInFile:
		vm_printf("file %s", buf);
		break;
	case CompInBuffer:
		if(strlen(buf)<=43) {
			vm_printf("buffer << %s >>", buf);
		} else {
			vm_printf("buffer << %20.20s...%20.20s >>", buf, buf+strlen(buf)-20);
		}
		break;
	case CompInVWalker:
		vm_printf("walker %s", buf);
		break;
	default:
		vm_printf("<UNHANDLED %i:%s>", ci, buf);
	};
}

void vm_print_data(vm_t vm, vm_data_t d) {
	_IFC tmp;
	if(!d) {
		vm_printf("[(null)]");
		return;
	}
	tmp.i = d->data;
	switch(d->type) {
	case DataInt:
		vm_printf("[Int %li]", tmp.i);
		break;
	case DataFloat:
		vm_printf("[Float %lf]", tmp.f);
		break;
	case DataString:
		vm_printf("[String \"%s\"]", (const char*) tmp.i);
		break;
	case DataObjStr:
		vm_printf("[ObjStr  \"%s\"]",(const char*)tmp.i);
		break;
	case DataObjSymTab:
		vm_printf("[SymTab  %p]",(void*)tmp.i);
		break;
	case DataObjMutex:
		vm_printf("[Mutex  %p]",(void*)tmp.i);
		break;
	case DataObjThread:
		vm_printf("[Thread  %p]",(void*)tmp.i);
		break;
	case DataObjArray:
		vm_printf("[Array  %p]",(void*)tmp.i);
		break;
	case DataObjEnv:
		vm_printf("[Map  %p]",(void*)tmp.i);
		break;
	case DataObjStack:
		vm_printf("[Stack  %p]",(void*)tmp.i);
		break;
	case DataObjFun:
		vm_printf("[Function  %p]",(void*)tmp.i);
		break;
	case DataObjVObj:
		vm_printf("[V-Obj  %p]",(void*)tmp.i);
		break;
	case DataObjUser:
		vm_printf("[UserObj  %p : %X]",(void*)tmp.i, *(word_t*)tmp.i);
		break;
	case DataManagedObjectFlag:
		vm_printf("[Undefined Object ! %p]", (opcode_stub_t*)tmp.i);
		break;
	case DataTypeMax:
	default:;
		vm_printf("[Erroneous data %X %lX]", d->type, tmp.i);
	};
	/*fflush(stderr);*/

}


extern volatile int line_number_bias;

void default_error_handler(vm_t vm, const char* input, int is_buffer) {
	int ofs, sz;
	compinput_t ci;
	const char* buf;
	vm_data_t d;
	wast_t wa;
	sz = vm->compinput_stack.sp;
	vm_printerrf("Error during compilation :\n");
	for(ofs=-sz;ofs<=0;ofs+=1) {
		vm_printf(" * in ");
		vm_print_compilation_source(vm, ofs);
		vm_printf("\n");
	}
	sz = vm->cn_stack.sp;
	vm_printf("Node stack :\n");
	for(ofs=-sz;ofs<=0;ofs+=1) {
		wa = *(wast_t*) _gpeek(&vm->cn_stack, ofs);
		if(wa) {
			vm_printf(" * %i:%i %s (%i)\n", wa_row(wa)+line_number_bias, wa_col(wa), wa_op(wa), wa_opd_count(wa));
		}
	}
	vm_printf("at %i:%i (%s)\n", wa_row(vm->current_node)+line_number_bias, wa_col(vm->current_node), wa_op(vm->current_node));
	exit(-1);
}

vm_t vm_set_error_handler(vm_t vm, vm_error_handler handler) {
	vm->onCompileError = handler;
	return vm;
}

vm_error_handler vm_get_error_handler(vm_t vm) {
	return vm->onCompileError;
}

void vm_compinput_push_file(vm_t vm, const char*filename) {
	struct _data_stack_entry_t d;
	d.type=(vm_data_type_t)CompInFile;
	d.data=(word_t)filename;
	gpush(&vm->compinput_stack, &d);
}

void vm_compinput_push_buffer(vm_t vm, const char*buffer) {
	struct _data_stack_entry_t d;
	d.type=(vm_data_type_t)CompInBuffer;
	d.data=(word_t)buffer;
	gpush(&vm->compinput_stack, &d);
}

void vm_compinput_push_walker(vm_t vm, const char*wname) {
	struct _data_stack_entry_t d;
	d.type=(vm_data_type_t)CompInVWalker;
	d.data=(word_t)wname;
	gpush(&vm->compinput_stack, &d);
}

void vm_compinput_pop(vm_t vm) {
	_gpop(&vm->compinput_stack);
}

/* the VM is a singleton */
vm_t vm_new() {
	vm_t ret;

	if(_glob_vm) {
		return _glob_vm;
	}

	ret = (vm_t)malloc(sizeof(struct _vm_t));
	_glob_vm = ret;
	tinyap_init();
	ret->onCompileError = default_error_handler;
	gstack_init(&ret->compinput_stack,sizeof(struct _data_stack_entry_t));
	ret->compile_reent=0;
	ret->engine=stub_engine;
	ret->engine->vm=ret;
	ret->result=NULL;
	ret->current_edit_prg=NULL;
	ret->exception.type=DataInt;
	ret->exception.data=0;
	ret->parser = tinyap_new();
	tinyap_set_grammar_ast(ret->parser,ast_unserialize(ml_core_grammar));
	opcode_dict_init(&ret->opcodes);
	init_hashtab(&ret->loadlibs, (hash_func) hash_str, (compare_func) strcmp);
	init_hashtab(&ret->required, (hash_func) hash_str, (compare_func) strcmp);
	ret->current_node=NULL;
	gstack_init(&ret->cn_stack,sizeof(wast_t));
	ret->threads_count=0;
	ret->current_thread=NULL;
	ret->virt_walker=NULL;
	ret->timeslice=100;
	text_seg_init(&ret->gram_nodes);
	dynarray_init(&ret->compile_vectors.by_index);
	init_hashtab(&ret->compile_vectors.by_text, (hash_func) hash_str, (compare_func) strcmp);
	dynarray_set(&ret->compile_vectors.by_index,1,0);
	dynarray_set(&ret->compile_vectors.by_index,0,0);
	dlist_init(&ret->init_routines);
	dlist_init(&ret->term_routines);
	slist_init(&ret->all_handles);
	slist_init(&ret->all_programs);
	dlist_init(&ret->ready_threads);
	dlist_init(&ret->running_threads);
	dlist_init(&ret->yielded_threads);
	dlist_init(&ret->zombie_threads);
	dlist_init(&ret->gc_pending);

	ret->dl_handle=NULL;


	ret->cycles=0;

	/* fill opcodes dictionary with basic library */
	vm_compile_buffer(ret, ml_core_lib);
	/* nops are hardcoded due to a limitation of tinyap */
	vm_add_opcode(ret,"nop",OpcodeNoArg, vm_op_nop);
	vm_add_opcode(ret,"nop",OpcodeArgString, vm_op_nop);
	vm_add_opcode(ret,"nop",OpcodeArgLabel, vm_op_nop);
	vm_add_opcode(ret,"nop",OpcodeArgEnvSym, vm_op_nop);
	vm_add_opcode(ret,"nop",OpcodeArgInt, vm_op_nop);
	vm_add_opcode(ret,"nop",OpcodeArgFloat, vm_op_nop);

	ret->env = vm_env_new();
	vm_obj_ref_ptr(ret,ret->env);

	if(!dynFun_exec) {
		dynFun_exec = vm_compile_buffer(ret, "asm call ret 0 end");
	}

	return ret;
}

void htab_free_dict(htab_entry_t);


void vm_del(vm_t ret) {
	dlist_node_t dn;

	ret->engine->_kill(ret->engine);
	ret->engine=stub_engine;
	ret->engine->vm=ret;
	/*vm_printf("delete vm\n");*/
	ret->engine->_vm_lock(ret->engine);
	#define thd_del(_x) vm_kill_thread(ret,_x)
	dlist_forward(&ret->ready_threads,thread_t,thd_del);
	dlist_forward(&ret->running_threads,thread_t,thd_del);
	dlist_forward(&ret->yielded_threads,thread_t,thd_del);
	#undef thd_del

	ret->current_thread=NULL;

	#define onCompDel(_x) if(_x) vm_obj_deref_ptr(ret, _x)
	dlist_forward(&ret->init_routines, vm_dyn_func_t, onCompDel);
	dlist_flush(&ret->init_routines);
	dlist_forward(&ret->term_routines, vm_dyn_func_t, onCompDel);
	dlist_flush(&ret->term_routines);
	#undef onCompDel

	#define prg_free(_x) program_free(ret,_x)
	/*slist_forward(&ret->all_programs,program_t,program_dump_stats);*/
	slist_forward(&ret->all_programs,program_t,prg_free);
	slist_flush(&ret->all_programs);
	#undef prg_free

	#define dl_cl(_x) if(_x) dlclose(_x)
	/*slist_forward(&ret->all_handles, void*, dl_cl);*/
	slist_forward(&ret->all_handles, void*, dlclose);
	slist_flush(&ret->all_handles);
	#undef dl_cl

	dynarray_deinit(&ret->compile_vectors.by_index,NULL);
	clean_hashtab(&ret->compile_vectors.by_text,htab_free_dict);
	clean_hashtab(&ret->loadlibs, htab_free_dict);
	clean_hashtab(&ret->required, htab_free_dict);

	tinyap_delete(ret->parser);
	opcode_dict_deinit(&ret->opcodes);
	text_seg_deinit(&ret->gram_nodes);

	/*#define thd_del(_x) vm_collect(ret,PTR_TO_OBJ(_x))*/
	/*dlist_forward(&ret->zombie_threads,thread_t,thd_del);*/
	/*#undef thd_del*/

	vm_obj_deref_ptr(ret, ret->env);
	/*vm_collect(ret,PTR_TO_OBJ(ret->env));*/

	while(ret->gc_pending.tail) {
		dn = ret->gc_pending.tail;
		ret->gc_pending.tail=dn->prev;
		if(dn->prev) {
			dn->prev->next=NULL;
		} else {
			ret->gc_pending.head=NULL;
		}
		vm_obj_free_obj(ret,(void*)dn->value);
		free(dn);
	}
	/*dlist_forward(&ret->gc_pending,void*,obj_free);*/
	dlist_flush(&ret->gc_pending);

	gstack_deinit(&ret->cn_stack,NULL);
	gstack_deinit(&ret->compinput_stack,NULL);
	ret->engine->_vm_unlock(ret->engine);
	free(ret);
	_glob_vm = NULL;
}


void vm_set_timeslice(vm_t vm, int timeslice) {
	vm->timeslice=timeslice;
}

int comp_prio(dlist_node_t a, dlist_node_t b) {
	return (int)(node_value(thread_t,b)->prio-node_value(thread_t,a)->prio);
}


vm_t vm_set_lib_file(vm_t vm, const char*fname) {
	char buffer[1024];
	if(vm->dl_handle) {
		slist_insert_tail(&vm->all_handles,vm->dl_handle);
	}
	if(!fname) {
		vm->dl_handle=NULL;
		return vm;
	}
	snprintf(buffer,1024,"%s/libtinyaml_%s.so",TINYAML_EXT_DIR,fname);
	vm->dl_handle = dlopen(buffer, RTLD_LAZY);
	if(!vm->dl_handle) {
		vm_printerrf("[VM:ERR] Couldn't open library \"%s\"\n: %s\n",buffer, dlerror());
	}
	return vm;
}

vm_t vm_add_opcode(vm_t vm, const char*name, opcode_arg_t arg_type, opcode_stub_t stub) {
	vm->engine->_client_lock(vm->engine);
	opcode_dict_add(&vm->opcodes,arg_type,name,stub);
	vm->engine->_client_unlock(vm->engine);
	return vm;
}

opcode_dict_t vm_get_dict(vm_t vm) {
	return &vm->opcodes;
}







#define ENDIAN_TEST 0x01000000

program_t vm_unserialize_program(vm_t vm, reader_t r) {
	program_t ret;
	if(strcmp(read_string(r),TINYAML_SHEBANG)) {
		return NULL;
	}

	if(ENDIAN_TEST!=read_word(r)) {
		reader_swap_endian(r);
	}

	ret = program_unserialize(vm,r);
	if(ret) {
		vm->engine->_client_lock(vm->engine);
		slist_insert_tail(&vm->all_programs,ret);
		vm->engine->_client_unlock(vm->engine);
		/*slist_forward(&vm->all_programs,program_t,program_dump_stats);*/
	}
	return ret;
}



vm_t vm_serialize_program(vm_t vm, program_t p, writer_t w) {
	/* write header */
	write_string(w, TINYAML_SHEBANG);
	write_word(w,ENDIAN_TEST);
	program_serialize(vm,p,w);
	return vm;
}



program_t compile_wast(wast_t, vm_t);
void wa_del(wast_t w);

program_t vm_compile_file(vm_t vm, const char* fname) {
	program_t p=NULL;
	word_t reent = vm->compile_reent;
	vm->compile_reent=0;

	/*vm_printf("vm_compile_file(%s)\n",fname);*/

	tinyap_set_source_file(vm->parser,fname);
	vm_compinput_push_file(vm, fname);
	tinyap_parse(vm->parser);

	if(tinyap_parsed_ok(vm->parser)&&tinyap_get_output(vm->parser)) {
		wast_t wa = tinyap_make_wast( tinyap_list_get_element( tinyap_get_output(vm->parser), 0) );
		p = compile_wast(wa, vm);
		wa_del(wa);
		vm->engine->_client_lock(vm->engine);
		slist_insert_tail(&vm->all_programs,p);
		vm->engine->_client_unlock(vm->engine);
		/*slist_forward(&vm->all_programs,program_t,program_dump_stats);*/
	} else {
		vm_printerrf("parse error at %i:%i\n%s",tinyap_get_error_row(vm->parser),tinyap_get_error_col(vm->parser),tinyap_get_error(vm->parser));
		vm->onCompileError(vm, "<not implemented>", 0);
	}
	vm_compinput_pop(vm);
	vm->compile_reent=reent;
	return p;
}


program_t vm_compile_buffer(vm_t vm, const char* buffer) {
	program_t p=NULL;
	word_t reent = vm->compile_reent;
	vm->compile_reent=0;
	/*vm_printf("vm_compile_buffer(%s)\n",buffer);*/
	tinyap_set_source_buffer(vm->parser,buffer,strlen(buffer));
	vm_compinput_push_buffer(vm, buffer);
	tinyap_parse(vm->parser);

	if(tinyap_parsed_ok(vm->parser)&&tinyap_get_output(vm->parser)) {
		wast_t wa = tinyap_make_wast( tinyap_list_get_element( tinyap_get_output(vm->parser), 0) );
		p = compile_wast(wa, vm);
		wa_del(wa);
		vm->engine->_client_lock(vm->engine);
		slist_insert_tail(&vm->all_programs,p);
		vm->engine->_client_unlock(vm->engine);
		/*slist_forward(&vm->all_programs,program_t,program_dump_stats);*/
	} else {
		vm_printerrf("Parse error at %i:%i\n%s",tinyap_get_error_row(vm->parser),tinyap_get_error_col(vm->parser),tinyap_get_error(vm->parser));
		vm->onCompileError(vm, "<not implemented>", 0);
	}
	vm_compinput_pop(vm);
	vm->compile_reent=reent;
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

/* FIXME : include support for thread args */
thread_t vm_add_thread_helper(vm_t vm, thread_t t, int fg) {
	struct _data_stack_entry_t argc = { DataInt, 0 };
	vm_obj_ref_ptr(vm, t);
	t->state=ThreadStateMax;
	t->_sync=fg;
	mutex_lock(vm,&t->join_mutex,t);
	/* FIXME : this should go into thread_new() */
	vm->threads_count += 1;
	if(vm->threads_count==1) {
		/*vm_printf("SchedulerMonoThread\n");*/
		vm->scheduler = SchedulerMonoThread;
		vm->engine->_init(vm->engine);
	} else if(vm->threads_count==2) {
		/*vm_printf("SchedulerRoundRobin\n");*/
		vm->scheduler = SchedulerRoundRobin;
	}
	/* push argc on top of data stack */
	gpush(&t->data_stack, &argc);

	/*dn = (dlist_node_t) malloc(sizeof(struct _dlist_node_t));*/
	/*dn->value=(word_t)t;*/

	/*dlist_insert_sorted(&vm->ready_threads,dn,comp_prio);*/
	thread_set_state(vm,t,ThreadReady);
	return t;
}

thread_t vm_add_thread(vm_t vm, program_t p, word_t ip, word_t prio,int fg) {
	thread_t t;
	/*dlist_node_t dn;*/
	if(!p) {
		return NULL;
	}
	vm->engine->_client_lock(vm->engine);
	t = vm_thread_new(vm,prio,p,ip);

	vm_obj_ref_ptr(vm, t);

	vm_add_thread_helper(vm, t, fg);

	vm->engine->_client_unlock(vm->engine);

	return t;
}


thread_t vm_get_thread(vm_t vm, word_t index) {
	return NULL;
}


thread_t vm_get_current_thread(vm_t vm) {
	if(!vm->current_thread) {
		return NULL;
	}
	return vm->current_thread;
}

program_t _VM_CALL vm_get_CS(vm_t vm) {
	if(!vm->current_thread) {
		return NULL;
	}
	return vm->current_thread->program;
}

word_t _VM_CALL vm_get_IP(vm_t vm) {
	if(!vm->current_thread) {
		return 0;
	}
	return vm->current_thread->IP;
}



void deref_stack(vm_t vm, generic_stack_t gs) {
	long i;
	vm_data_t dt = (vm_data_t)gs->stack;
	if(!dt) {
		return;
	}
	for(i=0;i<=(long)gs->sp;i+=1) {
		if(dt[i].type&DataManagedObjectFlag&&dt[i].type<DataTypeMax) {
			/*vm_printf("stack-deref::found an object : %p\n",(void*)dt[i].data);*/
			vm_obj_deref_ptr(vm,(void*)dt[i].data);
		}
	}
}


vm_t vm_kill_thread(vm_t vm, thread_t t) {
	/*vm_printf("KILLing thread %p\n",t);*/
	vm->engine->_client_lock(vm->engine);
	vm->threads_count-=1;
	if(vm->current_thread&&t==vm->current_thread) {
		vm->current_thread=NULL;
	}
	vm_obj_deref_ptr(vm, t);
	thread_set_state(vm, t, ThreadZombie);
	deref_stack(vm,&t->locals_stack);
	deref_stack(vm,&t->data_stack);
	mutex_unlock(vm,&t->join_mutex,t);
	if(vm->threads_count==1) {
		/*vm_printf("SchedulerMonoThread\n");*/
		vm->scheduler = SchedulerMonoThread;
		vm->engine->_client_unlock(vm->engine);
	} else if(vm->threads_count==0) {
		/*vm_printf("SchedulerIdle\n");*/
		vm->scheduler = SchedulerIdle;
		vm->engine->_client_unlock(vm->engine);
		vm->engine->_deinit(vm->engine);
	}

	return vm;
}


void vm_dump_data_stack(vm_t vm) {
	vm_data_t e;
	generic_stack_t stack = &vm->current_thread->data_stack;
	int sz = gstack_size(stack);
	int i;

	vm_printf("sz = %i\n",sz);
	for(i=0;i<sz;i+=1) {
		e = (vm_data_t ) gpeek(vm_data_t ,stack,-i);
		vm_printf("#%i : %4.4X %8.8lX\n",i,e->type, e->data);
	}
}

vm_t vm_push_data(vm_t vm, vm_data_type_t type, word_t value) {
	struct _data_stack_entry_t e;
	generic_stack_t stack = &vm->current_thread->data_stack;
	/*vm_printf("vm push data : %lu %lu\n",type,value);*/
	e.type = type;
	e.data = value;
	if(type&DataManagedObjectFlag) {
		vm_obj_ref_ptr(vm,(void*)value);
	}
	gpush( stack, &e );
	/*vm_dump_data_stack(vm);*/
	return vm;
}

vm_t vm_push_caller(vm_t vm, program_t seg, word_t ofs, word_t has_closure, vm_dyn_func_t df_callee) {
	struct _call_stack_entry_t e;
	generic_stack_t stack = &vm->current_thread->call_stack;
	e.cs = seg;
	e.ip = ofs;
	e.has_closure = has_closure;
	e.df_callee = df_callee;
	if(df_callee) {
		vm_obj_ref_ptr(vm,df_callee);
	}
	gpush( stack, &e );
	return vm;
}

vm_t vm_push_catcher(vm_t vm, program_t seg, word_t ofs) {
	struct _call_stack_entry_t e;
	generic_stack_t stack = &vm->current_thread->catch_stack;
	e.cs = seg;
	e.ip = ofs;
	e.has_closure = vm->current_thread->call_stack.sp;
	gpush( stack, &e );
	return vm;
}


vm_t vm_peek_data(vm_t vm, int rel_ofs, vm_data_type_t* type, word_t* value) {
	generic_stack_t stack = &vm->current_thread->data_stack;
	vm_data_t top = gpeek( vm_data_t , stack, rel_ofs );
	*type = (vm_data_type_t) top->type;
	*value = top->data;
	return vm;
}

vm_t vm_poke_data(vm_t vm, vm_data_type_t type, word_t value) {
	generic_stack_t stack = &vm->current_thread->data_stack;
	vm_data_t top = gpeek( vm_data_t , stack, 0 );
	if(top->type&DataManagedObjectFlag) {
		vm_obj_deref_ptr(vm,(void*)top->data);
	}
	top->type = type;
	top->data = value;
	if(top->type&DataManagedObjectFlag) {
		vm_obj_ref_ptr(vm,(void*)top->data);
	}
	return vm;
}

vm_t vm_peek_caller(vm_t vm, program_t* cs, word_t* ip) {
	generic_stack_t stack = &vm->current_thread->call_stack;
	struct _call_stack_entry_t* top = gpeek( struct _call_stack_entry_t*, stack, 0 );
	*cs = top->cs;
	*ip = top->ip;
	return vm;
}

vm_t vm_peek_catcher(vm_t vm, program_t* cs, word_t* ip) {
	generic_stack_t stack = &vm->current_thread->catch_stack;
	struct _call_stack_entry_t* top = gpeek( struct _call_stack_entry_t*, stack, 0 );
	*cs = top->cs;
	*ip = top->ip;
	return vm;
}


vm_data_t _VM_CALL _vm_peek(vm_t vm) {
	return (vm_data_t) _gpeek(&vm->current_thread->data_stack,0);
}


vm_data_t _VM_CALL _vm_pop(vm_t vm) {
	generic_stack_t stack = &vm->current_thread->data_stack;
	vm_data_t top = _gpop(stack);
	if(top->type&DataManagedObjectFlag) {
		vm_obj_deref_ptr(vm,(void*)top->data);
	}
	return top;
}


vm_t _VM_CALL vm_collect(vm_t vm, vm_obj_t o) {
	dlist_node_t dn;
	assert(o->ref_count==0);
	/* also assert that obj is not yet collected */
	dn = vm->gc_pending.head;
	while(dn&&(void*)dn->value!=o) { dn = dn->next; }
	if(dn) {
		vm_printerrf("[VM:ERR] object %p is already collected ! collection aborted.\n",o);
	} else {
		dlist_insert_head(&vm->gc_pending,o);
	}
	return vm;
}

vm_t _VM_CALL vm_uncollect(vm_t vm, vm_obj_t o) {
	dlist_node_t dn;
	/*vm_printf("vm uncollect %p\n",o);*/
	if(!vm->gc_pending.head) {
		/*vm_printf("   => nothing to do.\n");*/
		return vm;
	}
	dn = vm->gc_pending.head;
	do {
		if(dn->value==(word_t)o) {
			dlist_remove(&vm->gc_pending,dn);
			return vm;
		}
		dn = dn->next;
	} while(dn);
	/*vm_printf("   => failed.\n");*/
	return vm;
}

vm_t vm_pop_data(vm_t vm, word_t count) {
	/*generic_stack_t stack = &vm->current_thread->data_stack;*/
	while(count>0) {
		_vm_pop(vm);
		count-=1;
	}
	return vm;
}

vm_t vm_pop_caller(vm_t vm, word_t count) {
	generic_stack_t stack = &vm->current_thread->call_stack;
	call_stack_entry_t cse;
	while(count>0) {
		cse = _gpop(stack);
		if(cse->has_closure) {
			_gpop(&vm->current_thread->closures_stack);
		}
		if(cse->df_callee) {
			vm_obj_deref_ptr(vm, cse->df_callee);
		}
		count-=1;
	}
	return vm;
}

vm_t vm_pop_catcher(vm_t vm, word_t count) {
	generic_stack_t stack = &vm->current_thread->catch_stack;
	while(count>0) {
		_gpop(stack);
		count-=1;
	}
	return vm;
}





thread_state_t _VM_CALL vm_exec_cycle(vm_t vm, thread_t t) {
	word_t*array;
	word_t state_change_ndata;
/*
	if(t->IP>=t->program->code.size) {
		t->state=ThreadDying;
		return;
	}
*/
	/* fetch */
	/*program_fetch(t->program, t->IP, (word_t*)&op, &arg);*/
	array = t->program->code.data+t->IP;

//*
	if(_vm_trace) {
/*		const char* label = program_lookup_label(t->program,t->IP);*/
		static char ip_lbl_ofs_buf[128];
		word_t ofs;
		const char*label;
		const char* disasm = program_disassemble(vm,t->program,t->IP);
		lookup_label_and_ofs(t->program,t->IP,&label,&ofs);
		snprintf(ip_lbl_ofs_buf, 127, "(%s:%s+%li", t->program->source, label, ofs);
		
		fprintf(stdout,"[VM:EXEC:%p:%5.5lu] %s\n", t, t->IP, ip_lbl_ofs_buf);
		if(strlen(disasm)>80) {
			fprintf(stdout,"[VM:EXEC]\t%-77.77s...\n", disasm);
		} else {
			fprintf(stdout,"[VM:EXEC]\t%-80.80s\n", disasm);
		}
		fflush(stdout);
		free((char*)disasm);
		state_change_ndata = t->data_stack.sp;
	}
// */

	/* decode */
	if(*array&&!setjmp(_glob_vm_jmpbuf)) {
		t->data_sp_backup = t->data_stack.sp;
		((opcode_stub_t) *array) ( vm, *(array+1) );
	}

	if(_vm_trace&&(t->jmp_ofs||state_change_ndata!=t->data_stack.sp)) {
		fputs("[VM:EXEC]\t`: ", stdout);
		if(t->jmp_ofs) {
			fprintf(stdout," JMP=>[%s:%li]", program_get_source(t->jmp_seg), t->jmp_ofs);
		}
		if(t->data_stack.sp>state_change_ndata) {
			vm_data_t d;
			int ofs;
			fprintf(stdout, " Stack size %lu, pushed", t->data_stack.sp);
			for(ofs=state_change_ndata-t->data_stack.sp+1;ofs<=0;ofs+=1) {
				fputc(' ', stdout);
				vm_print_data(vm, gpeek(vm_data_t , &t->data_stack, ofs));
			}
		} else if (t->data_stack.sp<state_change_ndata) {
			fprintf(stdout, "Stack size %lu, pop'd %li", t->data_stack.sp, state_change_ndata-t->data_stack.sp);
			if(t->data_stack.sp!=(word_t)-1) {
				fprintf(stdout, ", stack top is ");
				vm_print_data(vm, gpeek(vm_data_t, &t->data_stack, 0));
			}
		}
		fputc('\n', stdout);
	}

	if(vm->current_thread) {
		/* iterate */
		if(t->jmp_ofs) {
			/* it's a jump */
			/* assume call stack is already filled */
			t->program = t->jmp_seg;
			t->IP=t->jmp_ofs;
			t->jmp_ofs=0;
		} else if(t->IP==t->program->code.size-2) {
			/* we are at the end of the segment, thread should die. */
			if(t->state!=ThreadDying) {
				thread_set_state(vm,t,ThreadDying);
			}
		} else {
			/* hop to next instruction */
			t->IP+=2;
		}
		if(t->IP&0x80000000) {	/* sign bit is illegal, kill thread because of faulty jump */
			if(t->state!=ThreadDying) {
				thread_set_state(vm,t,ThreadDying);
			}
		}
		return t->state;
	} else {
		return ThreadZombie;
	}
}





typedef thread_t (*_VM_CALL _sched_method_t) (vm_t);



thread_t _VM_CALL _sched_idle(vm_t vm) {
	return NULL;
}



thread_t _VM_CALL _sched_mono(vm_t vm) {
	register thread_t current=NULL;
	switch((word_t)vm->current_thread) {
	case 0:
		if(vm->running_threads.head) {
			vm->current_thread=(thread_t)vm->running_threads.head;
			return vm->current_thread;
		} else if(vm->ready_threads.head) {
			vm->current_thread=(thread_t)vm->ready_threads.head;
			thread_set_state(vm,vm->current_thread,ThreadRunning);
			return current;
		} else {
			return NULL;
		}
	default:
		if(vm->current_thread->state==ThreadRunning) {
			return vm->current_thread;
		}
		return NULL;
	};
}



thread_t _VM_CALL _sched_rr(vm_t vm) {
	register thread_t current=vm->current_thread;

	/* quick pass if current meets conditions */
	if(current) {
		/*vm_printf("\thave current thread %p, state=%i, remaining=%li\n",current,current->state,current->remaining);*/
		if(current->state==ThreadRunning) {
			if(current->remaining>0 || current->exec_flags&IN_CRITICAL_SECTION) {
				/*vm_printf("\tcurrent still running for %lu more cycles%s\n",current->remaining, current->exec_flags&IN_CRITICAL_SECTION ? " (critical section)" : "");*/
				return current;
			} else if(vm->current_thread->sched_data.next) {
				vm->current_thread = (thread_t)vm->current_thread->sched_data.next;
				/*vm_printf("\tnext thread\n");*/
			} else {
				vm->current_thread = NULL;
			}
		}
	}
	if((!vm->current_thread)&&vm->running_threads.head) {
		vm->current_thread = (thread_t)vm->running_threads.head;
		/*vm_printf("\tnext thread (looped back to head)\n");*/
	} else {
		vm->current_thread=NULL;
	}
	/* solve the conflict between next ready and next running threads, if any */
	if(vm->ready_threads.head) {
		dlist_node_t a = &vm->current_thread->sched_data;
		dlist_node_t b = vm->ready_threads.head;
		thread_t tb = node_value(thread_t,b);
		if((!a)||node_value(thread_t,a)->prio <= tb->prio) {
			/* thread is no more ready */
			thread_set_state(vm,tb,ThreadRunning);
			vm->current_thread = (thread_t)b;
			/*vm_printf("\trunning next ready thread\n");*/
		}
	}

	if(vm->current_thread) {
		vm->current_thread->remaining=vm->timeslice;
		/*vm_printf("\tthread %p restarts for %li cycles\n",vm->current_thread,vm->current_thread->remaining);*/
		vm_printf("\tcurrent thread is %p  (%li cycles to go)\n", vm->current_thread, vm->current_thread->remaining);
		return vm->current_thread;
	} else {
		return NULL;
	}
}



_sched_method_t schedulers[SchedulerAlgoMax] = {
	_sched_idle,
	/*_sched_mono,*/
	_sched_rr,
	_sched_rr,
};

/* if has no thread, do nothing.
 * if has one thread, execute it.
 * if has many threads, time-slice round-robin them.
 */
void _VM_CALL vm_schedule_cycle(vm_t vm) {
	thread_t current;

	vm->engine->_client_lock(vm->engine);

	current = schedulers[vm->scheduler](vm);

/*	vm_printf("CURRENT THREAD %p\n",current);
	vm_printf("CURRENT PROGRAM %p\n",current->program);
	if(current) {
*/
	if(current) {
		if(vm->engine->_debug) {
			vm->engine->_debug(vm->engine);
		}
		current->remaining -= 1;
		if(current&&vm_exec_cycle(vm,current)==ThreadDying) {
			/* current thread may be already dead */
			if(vm->current_thread) {
				/*vm_printf("Dead zombie\n");*/
				vm_kill_thread(vm, current);
			}
			if(vm->zombie_threads.head) {
				dlist_node_t tmp=vm->zombie_threads.head;
				do {
					if(vm_obj_refcount_ptr(tmp)==0) {
						/* TODO */
					}
					tmp = tmp->next;
				} while(tmp);
			}
		}

/*
		do {
			dlist_node_t dn = vm->gc_pending.head;
			vm_printf(" *** - gc_pending :");
			while(dn) {
				vm_printf(" 0x%lx",dn->value);
				dn = dn->next;
			}
			puts(" - ***");
		} while(0);
// */		
		/* FIXME : hardcoded incremental finalization */
		if(vm->gc_pending.tail) {
			dlist_node_t dn;
			dn = vm->gc_pending.tail;
			/*if(((vm_obj_t)dn->value)->ref_count!=0) {*/
				/*vm_printerrf("dn->magic==%X\n", ((vm_obj_t)dn->value)->magic);*/
			/*}*/
			/*vm_printf("[VM:INFO] collecting %p : %X\n",dn->value, ((vm_obj_t)dn->value)->magic);*/
			assert(((vm_obj_t)dn->value)->ref_count==0);
			vm->gc_pending.tail=dn->prev;
			if(dn->prev) {
				dn->prev->next=NULL;
			} else {
				vm->gc_pending.head=NULL;
			}
			vm_obj_free_obj(vm,(void*)dn->value);
			free(dn);
		}

		vm->cycles+=1;
	}
	vm->engine->_client_unlock(vm->engine);
}

vm_t vm_set_engine(vm_t vm, vm_engine_t e) {
	if(vm->engine) {
		/*vm->engine->deinit(vm->engine);*/
		/* deinit */
	}
	vm->engine=e;
	e->vm=vm;
	/*e->_init(e);*/
	return vm;
}





void _vm_assert_fail(const char* assertion, const char*file, unsigned int line, const char* function) {
	if(strncmp(function,"vm_op_",6)) {
		vm_printf( "[VM:FATAL] In function `%s' at %s:%u : %s\n", function, file, line, assertion);
	} else {
		vm_printf( "[VM:FATAL] In opcode `%s' at %s:%u : %s\n", function+6, file, line, assertion);
		if(!strcmp(function+6, "throw") &&
				(_glob_vm->exception.type==DataString ||
					_glob_vm->exception.type==DataObjStr)) {
			vm_printf("Exception String : %s\n", (char*) _glob_vm->exception.data);
			
		}
	}
	if(_glob_vm&&_glob_vm->current_node) {
		vm_printf("[VM:INFO] While compiling ");
		vm_print_compilation_source(_glob_vm,0);
		vm_printf(" ");
		vm_printf("at %i:%i (%s)\n", wa_row(_glob_vm->current_node), wa_col(_glob_vm->current_node), wa_op(_glob_vm->current_node));
	}
	if(_glob_vm&&_glob_vm->current_thread) {
		thread_t t = _glob_vm->current_thread;
		_glob_vm->current_thread = NULL;
		vm_printf("[VM:NOTICE] Killing current thread %p\n", t);
		_glob_vm->engine->_thread_failed(_glob_vm,t);
		vm_kill_thread(_glob_vm,t);
		longjmp(_glob_vm_jmpbuf,1);
	} else {
		abort();
	}
}



int vm_printf(const char* fmt, ...) {
	char buffer[4096];
	va_list ap;
	va_start(ap,fmt);
	vsnprintf(buffer, 4096, fmt, ap);
	va_end(ap);
	_glob_vm->engine->_put_std(buffer);
	return strlen(buffer);
}


int vm_printerrf(const char* fmt, ...) {
	char buffer[4096];
	va_list ap;
	va_start(ap,fmt);
	vsnprintf(buffer, 4096, fmt, ap);
	va_end(ap);
	_glob_vm->engine->_put_err(buffer);
	return strlen(buffer);
}

void _VM_CALL e_run_subthread(vm_engine_t e, program_t p, word_t ip, word_t prio);

thread_t vm_exec_dynFun(vm_t vm, vm_dyn_func_t df) {
	return NULL;
}


