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

#ifndef _BML_VM_H_
#define _BML_VM_H_

#include "vm_types.h"
#include "containers.h"
#include "abstract_io.h"
#include "code.h"
#include "thread.h"

#define TINYAML_VERSION "0.1"

vm_t vm_new();
void vm_del(vm_t);

vm_t vm_serialize_program(vm_t, program_t, writer_t);

program_t vm_unserialize_program(vm_t, reader_t);
program_t vm_compile_file(vm_t, const char*);
program_t vm_compile_buffer(vm_t, const char*);

vm_t vm_run_program_fg(vm_t, program_t, word_t ip, word_t prio);
vm_t vm_run_program_bg(vm_t, program_t, word_t ip, word_t prio);


vm_t vm_set_lib_file(vm_t, const char*);
vm_t vm_add_opcode(vm_t, const char*name, opcode_arg_t, opcode_stub_t);
opcode_dict_t vm_get_dict(vm_t);

opcode_t vm_get_opcode_by_name(vm_t, const char*);

thread_t vm_add_thread(vm_t, program_t, word_t ip, word_t prio);

thread_t vm_get_thread(vm_t, word_t);
word_t vm_get_current_thread_index(vm_t);
thread_t vm_get_current_thread(vm_t);

vm_t vm_kill_thread(vm_t,thread_t);


vm_t vm_push_data(vm_t,vm_data_type_t, word_t);
vm_t vm_push_caller(vm_t, program_t, word_t);
vm_t vm_push_catcher(vm_t, program_t, word_t);
vm_t vm_peek_data(vm_t,int,vm_data_type_t*,word_t*);
vm_t vm_poke_data(vm_t,vm_data_type_t,word_t);
vm_t vm_peek_caller(vm_t,program_t*,word_t*);
vm_t vm_peek_catcher(vm_t,program_t*,word_t*);
vm_t vm_pop_data(vm_t,word_t);
vm_t vm_pop_caller(vm_t,word_t);
vm_t vm_pop_catcher(vm_t,word_t);

vm_t vm_set_engine(vm_t, vm_engine_t);

void _VM_CALL vm_schedule_cycle(vm_t);

program_t _VM_CALL vm_get_CS(vm_t);
word_t _VM_CALL vm_get_IP(vm_t);

vm_t _VM_CALL vm_collect(vm_t vm, vm_obj_t o);
vm_t _VM_CALL vm_uncollect(vm_t vm, vm_obj_t o);

vm_data_t _VM_CALL _vm_pop(vm_t vm);

#endif

