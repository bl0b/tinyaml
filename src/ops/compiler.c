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

void* try_walk(wast_t node, const char* pname, vm_t vm);

/*! \addtogroup vcop_comp
 * @{
 */


void _VM_CALL vm_op__langDef_String(vm_t vm, const char* sernode) {
	word_t test = text_seg_text_to_index(&vm->gram_nodes,sernode);
	/*vm_printf("langDef :: test=%i\n", test);*/
	if(test==-1) {
		/*vm_printf("ML::appending new grammar rules\n");*/
		ast_node_t n = ast_unserialize(text_seg_find_by_text(&vm->gram_nodes,sernode));
		tinyap_append_grammar(vm->parser,n);
	}
}

void _VM_CALL vm_op__langPlug_String(vm_t vm, const char* plugin) {
	vm_data_t arg = _vm_pop(vm);
	const char* plug = (const char*) arg->data;
	assert(arg->type==DataString);
	/*vm_printf("ML::plugging %s into %s\n",plugin,plug);*/
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
		opcode_chain_add_data(vm->result, d->type, tmp_d, tmp_r, -1, -1);
		break;
	case DataFloat:
		conv.i = d->data;
		sprintf(tmp_d,"%f",conv.f);
		opcode_chain_add_data(vm->result, d->type, tmp_d, tmp_r, -1, -1);
		break;
	case DataString:
		/*vm_printf("add String data \"%s\"\n",(const char*)d->data);*/
		opcode_chain_add_data(vm->result, d->type, (const char*)d->data, tmp_r, -1, -1);
		break;
	case DataManagedObjectFlag:
		vm_printerrf("[COMP:ERR] Inline Objects aren't allowed in the data segment !\n");
	default:;
	}
	/*vm_printf("vm_op_write_data\n");*/
}

void _VM_CALL vm_op_write_label_String(vm_t vm, const char* label) {
	opcode_chain_add_label(vm->result,label, -1, -1);
	/*vm_printf("vm_op_write_Label_String\n");*/
}

void _VM_CALL vm_op_write_label(vm_t vm, word_t unused) {
	vm_data_t d = _vm_pop(vm);
	assert(d->type==DataString||d->type==DataObjStr);
	vm_op_write_label_String(vm,(const char*)d->data);
}

void _VM_CALL vm_op_write_oc_String(vm_t vm, const char* name) {
	opcode_chain_add_opcode(vm->result, OpcodeNoArg, name, NULL, -1, -1);
	/*vm_printf("vm_op_write_ocString %s\n",name);*/
}

void _VM_CALL vm_op_write_oc(vm_t vm, word_t unused) {
	vm_data_t d = _vm_pop(vm);
	assert(d->type==DataString||d->type==DataObjStr);
	vm_op_write_oc_String(vm,(const char*)d->data);
}

void _VM_CALL vm_op_write_ocString_String(vm_t vm, const char* name) {
	vm_data_t arg = _vm_pop(vm);	/* -1 becomes 0 */
	const char*argstr;
	assert(arg->type==DataString||arg->type==DataObjStr);
	argstr = (const char*) arg->data;
	/*vm_printf("vm_op_write_ocString_String %s %s\n",name,argstr);*/
	opcode_chain_add_opcode(vm->result, OpcodeArgString, name, argstr, -1, -1);
}

void _VM_CALL vm_op_write_ocString(vm_t vm, word_t unused) {
	vm_data_t d = _vm_pop(vm);
	assert(d->type==DataString||d->type==DataObjStr);
	vm_op_write_ocString_String(vm,(const char*)d->data);
}

void _VM_CALL vm_op_write_ocInt_String(vm_t vm, const char* name) {
	vm_data_t arg = _vm_pop(vm);	/* -1 becomes 0 */
	char argstr[512];
	assert(arg->type==DataInt);
	sprintf(argstr,"%li",(long int)arg->data);
	/*vm_printf("vm_op_write_ocInt_String %s %s\n",name,argstr);*/
	opcode_chain_add_opcode(vm->result, OpcodeArgInt, name, argstr, -1, -1);
}

void _VM_CALL vm_op_write_ocInt(vm_t vm, word_t unused) {
	vm_data_t d = _vm_pop(vm);
	assert(d->type==DataString||d->type==DataObjStr);
	vm_op_write_ocInt_String(vm,(const char*)d->data);
}

void _VM_CALL vm_op_write_ocLabel_String(vm_t vm, const char* name) {
	vm_data_t arg = _vm_pop(vm);	/* -1 becomes 0 */
	assert(arg->type==DataString||arg->type==DataObjStr);
	/*vm_printf("vm_op_write_ocLabel_String %s %s\n",name,argstr);*/
	opcode_chain_add_opcode(vm->result, OpcodeArgLabel, name, (const char*)arg->data, -1, -1);
}

void _VM_CALL vm_op_write_ocLabel(vm_t vm, word_t unused) {
	vm_data_t d = _vm_pop(vm);
	assert(d->type==DataString||d->type==DataObjStr);
	vm_op_write_ocLabel_String(vm,(const char*)d->data);
}

void _VM_CALL vm_op_write_ocEnvSym_String(vm_t vm, const char* name) {
	vm_data_t arg = _vm_pop(vm);	/* -1 becomes 0 */
	/*char argstr[512];*/
	assert(arg->type==DataString||arg->type==DataObjStr);
	/*vm_printf("vm_op_write_ocLabel_String %s %s\n",name,argstr);*/
	opcode_chain_add_opcode(vm->result, OpcodeArgEnvSym, name, (char*)arg->data, -1, -1);
}

void _VM_CALL vm_op_write_ocEnvSym(vm_t vm, word_t unused) {
	vm_data_t d = _vm_pop(vm);
	assert(d->type==DataString||d->type==DataObjStr);
	vm_op_write_ocEnvSym_String(vm,(const char*)d->data);
}

void _VM_CALL vm_op_write_ocFloat_String(vm_t vm, const char* name) {
	vm_data_t arg = _vm_pop(vm);	/* -1 becomes 0 */
	char argstr[512];
	_IFC conv;
	assert(arg->type==DataFloat);
	conv.i=arg->data;
	sprintf(argstr,"%f",conv.f);
	/*vm_printf("vm_op_write_ocFloat_String %s %s\n",name,argstr);*/
	opcode_chain_add_opcode(vm->result, OpcodeArgFloat, name, argstr, -1, -1);
}


void _VM_CALL vm_op_write_ocFloat(vm_t vm, word_t unused) {
	vm_data_t d = _vm_pop(vm);
	assert(d->type==DataString||d->type==DataObjStr);
	vm_op_write_ocFloat_String(vm,(const char*)d->data);
}



void _VM_CALL vm_op___addCompileMethod_Label(vm_t vm, int rel_ofs) {
	thread_t t=vm->current_thread;
	vm_data_t local = _vm_pop(vm);	/* -1 becomes 0 */
	word_t ofs = t->IP+rel_ofs;
	word_t vec_ofs;
	const char*name;
	if(local->type!=DataString) {
		vm_printf("[VM:ERR] willingly obfuscated internal error.\n");
		return;
	}
	vec_ofs = (word_t)hash_find(&vm->compile_vectors.by_text,(hash_elem)local->data);
	if(vec_ofs) {
		vm_printf("[VM:ERR] vector %s already exists !\n", (const char*)local->data);
		return;
	}
	name = strdup((const char*)local->data);
	vec_ofs = dynarray_size(&vm->compile_vectors.by_index);
	/*vm_printf("adding compile method for %s\n",(const char*)local->data);*/
	hash_addelem(&vm->compile_vectors.by_text, (hash_key) name, (hash_elem) vec_ofs);
	dynarray_set(&vm->compile_vectors.by_index, vec_ofs, (word_t) t->program);
	dynarray_set(&vm->compile_vectors.by_index, vec_ofs+1, ofs);
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
	/*vm_printerrf("ERROR ! %s:%i\n", __FILE__, __LINE__);*/
	vm->compile_state=Error;
}

void _VM_CALL vm_op_astGetOp(vm_t vm, word_t x) {
	vm_push_data(vm,DataString,(word_t)wa_op(vm->current_node));
}

void _VM_CALL vm_op_astGetRow(vm_t vm, word_t x) {
	vm_push_data(vm,DataInt,(word_t)wa_row(vm->current_node));
}

void _VM_CALL vm_op_astGetCol(vm_t vm, word_t x) {
	vm_push_data(vm,DataInt,(word_t)wa_col(vm->current_node));
}

void vm_dump_data_stack(vm_t vm);
void _VM_CALL vm_op_astGetChildrenCount(vm_t vm, word_t x) {
	/*vm_printf("vm_op_astGetChildrenCount => %u\n",wa_opd_count(vm->current_node));*/
	vm_push_data(vm,DataInt,wa_opd_count(vm->current_node));
	/*vm_dump_data_stack(vm);*/
}

void _VM_CALL vm_op_astCompileChild_Int(vm_t vm, word_t x) {
	assert(x<wa_opd_count(vm->current_node));
/*	vm_printf("calling sub compiler on child #%lu\n", x);*/
/*	tinyap_walk(wa_opd(vm->current_node,x),"prettyprint",NULL);*/
	try_walk(wa_opd(vm->current_node,x), "compiler", vm);
}


void _VM_CALL vm_op_doWalk_String(vm_t vm, const char* walker) {
	const char* backup = vm->virt_walker;
	vm->virt_walker = walker;
	vm_compinput_push_walker(vm, walker);
	try_walk(vm->current_node, "virtual", vm);
	vm_compinput_pop(vm);
	vm->virt_walker = backup;
}

void _VM_CALL vm_op_doWalk(vm_t vm, word_t unused) {
	vm_data_t d = _vm_pop(vm);
	assert(d->type==DataString||d->type==DataObjStr);
	vm_op_doWalk_String(vm,(const char*)d->data);
}


void _VM_CALL vm_op_walkChild_Int(vm_t vm, word_t idx) {
	assert(idx<wa_opd_count(vm->current_node));
	/*vm_printf("calling sub compiler\n");*/
	/*try_walk(wa_opd(vm->current_node,x),"prettyprint",NULL);*/
	try_walk(wa_opd(vm->current_node,idx), "virtual", vm);
}


void _VM_CALL vm_op_walkChild(vm_t vm, word_t unused) {
	vm_data_t d = _vm_pop(vm);
	assert(d->type==DataInt);
	vm_op_walkChild_Int(vm,d->data);
}


void _VM_CALL vm_op_astCompileChild(vm_t vm, word_t unused) {
	vm_data_t d = _vm_pop(vm);
	assert(d->type==DataInt);
	/*try_walk(vm->current_node, "prettyprint", vm);*/
	assert(d->data<wa_opd_count(vm->current_node));
	vm_op_astCompileChild_Int(vm,d->data);
}


/*void _VM_CALL vm_op__pop_curNode(vm_t vm, word_t x) {*/
	/*vm_printf("VM pop cur node !\n");*/
	/*vm->current_node=*(wast_t*)_gpop(&vm->cn_stack);*/
/*}*/


void _VM_CALL vm_op_astGetChildString_Int(vm_t vm, word_t x) {
	/*vm_printf("astGetChildString(%lu) : <%s> ip=%lX\n",x,wa_op(wa_opd(vm->current_node,x)),vm->current_thread->IP);*/
	/*vm_printf("stack size %lu\n",vm->current_thread->data_stack.sp);*/
	vm_push_data(vm,DataString,(word_t)wa_op(wa_opd(vm->current_node,x)));
	/*vm_printf("stack size %lu\n",vm->current_thread->data_stack.sp);*/
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
	assert(d->type==DataString||d->type==DataObjStr);

	tinyap_set_source_buffer(vm->parser,buffer,strlen(buffer));
	tinyap_parse(vm->parser);

	if(tinyap_parsed_ok(vm->parser)&&tinyap_get_output(vm->parser)) {
		wast_t wa = tinyap_make_wast( tinyap_list_get_element( tinyap_get_output(vm->parser), 0) );
		vm_compinput_push_buffer(vm, buffer);
		try_walk(wa, "compiler", vm);
		vm_compinput_pop(vm);
		wa_del(wa);
	} else {
		vm_printerrf("parse error at %i:%i\n%s",tinyap_get_error_row(vm->parser),tinyap_get_error_col(vm->parser),tinyap_get_error(vm->parser));
	}
}

/*@}*/

/*! \addtogroup vcop_st
 * @{
 */
void _VM_CALL vm_op_newSymTab(vm_t vm, int rel_ofs) {
	vm_push_data(vm, DataObjSymTab, (word_t) vm_symtab_new(vm));
}


void _VM_CALL vm_op_symTabSz(vm_t vm, word_t x) {
	vm_data_t t = _vm_pop(vm);
	text_seg_t ts;
	if(t->type==DataObjEnv) {
		ts = & ((vm_dyn_env_t)t->data)->symbols;
	} else {
		assert(t->type==DataObjSymTab);
		ts = (text_seg_t) t->data;
	}
	vm_push_data(vm,DataInt,ts->by_index.size);
}


void _VM_CALL vm_op_getSym(vm_t vm, word_t x) {
	vm_data_t k = _vm_pop(vm);
	vm_data_t t = _vm_pop(vm);
	word_t idx;
	text_seg_t ts;
	if(t->type==DataObjEnv) {
		ts = & ((vm_dyn_env_t)t->data)->symbols;
	} else {
		assert(t->type==DataObjSymTab);
		ts = (text_seg_t) t->data;
	}
	assert(k->type==DataString||k->type==DataObjStr);
	idx=text_seg_text_to_index(ts, (const char*)k->data);
	/*vm_printf("getSym(%s) => %lu\n",(const char*)k->data,idx);*/
	vm_push_data(vm,DataInt, idx);
}

void _VM_CALL vm_op_getSymName(vm_t vm, word_t x) {
	vm_data_t k = _vm_pop(vm);
	vm_data_t t = _vm_pop(vm);
	char* sym;
	text_seg_t ts;
	if(t->type==DataObjEnv) {
		ts = & ((vm_dyn_env_t)t->data)->symbols;
	} else {
		assert(t->type==DataObjSymTab);
		ts = (text_seg_t) t->data;
	}
	assert(k->type==DataInt);
	sym=(char*)text_seg_find_by_index(ts, k->data);
	/*vm_printf("getSym(%s) => %lu\n",(const char*)k->data,idx);*/
	vm_push_data(vm,DataString, (word_t)sym);
}

void _VM_CALL vm_op_addSym(vm_t vm, word_t x) {
	vm_data_t k = _vm_pop(vm);
	vm_data_t t = _vm_pop(vm);
	text_seg_t ts = (text_seg_t) t->data;
	assert(t->type==DataObjSymTab);
	assert(k->type==DataString||k->type==DataObjStr);
	(void)text_seg_find_by_text(ts, (const char*)k->data);
	/*vm_printf("addSym(%s) => %lu\n",(const char*)k->data,text_seg_text_to_index(ts, (const char*)k->data));*/
}
/*@}*/

