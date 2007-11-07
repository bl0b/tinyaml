#include "core.h"
#include <tinyape.h>

obj_t ml_tree_get(obj_t self, obj_t env) {
	obj_t ret;
	wast_t handle = ml_env_self(env)->_intrinsic.ptr;
	int opd = ml_env_find(env,"operand")->_intrinsic.i;
	ret = obj_new(ml_env_find(env,"AST"));
	ret->content_bits = CTS_INTRINSIC|CTS_PTR;
	ret->_intrinsic.ptr = wa_opd(handle,opd);
	return ret;
}

obj_t ml_tree_opd_list(obj_t self, obj_t env) {
	return NULL;
}

obj_t ml_compile_tree(obj_t self, obj_t env) {
	wast_t node;/*TODO*/
	return (obj_t)tinyap_walk(node,"compiler",env);
}

/* AST objects bear an intrinsic value (CTS_PTR), which is the wast_t handle. */
DeclObj(class_AST, &class_class, 0,
Cts(3,
	NAME("AST"),
	_e("get", ObjCFunc(ml_tree_get)),
	_e("operands", ObjCFunc(ml_tree_opd_list)),
	_e( "compile", ObjCFunc(ml_compile_tree)),
), 0);
