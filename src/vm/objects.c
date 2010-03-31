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

#include "vm.h"
#include "_impl.h"
#include "object.h"
#include "text_seg.h"
#include "thread.h"

#include <string.h>


void* vm_string_dup(vm_t vm, void*src) {
	return (void*)vm_string_new((const char*)src);
}

char* vm_string_new(const char*src) {
	char* str = (char*)vm_obj_new(strlen(src)+1, NULL, vm_string_dup, DataObjStr);
	/*vm_printf("new String\n");*/
	strcpy(str,src?src:"");
	return str;
}

char* vm_string_new_buf(word_t sz) {
	return (char*)vm_obj_new(sz+1, NULL, vm_string_dup, DataObjStr);
}


text_seg_t vm_symtab_new();

text_seg_t vm_symtab_clone(vm_t vm, text_seg_t seg) {
	text_seg_t ret = vm_symtab_new();
	text_seg_copy(ret, seg);
	return ret;
}

void symtab_deinit(vm_t vm, text_seg_t seg);

text_seg_t vm_symtab_new() {
	text_seg_t handle = (text_seg_t)
		vm_obj_new(sizeof(struct _text_seg_t),
			(void (*)(vm_t,void*))	symtab_deinit,
			(void*(*)(vm_t,void*))	vm_symtab_clone,
			DataObjSymTab);
	/*vm_printf("new SymTab\n");*/
	text_seg_init(handle);
	return handle;
}

mutex_t mutex_clone(mutex_t in) {
	return in;
}

mutex_t vm_mutex_new() {
	mutex_t handle = (mutex_t) vm_obj_new(sizeof(struct _mutex_t),
			(void (*)(vm_t,void*)) mutex_deinit,
			(void*(*)(vm_t,void*)) mutex_clone,
			DataObjMutex);
	/*vm_printf("new Mutex\n");*/
	mutex_init(handle);
	/*vm_printf("new mutex => %p\n",handle);*/
	return handle;
}


thread_t thread_clone(thread_t in) {
	/* FIXME : shouldn't be such a semi-singleton */
	return in;
}


thread_t vm_thread_new(vm_t vm,word_t prio, program_t p, word_t ip) {
	thread_t ret = (thread_t)vm_obj_new(sizeof(struct _thread_t),
			(void (*)(vm_t,void*)) thread_deinit,
			(void*(*)(vm_t,void*)) thread_clone,
			DataObjThread);
	/*vm_printf("new Thread\n");*/
	thread_init(ret,prio,p,ip);
	vm_obj_ref_ptr(vm,ret);
	return ret;
}




void vm_da_deinit(vm_t vm, dynarray_t da) {
	word_t i;
	/*vm_printf("deinit Array\n");*/
	/* FIXME : too heavy to be done at once */
	for(i=0;i<da->size;i+=2) {
		if((vm_data_type_t)da->data[i] &DataManagedObjectFlag) {
			/*vm_printf(" Array : dereffing object %p\n",(void*)da->data[i+1]);*/
			vm_obj_deref_ptr(vm,(void*)da->data[i+1]);
		}
	}
	dynarray_deinit(da,NULL);
}


void vm_da_copy(vm_t vm, dynarray_t dest, dynarray_t src) {
	word_t i;
	dynarray_reserve(dest,dynarray_size(src));
	/* FIXME : too heavy to be done at once */
	for(i=0;i<src->size;i+=2) {
		dest->data[i]=src->data[i];
		if((vm_data_type_t)src->data[i] &DataManagedObjectFlag) {
			vm_obj_ref_ptr(vm,(void*)src->data[i+1]);
			dest->data[i+1]=(word_t)vm_obj_clone_obj(vm,PTR_TO_OBJ(src->data[i+1]));
		} else {
			dest->data[i+1]=src->data[i+1];
		}
	}
}

void vm_da_clone(vm_t vm, dynarray_t da) {
	dynarray_t ret = vm_array_new();
	vm_da_copy(vm, ret, da);
}


dynarray_t vm_array_new() {
	dynarray_t ret = (dynarray_t)vm_obj_new(sizeof(struct _dynarray_t),
			(void (*)(vm_t,void*)) vm_da_deinit,
			(void*(*)(vm_t,void*)) vm_da_clone,
			DataObjArray);
	/*vm_printf("new Array\n");*/
	dynarray_init(ret);
	return ret;
}



void vm_env_deinit(vm_t vm, vm_dyn_env_t env) {
	/*vm_printf("deinit Env\n");*/
	vm_da_deinit(vm,&env->data);
	symtab_deinit(vm,&env->symbols);
}

vm_dyn_env_t vm_env_new();

vm_dyn_env_t vm_env_clone(vm_t vm, vm_dyn_env_t env) {
	vm_dyn_env_t ret = vm_env_new();
	text_seg_copy(&ret->symbols, &env->symbols);
	vm_da_copy(vm, &ret->data, &env->data);
	return ret;
}

vm_dyn_env_t vm_env_new() {
	vm_dyn_env_t ret = (vm_dyn_env_t) vm_obj_new(sizeof(struct _vm_dyn_env_t),
			(void (*)(vm_t,void*)) vm_env_deinit,
			(void*(*)(vm_t,void*)) vm_env_clone,
			DataObjEnv);
	/*vm_printf("new Env\n");*/
	dynarray_init(&ret->data);
	dynarray_set(&ret->data,0,0);
	text_seg_init(&ret->symbols);
	return ret;
}

void deref_stack(vm_t vm, generic_stack_t gs);

void vm_stack_deinit(vm_t vm, generic_stack_t src) {
	/* TODO */
	deref_stack(vm,src);
	gstack_deinit(src,NULL);
}

generic_stack_t vm_stack_clone(vm_t vm, generic_stack_t src) {
	/* TODO */
	return src;
}

generic_stack_t vm_stack_new() {
	generic_stack_t ret = (generic_stack_t) vm_obj_new(sizeof(struct _generic_stack_t),
			(void (*)(vm_t,void*)) vm_stack_deinit,
			(void*(*)(vm_t,void*)) vm_stack_clone,
			DataObjStack);
	/*vm_printf("new Stack\n");*/
	/*gstack_init(ret,sizeof(struct _data_stack_entry_t));*/
	gstack_init(ret, sizeof(word_t)*2);
	return ret;
}


void vm_df_deinit(vm_t vm, vm_dyn_func_t src) {
	/*vm_printf("deinit DynFun %p\n",src);*/
	/* TODO */
	if(src->closure) {
		/*vm_da_deinit(vm,src->closure);*/
		/*free(PTR_TO_OBJ(src->closure));*/
		/*vm_printf("DynFun has closure, dereff'ing\n");*/
		vm_obj_deref_ptr(vm,src->closure);
		/*vm_printf("DynFun closure is reff'ed %ld times\n",vm_obj_refcount_ptr(src->closure));*/
	}
}

vm_dyn_func_t vm_df_clone(vm_t vm, vm_dyn_func_t src) {
	/* TODO */
	return src;
}

vm_dyn_func_t vm_dyn_fun_new() {
	vm_dyn_func_t ret = (vm_dyn_func_t) vm_obj_new(sizeof(struct _vm_dyn_func_t),
			(void (*)(vm_t,void*)) vm_df_deinit,
			(void*(*)(vm_t,void*)) vm_df_clone,
			DataObjFun);
	/*vm_printf("new DynFun\n");*/
	memset(ret,0,sizeof(struct _vm_dyn_func_t));
	return ret;
}




vobj_ref_t ObjNil = NULL;
vobj_class_t ClassNil = NULL;




void vm_vor_deinit(vm_t vm, vobj_ref_t obj) {
	vm_obj_deref_ptr(vm, obj->members);
}

vobj_ref_t vm_vor_clone(vm_t vm, vobj_ref_t obj) {
	vobj_ref_t ret = (vobj_ref_t) vm_obj_new(sizeof(struct _vobj_ref_t),
			(void (*)(vm_t, void*)) vm_vor_deinit,
			(void*(*)(vm_t, void*)) vm_vor_clone,
			DataObjVObj);
	ret->cls = obj->cls;
	ret->members = obj->members;
	ret->own=VOBJ_COPYONWRITE;
	ret->offset=0;
	vm_obj_ref_ptr(vm, ret->members);
	return ret;
}

vobj_ref_t vobj_new() {
	vobj_ref_t ret = (vobj_ref_t) vm_obj_new(sizeof(struct _vobj_ref_t),
			(void (*)(vm_t, void*)) vm_vor_deinit,
			(void*(*)(vm_t, void*)) vm_vor_clone,
			DataObjVObj);
	ret->cls = NULL;
	ret->own=VOBJ_OWN;
	ret->offset=0;
	ret->members = vm_array_new();
	vm_obj_ref_ptr(_glob_vm, ret->members);
	return ret;
}

void vobj_set_class(vobj_ref_t o, vobj_class_t cls) {
	if(o->cls) {
		vm_obj_deref_ptr(_glob_vm, o->cls);
	}
	o->cls = cls;
	vm_obj_ref_ptr(_glob_vm, cls);
}

vobj_class_t vobj_get_class(vobj_ref_t o) {
	return o->cls?o->cls:ClassNil;
}

void vobj_set_member(vobj_ref_t o, word_t index, vm_data_t d) {
	index<<=1;
	dynarray_set(o->members, index, (word_t) d->type);
	dynarray_set(o->members, index+1, (word_t) d->data);
}

vm_data_t vobj_get_member(vobj_ref_t o, word_t index) {
	return ((vm_data_t)o->members->data)+index;
}

void htab_free_oo(htab_entry_t e) {
	vm_data_t d = (vm_data_t) e->e;
	if(d->type&DataManagedObjectFlag) {
		vm_obj_deref_ptr(_glob_vm, (void*) d->data);
	}
	free(d);
}

void vm_vcls_deinit(vm_t vm, vobj_class_t cls) {
	int i;
	for(i=0;i<DataTypeMax;i+=1) {
		if(cls->_cast_to[i]) {
			vm_obj_deref_ptr(vm, cls->_cast_to[i]);
		}
		if(cls->_cast_from[i]) {
			vm_obj_deref_ptr(vm, cls->_cast_from[i]);
		}
	}
	clean_hashtab(&cls->_overloads, htab_free_oo);
	if(cls->_name) {
		free(cls->_name);
	}
}

vobj_class_t vm_vcls_clone(vm_t vm, vobj_class_t cls) {
	return cls;
}

vobj_class_t vclass_new() {
	vobj_class_t ret = (vobj_class_t) vm_obj_new(sizeof(struct _vobj_class_t),
			(void (*)(vm_t, void*)) vm_vcls_deinit,
			(void*(*)(vm_t, void*)) vm_vcls_clone,
			DataObjVObj);
	ret->ref.cls = NULL;
	ret->ref.own=VOBJ_OWN;
	ret->ref.offset=0;
	ret->_name=strdup("(unset)");
	init_hashtab(&ret->_overloads, (hash_func) hash_ptr, (compare_func) cmp_ptr);
	memset(ret->_cast_from, 0, sizeof(vm_dyn_func_t)*DataTypeMax);
	memset(ret->_cast_to, 0, sizeof(vm_dyn_func_t)*DataTypeMax);
	ret->ref.members = vm_array_new();
	vm_obj_ref_ptr(_glob_vm, ret->ref.members);
	return ret;
}

const char* vclass_get_name(vobj_class_t cls) {
	return cls->_name?cls->_name:"(unset)";
}

void vclass_set_name(vobj_class_t cls, const char* name) {
	if(cls->_name) {
		free(cls->_name);
	}
	cls->_name=strdup(name);
}

void vclass_set_overload(vobj_class_t cls, opcode_stub_t stub, vm_data_t ovl) {
	vm_data_t d = (vm_data_t) hash_find(&cls->_overloads, (hash_key) stub);
	if(!d) {
		d = (vm_data_t) malloc(sizeof(struct _data_stack_entry_t));
		d->type=0;
		hash_addelem(&cls->_overloads, (hash_key) stub, (hash_elem) d);
	}
	if(d->type&DataManagedObjectFlag) {
		vm_obj_deref_ptr(_glob_vm, (void*) d->data);
	}
	d->type=ovl->type;
	d->data=ovl->data;
	if(d->type&DataManagedObjectFlag) {
		vm_obj_ref_ptr(_glob_vm, (void*) d->data);
	}
}


void vclass_define_cast_to(vobj_class_t cls, vm_data_type_t totype, vm_dyn_func_t cast) {
	if(cls->_cast_to[totype]) {
		vm_obj_deref_ptr(_glob_vm, cls->_cast_to[totype]);
	}
	vm_obj_deref_ptr(_glob_vm, cls->_cast_to[totype]);
	cls->_cast_to[totype] = cast;
	vm_obj_ref_ptr(_glob_vm, cast);
}


void vclass_define_cast_from(vobj_class_t cls, vm_data_type_t totype, vm_dyn_func_t cast) {
	if(cls->_cast_from[totype]) {
		vm_obj_deref_ptr(_glob_vm, cls->_cast_from[totype]);
	}
	vm_obj_deref_ptr(_glob_vm, cls->_cast_from[totype]);
	cls->_cast_from[totype] = cast;
	vm_obj_ref_ptr(_glob_vm, cast);
}


