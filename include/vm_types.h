/* TinyaML
 * Copyright (C) 2007 Damien Leroux
 *
 * This program is free software;
 * you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation;
 * either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY;
 * without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program;
 * if not, write to the Free Software
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

/*! \weakgroup vm
 * @{
 */
/*! \brief An instance of the Virtual Machine. */
typedef struct _vm_t* vm_t;
/*@}*/

/*! \weakgroup vm
 * @{
 */
/*! \brief An entry of the the VM's opcode dictionary. */
typedef struct _opcode_t* opcode_t;

typedef void (*vm_error_handler)(vm_t, const char*, int);

/*@}*/

/*! \weakgroup vm_prgs
 * @{
 */
/*! \brief An instance of a program. */
typedef struct _program_t* program_t;
/*@}*/

/*! \weakgroup data_struc Data Structures and Representations
 * @{
 */

/*! \brief The basic processing unit. */
typedef unsigned long int word_t;
/*@}*/

/*! \deprecated \brief Just an alias. */
typedef word_t value_t;
/*@}*/

/*@}*/

/*! \weakgroup objects
 * @{
 */
/*! \brief A managed object. */
typedef struct _vm_obj_t* vm_obj_t;
/*@}*/
/*! \weakgroup objects
 * @{
 */
/*! \brief A map */
typedef struct _vm_dyn_env_t* vm_dyn_env_t;
/*@}*/
/*! \weakgroup objects
 * @{
 */
/*! \brief A function object. */
typedef struct _vm_dyn_func_t* vm_dyn_func_t;
/*@}*/


/*! \weakgroup dynarray_t
 * @{
 */
/*! \brief An instance of a dynamic array of words. */
typedef struct _dynarray_t* dynarray_t;
/*! \brief Type for dynarray indices. */
typedef word_t dynarray_index_t;
/*! \brief Type for dynarray values. */
typedef word_t dynarray_value_t;
/*@}*/

/*! \weakgroup gstack_t
 * @{
 */
/*! \brief An instance of a generic stack. */
typedef struct _generic_stack_t* generic_stack_t;
/*@}*/

/*! \weakgroup dynarray_t
 * @{
 */
/*! \brief A code segment. */
typedef dynarray_t code_seg_t;
/*@}*/

/*! \weakgroup symtab_t
 * @{
 */
/*! \brief A text segment (holds static strings). */
typedef struct _text_seg_t* text_seg_t;
/*@}*/

/*! \weakgroup vm
 * @{
 */
/*! \brief Valid Opcode Arguments. */
typedef enum {
	OpcodeNoArg=0,		/*!< No argument */
	OpcodeArgInt,		/*!< 32-bit signed integer */
	OpcodeArgFloat,		/*!< 32-bit floating point */
	OpcodeArgPtr,		/*!< \deprecated unused. */
	OpcodeArgLabel,		/*!< Label */
	OpcodeArgString,	/*!< Static string */
	OpcodeArgEnvSym,	/*!< Global symbol */
	OpcodeTypeMax		/*!< Boundary \note This is not a valid type. */
} opcode_arg_t;

typedef enum {
	CompInNone=0,
	CompInFile,
	CompInBuffer,
	CompInVWalker,
	CompInMax
} compinput_t;
/*@}*/

/*! \weakgroup thread_t
 * @{
 */
/*! \brief Thread states. */
typedef enum {
	ThreadBlocked,
	ThreadReady,
	ThreadRunning,
	ThreadDying,
	ThreadZombie,
	ThreadStateMax		/*!< Boundary. \note This is not a valid thread state. \note This non-state is used during thread pre-initialization. */
} thread_state_t;
/*@}*/

/*! \weakgroup mutex_t
 * @{
 */
typedef struct _mutex_t* mutex_t;
/*@}*/
/*! \weakgroup thread_t
 * @{
 */
typedef struct _thread_t* thread_t;
/*@}*/


/*! \weakgroup vm
 * @{
 */
typedef enum {
	SchedulerIdle=0,
	SchedulerMonoThread,
	SchedulerRoundRobin,

	SchedulerAlgoMax
} scheduler_algorithm_t;
/*@}*/

/*! \weakgroup vm
 * @{
 */
typedef struct _vm_engine_t* vm_engine_t;
/*@}*/

/*! \weakgroup vm
 * @{
 */
typedef struct _opcode_dict_t* opcode_dict_t;
/*@}*/

/*! \weakgroup vm
 * @{
 */
typedef enum {
	DataNone=0,
	DataInt=1,			/*!< 32-bit integer */
	DataFloat=2,			/*!< 32-bit floating point */
	DataString=OpcodeArgString,	/*!< static string */

	DataManagedObjectFlag=0x100,	/*!< Managed object types have this bit set */
	DataObjStr,			/*!< Managed string */
	DataObjSymTab,			/*!< Managed \ref symtab_t "symbol table" */
	DataObjMutex,			/*!< Managed \ref mutex_t "mutex" */
	DataObjThread,			/*!< Managed \ref thread_t "thread" */
	DataObjArray,			/*!< Managed \ref dynarray_t "dynamic array" */
	DataObjEnv,			/*!< Managed \ref vm_env_t "env" */
	DataObjStack,			/*!< Managed \ref generic_stack_t "stack" */
	DataObjFun,			/*!< Managed \ref dyn_func_t "function object" */
	DataObjVObj,			/*!< Reserved for object model management. VObj should be a dynarray with a header. */
	DataObjUser,			/*!< Managed user object. (meant for extensions) */

	DataTypeMax,			/*!< Boundary \note This is not a valid type. */
} vm_data_type_t;
/*@}*/

/*! \weakgroup compilation
 * @{
 */
/*! \brief A chain of symbolic program elements. */
typedef struct _slist_t* opcode_chain_t;
/*! \brief An atomic symbolic program element. */
typedef struct _opcode_chain_node_t* opcode_chain_node_t;
/*! \brief Types of symbolic elements in a program. */
typedef enum {
	NodeOpcode,	/*!< Element is an instruction. */
	NodeLangDef,	/*!< Element is a serialized Grammar AST. */
	NodeLangPlug,	/*!< Element is a \ref ml_ml "plug statement". */
	NodeData,	/*!< Element is a static data initializer. */
	NodeLabel	/*!< Element is a label. */
} opcode_chain_node_type_t;
/*@}*/


/*! \weakgroup vm_blocker_t
 * @{
 */
/*! \brief An instance of a thread blocker. */
typedef generic_stack_t vm_blocker_t;
/*@}*/

/*! \weakgroup vm
 * @{
 */
/*! \brief One piece of data. */
typedef struct _data_stack_entry_t* vm_data_t;
/*@}*/
/*! \weakgroup thread_t
 * @{
 */
/*! \brief A caller. */
typedef struct _call_stack_entry_t* call_stack_entry_t;
/*@}*/
/*! \weakgroup thread_t
 * @{
 */
/*! \brief A catcher. */
typedef struct _call_stack_entry_t* catch_stack_entry_t;
/*@}*/

/*! \weakgroup vm
 * @{
 */
/*! \brief Signature for all C opcode routines. */
typedef void _VM_CALL (*opcode_stub_t) (vm_t, word_t t);
/*@}*/

/*! \brief Bitsize of argument_type field in wordcode. */
#define _WC_ARGTYPE_BSZ 4

/*! \brief Bitsize of opcode_index field in wordcode. */
#define _WC_OP_BSZ (32-_WC_ARGTYPE_BSZ)
/*! \brief Mask of opcode_index field in wordcode. */
#define _WC_OP_MASK ((1<<_WC_OP_BSZ)-1)
/*! \brief Shift of opcode_index field in wordcode. */
#define _WC_ARGTYPE_SHIFT _WC_OP_BSZ

/*! \brief Get opcode_index from wordcode */
#define WC_GET_OP(_wc) (((word_t)(_wc))&_WC_OP_MASK)
/*! \brief Get argument_type from wordcode */
#define WC_GET_ARGTYPE(_wc) (((word_t)(_wc))>>_WC_ARGTYPE_SHIFT)

/*! \brief Build wordcode from argument_type and opcode_index */
#define MAKE_WC(_a,_op) (((_a)<<_WC_ARGTYPE_SHIFT)|(_op))


#endif

