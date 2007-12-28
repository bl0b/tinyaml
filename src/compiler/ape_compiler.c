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

#include <assert.h>

#include "vm.h"
#include "opcode_chain.h"
#include "opcode_dict.h"
#include "program.h"
#include "_impl.h"

/* hidden tinyap feature */
ast_node_t newAtom(const char*data,int row,int col);
ast_node_t newPair(const ast_node_t a,const ast_node_t d,const int row,const int col);

void dump_ocn(opcode_chain_node_t ocn) {
	switch(ocn->type) {
	case NodeData:
		printf("data %i:%s [%s]\n",ocn->arg_type,ocn->name,ocn->arg);
		break;
	case NodeOpcode:
		switch(ocn->arg_type) {
		case OpcodeArgString:
			printf("asm       %s\t\"%s\"\n",ocn->name,ocn->arg);
			break;
		case OpcodeArgLabel:
			printf("asm       %s\t@%s\n",ocn->name,ocn->arg);
			break;
		default:
			printf("asm       %s\t%s\n",ocn->name,ocn->arg?ocn->arg:"");
			break;
		};
		break;
	case NodeLabel:
		printf("asm %s:\n",ocn->name);
		break;
	};
}

program_t compile_wast(wast_t node, vm_t vm) {
	tinyap_walk(node, "compiler", vm);
//	if(vm->result) {
		program_t ret = program_new();
		/*printf("now %p\n",vm->result);*/
		opcode_chain_add_opcode(vm->result, OpcodeArgInt, "ret", "0");
		opcode_chain_apply(vm->result,dump_ocn);
		opcode_chain_serialize(vm->result, vm_get_dict(vm), ret, vm->dl_handle);
		opcode_chain_delete(vm->result);
		vm->result=NULL;
		/*printf("\n-- New program compiled.\n-- Data size : %lu\n-- Code size : %lu\n\n",ret->data.size,ret->code.size);*/
		return ret;
//	}
//	return NULL;
}


void* ape_compiler_init(vm_t vm) {
	/* allow reentrant calls */
	if(vm->result==NULL) {
		vm->result = opcode_chain_new();
	}
	/*printf("vm new ochain : %p\n",vm->result);*/
	return vm;
}


void ape_compiler_free(vm_t vm) {
}


WalkDirection ape_compiler_default(wast_t node, vm_t vm) {
	char* vec_name = (char*)malloc(strlen(wa_op(node))+10);
	sprintf(vec_name,".compile_%s",wa_op(node));
	word_t vec_ofs = (word_t)hash_find(&vm->compile_vectors.by_text,(hash_key)vec_name);

	/*printf("search for vector %s in %p gave %lu\n",vec_name, vm->result, vec_ofs);*/

	if(vec_ofs) {
		vm->compile_state = Error;
		vm->current_node = node;
		program_t p = (program_t)*(vm->compile_vectors.by_index.data+vec_ofs);
		word_t ip = *(vm->compile_vectors.by_index.data+vec_ofs+1);
		/*printf("compiler calling %p:%lu\n",p,ip);*/
		vm_run_program_fg(vm,p,ip,50);
		free(vec_name);
		return vm->compile_state;
	}

	free(vec_name);

	fprintf(stderr,"Node is not known '%s'\n",wa_op(node));
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
	opcode_chain_add_label(vm->result,wa_op(wa_opd(node,0)));
	return Next;
}

WalkDirection ocao(wast_t node, vm_t vm, opcode_arg_t t) {
	const char*op = wa_op(wa_opd(node,0));
	if(t==OpcodeNoArg) {
		/*printf("ocao::opcode without arg\t\t%s\n",op);*/
		opcode_chain_add_opcode(vm->result,t,op,NULL);
	} else if(t==OpcodeArgOpcode) {
		const char*type = wa_op(wa_opd(node,1))+strlen("DeclOpcode_");
		const char*name = wa_op(wa_opd(wa_opd(node,1),0));
		char*arg = (char*)malloc(strlen(type)+strlen(name)+2);
		sprintf(arg,"%s_%s",name,type);
		/*printf("ocao::opcode with arg\t\t%s %s\n",op,arg);*/
		opcode_chain_add_opcode(vm->result,t,op,arg);
		free(arg);
	} else {
		const char*arg = wa_op(wa_opd(node,1));
		/*printf("ocao::opcode with arg\t\t%s %s\n",op,arg);*/
		opcode_chain_add_opcode(vm->result,t,op,arg);
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

WalkDirection ape_compiler_Opcode_Opcode(wast_t node, vm_t vm) {
	return ocao(node,vm,OpcodeArgOpcode);
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
	/*printf("adding new opcode RE : %s\n",ast_serialize_to_string(new_rule));*/

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
		fprintf(stderr,"warning : loading NULL opcode : %s:Float\n",name);
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
		fprintf(stderr,"warning : loading NULL opcode : %s:Int\n",name);
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
		fprintf(stderr,"warning : loading NULL opcode : %s:Label\n",name);
	}
	opcode_dict_add(vm_get_dict(vm), OpcodeArgLabel, name, os);
	return Next;
}

WalkDirection ape_compiler_DeclOpcode_Opcode(wast_t node, vm_t vm) {
	opcode_stub_t os;
	const char* name = wa_op(wa_opd(node,0));
	plug_opcode(vm->parser, "Opcode", name);
	os = opcode_stub_resolve(OpcodeArgOpcode,name,vm->dl_handle);
	if(!os) {
		fprintf(stderr,"warning : loading NULL opcode : %s:Opcode\n",name);
	}
	opcode_dict_add(vm_get_dict(vm), OpcodeArgOpcode, name, os);
	return Next;
}

WalkDirection ape_compiler_DeclOpcode_String(wast_t node, vm_t vm) {
	opcode_stub_t os;
	const char* name = wa_op(wa_opd(node,0));
	plug_opcode(vm->parser, "String", name);
	os = opcode_stub_resolve(OpcodeArgString,name,vm->dl_handle);
	if(!os) {
		fprintf(stderr,"warning : loading NULL opcode : %s:String\n",name);
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
		fprintf(stderr,"warning : loading NULL opcode : %s:NoArg\n",name);
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
	opcode_chain_add_data(vm->result,DataInt,wa_op(wa_opd(node,0)),rep);
	return Next;
}


WalkDirection ape_compiler_DataFloat(wast_t node, vm_t vm) {
	const char*rep=NULL;
	if(wa_opd_count(node)==2) {
		rep=wa_op(wa_opd(node,1));
	}
	opcode_chain_add_data(vm->result,DataFloat,wa_op(wa_opd(node,0)),rep);
	return Next;
}


WalkDirection ape_compiler_DataString(wast_t node, vm_t vm) {
	opcode_chain_add_data(vm->result,DataString,wa_op(wa_opd(node,0)),NULL);
	return Next;
}


WalkDirection ape_compiler_LangDef(wast_t node, vm_t vm) {
	tinyap_append_grammar(vm->parser,make_ast(wa_opd(node,0)));
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

	/*printf("%p plugging %s into %s\n",vm->result,methname,plug);*/

	/* plug grammar */
	tinyap_plug(vm->parser, plugin, plug);

	free(methname);
	return Next;
}

WalkDirection ape_compiler_LangComp(wast_t node, vm_t vm) {
	const char* start = strdup(gen_unique_label());
	const char* end = strdup(gen_unique_label());
	const char* plugin = wa_op(wa_opd(node,0));
	char* methname = (char*)malloc(strlen(plugin)+10);

	sprintf(methname,".compile_%s",plugin);

	
	/* compile compiling code */
	opcode_chain_add_opcode(vm->result,OpcodeArgLabel,"jmp",end);
	opcode_chain_add_label(vm->result,start);

	//ape_compiler_AsmBloc(wa_opd(node,2),vm);
	tinyap_walk(wa_opd(node,1), "compiler", vm);
	
	
	/* plug compiling code */
	opcode_chain_add_opcode(vm->result, OpcodeArgInt, "ret", "0");
	opcode_chain_add_label(vm->result, end);
	opcode_chain_add_opcode(vm->result, OpcodeArgString, "push", methname);
	opcode_chain_add_opcode(vm->result,OpcodeArgLabel,"__addCompileMethod",start);

	free((char*)start);
	free((char*)end);
	free(methname);
	return Next;
}
