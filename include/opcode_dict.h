#ifndef _BML_OPCODE_DICT_H_
#define _BML_OPCODE_DICT_H_

#include <stdio.h>

void opcode_dict_init(opcode_dict_t);

void opcode_dict_add(opcode_dict_t, opcode_arg_t, const char*, opcode_stub_t);

opcode_stub_t opcode_stub_by_name(opcode_dict_t, opcode_arg_t, const char*);
opcode_stub_t opcode_stub_by_code(opcode_dict_t, word_t);
word_t opcode_code_by_stub(opcode_dict_t, opcode_stub_t);
const char* opcode_name_by_stub(opcode_dict_t, opcode_stub_t);

int opcode_dict_link_stubs(opcode_dict_t target, opcode_dict_t src);
int opcode_dict_resolve_stubs(opcode_dict_t src);


void opcode_dict_serialize(opcode_dict_t od, writer_t);
void opcode_dict_unserialize(opcode_dict_t od, reader_t, void*);



#endif

