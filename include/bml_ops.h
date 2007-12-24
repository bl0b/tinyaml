#ifndef _BML_OPS_H_
#define _BML_OPS_H_

void vm_op_push_Int(vm_t vm, word_t data);
void vm_op_push_Float(vm_t vm, word_t data);
void vm_op_push_String(vm_t vm, word_t data);
void vm_op_push_Opcode(vm_t vm, word_t data);

void vm_op_pop(vm_t vm, word_t data);
void vm_op_pop_Int(vm_t vm, word_t data);

void vm_op_dup_Int(vm_t vm, word_t data);

void vm_op_nop(vm_t vm, word_t data);

void vm_op_ST(vm_t vm, word_t data);

void vm_op_SF(vm_t vm, word_t data);

void vm_op_jmp_Label(vm_t vm, word_t data);

void vm_op_print_String(vm_t vm, word_t data);

#endif

