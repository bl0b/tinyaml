/* TinyaML
 * Copyright (C) 2007 Damien Leroux
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "vm.h"
#include "_impl.h"
#include <stdio.h>
#include <string.h>

#include <math.h>
#include "fastmath.h"
#include "text_seg.h"
#include "opcode_chain.h"
#include "object.h"

ast_node_t ast_unserialize(const char*);

void _VM_CALL vm_op__langDef_String(vm_t vm, const char* sernode) {
	word_t test = text_seg_text_to_index(&vm->gram_nodes,sernode);
	if(!test) {
		/*printf("ML::appending new grammar rules\n");*/
		ast_node_t n = ast_unserialize(text_seg_find_by_text(&vm->gram_nodes,sernode));
		tinyap_append_grammar(vm->parser,n);
	}
}

void _VM_CALL vm_op__langPlug_String(vm_t vm, const char* plugin) {
	vm_data_t arg = _vm_pop(vm);
	const char* plug = (const char*) arg->data;
	assert(arg->type==DataString);
	/*printf("ML::plugging %s into %s\n",plugin,plug);*/
	tinyap_plug(vm->parser,plugin,plug);
}



void _VM_CALL vm_op_write_data(vm_t vm, word_t x) {
	char tmp_r[512], tmp_d[512];
	_IFC conv;
	vm_data_t d = _vm_pop(vm);
	vm_data_t rep = _vm_pop(vm);
	assert(rep->type==DataInt);
	sprintf(tmp_r,"%lu",rep->data);
	switch(d->type) {
	case DataInt:
		sprintf(tmp_d,"%li",d->data);
		opcode_chain_add_data(vm->result, d->type, tmp_d, tmp_r);
		break;
	case DataFloat:
		conv.i = d->data;
		sprintf(tmp_d,"%f",conv.f);
		opcode_chain_add_data(vm->result, d->type, tmp_d, tmp_r);
		break;
	case DataString:
		/*printf("add String data \"%s\"\n",(const char*)d->data);*/
		opcode_chain_add_data(vm->result, d->type, (const char*)d->data, tmp_r);
		break;
	case DataObject:
		fprintf(stderr,"[COMPILER:ERR] Inline Objects aren't allowed in the data segment !\n");
	default:;
	}
	/*printf("vm_op_write_data\n");*/
}

void _VM_CALL vm_op_write_label_String(vm_t vm, const char* label) {
	opcode_chain_add_label(vm->result,label);
	/*printf("vm_op_write_Label_String\n");*/
}

void _VM_CALL vm_op_write_oc_String(vm_t vm, const char* name) {
	opcode_chain_add_opcode(vm->result, OpcodeNoArg, name, NULL);
	/*printf("vm_op_write_oc_String %s\n",name);*/
}

void _VM_CALL vm_op_write_oc_String_String(vm_t vm, const char* name) {
	vm_data_t arg = _vm_pop(vm);	/* -1 becomes 0 */
	const char*argstr;
	assert(arg->type==DataString);
	argstr = (const char*) arg->data;
	/*printf("vm_op_write_oc_String_String %s %s\n",name,argstr);*/
	opcode_chain_add_opcode(vm->result, OpcodeArgString, name, argstr);
}

void _VM_CALL vm_op_write_oc_Int_String(vm_t vm, const char* name) {
	vm_data_t arg = _vm_pop(vm);	/* -1 becomes 0 */
	char argstr[512];
	assert(arg->type==DataInt);
	sprintf(argstr,"%li",(long int)arg->data);
	/*printf("vm_op_write_oc_Int_String %s %s\n",name,argstr);*/
	opcode_chain_add_opcode(vm->result, OpcodeArgInt, name, argstr);
}

void _VM_CALL vm_op_write_oc_Label_String(vm_t vm, const char* name) {
	vm_data_t arg = _vm_pop(vm);	/* -1 becomes 0 */
	char argstr[512];
	assert(arg->type==DataInt);
	sprintf(argstr,"%li",(long int)arg->data);
	/*printf("vm_op_write_oc_Label_String %s %s\n",name,argstr);*/
	opcode_chain_add_opcode(vm->result, OpcodeArgLabel, name, argstr);
}

void _VM_CALL vm_op_write_oc_EnvSym_String(vm_t vm, const char* name) {
	vm_data_t arg = _vm_pop(vm);	/* -1 becomes 0 */
	char argstr[512];
	assert(arg->type==DataInt);
	sprintf(argstr,"%li",(long int)arg->data);
	/*printf("vm_op_write_oc_Label_String %s %s\n",name,argstr);*/
	opcode_chain_add_opcode(vm->result, OpcodeArgEnvSym, name, argstr);
}

void _VM_CALL vm_op_write_oc_Float_String(vm_t vm, const char* name) {
	vm_data_t arg = _vm_pop(vm);	/* -1 becomes 0 */
	char argstr[512];
	_IFC conv;
	assert(arg->type==DataFloat);
	conv.i=arg->data;
	sprintf(argstr,"%f",conv.f);
	/*printf("vm_op_write_oc_Float_String %s %s\n",name,argstr);*/
	opcode_chain_add_opcode(vm->result, OpcodeArgFloat, name, argstr);
}




void _VM_CALL vm_op___addCompileMethod_Label(vm_t vm, int rel_ofs) {
	thread_t t=vm->current_thread;
	vm_data_t local = _vm_pop(vm);	/* -1 becomes 0 */
	word_t ofs = t->IP+rel_ofs;
	word_t vec_ofs;
	const char*name;
	if(local->type!=DataString) {
		printf("[VM:ERR] willingly obfuscated internal error.\n");
		return;
	}
	vec_ofs = (word_t)hash_find(&vm->compile_vectors.by_text,(hash_elem)local->data);
	if(vec_ofs) {
		printf("[VM:ERR] vector already exists !\n");
		return;
	}
	name = strdup((const char*)local->data);
	vec_ofs = dynarray_size(&vm->compile_vectors.by_index);
	/*printf("adding compile method for %s\n",(const char*)local->data);*/
	hash_addelem(&vm->compile_vectors.by_text, (hash_key) name, (hash_elem) vec_ofs);
	dynarray_set(&vm->compile_vectors.by_index, vec_ofs, (word_t) t->program);
	dynarray_set(&vm->compile_vectors.by_index, vec_ofs+1, ofs);
}



void _VM_CALL vm_op_newSymTab(vm_t vm, int rel_ofs) {
	vm_push_data(vm, DataObject, (word_t) vm_symtab_new(vm));
}


void _VM_CALL vm_op_symTabSz(vm_t vm, word_t x) {
	vm_data_t t = _vm_pop(vm);
	text_seg_t ts = (text_seg_t) t->data;
	assert(t->type==DataObject);
	vm_push_data(vm,DataInt,ts->by_index.size);
}


void _VM_CALL vm_op_getSym(vm_t vm, word_t x) {
	vm_data_t k = _vm_pop(vm);
	vm_data_t t = _vm_pop(vm);
	word_t idx;
	text_seg_t ts = (text_seg_t) t->data;
	assert(t->type==DataObject);
	assert(k->type==DataString);
	idx=text_seg_text_to_index(ts, (const char*)k->data);
	/*printf("getSym(%s) => %lu\n",(const char*)k->data,idx);*/
	vm_push_data(vm,DataInt, idx);
}

void _VM_CALL vm_op_addSym(vm_t vm, word_t x) {
	vm_data_t k = _vm_pop(vm);
	vm_data_t t = _vm_pop(vm);
	text_seg_t ts = (text_seg_t) t->data;
	assert(t->type==DataObject);
	assert(k->type==DataString);
	(void)text_seg_find_by_text(ts, (const char*)k->data);
	/*printf("addSym(%s) => %lu\n",(const char*)k->data,text_seg_text_to_index(ts, (const char*)k->data));*/
}


void _VM_CALL vm_op_compileStateNext(vm_t vm, word_t x) {
	vm->compile_state=Next;
}

void _VM_CALL vm_op_compileStateDown(vm_t vm, word_t x) {
	vm->compile_state=Down;
}

void _VM_CALL vm_op_compileStateUp(vm_t vm, word_t x) {
	vm->compile_state=Up;
}

void _VM_CALL vm_op_compileStateDone(vm_t vm, word_t x) {
	vm->compile_state=Done;
}

void _VM_CALL vm_op_compileStateError(vm_t vm, word_t x) {
	vm->compile_state=Error;
}

void _VM_CALL vm_op_astGetOp(vm_t vm, word_t x) {
	vm_push_data(vm,DataString,(word_t)wa_op(vm->current_node));
}

void vm_dump_data_stack(vm_t vm);
void _VM_CALL vm_op_astGetChildrenCount(vm_t vm, word_t x) {
	/*printf("vm_op_astGetChildrenCount => %u\n",wa_opd_count(vm->current_node));*/
	vm_push_data(vm,DataInt,wa_opd_count(vm->current_node));
	/*vm_dump_data_stack(vm);*/
}

void _VM_CALL vm_op_astCompileChild_Int(vm_t vm, word_t x) {
	assert(x<wa_opd_count(vm->current_node));
	/*printf("calling sub compiler\n");*/
	/*tinyap_walk(wa_opd(vm->current_node,x),"prettyprint",NULL);*/
	tinyap_walk(wa_opd(vm->current_node,x), "compiler", vm);
}

void _VM_CALL vm_op_astCompileChild(vm_t vm, word_t x) {
	vm_data_t d = _vm_pop(vm);
	assert(d->type==DataInt);
	/*tinyap_walk(vm->current_node, "prettyprint", vm);*/
	assert(d->data<wa_opd_count(vm->current_node));
	vm_op_astCompileChild_Int(vm,d->data);
}


void _VM_CALL vm_op__pop_curNode(vm_t vm, word_t x) {
	/*printf("VM pop cur node !\n");*/
	vm->current_node=*(wast_t*)_gpop(&vm->cn_stack);
}


void _VM_CALL vm_op_astGetChildString_Int(vm_t vm, word_t x) {
	/*printf("astGetChildString(%lu) : <%s> ip=%lX\n",x,wa_op(wa_opd(vm->current_node,x)),vm->current_thread->IP);*/
	/*printf("stack size %lu\n",vm->current_thread->data_stack.sp);*/
	vm_push_data(vm,DataString,(word_t)wa_op(wa_opd(vm->current_node,x)));
	/*printf("stack size %lu\n",vm->current_thread->data_stack.sp);*/
}

void _VM_CALL vm_op_astGetChildString(vm_t vm, word_t x) {
	vm_data_t d = _vm_pop(vm);
	assert(d->type==DataInt);
	vm_op_astGetChildString_Int(vm,d->data);
}


void _VM_CALL vm_op_pp_curNode(vm_t vm, word_t x) {
	tinyap_walk(vm->current_node,"prettyprint",NULL);
}

void _VM_CALL vm_op_compileString(vm_t vm, word_t unused) {
	vm_data_t d = _vm_pop(vm);
	const char*buffer = (const char*)d->data;
	assert(d->type==DataString||d->type==DataObject);

	tinyap_set_source_buffer(vm->parser,buffer,strlen(buffer));
	tinyap_parse(vm->parser);

	if(tinyap_parsed_ok(vm->parser)&&tinyap_get_output(vm->parser)) {
		wast_t wa = tinyap_make_wast( tinyap_list_get_element( tinyap_get_output(vm->parser), 0) );
		tinyap_walk(wa, "compiler", vm);
		wa_del(wa);
	} else {
		fprintf(stderr,"parse error at %i:%i\n%s",tinyap_get_error_row(vm->parser),tinyap_get_error_col(vm->parser),tinyap_get_error(vm->parser));
	}
}

