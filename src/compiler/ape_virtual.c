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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <tinyape.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "vm_assert.h"

#include "vm.h"
#include "opcode_chain.h"
#include "opcode_dict.h"
#include "program.h"
#include "_impl.h"

/* hidden tinyap feature */
/*ast_node_t newAtom(const char*data, unsigned int offset);*/
/*ast_node_t newPair(const ast_node_t a,const ast_node_t d);*/


extern volatile long _vm_trace;
const char* state2str(WalkDirection wd);

WalkDirection try_method(const char*op, vm_t vm, wast_t node) {
	char* vec_name = (char*)malloc(strlen(op)+strlen(vm->virt_walker)+8);
	word_t vec_ofs;
	sprintf(vec_name,".virt_%s_%s", vm->virt_walker, op);
	vec_ofs = (word_t)hash_find(&vm->compile_vectors.by_text,(hash_key)vec_name);
	free(vec_name);
	if(!vec_ofs) {
		/* try default method instead */
		vec_name = (char*)malloc(8+strlen(vm->virt_walker)+8);
		sprintf(vec_name,".virt_%s___dflt__", vm->virt_walker);
		vec_ofs = (word_t)hash_find(&vm->compile_vectors.by_text,(hash_key)vec_name);
		free(vec_name);
	}
	if(vec_ofs) {
		WalkDirection backup = vm->compile_state;
		program_t p = (program_t)*(vm->compile_vectors.by_index.data+vec_ofs);
		word_t ip = *(vm->compile_vectors.by_index.data+vec_ofs+1);
		if(_vm_trace) {
			vm_printf("\nVWALKER_VECTOR %s::%s [%p:%lX]\n", vm->virt_walker, op, p,ip);
		}
/*		vm_printf("virtual walker calling %p:%lX (%s)\n",p,ip,op);*/
		gpush(&vm->cn_stack, &vm->current_node);
		vm->current_node = node;
		vm_run_program_fg(vm,p,ip,50);
		vm->current_node=*(wast_t*)_gpop(&vm->cn_stack);
		vm->virt_walker_state=vm->compile_state;
		vm->compile_state=backup;
/*		vm_printf("   vm return state : %i\n",vm->virt_walker_state);*/
		if(_vm_trace) {
			vm_printf("\nVWALKER_VECTOR %s::%s return state : %s\n", vm->virt_walker, op, state2str(vm->compile_state));
		}
		return vm->virt_walker_state;
	}

	vm_printerrf("In walker '%s' : Node '%s' is not known.\n",vm->virt_walker,op);
	return Error;
}



void* ape_virtual_init(vm_t vm) {
	/* allow reentrant calls */
	gpush(&vm->cn_stack,&vm->current_node);
	try_method("__init__", vm, vm->current_node);
	return vm;
}


void ape_virtual_free(vm_t vm) {
	try_method("__term__", vm, vm->current_node);
	vm->current_node=*(wast_t*)_gpop(&vm->cn_stack);
	vm->compile_state=vm->virt_walker_state;
}


WalkDirection ape_virtual_default(wast_t node, vm_t vm) {
	/*vm_printf("search for vector %s in %p gave %lu\n",vec_name, vm->result, vec_ofs);*/
	return try_method(wa_op(node),vm,node);
}


