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


text_seg_t vm_symtab_clone(vm_t vm, text_seg_t seg) {
	/* FIXME : shouldn't be such a semi-singleton */
	return seg;
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
	/*vm_obj_ref_ptr(vm,ret);*/
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


void vm_da_clone(vm_t vm, dynarray_t da) {
	dynarray_t ret = vm_array_new();
	word_t i;
	dynarray_reserve(ret,dynarray_size(da));
	/* FIXME : too heavy to be done at once */
	for(i=0;i<da->size;i+=2) {
		ret->data[i]=da->data[i];
		if((vm_data_type_t)da->data[i] &DataManagedObjectFlag) {
			vm_obj_ref_ptr(vm,(void*)da->data[i+1]);
			ret->data[i+1]=(word_t)vm_obj_clone_obj(vm,PTR_TO_OBJ(da->data[i+1]));
		} else {
			ret->data[i+1]=da->data[i+1];
		}
	}
	dynarray_deinit(da,NULL);
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

vm_dyn_env_t vm_env_clone(vm_t vm, vm_dyn_env_t env) {
	/* TODO */
	return env;
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


