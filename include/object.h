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

#include <stdio.h>
/*! \addtogroup objects
 * \brief This page describes what a managed object is, how the VM manages objects, and the corresponding API.
 *
 * TODO
 * \section obj_howto How to write user managed objects
 * TODO
 * @{*/


/*! \brief size of the object header */
#define VM_OBJ_OFS sizeof(struct _vm_obj_t)

/*! \brief translate from a managed buffer to its object header */
#define PTR_TO_OBJ(_x) ((vm_obj_t)(((char*)(_x))-VM_OBJ_OFS))
/*! \brief translate from an object header to its managed buffer */
#define OBJ_TO_PTR(_x) ((void*)(((char*)(_x))+VM_OBJ_OFS))

/*! \brief magic value for managed objects : bit pattern */
#define VM_OBJ_MAGIC	0x61060000
/*! \brief magic value for managed objects : bit mask */
#define VM_OBJ_MASK	0xFFFF0000

/*! \brief retrieve the type of managed object (DataObj* constant) */
#define _obj_type(_x) (((vm_obj_t)(_x))->magic^VM_OBJ_MAGIC)

/*\@{*/
/*! \brief assertions */
#define _is_obj_obj(_x) (((_x)&&(((_x)->magic&VM_OBJ_MASK) == VM_OBJ_MAGIC)) || (fprintf(stderr,"Data at %p is not a managed object (magic = %8.8lX)\n",(_x),(_x)?(_x)->magic:0) && 0))
#define _is_a_obj(_x,_t) (_is_obj(_x) && (_obj_type(_x)==(_t)))

#define _is_obj_ptr(_x) ((_x)&&((PTR_TO_OBJ(_x)->magic&VM_OBJ_MASK) == VM_OBJ_MAGIC))
#define _is_a_ptr(_x,_t) (_is_obj_ptr(_x) && (_obj_type(PTR_TO_OBJ(_x))==(_t)))
/*\@}*/

/*! \brief allocate a new managed buffer.
 * \param struc_size the size of the buffer to allocate
 * \param _free a routine that will be called just before freeing the buffer
 * \param _clone a routine that will be called to clone an object (used when initializing a closure for instance)
 * \param obj_type any DataObj* constant (see DataObjUser).
 */
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

/*! \brief free a managed object.
 * \note you should let the VM do its job and never call this routine
 */
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

/*! \brief clone an object
 */
static inline vm_obj_t vm_obj_clone_obj(vm_t vm, vm_obj_t o) {
	assert(_is_obj_obj(o));
	
	if(o->_clone) {
		return o->_clone(vm, (void*)(((char*)o)+VM_OBJ_OFS));
	}
	return o;
}

/*! \brief get the reference count for object \c ptr
 */
static inline word_t vm_obj_refcount_ptr(void* ptr) {
	vm_obj_t o = PTR_TO_OBJ(ptr);
	assert(_is_obj_obj(o));
	return o->ref_count;
}

/*! \brief increase reference count for \c ptr
 */
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


/*! \brief decrease reference count for \c ptr and collect object if necessary
 */
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

/*! \brief create a new managed string containing a copy of \c src
 */
char* vm_string_new(const char*src);

/*! \brief create a new managed string of length \c sz
 */
char* vm_string_new_buf(word_t sz);

/*! \brief create a new symbol table ( [symbol] => index not null )
 */
text_seg_t vm_symtab_new();

/*! \brief create a new mutex
 */
mutex_t vm_mutex_new();

/*! \brief create a new thread
 */
thread_t vm_thread_new(vm_t vm,word_t prio, program_t p, word_t ip);

/*! \brief create a new dynamic array
 */
dynarray_t vm_array_new();

/*! \brief create a new map ( symbol table + array)
 */
vm_dyn_env_t vm_env_new();

/*! \brief create a new stack
 */
generic_stack_t vm_stack_new();

/*! \brief create a new function object
 */
vm_dyn_func_t vm_dyn_fun_new();

/*@}*/

#endif

