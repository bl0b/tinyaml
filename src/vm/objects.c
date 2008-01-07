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
	char* str = (char*)vm_obj_new(strlen(src)+1, NULL, vm_string_dup);
	strcpy(str,src?src:"");
	return str;
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
			(void*(*)(vm_t,void*))	vm_symtab_clone);
	text_seg_init(handle);
	return handle;
}

mutex_t mutex_clone(mutex_t in) {
	return in;
}

mutex_t vm_mutex_new() {
	mutex_t handle = (mutex_t) vm_obj_new(sizeof(struct _mutex_t),
			(void (*)(vm_t,void*)) mutex_deinit,
			(void*(*)(vm_t,void*)) mutex_clone);
	mutex_init(handle);
	/*printf("new mutex => %p\n",handle);*/
	return handle;
}


thread_t thread_clone(thread_t in) {
	/* FIXME : shouldn't be such a semi-singleton */
	return in;
}


thread_t vm_thread_new(vm_t vm,word_t prio, program_t p, word_t ip) {
	thread_t ret = (thread_t)vm_obj_new(sizeof(struct _thread_t),
			(void (*)(vm_t,void*)) thread_deinit,
			(void*(*)(vm_t,void*)) thread_clone);
	thread_init(ret,prio,p,ip);
	vm_obj_ref(vm,ret);
	return ret;
}




void vm_da_deinit(vm_t vm, dynarray_t da) {
	word_t i;
	/* FIXME : too heavy to be done at once */
	for(i=0;i<da->size;i+=2) {
		if((vm_data_type_t)da->data[i] == DataObject) {
			vm_obj_deref(vm,(void*)da->data[i+1]);
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
		if((vm_data_type_t)da->data[i] == DataObject) {
			vm_obj_ref(vm,(void*)da->data[i+1]);
			ret->data[i+1]=(word_t)vm_obj_clone(vm,PTR_TO_OBJ(da->data[i+1]));
		} else {
			ret->data[i+1]=da->data[i+1];
		}
	}
	dynarray_deinit(da,NULL);
}


dynarray_t vm_array_new() {
	dynarray_t ret = (dynarray_t)vm_obj_new(sizeof(struct _dynarray_t),
			(void (*)(vm_t,void*)) vm_da_deinit,
			(void*(*)(vm_t,void*)) vm_da_clone);
	dynarray_init(ret);
	return ret;
}

