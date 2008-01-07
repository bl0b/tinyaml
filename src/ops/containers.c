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

/**********************************************************
 * arrayNew
 * creates a managed array
 * nil -> Object
 */
void _VM_CALL vm_op_arrayNew(vm_t vm, word_t unused) {
	dynarray_t da = vm_array_new();
	printf("new array %p\n",da);
	vm_push_data(vm,DataObject,(word_t)da);
}

/**********************************************************
 * arrayResv:Int
 * reserves [arg] items in array
 * Object -> nil
 */
void _VM_CALL vm_op_arrayResv_Int(vm_t vm, word_t sz) {
	vm_data_t d = _vm_pop(vm);
	dynarray_t da = (dynarray_t) d->data;
	assert(d->type==DataObject);
	dynarray_reserve(da,sz<<1);
	printf("reserved %lu words for array %p\n",sz,da);
}

/**********************************************************
 * arrayResv
 * reserves some items in array
 * Object X Int -> nil
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
	word_t* data = da->data+ofs;
	assert(d->type==DataObject);
	if(da->size>ofs+1) {
		vm_push_data(vm,*data,*(data+1));
		printf("get %lu:%8.8lx from array %p\n",*data,*(data+1),da);
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
 * Object X (something) -> nil
 */
void _VM_CALL vm_op_arraySet_Int(vm_t vm, word_t index) {
	vm_data_t data = _vm_pop(vm);
	vm_data_t d = _vm_pop(vm);
	dynarray_t da = (dynarray_t) d->data;
	word_t ofs = index<<1;
	vm_data_t da_data = (vm_data_t)(da->data+ofs);
	assert(d->type==DataObject);
	if(da->size>ofs+1) {
		if(da_data->type==DataObject) { vm_obj_deref(vm,(void*)da_data->data); }
		da_data->type = data->type;
		da_data->data = data->data;
		printf("set %u:%8.8lx in array %p\n",da_data->type,da_data->data,da);
	} else {
		dynarray_reserve(da,ofs+2);
		da_data = (vm_data_t)(da->data+ofs);
		da_data->type = data->type;
		da_data->data = data->data;
		da->size = ofs+2;
	}
	if(data->type==DataObject) { vm_obj_ref(vm,(void*)data->data); }
}

/**********************************************************
 * arraySet
 * set the item at some index in array to (something)
 * Object X (something) X Int -> nil
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



