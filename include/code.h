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


#ifndef _BML_CODE_H_
#define _BML_CODE_H_


typedef void _VM_CALL (*opcode_noarg_t) (vm_t, word_t);
typedef void _VM_CALL (*opcode_int_t) (vm_t, long);
typedef void _VM_CALL (*opcode_float_t) (vm_t, float);
typedef void _VM_CALL (*opcode_ptr_t) (vm_t, void*);
typedef void _VM_CALL (*opcode_label_t) (vm_t, long);
typedef void _VM_CALL (*opcode_string_t) (vm_t, const char*);
typedef void _VM_CALL (*opcode_opcode_t) (vm_t, word_t);

void vm_compile_noarg(vm_t, opcode_t);
void vm_compile_int(vm_t, opcode_t);
void vm_compile_float(vm_t, opcode_t);
void vm_compile_ptr(vm_t, opcode_t);
void vm_compile_label(vm_t, opcode_t);
void vm_compile_string(vm_t, opcode_t);
void vm_compile_opcode(vm_t, opcode_t);




struct _opcode_t {
	const char* name;
	opcode_arg_t arg_type;
	opcode_stub_t exec;
};


#endif

