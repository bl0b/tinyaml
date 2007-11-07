#include "core.h"

DeclObj(class_deferred_message_list,
	&class_class, 0,
	Cts(2, NAME("DeferredMessageList"), _e( "eval", ObjCFunc(ml_eval_defrd_msg_list))),0
);

