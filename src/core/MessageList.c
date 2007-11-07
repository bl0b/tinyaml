#include "core.h"

DeclObj(class_message_list,
	&class_class, 0,
	Cts(2, NAME("MessageList"), _e( "eval", ObjCFunc(ml_eval_msg_list))),0
);

