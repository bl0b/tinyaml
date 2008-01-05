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

#define VM_OBJ_MAGIC 0x61064223

#define ref(_o) (((vm_obj_t)o)->ref_count+=1)
#define deref(_o) (((vm_obj_t)o)->ref_count-=1)

#define is_reffed(_o) (((vm_obj_t)o)->ref_count>0)


#include <stdio.h>

static inline void* vm_obj_new(word_t struc_size, void(*_free)(vm_t,void*), void*(*_clone)(vm_t,void*)) {
	vm_obj_t o = (vm_obj_t) malloc(VM_OBJ_OFS+struc_size);
	/*printf("obj new, %u+%lu bytes long at %p\n",VM_OBJ_OFS,struc_size,o);*/
	o->ref_count=0;
	o->magic = VM_OBJ_MAGIC;
	o->_free=_free;
	o->_clone=_clone;
	return (void*)(((char*)o)+VM_OBJ_OFS);
}

static inline void vm_obj_free(vm_t vm, vm_obj_t o) {
	assert(o->magic==VM_OBJ_MAGIC);
	/*printf("obj free %p\n",o);*/
	if(o->_free) {
		o->_free(vm, (void*)(((char*)o)+VM_OBJ_OFS));
	}
	free(o);
}

static inline vm_obj_t vm_obj_clone(vm_t vm, vm_obj_t o) {
	assert(o->magic==VM_OBJ_MAGIC);
	
	if(o->_clone) {
		return o->_clone(vm, (void*)(((char*)o)+VM_OBJ_OFS));
	}
	return o;
}

static inline word_t vm_obj_refcount(void* ptr) {
	vm_obj_t o = (vm_obj_t)(((char*)ptr)-VM_OBJ_OFS);
	assert(o->magic==VM_OBJ_MAGIC);
	return o->ref_count;
}

static inline void vm_obj_ref(vm_t vm, void* ptr) {
	vm_obj_t o = (vm_obj_t)(((char*)ptr)-VM_OBJ_OFS);
	assert(o->magic==VM_OBJ_MAGIC);
	assert(o->ref_count>=0);
	o->ref_count+=1;
	/*printf("obj ref %p => %li\n",o,o->ref_count);*/
	if(o->ref_count==1) {
		vm_uncollect(vm, o);
	}
}

static inline void vm_obj_deref(vm_t vm, void* ptr) {
	vm_obj_t o = (vm_obj_t)(((char*)ptr)-VM_OBJ_OFS);
	assert(o->magic==VM_OBJ_MAGIC);
	assert(o->ref_count>0);
	o->ref_count-=1;
	/*printf("obj deref %p => %li\n",o,o->ref_count);*/
	if(o->ref_count==0) {
		vm_collect(vm, o);
	}
}

#define assert_ptr_is_obj(_x) assert(((vm_obj_t)(((char*)(_x))-VM_OBJ_OFS))->magic==VM_OBJ_MAGIC)

char* vm_string_new(const char*src);
text_seg_t vm_symtab_new();
mutex_t vm_mutex_new();

#endif

