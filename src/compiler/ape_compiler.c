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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "vm_assert.h"

#include "vm.h"
#include "opcode_chain.h"
#include "opcode_dict.h"
#include "program.h"
#include "_impl.h"

/* hidden tinyap feature */
ast_node_t newAtom(const char*data,int row,int col);
ast_node_t newPair(const ast_node_t a,const ast_node_t d,const int row,const int col);

word_t	docn_dat=0,docn_cod=0;

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

program_t compile_wast(wast_t node, vm_t vm) {
	/*vm_printf("compile_wast\n");*/
	gpush(&vm->cn_stack,&vm->current_node);
	vm->current_node = node;
	tinyap_walk(node, "compiler", vm);
	vm_set_lib_file(vm,NULL);
	vm->current_node=*(wast_t*)_gpop(&vm->cn_stack);
	program_t ret = program_new();
	ret->env = vm->env;
	/*vm_printf("now %p\n",vm->result);*/
	opcode_chain_add_opcode(vm->result, OpcodeArgInt, "ret", "0", -1, -1);
	docn_dat=docn_cod=0;
	if(vm->result) {
		if(vm->result->head) {
			/*opcode_chain_apply(vm->result,dump_ocn);*/
			opcode_chain_serialize(vm->result, vm_get_dict(vm), ret, vm->dl_handle);
		}
		opcode_chain_delete(vm->result);
		vm->result=NULL;
	}
	/*vm_printf("\n-- New program compiled.\n-- Data size : %lu\n-- Code size : %lu\n\n",ret->data.size,ret->code.size);*/
	return ret;
}


void* ape_compiler_init(vm_t vm) {
	/* allow reentrant calls */
	gpush(&vm->cn_stack,&vm->current_node);
	if(vm->result==NULL) {
		/*vm_printf("###      NEW       top-level compiler [at %p:%lX]\n",vm_get_CS(vm),vm_get_IP(vm));*/
		vm->result = opcode_chain_new();
	/*} else {*/
		/*vm_printf("###      NEW       sub-compiler [at %p:%lX]\n",vm_get_CS(vm),vm_get_IP(vm));*/
	}
	/*vm_printf("vm new ochain : %p\n",vm->result);*/
	return vm;
}


void ape_compiler_free(vm_t vm) {
	vm->current_node=*(wast_t*)_gpop(&vm->cn_stack);
}


WalkDirection ape_compiler_default(wast_t node, vm_t vm) {
	char* vec_name = (char*)malloc(strlen(wa_op(node))+10);
	sprintf(vec_name,".compile_%s",wa_op(node));
	word_t vec_ofs = (word_t)hash_find(&vm->compile_vectors.by_text,(hash_key)vec_name);

	/*vm_printf("search for vector %s in %p gave %lu\n",vec_name, vm->result, vec_ofs);*/

	if(vec_ofs) {
		vm->compile_state = Error;
		program_t p = (program_t)*(vm->compile_vectors.by_index.data+vec_ofs);
		word_t ip = *(vm->compile_vectors.by_index.data+vec_ofs+1);
		/*vm_printf("compiler calling %p:%lX (%s)\n",p,ip,wa_op(node));*/
		gpush(&vm->cn_stack,&vm->current_node);
		vm->current_node = node;
		vm_run_program_fg(vm,p,ip,50);
		vm->current_node=*(wast_t*)_gpop(&vm->cn_stack);
		free(vec_name);
		/*vm_printf("   vm return state : %i\n",vm->compile_state);*/
		return vm->compile_state;
	}

	free(vec_name);

	vm_printerrf("Node is not known '%s'\n",wa_op(node));
	return Error;
}


void* ape_compiler_result(vm_t vm) {
	return vm->result;
}

WalkDirection ape_compiler_Program(wast_t node, vm_t vm) {
	return Down;
}

WalkDirection ape_compiler_AsmBloc(wast_t node, vm_t vm) {
	return Down;
}

WalkDirection ape_compiler_DeclLabel(wast_t node, vm_t vm) {
	opcode_chain_add_label(vm->result,wa_op(wa_opd(node,0)), wa_row(node), wa_col(node));
	return Next;
}

WalkDirection ocao(wast_t node, vm_t vm, opcode_arg_t t) {
	const char*op = wa_op(wa_opd(node,0));
	if(t==OpcodeNoArg) {
		/*vm_printf("ocao::opcode without arg\t\t%s\n",op);*/
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
		/*vm_printf("ocao::opcode with arg\t\t%s %s\n",op,arg);*/
		opcode_chain_add_opcode(vm->result,t,op,arg, wa_row(node), wa_col(node));
	}
	return Next;
}


WalkDirection ape_compiler_Opcode_Int(wast_t node, vm_t vm) {
	return ocao(node,vm,OpcodeArgInt);
}

WalkDirection ape_compiler_Opcode_Float(wast_t node, vm_t vm) {
	return ocao(node,vm,OpcodeArgFloat);
}

WalkDirection ape_compiler_Opcode_String(wast_t node, vm_t vm) {
	return ocao(node,vm,OpcodeArgString);
}

WalkDirection ape_compiler_Opcode_Label(wast_t node, vm_t vm) {
	return ocao(node,vm,OpcodeArgLabel);
}

WalkDirection ape_compiler_Opcode_EnvSym(wast_t node, vm_t vm) {
	return ocao(node,vm,OpcodeArgEnvSym);
}

WalkDirection ape_compiler_Opcode_NoArg(wast_t node, vm_t vm) {
	return ocao(node,vm,OpcodeNoArg);
}


/*
 * Opcode declarations
 */


WalkDirection ape_compiler_Library(wast_t node, vm_t vm) {
	return Down;
}


WalkDirection ape_compiler_LibFile(wast_t node, vm_t vm) {
	vm_set_lib_file(vm, wa_op(wa_opd(node,0)));
	return Next;
}


opcode_stub_t opcode_stub_resolve(opcode_arg_t arg_type, const char* name, void* dl_handle);


void plug_opcode(tinyap_t parser, const char* arg_type, const char* opcode) {
	ast_node_t new_rule;

	char* re = (char*)malloc(strlen(opcode)+5);
	char* plug = (char*) malloc(strlen(arg_type)+10);

	sprintf(plug, "p_Opcode_%s",arg_type);
	sprintf(re,"\\<%s\\>",opcode);

	/* create node (RE [re]) */
	new_rule = newPair(
			newPair(newAtom("RE",0,0), newPair(
				newAtom(re,0,0), NULL, 0, 0), 0, 0),
			NULL,0,0);
	/*vm_printf("adding new opcode RE : %s\n",ast_serialize_to_string(new_rule));*/

	tinyap_plug_node(parser, new_rule, opcode, plug);

	free(re);
	free(plug);
}


WalkDirection ape_compiler_DeclOpcode_Float(wast_t node, vm_t vm) {
	opcode_stub_t os;
	const char* name = wa_op(wa_opd(node,0));
	plug_opcode(vm->parser, "Float", name);
	os = opcode_stub_resolve(OpcodeArgFloat,name,vm->dl_handle);
	if(!os) {
		vm_printerrf("warning : loading NULL opcode : %s:Float\n",name);
	}
	opcode_dict_add(vm_get_dict(vm), OpcodeArgFloat, name, os);
	return Next;
}

WalkDirection ape_compiler_DeclOpcode_Int(wast_t node, vm_t vm) {
	opcode_stub_t os;
	const char* name = wa_op(wa_opd(node,0));
	plug_opcode(vm->parser, "Int", name);
	os = opcode_stub_resolve(OpcodeArgInt,name,vm->dl_handle);
	if(!os) {
		vm_printerrf("warning : loading NULL opcode : %s:Int\n",name);
	}
	opcode_dict_add(vm_get_dict(vm), OpcodeArgInt, name, os);
	return Next;
}

WalkDirection ape_compiler_DeclOpcode_Label(wast_t node, vm_t vm) {
	opcode_stub_t os;
	const char* name = wa_op(wa_opd(node,0));
	plug_opcode(vm->parser, "Label", name);
	os = opcode_stub_resolve(OpcodeArgLabel,name,vm->dl_handle);
	if(!os) {
		vm_printerrf("warning : loading NULL opcode : %s:Label\n",name);
	}
	opcode_dict_add(vm_get_dict(vm), OpcodeArgLabel, name, os);
	return Next;
}

WalkDirection ape_compiler_DeclOpcode_EnvSym(wast_t node, vm_t vm) {
	opcode_stub_t os;
	const char* name = wa_op(wa_opd(node,0));
	plug_opcode(vm->parser, "EnvSym", name);
	os = opcode_stub_resolve(OpcodeArgEnvSym,name,vm->dl_handle);
	if(!os) {
		vm_printerrf("warning : loading NULL opcode : %s:EnvSym\n",name);
	}
	opcode_dict_add(vm_get_dict(vm), OpcodeArgEnvSym, name, os);
	return Next;
}

WalkDirection ape_compiler_DeclOpcode_String(wast_t node, vm_t vm) {
	opcode_stub_t os;
	const char* name = wa_op(wa_opd(node,0));
	plug_opcode(vm->parser, "String", name);
	os = opcode_stub_resolve(OpcodeArgString,name,vm->dl_handle);
	if(!os) {
		vm_printerrf("warning : loading NULL opcode : %s:String\n",name);
	}
	opcode_dict_add(vm_get_dict(vm), OpcodeArgString, name, os);
	return Next;
}

WalkDirection ape_compiler_DeclOpcode_NoArg(wast_t node, vm_t vm) {
	opcode_stub_t os;
	const char* name = wa_op(wa_opd(node,0));
	plug_opcode(vm->parser, "NoArg", name);
	os = opcode_stub_resolve(OpcodeNoArg,name,vm->dl_handle);
	if(!os) {
		vm_printerrf("warning : loading NULL opcode : %s:NoArg\n",name);
	}
	opcode_dict_add(vm_get_dict(vm), OpcodeNoArg, name, os);
	return Next;
}



WalkDirection ape_compiler_DataBloc(wast_t node, vm_t vm) {
	return Down;
}


WalkDirection ape_compiler_DataInt(wast_t node, vm_t vm) {
	const char*rep=NULL;
	if(wa_opd_count(node)==2) {
		rep=wa_op(wa_opd(node,1));
	}
	opcode_chain_add_data(vm->result,DataInt,wa_op(wa_opd(node,0)),rep, wa_row(node), wa_col(node));
	return Next;
}


WalkDirection ape_compiler_DataFloat(wast_t node, vm_t vm) {
	const char*rep=NULL;
	if(wa_opd_count(node)==2) {
		rep=wa_op(wa_opd(node,1));
	}
	opcode_chain_add_data(vm->result,DataFloat,wa_op(wa_opd(node,0)),rep, wa_row(node), wa_col(node));
	return Next;
}


WalkDirection ape_compiler_DataString(wast_t node, vm_t vm) {
	opcode_chain_add_data(vm->result,DataString,wa_op(wa_opd(node,0)),NULL, wa_row(node), wa_col(node));
	return Next;
}


void delete_node(ast_node_t);

WalkDirection ape_compiler_LangDef(wast_t node, vm_t vm) {
	/*tinyap_append_grammar(vm->parser,make_ast(wa_opd(node,0)));*/
	/*opcode_chain_add_langdef(vm->result,node);*/
	ast_node_t n = make_ast(wa_opd(node,0));
	const char* str = tinyap_serialize_to_string(n);
	delete_node(n);
	opcode_chain_add_opcode(vm->result, OpcodeArgString, "_langDef", str, wa_row(node), wa_col(node));
	free((char*)str);
	return Next;
}

char* gen_unique_label() {
	static word_t n=0;
	static char l[10];
	sprintf(l,"_%8.8lx",n);
	n+=1;
	return l;
}

WalkDirection ape_compiler_LangPlug(wast_t node, vm_t vm) {
	/*int i;*/
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
	return Next;
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
	tinyap_walk(wa_opd(node,1), "compiler", vm);
	
	
	/* plug compiling code */
	/*opcode_chain_add_opcode(vm->result, OpcodeNoArg, "_pop_curNode",0);*/
	opcode_chain_add_opcode(vm->result, OpcodeArgInt, "ret", "0", -1, -1);
	opcode_chain_add_label(vm->result, end, -1, -1);
	opcode_chain_add_opcode(vm->result, OpcodeArgString, "push", methname, -1, -1);
	opcode_chain_add_opcode(vm->result,OpcodeArgLabel,"__addCompileMethod",start, -1, -1);

	free((char*)start);
	free((char*)end);
	free(methname);
	return Next;
}


WalkDirection ape_compiler_Preproc(wast_t node, vm_t vm) {
	return Down;
}

WalkDirection ape_compiler_Require(wast_t node, vm_t vm) {
	/* compile and fg-execute the mentioned program */
	FILE*f;
	char buffy[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	const char* fname = wa_op(wa_opd(node,0));
	program_t p;
	f = fopen(fname,"r");
	if(!f) {
		vm_printerrf("ERROR : compiler couldn't open file %s\n",fname);
		return Error;
	}
	fread(buffy,8,1,f);
	fclose(f);
	if(strcmp(buffy,"BML_PRG")) {
		/* looks like a source file */
		/* try and compile the file */
		opcode_chain_delete(vm->result);	/* discard result, anyway it is empty at this point */
		vm->result=NULL;
		p = vm_compile_file(vm, fname);
		vm->result = opcode_chain_new();
	} else {
		/* looks like a serialized program */
		/* unserialize program */
		reader_t r = file_reader_new(fname);
		p = vm_unserialize_program(vm,r);
		reader_close(r);
	}
	if(p) {
		vm_run_program_fg(vm,p,0,50);
		/*vm_printf("Required file executed.\n");*/
	} else {
		vm_printerrf("ERROR : nothing to execute while requiring %s\n",fname);
		return Error;
	}
	return Next;
}


WalkDirection ape_compiler_Postponed(wast_t node, vm_t vm) {
	char* buffy = strdup(wa_op(wa_opd(node,0)));

	/*const char* debug = tinyap_serialize_to_string(tinyap_get_grammar_ast(vm->parser));*/

	/*vm_printf("Now compiling deferred buffer (%u bytes)\n"*/
		/*"===================================\n"*/
		/*"%s\n"*/
		/*"===================================\n with grammar \n%s\n", strlen(buffy), buffy, debug); fflush(stdout);*/
	/*free((char*)debug);*/
	
	tinyap_set_source_buffer(vm->parser,buffy,strlen(buffy));
	tinyap_parse(vm->parser);
	free(buffy);

	if(tinyap_get_output(vm->parser)) {
		wast_t wa = tinyap_make_wast( tinyap_list_get_element( tinyap_get_output(vm->parser), 0) );
		tinyap_walk(wa, "compiler", vm);
		wa_del(wa);
	} else {
		vm_printf("parser output : %p\n",tinyap_get_output(vm->parser));
		vm_printerrf("parse error at %i:%i\n%s",tinyap_get_error_row(vm->parser),tinyap_get_error_col(vm->parser),tinyap_get_error(vm->parser));
	}
	return Done;
}





WalkDirection ape_compiler_NewWalker(wast_t node, vm_t vm) {
	vm->virt_walker = strdup(wa_op(wa_opd(node,0)));

	tinyap_walk(wa_opd(node,1), "compiler", vm);
	/*tinyap_walk(node,"prettyprint",NULL);*/

	free((char*)vm->virt_walker);
	return Next;
}



WalkDirection ape_compiler_WalkerBodies(wast_t node, vm_t vm) {
	return Down;
}


void compile_walker_method(wast_t node, vm_t vm, const char* plugin, int body_index) {
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
	tinyap_walk(wa_opd(node,body_index), "compiler", vm);
	
	
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
	return Next;
}


WalkDirection ape_compiler_WalkerTerminate(wast_t node, vm_t vm) {
	compile_walker_method(node,vm,"__term__",0);
	return Next;
}


WalkDirection ape_compiler_WalkerDefault(wast_t node, vm_t vm) {
	compile_walker_method(node,vm,"__dflt__",0);
	return Next;
}


WalkDirection ape_compiler_WalkerBody(wast_t node, vm_t vm) {
	compile_walker_method(node,vm,wa_op(wa_opd(node,0)),1);
	return Next;
}



