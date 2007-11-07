#include "core.h"


obj_t ml_ref_deref(obj_t self, obj_t env) {
	obj_t ret;
	self = ml_env_self(env);
	//printf("deref'ing %p [target: %s]\n",self, (const char*)(
	//		obj_get_by_name(self,
	//				"target")
	//		->_intrinsic.ptr));
	ret = ml_env_find(env, (const char*)(
			obj_get_by_name(self,
					"target")
			->_intrinsic.ptr));
	//printf("deref'd to %p\n",ret);
	return ret;
}


obj_t ml_autoeval(obj_t self, obj_t env) {
	printf("-- autoeval --\n");
	return ml_env_self(env);
	//return ml_env_find(env,"self");
}


DeclObj(class_ref,
	&class_class, 0,
	Cts(2, NAME("Ref"), _e( "eval", ObjCFunc(ml_ref_deref))),0
);


