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
#include "vm_assert.h"
/*! \addtogroup objects
 * @{
 */


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

/*! \brief assertions */
/*@{*/
#define _is_obj_obj(_x) (((_x)&&(((_x)->magic&VM_OBJ_MASK) == VM_OBJ_MAGIC)) || (vm_printerrf("Data at %p is not a managed object (magic = %8.8lX)\n",(_x),(_x)?(_x)->magic:0) && 0))
#define _is_a_obj(_x,_t) (_is_obj(_x) && (_obj_type(_x)==(_t)))

#define _is_obj_ptr(_x) ((_x)&&((PTR_TO_OBJ(_x)->magic&VM_OBJ_MASK) == VM_OBJ_MAGIC))
#define _is_a_ptr(_x,_t) (_is_obj_ptr(_x) && (_obj_type(PTR_TO_OBJ(_x))==(_t)))
/*@}*/

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
	/*vm_printf("obj new, %u+%lu bytes long at %p ; magic is %8.8lX\n",VM_OBJ_OFS,struc_size,o,o->magic);*/
	/*if(o->magic==0x6106010A) {*/
		/*vm_printf("user obj new, %u+%lu bytes long at %p ; _free=%p\n",VM_OBJ_OFS,struc_size,o, o->_free);*/
	/*}*/
	return (void*)(((char*)o)+VM_OBJ_OFS);
}

/*! \brief free a managed object.
 * \note you should let the VM do its job and never call this routine
 */
static inline void vm_obj_free_obj(vm_t vm, vm_obj_t o) {
	if(!o||!_is_obj_obj(o)) {
		vm_printerrf("[VM:ERR] trying to free something not a managed object (%p).\n",o);
		return;
	}
	/*if(o->magic==0x6106010A) {*/
		/*vm_printf("user obj free %p (%li refs), magic=%8.8lX, _free=%p\n",o,o->ref_count, *(((word_t*)o)+4), o->_free);*/
	/*} else {*/
		/*vm_printf("obj free %p (%li refs), magic=%8.8lX\n",o,o->ref_count, o->magic);*/
	/*}*/
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
	/*vm_printf("obj ref %p => %li\n",o,o->ref_count);*/
	if(o->ref_count==1) {
		vm_uncollect(vm, o);
	}
}


/*! \brief decrease reference count for \c ptr and collect object if necessary
 */
static inline void vm_obj_deref_ptr(vm_t vm, void* ptr) {
	vm_obj_t o = PTR_TO_OBJ(ptr);
	assert(ptr);
	assert(o);
	assert(_is_obj_obj(o));
	/*assert(o->ref_count>0);*/
	if(o->ref_count>0) {
		o->ref_count-=1;
		/*vm_printf("obj deref %p => %li\n",o,o->ref_count);*/
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

/*! \brief create a new set (unordered associative container with unique keys)
 */
vm_set_t vm_set_new(vm_t vm, vm_data_t compare, vm_data_t hash);


/*! \brief create a new stack
 */
generic_stack_t vm_stack_new();

/*! \brief create a new function object
 */
vm_dyn_func_t vm_dyn_fun_new();

void vm_set_add(vm_data_t key, vm_data_t value, vm_set_t s);
void vm_set_del(vm_set_t s, vm_data_t key);
int vm_set_has(vm_set_t s, vm_data_t key);
void _set_df_exec(vm_data_t k, vm_data_t v, vm_data_t fun);
void vm_set_foreach(vm_set_t s, void (*f) (vm_data_t, vm_data_t, void*), void* state);
vm_set_t vm_set_clone(vm_t vm, vm_set_t s);
vm_set_t vm_set_union(vm_set_t s1, vm_set_t s2);
vm_set_t vm_set_difference(vm_set_t s1, vm_set_t s2);
vm_set_t vm_set_intersection(vm_set_t s1, vm_set_t s2);
void vm_set_union_i(vm_set_t s1, vm_set_t s2);
void vm_set_difference_i(vm_set_t s1, vm_set_t s2);
void vm_set_intersection_i(vm_set_t s1, vm_set_t s2);



/*! \brief create a new virtual object instance
 */
vobj_ref_t vobj_new();
vobj_ref_t vobj_add_class(vm_t vm, vobj_ref_t o, vobj_class_t cls);
void vobj_set_member(vobj_ref_t o, word_t index, vm_data_t d);
vm_data_t vobj_get_member(vobj_ref_t o, word_t index);
#define VOBJ_OWN 0x1
#define VOBJ_COPYONWRITE 0x100

extern vobj_ref_t ObjNil;
extern vobj_class_t ClassNil;

/*@}*/


/*! virtual object class handling */
/*@{*/
vobj_class_t vclass_new();
const char* vclass_get_name(vobj_class_t cls);
void vclass_set_name(vobj_class_t cls, const char* name);
void vclass_set_override(vobj_class_t cls, opcode_stub_t stub, vm_data_t ovl);
void vclass_define_cast_to(vobj_class_t cls, vm_data_type_t totype, vm_dyn_func_t cast);
void vclass_define_cast_from(vobj_class_t cls, vm_data_type_t totype, vm_dyn_func_t cast);
/*@}*/


#endif

