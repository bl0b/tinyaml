#include "core.h"
#include "../stack.h"

#define _s(_a,_b,_c) (struct _stack_t[]){_a,_b,_c}

size_t env_stack[3]= { 0, 0, 0 };

DeclObj(obj_Env,
	&class_object, 0,
	Cts(3,
		_e("stack", Obj(&class_stack,CTS_INTRINSIC|CTS_PTR,Cts(0),&env_stack)),
		_e("symbols", Obj(&class_object,0, Cts(15,
			_e( "parser", &obj_parser ),
			_e( "Object", &class_object ),
			_e( "Class", &class_class ),
			_e( "Ref", &class_ref ),
			_e( "Message", &class_message ),
			_e( "MessageList", &class_message_list ),

			_e( "DeferredMessageList", &class_deferred_message_list ),
			_e( "String", &class_string ),
			_e( "Int", &class_int ),
			_e( "Float", &class_float ),
			_e( "CFunc", &class_func ),

			_e( "AST", &class_AST ),
			_e( "Stack", &class_stack ),
			_e( "io", &obj_IO ),
			_e( "compiler", &obj_Compiler ),

		),0)),
		NAME("env")
	), 0);

