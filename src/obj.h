/*
 *   ________    _       __            __      _           __       ____        __     
 *  /_  __/ /_  (_)___  / /__   ____  / /_    (_)__  _____/ /_     / __ \____  / /_  __
 *   / / / __ \/ / __ \/ //_/  / __ \/ __ \  / / _ \/ ___/ __/    / / / / __ \/ / / / /
 *  / / / / / / / / / / ,<    / /_/ / /_/ / / /  __/ /__/ /__    / /_/ / / / / / /_/ / 
 * /_/ /_/ /_/_/_/ /_/_/|_|   \____/_.___/_/ /\___/\___/\__(_)   \____/_/ /_/_/\__, /  
 *                                      /___/                                 /____/   
 *                       __                   __                        __  __           
 *     ____  _________  / /_____  _________  / /____   ____ ___  ____ _/ /_/ /____  _____
 *    / __ \/ ___/ __ \/ __/ __ \/ ___/ __ \/ / ___/  / __ `__ \/ __ `/ __/ __/ _ \/ ___/
 *   / /_/ / /  / /_/ / /_/ /_/ / /__/ /_/ / (__  )  / / / / / / /_/ / /_/ /_/  __/ /  _ 
 *  / .___/_/   \____/\__/\____/\___/\____/_/____/  /_/ /_/ /_/\__,_/\__/\__/\___/_/  (_)
 * /_/                                                                                   
 */


#ifndef __OBJ_H__
#define __OBJ_H__

#include <stdlib.h>
#include <tinyap.h>

typedef struct _obj_id_t* obj_id_t;
typedef struct _obj_t* obj_t;

typedef const char* id_string;

struct _obj_id_t {
	id_string sym;
	obj_t obj;
};

#define REALLOC_GRANUL 32

#include "obj_id_container.h"

/* il faut hardcoder REF, CALL (send message), et GET */


#define Obj(_cls, _csz, _cts, _i) (struct _obj_t[]) {{ _cls, _csz, _cts, { (size_t) _i } }}
#define DeclObj(_x, _cls, _csz, _cts, _i) struct _obj_t _x = { _cls, _csz, _cts, { (size_t) _i } }

#define Cts( _n, _args... ) { _n, _n, (struct _obj_id_t[]) { _args } }
//#define Cts (struct _obj_id_t[])
#define _e( _a, _b) { _a, _b }

#define ObjString(_str) Obj(&class_string, CTS_INTRINSIC|CTS_STRING, Cts(0), (const char*)(_str))
#define ObjCFunc(_f) Obj(&class_func, CTS_INTRINSIC|CTS_FUNC, Cts(0), _f)
#define ObjCptr(_f) Obj(&class_object, CTS_INTRINSIC|CTS_PTR, Cts(0), _f)
#define ObjInt(_i) Obj(&class_int, CTS_INTRINSIC|CTS_INT, Cts(0), _i)

// sign bit denotes intrinsic value
#define CTS_INTRINSIC (1<<(sizeof(size_t)*8-1))

#define CTS_WORD	1
#define CTS_INT		2
#define CTS_FLOAT	3
#define CTS_PTR		4
#define CTS_FUNC	5

// PTR derivations
#define CTS_STRING	6
#define CTS_ARRAY	7
#define CTS_TABLE	8

struct _obj_t {
	obj_t _class;
	size_t content_bits;	// flags = intrinsic value ? CTS_INTRINSIC|CTS_... : 0
	struct _obj_id_container_t _contents;
	union {
		size_t word;
		int i;
		float f;
		void* ptr;
		obj_t (*c_func)(obj_t self, obj_t env);
	} _intrinsic;
};

struct _msg_context_t {
	obj_t receiver;
	obj_t method;
	struct _obj_id_container_t _parameters;
};



obj_t obj_new(obj_t _class);
obj_t obj_find(obj_t recv, const char* name);
obj_t msg_send(obj_t receiver, obj_t method, obj_t params, obj_t env);
void obj_add(obj_t container, const char*name, obj_t containee);
void obj_set(obj_t container, const char*name, obj_t containee);
obj_t obj_get_by_name(obj_t container, const char*name);
obj_t obj_get_by_index(obj_t container, size_t index);
id_string obj_get_name(obj_t container, size_t index);

obj_t ml_obj_eval(obj_t,obj_t);
obj_t ml_eval_msg_list(obj_t,obj_t);
obj_t ml_eval_defrd_msg_list(obj_t,obj_t);

obj_t obj_clone(obj_t self);


tinyap_t ml_parser(obj_t);
obj_t ml_env_find(obj_t,const char*);
obj_t ml_env_find_in_obj(obj_t env, obj_t receiver, const char*name);
obj_t ml_env_self(obj_t);
obj_t env_new();
void env_add(obj_t env, const char*name, obj_t obj);

#include "core/core.h"

#endif

