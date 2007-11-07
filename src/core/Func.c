#include "core.h"

obj_t ml_eval_func(obj_t obj, obj_t env) {
	return obj->_intrinsic.c_func(obj,env);
}


DeclObj(class_func,
	&class_class, 0,
	Cts(2, NAME("CFunc"), _e("eval",ObjCFunc(ml_eval_func))),0
);

