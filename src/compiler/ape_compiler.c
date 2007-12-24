#include <tinyape.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <assert.h>

#include "vm.h"
#include "opcode_chain.h"
#include "opcode_dict.h"
#include "program.h"

typedef struct _env_t {
	vm_t vm;
	opcode_chain_t result;
}* env_t;

program_t compile_wast(wast_t node, vm_t vm) {
	opcode_chain_t result = (opcode_chain_t)tinyap_walk(node, "compiler", vm);
	program_t ret = program_new();
	opcode_chain_serialize(result, vm_get_dict(vm), ret);
	return ret;
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
//	obj_t compiler = ml_env_find(env->env,"compiler");
//	obj_t compileMethod = obj_get_by_name(obj_get_by_name(compiler,"compileMethods"),wa_op(node));
//	if(compileMethod) {
//		obj_t paramlist = obj_new(ml_env_find(env->env,"Object"));
//		obj_t tree = obj_new(ml_env_find(env->env,"AST"));
//		tree->content_bits = CTS_INTRINSIC|CTS_PTR;
//		tree->_intrinsic.ptr = node;
//		obj_add(paramlist, "tree", tree);
//		env->result = msg_send(compiler, compileMethod, paramlist, env->env);
//		/* build a parameter list containing tree */
//		//fprintf(stderr,"TODO : compileMethod\n");
//		return Done;
//	} else {
//		fprintf(stderr,"Node is not known '%s'\n",wa_op(node));
//		return Error;
//	}
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
	} else {
		const char*arg = wa_op(wa_opd(node,1));
		printf("ocao::opcode with arg\t\t%s %s\n",op,arg);
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


