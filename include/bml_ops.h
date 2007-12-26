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

#ifndef _BML_OPS_H_
#define _BML_OPS_H_

void vm_op_push_Int(vm_t vm, word_t data);
void vm_op_push_Float(vm_t vm, word_t data);
void vm_op_push_String(vm_t vm, word_t data);
void vm_op_push_Opcode(vm_t vm, word_t data);

void vm_op_pop(vm_t vm, word_t data);
void vm_op_pop_Int(vm_t vm, word_t data);

void vm_op_dup_Int(vm_t vm, word_t data);

void vm_op_nop(vm_t vm, word_t data);

void vm_op_ST(vm_t vm, word_t data);

void vm_op_SF(vm_t vm, word_t data);

void vm_op_jmp_Label(vm_t vm, word_t data);

void vm_op_print_String(vm_t vm, word_t data);

#endif

