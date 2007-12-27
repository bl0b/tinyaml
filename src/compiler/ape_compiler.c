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



typedef struct _env_t {
	vm_t vm;
	opcode_chain_t result;
}* env_t;

program_t compile_wast(wast_t node, vm_t vm) {
	opcode_chain_t result = (opcode_chain_t)tinyap_walk(node, "compiler", vm);
	if(result) {
		program_t ret = program_new();
		opcode_chain_serialize(result, vm_get_dict(vm), ret, vm->dl_handle);
		opcode_chain_delete(result);
		printf("\n-- New program compiled.\n-- Data size : %lu\n-- Code size : %lu\n\n",ret->data.size,ret->code.size);
		return ret;
	}
	return NULL;
}


void* ape_compiler_init(void* init_data) {
	env_t ret = malloc(sizeof(struct _env_t));
	ret->vm = (vm_t)init_data;
	ret->result = NULL;
	return ret;
}


void ape_compiler_free(void*data) {
	free(data);
}


WalkDirection ape_compiler_default(wast_t node, env_t env) {
/*
	obj_t compiler = ml_env_find(env->env,"compiler");
	obj_t compileMethod = obj_get_by_name(obj_get_by_name(compiler,"compileMethods"),wa_op(node));
	if(compileMethod) {
		obj_t paramlist = obj_new(ml_env_find(env->env,"Object"));
		obj_t tree = obj_new(ml_env_find(env->env,"AST"));
		tree->content_bits = CTS_INTRINSIC|CTS_PTR;
		tree->_intrinsic.ptr = node;
		obj_add(paramlist, "tree", tree);
		env->result = msg_send(compiler, compileMethod, paramlist, env->env);
		//fprintf(stderr,"TODO : compileMethod\n");
		return Done;
	} else {
		fprintf(stderr,"Node is not known '%s'\n",wa_op(node));
		return Error;
	}
*/
	fprintf(stderr,"Node is not known '%s'\n",wa_op(node));
	return Error;
}


void* ape_compiler_result(env_t env) {
	return env->result;
}

WalkDirection ape_compiler_Program(wast_t node, env_t env) {
	env->result = opcode_chain_new();
	return Down;
}

WalkDirection ape_compiler_AsmBloc(wast_t node, env_t env) {
	return Down;
}

WalkDirection ape_compiler_DeclLabel(wast_t node, env_t env) {
	opcode_chain_add_label(env->result,wa_op(wa_opd(node,0)));
	return Next;
}

WalkDirection ocao(wast_t node, env_t env, opcode_arg_t t) {
	const char*op = wa_op(wa_opd(node,0));
	if(t==OpcodeNoArg) {
		opcode_chain_add_opcode(env->result,t,op,NULL);
	} else if(t==OpcodeArgOpcode) {
		const char*type = wa_op(wa_opd(node,1))+strlen("DeclOpcode_");
		const char*name = wa_op(wa_opd(wa_opd(node,1),0));
		char*arg = (char*)malloc(strlen(type)+strlen(name)+2);
		sprintf(arg,"%s_%s",name,type);
		printf("ocao::opcode with arg\t\t%s %s\n",op,arg);
		opcode_chain_add_opcode(env->result,t,op,arg);
		free(arg);
	} else {
		const char*arg = wa_op(wa_opd(node,1));
		/*printf("ocao::opcode with arg\t\t%s %s\n",op,arg);*/
		opcode_chain_add_opcode(env->result,t,op,arg);
	}
	return Next;
}


WalkDirection ape_compiler_Opcode_Int(wast_t node, env_t env) {
	return ocao(node,env,OpcodeArgInt);
}

WalkDirection ape_compiler_Opcode_Float(wast_t node, env_t env) {
	return ocao(node,env,OpcodeArgFloat);
}

WalkDirection ape_compiler_Opcode_String(wast_t node, env_t env) {
	return ocao(node,env,OpcodeArgString);
}

WalkDirection ape_compiler_Opcode_Label(wast_t node, env_t env) {
	return ocao(node,env,OpcodeArgLabel);
}

WalkDirection ape_compiler_Opcode_Opcode(wast_t node, env_t env) {
	return ocao(node,env,OpcodeArgOpcode);
}

WalkDirection ape_compiler_Opcode_NoArg(wast_t node, env_t env) {
	return ocao(node,env,OpcodeNoArg);
}


/*
 * Opcode declarations
 */


WalkDirection ape_compiler_Library(wast_t node, env_t env) {
	return Down;
}


WalkDirection ape_compiler_LibFile(wast_t node, env_t env) {
	vm_set_lib_file(env->vm, wa_op(wa_opd(node,0)));
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


WalkDirection ape_compiler_DeclOpcode_Float(wast_t node, env_t env) {
	opcode_stub_t os;
	const char* name = wa_op(wa_opd(node,0));
	plug_opcode(env->vm->parser, "Float", name);
	os = opcode_stub_resolve(OpcodeArgFloat,name,env->vm->dl_handle);
	if(!os) {
		fprintf(stderr,"warning : loading NULL opcode : %s:Float\n",name);
	}
	opcode_dict_add(vm_get_dict(env->vm), OpcodeArgFloat, name, os);
	return Next;
}

WalkDirection ape_compiler_DeclOpcode_Int(wast_t node, env_t env) {
	opcode_stub_t os;
	const char* name = wa_op(wa_opd(node,0));
	plug_opcode(env->vm->parser, "Int", name);
	os = opcode_stub_resolve(OpcodeArgInt,name,env->vm->dl_handle);
	if(!os) {
		fprintf(stderr,"warning : loading NULL opcode : %s:Int\n",name);
	}
	opcode_dict_add(vm_get_dict(env->vm), OpcodeArgInt, name, os);
	return Next;
}

WalkDirection ape_compiler_DeclOpcode_Label(wast_t node, env_t env) {
	opcode_stub_t os;
	const char* name = wa_op(wa_opd(node,0));
	plug_opcode(env->vm->parser, "Label", name);
	os = opcode_stub_resolve(OpcodeArgLabel,name,env->vm->dl_handle);
	if(!os) {
		fprintf(stderr,"warning : loading NULL opcode : %s:Label\n",name);
	}
	opcode_dict_add(vm_get_dict(env->vm), OpcodeArgLabel, name, os);
	return Next;
}

WalkDirection ape_compiler_DeclOpcode_Opcode(wast_t node, env_t env) {
	opcode_stub_t os;
	const char* name = wa_op(wa_opd(node,0));
	plug_opcode(env->vm->parser, "Opcode", name);
	os = opcode_stub_resolve(OpcodeArgOpcode,name,env->vm->dl_handle);
	if(!os) {
		fprintf(stderr,"warning : loading NULL opcode : %s:Opcode\n",name);
	}
	opcode_dict_add(vm_get_dict(env->vm), OpcodeArgOpcode, name, os);
	return Next;
}

WalkDirection ape_compiler_DeclOpcode_String(wast_t node, env_t env) {
	opcode_stub_t os;
	const char* name = wa_op(wa_opd(node,0));
	plug_opcode(env->vm->parser, "String", name);
	os = opcode_stub_resolve(OpcodeArgString,name,env->vm->dl_handle);
	if(!os) {
		fprintf(stderr,"warning : loading NULL opcode : %s:String\n",name);
	}
	opcode_dict_add(vm_get_dict(env->vm), OpcodeArgString, name, os);
	return Next;
}

WalkDirection ape_compiler_DeclOpcode_NoArg(wast_t node, env_t env) {
	opcode_stub_t os;
	const char* name = wa_op(wa_opd(node,0));
	plug_opcode(env->vm->parser, "NoArg", name);
	os = opcode_stub_resolve(OpcodeNoArg,name,env->vm->dl_handle);
	if(!os) {
		fprintf(stderr,"warning : loading NULL opcode : %s:NoArg\n",name);
	}
	opcode_dict_add(vm_get_dict(env->vm), OpcodeNoArg, name, os);
	return Next;
}



WalkDirection ape_compiler_DataBloc(wast_t node, env_t env) {
	return Down;
}


WalkDirection ape_compiler_DataInt(wast_t node, env_t env) {
	const char*rep=NULL;
	if(wa_opd_count(node)==2) {
		rep=wa_op(wa_opd(node,1));
	}
	opcode_chain_add_data(env->result,DataInt,wa_op(wa_opd(node,0)),rep);
	return Next;
}


WalkDirection ape_compiler_DataFloat(wast_t node, env_t env) {
	const char*rep=NULL;
	if(wa_opd_count(node)==2) {
		rep=wa_op(wa_opd(node,1));
	}
	opcode_chain_add_data(env->result,DataFloat,wa_op(wa_opd(node,0)),rep);
	return Next;
}


WalkDirection ape_compiler_DataString(wast_t node, env_t env) {
	opcode_chain_add_data(env->result,DataString,wa_op(wa_opd(node,0)),NULL);
	return Next;
}


WalkDirection ape_compiler_LangDef(wast_t node, env_t env) {
	return Next;
}

WalkDirection ape_compiler_LangPlug(wast_t node, env_t env) {
	return Next;
}

