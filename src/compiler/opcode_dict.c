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

#include "_impl.h"
#include "vm.h"
#include "opcode_dict.h"

#include <dlfcn.h>

#include <string.h>

word_t hash_str(char*str) {
	word_t ret;
	if(!str) {
		return 0;
	}
	ret=strlen(str);
	while(*str) {
		ret*=*str;
		str+=1;
	}
	return ret%HASH_SIZE;
}

word_t hash_ptr(void* w) {
	return ((word_t)w)%HASH_SIZE;
}

int cmp_ptr(void* w, void* v) {
	return (int)(w-v);
}

opcode_dict_t opcode_dict_new() {
	opcode_dict_t od = (opcode_dict_t)malloc(sizeof(struct _opcode_dict_t));
	opcode_dict_init(od);
	return od;
}

void opcode_dict_free(opcode_dict_t od) {
	opcode_dict_deinit(od);
	free(od);
}






opcode_dict_t opcode_dict_optimize(vm_t vm, program_t prog) {
	opcode_dict_t glob = vm_get_dict(vm);
	opcode_dict_t od = opcode_dict_new();
	const char* name;
	opcode_arg_t arg_type;
	opcode_stub_t stub;
	int i;
	/*vm_printf("Opcode dictionary :\n");*/
	for(i=0;i<prog->code.size;i+=2) {
		stub = (opcode_stub_t)prog->code.data[i];
		name = opcode_name_by_stub(glob,stub);
		arg_type = WC_GET_ARGTYPE(opcode_code_by_stub(glob,stub));
		opcode_dict_add(od, arg_type, name, stub);
		/*vm_printf("- %s:%i [%p]\n",name,arg_type,stub);*/
	}
	return od;
}



void opcode_dict_init(opcode_dict_t od) {
	int i;
	for(i=0;i<OpcodeTypeMax;i+=1) {
		dynarray_init(&od->stub_by_index[i]);
		init_hashtab(&od->stub_by_name[i],(hash_func) hash_str, (compare_func) strcmp);
	}
	init_hashtab(&od->wordcode_by_stub, (hash_func) hash_ptr, (compare_func) cmp_ptr);
	init_hashtab(&od->name_by_stub, (hash_func) hash_ptr, (compare_func) cmp_ptr);
}

void htab_free_dict(htab_entry_t e) {
	free(e->key);
}

void opcode_dict_deinit(opcode_dict_t od) {
	int i;
	for(i=0;i<OpcodeTypeMax;i+=1) {
		dynarray_deinit(&od->stub_by_index[i],NULL);
		clean_hashtab(&od->stub_by_name[i],htab_free_dict);
	}
	clean_hashtab(&od->wordcode_by_stub, NULL);
	clean_hashtab(&od->name_by_stub, NULL);
}


void opcode_dict_add(opcode_dict_t od, opcode_arg_t arg_type, const char* name, opcode_stub_t stub) {
	word_t ofs = dynarray_size(&od->stub_by_index[arg_type]);
	/* should check for duplicates in arg_type:name AND in stub */

	const char* common_key;

	if(!stub) {
		vm_printerrf("[VM:ERR] Tried to load NULL opcode %s:%i\n", name, arg_type);
		return;
	}

	if(opcode_stub_by_name(od, arg_type, name)) {
		return;
	}
	common_key = strdup(name);

	hash_addelem(&od->stub_by_name[arg_type], (hash_key)common_key, (hash_elem)stub);
	hash_addelem(&od->wordcode_by_stub, (hash_key)stub, (hash_elem) MAKE_WC(arg_type,ofs));
	hash_addelem(&od->name_by_stub, (hash_key)stub, (hash_elem) common_key);
	dynarray_set(&od->stub_by_index[arg_type],ofs,(word_t)stub);
}


opcode_stub_t opcode_stub_by_name(opcode_dict_t od, opcode_arg_t arg_type, const char* name) {
	return (opcode_stub_t) hash_find(&od->stub_by_name[arg_type], (hash_key)(char*)name);
}

opcode_stub_t opcode_stub_by_code(opcode_dict_t od, word_t wordcode) {
	return (opcode_stub_t) dynarray_get(&od->stub_by_index[WC_GET_ARGTYPE(wordcode)], WC_GET_OP(wordcode));
}

word_t opcode_code_by_stub(opcode_dict_t od, opcode_stub_t stub) {
	return (word_t) hash_find(&od->wordcode_by_stub, (hash_key)stub);
}

const char* opcode_name_by_stub(opcode_dict_t od, opcode_stub_t stub) {
	return (const char*) hash_find(&od->name_by_stub, (hash_key)stub);
}


int opcode_dict_link_stubs(opcode_dict_t target, opcode_dict_t src) {
	return 0;
}

int opcode_dict_resolve_stubs(opcode_dict_t src) {
	return 0;
}


void opcode_dict_serialize(opcode_dict_t od, writer_t w) {
	int i,j,tot;
	const char* name;
	/* write header */
	write_string(w,"DIC");
	/* write number of opcode arrays */
	write_word(w,OpcodeTypeMax);

	/* now for each array */
	for(i=0;i<OpcodeTypeMax;i+=1) {
		tot = dynarray_size(&od->stub_by_index[i]);
		/* write number of records in array */
		write_word(w,tot);
		/* now for each record */
		for(j=0;j<tot;j+=1) {
			/* retrieve name */
			name = opcode_name_by_stub( od, (opcode_stub_t) dynarray_get( &od->stub_by_index[i], j));
			/* write string length */
			write_word(w,1+strlen(name));
			/* write string */
			write_string(w,name);
		}
		/* write 0xFFFFFF to end the record */
			write_word(w,0xFFFFFFFF);
	}
}


opcode_stub_t opcode_stub_resolve(opcode_arg_t arg_type, const char* name, void* dl_handle) {
	char* stub_name;
	const char* arg_name;
	opcode_stub_t ret;
	if(!name) {
		return NULL;
	}
	ret = opcode_stub_by_name(&_glob_vm->opcodes, arg_type, name);
	if(ret) {
		return ret;
	}
	stub_name = (char*)malloc(strlen(name)+20);
	switch(arg_type) {
	case OpcodeArgInt:	arg_name="_Int";	break;
	case OpcodeArgChar:	arg_name="_Char";	break;
	case OpcodeArgFloat:	arg_name="_Float";	break;
	case OpcodeArgLabel:	arg_name="_Label";	break;
	case OpcodeArgString:	arg_name="_String";	break;
	case OpcodeArgEnvSym:	arg_name="_EnvSym";	break;
	default:;
	case OpcodeNoArg:	arg_name="";
	};
	sprintf(stub_name, "vm_op_%s%s", name, arg_name);
	ret = dlsym(dl_handle, stub_name);
	free(stub_name);
	return ret;
}



void opcode_dict_unserialize(opcode_dict_t od, reader_t r, void* dl_handle) {
	int i,j,tot,typtot;
	const char* name;

	/* read dict header and check it against "DIC" */
	name = read_string(r);
	assert(!strcmp(name,"DIC"));
	
	/* read number of arrays, check it against OpcodeTypeMax */
	typtot = read_word(r);
	assert(typtot==OpcodeTypeMax);

	/* now for each array */
	for(i=0;i<OpcodeTypeMax;i+=1) {
		/* read number of records in array */
		tot = read_word(r);
		/* now for each record */
		for(j=0;j<tot;j+=1) {
			/* retrieve name */
			typtot = read_word(r);
			name = read_string(r);
			assert(typtot == 1+strlen(name));
			opcode_dict_add(od, i, name, opcode_stub_resolve(i,name,dl_handle));
		}
		/* read END OF RECORD */
		typtot = read_word(r);
		assert(typtot==0xFFFFFFFF);
	}
}



