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
#include "vm_types.h"
#include "text_seg.h"
#include "program.h"
#include "fastmath.h"
#include "opcode_chain.h"
#include "opcode_dict.h"
#include "vm.h"
#include "object.h"

#include <limits.h>
#include <string.h>
#include <stdio.h>

void _VM_CALL vm_op_push_Int(vm_t,word_t);
void _VM_CALL vm_op_dup_Int(vm_t,word_t);
void _VM_CALL vm_op_ret_Int(vm_t,word_t);
void _VM_CALL vm_op_setmem(vm_t,word_t);
void _VM_CALL vm_op_SZ(vm_t,word_t);
void _VM_CALL vm_op_dec(vm_t,word_t);
void _VM_CALL vm_op_jmp_Label(vm_t,word_t);
vm_t vm_run_program_fg(vm_t vm, program_t p, word_t ip, word_t prio);




word_t clean_data_seg[20] = {
	(word_t)vm_op_push_Int,	0,		/* counter, */
/* label: */
	(word_t)vm_op_push_Int,	0,		/* counter, 0 */
	(word_t)vm_op_dup_Int,	(word_t)-1,	/* counter, 0, counter */
	(word_t)vm_op_dec,	0,		/* counter, 0, counter-1 */
	(word_t)vm_op_setmem,	0,		/* counter, */
	(word_t)vm_op_dec,	0,		/* counter-1, */
	(word_t)vm_op_dup_Int,	0,		/* counter-1, counter-1, */
	(word_t)vm_op_SZ,	0,		/* counter-1, */
	(word_t)vm_op_jmp_Label,(word_t)-14,	/* counter-1, */
	(word_t)vm_op_ret_Int,	1,		/* , */
};

void program_set_source(program_t p, const char* s) {
	free((char*)p->source);
	p->source = strdup(s);
}

const char* program_get_source(program_t p) {
	return p->source;
}



const char* find_file(const char*fname) {
	FILE*f;
	static char buf[512];
	f = fopen(fname, "r");				/* try $PWD first */
	if(!f) {
		sprintf(buf,"~/.tinyaml/%s", fname);
		f = fopen(buf, "r");			/* try ~/.tinyaml/ then */
		if(!f) {
			sprintf(buf, TINYAML_DATA_DIR "/%s", fname);
			f = fopen(buf, "r");		/* finally, try site dir */
			if(f) {
				fclose(f);
				return strdup(buf);
			}
		} else {
			fclose(f);
			return strdup(buf);
		}
	} else {
		fclose(f);
		return strdup(fname);
	}
	return NULL;
}


void program_add_require(program_t prg, const char*fname) {
	/* compile and fg-execute the mentioned program */
	FILE*f;
	char buffy[256] = "";
	char* found_fname;
	program_t p;
	p = (program_t) hash_find(&_glob_vm->required,(hash_key)fname);
	if(!p) {
		found_fname = (char*)find_file(fname);
		if(!found_fname) {
			vm_printerrf("[VM:ERR] : compiler couldn't find file %s\n",fname);
			return;
		}
		f = fopen(found_fname, "r");
		if(!f) {
			vm_printerrf("[VM:ERR] : compiler couldn't open file %s\n",fname);
			return;
		}
		fread(buffy,strlen(TINYAML_SHEBANG),1,f);
		fclose(f);
		if(strcmp(buffy,TINYAML_SHEBANG)) {
			/* looks like a source file */
			/* try and compile the file */
			if(_glob_vm->result) {
				opcode_chain_delete(_glob_vm->result);	/* discard result, anyway it is empty at this point */
				/*_glob_vm->result=NULL;*/
				p = vm_compile_file(_glob_vm, found_fname);
				_glob_vm->result = opcode_chain_new();
			} else {
				p = vm_compile_file(_glob_vm, found_fname);
			}
		} else {
			/* looks like a serialized program */
			/* unserialize program */
			reader_t r = file_reader_new(found_fname);
			p = vm_unserialize_program(_glob_vm,r);
			reader_close(r);
		}
		if(p) {
			program_set_source(p, fname);
			vm_run_program_fg(_glob_vm,p,0,50);
			vm_printf("[VM:INFO] Required file executed.\n");
			hash_addelem(&_glob_vm->required,strdup(fname),p);
		} else {
			vm_printerrf("[VM:ERR] Nothing to execute while requiring %s\n",fname);
		}
		if(found_fname) {
			free(found_fname);
		}
	/*} else {*/
		/*vm_printerrf("[VM:INFO] Required file %s has already been loaded.\n",fname);*/
	}
}



void program_add_loadlib(program_t prg, const char*libname) {
	static char libpath[PATH_MAX];
	opcode_chain_t backup;
	program_t p;
	snprintf(libpath,PATH_MAX,TINYAML_EXT_DIR "/%s.tinyalib",libname);
	p = (program_t) hash_find(&_glob_vm->loadlibs,libpath);
	if(!p) {
		backup=_glob_vm->result;
		_glob_vm->result=NULL;
		p = vm_compile_file(_glob_vm, libpath);
		_glob_vm->result = backup;
		hash_addelem(&_glob_vm->loadlibs,strdup(libpath),p);
		if(p) {
			vm_run_program_fg(_glob_vm,p,0,50);
			/*vm_printerrf("[VM:INFO] Library %s loaded.\n",libname);*/
		} else {
			vm_printerrf("[VM:ERR] Couldn't load library %s\n",libname);
		}
	/*} else {*/
		/*vm_printerrf("[VM:INFO] Library %s has already been loaded.\n",libname);*/
	}
}



program_t program_new() {
	program_t ret = (program_t)malloc(sizeof(struct _program_t));
	/*init_hashtab(&ret->labels, (hash_func) hash_str, (compare_func) strcmp);*/
	ret->source=strdup("(unset)");
	ret->env=NULL;
	text_seg_init(&ret->labels.labels);
	dynarray_init(&ret->labels.offsets);
	dynarray_set(&ret->labels.offsets,0,0);
	text_seg_init(&ret->strings);
	text_seg_init(&ret->loadlibs);
	text_seg_init(&ret->requires);
	dynarray_init(&ret->gram_nodes_indexes);
	dynarray_init(&ret->data);
	dynarray_init(&ret->code);
	/*program_write_code(ret, (word_t)vm_op_nop, 0);*/
	/*vm_printf("PROGRAM NEW %p\n",ret);*/
	return ret;
}


void program_free(vm_t vm, program_t p) {
	/*vm_printf("program_free %p\n",p);*/
	text_seg_deinit(&p->strings);
	dynarray_deinit(&p->code,NULL);
	if(p->data.size) {
		/*p->code.size=18;*/
		/*p->code.reserved=0;*/
		/*p->code.data=clean_data_seg;*/
		/*clean_data_seg[1]=p->data.size>>1;*/
		/*vm_printf("cleaning %lu data items\n",clean_data_seg[1]);*/
		/*vm_run_program_fg(vm,p,0,99);*/
		int i;
		/*vm_printf("dereff'ing data segment (%lu items)\n",p->data.size>>1);*/
		for(i=0;i<p->data.size;i+=2) {
			if((vm_data_type_t)p->data.data[i]&DataManagedObjectFlag) {
				/*vm_printf("found an object : %p\n",(void*)p->data.data[i+1]);*/
				vm_obj_deref_ptr(vm,(void*)p->data.data[i+1]);
			}
		}
	}
	text_seg_deinit(&p->labels.labels);
	dynarray_deinit(&p->labels.offsets,NULL);
	dynarray_deinit(&p->gram_nodes_indexes,NULL);
	dynarray_deinit(&p->data,NULL);
	free((char*)p->source);
	free(p);
}


word_t opcode_arg_serialize(program_t p, opcode_arg_t arg_type, word_t arg, vm_dyn_env_t smallenv) {
	switch(arg_type) {
	case OpcodeArgEnvSym:
		vm_printerrf("[VM:INFO] Serializing EnvSym #%i '%s'\n", arg, text_seg_find_by_index(&p->env->symbols, arg));
		arg = text_seg_text_to_index(&smallenv->symbols, text_seg_find_by_index(&p->env->symbols, arg));
		if(!arg) {
			vm_printerrf("[VM:ERR] Serializing EnvSym : Couldn't resolve environment symbol  #%i '%s'\n", arg, text_seg_find_by_index(&p->env->symbols, arg));
			vm_fatal("Aborted.");
		} else {
			vm_printerrf("[VM:INFO] Serializing EnvSym : environment symbol '%s' resolved to #%i\n", text_seg_find_by_index(&p->env->symbols, arg), arg);
		}
		break;
	case OpcodeArgString:
		arg = text_seg_text_to_index(&p->strings,(const char*)arg);
		break;
	default:;
	};
	return arg;
}


word_t opcode_arg_unserialize(program_t p, opcode_arg_t arg_type, word_t arg, vm_dyn_env_t smallenv) {
	switch(arg_type) {
	case OpcodeArgEnvSym:
		/*vm_printerrf("[VM:INFO] Unserializing EnvSym #%i '%s'\n", arg, text_seg_find_by_index(&smallenv->symbols, arg));*/
		arg = text_seg_text_to_index(&p->env->symbols, text_seg_find_by_index(&smallenv->symbols, arg));
		if(!arg) {
			vm_printerrf("[VM:ERR] Unserializing EnvSym : Couldn't resolve environment symbol  #%i '%s'\n", arg, text_seg_find_by_index(&smallenv->symbols, arg));
			vm_fatal("Aborted.");
		} else {
			/*vm_printerrf("[VM:INFO] Unserializing EnvSym : environment symbol '%s' resolved to #%i\n", text_seg_find_by_index(&smallenv->symbols, arg), arg);*/
		}
		break;
	case OpcodeArgString:
		arg = (word_t) text_seg_find_by_index(&p->strings,arg);
		break;
	default:;
	};
	return arg;
}


vm_dyn_env_t program_env_optimize(vm_t vm, program_t prog) {
	opcode_dict_t glob = vm_get_dict(vm);
	vm_dyn_env_t ret = vm_env_new();
	opcode_arg_t arg_type;
	opcode_stub_t stub;
	word_t idx;
	int i;

	for(i=0;i<prog->code.size;i+=2) {
		stub = (opcode_stub_t)prog->code.data[i];
		arg_type = WC_GET_ARGTYPE(opcode_code_by_stub(glob,stub));
		if(arg_type==OpcodeArgEnvSym) {
			idx = text_seg_text_to_index(&ret->symbols,text_seg_find_by_text(&ret->symbols,(const char*) vm->env->symbols.by_index.data[ prog->code.data[i+1] ] ));
			/*dynarray_set(&ret->data,idx<<1, DataInt);*/
			/*dynarray_set(&ret->data,1+(idx<<1), DataInt);*/
		}
	}

	return ret;
}


void program_serialize(vm_t vm, program_t p, writer_t w) {
	int i;
	word_t op;
	word_t arg;
	const char* lbl;
	vm_dyn_env_t env;
	/* optimize opcode dictionary */
	opcode_dict_t odopt = opcode_dict_optimize(vm,p);
	/* optimize env */
	env = program_env_optimize(vm, p);
	/* write libs */
	text_seg_serialize(&p->loadlibs,w,"LIB");
	/* write requires */
	text_seg_serialize(&p->requires,w,"REQ");
	/* write env */
	text_seg_serialize(&env->symbols,w,"ENV");
	/* write text segment */
	text_seg_serialize(&p->strings,w,"STRINGS");
	/* write dict */
	opcode_dict_serialize(odopt,w);
	/* write code segment */
	write_string(w,"LABELS-");
	/* write labels count */
	write_word(w,dynarray_size(&p->labels.offsets));
	for(i=1;i<dynarray_size(&p->labels.offsets);i+=1) {
		lbl = (const char*) p->labels.labels.by_index.data[i];
		write_word(w,strlen(lbl));
		write_string(w,lbl);
		write_word(w,p->labels.offsets.data[i]);
	}
	write_word(w,0xFFFFFFFF);
	/* write code segment */
	write_string(w,"CODE---");
	/* write code segment size */
	write_word(w,dynarray_size(&p->code));

	/* write serialized word code */
	for(i=0;i<dynarray_size(&p->code);i+=2) {
		op = opcode_code_by_stub(odopt, (opcode_stub_t)dynarray_get(&p->code,i));
		arg = opcode_arg_serialize(p, WC_GET_ARGTYPE(op), dynarray_get(&p->code,i+1), env);
		/*vm_printf("%8.8lX %8.8lX  ",op,arg);*/
		/*if(i%8==6) vm_printf("\n");*/
		write_word(w,op);
		write_word(w,arg);
	}
	/*vm_printf("\n");*/
	opcode_dict_free(odopt);
	write_word(w,0xFFFFFFFF);
	write_string(w,"DATA---");
	write_word(w,dynarray_size(&p->data));
	for(i=0;i<dynarray_size(&p->data);i+=2) {
		op = dynarray_get(&p->data,i);
		arg = opcode_arg_serialize(p, op, dynarray_get(&p->data,i+1),NULL);	/* EnvSym isn't supposed to happen in a data segment */
		write_word(w,op);
		write_word(w,arg);
	}
	vm_obj_free_obj(vm,PTR_TO_OBJ(env));
}



program_t program_unserialize(vm_t vm, reader_t r) {
	int i;
	const char*str;
	opcode_dict_t od;
	program_t p;
	word_t tot;
	word_t op;
	word_t wc;
	word_t arg;
	vm_dyn_env_t env = vm_env_new();

	/*vm_printf("program_unserialize\n");*/
	p=program_new();

	text_seg_unserialize(&p->loadlibs,r,"LIB");

	for(i=1;i<dynarray_size(&p->loadlibs.by_index);i+=1) {
		vm_printerrf("[VM:INFO] Program requires library '%s'\n", text_seg_find_by_index(&p->loadlibs,i));
		program_add_loadlib(p, text_seg_find_by_index(&p->loadlibs,i));
	}

	text_seg_unserialize(&p->requires,r,"REQ");

	for(i=1;i<dynarray_size(&p->requires.by_index);i+=1) {
		vm_printerrf("[VM:INFO] Program requires file '%s'\n", text_seg_find_by_index(&p->requires,i));
		program_add_require(p, text_seg_find_by_index(&p->requires,i));
	}

	text_seg_unserialize(&env->symbols,r,"ENV");

	p->env=vm->env;

	text_seg_unserialize(&p->strings,r,"STRINGS");

	od = opcode_dict_new();
	opcode_dict_unserialize(od,r,vm->dl_handle);

	str = read_string(r);
	assert(!strcmp(str,"LABELS-"));
	tot = read_word(r);
	for(i=1;i<tot;i+=1) {
		wc = read_word(r);
		str = read_string(r);
		assert(strlen(str)==wc);
		wc = read_word(r);
		program_add_label(p,wc,str);
	}
	wc = read_word(r);
	assert(wc==0xFFFFFFFF);

	str = read_string(r);
	assert(!strcmp(str,"CODE---"));
	tot = read_word(r);
	dynarray_reserve(&p->code,tot+2);
	/*vm_printf("going for %lu (%lX) words\n",tot,tot);*/
	for(i=0;i<tot;i+=2) {
		wc = read_word(r);
		op = (word_t) opcode_stub_by_code(od, wc);
		arg = opcode_arg_unserialize(p, WC_GET_ARGTYPE(wc), read_word(r), env);
		program_write_code(p,op,arg);
		/*vm_printf("unserialized (%p,%lX) at %lu (%lX) (code size=%lu (%lX))\n",op,arg,i,i,p->code.size,p->code.size);*/
	}
	opcode_dict_free(od);
	wc = read_word(r);
	assert(wc==0xFFFFFFFF);
	str = read_string(r);
	assert(!strcmp(str,"DATA---"));
	wc = read_word(r);
	dynarray_reserve(&p->data,wc+2);
	for(i=0;i<wc;i+=2) {
		p->data.data[i] = read_word(r);
		p->data.data[i+1] = opcode_arg_unserialize(p, p->data.data[i], read_word(r), NULL);
	}
	p->data.size=wc;
	vm_obj_free_obj(vm,PTR_TO_OBJ(env));
	return p;
}




void program_add_label(program_t p, word_t ip, const char* label) {
	word_t index;
	text_seg_find_by_text(&p->labels.labels,label);
	index = text_seg_text_to_index(&p->labels.labels,label);
	if(index>=dynarray_size(&p->labels.offsets)) {
		dynarray_set(&p->labels.offsets,index,ip);
	} else {
		vm_printerrf("error : label %s is already defined.\n",label);
	}
}


word_t program_label_to_ofs(program_t p, const char* label) {
	word_t index = text_seg_text_to_index(&p->labels.labels,label);
	if(index==0) {
		vm_printerrf("warning : label unknown '%s'.\n",label);
		return 0;
	} else {
		return p->labels.offsets.data[index];
	}
}

const char* program_ofs_to_label(program_t p, word_t ip) {
	int i;
	for(i=0;i<p->labels.offsets.size;i+=1) {
		if(p->labels.offsets.data[i]==ip) {
			return text_seg_find_by_index(&p->labels.labels,i);
		}
	}
	return NULL;
}






void program_fetch(program_t p, word_t ip, word_t* op, word_t* arg) {
/*	vm_printf("fetch at @%p:%8.8lX : %8.8lX %8.8lX\n",p,ip,dynarray_get(&p->code,ip),dynarray_get(&p->code,ip+1)); */
	*arg = dynarray_get(&p->code,ip+1);
	*op = dynarray_get(&p->code,ip);
}

void program_reserve_code(program_t p, word_t sz) {
	sz+=dynarray_size(&p->code);
	dynarray_reserve(&p->code,sz);
}

void program_reserve_data(program_t p, word_t sz) {
	sz+=dynarray_size(&p->data);
	dynarray_reserve(&p->data,sz);
}

void program_write_code(program_t p, word_t op, word_t arg) {
	word_t ip = dynarray_size(&p->code);
/*	vm_printf("writing %8.8lX:%8.8lX into code seg at %p (%lu words long)\n",op,arg,p,dynarray_size(&p->code));
*/	dynarray_set(&p->code,ip+1,arg);
	dynarray_set(&p->code,ip,op);
/*	{
		int i;
		for(i=0;i<dynarray_size(&p->code);i+=2) {
			vm_printf("%8.8lX %8.8lX   ",dynarray_get(&p->code,i),dynarray_get(&p->code,i+1));
			if(i%8==6) vm_printf("\n");
		}
		vm_printf("\n");
	}
*/}

word_t program_get_code_size(program_t p) {
	return dynarray_size(&p->code);
}

word_t program_find_string(program_t p, const char*str) {
	return (word_t)text_seg_find_by_text(&p->strings,str);
}


const char* lookup_label_by_offset(struct _label_tab_t* ts, word_t IP) {
	/* effectue une recherche par dichotomie */
	word_t lower=0,upper=ts->offsets.size;
	word_t mid,val,found=0;

	/* because of thread IP increment after jump */
	/*IP += 2;*/

	do {
		mid = (lower+upper)>>1;
		val=ts->offsets.data[mid];
/*		vm_printf("lookup label for @%i in [%i:%i] : label[%i] = @%i\n", IP, lower, upper, mid, val);*/
		if(val==IP) {
			found=mid;
		} else if(val<IP) {
			lower=mid;
		} else {
			upper=mid;
		}
	} while((!found)&&upper>lower+1);
	return found?(const char*)ts->labels.by_index.data[found]:"";
}





const char* program_lookup_label(program_t p, word_t IP) {
	return lookup_label_by_offset(&p->labels,IP);
/*	int i;*/
/*	if(!p->labels.offsets.data) {*/
/*		return NULL;*/
/*	}*/
/*	vm_printf("lookup label @%i\n", IP);*/
/*	for(i=0;i<p->labels.offsets.size;i+=1) {*/
/*		if(p->labels.offsets.data[i]==IP) {*/
/*			return (const char*)p->labels.labels.by_index.data[i];*/
/*		}*/
/*	}*/
/*	return NULL;*/
}


const char* program_disassemble(vm_t vm, program_t p, word_t IP) {
	/* label sep opcode+arg \0 */
	char* buffy;
	word_t wc = opcode_code_by_stub(vm_get_dict(vm),(opcode_stub_t)p->code.data[IP]);
	const char* op = opcode_name_by_stub(vm_get_dict(vm),(opcode_stub_t)p->code.data[IP]);
	char* argstr;
	const char*tmp;
	char tmpbuf[40];
	_IFC conv;
	word_t arg = p->code.data[IP+1];

	if(!op) {
		return strdup("ERROR : unknown opcode.");
	}

	switch((opcode_arg_t)WC_GET_ARGTYPE(wc)) {
		case OpcodeArgLabel:
			tmp = (char*)program_lookup_label(p,IP+arg);
			if(tmp) {
				argstr=(char*)malloc(strlen(tmp)+2);
				sprintf(argstr,"@%s",tmp);
			} else {
				sprintf(tmpbuf,"%+li",(long)arg);
				argstr=strdup(tmpbuf);
			}
			break;
		case OpcodeArgString:
			argstr = (char*)malloc(strlen((char*)arg)+3);
			argstr[0]='"';
			argstr[1]=0;
			strcat(argstr,(const char*)arg);
			strcat(argstr,"\"");
			break;
		case OpcodeArgInt:
			sprintf(tmpbuf,"%li",(long)arg);
			argstr=strdup(tmpbuf);
			break;
		case OpcodeArgFloat:
			conv.i = arg;
			sprintf(tmpbuf,"%f",conv.f);
			argstr=strdup(tmpbuf);
			break;
		case OpcodeArgEnvSym:
			tmp = env_index_to_sym(vm->env,arg);
			printf("OpcodeArgEnvSym #%li : %s\n", arg, tmp);
			argstr=(char*)malloc(strlen(tmp)+2);
			sprintf(argstr,"&%s",tmp);
			/*argstr=strdup("[opcode :: TODO]");*/
			break;
		case OpcodeNoArg:
		default:
			argstr=NULL;
			break;
	};

	if(argstr) {
		buffy = (char*) malloc(22+strlen(argstr));
		sprintf(buffy,"%s  %s",op,argstr);
		free(argstr);
	} else {
		buffy = strdup(op);
	}
	return buffy;
}


void program_dump_stats(program_t p) {
	word_t sz=0,i;
	vm_printf("Program statistics for %p :\n",p);
	vm_printf("    Data size : %lu (%lX)\n",p->data.size,p->data.size);
	vm_printf("    Code size : %lu (%lX)\n",p->code.size,p->code.size);
	for(i=1;i<p->strings.by_index.size;i+=1) {
		sz += 1+strlen((const char*)p->strings.by_index.data[i]);
	}
	vm_printf("    Strings size : %lu strings, %lu bytes (%lX)\n",p->strings.by_index.size-1,sz,sz);
}

