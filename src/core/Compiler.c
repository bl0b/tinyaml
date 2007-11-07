#include "core.h"
#include <tinyap.h>



obj_t ml_add_cpl(obj_t self, obj_t env) {
	obj_t container = obj_get_by_index(ml_env_self(env),0);
	const char*name = ml_env_find(env,"name")->_intrinsic.ptr;
	obj_t body = ml_env_find(env,"body");
	obj_set(container,name,body);
	return ml_env_self(env);
}


DeclObj(obj_Compiler, &class_object, 0,
Cts(2,
	_e( "compileMethods", Obj(&class_object,0,Cts(0),0)),
	_e( "addCompileMethod", ObjCFunc(ml_add_cpl)),
	NAME( "compiler" ),
), 0);

