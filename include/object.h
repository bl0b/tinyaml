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

#ifndef _BML_OBJ_H_
#define _BML_OBJ_H_

#define VM_OBJ_OFS sizeof(struct _vm_obj_t)

#define PTR_TO_OBJ(_x) ((vm_obj_t)(((char*)(_x))-VM_OBJ_OFS))
#define OBJ_TO_PTR(_x) ((void*)(((char*)(_x))+VM_OBJ_OFS))

#define VM_OBJ_MAGIC	0x61060000
#define VM_OBJ_MASK	0xFFFF0000

#define _obj_type(_x) (((vm_obj_t)(_x))->magic^VM_OBJ_MAGIC)

#define _is_obj_obj(_x) (((_x)&&(((_x)->magic&VM_OBJ_MASK) == VM_OBJ_MAGIC)) || (fprintf(stderr,"Data at %p is not a managed object (magic = %8.8lX)\n",(_x),(_x)?(_x)->magic:0) && 0))
#define _is_a_obj(_x,_t) (_is_obj(_x) && (_obj_type(_x)==(_t)))

#define _is_obj_ptr(_x) ((_x)&&((PTR_TO_OBJ(_x)->magic&VM_OBJ_MASK) == VM_OBJ_MAGIC))
#define _is_a_ptr(_x,_t) (_is_obj_ptr(_x) && (_obj_type(PTR_TO_OBJ(_x))==(_t)))

/*
#define ref(_o) (((vm_obj_t)_o)->ref_count+=1)
#define deref(_o) (((vm_obj_t)_o)->ref_count-=1)

#define is_reffed(_o) (((vm_obj_t)_o)->ref_count>0)

#define is_obj(_x)

#define assert_ptr_is_obj(_x) assert(((vm_obj_t)(((char*)(_x))-VM_OBJ_OFS))->magic==VM_OBJ_MAGIC)
*/

#include <stdio.h>

static inline void* vm_obj_new(word_t struc_size, void(*_free)(vm_t,void*), void*(*_clone)(vm_t,void*), vm_data_type_t obj_type) {
	vm_obj_t o = (vm_obj_t) malloc(VM_OBJ_OFS+struc_size);
	assert((obj_type&DataManagedObjectFlag)&&obj_type<DataTypeMax);
	o->ref_count=0;
	o->magic = VM_OBJ_MAGIC | obj_type;
	o->_free=_free;
	o->_clone=_clone;
	/*printf("obj new, %u+%lu bytes long at %p ; magic is %8.8lX\n",VM_OBJ_OFS,struc_size,o,o->magic);*/
	return (void*)(((char*)o)+VM_OBJ_OFS);
}

static inline void vm_obj_free_obj(vm_t vm, vm_obj_t o) {
	if(!_is_obj_obj(o)) {
		fprintf(stderr,"[VM:ERR] trying to free something not a managed object (%p).\n",o);
		return;
	}
	/*assert(o->magic==VM_OBJ_MAGIC);*/
	/*printf("obj free %p (%li refs)\n",o,o->ref_count);*/
	if(o->_free) {
		o->_free(vm, (void*)(((char*)o)+VM_OBJ_OFS));
	}
	free(o);
}

static inline vm_obj_t vm_obj_clone_obj(vm_t vm, vm_obj_t o) {
	assert(_is_obj_obj(o));
	
	if(o->_clone) {
		return o->_clone(vm, (void*)(((char*)o)+VM_OBJ_OFS));
	}
	return o;
}

static inline word_t vm_obj_refcount_ptr(void* ptr) {
	vm_obj_t o = PTR_TO_OBJ(ptr);
	assert(_is_obj_obj(o));
	return o->ref_count;
}

static inline void vm_obj_ref_ptr(vm_t vm, void* ptr) {
	vm_obj_t o = PTR_TO_OBJ(ptr);
	assert(_is_obj_obj(o)/*||(1/0)*/);
	assert(o->ref_count>=0);
	o->ref_count+=1;
	/*printf("obj ref %p => %li\n",o,o->ref_count);*/
	if(o->ref_count==1) {
		vm_uncollect(vm, o);
	}
}

static inline void vm_obj_deref_ptr(vm_t vm, void* ptr) {
	vm_obj_t o = PTR_TO_OBJ(ptr);
	assert(_is_obj_obj(o));
	/*assert(o->ref_count>0);*/
	if(o->ref_count>0) {
		o->ref_count-=1;
		/*printf("obj deref %p => %li\n",o,o->ref_count);*/
		if(o->ref_count==0) {
			vm_collect(vm, o);
		}
	} else {
		vm_collect(vm,o);
	}
}


char* vm_string_new(const char*src);
char* vm_string_new_buf(word_t sz);
text_seg_t vm_symtab_new();
mutex_t vm_mutex_new();
thread_t vm_thread_new(vm_t vm,word_t prio, program_t p, word_t ip);
dynarray_t vm_array_new();
vm_dyn_env_t vm_env_new();
generic_stack_t vm_stack_new();
vm_dyn_func_t vm_dyn_fun_new();

#endif

