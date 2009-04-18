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

#ifndef _BML_VM_IMPL_H_
#define  _BML_VM_IMPL_H_

#include "vm_types.h"
#include "containers.h"
#include <tinyape.h>


/*! \weakgroup dynarray_t
 * @{
 */
struct _dynarray_t {
	word_t reserved;
	word_t size;
	dynarray_value_t* data;
};
/*@}*/

/*! \weakgroup gstack_t
 * @{
 */
struct _generic_stack_t {
	word_t sz;
	word_t sp;
	word_t tok_sp;
	word_t token_size;
	void* stack;
};
/*@}*/

/*! \weakgroup vm
 * @{
 */
struct _vm_engine_t {
	void(*_VM_CALL _init)(vm_engine_t);
	void(*_VM_CALL _deinit)(vm_engine_t);
	void(*_VM_CALL _run_sync)(vm_engine_t, program_t, word_t ip, word_t prio);
	void(*_VM_CALL _fg_thread_start_cb)(vm_engine_t);
	void(*_VM_CALL _fg_thread_done_cb)(vm_engine_t);
	void(*_VM_CALL _run_async)(vm_engine_t, program_t, word_t ip, word_t prio);
	void(*_VM_CALL _kill)(vm_engine_t);
	void(*_VM_CALL _client_lock)(vm_engine_t);
	void(*_VM_CALL _client_unlock)(vm_engine_t);
	void(*_VM_CALL _vm_lock)(vm_engine_t);
	void(*_VM_CALL _vm_unlock)(vm_engine_t);
	void(*_VM_CALL _thread_failed)(vm_t,thread_t);
	void(*_VM_CALL _debug)(vm_engine_t);
	void(*_VM_CALL _put_std)(const char*);
	void(*_VM_CALL _put_err)(const char*);
	volatile vm_t vm;
};
/*@}*/



/*! \weakgroup compilation
 * @{
 */
struct _opcode_chain_node_t {
	opcode_chain_node_type_t type;
	const char* name;
	const char* arg;
	opcode_arg_t arg_type;
	word_t lofs;
	int row,col;
};
/*@}*/



/*! \weakgroup vm
 * @{
 */
struct _opcode_dict_t {
	struct _dynarray_t stub_by_index[OpcodeTypeMax];
	/* mnemonic lookup */
	struct _hashtab_t stub_by_name[OpcodeTypeMax];
	/* reverse lookup */
	struct _hashtab_t wordcode_by_stub;
	struct _hashtab_t name_by_stub;
};
/*@}*/



/*! \weakgroup symtab_t
 * @{
 */
struct _text_seg_t {
	struct _dynarray_t by_index;
	struct _hashtab_t by_text;
};
/*@}*/

/*! \weakgroup thread_t
 * @{
 */
struct _mutex_t {
	struct _dlist_t pending;
	long int count;
	thread_t owner;
};
/*@}*/

/*! \weakgroup thread_t
 * @{
 */
struct _data_stack_entry_t {
	vm_data_type_t type;
	word_t data;
};
/*@}*/

/*! \weakgroup thread_t
 * @{
 */
struct _call_stack_entry_t {
	program_t cs;
	word_t ip;
	word_t has_closure;	/* FIXME : this should be a set of flags, not just one flag */
	/* FIXME again : has_closure is used to store call_stack.sp in catch stack */
};
/*@}*/


/*! \weakgroup objects
 * @{
 */
struct _vm_obj_t {
	long ref_count;
	void (*_free)(vm_t,void*);
	void* (*_clone)(vm_t,void*);
	word_t magic;
};
/*@}*/

/*! \weakgroup vm_env_t
 * @{
 */
struct _vm_dyn_env_t {
	vm_dyn_env_t parent;
	struct _text_seg_t symbols;
	struct _dynarray_t data;
};
/*@}*/

/*! \weakgroup dyn_func_t
 * @{
 */
struct _vm_dyn_func_t {
	program_t cs;
	word_t ip;
	dynarray_t closure;
};
/*@}*/

/*! \weakgroup vm
 * @{
 */
struct _vm_t {
	/* embedded parser */
	tinyap_t parser;
	/* meta-compiler */
	word_t compile_reent;
	struct _text_seg_t compile_vectors;
	program_t current_edit_prg;
	opcode_chain_t result;
	WalkDirection compile_state;
	wast_t current_node;
	struct _generic_stack_t cn_stack;
	struct _dlist_t init_routines;
	struct _dlist_t term_routines;
	/* compilation error handling */
	struct _generic_stack_t compinput_stack;
	void (*onCompileError) (vm_t vm, const char* input, int is_buffer);
	/* support of virtual AST walkers */
	const char* virt_walker;
	WalkDirection virt_walker_state;
	/* meta-language serialized state */
	struct _text_seg_t gram_nodes;
	/* known opcodes */
	struct _opcode_dict_t opcodes;
	/* library management */
	void* dl_handle;
	struct _slist_t all_handles;
	/* all programs */
	struct _slist_t all_programs;
	struct _hashtab_t loadlibs;
	struct _hashtab_t required;
	/* globals */
	vm_dyn_env_t env;
	struct _data_stack_entry_t exception;
	/* threads */
	scheduler_algorithm_t scheduler;
	word_t threads_count;
	struct _dlist_t ready_threads;
	struct _dlist_t running_threads;
	thread_t current_thread;
	struct _dlist_t yielded_threads;
	struct _dlist_t zombie_threads;
	word_t timeslice;
	/* runtime engine */
	vm_engine_t engine;
	/* stats */
	volatile word_t cycles;
	/* garbage collecting */
	struct _dlist_t gc_pending;
};
/*@}*/



/*! \weakgroup vm_prgs
 * @{
 */
struct _label_tab_t {
	struct _text_seg_t labels;
	struct _dynarray_t offsets;
};
/*@}*/


/*! \weakgroup vm_prgs
 * @{
 */
struct _program_t {
	/* identification */
	const char* source;
	/* globals */
	vm_dyn_env_t env;
	/* segments */
	struct _text_seg_t loadlibs;
	struct _text_seg_t requires;
	struct _text_seg_t strings;
	struct _label_tab_t labels;
	struct _dynarray_t gram_nodes_indexes;
	struct _dynarray_t data;
	struct _dynarray_t code;
};
/*@}*/

/*! \weakgroup thread_t
 * @{
 */
struct _thread_t {
	/* thread is aliased to a dlist_node */
	struct _dlist_node_t sched_data;
	/* attached program */
	volatile program_t program;
	/* execution context */
	word_t data_sp_backup;
	struct _generic_stack_t closures_stack;
	struct _generic_stack_t locals_stack;
	struct _generic_stack_t data_stack;
	struct _generic_stack_t call_stack;
	struct _generic_stack_t catch_stack;
	volatile word_t IP;
	volatile program_t jmp_seg;
	volatile word_t jmp_ofs;
	/* scheduling */
	volatile thread_state_t state;
	/*word_t IP_status;*/
	int _sync;
	word_t prio;
	volatile word_t remaining;
	volatile mutex_t pending_lock;
	struct _mutex_t join_mutex;
	/* registers */
	struct _data_stack_entry_t registers[TINYAML_N_REGISTERS];
};
/*@}*/

/*! \weakgroup vm
 * @{
 */
extern volatile vm_t _glob_vm;
/*@}*/


#endif

