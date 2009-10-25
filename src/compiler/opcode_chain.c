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
#include "opcode_chain.h"
#include "opcode_dict.h"
#include "program.h"
#include "text_seg.h"

#include <string.h>
#include <stdio.h>
#include <dlfcn.h>

opcode_chain_node_t ochain_new_opcode(opcode_arg_t arg_type, const char* opcode, const char* arg,long row,long col) {
	opcode_chain_node_t ocn = (opcode_chain_node_t)malloc(sizeof(struct _opcode_chain_node_t));
	ocn->type=NodeOpcode;
	ocn->arg=NULL;
	//ocn->name=strdup(opcode);
	ocn->arg_type=arg_type;
	//ocn->arg=strdup(opcode);
	ocn->row=row;
	ocn->col=col;
	return ocn;
}

opcode_chain_node_t ochain_new_label(const char* label,long row,long col) {
	opcode_chain_node_t ocn = (opcode_chain_node_t)malloc(sizeof(struct _opcode_chain_node_t));
	ocn->type=NodeLabel;
	ocn->arg=NULL;
	ocn->name=strdup(label);
	ocn->row=row;
	ocn->col=col;
	return ocn;
}

opcode_chain_node_t ochain_new_langdef(const char* nodestr,long row,long col) {
	opcode_chain_node_t ocn = (opcode_chain_node_t)malloc(sizeof(struct _opcode_chain_node_t));
	ocn->type=NodeLangDef;
	ocn->arg=NULL;
	ocn->name=nodestr;
	ocn->row=row;
	ocn->col=col;
	return ocn;
}

opcode_chain_node_t ochain_new_langplug(const char* plugin, const char* plug,long row,long col) {
	opcode_chain_node_t ocn = (opcode_chain_node_t)malloc(sizeof(struct _opcode_chain_node_t));
	ocn->type=NodeLangPlug;
	ocn->arg=strdup(plug);
	ocn->name=strdup(plugin);
	ocn->row=row;
	ocn->col=col;
	return ocn;
}

void opcode_chain_node_free(opcode_chain_node_t ocn) {
	if(ocn->arg) {
		free((char*)ocn->arg);
	}
	free((char*)ocn->name);	/* label is an alias for opcode.name */
	free(ocn);
}


opcode_chain_t opcode_chain_new() {
	opcode_chain_t oc = (opcode_chain_t)malloc(sizeof(struct _slist_t));
	slist_init(oc);
	return oc;
}

opcode_chain_t opcode_chain_add_label(opcode_chain_t oc, const char*label,long row,long col) {
	opcode_chain_node_t ocn = ochain_new_label(label,row,col);
	slist_insert_tail(oc, ocn);
	return oc;
}

void delete_node(ast_node_t);

opcode_chain_t opcode_chain_add_langdef(opcode_chain_t oc, wast_t node,long row,long col) {
	ast_node_t n = make_ast(node);
	const char* str = tinyap_serialize_to_string(n);
	opcode_chain_node_t ocn = ochain_new_langdef(str,row,col);
	delete_node(n);
	slist_insert_tail(oc, ocn);
	return oc;
}

opcode_chain_t opcode_chain_add_langplug(opcode_chain_t oc, const char* plugin, const char*plug,long row,long col) {
	opcode_chain_node_t ocn = ochain_new_langplug(plugin,plug,row,col);
	slist_insert_tail(oc, ocn);
	return oc;
}

opcode_chain_t opcode_chain_add_data(opcode_chain_t oc, vm_data_type_t argtyp, const char* data, const char* rep,long row,long col) {
	const char*repdup;
	/* FIXME : this should be ochain_new_data() */
	opcode_chain_node_t ocn = ochain_new_opcode(argtyp,data,rep,row,col);
	/* FIXME : this should go into ochain_new_data() */
	ocn->type = NodeData;
	ocn->name=strdup(data);
	ocn->arg_type=(opcode_arg_t)argtyp;
	if(rep) {
		repdup=strdup(rep);
	} else {
		repdup=strdup("1");
	}
	ocn->arg=repdup;
	slist_insert_tail(oc, ocn);
	return oc;
}

opcode_chain_t opcode_chain_add_opcode(opcode_chain_t oc, opcode_arg_t argtyp, const char* opcode, const char* arg,long row,long col) {
	const char*argdup;
	opcode_chain_node_t ocn = ochain_new_opcode(argtyp,opcode,arg,row,col);
	/* FIXME : this should go into ochain_new_opcode() */
	ocn->name=strdup(opcode);
	ocn->arg_type=argtyp;
	if(arg) {
		argdup=strdup(arg);
	} else {
		argdup=NULL;
	}
	ocn->arg=argdup;
	slist_insert_tail(oc, ocn);
	return oc;
}

void opcode_chain_apply(opcode_chain_t oc, void(*fun)(opcode_chain_node_t)) {
	slist_forward(oc, opcode_chain_node_t, fun);
}

void opcode_chain_delete(opcode_chain_t oc) {
	if(list_not_empty(oc)) {
		opcode_chain_apply(oc,opcode_chain_node_free);
		slist_flush(oc);
	}
	free(oc);
}

static long count;

static void incr(opcode_chain_node_t ocn) {
	count+=(ocn->type==NodeOpcode);
}

word_t opcode_chain_count(opcode_chain_t oc) {
	count=0;
	opcode_chain_apply(oc,incr);
	return count;
}



word_t opcode_label_to_ofs(opcode_chain_t oc, const char* label) {
	slist_node_t sn;
	opcode_chain_node_t ocn;

	sn = list_head(oc);
	while(sn!=NULL) {
		ocn = node_value(opcode_chain_node_t,sn);
		if(ocn->type==NodeLabel&&!strcmp(ocn->name,label)) {
			return ocn->lofs;
		}
		sn=sn->next;
	}
	return 0;
}


void _VM_CALL vm_op_nop(vm_t vm, word_t data);

/*
 * transforms a symbolic opcode into raw executable wordcode
 */
void opcode_serialize(opcode_dict_t od, opcode_chain_t oc, word_t ip, opcode_chain_node_t ocn, program_t p, void* dl_handle) {
	word_t op = (word_t)opcode_stub_by_name(od,ocn->arg_type, ocn->name);
	word_t arg;
	union { word_t i; tinyaml_float_t f; } conv;
	long skip_wr=0;
	unsigned char c;
	/*char*str;*/
	/*vm_printf("got opcode %s.",ocn->name);*/
	switch(ocn->arg_type) {
	case OpcodeNoArg:
		/*vm_printf("noArg      ");*/
		arg=0;
		break;
	case OpcodeArgInt:
		/*vm_printf("Int   \t(%s)",ocn->arg);*/
		arg=(word_t)atoi(ocn->arg);
		break;
	case OpcodeArgChar:
		/*vm_printf("Char  \t(%s)",ocn->arg);*/
		sscanf(ocn->arg, "%c", &c);
		arg=c;
		break;
	case OpcodeArgFloat:
		/*vm_printf("Float \t(%s)", ocn->arg);*/
		conv.f=(tinyaml_float_t)atof(ocn->arg);
		arg=conv.i;
		break;
	case OpcodeArgString:
		/*vm_printf("String\t(%s)", ocn->arg);*/
		arg = program_find_string(p, ocn->arg);
		if(!strcmp(ocn->name, "__LL__")) {
			text_seg_find_by_text(&p->loadlibs, ocn->arg);
			skip_wr=1;
		} else if(!strcmp(ocn->name, "__RQ__")) {
			text_seg_find_by_text(&p->requires, ocn->arg);
			skip_wr=1;
		}
		break;
	case OpcodeArgLabel:
		/*vm_printf("Label \t(%s)", ocn->arg);*/
		if(ocn->arg[0]=='+'||ocn->arg[0]=='-') {
			arg = atoi(ocn->arg);
		} else {
			arg = opcode_label_to_ofs(oc,ocn->arg) - ip;
		}
		break;
	case OpcodeArgEnvSym:
		arg = (word_t) env_sym_to_index(p->env, ocn->arg);
		if(arg==-1) {
			vm_printerrf("[VM:ERR] Symbol '%s' doesn't exist in environment !\n",ocn->arg);
		}
		/*vm_printf("Opcode\t(%s)", ocn->arg);*/
		/*str = (char*)malloc(strlen(ocn->arg)+8);*/
		/*sprintf(str,"vm_op_%s",ocn->arg);*/
		/*arg = (word_t) dlsym(dl_handle,str);*/
		/*free(str);*/
		/* TODO */
		/*arg=(word_t)-1;*/
		break;
	default:;
		arg=0;
		vm_printf("[VM:ERR] Arg type not supported %X\n",ocn->arg_type);
	};
	/*vm_printf("\tserialized %8.8lX : %8.8lX\n",op,arg);*/
	if(!skip_wr) {
		if(!op) {
			char errbuf[512];
			sprintf(errbuf, "[COMP:ERR] : at %li:%li : illegal opcode %s:%u\n",ocn->row,ocn->col,ocn->name,ocn->arg_type);
			vm_fatal(errbuf);
			op=(word_t)vm_op_nop;
		}
		program_write_code(p,op,arg);
	}
}


word_t str2data(program_t p, vm_data_type_t dt, const char*data) {
	union { word_t i; tinyaml_float_t f; } conv;
	char buf[4];
	switch((vm_data_type_t)dt) {
	case DataChar:
		snprintf(buf, 4, "%s", data);		/* account for \* sequences */
		return (word_t)((unsigned char)buf[0]);
	case DataInt:
		return (word_t)atoi(data);
	case DataFloat:
		conv.f = (tinyaml_float_t) atof(data);
		return conv.i;
	case DataString:
		return program_find_string(p, data);
	default:;
	};
	return 0;
}

void opcode_chain_serialize(opcode_chain_t oc, opcode_dict_t od, program_t p, void* dl_handle) {
	slist_node_t sn;
	opcode_chain_node_t ocn;
	word_t ofs = program_get_code_size(p), backup=ofs, code_sz=0, data_sz=0;

	/*
	 * 1st pass : compute labels addresses and init data segment
	 */
	sn = list_head(oc);
	while(sn!=NULL) {
		ocn = node_value(opcode_chain_node_t,sn);
		switch(ocn->type) {
		case NodeLabel:
			ocn->lofs=ofs;
			program_add_label(p,ofs,ocn->name);
			break;
		case NodeData:
			data_sz+= 2*atoi(ocn->arg);
			/*vm_printf("data rep %i\n",atoi(ocn->arg));*/
			break;
		case NodeOpcode:
			ofs+=2;	/* two words per instruction */
			code_sz+=2;
			break;
		case NodeLangPlug:
		case NodeLangDef:
		default:;
		};
		sn=sn->next;
	}

	/* reserve segments sizes */

	program_reserve_code(p, code_sz);
	program_reserve_data(p, data_sz?2+data_sz:0);

	/*
	 * 2nd pass : serialize opcodes
	 */
	ofs=backup;
	sn = list_head(oc);
	while(sn!=NULL) {
		ocn = node_value(opcode_chain_node_t,sn);
		if(ocn->type==NodeOpcode) {
			opcode_serialize(od,oc,ofs,ocn,p,dl_handle);
			ofs+=2;
		} else if(ocn->type==NodeData) {
			long rep = atoi(ocn->arg);
			word_t dat = str2data(p,(vm_data_type_t)ocn->arg_type,ocn->name);
			dynarray_t data_seg = &p->data;
			while(rep>0) {
				dynarray_set(data_seg,dynarray_size(data_seg), ocn->arg_type);
				dynarray_set(data_seg,dynarray_size(data_seg), dat);
				rep-=1;
			}
		}
		sn=sn->next;
	}
}


