#include "core.h"

obj_t ml_obj_clone(obj_t self, obj_t env) {
	return obj_clone(ml_env_self(env));
}

obj_t ml_obj_ref(obj_t self, obj_t env) {
	return ml_env_self(env);
	//return ml_env_find(env,"self");
}

obj_t ml_obj_get(obj_t self, obj_t env) {
	obj_t name = ml_env_find(env,"name");
	if(name) {
		return obj_get_by_name(ml_env_self(env),(const char*)name->_intrinsic.ptr);
	} else {
		name = ml_env_find(env,"index");
		return obj_get_by_index(ml_env_self(env),name->_intrinsic.i);
	}
}


obj_t ml_obj_export(obj_t self, obj_t env) {
	obj_t name = ml_env_find(env,"name");
	self = ml_env_self(env);
	env_add(env, (const char*)name->_intrinsic.ptr, self);
	return self;
}

obj_t ml_obj_add(obj_t self, obj_t env) {
	obj_t name = ml_env_find(env,"name");
	obj_t value = ml_env_find(env,"value");
	self = ml_env_self(env);
	obj_set(self,(const char*)name->_intrinsic.ptr,value);
	return self;
}

obj_t ml_obj_foreach(obj_t self, obj_t env) {
	return ml_env_self(env);
}

obj_t ml_obj_spawn(obj_t self, obj_t env) {
	return obj_new(ml_env_self(env));
}

obj_t ml_obj_receive(obj_t self, obj_t env) {
	return ml_env_self(env);
}

obj_t ml_ref_find(obj_t self, obj_t env) {
	return ml_env_self(env);
}



DeclObj(class_object,
	NULL, 0,
	Cts(10,	NAME("Object"),
		_e( "ref", ObjCFunc(ml_obj_ref) ),
		_e( "clone", ObjCFunc(ml_obj_clone) ),
		_e( "spawn", ObjCFunc(ml_obj_spawn) ),
		_e( "receive", ObjCFunc(ml_obj_receive) ),

		_e( "get", ObjCFunc(ml_obj_get) ),
		_e( "add", ObjCFunc(ml_obj_add) ),
		_e( "foreach", ObjCFunc(ml_obj_foreach) ),
		_e( "eval", ObjCFunc(ml_obj_ref) ),
		_e( "export", ObjCFunc(ml_obj_ref) ),
	),0
);


