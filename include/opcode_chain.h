#ifndef _BML_OPCODE_CHAIN_H_
#define _BML_OPCODE_CHAIN_H_


opcode_chain_t opcode_chain_new();
opcode_chain_t opcode_chain_add_label(opcode_chain_t, const char*);
opcode_chain_t opcode_chain_add_opcode(opcode_chain_t, opcode_arg_t, const char* opcode, const char* arg);

void opcode_chain_delete(opcode_chain_t);

void opcode_chain_serialize(opcode_chain_t, opcode_dict_t, program_t);

#endif

