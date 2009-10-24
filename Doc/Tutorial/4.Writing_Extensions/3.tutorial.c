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

#include <tinyaml/tinyaml.h>
#include <tinyaml/dynarray.h>

#include <string.h>
#include <stdlib.h>

/* opcodes declaration have the following syntax :

void _VM_CALL vm_op_OPCODENAME_ARGUMENTTYPE ( vm_t vm, word_t argument_value );

where OPCODENAME is the name as declared in the corresponding opcode statement
in the .tinyalib file and ARGUMENTTYPE is one of Int, Float, String, Label,
EnvSym. For opcodes without arguments, the name is simply vm_op_OPCODENAME.

argument value is encoded as a 32bit integer but can be a (const char*), (tinyaml_float_t), (long) according to the argument type.

*/

void _VM_CALL vm_op_hello(vm_t vm, word_t unused) {
	/* to output something, use vm_printf for stdout and vm_printerrf for
	 * stderr. They both behave like printf.
	 */
	vm_printf("Hello, opcode world !\n");
}


void _VM_CALL vm_op_withArg_Int(vm_t vm, long i) {
	vm_printerrf("withArg : got long %i\n", i);
}


void _VM_CALL vm_op_withArg_Float(vm_t vm, tinyaml_float_t f) {
	vm_printerrf("withArg : got tinyaml_float_t %f\n", f);
}


void _VM_CALL vm_op_withArg_String(vm_t vm, const char* str) {
	vm_printerrf("withArg : got string \"%s\"\n", str);
}


void _VM_CALL vm_op_consume(vm_t vm, word_t unused) {
	/* a union to q&d convert representations (NOT DATA). */
	_IFC conv;
	/* a VERY typical line to fetch data from stack top */
	vm_data_t d = _vm_pop(vm);
	/* now d->type holds data type (see the vm_data_types_t enum in
	 * vm_types.h) and d->data holds the actual value, stored as a
	 * word_t.
	 */
	char dump[128]="";
	switch(d->type) {
	case DataNone		: sprintf(dump, "None"); break;
	case DataInt		: sprintf(dump, "Integer=%li", d->data); break;
	case DataFloat		: conv.i = d->data; sprintf(dump, "Float=%f", conv.f); break;
	case DataString		:
	case DataObjStr		:
		sprintf(dump, "String=\"%s\"", (const char*)d->data);
		break;
	case DataObjSymTab	: sprintf(dump, "Symbol table"); break;
	case DataObjMutex	: sprintf(dump, "Mutex"); break;
	case DataObjThread	: sprintf(dump, "Thread"); break;
	case DataObjArray	: sprintf(dump, "Array"); break;
	case DataObjEnv		: sprintf(dump, "Map"); break;
	case DataObjStack	: sprintf(dump, "Stack"); break;
	case DataObjFun		: sprintf(dump, "Dynamic Function"); break;
	case DataObjVObj	: sprintf(dump, "Object (yet to be implemented, heh)"); break;
	case DataObjUser	: sprintf(dump, "User type (managed) magic=%8.8lX", *(word_t*)d->data); break;
	default	:
		sprintf(dump, "DataTypeError [%x]", d->type);
	};
	
}


void _VM_CALL vm_op_maxInArray(vm_t vm, word_t unused) {
	long max = 0x80000000;
	word_t size = 0;
	word_t i;
	vm_data_type_t item_type;
	long item_value;
	vm_data_t d = _vm_pop(vm);
	/* introducing the dyn(amic) array type */
	dynarray_t a = (dynarray_t) d->data;
	/* check that we actually fetched an array */
	assert(d->data == DataObjArray);

	/* get the array size */
	size = dynarray_size(a);
	/* look for maximum */
	for(i=0;i<size;i+=2) {
		/* data in dynarrays is encoded as two words */
		item_type = (vm_data_type_t) dynarray_get(a, i);
		item_value = (long) dynarray_get(a, i+1);
		assert(item_type == DataInt);
		if(max<item_value) {
			max = item_value;
		}
	}

	vm_push_data(vm, DataInt, max);
}


void _VM_CALL vm_op_makeFibo_Int(vm_t vm, word_t size) {
	dynarray_t fibo = (dynarray_t) vm_array_new();
	word_t i;
	long fi;
	/* convert size from data size to word size */
	size<<=1;
	/* reserve space so there will be no realloc() */
	dynarray_reserve(fibo, size);
	if(size>0) {
		dynarray_set(fibo, 0, (word_t) DataInt);
		dynarray_set(fibo, 1, 1);
		if(size>1) {
			dynarray_set(fibo, 2, (word_t) DataInt);
			dynarray_set(fibo, 3, 1);
		}
	}
	for(i=4;i<size;i+=2) {
		fi = dynarray_get(fibo, i-1)+dynarray_get(fibo, i-3);
		dynarray_set(fibo, i, (word_t) DataInt);
		dynarray_set(fibo, i+1, (word_t) fi);
	}
	vm_push_data(vm, DataObjArray, (word_t) fibo);
}


void _VM_CALL vm_op_makeFibo(vm_t vm, word_t unused) {
	/* simple wrapper around makeFibo_Int, where the argument is taken from the stack */
	vm_data_t d = _vm_pop(vm);
	assert(d->type==DataInt);

	vm_op_makeFibo_Int(vm, d->data);
}


extern void _VM_CALL vm_op_mapSet_String(vm_t, word_t);

void _VM_CALL vm_op_exampleMap(vm_t vm, word_t unused) {
	vm_dyn_env_t env = vm_env_new();

	/* use existing opcode to set a map value,
	 * it takes care of reference counting for
	 * managed objects and so on.
	 */
	vm_push_data(vm, DataString, (word_t) "Hello, map item here");
	vm_push_data(vm,DataObjEnv, (word_t) env);
	vm_op_mapSet_String(vm, (word_t) "hello");

	vm_push_data(vm,DataObjEnv,(word_t)env);
}


void _VM_CALL vm_op_getLabelOfs_Label(vm_t vm, long offset) {
	vm_push_data(vm, DataInt, offset);
}


extern void _VM_CALL vm_op_envGet_EnvSym(vm_t vm, long index);

void _VM_CALL vm_op_getEnvSymOfsAndValue_EnvSym(vm_t vm, long envsym) {
	vm_push_data(vm, DataInt, envsym);
	/*vm_printf("envsym=%i %X\n", envsym, envsym);*/
	vm_op_envGet_EnvSym(vm, envsym);
}

