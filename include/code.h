
#ifndef _BML_CODE_H_
#define _BML_CODE_H_


typedef void (*opcode_noarg_t) (vm_t);
typedef void (*opcode_int_t) (vm_t, int);
typedef void (*opcode_float_t) (vm_t, float);
typedef void (*opcode_ptr_t) (vm_t, void*);
typedef void (*opcode_label_t) (vm_t, int);
typedef void (*opcode_string_t) (vm_t, const char*);
typedef void (*opcode_opcode_t) (vm_t, word_t);

typedef void (*opcode_stub_t) (vm_t, word_t t);

void vm_compile_noarg(vm_t, opcode_t);
void vm_compile_int(vm_t, opcode_t);
void vm_compile_float(vm_t, opcode_t);
void vm_compile_ptr(vm_t, opcode_t);
void vm_compile_label(vm_t, opcode_t);
void vm_compile_string(vm_t, opcode_t);
void vm_compile_opcode(vm_t, opcode_t);




struct _opcode_t {
	const char* name;
	opcode_arg_t arg_type;
	opcode_stub_t exec;
};


#endif

