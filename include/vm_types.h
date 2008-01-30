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


#ifndef _BML_VM_TYPES_H_
#define _BML_VM_TYPES_H_

#if defined(__GNUC__)
#define _VM_CALL __attribute__((fastcall))
#elif defined(WIN32)||defined(__MSVC__)
#define _VM_CALL __fastcall
#else
#warning "using cdecl"
#define _VM_CALL
#endif

/*! \addtogroup vm */
typedef struct _vm_t* vm_t;

/*! \addtogroup vm */
typedef struct _opcode_t* opcode_t;

/*! \addtogroup vm_prgs */
typedef struct _program_t* program_t;

/*! \addtogroup data_struc Data Structures and Representations
 * @{
 */

typedef unsigned long int word_t;

typedef word_t value_t;

/*@}*/

/*! \addtogroup objects */
/*! \brief A managed object. */
typedef struct _vm_obj_t* vm_obj_t;
/*! \addtogroup objects */
/*! \brief A map */
typedef struct _vm_dyn_env_t* vm_dyn_env_t;
/*! \addtogroup objects */
/*! \brief A function object. */
typedef struct _vm_dyn_func_t* vm_dyn_func_t;


/*! \addtogroup dynarray_t
 * @{
 */
typedef struct _dynarray_t* dynarray_t;
typedef word_t dynarray_index_t;
typedef word_t dynarray_value_t;
/*@}*/

/*! \addtogroup gstack_t */
typedef struct _generic_stack_t* generic_stack_t;

typedef dynarray_t code_seg_t;
/*! \addtogroup symtab_t */
typedef struct _text_seg_t* text_seg_t;

typedef enum {
	OpcodeNoArg=0,
	OpcodeArgInt,
	OpcodeArgFloat,
	OpcodeArgPtr,
	OpcodeArgLabel,
	OpcodeArgString,
	OpcodeArgEnvSym,

	OpcodeTypeMax
} opcode_arg_t;

typedef enum {
	ThreadBlocked,
	ThreadReady,
	ThreadRunning,
	ThreadDying,
	ThreadZombie,

	ThreadStateMax
} thread_state_t;

typedef struct _mutex_t* mutex_t;
typedef struct _thread_t* thread_t;


typedef enum {
	SchedulerIdle=0,
	SchedulerMonoThread,
	SchedulerRoundRobin,

	SchedulerAlgoMax
} scheduler_algorithm_t;

typedef struct _vm_engine_t* vm_engine_t;

typedef struct _opcode_dict_t* opcode_dict_t;

typedef enum {
	DataInt=0,
	DataFloat=1,
	DataString=OpcodeArgString,

	DataManagedObjectFlag=0x100,
	DataObjStr,
	DataObjSymTab,
	DataObjMutex,
	DataObjThread,
	DataObjArray,
	DataObjEnv,
	DataObjStack,
	DataObjFun,
	DataObjVObj,
	DataObjUser,

	DataTypeMax,
} vm_data_type_t;

typedef struct _slist_t* opcode_chain_t;
typedef struct _opcode_chain_node_t* opcode_chain_node_t;

typedef enum {
	NodeOpcode,
	NodeLangDef,
	NodeLangPlug,
	NodeData,
	NodeLabel
} opcode_chain_node_type_t;


typedef generic_stack_t vm_blocker_t;

typedef struct _data_stack_entry_t* vm_data_t;
typedef struct _call_stack_entry_t* call_stack_entry_t;
typedef struct _call_stack_entry_t* catch_stack_entry_t;

typedef void _VM_CALL (*opcode_stub_t) (vm_t, word_t t);

#define _WC_ARGTYPE_BSZ 4

#define _WC_OP_BSZ (32-_WC_ARGTYPE_BSZ)
#define _WC_OP_MASK ((1<<_WC_OP_BSZ)-1)
#define _WC_ARGTYPE_SHIFT _WC_OP_BSZ

#define WC_GET_OP(_wc) (((word_t)(_wc))&_WC_OP_MASK)
#define WC_GET_ARGTYPE(_wc) (((word_t)(_wc))>>_WC_ARGTYPE_SHIFT)

#define MAKE_WC(_a,_op) (((_a)<<_WC_ARGTYPE_SHIFT)|(_op))


#endif

