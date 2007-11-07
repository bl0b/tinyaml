#include "core.h"

obj_t ml_print(obj_t self, obj_t env) {
	obj_t string = ml_env_find(env, "string");
	fputs((const char*)string->_intrinsic.ptr,stdout);
	return ml_env_self(env);
}

obj_t ml_newline(obj_t self, obj_t env) {
	fputc('\n',stdout);
	return ml_env_self(env);
}

/* AST objects bear an intrinsic value (CTS_PTR), which is the wast_t handle. */
DeclObj(obj_IO, &class_object, 0,
Cts(3,
	NAME("io"),
	_e("print", ObjCFunc(ml_print)),
	_e("newLine", ObjCFunc(ml_newline)),
), 0);
