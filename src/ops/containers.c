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
#include "opcode_chain.h"
#include "object.h"
#include "text_seg.h"

/**********************************************************
 * arrayNew
 * creates a managed array
 * nil -> Object
 */
void _VM_CALL vm_op_arrayNew(vm_t vm, word_t unused) {
	dynarray_t da = vm_array_new();
	/*printf("new array %p\n",da);*/
	vm_push_data(vm,DataObject,(word_t)da);
}

/**********************************************************
 * arrayResv:Int
 * reserves [arg] items in array
 * Object -> Object
 */
void _VM_CALL vm_op_arrayResv_Int(vm_t vm, word_t sz) {
	vm_data_t d = _vm_peek(vm);
	dynarray_t da = (dynarray_t) d->data;
	assert(d->type==DataObject);
	dynarray_reserve(da,sz<<1);
	/*printf("reserved %lu words for array %p\n",sz,da);*/
}

/**********************************************************
 * arrayResv
 * reserves some items in array
 * Object X Int -> Object
 */
void _VM_CALL vm_op_arrayResv(vm_t vm, word_t unused) {
	vm_data_t d = _vm_pop(vm);
	assert(d->type==DataInt);
	vm_op_arrayResv(vm,d->data);
}

/**********************************************************
 * arrayGet:Int
 * fetch some item at index [arg] in array
 * Object -> (something)
 */
void _VM_CALL vm_op_arrayGet_Int(vm_t vm, word_t index) {
	vm_data_t d = _vm_pop(vm);
	dynarray_t da = (dynarray_t) d->data;
	word_t ofs = index<<1;
	word_t* data;
	assert(d->type==DataObject);
	data = da->data+ofs;
	if(da->size>ofs+1) {
		vm_push_data(vm,*data,*(data+1));
		/*printf("get %lu:%8.8lx from array %p\n",*data,*(data+1),da);*/
	} else {
		dynarray_reserve(da,ofs+2);
		/* FIXME ? */
		printf("[ARRAY:WRN] index is out of bounds (%lu >= %lu\n",index,da->size);
		vm_push_data(vm,DataInt,0);
	}
}

/**********************************************************
 * arrayGet
 * fetch some item at some index in array
 * Object X Int -> (something)
 */
void _VM_CALL vm_op_arrayGet(vm_t vm, word_t unused) {
	vm_data_t d = _vm_pop(vm);
	assert(d->type==DataInt);
	vm_op_arrayGet_Int(vm,d->data);
}

/**********************************************************
 * arraySet:Int
 * set the item at index [arg] in array to (something)
 * Object X (something) -> Object
 */
void _VM_CALL vm_op_arraySet_Int(vm_t vm, word_t index) {
	vm_data_t data = _vm_pop(vm);
	vm_data_t d = _vm_peek(vm);
	dynarray_t da = (dynarray_t) d->data;
	word_t ofs = index<<1;
	vm_data_t da_data = (vm_data_t)(da->data+ofs);
	assert(d->type==DataObject);
	if(da->size>ofs+1) {
		if(da_data->type==DataObject) { vm_obj_deref(vm,(void*)da_data->data); }
	} else {
		dynarray_reserve(da,ofs+2);
		da_data = (vm_data_t)(da->data+ofs);
		da->size = ofs+2;
	}
	da_data->type = data->type;
	da_data->data = data->data;
	/*printf("set %u:%8.8lx in array %p\n",da_data->type,da_data->data,da);*/
	if(data->type==DataObject) { vm_obj_ref(vm,(void*)data->data); }
}

/**********************************************************
 * arraySet
 * set the item at some index in array to (something)
 * Object X (something) X Int -> Object
 */
void _VM_CALL vm_op_arraySet(vm_t vm, word_t unused) {
	vm_data_t d = _vm_pop(vm);
	assert(d->type==DataInt);
	vm_op_arraySet_Int(vm,d->data);
}

/**********************************************************
 * arraySize
 * get the current array size
 * Object -> Int
 */
void _VM_CALL vm_op_arraySize(vm_t vm, word_t unused) {
	vm_data_t d = _vm_pop(vm);
	dynarray_t da = (dynarray_t) d->data;
	assert(d->type==DataObject);
	vm_push_data(vm,DataInt,dynarray_size(da)>>1);
}



void _VM_CALL vm_op_envNew(vm_t vm, word_t unused) {
	vm_dyn_env_t env = vm_env_new();
	vm_push_data(vm,DataObject,(word_t)env);
	/*printf("vm_op_envNew\n");*/
}


void _VM_CALL vm_op_envGet_EnvSym(vm_t vm, long index) {
	vm_dyn_env_t env = vm->current_thread->program->env;
	if(!env) {
		printf("### program->env should be set ! ###\n");
		env = vm->env;
	}
	/*printf("vm_op_envGet %lu:%s\n",index,(const char*)env->symbols.by_index.data[index]);*/
	index<<=1;
	vm_push_data(vm,env->data.data[index],env->data.data[index+1]);
}



void _VM_CALL vm_op_envAdd(vm_t vm, word_t unused) {
	vm_data_t dk = _vm_pop(vm);
	vm_data_t dc = _vm_pop(vm);
	vm_dyn_env_t env = vm->current_thread->program->env;
	word_t index;
	word_t data=0;
	if(!env) {
		printf("### program->env should be set ! ###\n");
		env = vm->env;
	}

	/* slow but safe */
	index = text_seg_text_to_index(&env->symbols,text_seg_find_by_text(&env->symbols,(const char*)dk->data));

	/*printf("vm_op_envAdd %lu:%s\n",index,(const char*)env->symbols.by_index.data[index]);*/

	if(dc->type==DataObject) {
		data = (word_t) vm_obj_clone(vm,PTR_TO_OBJ(dc->data));
		vm_obj_ref(vm,(void*)data);
	} else {
		data = dc->data;
	}
	index<<=1;
	dynarray_set(&env->data,index,dc->type);
	dynarray_set(&env->data,index+1,data);
}

void _VM_CALL vm_op_envSet_EnvSym(vm_t vm, long index) {
	vm_dyn_env_t env = vm->current_thread->program->env;
	vm_data_t dc = _vm_pop(vm);
	vm_data_t env_dc;
	if(!env) {
		printf("### program->env should be set ! ###\n");
		env = vm->env;
	}
	/*printf("vm_op_envSet %lu:%s\n",index,(const char*)env->symbols.by_index.data[index]);*/
	env_dc = &((vm_data_t)env->data.data)[index];
	if(env_dc->type==DataObject) {
		vm_obj_deref(vm,PTR_TO_OBJ(env_dc->data));
	}
	env_dc->type = dc->type;
	env_dc->data = dc->data;
	/*index<<=1;*/
	/**/
	/*if(dc->type==DataObject) {*/
		/*vm_obj_ref(vm,PTR_TO_OBJ(dc));*/
	/*}*/
	/*dynarray_set(&env->data,index,dc->type);*/
	/*dynarray_set(&env->data,index+1,dc->data);*/
}




void _VM_CALL vm_op_envLookup(vm_t vm, long index) {
	vm_data_t key = _vm_pop(vm);
	vm_dyn_env_t env = vm->current_thread->program->env;
	assert(key->type==DataString||key->type==DataObject);	/* let string objects in */
	index = text_seg_text_to_index(&env->symbols,(const char*)key->data);
	if(!env) {
		printf("### program->env should be set ! ###\n");
		env = vm->env;
	}
	vm_push_data(vm,DataInt,index);
}



void _VM_CALL vm_op_stackNew(vm_t vm, word_t unused) {
	vm_push_data(vm,DataObject,(word_t)vm_stack_new());
}


void _VM_CALL vm_op_stackPush(vm_t vm, word_t unused) {
	vm_data_t d = _vm_pop(vm);
	vm_data_t sk = _vm_pop(vm);
	assert(sk->type==DataObject);
	gpush((generic_stack_t)sk->data,d);
}


void _VM_CALL vm_op_stackPop_Int(vm_t vm, word_t counter) {
	vm_data_t sk = _vm_pop(vm);
	assert(sk->type==DataObject);
	while(counter>0) {
		(void)_gpop((generic_stack_t)sk->data);
		counter-=1;
	}
	/*vm_push_data(vm, d->type, d->data);*/
}


void _VM_CALL vm_op_stackPeek_Int(vm_t vm, long ofs) {
	vm_data_t sk = _vm_pop(vm);
	vm_data_t d;
	assert(sk->type==DataObject);
	d = (vm_data_t) _gpeek((generic_stack_t)sk->data,-ofs);
	vm_push_data(vm, d->type, d->data);
}


void _VM_CALL vm_op_stackPop(vm_t vm, word_t counter) {
	vm_op_stackPop_Int(vm,1);
}


void _VM_CALL vm_op_stackPeek(vm_t vm, word_t unused) {
	vm_op_stackPeek_Int(vm,0);
}


void _VM_CALL vm_op_stackSize(vm_t vm, word_t unused) {
	vm_data_t sk = _vm_pop(vm);
	assert(sk->type==DataObject);
	vm_push_data(vm,DataInt,gstack_size((generic_stack_t)sk->data));
}


