#include "obj.h"
#include <stdio.h>
#include <stdlib.h>
#include <tinyap.h>

void obj_add(obj_t,const char*,obj_t);
obj_t ml_env_self(obj_t);

obj_t ml_print(obj_t self, obj_t env) {
	obj_t string = ml_env_find(env, "string");
	fputs((const char*)string->_intrinsic.ptr,stdout);
	return ml_env_self(env);
}

obj_t ml_newline(obj_t self, obj_t env) {
	fputc('\n',stdout);
	return ml_env_self(env);
}

obj_t new_string(const char* str) {
	obj_t ret = obj_new(&class_string);
	ret->_intrinsic.ptr = (void*)str;
	return ret;
}


int main() {
	tinyap_t parser;
	wast_t wa;
	obj_t env = env_new();

	parser = ml_parser(env);

	//printf(tinyap_serialize_to_string(tinyap_get_grammar_ast(parser)));

	tinyap_set_source_file(parser,"../ml/test.ml");
	tinyap_parse(parser);
	if(tinyap_parsed_ok(parser)) {
		wa = tinyap_make_wast(tinyap_list_get_element(tinyap_get_output(parser),0));
		//tinyap_walk(wa,"prettyprint",NULL);
		tinyap_walk(wa,"compiler",env);
//	printf(tinyap_serialize_to_string(tinyap_get_grammar_ast(parser)));

		tinyap_free_wast(wa);
	}

/*	tinyap_set_source_file(parser,"../ml/test_dot.ml");
	tinyap_parse(parser);
	if(tinyap_parsed_ok(parser)) {
		wa = tinyap_make_wast(tinyap_list_get_element(tinyap_get_output(parser),0));
		//tinyap_walk(wa,"prettyprint",NULL);
		tinyap_walk(wa,"compiler",env);
//	printf(tinyap_serialize_to_string(tinyap_get_grammar_ast(parser)));

		tinyap_free_wast(wa);
	}
*/

/*	tinyap_init();

	parser = tinyap_new();

	tinyap_set_source_file(parser,"../ml/ml.gram");
	tinyap_parse_as_grammar(parser);
	if(tinyap_parsed_ok(parser)) {
		tinyap_set_source_file(parser,"../ml/test.ml");
		tinyap_parse(parser);
		if(tinyap_parsed_ok(parser)) {
			wa = tinyap_make_wast(tinyap_list_get_element(tinyap_get_output(parser),0));
			//tinyap_walk(wa,"prettyprint",NULL);
			tinyap_walk(wa,"compiler",env);
			tinyap_free_wast(wa);
		}
	}

	tinyap_delete(parser);
*/

	obj_dump(env);

	return 0;
}

