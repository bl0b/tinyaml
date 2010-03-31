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
#include "opcode_dict.h"
#include "object.h"
#include "generic_stack.h"


/*! \addtogroup vcop_mem
 * @{
 */

void _VM_CALL vm_op_getmem_Int(vm_t vm, long n) {
	thread_t t=vm->current_thread;
	vm_data_t var;
	if(n<0) {
		vm_data_t local = _gpeek(&t->locals_stack,1+n);	/* -1 becomes 0 */
		vm_push_data(vm,local->type,local->data);
	} else {
		var = (vm_data_t ) (t->program->data.data+(n<<1));
		vm_push_data(vm,var->type,var->data);
	}
}


void _VM_CALL vm_op_setmem_Int(vm_t vm, long n) {
	thread_t t=vm->current_thread;
	vm_data_t top = _vm_pop(vm);
	vm_data_t var=NULL;
	if(n<0) {
		assert(t->locals_stack.sp>=-1-n);
		var = gpeek( vm_data_t , &t->locals_stack, 1+n );
	} else {
		n<<=1;
		/*vm_printf("setmem at %lu of %lu/%lu\n",n,t->program->data.size,t->program->data.reserved);*/
		assert(t->program->data.reserved>n);
		if(n>=t->program->data.size) {
			t->program->data.size = n+2;
		}
		var = (vm_data_t ) (t->program->data.data+n);
	}
	if(var->type&DataManagedObjectFlag) {
		vm_obj_deref_ptr(vm,(void*)var->data);
	}
	var->type=top->type;
	var->data=top->data;
	if(top->type&DataManagedObjectFlag) {
		vm_obj_ref_ptr(vm,(void*)top->data);
	}
}


void _VM_CALL vm_op_typeof(vm_t vm, word_t unused) {
	vm_data_t d = _vm_pop(vm);
	vm_push_data(vm, DataInt, (word_t)d->type);
}

void _VM_CALL vm_op_setmem(vm_t vm, long n) {
	vm_data_t top = _vm_pop(vm);
	if(top->type!=DataInt) {
		return;
	}
	vm_op_setmem_Int(vm, (long)top->data);
}


void _VM_CALL vm_op_getmem(vm_t vm, long n) {
	vm_data_t top = _vm_pop(vm);
	if(top->type!=DataInt) {
		return;
	}
	vm_op_getmem_Int(vm, (long)top->data);
}

void _VM_CALL vm_op_getClosure_Int(vm_t vm, word_t index) {
	thread_t t=vm->current_thread;
	dynarray_t da = *(dynarray_t*)_gpeek(&t->closures_stack,0);
	index<<=1;
	vm_push_data(vm,da->data[index],da->data[index+1]);
	/*vm_printf("getClosure(%li) : %li,%8.8lX\n",index>>1,da->data[index],da->data[index+1]);*/
}

void _VM_CALL vm_op_getClosure(vm_t vm, word_t unused) {
	vm_data_t top = _vm_pop(vm);
	if(top->type!=DataInt) {
		return;
	}
	vm_op_getClosure_Int(vm, (long)top->data);
}

void _VM_CALL vm_op_setClosure_Int(vm_t vm, word_t index) {
	vm_data_t d = _vm_pop(vm);
	thread_t t=vm->current_thread;
	dynarray_t da = *(dynarray_t*)_gpeek(&t->closures_stack,0);
	index<<=1;
	if(da->data[index]&DataManagedObjectFlag) {
		vm_obj_deref_ptr(vm,(void*)da->data[index+1]);
	}
	if(d->type&DataManagedObjectFlag) {
		vm_obj_ref_ptr(vm,(void*)d->data);
	}
	da->data[index] = d->type;
	da->data[index+1] = d->data;
}

void _VM_CALL vm_op_setClosure(vm_t vm, word_t unused) {
	vm_data_t top = vm_pop_int(vm);
	vm_op_setClosure_Int(vm, (long)top->data);
}

/*@}*/


/*! \addtogroup vcop_data
 * @{
 */
/*void _VM_CALL vm_op_toC(vm_t vm, word_t unused) {*/
	/*_IFC conv;*/
	/*vm_data_t d = _vm_pop(vm);*/
	/*switch(d->type) {*/
	/*case DataInt:*/
	/*case DataChar:*/
		/*vm_push_data(vm,DataChar,d->data);*/
		/*break;*/
	/*case DataFloat:*/
		/*conv.i=d->data;*/
		/*vm_push_data(vm,DataChar,f2i(conv.f));*/
		/*break;*/
	/*case DataString:*/
		/*vm_push_data(vm,DataChar,(unsigned char*)(d->data)[0]);*/
		/*break;*/
	/*default:*/
		/*vm_printerrf("[VM:WRN] can't convert to char.\n");*/
		/*vm_push_data(vm,DataInt,0);*/
	/*};*/
/*}*/


void _VM_CALL vm_op_toI(vm_t vm, word_t unused) {
	_IFC conv;
	vm_data_t d = _vm_pop(vm);
	switch(d->type) {
	case DataInt:
	case DataChar:
		vm_push_data(vm,DataInt,d->data);
		break;
	case DataFloat:
		conv.i=d->data;
		vm_push_data(vm,DataInt,f2i(conv.f));
		break;
	case DataString:
		/*vm_printf("convert \"%s\" to long\n",(const char*)d->data);*/
		vm_push_data(vm,DataInt,atoi((const char*)d->data));
		break;
	default:
		vm_printerrf("[VM:WRN] can't convert to long.\n");
		vm_push_data(vm,DataInt,0);
	};
}


void _VM_CALL vm_op_toF(vm_t vm, word_t unused) {
	_IFC conv;
	vm_data_t d = _vm_pop(vm);
	switch(d->type) {
	case DataInt:
		vm_push_data(vm,DataFloat,i2f(d->data));
		break;
	case DataFloat:
		vm_push_data(vm,DataFloat,d->data);
		break;
	case DataString:
		conv.f = atof((const char*)d->data);
		vm_push_data(vm,DataFloat,conv.i);
		break;
	default:
		vm_printerrf("[VM:WRN] can't convert to tinyaml_float_t.\n");
		vm_push_data(vm,DataFloat,0);
	};
}


void _VM_CALL vm_op_toS(vm_t vm, word_t unused) {
	static char buf[40];
	vm_data_t d = _vm_pop(vm);
	_IFC conv;
	char* str;
	switch(d->type) {
	case DataNone:
		strcpy(buf, "nil");
		break;
	case DataInt:
		sprintf(buf,"%li",(long)d->data);
		str=vm_string_new(buf);
		vm_push_data(vm,DataObjStr,(word_t)str);
		break;
	case DataFloat:
		conv.i=d->data;
		sprintf(buf,"%f",conv.f);
		str=vm_string_new(buf);
		vm_push_data(vm,DataObjStr,(word_t)str);
		break;
	case DataObjStr:
		vm_push_data(vm,DataObjStr,d->data);
		break;
	case DataString:
		vm_push_data(vm,DataString,d->data);
		break;
	default:;
		sprintf(buf,"[Object %X %p]",d->type,(void*)d->data);
		str=vm_string_new(buf);
		vm_push_data(vm,DataObjStr,(word_t)str);
//	default:
//		vm_push_data(vm,DataString,0);
	};
}
/*@}*/


/*! \addtogroup vcop_str
 * @{
 */
void _VM_CALL vm_op_strcmp_String(vm_t vm, const char* s2) {
	vm_data_t s1 = _vm_pop(vm);
	assert(s1->type==DataString||s1->type==DataObjStr);
	vm_push_data(vm,DataInt,strcmp((const char*)s1->data, (const char*)s2));
}


void _VM_CALL vm_op_strcat_String(vm_t vm, const char* s2) {
	vm_data_t s1 = _vm_pop(vm);
	char* ret;
	assert(s1->type==DataString||s1->type==DataObjStr);
	ret = vm_string_new_buf(strlen(s2)+strlen((const char*)s1->data));
	sprintf(ret,"%s%s",(const char*)s1->data,s2);
	vm_push_data(vm,DataObjStr,(word_t)ret);
}


void _VM_CALL vm_op_strdup_String(vm_t vm, const char* s2) {
	vm_push_data(vm,DataObjStr,(word_t)vm_string_new(s2));
}


void _VM_CALL vm_op_strcmp(vm_t vm, word_t unused) {
	vm_data_t s2 = _vm_pop(vm);
	assert(s2->type==DataString||s2->type==DataObjStr);
	vm_op_strcmp_String(vm,(const char*)s2->data);
}


void _VM_CALL vm_op_strcat(vm_t vm, word_t unused) {
	vm_data_t s2 = _vm_pop(vm);
	assert(s2->type==DataString||s2->type==DataObjStr);
	vm_op_strcat_String(vm,(char*)s2->data);
}


void _VM_CALL vm_op_strdup(vm_t vm, word_t unused) {
	vm_data_t s2 = _vm_pop(vm);
	assert(s2->type==DataString||s2->type==DataObjStr);
	vm_op_strdup_String(vm,(char*)s2->data);
}


void _VM_CALL vm_op_charAt_Int(vm_t vm, word_t ofs) {
	vm_data_t d = _vm_pop(vm);
	const char* s = (const char*)d->data;
	assert(d->type==DataString||d->type==DataObjStr);
	vm_push_data(vm,DataChar,(word_t)s[ofs]);
}


void _VM_CALL vm_op_charAt(vm_t vm, word_t unused) {
	vm_data_t d = _vm_pop(vm);
	assert(d->type==DataInt);
	vm_op_charAt_Int(vm,d->data);
}


void _VM_CALL vm_op_setCharAt_Int(vm_t vm, word_t ofs) {
	vm_data_t c = _vm_pop(vm);
	vm_data_t d = _vm_pop(vm);
	char* s = (char*)d->data;
	assert(d->type==DataString||d->type==DataObjStr);
	assert(c->type==DataChar);
	s[ofs] = (char)c->data;
}


void _VM_CALL vm_op_setCharAt(vm_t vm, word_t unused) {
	vm_data_t d = _vm_pop(vm);
	assert(d->type==DataInt);
	vm_op_setCharAt_Int(vm,d->data);
}


void _VM_CALL vm_op_substr(vm_t vm, word_t unused) {
	vm_data_t end = _vm_pop(vm);
	vm_data_t start = _vm_pop(vm);
	vm_data_t str = _vm_pop(vm);
	const char*s = (const char*) str->data;
	char*ret;
	assert(str->type==DataObjStr||str->type==DataString);
	ret = vm_string_new_buf(end-start+1);
	ret[end-start+1]=0;
	strncpy(ret,s+start->data,end->data-start->data);
	vm_push_data(vm,DataObjStr,(word_t)ret);
}


void _VM_CALL vm_op_strlen(vm_t vm, word_t unused) {
	vm_data_t str = _vm_pop(vm);
	assert(str->type==DataObjStr||str->type==DataString);
	vm_push_data(vm,DataInt,(word_t)strlen((char*)str->data));
}

/**********************************************************
 * regGet:Int
 * fetch some item at index [arg] in array
 * Object -> (something)
 */
void _VM_CALL vm_op_regGet_Int(vm_t vm, word_t index) {
	word_t* data = (word_t*)(vm->current_thread->registers+index);
	assert(index<TINYAML_N_REGISTERS);
	vm_push_data(vm, *data, *(data+1));
}

/**********************************************************
 * regGet
 * fetch some item at some index in array
 * Object X Int -> (something)
 */
void _VM_CALL vm_op_regGet(vm_t vm, word_t unused) {
	vm_data_t d = _vm_pop(vm);
	assert(d->type==DataInt);
	vm_op_regGet_Int(vm,d->data);
}

/**********************************************************
 * regSet:Int
 * set the item at index [arg] in array to (something)
 * Object X (something) -> Object
 */
void _VM_CALL vm_op_regSet_Int(vm_t vm, word_t index) {
	vm_data_t olddata = vm->current_thread->registers+index;
	vm_data_t data = _vm_pop(vm);
	assert(index<TINYAML_N_REGISTERS);
	if(olddata->type&DataManagedObjectFlag) { vm_obj_deref_ptr(vm,(void*)olddata->data); }
	memcpy(vm->current_thread->registers+index, data, sizeof(struct _data_stack_entry_t));
	if(data->type&DataManagedObjectFlag) { vm_obj_ref_ptr(vm,(void*)data->data); }
}

/**********************************************************
 * regSet
 * set the item at some index in array to (something)
 * Object X (something) X Int -> Object
 */
void _VM_CALL vm_op_regSet(vm_t vm, word_t unused) {
	vm_data_t d = _vm_pop(vm);
	assert(d->type==DataInt);
	vm_op_regSet_Int(vm,d->data);
}


/*@}*/

/*! \addtogroup vcop_vobj
 * @{
 */

void _VM_CALL vm_op__vobj_new(vm_t vm, word_t unused) {
	vm_push_data(vm, DataObjVObj, (word_t) vobj_new());
}

void _VM_CALL vm_op__vobj_gcls(vm_t vm, word_t unused) {
	vobj_ref_t obj = (vobj_ref_t) dynamic_cast(vm, _vm_pop(vm), DataObjVObj, NULL, NULL);
	vm_push_data(vm, DataObjVCls, (word_t) (obj->cls?obj->cls:ClassNil));
}

void _VM_CALL vm_op__vobj_scls(vm_t vm, word_t unused) {
	vobj_class_t cls = (vobj_class_t) dynamic_cast(vm, _vm_pop(vm), DataObjVCls, NULL, NULL);
	vobj_ref_t obj = (vobj_ref_t) dynamic_cast(vm, _vm_pop(vm), DataObjVObj, NULL, NULL);
	vm_printf("set class to %p for object %p\n", cls, obj);
	obj->cls = cls;
}

void _VM_CALL vm_op__vobj_gmbr_Int(vm_t vm, word_t index) {
	vobj_ref_t obj = (vobj_ref_t) dynamic_cast(vm, _vm_pop(vm), DataObjVObj, NULL, NULL);
	vm_data_t d = vobj_get_member(obj, index);
	vm_push_data(vm, d->type, d->data);
}

void _VM_CALL vm_op__vobj_gmbr(vm_t vm, word_t unused) {
	word_t index = dynamic_cast(vm, _vm_pop(vm), DataInt, NULL, NULL);
	vm_op__vobj_gmbr_Int(vm, index);
}

void _VM_CALL vm_op__vobj_smbr_Int(vm_t vm, word_t index) {
	vm_data_t d = _vm_pop(vm);
	vobj_ref_t obj = (vobj_ref_t) dynamic_cast(vm, _vm_pop(vm), DataObjVObj, NULL, NULL);
	vobj_set_member(obj, index, d);
}

void _VM_CALL vm_op__vobj_smbr(vm_t vm, word_t unused) {
	word_t index = dynamic_cast(vm, _vm_pop(vm), DataInt, NULL, NULL);
	vm_op__vobj_smbr_Int(vm, index);
}

void _VM_CALL vm_op__vcls_new(vm_t vm, word_t unused) {
	vm_push_data(vm, DataObjVCls, (word_t) vclass_new());
}

void _VM_CALL vm_op__vcls_gname(vm_t vm, word_t unused) {
	vobj_class_t cls = (vobj_class_t) dynamic_cast(vm, _vm_pop(vm), DataObjVCls, NULL, NULL);
}

void _VM_CALL vm_op__vcls_sname(vm_t vm, word_t unused) {
	const char* name = (const char*) dynamic_cast(vm, _vm_pop(vm), DataString, NULL, NULL);
	vobj_class_t cls = (vobj_class_t) dynamic_cast(vm, _vm_pop(vm), DataObjVCls, NULL, NULL);
	vclass_set_name(cls, name);
}

void _VM_CALL vm_op__vcls_cto(vm_t vm, word_t unused) {
	int fail=0;
	vobj_class_t cls;
	vobj_ref_t obj;
	vm_data_t d = _vm_pop(vm);
	cls = (vobj_class_t) dynamic_cast(vm, d, DataObjVCls, NULL, &fail);
	if(fail) {
		vm_data_type_t dt  = (vm_data_type_t) dynamic_cast(vm, d, DataInt, NULL, NULL);
		vm_push_data(vm, dt, dynamic_cast(vm, d, dt, NULL, NULL));
	} else {
		vm_push_data(vm, DataObjVObj, dynamic_cast(vm, d, DataObjVObj, cls, NULL));
	}
}

void _VM_CALL vm_op__vcls_cfrom(vm_t vm, word_t unused) {
	vobj_class_t cls = (vobj_class_t) dynamic_cast(vm, _vm_pop(vm), DataObjVCls, NULL, NULL);
	vm_data_t d = _vm_pop(vm);
	vm_push_data(vm, DataObjVObj, dynamic_cast(vm, d, DataObjVObj, cls, NULL));
}

void _VM_CALL vm_op__vcls_soo(vm_t vm, word_t unused) {
	vm_dyn_func_t df = (vm_dyn_func_t) dynamic_cast(vm, _vm_pop(vm), DataObjFun, NULL, NULL);
	word_t ocarg = dynamic_cast(vm, _vm_pop(vm), DataInt, NULL, NULL);
	char* oc = (char*) dynamic_cast(vm, _vm_pop(vm), DataString, NULL, NULL);
	vobj_class_t cls = (vobj_class_t) dynamic_cast(vm, _vm_pop(vm), DataObjVCls, NULL, NULL);
	struct _data_stack_entry_t d = { DataObjFun, (word_t) df };
	vclass_set_overload(cls, opcode_stub_by_name(&vm->opcodes, ocarg, oc), &d);
}

/*@}*/

