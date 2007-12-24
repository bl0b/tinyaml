
#ifndef _BML_VM_TYPES_H_
#define _BML_VM_TYPES_H_

typedef unsigned long int word_t;

typedef word_t value_t;

typedef struct _vm_t* vm_t;

typedef struct _program_t* program_t;
typedef struct _opcode_t* opcode_t;

typedef struct _dynarray_t* dynarray_t;
typedef word_t dynarray_index_t;
typedef word_t dynarray_value_t;


typedef dynarray_t code_seg_t;
typedef struct _text_seg_t* text_seg_t;

typedef enum {
	OpcodeNoArg=0,
	OpcodeArgInt,
	OpcodeArgFloat,
	OpcodeArgPtr,
	OpcodeArgLabel,
	OpcodeArgString,
	OpcodeArgOpcode,

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
	DataFloat,
	RefString,
	RefObject
} vm_data_type_t;

typedef struct _slist_t* opcode_chain_t;
typedef struct _opcode_chain_node_t* opcode_chain_node_t;

typedef enum {
	NodeOpcode,
	NodeLabel
} opcode_chain_node_type_t;


typedef struct _call_stack_entry_t* call_stack_entry_t;
typedef struct _call_stack_entry_t* catch_stack_entry_t;

#define _WC_ARGTYPE_BSZ 4

#define _WC_OP_BSZ (32-_WC_ARGTYPE_BSZ)
#define _WC_OP_MASK ((1<<_WC_OP_BSZ)-1)
#define _WC_ARGTYPE_SHIFT _WC_OP_BSZ

#define WC_GET_OP(_wc) (((word_t)(_wc))&_WC_OP_MASK)
#define WC_GET_ARGTYPE(_wc) (((word_t)(_wc))>>_WC_ARGTYPE_SHIFT)

#define MAKE_WC(_a,_op) (((_a)<<_WC_ARGTYPE_SHIFT)|(_op))


#endif

