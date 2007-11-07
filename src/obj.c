#include <string.h>
#include <stdio.h>
#include "obj.h"
#include "stack.h"
#include <assert.h>
#include <tinyap.h>

void prefix(int n) {
	printf("%*.*s",n,n,"");
}

void obj_dump(obj_t obj) {
	static int rec_lvl=0;
	//printf("%*.*s[ <",rec_lvl,rec_lvl,"");
	if(!obj) {
		printf("[null]");
		return;
	}
	printf("[ <");
	if(obj->_class) {
		obj_t cname = obj_get_by_name(obj->_class,"name");
		if(cname) {
			printf("%s %p",(const char*)cname->_intrinsic.ptr,obj->_class);
		} else {
			printf("anonymous class %p", obj->_class);
		}
	} else {
		printf("nil");
	}
	printf("> ");
	if(obj->content_bits&CTS_INTRINSIC) {
		switch(obj->content_bits&(~CTS_INTRINSIC)) {
		case CTS_INT:
			printf("i:%i",obj->_intrinsic.i);
			break;
		case CTS_FLOAT:
			printf("f:%f",obj->_intrinsic.f);
			break;
		case CTS_PTR:
			printf("ptr:%p",obj->_intrinsic.ptr);
			break;
		case CTS_STRING:
			printf("str:\"%s\"",obj->_intrinsic.ptr);
			break;
		case CTS_FUNC:
			printf("func:%p",obj->_intrinsic.c_func);
			break;
		};
	} else {
		int i;
		rec_lvl+=3;
		printf("\n");
		for(i=0;i<obj->_contents.size;i++) {
			prefix(rec_lvl);
			printf("%s: ",obj_get_name(obj,i));
			obj_dump(obj_get_by_index(obj,i));
			printf("\n");
		}
		rec_lvl-=3;
		prefix(rec_lvl);
	}
	printf(" ]");
}


void obj_add(obj_t container, const char*name, obj_t containee) {
	obj_id_container_add(&(container->_contents), name, containee);
}

void obj_set(obj_t container, const char*name, obj_t containee) {
	obj_id_container_set(&(container->_contents), name, containee);
}

obj_t obj_get_by_name(obj_t container, const char*name) {
	return obj_id_container_by_name(&(container->_contents),name);
}

obj_t obj_get_by_index(obj_t container, size_t index) {
	return obj_id_container_by_index(&(container->_contents),index);
}

id_string obj_get_name(obj_t container, size_t index) {
	return obj_id_container_name(&(container->_contents),index);
}


obj_t obj_clone(obj_t self) {
//	static int rec_lvl = 0;
	obj_t ret;
	if(!self) {
		return NULL;
	}
//	prefix(rec_lvl); printf("cloning obj %p class:%p\n",self,self->_class);
//	rec_lvl+=2;
	ret = obj_new(self->_class);
	if(self->content_bits&CTS_INTRINSIC) {
		ret->content_bits = self->content_bits;
		ret->_intrinsic.word = self->_intrinsic.word;
	} else {
		const char*k;
		int i;
		for(i=0;i<self->_contents.size;i+=1) {
			k=obj_get_name(self,i);
//			prefix(rec_lvl); printf("cloning key %s\n",k);
//			rec_lvl+=2;
			obj_add(ret,k?strdup(k):NULL,obj_clone(obj_get_by_index(self,i)));
//			rec_lvl-=2;
		}
//		prefix(rec_lvl);
//		obj_dump(ret);
//		fputc('\n',stdout);
	}
//	rec_lvl-=2;
	return ret;
}


/****************************************************
 * ENVIRONMENT MANAGEMENT
 ***************************************************/

#define ENV_STACK 0
#define ENV_SYMBOLS 1
#define _env_stack_ ((stack_t)(obj_id_container_by_index(&env->_contents,ENV_STACK)->_intrinsic.ptr))
#define _env_symbols_ (obj_id_container_by_index(&env->_contents,ENV_SYMBOLS))


void env_push_context(obj_t env, struct _msg_context_t* context) {
	stack_push(_env_stack_, context);
}

void env_pop_context(obj_t env) {
	(void)stack_pop(_env_stack_);
}

struct _msg_context_t* env_peek_context(obj_t env) {
	obj_t stack = obj_id_container_by_index(&env->_contents,ENV_STACK);
	if(!stack) {
		return NULL;
	}
	return (struct _msg_context_t*)stack_peek((stack_t)stack->_intrinsic.ptr);
}



void env_clone(obj_t ret, obj_t self) {
	static int rec=0;
	int i,j;
	const char*k;
	obj_t inner;

	rec+=1;

	if(self->content_bits) {
		ret->content_bits = self->content_bits;
		ret->_intrinsic.word = self->_intrinsic.word;
	} else {
		for(i=0;i<self->_contents.size;i++) {
			k = obj_get_name(self,i);
			inner = obj_new(NULL);
//			printf("%5.5i env-clone prepare #%i %s : %p\n",rec,i,k,inner);
			obj_add(ret, k?strdup(k):NULL, inner);
//			for(j=0;j<ret->_contents.size;j++) {
//				printf("%s %p, ",ret->_contents.table[j].sym,ret->_contents.table[j].obj);
//			}
//			printf("\n");
		}
		assert(ret->_contents.size==self->_contents.size);

		for(i=0;i<self->_contents.size;i++) {
//			printf("%5.5i env-clone into #%i %s : %p (src %p)\n",rec,i,obj_get_name(ret,i),
//				obj_get_by_index(ret,i), obj_get_by_index(self,i));
			env_clone(obj_get_by_index(ret,i), obj_get_by_index(self,i));
		}
	}
	rec-=1;
}

void env_set_classes(obj_t env, obj_t ret, obj_t self) {
	int i;
	const char*k;

	if(self->_class) {
		ret->_class = ml_env_find(env,(const char*)obj_get_by_name(self->_class,"name")->_intrinsic.ptr);
	}
	if(!self->content_bits) {
		for(i=0;i<self->_contents.size;i++) {
			env_set_classes(env,obj_get_by_index(ret,i), obj_get_by_index(self,i));
		}
	}

}

tinyap_t ml_parser(obj_t env) {
	return (tinyap_t)ml_env_find(env,"parser")->_intrinsic.ptr;
}

obj_t env_new() {
	static int tinyap_initialized = 0;
	obj_t ret = obj_new(NULL);
	obj_t parser;
	tinyap_t tp;

	env_clone(ret, &obj_Env);
	/* init stack */
	obj_get_by_index(ret,0)->_intrinsic.ptr = stack_new();
	env_set_classes(ret, ret, &obj_Env);
	/* init C parser object */
	if(!tinyap_initialized) {
		tinyap_init();
		tinyap_initialized=1;
	}
	tp = tinyap_new();
	/* FIXME : put file contents into a char* */
	tinyap_set_source_file(tp,"../ml/ml.gram");
	tinyap_parse_as_grammar(tp);
	printf(tinyap_serialize_to_string(tinyap_get_grammar_ast(tp)));
	parser=ml_env_find(ret,"parser");
	parser->_intrinsic.ptr = tp;

	printf("new env at %p with parser at %p\n",ret,tp);
	return ret;
}



void env_delete(obj_t env) {
	//TODO
}


obj_t ml_env_self(obj_t env) {
	struct _msg_context_t* context = env_peek_context(env);
	if(context) {
		return context->receiver;
	}
	return NULL;
}

#define find_in(_c,_n) (ret = obj_id_container_by_name(&_c,_n))

obj_t ml_env_find_in_obj(obj_t env, obj_t receiver, const char*name) {
	obj_t ret,class;
	if(!find_in(receiver->_contents, name)) {
		//printf(" not in object\n");
		class = receiver->_class;
		while(class) {
			//printf(" not in class\n");
			find_in(class->_contents, name);
			class = ret ? NULL:class->_class;
		}
	}
	return ret;
}


obj_t ml_env_find(obj_t env, const char* name) {
	struct _msg_context_t* context = env_peek_context(env);
	obj_t ret=NULL;
	//printf("ml_env_find(%s) in context %p\n",name,context);
	if(context) {
		//printf(" have context\n");
		if(!find_in(context->_parameters, name)) {
			//printf(" not in params\n");
			ret = ml_env_find_in_obj(env, context->receiver, name);
		}
	}
	if(!ret) {
		//printf("env find in global symbols...\n");
		ret = obj_get_by_name(_env_symbols_, name);
	}
	return ret;
}

void env_add(obj_t env, const char*name, obj_t obj) {
	obj_id_container_add(&_env_symbols_->_contents, name, obj);
}


/****************************************************
 * OBJECT MANAGEMENT
 ***************************************************/

obj_t obj_new(obj_t _class) {
	obj_t ret = (obj_t) malloc(sizeof(struct _obj_t));
	memset(ret,0,sizeof(struct _obj_t));
	ret->_class=_class;
	return ret;
}


obj_t obj_find(obj_t recv, const char* name) {
	int i;
	const char*sym;
	//obj_t class,ret;
	//printf("finding '%s' in %p\n",name,recv);
	if(recv->content_bits&CTS_INTRINSIC) {
		return NULL;
	}
	for(i=0;i<recv->_contents.size;i+=1) {
		sym=obj_id_container_name(&recv->_contents,i);
		//printf("  match against '%s'\n",sym);
		if(sym&&!strcmp(sym,name)) {
			return obj_id_container_by_index(&recv->_contents,i);
		}
	}
	if(recv->_class) {
		return obj_find(recv->_class,name);
	}
	return NULL;
}


obj_t ml_obj_eval(obj_t obj, obj_t env) {
	static size_t rec_count=0;
	obj_t inner,ret=NULL;
	if(!obj) {
		return NULL;
	}
	rec_count+=1;
	//printf("\n%4.4i ## EVAL %s %p\n",rec_count, obj_get_by_name(obj->_class,"name")->_intrinsic.ptr,obj);
	if(obj->content_bits&CTS_INTRINSIC) {
		//printf("   obj has intrinsic value (%i)\n",obj->content_bits&(~CTS_INTRINSIC));
		switch(obj->content_bits&(~CTS_INTRINSIC)) {
		case CTS_FUNC:
			ret = obj->_intrinsic.c_func(obj,env);
			break;
		default:
			ret = obj;
			break;
		};
	} else {
		/* try and send eval message to self */
		inner = obj_find(obj,"eval");
		//printf("eval of %p uses %p\n",obj,inner);
		if(inner) {
			/* FIXME : find a better way to propagate parameters */
			struct _obj_t local = { 0, 0, Cts(0), {0} };
			struct _msg_context_t* ctxt = env_peek_context(env);
			if(ctxt) {
				if(ctxt->_parameters.size) {
					memcpy(&local._contents, &ctxt->_parameters,sizeof(struct _obj_id_container_t));
				}
			}
			local._class = ml_env_find(env, "Object");
			ret = msg_send(obj, inner, &local, env);
		}
	}
	if(!ret) {
		fprintf(stderr,"%4.4i WARNING : object evaluated to nil\n",rec_count);
		obj_dump(obj);
		fputc('\n',stdout);
	}
	rec_count-=1;
	return ret;
}


obj_t ml_eval_msg_list(obj_t self, obj_t env) {
	int i;
	obj_t _this,ret=NULL;
	_this = ml_env_self(env);
	//printf("ml_eval_msg_list for %i messages\n",_this->_contents.size);
	//_this = ml_env_find(env,"self");
	for(i=0;i<_this->_contents.size;i+=1) {
		ret = ml_obj_eval(obj_get_by_index(_this,i),env);
	}
	return ret;
}





obj_t ml_eval_defrd_msg_list(obj_t self, obj_t env) {
	//printf("ml_eval_defrd_msg_list\n");
	return obj_get_by_name(ml_env_self(env),"messages");
}





obj_t msg_send(obj_t receiver, obj_t method, obj_t params, obj_t env) {
	int i;
	struct _msg_context_t ctxt;
	obj_t ret;
	memset(&ctxt._parameters,0,sizeof(struct _obj_id_container_t));

	//printf("msg_send(%p,%p,%p,%p)\n",receiver,method,params,env);

	ctxt.receiver = receiver;
	ctxt.method = method; //obj_find(receiver,(const char*)self->_contents[1].obj->_intrinsic.ptr);

	//obj_id_container_add(&ctxt._parameters, "self", receiver);

	if(params) {
		for(i=0;i<params->_contents.size;i+=1) {
			ret = ml_obj_eval(obj_id_container_by_index(&params->_contents,i),env);
			//printf("  adding parameter %s [%p]\n", obj_id_container_name(&params->_contents,i), ret);
			obj_id_container_add(	&ctxt._parameters,
						obj_id_container_name(&params->_contents,i),
						ret);
		}
	}

	env_push_context(env,&ctxt);
	ret = ml_obj_eval(method,env);
	env_pop_context(env);

	//printf("   => %p\n",ret);
	return ret;
}



