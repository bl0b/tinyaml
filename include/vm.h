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

#include "config.h"

#include "vm_types.h"
#include "containers.h"
#include "abstract_io.h"
#include "code.h"
#include "thread.h"

/*! \addtogroup _pad _
 * F***ing bug in doxygen's grouping... nevermind.
 */

/*! \defgroup Tinyaml Tinyaml
 * @{
 */
/*! \defgroup abstract_io Tinyaml file and buffer IO
 * @{
 * \brief Define a common interface for file and memory buffer I/O, independantly of non-portable fmemopen.
 *
 * Readers and Writers in Tinyaml can handle 32-bit words and character strings. A reader can be configured to swap bytes when reading words, so that it can read words serialized in the other endianness. Endianness recognition is done by the user, NOT by the Reader itself.
 *
 * Internals are not discussed here.
 * @}
 */
/*! \defgroup data_struc Data Structures and Representations
 * @{
 * 	\defgroup data_containers Base containers
 * 	@{
 *	\brief The containers used everywhere in Tinyaml.
 *
 * 		\defgroup dynarray_t Dynamic Array
 * 		\defgroup gstack_t Generic stack
 * 		\defgroup list_t Chained lists
 * 		\defgroup hashtab_t Hashtable
 * 		\defgroup symtab_t Text segment / Symbol table
 * 		\defgroup vm_env_t Environment/Map
 * 	\defgroup objects Managed Objects
 * 	@{
 * 	\brief Buffers with reference counting and automated cloning and collection.
 *
 * 	Managed objects at low-level are \c void* buffers with a prefix. To create a managed buffer,
 * 	one must provide the buffer size and the following two routines : \c _clone() and \c _deinit().
 *
 * 	\c _clone() should duplicate the buffer given as a parameter and \c _deinit() should free the resources associated with the buffer, but not the buffer itself.
 *
 * 	Once a buffer is created, the \ref vm manages its reference counter and automatically collects unreferenced buffers.
 *
 * 	Buffers are to be \c _clone 'd when enclosing their reference in \ref dyn_func_t.
 *
 * 	\section Garbage Collector
 * 		Currently, the GC collects at most one buffer per instruction cycle and has not been tested under heavy load.
 *
 * 		This might change anytime in function of the needs.
 * 	@}
 * 	@}
 * 	\weakgroup _pad
 * @}
 */
/*! \defgroup vm Virtual Machine
 * @{
 * 	\defgroup vm_mgmt Management
 * 	\defgroup vm_engine Engine
 * 	\defgroup compilation Compiler
 * 	@{
 * 		\defgroup compil_ch Opcode chain
 * 		\defgroup opcode_dict Opcode dictionary
 * 	\defgroup vm_prgs Programs : reading, compiling, executing
 * 	@}
 * 	\defgroup Threads Threads
 * 	\defgroup dyn_func_t Function objects (dynamic functions)
 * 	\defgroup lolvl At opcode level
 * 	\weakgroup _pad
 * @}
 */
/*! \defgroup misc Miscellaneous
 * @{
 * 	\defgroup vm_assert Assertions
 * 	\defgroup fast_math Fast 32-bit maths
 * 	\weakgroup _pad
 * @}
 */
/*! \defgroup vm_core_ops Core Opcodes
 * @{
 * 	\defgroup vcop_data Handling data
 * 	@{
 * 		\defgroup vcop_mem Memory operations
 * 		\defgroup vcop_da Arrays
 * 		\defgroup vcop_stack Stacks
 * 		\defgroup vcop_st Symtabs
 * 		\defgroup vcop_map Maps
 * 		\defgroup vcop_df Function objects
 * 	\defgroup vcop_ctrl Control flow
 * 	@}
 * 	\defgroup vcop_str String operations
 * 	\defgroup vcop_thrd Threading
 * 	\defgroup vcop_comp Compiling
 * 	\defgroup vcop_arit Arithmetic & bitwise operations
 * 	\weakgroup _pad
 * @}
 * @}
 */
/*! \defgroup tinyaml_dbg Debugger
 * @{
 * 	\defgroup dbg_internals Tinyaml renderers
 * 	\defgroup dbg_ncurses NCurses
 * 	\defgroup debug_engine Synchronous debug engine
 * @}
 */

/*! \addtogroup misc
 * @{
 */

#define TINYAML_VERSION PACKAGE_VERSION

/*@}*/

/*! \addtogroup vm
 * The VM blabla TODO
 * @{
 */

/*! \addtogroup vm_mgmt
 * @{
 */

/*! \brief create a new Virtual Machine
 * Actually, the VM is a singleton for internal reasons. But vm_new must be called once.
 */
vm_t vm_new();
/*! destroy the given Virtual Machine */
void vm_del(vm_t);

vm_t vm_set_error_handler(vm_t vm, vm_error_handler handler);
vm_error_handler vm_get_error_handler(vm_t vm);

/*! \brief link an external library file to the VM. Actual file name is dependant on architecture. On Linux, it is $prefix/lib/tinyaml/libtinyaml_&lt;lib_file_name&gt;.so */
vm_t vm_set_lib_file(vm_t, const char*);
/*! \brief declare a new opcode with (name, argument type, C function). */
vm_t vm_add_opcode(vm_t, const char*name, opcode_arg_t, opcode_stub_t);
/*! \brief get the opcode dictionary */
opcode_dict_t vm_get_dict(vm_t);

/*! \brief as it says. For VM analysis. Not much use otherwise. */
opcode_t vm_get_opcode_by_name(vm_t, const char*);

/*@}*/


/*! \addtogroup vm_prgs
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

/*! \brief Compile a file (append to the current program). */
program_t vm_compile_append_file(vm_t, const char*, word_t*, long);
/*! \brief Compile a character string (append to the current program). */
program_t vm_compile_append_buffer(vm_t, const char*, word_t*, long);

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
thread_t vm_add_thread(vm_t, program_t, word_t ip, word_t prio, long fg);

/*! \brief get current thread. */
thread_t vm_get_current_thread(vm_t);

/*! \brief kill a thread (thread resources won't be freed while it's referenced). */
vm_t vm_kill_thread(vm_t,thread_t);


/*! \brief set the timeslice for thread scheduling. */
void vm_set_timeslice(vm_t vm, long timeslice);


/*@}*/

/*! \addtogroup lolvl
 * @{
 */

/*! \brief push data onto current thread's data stack*/
vm_t vm_push_data(vm_t,vm_data_type_t, word_t);
/*! \brief push a caller onto current thread's call stack */
vm_t vm_push_caller(vm_t, program_t, word_t ip, word_t has_closure, vm_dyn_func_t df_callee);
/*! \brief push a catcher onto current thread's catch stack */
vm_t vm_push_catcher(vm_t, program_t, word_t);
/*! \brief peek data at top of current thread's data stack */
vm_t vm_peek_data(vm_t,long,vm_data_type_t*,word_t*);
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

/*! \addtogroup vm_engine
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

/*! \brief output formatted data to VM's stdout. */
long vm_printf(const char* fmt, ...);
/*! \brief output formatted data to VM's stderr. */
long vm_printerrf(const char* fmt, ...);
/*@}*/

thread_t vm_exec_dynFun(vm_t, vm_dyn_func_t);

#define TINYAML_SHEBANG "#!/usr/bin/env tinyaml\nBML_PRG"


void vm_compinput_push_file(vm_t vm, const char*filename);
void vm_compinput_push_buffer(vm_t vm, const char*buffer);
void vm_compinput_push_walker(vm_t vm, const char*wname);
void vm_compinput_pop(vm_t vm);


#endif

