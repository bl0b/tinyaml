#ifndef _BML_VM_H_
#define _BML_VM_H_

#include "vm_types.h"
#include "containers.h"

#include "abstract_io.h"

#include "code.h"

#include "thread.h"

vm_t vm_new();
void vm_del(vm_t);

vm_t vm_serialize_program(vm_t, program_t, writer_t);

program_t vm_unserialize_program(vm_t, reader_t);
program_t vm_compile_file(vm_t, const char*);
program_t vm_compile_buffer(vm_t, const char*);

vm_t vm_run_program(vm_t, program_t, word_t);


vm_t vm_set_lib_file(vm_t, const char*);
vm_t vm_add_opcode(vm_t, const char*name, opcode_arg_t, opcode_stub_t);
opcode_dict_t vm_get_dict(vm_t);

opcode_t vm_get_opcode_by_name(vm_t, const char*);

vm_t vm_add_thread(vm_t, program_t, word_t ip, word_t prio);

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

vm_t vm_schedule_cycle(vm_t);


#endif

