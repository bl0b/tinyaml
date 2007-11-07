#include <tinyape.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "obj.h"
#include <assert.h>


static obj_t new_string(const char* str) {
	obj_t ret = obj_new(&class_string);
	ret->content_bits = CTS_INTRINSIC|CTS_STRING;
	ret->_intrinsic.ptr = (void*)strdup(str);
	return ret;
}


obj_t new_ref(const char*str) {
	obj_t ret = obj_new(&class_ref);
	obj_add(ret, "target", new_string(str));
	return ret;
}


static obj_t new_int(const char* str) {
	obj_t ret = obj_new(&class_int);
	ret->content_bits = CTS_INTRINSIC|CTS_INT;
	ret->_intrinsic.i = atoi(str);
	return ret;
}


typedef struct _env_t {
	obj_t env;
	obj_t result;
}* env_t;


obj_t compile(wast_t node,env_t env) {
	return (obj_t)tinyap_walk(node,"compiler",env->env);
}



void* ape_compiler_init(void* init_data) {
	env_t ret = malloc(sizeof(struct _env_t));
/*	if(init_data) {
		ret->env = (obj_t)init_data;
	} else {
		ret->env = env_new();
	}
*/
	ret->env = (obj_t)init_data;
	ret->result = NULL;
	return ret;
}


void ape_compiler_free(void*data) {
	free(data);
}


WalkDirection ape_compiler_default(wast_t node, env_t env) {
	obj_t compiler = ml_env_find(env->env,"compiler");
	obj_t compileMethod = obj_get_by_name(obj_get_by_name(compiler,"compileMethods"),wa_op(node));
	if(compileMethod) {
		obj_t paramlist = obj_new(ml_env_find(env->env,"Object"));
		obj_t tree = obj_new(ml_env_find(env->env,"AST"));
		tree->content_bits = CTS_INTRINSIC|CTS_PTR;
		tree->_intrinsic.ptr = node;
		obj_add(paramlist, "tree", tree);
		env->result = msg_send(compiler, compileMethod, paramlist, env->env);
		/* build a parameter list containing tree */
		//fprintf(stderr,"TODO : compileMethod\n");
		return Done;
	} else {
		fprintf(stderr,"Node is not known '%s'\n",wa_op(node));
		return Error;
	}
}


void* ape_compiler_result(env_t env) {
	return env->result;
}


void obj_dump(obj_t);

WalkDirection ape_compiler_BODY(wast_t node, env_t env) {
	/* for each sub node, do something different */
	int i;
	obj_t o;
	for(i=0;i<wa_opd_count(node);i++) {
		o = compile(wa_opd(node,i),env);
		if(o) {
			//obj_dump(o);
			//obj_dump(ml_obj_eval(o,env->env));
			ml_obj_eval(o,env->env);
		}
	}
	//obj_dump(env->env);
	return Done;
}

WalkDirection ape_compiler_MSG_SEQ(wast_t node, env_t env) {
	int i;
	//printf("MSG_SEQ\n");
	env->result = obj_new(&class_message_list);
	for(i=0;i<wa_opd_count(node);i++) {
		obj_add(env->result, NULL, compile(wa_opd(node,i),env));
	}
	
	return Done;
}

WalkDirection ape_compiler_DEFERRED(wast_t node, env_t env) {
	obj_t messages;
	//printf("DEFERRED_");
	ape_compiler_MSG_SEQ(node, env);
	messages = env->result;
	env->result = obj_new(&class_deferred_message_list);
	obj_add(env->result,"messages",messages);
	
	return Done;
}

WalkDirection ape_compiler_MESSAGE(wast_t node, env_t env) {
	int i;
	wast_t p;
	obj_t paramlist = obj_new(&class_object);
	env->result = obj_new(&class_message);
	assert(wa_opd(node,0));
	obj_add(env->result, "to", compile(wa_opd(node,0),env));
	obj_add(env->result, "what", new_string(wa_op(wa_opd(node,1))));
	//obj_add(env->result, "what", new_ref(wa_op(wa_opd(node,1))));
	//obj_add(env->result, "what", compile(wa_opd(node,1),env));
	obj_add(env->result, "paramlist", paramlist);
	
	for(i=2;i<wa_opd_count(node);i+=1) {
		p = wa_opd(node,i);
		assert(!strcmp(wa_op(p),"PARAM"));
		obj_add(paramlist,
			wa_op(wa_opd(p,0)),
			compile(wa_opd(p,1),env));
	}
	return Done;
}

WalkDirection ape_compiler_String(wast_t node, env_t env) {
	env->result = new_string(wa_op(wa_opd(node,0)));
	return Done;
}

WalkDirection ape_compiler_Integer(wast_t node, env_t env) {
	env->result = new_int(wa_op(wa_opd(node,0)));
	return Done;
}


WalkDirection ape_compiler_REF(wast_t node, env_t env) {
	env->result = new_ref(wa_op(wa_opd(node,0)));
	return Done;
}


WalkDirection ape_compiler_Grammar(wast_t node, env_t env) {
	//printf("Grammar\n");
	//fprintf(stderr,"FIXME : Grammar node not implemented\n");
	tinyap_t parser = ml_parser(env->env);
	//printf("\nenv at %p with parser at %p\n",env,parser);

	tinyap_append_grammar(parser, tinyap_make_ast(node));

	return Done;
}

