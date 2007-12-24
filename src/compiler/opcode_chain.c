#include "_impl.h"
#include "vm.h"
#include "opcode_chain.h"
#include "opcode_dict.h"
#include "program.h"

#include <string.h>
#include <stdio.h>

opcode_chain_node_t ochain_new_opcode(opcode_arg_t arg_type, const char* opcode, const char* arg) {
	opcode_chain_node_t ocn = (opcode_chain_node_t)malloc(sizeof(opcode_chain_node_t));
	ocn->type=NodeOpcode;
	ocn->node.opcode.name=strdup(opcode);
	ocn->node.opcode.arg_type=arg_type;
	ocn->node.opcode.arg=strdup(opcode);
	return ocn;
}

opcode_chain_node_t ochain_new_label(const char* label) {
	opcode_chain_node_t ocn = (opcode_chain_node_t)malloc(sizeof(opcode_chain_node_t));
	ocn->type=NodeLabel;
	ocn->node.label=strdup(label);
	return ocn;
}

void opcode_chain_node_free(opcode_chain_node_t ocn) {
	if(ocn->type==NodeOpcode) {
		free((char*)ocn->node.opcode.arg);
	}
	free((char*)ocn->node.label);	/* label is an alias for opcode.name */
	free(ocn);
}


opcode_chain_t opcode_chain_new() {
	opcode_chain_t oc = (opcode_chain_t)malloc(sizeof(struct _slist_t));
	slist_init(oc);
	return oc;
}

opcode_chain_t opcode_chain_add_label(opcode_chain_t oc, const char*label) {
	opcode_chain_node_t ocn = ochain_new_label(label);
	slist_insert_tail(oc, ocn);
	return oc;
}

opcode_chain_t opcode_chain_add_opcode(opcode_chain_t oc, opcode_arg_t argtyp, const char* opcode, const char* arg) {
	const char*argdup;
	opcode_chain_node_t ocn = ochain_new_opcode(argtyp,opcode,arg);
	ocn->node.opcode.name=strdup(opcode);
	ocn->node.opcode.arg_type=argtyp;
	if(arg) {
		argdup=strdup(arg);
	} else {
		argdup=NULL;
	}
	ocn->node.opcode.arg=argdup;
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
}

static int count;

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
		if(ocn->type==NodeLabel&&!strcmp(ocn->node.label,label)) {
			return ocn->lofs;
		}
		sn=sn->next;
	}
	return 0;
}

/*
 * transforms a symbolic opcode into raw executable wordcode
 */
void opcode_serialize(opcode_dict_t od, opcode_chain_t oc, word_t ip, opcode_chain_node_t ocn, program_t p) {
	word_t op = (word_t)opcode_stub_by_name(od,ocn->node.opcode.arg_type, ocn->node.opcode.name);
	word_t arg;
	float tmp;
	printf("got opcode %s.",ocn->node.opcode.name);
	switch(ocn->node.opcode.arg_type) {
	case OpcodeNoArg:
		printf("noArg      ");
		arg=0;
		break;
	case OpcodeArgInt:
		printf("Int   \t(%s)",ocn->node.opcode.arg);
		arg=(word_t)atoi(ocn->node.opcode.arg);
		break;
	case OpcodeArgFloat:
		printf("Float \t(%s)", ocn->node.opcode.arg);
		tmp=(float)atof(ocn->node.opcode.arg);
		arg=*(word_t*)(&tmp);
		break;
	case OpcodeArgString:
		printf("String\t(%s)", ocn->node.opcode.arg);
		arg = program_find_string(p, ocn->node.opcode.arg);
		break;
	case OpcodeArgLabel:
		printf("Label \t(%s)", ocn->node.opcode.arg);
		arg = opcode_label_to_ofs(oc,ocn->node.opcode.arg) - ip;
		break;
	case OpcodeArgOpcode:
		printf("Opcode\t(%s)", ocn->node.opcode.arg);
		/* TODO */
		arg=(word_t)-1;
		break;
	default:;
		printf("[ERROR:VM] Arg type not supported %X\n",ocn->node.opcode.arg_type);
	};
	printf("\tserialized %8.8lX : %8.8lX\n",op,arg);
	program_write_code(p,op,arg);
}


void opcode_chain_serialize(opcode_chain_t oc, opcode_dict_t od, program_t p) {
	slist_node_t sn;
	opcode_chain_node_t ocn;
	word_t ofs = program_get_code_size(p), backup=ofs;

	program_reserve_code(p, 2*opcode_chain_count(oc));

	/*
	 * 1st pass : compute labels addresses
	 */
	sn = list_head(oc);
	while(sn!=NULL) {
		ocn = node_value(opcode_chain_node_t,sn);
		if(ocn->type==NodeLabel) {
			ocn->lofs=ofs;
		} else {
			ofs+=2;	/* two words per instruction */
		}
		sn=sn->next;
	}
	/*
	 * 2nd pass : serialize opcodes
	 */
	ofs=backup;
	sn = list_head(oc);
	while(sn!=NULL) {
		ocn = node_value(opcode_chain_node_t,sn);
		if(ocn->type==NodeOpcode) {
			opcode_serialize(od,oc,ofs,ocn,p);
			ofs+=2;
		}
		sn=sn->next;
	}
}


