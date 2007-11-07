#ifndef _TINYAML_CORE_H_
#define _TINYAML_CORE_H_

#include "../obj.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>

extern struct _obj_t
	class_object,
	class_class,
	class_ref,
	class_msessage_sequence,
	class_message,
	class_message_list,
	class_deferred_message_list,
	class_string,
	class_int,
	class_float,
	class_func,
	class_AST,
	class_stack,
	obj_Env,
	obj_IO,
	obj_parser,
	obj_Compiler;

#define NAME(_x) _e("name", ObjString(_x))

#endif
