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

#ifndef _BML_OPCODE_DICT_H_
#define _BML_OPCODE_DICT_H_

#include <stdio.h>

/*! \addtogroup compiler
 * @{
 * \addtogroup opcode_dict Opcode Dictionary
 * @{
 * \brief Opcode dictionary associates opcodes name and arg type, C function, and wordcode.
 */


opcode_dict_t opcode_dict_new();
void opcode_dict_free(opcode_dict_t);
void opcode_dict_init(opcode_dict_t);
void opcode_dict_deinit(opcode_dict_t);

void opcode_dict_add(opcode_dict_t, opcode_arg_t, const char*, opcode_stub_t);

opcode_stub_t opcode_stub_by_name(opcode_dict_t, opcode_arg_t, const char*);
opcode_stub_t opcode_stub_by_code(opcode_dict_t, word_t);
word_t opcode_code_by_stub(opcode_dict_t, opcode_stub_t);
const char* opcode_name_by_stub(opcode_dict_t, opcode_stub_t);

int opcode_dict_link_stubs(opcode_dict_t target, opcode_dict_t src);
int opcode_dict_resolve_stubs(opcode_dict_t src);


void opcode_dict_serialize(opcode_dict_t od, writer_t);
void opcode_dict_unserialize(opcode_dict_t od, reader_t, void*);

opcode_dict_t opcode_dict_optimize(vm_t vm, program_t prog);


/*@}@}*/

#endif

