#include "core.h"
#include <tinyap.h>


/* produce an AST object, which is compilable */
obj_t ml_parse(obj_t self, obj_t env) {
	obj_t string,file;
	wast_t wa;
	obj_t ret=NULL;
	tinyap_t parser = ml_parser(env);

	string = ml_env_find(env,"string");
	if(string) {
		const char*buffer = (const char*)string->_intrinsic.ptr;
		tinyap_set_source_buffer(parser,buffer,strlen(buffer));
		tinyap_parse(parser);
		if(tinyap_parsed_ok(parser)) {
			wa = tinyap_make_wast(tinyap_list_get_element(tinyap_get_output(parser),0));
			//tinyap_walk(wa,"prettyprint",NULL);
			//tinyap_walk(wa,"compiler",env);
			//tinyap_free_wast(wa);
			ret = obj_new(&class_AST);
			ret->content_bits = CTS_INTRINSIC|CTS_PTR;
			ret->_intrinsic.ptr = wa;
		}
	} else {
		file = ml_env_find(env,"file");
		if(file) {
			tinyap_set_source_file(parser,file->_intrinsic.ptr);
			tinyap_parse(parser);
			if(tinyap_parsed_ok(parser)) {
				wa = tinyap_make_wast(tinyap_list_get_element(tinyap_get_output(parser),0));
				ret = obj_new(&class_AST);
				ret->content_bits = CTS_INTRINSIC|CTS_PTR;
				ret->_intrinsic.ptr = wa;
			}
		}
	}
	return ret;
}



obj_t ml_plug(obj_t self, obj_t env) {
	const char*name = (const char*)ml_env_find(env,"name")->_intrinsic.ptr;
	const char*into = (const char*)ml_env_find(env,"into")->_intrinsic.ptr;
	tinyap_t parser = ml_parser(env);

	tinyap_plug(parser,name,into);

	return NULL;
}



DeclObj(obj_parser, &class_object, 0,
Cts(3,
	_e( "handle", ObjCptr(0)),
	_e( "parse", ObjCFunc(ml_parse)),
	_e( "plug", ObjCFunc(ml_plug)),
), 0);


