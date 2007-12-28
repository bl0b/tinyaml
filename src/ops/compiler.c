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

void _VM_CALL vm_op_write_label_String(vm_t vm, const char* label) {
	opcode_chain_add_label(vm->result,label);
	/*printf("vm_op_write_Label_String\n");*/
}

void _VM_CALL vm_op_write_oc_String(vm_t vm, const char* name) {
	opcode_chain_add_opcode(vm->result, OpcodeNoArg, name, NULL);
	/*printf("vm_op_write_oc_String\n");*/
}

void _VM_CALL vm_op_write_oc_String_String(vm_t vm, const char* name) {
	vm_data_t arg = _vm_pop(vm);	/* -1 becomes 0 */
	const char*argstr;
	assert(arg->type==DataString);
	/*printf("vm_op_write_oc_String_String\n");*/
	argstr = (const char*) arg->data;
	opcode_chain_add_opcode(vm->result, OpcodeArgString, name, argstr);
}

void _VM_CALL vm_op_write_oc_Int_String(vm_t vm, const char* name) {
	vm_data_t arg = _vm_pop(vm);	/* -1 becomes 0 */
	char argstr[20];
	assert(arg->type==DataInt);
	/*printf("vm_op_write_oc_Int_String\n");*/
	sprintf(argstr,"%li",(long int)arg->data);
	opcode_chain_add_opcode(vm->result, OpcodeArgInt, name, argstr);
}

void _VM_CALL vm_op_write_oc_Label_String(vm_t vm, const char* name) {
	vm_data_t arg = _vm_pop(vm);	/* -1 becomes 0 */
	char argstr[20];
	assert(arg->type==DataInt);
	/*printf("vm_op_write_oc_Label_String\n");*/
	sprintf(argstr,"%li",(long int)arg->data);
	opcode_chain_add_opcode(vm->result, OpcodeArgFloat, name, argstr);
}

void _VM_CALL vm_op_write_oc_Float_String(vm_t vm, const char* name) {
	vm_data_t arg = _vm_pop(vm);	/* -1 becomes 0 */
	char argstr[40];
	_IFC conv;
	assert(arg->type==DataFloat);
	/*printf("vm_op_write_oc_Float_String\n");*/
	conv.i=arg->data;
	sprintf(argstr,"%f",conv.f);
	opcode_chain_add_opcode(vm->result, OpcodeArgFloat, name, argstr);
}




void _VM_CALL vm_op___addCompileMethod_Label(vm_t vm, int rel_ofs) {
	thread_t t=node_value(thread_t,vm->current_thread);
	vm_data_t local = _vm_pop(vm);	/* -1 becomes 0 */
	word_t ofs = t->IP+rel_ofs;
	word_t vec_ofs;
	const char*name = strdup((const char*)local->data);
	if(local->type!=DataString) {
		printf("[VM:ERR] willingly obfuscated internal error.\n");
		return;
	}
	vec_ofs = (word_t)hash_find(&vm->compile_vectors.by_text,(hash_elem)local->data);
	if(vec_ofs) {
		printf("[VM:ERR] vector already exists !\n");
		return;
	}
	vec_ofs = dynarray_size(&vm->compile_vectors.by_index);
	/*printf("adding compile method for %s\n",(const char*)local->data);*/
	hash_addelem(&vm->compile_vectors.by_text, (hash_key) name, (hash_elem) vec_ofs);
	dynarray_set(&vm->compile_vectors.by_index, vec_ofs, (word_t) t->program);
	dynarray_set(&vm->compile_vectors.by_index, vec_ofs+1, ofs);
}



void _VM_CALL vm_op_newSymTab(vm_t vm, int rel_ofs) {
	void* handle = vm_obj_new(sizeof(struct _text_seg_t), (void(*)(void*))text_seg_deinit);
	text_seg_init((text_seg_t)handle);
	vm_push_data(vm, DataObject, (word_t) handle);
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
	text_seg_t ts = (text_seg_t) t->data;
	assert(t->type==DataObject);
	assert(k->type==DataString);
	vm_push_data(vm,DataInt, text_seg_text_to_index(ts, (const char*)k->data));
}

void _VM_CALL vm_op_addSym(vm_t vm, word_t x) {
	vm_data_t k = _vm_pop(vm);
	vm_data_t t = _vm_pop(vm);
	text_seg_t ts = (text_seg_t) t->data;
	assert(t->type==DataObject);
	assert(k->type==DataString);
	(void)text_seg_find_by_text(ts, (const char*)k->data);
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

void _VM_CALL vm_op_compileStateError(vm_t vm, word_t x) {
	vm->compile_state=Error;
}

void _VM_CALL vm_op_astGetOp(vm_t vm, word_t x) {
	vm_push_data(vm,DataString,(word_t)wa_op(vm->current_node));
}

void _VM_CALL vm_op_astGetChildrenCount(vm_t vm, word_t x) {
	vm_push_data(vm,DataString,wa_opd_count(vm->current_node));
}

void _VM_CALL vm_op_astCompileChild_Int(vm_t vm, word_t x) {
	tinyap_walk(wa_opd(vm->current_node,x), "compiler", vm);
}

void _VM_CALL vm_op_astCompileChild(vm_t vm, word_t x) {
	vm_data_t d = _vm_pop(vm);
	assert(d->type==DataInt);
	tinyap_walk(wa_opd(vm->current_node,d->data), "compiler", vm);
}


