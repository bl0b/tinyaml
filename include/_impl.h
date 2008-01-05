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


struct _dynarray_t {
	word_t reserved;
	word_t size;
	dynarray_value_t* data;
};

struct _generic_stack_t {
	word_t sz;
	word_t sp;
	word_t tok_sp;
	word_t token_size;
	void* stack;
};

struct _vm_engine_t {
	void(*_init)(vm_engine_t);
	void(*_deinit)(vm_engine_t);
	void(*_run_sync)(vm_engine_t, program_t, word_t ip, word_t prio);
	void(*_run_async)(vm_engine_t, program_t, word_t ip, word_t prio);
	void(*_kill)(vm_engine_t);
	vm_t vm;
};



struct _opcode_chain_node_t {
	opcode_chain_node_type_t type;
	const char* name;
	const char* arg;
	opcode_arg_t arg_type;
	word_t lofs;
};



struct _opcode_dict_t {
	struct _dynarray_t stub_by_index[OpcodeTypeMax];
	/* mnemonic lookup */
	struct _hashtab_t stub_by_name[OpcodeTypeMax];
	/* reverse lookup */
	struct _hashtab_t wordcode_by_stub;
	struct _hashtab_t name_by_stub;
};



struct _text_seg_t {
	struct _dynarray_t by_index;
	struct _hashtab_t by_text;
};

struct _mutex_t {
	struct _dlist_t pending;
	long int count;
	thread_t owner;
};

struct _data_stack_entry_t {
	vm_data_type_t type;
	word_t data;
};

struct _call_stack_entry_t {
	program_t cs;
	word_t ip;
};


struct _vm_obj_t {
	long ref_count;
	void (*_free)(vm_t,void*);
	void* (*_clone)(vm_t,void*);
	word_t magic;
};


struct _vm_t {
	/* embedded parser */
	tinyap_t parser;
	/* meta-compiler */
	struct _text_seg_t compile_vectors;
	opcode_chain_t result;
	WalkDirection compile_state;
	wast_t current_node;
	struct _generic_stack_t cn_stack;
	/* meta-language serialized state */
	struct _text_seg_t gram_nodes;
	/* known opcodes */
	struct _opcode_dict_t opcodes;
	/* library management */
	void* dl_handle;
	/* all programs */
	struct _slist_t all_programs;
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
	word_t cycles;
	/* garbage collecting */
	struct _dlist_t gc_pending;
};



struct _label_tab_t {
	struct _text_seg_t labels;
	struct _dynarray_t offsets;
};


struct _program_t {
	struct _text_seg_t strings;
	struct _label_tab_t labels;
	struct _dynarray_t gram_nodes_indexes;
	struct _dynarray_t data;
	struct _dynarray_t code;
};

struct _thread_t {
	/* thread is aliased to a dlist_node */
	struct _dlist_node_t sched_data;
	/* attached program */
	program_t program;
	/* execution context */
	struct _generic_stack_t locals_stack;
	struct _generic_stack_t data_stack;
	struct _generic_stack_t call_stack;
	struct _generic_stack_t catch_stack;
	word_t IP;
	program_t jmp_seg;
	word_t jmp_ofs;
	/* scheduling */
	thread_state_t state;
	word_t IP_status;
	word_t prio;
	word_t remaining;
	mutex_t pending_lock;
	struct _mutex_t join_mutex;
};


#endif

