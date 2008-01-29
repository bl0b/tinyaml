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

#include "../config.h"

/*! \addtogroup misc Miscellaneous
 * @{
 */

#define TINYAML_VERSION PACKAGE_VERSION

/*@}*/

/*! \addtogroup vm Virtual Machine
 * The VM blabla TODO
 * @{
 */

/*! \addtogroup vm_mgmt Management
 * @{
 */

/*! \brief create a new Virtual Machine
 * Actually, the VM is a singleton for internal reasons. But vm_new must be called once.
 */
vm_t vm_new();
/*! destroy the given Virtual Machine */
void vm_del(vm_t);

/*! \brief link an external library file to the VM. Actual file name is dependant on architecture. On Linux, it is $prefix/lib/tinyaml/libtinyaml_&lt;lib_file_name&gt;.so */
vm_t vm_set_lib_file(vm_t, const char*);
/*! \brief declare a new opcode with (name, argument type, C function). */
vm_t vm_add_opcode(vm_t, const char*name, opcode_arg_t, opcode_stub_t);
/*! \brief get the opcode dictionary */
opcode_dict_t vm_get_dict(vm_t);

/*! \brief as it says. For VM analysis. Not much use otherwise. */
opcode_t vm_get_opcode_by_name(vm_t, const char*);

/*@}*/


/*! \addtogroup vm_prgs Programs : reading, compiling, executing
 * @{
 */

/*! Serialize the given program using the given writer (see \ref abstract_io).
 * \note This is a binary serialization.
 */
vm_t vm_serialize_program(vm_t, program_t, writer_t);

/*! \brief Unserialize a program using the given reader (see \ref abstract_io).
 * \note Deserialization is endian-safe.
 */
program_t vm_unserialize_program(vm_t, reader_t);
/*! \brief Compile a file. */
program_t vm_compile_file(vm_t, const char*);
/*! \brief Compile a character string. */
program_t vm_compile_buffer(vm_t, const char*);

/*@}*/

/*! \addtogroup Threads
 * @{
 */

/*! \brief run the given program with priority level \c prio and starting at offset \c ip, and join the thread. */
vm_t vm_run_program_fg(vm_t, program_t, word_t ip, word_t prio);
/*! \brief run the given program with priority level \c prio and starting at offset \c ip, and return immediately. */
vm_t vm_run_program_bg(vm_t, program_t, word_t ip, word_t prio);

/*! \brief start a new thread.
 * Start a new thread with priority level \c prio and starting at offset \c ip in program.
 * The VM engine is signaled the start and death of the thread if \c fg is non-zero.
 */
thread_t vm_add_thread(vm_t, program_t, word_t ip, word_t prio, int fg);

/*! \brief get current thread. */
thread_t vm_get_current_thread(vm_t);

/*! \brief kill a thread (thread resources won't be freed while it's referenced). */
vm_t vm_kill_thread(vm_t,thread_t);

/*@}*/

/*! \addtogroup lolvl At opcode level
 * @{
 */

/*! \brief push data onto current thread's data stack*/
vm_t vm_push_data(vm_t,vm_data_type_t, word_t);
/*! \brief push a caller onto current thread's call stack */
vm_t vm_push_caller(vm_t, program_t, word_t ip, word_t has_closure);
/*! \brief push a catcher onto current thread's catch stack */
vm_t vm_push_catcher(vm_t, program_t, word_t);
/*! \brief peek data at top of current thread's data stack */
vm_t vm_peek_data(vm_t,int,vm_data_type_t*,word_t*);
/*! \brief poke data at top of current thread's data stack */
vm_t vm_poke_data(vm_t,vm_data_type_t,word_t);
/*! \brief peek caller at top of current thread's call stack*/
vm_t vm_peek_caller(vm_t,program_t*,word_t*);
/*! \brief peek catcher at top of current thread's catch stack */
vm_t vm_peek_catcher(vm_t,program_t*,word_t*);
/*! \brief pop from current thread's data stack */
vm_t vm_pop_data(vm_t,word_t);
/*! \brief pop from current thread's call stack */
vm_t vm_pop_caller(vm_t,word_t);
/*! \brief pop from current_thread's catch stack */
vm_t vm_pop_catcher(vm_t,word_t);

/*! \brief get current thread's code segment (program) */
program_t _VM_CALL vm_get_CS(vm_t);
/*! \brief get current thread's IP */
word_t _VM_CALL vm_get_IP(vm_t);

/*! \brief pop data from current thread's data stack */
vm_data_t _VM_CALL _vm_pop(vm_t vm);
/*! \brief peek data on top of current thread's data stack */
vm_data_t _VM_CALL _vm_peek(vm_t vm);

/*@}*/

/*! \addtogroup vm_exec Runtime
 * @{
 */

/*! \brief set the VM's engine. \see engine */
vm_t vm_set_engine(vm_t, vm_engine_t);

/*! \brief run one cycle.
 *
 * Cycle is :
 * - select thread
 * - execute if not NULL
 * - collect one object
 */
void _VM_CALL vm_schedule_cycle(vm_t);

/*@}*/

/*! \internal \brief insert \c o into the VM's collection list */
vm_t _VM_CALL vm_collect(vm_t vm, vm_obj_t o);
/*! \internal \brief remove \c o from the VM's collection list */
vm_t _VM_CALL vm_uncollect(vm_t vm, vm_obj_t o);

/*@}*/

#endif

