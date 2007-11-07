#include "core.h"

/* Message is
 * - evaluatur receiver
 * - string messagename
 * - array params
 */

obj_t ml_msg_emit(obj_t self, obj_t env) {
	obj_t receiver;
	obj_t method;

	self = ml_env_self(env);

	receiver = ml_obj_eval(obj_id_container_by_index(&self->_contents,0), env);
	if(!receiver) {
		fprintf(stderr,"ERROR : no receiver\n");
		return NULL;
	}
	//printf("message receiver is %p\n",receiver);
	//method = obj_get_by_name(receiver, (const char*) (obj_id_container_by_index(&self->_contents,1)->_intrinsic.ptr));
	method = ml_env_find_in_obj(env, receiver, (const char*) (obj_id_container_by_index(&self->_contents,1)->_intrinsic.ptr));
	if(!method) {
		fprintf(stderr,"ERROR : receiver doesn't implement method %s\n",(const char*) (obj_id_container_by_index(&self->_contents,1)->_intrinsic.ptr));
		return NULL;
	}
	//printf("        method is %p\n",method);

	return msg_send(receiver, method, obj_id_container_by_index(&self->_contents,2), env);
}


DeclObj(class_message,
	&class_class, 0,
	Cts(2, NAME("Message"), _e("eval", ObjCFunc(ml_msg_emit))),0
);


