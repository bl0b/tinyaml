#include "vm.h"
#include "_impl.h"
#include "object.h"
#include "text_seg.h"

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

