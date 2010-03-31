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
volatile long _vm_trace = 0;

jmp_buf _glob_vm_jmpbuf;

const char* thread_state_to_str(thread_state_t ts);

program_t dynFun_exec = NULL;

const char* vm_find_innermost_file(vm_t vm) {
	long ofs=0;
	vm_data_t d;
	do {
		d = (vm_data_t) _gpeek(&vm->compinput_stack, 0);
	} while((ofs=ofs+1)<vm->compinput_stack.sp&&CompInFile!=(compinput_t)d->type);
	if(CompInFile!=(compinput_t)d->type) {
		return "buffer";
	}
	return (const char*)d->data;
}

void vm_print_compilation_source(vm_t vm, long ofs) {
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
	case DataObjVCls:
		vm_printf("[V-Cls  %p]",(void*)tmp.i);
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

extern const vm_engine_t stub_engine;

word_t raise_exception(vm_t vm, vm_data_type_t dt, word_t exc) {
	call_stack_entry_t cse;
	if(dt&DataManagedObjectFlag) {
		vm_obj_ref_ptr(vm,(void*)exc);
	}
	vm->exception.data = exc;
	vm->exception.type= dt;
	if((long)vm->current_thread->catch_stack.sp>=0) {
		cse = _gpop(&vm->current_thread->catch_stack);
		/*vm_printf("throw : uninstall catcher %p:%lu\n",cse->cs,cse->ip);*/
		vm->current_thread->jmp_seg=cse->cs;
		vm->current_thread->jmp_ofs=cse->ip;
		while((long)vm->current_thread->call_stack.sp>(long)cse->has_closure) {
			(void)vm_pop_caller(vm,1);
		}
		/*vm_push_data(vm,e->type,e->data);*/
	} else {
		char buf[256];
		snprintf(buf, 255, "Uncaught exception [%s]. Aborting.", /* FIXME */ (char*) exc);
		vm_fatal(buf);
	}
	return 0;
}

int find_in_bases(vm_t vm, vm_data_t haystack, vobj_class_t needle) {
	vobj_class_t tmp;
	int i;
	dynarray_t da;
	/* single inheritance ? */
	switch(haystack->type) {
	case DataObjVObj:
		tmp = ((vobj_ref_t)haystack->data)->cls;
		return needle==tmp || find_in_bases(vm, &tmp->_base, needle);
	case DataObjVCls:
		tmp = (vobj_class_t) haystack->data;
		return needle==tmp || find_in_bases(vm, &tmp->_base, needle);
	case DataObjArray:
		da = (dynarray_t) haystack->data;
		for(i=0;i<dynarray_size(da);i+=1) {
			tmp = (vobj_class_t) dynarray_get(da, i);
			if(tmp==needle||find_in_bases(vm, &tmp->_base, needle)) {
				return 1;
			}
		}
		return 0;
	default:;
		return 0;
	};
	
}

static inline vm_data_t _safe_numeric(vm_t vm, vm_data_t d) {
	if(d->type!=DataInt&&d->type!=DataFloat) {
		_IFC conv;
		int fail=0;
		conv.i = dynamic_cast(vm, d, DataFloat, NULL, &fail);
		if(fail) {
			conv.i = dynamic_cast(vm, d, DataInt, NULL, &fail);
			if(fail) {
				raise_exception(vm, DataString, (word_t) "Type");
			} else {
				d->type=DataInt;
			}
		} else {
			d->type=DataFloat;
		}
		d->data=conv.i;
	}
	return d;
}

vm_data_t _VM_CALL vm_pop_numeric(vm_t vm) {
	return _safe_numeric(vm, _vm_pop(vm));
}

vm_data_t _VM_CALL vm_peek_numeric(vm_t vm) {
	return _safe_numeric(vm, _vm_peek(vm));
}

static inline vm_data_t _safe_obj(vm_t vm, vm_data_t d) {
	if(d->type&DataManagedObjectFlag) {
		return d;
	}
	raise_exception(vm, DataString, (word_t) "Type");
	return d;
}

vm_data_t _VM_CALL vm_pop_obj(vm_t vm) {
	return _safe_obj(vm, _vm_pop(vm));
}

vm_data_t _VM_CALL vm_peek_obj(vm_t vm) {
	return _safe_obj(vm, _vm_peek(vm));
}

static inline vm_data_t _safe_any(vm_t vm, vm_data_type_t dt, vm_data_t d) {
	if(d->type==dt) {
		return d;
	}
	d->data = dynamic_cast(vm, d, dt, NULL, NULL);
	d->type=dt;
	return d;
}

vm_data_t _VM_CALL vm_pop_any(vm_t vm, vm_data_type_t dt) {
	return _safe_any(vm, dt, _vm_pop(vm));
}

vm_data_t _VM_CALL vm_peek_any(vm_t vm, vm_data_type_t dt) {
	return _safe_any(vm, dt, _vm_peek(vm));
}


word_t dynamic_cast(vm_t vm, vm_data_t d, vm_data_type_t newtype, vobj_class_t newcls, int*fail) {
	_IFC conv;
	vobj_class_t objcls;
	dynarray_t da;
	vm_dyn_func_t* cast;
	vm_dyn_func_t df;
	char* f2str;
	if(_vm_trace) {
		vm_printf("dynamic_cast from (%i:%8.8X) to %i/%p\n", d->type, d->data, newtype, newcls);
	}
	fail?*fail=0:(void)0;
	if(newcls!=NULL) {
		/*vm_printf("dc to class instance\n");*/
		switch(d->type) {
		case DataObjVCls:
		case DataObjVObj:
			objcls = ((vobj_ref_t)d->data)->cls;
			if(newcls==objcls) {
				return d->data;
			}
			/* search bases */
			return find_in_bases(vm, &objcls->_base, newcls)
					? d->data
					: (fail	? *fail=1
						: raise_exception(vm, DataString, (word_t)"Type"));
		default:
			cast = newcls->_cast_from;
			df = cast[d->type];
			if(df) {
				stub_engine->_run_sync((vm_engine_t)&stub_engine, df->cs, df->ip, vm->current_thread->prio);
				d=_vm_pop(vm);
				return d->data;
			}
		};
	} else if(newtype==DataObjVObj&&(d->type==DataObjVObj||d->type==DataObjVCls)) {
		/*vm_printf("dc raw obj\n");*/
		return d->data;
	}
	if(d->type==newtype||d->type==DataObjVCls&&newtype==DataObjVObj) {
		/*vm_printf("dc idem or class->obj\n");*/
		return d->data;
	}
	switch(d->type) {
	case DataObjVCls:
	case DataObjVObj:
		/*vm_printf("dc from class/obj\n");*/
		objcls = ((vobj_ref_t)d->data)->cls;
		cast = objcls?objcls->_cast_to:NULL;
		df = cast?cast[newtype]:NULL;
		if(df) {
			stub_engine->_run_sync((vm_engine_t)&stub_engine, df->cs, df->ip, vm->current_thread->prio);
			d=_vm_pop(vm);
			return d->data;
		}
		raise_exception(vm, DataString, (word_t) "Type");
		break;
	case DataInt:
		/*vm_printf("dc from int\n");*/
		switch(newtype) {
		case DataChar:
			return d->data;
		case DataFloat:
			return i2f(d->data);
		case DataString:
		case DataObjStr:
			return atoi((const char*)d->data);
		default:;
		};
		break;
	case DataFloat:
		/*vm_printf("dc from float\n");*/
		switch(newtype) {
		case DataInt:
			conv.i = d->data;
			return f2i(conv.f);
		#if 0
		case DataString:
		case DataObjStr:
			conv.i = d->data;
			f2str = vm_string_new(32);
			vm_obj_deref_ptr(f2str); /* collect soon */
			sprintf(f2str, "%f", conv.f);
			return f2str;
		#endif
		default:;
		};
		break;
	case DataString:
	case DataObjStr:
		/*vm_printf("dc from string\n");*/
		switch(newtype) {
		case DataInt:
			return atoi((const char*)d->data);
		case DataFloat:
			conv.f = atof((const char*)d->data);
			return conv.i;
		case DataString:
		case DataObjStr:
			return d->data;
		default:;
		};
		break;
	case DataObjEnv:
		switch(newtype) {
		case DataObjSymTab:
			return (word_t) (&((vm_dyn_env_t)d->data)->symbols);
		case DataObjArray:
			return (word_t) (&((vm_dyn_env_t)d->data)->data);
		default:;
		};
	default:;
		/*vm_printf("dc fail\n");*/
		fail?*fail=1:raise_exception(vm, DataString, (word_t) "Type");
	};
	return 0;
}


extern volatile long line_number_bias;


void default_error_handler_no_exit(vm_t vm, const char* input, long is_buffer) {
	long ofs, sz;
	/*compinput_t ci;*/
	/*const char* buf;*/
	/*vm_data_t d;*/
	wast_t wa;
	sz = vm->compinput_stack.sp;
	vm_printerrf("Error during compilation :\n");
	for(ofs=-sz;ofs<=0;ofs+=1) {
		vm_printf(" * in ");
		vm_print_compilation_source(vm, ofs);
		vm_printf("\n");
	}
	sz = vm->cn_stack.sp;
	if(sz>0) {
		vm_printf("Node stack :\n");
		for(ofs=-sz;ofs<=0;ofs+=1) {
			wa = *(wast_t*) _gpeek(&vm->cn_stack, ofs);
			if(wa) {
				vm_printf(" * %i:%i %s (%i)\n", wa_row(wa)+line_number_bias, wa_col(wa), wa_op(wa), wa_opd_count(wa));
			}
		}
	}
	if(sz==0) {
		vm_printf("at %i:%i (%s)\n", wa_row(vm->current_node)+line_number_bias, wa_col(vm->current_node), wa_op(vm->current_node));
	}
}

void default_error_handler(vm_t vm, const char* input, long is_buffer) {
	default_error_handler_no_exit(vm, input, is_buffer);
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


void _VM_CALL vm_op_envAdd(vm_t vm, word_t unused);
void _VM_CALL vm_op_call(vm_t vm, word_t unused);
void _VM_CALL vm_op_call_vc(vm_t vm, word_t unused);



/* the VM is a singleton */
vm_t vm_new() {
	vm_t ret;
	/*struct _thread_t th;*/
	word_t index;

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

#define _SETENV(__key) \
	index = text_seg_text_to_index(&ret->env->symbols,text_seg_find_by_text(&ret->env->symbols, #__key))<<1;\
	dynarray_set(&ret->env->data, index+1, (word_t) __key); \
	dynarray_set(&ret->env->data, index, (word_t) DataInt);

	_SETENV(DataNone);
	_SETENV(DataInt);
	_SETENV(DataChar);
	_SETENV(DataFloat);
	_SETENV(DataString);
	_SETENV(DataManagedObjectFlag);
	_SETENV(DataObjStr);
	_SETENV(DataObjSymTab);
	_SETENV(DataObjMutex);
	_SETENV(DataObjThread);
	_SETENV(DataObjArray);
	_SETENV(DataObjEnv);
	_SETENV(DataObjStack);
	_SETENV(DataObjFun);
	_SETENV(DataObjVObj);
	_SETENV(DataObjVCls);
	_SETENV(DataObjUser);

	_SETENV(OpcodeNoArg);
	_SETENV(OpcodeArgInt);
	_SETENV(OpcodeArgChar);
	_SETENV(OpcodeArgFloat);
	_SETENV(OpcodeArgLabel);
	_SETENV(OpcodeArgString);
	_SETENV(OpcodeArgEnvSym);

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
		/*vm_printerrf("[VM:DBG] free obj @%p\n", dn);*/
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


void vm_set_timeslice(vm_t vm, long timeslice) {
	vm->timeslice=timeslice;
}

long comp_prio(dlist_node_t a, dlist_node_t b) {
	return (long)(node_value(thread_t,b)->prio - node_value(thread_t,a)->prio);
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
program_t compile_append_wast(wast_t, vm_t, word_t*, long);
void wa_del(wast_t w);

program_t vm_compile_any(vm_t vm, word_t* start_IP, long last) {
	program_t p=NULL;
	word_t reent = vm->compile_reent;
	vm->compile_reent=0;

	tinyap_parse(vm->parser);

	if(tinyap_parsed_ok(vm->parser)&&tinyap_get_output(vm->parser)) {
		wast_t wa = tinyap_make_wast( tinyap_list_get_element( tinyap_get_output(vm->parser), 0) );
		p = compile_append_wast(wa, vm, start_IP, last);
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

program_t vm_compile_append_file(vm_t vm, const char* fname, word_t* start_IP, long last) {
	/*vm_printf("vm_compile_file(%s)\n",fname);*/
	tinyap_set_source_file(vm->parser,fname);
	vm_compinput_push_file(vm, fname);
	return vm_compile_any(vm, start_IP, last);
}

program_t vm_compile_file(vm_t vm, const char* fname) {
	word_t zero;
	program_t backup = vm->current_edit_prg, ret;
	vm->current_edit_prg = program_new();
	ret = vm_compile_append_file(vm, fname, &zero, 1);
	vm->current_edit_prg = backup;
	return ret;
}


program_t vm_compile_append_buffer(vm_t vm, const char* buffer, word_t* start_IP, long last) {
	/*vm_printf("vm_compile_buffer(%s)\n",buffer);*/
	tinyap_set_source_buffer(vm->parser,buffer,strlen(buffer));
	vm_compinput_push_buffer(vm, buffer);
	return vm_compile_any(vm, start_IP, last);
}


program_t vm_compile_buffer(vm_t vm, const char* buffer) {
	word_t zero;
	program_t backup = vm->current_edit_prg, ret;
	vm->current_edit_prg = program_new();
	ret = vm_compile_append_buffer(vm, buffer, &zero, 1);
	vm->current_edit_prg = backup;
	return ret;
}



vm_t vm_run_program_bg(vm_t vm, program_t p, word_t ip, word_t prio) {
	if(p) {
		vm->engine->_run_async(vm->engine, p, ip, prio);
	}
	return vm;
}


vm_t vm_run_program_fg(vm_t vm, program_t p, word_t ip, word_t prio) {
	if(p) {
		vm->engine->_run_sync(vm->engine, p, ip, prio);
	}
	return vm;
}

/* FIXME : include support for thread args */
thread_t vm_add_thread_helper(vm_t vm, thread_t t, long fg) {
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

thread_t vm_add_thread(vm_t vm, program_t p, word_t ip, word_t prio,long fg) {
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
	long sz = gstack_size(stack);
	long i;

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


vm_t vm_peek_data(vm_t vm, long rel_ofs, vm_data_type_t* type, word_t* value) {
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




void lookup_label_and_ofs(program_t cs, word_t ip, const char** label, word_t* ofs);



vm_dyn_func_t find_overload(vobj_ref_t obj, opcode_stub_t stub) {
	hashtab_t oo = obj&&obj->cls ? &obj->cls->_overloads : NULL;
	vm_data_t d = oo?hash_find(oo, (hash_key) stub):NULL;
	printf("   find_overload obj=%p cls=%p oo=%p stub=%p => dt=%lu d=%8.8lX\n", obj, obj?obj->cls:NULL, oo, stub, d?d->type:0, d?d->data:0);
	return d?dynamic_cast(_glob_vm, d, DataObjFun, NULL, NULL):NULL;
}


thread_state_t _VM_CALL vm_exec_cycle(vm_t vm, thread_t t) {
	word_t*array;
	word_t offset;
	word_t state_change_ndata;
	vobj_ref_t obj;
	vm_data_t d;
	word_t argc;
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
		/* check if it has to be an object message passing */
		vm_dyn_func_t method = NULL;
		opcode_stub_overload_t ovl = opcode_overloads_by_stub(&vm->opcodes, (opcode_stub_t)*array);
		while(ovl&&!method) {
			/*if(_vm_trace) { printf("overload offset %i\n", ovl->offset); }*/
			if(gstack_size(&t->data_stack)>=ovl->offset) {
				offset = ovl->offset;
				d = gpeek( vm_data_t , &t->data_stack, -offset);
				obj = (vobj_ref_t) d->data;
				method = d->type==DataObjVObj?find_overload(obj, ovl->target):NULL;
				argc=ovl->argc;
				/*if(_vm_trace) {*/
					/*printf(" dt=%i d=%8.8X\n method=%p\n", d->type, d->data, method);*/
				/*}*/
			}
			ovl=ovl->next;
		}
		if(method) {
			if(_vm_trace) {
				fprintf(stdout,"[VM:EXEC]\t shadowed by v-obj !\n");
			}
			/* prepare stack */
			switch(offset) { 	/* remove object reference from stack (first, move it to top of stack) */
			case 0: break;
			case 1:	vm_op_swap_Int(vm, 1);
				break;
			default:
				memmove(_gpeek(&t->data_stack, -offset), _gpeek(&t->data_stack, 1-offset), (offset-1)*t->data_stack.token_size);
			};
			_gpop(&t->data_stack); 		/* drop object reference */
			switch(WC_GET_ARGTYPE(opcode_code_by_stub(&vm->opcodes, (opcode_stub_t) *array))) {
			case OpcodeArgInt:
				vm_push_data(vm, DataInt, *(array+1));
				argc+=1;
				break;
			case OpcodeArgChar:
				vm_push_data(vm, DataChar, *(array+1));
				argc+=1;
				break;
			case OpcodeArgFloat:
				vm_push_data(vm, DataFloat, *(array+1));
				argc+=1;
				break;
			case OpcodeArgLabel:
				vm_push_data(vm, DataInt, *(array+1));
				argc+=1;
				break;
			case OpcodeArgString:
				vm_push_data(vm, DataString, *(array+1));
				argc+=1;
				break;
			case OpcodeArgEnvSym:
				vm_push_data(vm, DataInt, *(array+1));
				argc+=1;
				break;
			case OpcodeNoArg:
				break;
			case OpcodeArgPtr:
			case OpcodeTypeMax:
				raise_exception(vm, DataString, (word_t) "OvlArg");
			};
			vm_push_data(vm, DataInt, argc); /* FIXME? calling convention : pop argc first */
			vm_push_data(vm, DataObjArray, (word_t) obj->members);
			vm_push_data(vm, DataObjFun, (word_t)method); 	/* setup stack for call_vc */
			vm_op_call_vc(vm, 0);
		} else {
			/*if(_vm_trace) {*/
				/*vm_printf("  (no overload)\n");*/
			/*}*/
			((opcode_stub_t) *array) ( vm, *(array+1) );
		}
	}

	if(_vm_trace&&(t->jmp_ofs||state_change_ndata!=t->data_stack.sp)) {
		fputs("[VM:EXEC]\t`: ", stdout);
		if(t->jmp_ofs) {
			fprintf(stdout," JMP=>[%s:%li]", program_get_source(t->jmp_seg), t->jmp_ofs);
		}
		if(t->data_stack.sp>state_change_ndata) {
			/*vm_data_t d;*/
			long ofs;
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
				vm->current_thread = (thread_t)vm->current_thread->sched_data.next->value;
				/*vm_printf("(%li) next thread in list : %p\n", vm->cycles, vm->current_thread);*/
			} else if(vm->running_threads.head) {
				vm->current_thread = (thread_t)vm->running_threads.head->value;
				/*vm_printf("(%li) next thread (looped back to head) : %p\n", vm->cycles, vm->current_thread);*/
			} else {
				vm->current_thread = NULL;
			}
		} else if(vm->running_threads.head) {
			vm->current_thread = (thread_t)vm->running_threads.head->value;
			/*vm_printf("(%li) next thread (looped back to head) : %p\n", vm->cycles, vm->current_thread);*/
		} else {
			vm->current_thread = NULL;
		}
	} else if(vm->running_threads.head) {
		vm->current_thread = (thread_t)vm->running_threads.head->value;
		/*vm_printf("(%li) next thread (looped back to head) : %p\n", vm->cycles, vm->current_thread);*/
	}
	/*vm_printf("(%li)    vm->current_thread = %p\n", vm->cycles, vm->current_thread);*/
	/* solve the conflict between next ready and next running threads, if any */
	if(vm->ready_threads.head) {
		thread_t tb = node_value(thread_t,vm->ready_threads.head);
		if((!vm->current_thread)||vm->current_thread->prio <= tb->prio) {
			/* thread is no more ready */
			vm->current_thread = tb;
			/*vm_printf("(%li)   running next ready thread %p\n", vm->cycles, vm->current_thread);*/
			thread_set_state(vm,tb,ThreadRunning);
		}
	}

	if(vm->current_thread) {
		vm->current_thread->remaining=vm->timeslice;
		/*vm_printf("\tthread %p restarts for %li cycles\n",vm->current_thread,vm->current_thread->remaining);*/
		/*vm_printf("(%li) current thread is %p  (%li cycles to go)\n", vm->cycles, vm->current_thread, vm->current_thread->remaining);*/
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
	register thread_t current;

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





void _vm_assert_fail(const char* assertion, const char*file, unsigned long line, const char* function) {
	if(strncmp(function,"vm_op_",6)) {
		vm_printerrf( "[VM:FATAL] In function `%s' at %s:%u : %s\n", function, file, line, assertion);
	} else {
		vm_printerrf( "[VM:FATAL] In opcode `%s' at %s:%u : %s\n", function+6, file, line, assertion);
		if(!strcmp(function+6, "throw") &&
				(_glob_vm->exception.type==DataString ||
					_glob_vm->exception.type==DataObjStr)) {
			vm_printerrf("Exception String : %s\n", (char*) _glob_vm->exception.data);
			
		}
	}
	if(_glob_vm&&_glob_vm->current_node) {
		vm_printerrf("[VM:INFO] While compiling ");
		vm_print_compilation_source(_glob_vm,0);
		vm_printerrf(" ");
		vm_printerrf("at %i:%i (%s)\n", wa_row(_glob_vm->current_node), wa_col(_glob_vm->current_node), wa_op(_glob_vm->current_node));
	}
	if(_glob_vm&&_glob_vm->current_thread) {
		thread_t t = _glob_vm->current_thread;
		_glob_vm->current_thread = NULL;
		vm_printerrf("[VM:NOTICE] Killing current thread %p\n", t);
		_glob_vm->engine->_thread_failed(_glob_vm,t);
		vm_kill_thread(_glob_vm,t);
		longjmp(_glob_vm_jmpbuf,1);
	} else {
		abort();
	}
}



long vm_printf(const char* fmt, ...) {
	char buffer[4096];
	va_list ap;
	va_start(ap,fmt);
	vsnprintf(buffer, 4096, fmt, ap);
	va_end(ap);
	_glob_vm->engine->_put_std(buffer);
	return strlen(buffer);
}


long vm_printerrf(const char* fmt, ...) {
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


