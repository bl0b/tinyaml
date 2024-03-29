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

#ifndef _BML_OPCODE_CHAIN_H_
#define _BML_OPCODE_CHAIN_H_

/*! \addtogroup compil_ch
 * \brief Opcode chain symbolically describes a program's contents.
 * @{
 */


opcode_chain_t opcode_chain_new();
opcode_chain_t opcode_chain_add_label(opcode_chain_t, const char*, long r, long c);
opcode_chain_t opcode_chain_add_opcode(opcode_chain_t, opcode_arg_t, const char* opcode, const char* arg, long r, long c);
opcode_chain_t opcode_chain_add_data(opcode_chain_t, vm_data_type_t, const char* opcode, const char* arg, long r, long c);
opcode_chain_t opcode_chain_add_langdef(opcode_chain_t oc, wast_t node, long r, long c);
opcode_chain_t opcode_chain_add_langplug(opcode_chain_t oc, const char* plugin, const char*plug, long r, long c);

void opcode_chain_delete(opcode_chain_t);

void opcode_chain_serialize(opcode_chain_t, opcode_dict_t, program_t, void*);

void opcode_chain_apply(opcode_chain_t oc, void(*fun)(opcode_chain_node_t));

/*@}*/

#endif

