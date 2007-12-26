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
#include <tinyap.h>

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
	void(*_run)(vm_engine_t, program_t, word_t);
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
	struct _slist_t pending;
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

struct _vm_t {
	/* embedded parser */
	tinyap_t parser;
	/* known opcodes */
	struct _opcode_dict_t opcodes;
	/* library management */
	void* dl_handle;
	/* threads */
	scheduler_algorithm_t scheduler;
	word_t threads_count;
	struct _dlist_t ready_threads;
	struct _dlist_t running_threads;
	dlist_node_t current_thread;
	struct _dlist_t yielded_threads;
	word_t timeslice;
	/* runtime engine */
	vm_engine_t engine;
	/* stats */
	word_t cycles;
};


struct _program_t {
	struct _text_seg_t strings;
	struct _dynarray_t code;
};

struct _thread_t {
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
};


#endif

