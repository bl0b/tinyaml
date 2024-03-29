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

#include <tinyape.h>

#include <limits.h>	/* for PATH_MAX */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "vm_assert.h"

#include "vm.h"
#include "list.h"
#include "opcode_chain.h"
#include "opcode_dict.h"
#include "program.h"
#include "_impl.h"
#include "text_seg.h"


word_t	docn_dat=0,docn_cod=0;

const char* state2str(WalkDirection wd) {
	static const char* wdStr[5] = { "Up", "Down", "Next", "Done", "Error" };
	return wdStr[wd];
}

void* try_walk(wast_t node, const char* pname, vm_t vm) {
	void* ret;
	wast_t backup_node = vm->current_node;
	/*gpush(&vm->cn_stack,&vm->current_node);*/
	/*vm->current_node = node;*/
	/*vm_printerrf("start walking (vwalker=%s)...\n", vm->virt_walker);*/
	ret = tinyap_walk(node, pname, vm);
	/*vm->current_node=*(wast_t*)_gpop(&vm->cn_stack);*/
	/*vm_printerrf("done walking... (vwalker=%s, state=%s)\n", vm->virt_walker, state2str(vm->compile_state));*/
	vm->compile_error = (vm->compile_state==Error);
	if(vm->compile_error) {
		vm->onCompileError(vm, "<not implemented>", 0);
		ret = NULL;
	}
	vm->current_node = backup_node;
	return ret;
}

void dump_ocn(opcode_chain_node_t ocn) {
	switch(ocn->type) {
	case NodeData:
		vm_printf("data_|%8.8lX\t%i:%s [%s]\n",docn_dat,ocn->arg_type,ocn->name,ocn->arg);
		docn_dat+=1;
		break;
	case NodeOpcode:
		switch(ocn->arg_type) {
		case OpcodeArgString:
			vm_printf("_asm_|%8.8lX\t%s\t\"%s\"\n",docn_cod,ocn->name,ocn->arg);
			break;
		case OpcodeArgLabel:
			vm_printf("_asm_|%8.8lX\t%s\t@%s\n",docn_cod,ocn->name,ocn->arg);
			break;
		default:
			vm_printf("_asm_|%8.8lX\t%s\t%s\n",docn_cod,ocn->name,ocn->arg?ocn->arg:"");
			break;
		};
		docn_cod+=2;
		break;
	case NodeLabel:
		vm_printf("label|\t    %s:\n",ocn->name);
		break;
	case NodeLangPlug:
		vm_printf("plug_| %s -> %s\n",ocn->name,ocn->arg);
		break;
	case NodeLangDef:
		vm_printf("gram_| (%i chars)\n",strlen(ocn->name));
		break;
	default:;
	};
}

void trie_dump(trie_t t, int indent);

program_t compile_append_wast(wast_t node, vm_t vm, word_t* start_IP, long last) {
	/*vm_printf("compile_wast\n");*/
    /*trie_t trie = tinyap_get_bow("opcode");*/
    /*printf("Dumping opcode tree %p\n", trie);*/
    /*trie_dump(trie, 0);*/
	/*try_walk(node, "prettyprint", vm);*/
	try_walk(node, "compiler", vm);
	vm_set_lib_file(vm,NULL);
	*start_IP = vm->current_edit_prg->code.size>>1;
	/*ret->env = vm->env;*/
	/*vm_printf("now %p\n",vm->result);*/
	if(last) {
		opcode_chain_add_opcode(vm->result, OpcodeArgInt, "ret", "0", -1, -1);
	}
	docn_dat=docn_cod=0;
	if(vm->result) {
		if(vm->result->head) {
			/*opcode_chain_apply(vm->result,dump_ocn);*/
			if(!vm->current_edit_prg->env) {
				vm->current_edit_prg->env = vm->env;
			}
			opcode_chain_serialize(vm->result, vm_get_dict(vm), vm->current_edit_prg, vm->dl_handle);
		}
		opcode_chain_delete(vm->result);
		vm->result=NULL;
	}
	/*vm_printf("\n-- New program compiled.\n-- Data size : %lu\n-- Code size : %lu\n\n",ret->data.size,ret->code.size);*/
	return vm->current_edit_prg;
}


program_t compile_wast(wast_t node, vm_t vm) {
	word_t zero;
	program_t backup = vm->current_edit_prg,ret;
	vm->current_edit_prg = program_new();
	ret = compile_append_wast(node, vm, &zero, 1);
	vm->current_edit_prg = backup;
	return ret;
}

void _VM_CALL vm_op_getmem_Int(vm_t, word_t);
void _VM_CALL vm_op_call(vm_t, word_t);
void _VM_CALL vm_op_ret_Int(vm_t, word_t);

void exec_routine(word_t df_ptr) {
	/*word_t cycles;*/
	vm_dyn_func_t fun = (vm_dyn_func_t) df_ptr;
	/*printf("[VM:DEBUG] About to exec dynFun @%p\n", (void*)df_ptr);*/
	/*cycles = _glob_vm->cycles;*/
	vm_run_program_fg(_glob_vm,fun->cs,fun->ip,50);
	/*printf("[VM:DEBUG] dynFun @%p ran in %u cycles\n", (void*)df_ptr, _glob_vm->cycles-cycles);*/
}


void* ape_compiler_init(vm_t vm) {
	/* allow reentrant calls */
	vm->compile_reent+=1;
/*	gpush(&vm->cn_stack,&vm->current_node);*/
	if(vm->compile_reent==1) {
/*		vm_printf("###      NEW       top-level compiler [at %p:%lX]\n",vm_get_CS(vm),vm_get_IP(vm));*/
		vm->result = opcode_chain_new();
		dlist_forward(&vm->init_routines, word_t, exec_routine);
/*	} else {*/
/*		vm_printf("###      NEW       sub-compiler [at %p:%lX]\n",vm_get_CS(vm),vm_get_IP(vm));*/
	}
	/*vm_printf("vm new ochain : %p\n",vm->result);*/
	return vm;
}


void ape_compiler_free(vm_t vm) {
/*	vm->current_node=*(wast_t*)_gpop(&vm->cn_stack);*/
	vm->compile_reent-=1;
	if(!vm->compile_reent) {
/*		vm_printf("###      DONE      top-level compiler [at %p:%lX]\n",vm_get_CS(vm),vm_get_IP(vm));*/
		dlist_reverse(&vm->term_routines, word_t, exec_routine);
/*	} else {*/
/*		vm_printf("###      DONE      sub-compiler [at %p:%lX]\n",vm_get_CS(vm),vm_get_IP(vm));*/
	}
}


extern volatile long _vm_trace;

WalkDirection ape_compiler_default(wast_t node, vm_t vm) {
	char* vec_name = (char*)malloc(strlen(wa_op(node))+10);
	sprintf(vec_name,".compile_%s",wa_op(node));
	word_t vec_ofs = (word_t)hash_find(&vm->compile_vectors.by_text,(hash_key)vec_name);

/*	vm_printf("search for vector %s in %p gave %lu\n",vec_name, vm->result, vec_ofs);*/

/*	vm_printerrf("ERROR ! %s:%i (cur state = %s)\n", __FILE__, __LINE__, state2str(vm->compile_state));*/
	vm->compile_state = Error;
	if(vec_ofs) {
		program_t p = (program_t)*(vm->compile_vectors.by_index.data+vec_ofs);
		word_t ip = *(vm->compile_vectors.by_index.data+vec_ofs+1);
/*		vm_printf("compiler calling %p:%lX (%s)\n",p,ip,wa_op(node));*/
		if(_vm_trace) {
			vm_printf("\nCOMPILE_VECTOR %s [%p:%lX]\n", wa_op(node), p,ip);
		}
		gpush(&vm->cn_stack,&vm->current_node);
		vm->current_node = node;
		vm_run_program_fg(vm,p,ip,50);
		vm->current_node=*(wast_t*)_gpop(&vm->cn_stack);
		free(vec_name);
/*		vm_printf("   vm return state : %i\n",vm->compile_state);*/
		if(_vm_trace) {
			vm_printf("\nCOMPILE_VECTOR %s return state : %s\n", wa_op(node), state2str(vm->compile_state));
		}
		return vm->compile_state;
	}

	free(vec_name);

	vm_printerrf("Node is not known '%s'\n",wa_op(node));
	return Error;
}


void* ape_compiler_result(vm_t vm) {
	return vm->result;
}

static inline WalkDirection update_vm_state(vm_t vm, WalkDirection wd) {
	vm->compile_error = (wd==Error);
	return vm->compile_state=wd;
}

WalkDirection ape_compiler_Program(wast_t node, vm_t vm) {
	return update_vm_state(vm, Down);
}

WalkDirection ape_compiler_AsmBloc(wast_t node, vm_t vm) {
	return update_vm_state(vm, Down);
}

volatile long line_number_bias=0;

const char* vm_find_innermost_file(vm_t vm);

void add_debug_label(wast_t node, vm_t vm) {
	static char linebuf[4096];
	static long prev_line=-1;
	if(prev_line<wa_row(node)) {
		prev_line=wa_row(node);
		sprintf(linebuf, ".%s_L%li", vm_find_innermost_file(vm), wa_row(node)+line_number_bias);
		opcode_chain_add_label(vm->result, linebuf, wa_row(node), wa_col(node));
	}
}

WalkDirection ape_compiler_DeclLabel(wast_t node, vm_t vm) {
	opcode_chain_add_label(vm->result,wa_op(wa_opd(node,0)), wa_row(node), wa_col(node));
	add_debug_label(node, vm);
	return update_vm_state(vm, Next);
}

WalkDirection ocao(wast_t node, vm_t vm, opcode_arg_t t) {
	const char*op = wa_op(wa_opd(node,0));
	add_debug_label(node, vm);
	if(t==OpcodeNoArg) {
/*		vm_printf("ocao::opcode without arg\t\t%s\n",op);*/
		opcode_chain_add_opcode(vm->result,t,op,NULL, wa_row(node), wa_col(node));
	/*} else if(t==OpcodeArgOpcode) {*/
		/*const char*type = wa_op(wa_opd(node,1))+strlen("DeclOpcode_");*/
		/*const char*name = wa_op(wa_opd(wa_opd(node,1),0));*/
		/*char*arg = (char*)malloc(strlen(type)+strlen(name)+2);*/
		/*sprintf(arg,"%s_%s",name,type);*/
		/*vm_printf("ocao::opcode with arg\t\t%s %s\n",op,arg);*/
		/*opcode_chain_add_opcode(vm->result,t,op,arg);*/
		/*free(arg);*/
	} else {
		const char*arg = wa_op(wa_opd(node,1));
/*		vm_printf("ocao::opcode with arg\t\t%s %s\n",op,arg);*/
		opcode_chain_add_opcode(vm->result,t,op,arg, wa_row(node), wa_col(node));
	}
	return update_vm_state(vm, Next);
}


WalkDirection ape_compiler_Opcode_Int(wast_t node, vm_t vm) {
	return update_vm_state(vm, ocao(node,vm,OpcodeArgInt));
}

WalkDirection ape_compiler_Opcode_Char(wast_t node, vm_t vm) {
	return update_vm_state(vm, ocao(node,vm,OpcodeArgChar));
}

WalkDirection ape_compiler_Opcode_Float(wast_t node, vm_t vm) {
	return update_vm_state(vm, ocao(node,vm,OpcodeArgFloat));
}

WalkDirection ape_compiler_Opcode_String(wast_t node, vm_t vm) {
	return update_vm_state(vm, ocao(node,vm,OpcodeArgString));
}

WalkDirection ape_compiler_Opcode_Label(wast_t node, vm_t vm) {
	return update_vm_state(vm, ocao(node,vm,OpcodeArgLabel));
}

WalkDirection ape_compiler_Opcode_EnvSym(wast_t node, vm_t vm) {
	return update_vm_state(vm, ocao(node,vm,OpcodeArgEnvSym));
}

WalkDirection ape_compiler_Opcode_NoArg(wast_t node, vm_t vm) {
	return update_vm_state(vm, ocao(node,vm,OpcodeNoArg));
}


/*
 * Opcode declarations
 */


WalkDirection ape_compiler_illegal_opcode(wast_t node, vm_t vm) {
	return Error;
}

WalkDirection ape_compiler_Library(wast_t node, vm_t vm) {
	return update_vm_state(vm, Down);
}


WalkDirection ape_compiler_LibFile(wast_t node, vm_t vm) {
	vm_set_lib_file(vm, wa_op(wa_opd(node,0)));
	return update_vm_state(vm, Next);
}


opcode_stub_t opcode_stub_resolve(opcode_arg_t arg_type, const char* name, void* dl_handle);


#if 0
void plug_opcode(tinyap_t parser, const char* arg_type, const char* opcode) {
	ast_node_t new_rule;

	char* re = (char*)malloc(strlen(opcode)+5);
	char* plug = (char*) malloc(strlen(arg_type)+10);

	sprintf(plug, "p_Opcode_%s",arg_type);
	sprintf(re,"\\<%s\\>",opcode);

	/* create node (RE [re]) */
	new_rule = tinyap_new_pair(
			tinyap_new_pair(tinyap_new_atom("RE",0), tinyap_new_pair(
				tinyap_new_atom(re,0), NULL)),
			NULL);
	/*vm_printf("adding new opcode RE : %s\n",ast_serialize_to_string(new_rule));*/

	tinyap_plug_node(parser, new_rule, opcode, plug);

	free(re);
	free(plug);
}
#endif



opcode_stub_t os;
opcode_stub_override_t osovl;

WalkDirection ape_compiler_DeclOpcodeOverloads(wast_t node, vm_t vm) {
	os=NULL;
	osovl=NULL;
	return Down;
}

WalkDirection ape_compiler_DeclOpcode_Overload(wast_t node, vm_t vm) {
	opcode_stub_override_t oo=osovl;
	if(!oo) {
		oo = (opcode_stub_override_t) malloc(sizeof(struct _opcode_stub_override_t));
		oo->next=NULL;
	} else {
		while(oo->next) { oo=oo->next; }
		oo->next = (opcode_stub_override_t) malloc(sizeof(struct _opcode_stub_override_t));
		oo=oo->next;
	}
	oo->offset = atoi(wa_op(wa_opd(node, 0)))-1;
	if(wa_opd_count(node)==1) {
		oo->target = os;
	} else {
		opcode_stub_t osbak = os;
		try_walk(wa_opd(node, 1), "compiler", vm);
		os=osbak;
	}
	if(os&&oo) {
		opcode_set_stub_overrides(&vm->opcodes, os, oo);
	}
	return Next;
}

WalkDirection ape_compiler__doovl_End(wast_t node, vm_t vm) {
	
	return Next;
}

WalkDirection ape_compiler_DeclOpcode_Float(wast_t node, vm_t vm) {
	const char* name = wa_op(wa_opd(node,0));
	/*plug_opcode(vm->parser, "Float", name);*/
	os = opcode_stub_resolve(OpcodeArgFloat,name,vm->dl_handle);
	if(!os) {
		vm_printerrf("warning : loading NULL opcode : %s:Float\n",name);
	}
    tinyap_add_bow("opcode", name);
	opcode_dict_add(vm_get_dict(vm), OpcodeArgFloat, name, os);
	return update_vm_state(vm, Next);
}

WalkDirection ape_compiler_DeclOpcode_Int(wast_t node, vm_t vm) {
	const char* name = wa_op(wa_opd(node,0));
	/*plug_opcode(vm->parser, "Int", name);*/
	os = opcode_stub_resolve(OpcodeArgInt,name,vm->dl_handle);
	if(!os) {
		vm_printerrf("warning : loading NULL opcode : %s:Int\n",name);
	}
    tinyap_add_bow("opcode", name);
	opcode_dict_add(vm_get_dict(vm), OpcodeArgInt, name, os);
	return update_vm_state(vm, Next);
}

WalkDirection ape_compiler_DeclOpcode_Char(wast_t node, vm_t vm) {
	const char* name = wa_op(wa_opd(node,0));
	/*plug_opcode(vm->parser, "Int", name);*/
	os = opcode_stub_resolve(OpcodeArgChar,name,vm->dl_handle);
	if(!os) {
		vm_printerrf("warning : loading NULL opcode : %s:Char\n",name);
	}
    tinyap_add_bow("opcode", name);
	opcode_dict_add(vm_get_dict(vm), OpcodeArgChar, name, os);
	return update_vm_state(vm, Next);
}

WalkDirection ape_compiler_DeclOpcode_Label(wast_t node, vm_t vm) {
	const char* name = wa_op(wa_opd(node,0));
	/*plug_opcode(vm->parser, "Label", name);*/
	os = opcode_stub_resolve(OpcodeArgLabel,name,vm->dl_handle);
	if(!os) {
		vm_printerrf("warning : loading NULL opcode : %s:Label\n",name);
	}
    tinyap_add_bow("opcode", name);
	opcode_dict_add(vm_get_dict(vm), OpcodeArgLabel, name, os);
	return update_vm_state(vm, Next);
}

WalkDirection ape_compiler_DeclOpcode_EnvSym(wast_t node, vm_t vm) {
	const char* name = wa_op(wa_opd(node,0));
	/*plug_opcode(vm->parser, "EnvSym", name);*/
	os = opcode_stub_resolve(OpcodeArgEnvSym,name,vm->dl_handle);
	if(!os) {
		vm_printerrf("warning : loading NULL opcode : %s:EnvSym\n",name);
	}
    tinyap_add_bow("opcode", name);
	opcode_dict_add(vm_get_dict(vm), OpcodeArgEnvSym, name, os);
	return update_vm_state(vm, Next);
}

WalkDirection ape_compiler_DeclOpcode_String(wast_t node, vm_t vm) {
	const char* name = wa_op(wa_opd(node,0));
	/*plug_opcode(vm->parser, "String", name);*/
	os = opcode_stub_resolve(OpcodeArgString,name,vm->dl_handle);
	if(!os) {
		vm_printerrf("warning : loading NULL opcode : %s:String\n",name);
	}
    tinyap_add_bow("opcode", name);
	opcode_dict_add(vm_get_dict(vm), OpcodeArgString, name, os);
	return update_vm_state(vm, Next);
}

WalkDirection ape_compiler_DeclOpcode_NoArg(wast_t node, vm_t vm) {
	const char* name = wa_op(wa_opd(node,0));
	/*plug_opcode(vm->parser, "NoArg", name);*/
	os = opcode_stub_resolve(OpcodeNoArg,name,vm->dl_handle);
	if(!os) {
		vm_printerrf("warning : loading NULL opcode : %s:NoArg\n",name);
	}
    tinyap_add_bow("opcode", name);
	opcode_dict_add(vm_get_dict(vm), OpcodeNoArg, name, os);
	return update_vm_state(vm, Next);
}



WalkDirection ape_compiler_DataBloc(wast_t node, vm_t vm) {
	return update_vm_state(vm, Down);
}


WalkDirection ape_compiler_DataInt(wast_t node, vm_t vm) {
	const char*rep=NULL;
	if(wa_opd_count(node)==2) {
		rep=wa_op(wa_opd(node,1));
	}
	opcode_chain_add_data(vm->result,DataInt,wa_op(wa_opd(node,0)),rep, wa_row(node), wa_col(node));
	return update_vm_state(vm, Next);
}


WalkDirection ape_compiler_DataFloat(wast_t node, vm_t vm) {
	const char*rep=NULL;
	if(wa_opd_count(node)==2) {
		rep=wa_op(wa_opd(node,1));
	}
	opcode_chain_add_data(vm->result,DataFloat,wa_op(wa_opd(node,0)),rep, wa_row(node), wa_col(node));
	_text_seg_append(vm->current_edit_prg->data_symbols, "");
	return update_vm_state(vm, Next);
}


WalkDirection ape_compiler_DataString(wast_t node, vm_t vm) {
	opcode_chain_add_data(vm->result,DataString,wa_op(wa_opd(node,0)),NULL, wa_row(node), wa_col(node));
	_text_seg_append(vm->current_edit_prg->data_symbols, "");
	return update_vm_state(vm, Next);
}

WalkDirection ape_compiler_DataChar(wast_t node, vm_t vm) {
	opcode_chain_add_data(vm->result,DataChar,wa_op(wa_opd(node,0)),NULL, wa_row(node), wa_col(node));
	_text_seg_append(vm->current_edit_prg->data_symbols, "");
	return update_vm_state(vm, Next);
}


void delete_node(ast_node_t);

WalkDirection ape_compiler_LangDef(wast_t node, vm_t vm) {
	/*opcode_chain_add_langdef(vm->result,node);*/
	ast_node_t n = make_ast(wa_opd(node,0));
	/*tinyap_append_grammar(vm->parser, n);*/
	const char* str = tinyap_serialize_to_string(n);
    char* str2 = (char*) malloc(3 + strlen(str));
    sprintf(str2, "(%s)", str);
	delete_node(n);
	free((char*)str);
	opcode_chain_add_opcode(vm->result, OpcodeArgString, "_langDef", str2, wa_row(node), wa_col(node));
    free(str2);
	return update_vm_state(vm, Next);
}

char* gen_unique_label() {
	static word_t n=0;
	static char l[10];
	sprintf(l,"_%8.8lx",n);
	n+=1;
	return l;
}

WalkDirection ape_compiler_LangPlug(wast_t node, vm_t vm) {
	/*long i;*/
	const char* plugin = wa_op(wa_opd(node,0));
	const char* plug = wa_op(wa_opd(node,1));
	char* methname = (char*)malloc(strlen(plugin)+10);

	sprintf(methname,".compile_%s",plugin);

	/*vm_printf("%p plugging %s into %s\n",vm->result,methname,plug);*/
	/*opcode_chain_add_langplug(vm->result,plugin,plug);*/
	opcode_chain_add_opcode(vm->result, OpcodeArgString, "push", plug, -1, -1);
	opcode_chain_add_opcode(vm->result, OpcodeArgString, "_langPlug", plugin, -1, -1);

	/* plug grammar */
	/*tinyap_plug(vm->parser, plugin, plug);*/

	free(methname);
	return update_vm_state(vm, Next);
}

WalkDirection ape_compiler_LangComp(wast_t node, vm_t vm) {
	const char* start = strdup(gen_unique_label());
	const char* end = strdup(gen_unique_label());
	const char* plugin = wa_op(wa_opd(node,0));
	char* methname = (char*)malloc(strlen(plugin)+10);
	char* tmp = (char*)malloc(strlen(start)+strlen(plugin)+2);
	sprintf(tmp,"%s_%s",start,plugin);
	free((char*)start);
	start=tmp;

	sprintf(methname,".compile_%s",plugin);

	
	/* compile compiling code */
	opcode_chain_add_opcode(vm->result,OpcodeArgLabel,"jmp",end, -1, -1);
	opcode_chain_add_label(vm->result,start, -1, -1);

	//ape_compiler_AsmBloc(wa_opd(node,2),vm);
	try_walk(wa_opd(node,1), "compiler", vm);
	
	
	/* plug compiling code */
	/*opcode_chain_add_opcode(vm->result, OpcodeNoArg, "_pop_curNode",0);*/
	opcode_chain_add_opcode(vm->result, OpcodeArgInt, "ret", "0", -1, -1);
	opcode_chain_add_label(vm->result, end, -1, -1);
	opcode_chain_add_opcode(vm->result, OpcodeArgString, "push", methname, -1, -1);
	opcode_chain_add_opcode(vm->result,OpcodeArgLabel,"__addCompileMethod",start, -1, -1);

	free((char*)start);
	free((char*)end);
	free(methname);
	return update_vm_state(vm, Next);
}


WalkDirection ape_compiler_Preproc(wast_t node, vm_t vm) {
	return update_vm_state(vm, Down);
}

WalkDirection ape_compiler_Include(wast_t node, vm_t vm) {
	/* compile and fg-execute the mentioned program */
	const char* fname = wa_op(wa_opd(node,0));
	tinyap_set_source_file(vm->parser,fname);
	tinyap_parse(vm->parser, 1);

	if(tinyap_get_output(vm->parser)) {
		wast_t wa = tinyap_make_wast(vm->parser, tinyap_list_get_element( tinyap_get_output(vm->parser), 0) );
		vm_compinput_push_file(vm, fname);
		try_walk(wa, "compiler", vm);
		vm_compinput_pop(vm);
		wa_del(wa);
	} else {
		vm_printf("Including %s :\nparser output : %p\n", fname, tinyap_get_output(vm->parser));
		vm_printerrf("parse error at %i:%i\n%s",tinyap_get_error_row(vm->parser),tinyap_get_error_col(vm->parser),tinyap_get_error(vm->parser));
	}
	return update_vm_state(vm, Next);
}



WalkDirection ape_compiler_Require(wast_t node, vm_t vm) {
	program_add_require(NULL, wa_op(wa_opd(node,0)));
	opcode_chain_add_opcode(vm->result, OpcodeArgString, "__RQ__", wa_op(wa_opd(node,0)), -1, -1);
	return update_vm_state(vm, Next);
}


WalkDirection ape_compiler_LoadLib(wast_t node, vm_t vm) {
	program_add_loadlib(NULL, wa_op(wa_opd(node,0)));
	opcode_chain_add_opcode(vm->result, OpcodeArgString, "__LL__", wa_op(wa_opd(node,0)), -1, -1);
	return update_vm_state(vm, Next);
}


WalkDirection ape_compiler_Postponed(wast_t node, vm_t vm) {
	char* buffy;
	long lnb_backup = line_number_bias;

    if(!strlen(wa_op(wa_opd(node,0)))) {
        return update_vm_state(vm, Done);
    }
    buffy = strdup(wa_op(wa_opd(node,0)));

	/*const char* debug = tinyap_serialize_to_string(tinyap_get_grammar_ast(vm->parser));*/

	/*vm_printf("Now compiling deferred buffer (%u bytes)\nwith grammar \n%s\n", strlen(buffy), debug); fflush(stdout);*/
	/*free((char*)debug);*/
	/*vm_printf("[COMP:DEBUG] PostPoned buffer at %i:%i (%i char)\n", wa_row(node), wa_col(node), strlen(buffy));*/
	line_number_bias += wa_row(node)-1;	/* skip current line :) */
	/*vm_printf("[COMP:DEBUG] line_number_bias=%i\n", line_number_bias);*/
	
	tinyap_set_source_buffer(vm->parser,buffy,strlen(buffy));
	tinyap_parse(vm->parser, 1);
	free(buffy);

	if(tinyap_get_output(vm->parser)) {
		wast_t wa = tinyap_make_wast(vm->parser, tinyap_list_get_element( tinyap_get_output(vm->parser), 0) );
		try_walk(wa, "compiler", vm);
		wa_del(wa);
	} else {
		vm_printf("parser output : %p\n",tinyap_get_output(vm->parser));
		vm_printerrf("parse error at %i:%i\n%s",tinyap_get_error_row(vm->parser),tinyap_get_error_col(vm->parser),tinyap_get_error(vm->parser));
	}
	line_number_bias = lnb_backup;
	return update_vm_state(vm, Done);
}





WalkDirection ape_compiler_NewWalker(wast_t node, vm_t vm) {
	vm->virt_walker = strdup(wa_op(wa_opd(node,0)));

	try_walk(wa_opd(node,1), "compiler", vm);
	/*try_walk(node,"prettyprint",NULL);*/

	free((char*)vm->virt_walker);
	return update_vm_state(vm, Next);
}



WalkDirection ape_compiler_WalkerBodies(wast_t node, vm_t vm) {
	return update_vm_state(vm, Down);
}


void compile_walker_method(wast_t node, vm_t vm, const char* plugin, long body_index) {
	/* FIXME : clean copypasta */
	const char* start = strdup(gen_unique_label());
	const char* end = strdup(gen_unique_label());
	/*const char* plugin = wa_op(wa_opd(node,0));*/
	char* methname = (char*)malloc(strlen(plugin)+strlen(vm->virt_walker)+8);
	char* tmp = (char*)malloc(strlen(start)+strlen(plugin)+2);
	sprintf(tmp,"%s_%s",start,plugin);
	free((char*)start);
	start=tmp;

	sprintf(methname,".virt_%s_%s",vm->virt_walker,plugin);

	
	/* compile compiling code */
	opcode_chain_add_opcode(vm->result,OpcodeArgLabel,"jmp",end, -1, -1);
	opcode_chain_add_label(vm->result,start, -1, -1);

	//ape_compiler_AsmBloc(wa_opd(node,2),vm);
	try_walk(wa_opd(node,body_index), "compiler", vm);
	
	
	/* plug compiling code */
	/*opcode_chain_add_opcode(vm->result, OpcodeNoArg, "_pop_curNode",0);*/
	opcode_chain_add_opcode(vm->result, OpcodeArgInt, "ret", "0", -1, -1);
	opcode_chain_add_label(vm->result, end, -1, -1);
	/*opcode_chain_add_opcode(vm->result, OpcodeArgString, "push", vm->virt_walker);*/
	opcode_chain_add_opcode(vm->result, OpcodeArgString, "push", methname, -1, -1);
	opcode_chain_add_opcode(vm->result,OpcodeArgLabel,"__addCompileMethod",start, -1, -1);

	free((char*)start);
	free((char*)end);
	free(methname);
}


WalkDirection ape_compiler_WalkerInit(wast_t node, vm_t vm) {
	compile_walker_method(node,vm,"__init__",0);
	return update_vm_state(vm, Next);
}


WalkDirection ape_compiler_WalkerTerminate(wast_t node, vm_t vm) {
	compile_walker_method(node,vm,"__term__",0);
	return update_vm_state(vm, Next);
}


WalkDirection ape_compiler_WalkerDefault(wast_t node, vm_t vm) {
	compile_walker_method(node,vm,"__dflt__",0);
	return update_vm_state(vm, Next);
}


WalkDirection ape_compiler_WalkerBody(wast_t node, vm_t vm) {
	compile_walker_method(node,vm,wa_op(wa_opd(node,0)),1);
	return update_vm_state(vm, Next);
}



