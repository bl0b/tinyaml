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

#include "vm.h"
#include "_impl.h"
#include <stdio.h>

#include <math.h>
#include "fastmath.h"

void _VM_CALL vm_op_nop(vm_t vm, word_t data) {
	/* complex algorithmic device */
}





/* FIXME : have print take an Int and pop&print as many values */
void _VM_CALL vm_op_print_Int(vm_t vm, int n) {
	vm_data_type_t dt;
	_IFC tmp;
	int k=1-n;
	while(k<=0) {
		vm_peek_data(vm,k,&dt,(word_t*)&tmp);
		switch(dt) {
		case DataInt:
			printf("%li", tmp.i);
			break;
		case DataFloat:
			printf("%lf", tmp.f);
			break;
		case DataString:
			printf("%s", (const char*) tmp.i);
			break;
		case DataObject:
			printf("[Object %p]", (opcode_stub_t*)tmp.i);
			break;
		};
		k+=1;
	}
	vm_pop_data(vm,n);
}



void _VM_CALL vm_op_push_Int(vm_t vm, word_t data) {
	vm_push_data(vm, DataInt, data);
}

void _VM_CALL vm_op_push_Float(vm_t vm, word_t data) {
	vm_push_data(vm, DataFloat, data);
}

void _VM_CALL vm_op_push_String(vm_t vm, word_t data) {
	vm_push_data(vm, DataString, data);
}

void _VM_CALL vm_op_push_Opcode(vm_t vm, word_t data) {
	vm_push_data(vm, DataInt, data);
}

void _VM_CALL vm_op_pop(vm_t vm, word_t data) {
	vm_pop_data(vm,1);
}

void _VM_CALL vm_op_pop_Int(vm_t vm, word_t data) {
	vm_pop_data(vm,data);
}

void _VM_CALL vm_op_dup_Int(vm_t vm, word_t data) {
	vm_data_type_t a;
	word_t b;
	vm_peek_data(vm,(int)data,&a,&b);
	vm_push_data(vm, a, b);
}

void _VM_CALL vm_op_SNZ(vm_t vm, word_t data) {
	vm_data_type_t a;
	word_t b;
	vm_peek_data(vm,(int)data,&a,&b);
	vm_pop_data(vm,1);
	if(b) {
		node_value(thread_t,vm->current_thread)->IP+=2;
	}
}

void _VM_CALL vm_op_SZ(vm_t vm, word_t data) {
	vm_data_type_t a;
	word_t b;
	vm_peek_data(vm,(int)data,&a,&b);
	vm_pop_data(vm,1);
	if(!b) {
		node_value(thread_t,vm->current_thread)->IP+=2;
	}
}

/*
 * Jumps
 */

void _VM_CALL vm_op_jmp_Label(vm_t vm, word_t data) {
	thread_t t=node_value(thread_t,vm->current_thread);
	t->jmp_ofs=t->IP+data;
}

/*
 * Call stack
 */

void _VM_CALL vm_op_call_Label(vm_t vm, word_t data) {
	thread_t t=node_value(thread_t,vm->current_thread);
	vm_push_caller(vm, t->program, t->IP);
	t->jmp_ofs=t->IP+data;
}

void _VM_CALL vm_op_lcall_Label(vm_t vm, word_t data) {
	thread_t t=node_value(thread_t,vm->current_thread);
	vm_data_type_t a;
	word_t b;
	vm_peek_data(vm,0,&a,&b);
	vm_push_caller(vm, t->program, t->IP);
	t->jmp_seg=(program_t)b;
	/* FIXME : can resolve_label() set up correct data value ? */
	t->jmp_ofs=data;
}


void _VM_CALL vm_op_enter_Int(vm_t vm, word_t size) {
	thread_t t=node_value(thread_t,vm->current_thread);
	gstack_grow(&t->locals_stack,size);
}

void _VM_CALL vm_op_leave_Int(vm_t vm, word_t size) {
	thread_t t=node_value(thread_t,vm->current_thread);
	gstack_shrink(&t->locals_stack,size);
}

void _VM_CALL vm_op_getmem_Int(vm_t vm, int n) {
	thread_t t=node_value(thread_t,vm->current_thread);
	if(n<0) {
		struct _data_stack_entry_t* local = _gpeek(&t->locals_stack,1+n);	/* -1 becomes 0 */
		vm_push_data(vm,local->type,local->data);
	} else {
		dynarray_t da = &t->program->data;
		n<<=1;
		vm_push_data(vm,da->data[n],da->data[n+1]);
	}
}


void _VM_CALL vm_op_setmem_Int(vm_t vm, int n) {
	thread_t t=node_value(thread_t,vm->current_thread);
	struct _data_stack_entry_t* top = gpop( struct _data_stack_entry_t*, &t->data_stack );
	if(n<0) {
		struct _data_stack_entry_t* local = gpeek( struct _data_stack_entry_t*, &t->locals_stack, 1+n );
		local->type=top->type;
		local->data=top->data;
	} else {
		n<<=1;
		t->program->data.data[n] = (word_t) top->type;
		t->program->data.data[n+1] = top->data;
	}
}


void _VM_CALL vm_op_setmem(vm_t vm, int n) {
	thread_t t=node_value(thread_t,vm->current_thread);
	struct _data_stack_entry_t* top = gpop( struct _data_stack_entry_t*, &t->data_stack );
	if(top->type!=DataInt) {
		return;
	}
	vm_op_setmem_Int(vm, (int)top->data);
}


void _VM_CALL vm_op_getmem(vm_t vm, int n) {
	thread_t t=node_value(thread_t,vm->current_thread);
	struct _data_stack_entry_t* top = gpop( struct _data_stack_entry_t*, &t->data_stack );
	if(top->type!=DataInt) {
		return;
	}
	vm_op_getmem_Int(vm, (int)top->data);
}



void _VM_CALL vm_op_retval_Int(vm_t vm, word_t n) {
	program_t cs;
	word_t ip;
	vm_data_type_t dt;
	word_t d;
	thread_t t=node_value(thread_t,vm->current_thread);
	vm_peek_data(vm,0,&dt,&d);
	vm_pop_data(vm,n);
	vm_poke_data(vm,dt,d);
	vm_peek_caller(vm,&cs,&ip);
	vm_pop_caller(vm,1);
	t->jmp_seg=cs;
	t->jmp_ofs=ip+2;
}


void _VM_CALL vm_op_ret_Int(vm_t vm, word_t n) {
	program_t cs;
	word_t ip;
	thread_t t=node_value(thread_t,vm->current_thread);
	vm_pop_data(vm,n);
	vm_peek_caller(vm,&cs,&ip);
	vm_pop_caller(vm,1);
	t->jmp_seg=cs;
	t->jmp_ofs=ip+2;
}

/*
 * Bin & Arith
 */

void _VM_CALL vm_op_shr(vm_t vm, word_t count) {
	vm_data_type_t oa;
	word_t data;
	vm_peek_data(vm,0,&oa,&data);
	if(oa==DataInt) {
		data>>=1;
		vm_poke_data(vm,oa,data);
	}
}

void _VM_CALL vm_op_shr_Int(vm_t vm, word_t count) {
	vm_data_type_t oa;
	word_t data;
	vm_peek_data(vm,0,&oa,&data);
	if(oa==DataInt) {
		data>>=count;
		vm_poke_data(vm,oa,data);
	}
}

void _VM_CALL vm_op_vshr(vm_t vm, word_t count) {
	vm_data_type_t oa;
	word_t data;
	vm_peek_data(vm,0,&oa,&count);
	vm_pop_data(vm,1);
	vm_peek_data(vm,0,&oa,&data);
	if(oa==DataInt) {
		data>>=count;
		vm_poke_data(vm,oa,data);
	}
}

void _VM_CALL vm_op_shl(vm_t vm, word_t count) {
	vm_data_type_t oa;
	word_t data;
	vm_peek_data(vm,0,&oa,&data);
	if(oa==DataInt) {
		data<<=1;
		vm_poke_data(vm,oa,data);
	}
}

void _VM_CALL vm_op_shl_Int(vm_t vm, word_t count) {
	vm_data_type_t oa;
	word_t data;
	vm_peek_data(vm,0,&oa,&data);
	if(oa==DataInt) {
		data<<=count;
		vm_poke_data(vm,oa,data);
	}
}

void _VM_CALL vm_op_vshl(vm_t vm, word_t count) {
	vm_data_type_t oa;
	word_t data;
	vm_peek_data(vm,0,&oa,&count);
	vm_pop_data(vm,1);
	vm_peek_data(vm,0,&oa,&data);
	if(oa==DataInt) {
		data<<=count;
		vm_poke_data(vm,oa,data);
	}
}

void _VM_CALL vm_op_and(vm_t vm, word_t data) {
	vm_data_type_t dt;
	word_t a,b;
	vm_peek_data(vm,-1,&dt,&a); if(dt!=DataInt) { return; }
	vm_peek_data(vm,0,&dt,&b); if(dt!=DataInt) { return; }
	vm_pop_data(vm,1);
	vm_poke_data(vm,dt,a&b);
}

void _VM_CALL vm_op_or(vm_t vm, word_t data) {
	vm_data_type_t dt;
	word_t a,b;
	vm_peek_data(vm,-1,&dt,&a); if(dt!=DataInt) { return; }
	vm_peek_data(vm,0,&dt,&b); if(dt!=DataInt) { return; }
	vm_pop_data(vm,1);
	vm_poke_data(vm,dt,a|b);
}

void _VM_CALL vm_op_xor(vm_t vm, word_t data) {
	vm_data_type_t dt;
	word_t a,b;
	vm_peek_data(vm,-1,&dt,&a); if(dt!=DataInt) { return; }
	vm_peek_data(vm,0,&dt,&b); if(dt!=DataInt) { return; }
	vm_pop_data(vm,1);
	vm_poke_data(vm,dt,a^b);
}

void _VM_CALL vm_op_not(vm_t vm, word_t data) {
	vm_data_type_t dt;
	word_t a;
	vm_peek_data(vm,0,&dt,&a); if(dt!=DataInt) { return; }
	vm_poke_data(vm,dt,~a);
}


void _VM_CALL vm_op_and_Int(vm_t vm, word_t immed) {
	vm_data_type_t dt;
	word_t a;
	vm_peek_data(vm,0,&dt,&a); if(dt!=DataInt) { return; }
	vm_poke_data(vm,dt,a&immed);
}

void _VM_CALL vm_op_or_Int(vm_t vm, word_t immed) {
	vm_data_type_t dt;
	word_t a;
	vm_peek_data(vm,0,&dt,&a); if(dt!=DataInt) { return; }
	vm_poke_data(vm,dt,a|immed);
}

void _VM_CALL vm_op_xor_Int(vm_t vm, word_t immed) {
	vm_data_type_t dt;
	word_t a;
	vm_peek_data(vm,0,&dt,&a); if(dt!=DataInt) { return; }
	vm_poke_data(vm,dt,a^immed);
}


void _VM_CALL vm_op_inc(vm_t vm, word_t immed) {
	vm_data_type_t dt;
	word_t a;
	vm_peek_data(vm,0,&dt,&a); if(dt!=DataInt) { return; }
	vm_poke_data(vm,dt,a+1);
}

void _VM_CALL vm_op_dec(vm_t vm, word_t immed) {
	vm_data_type_t dt;
	word_t a;
	vm_peek_data(vm,0,&dt,&a); if(dt!=DataInt) { return; }
	vm_poke_data(vm,dt,a-1);
}


#define m_mod(_a,_b) (_a%_b)

void _VM_CALL vm_op_mod(vm_t vm, word_t immed) {
	vm_data_type_t dta,dtb;
	word_t a,b;
	vm_peek_data(vm,0,&dta,&a);
	vm_peek_data(vm,-1,&dtb,&b);
	vm_pop_data(vm,1);
	fast_apply_bin_func(dta,a,dtb,b,m_mod,fmod,a,dta);
	vm_poke_data(vm,dta,a);
}

void _VM_CALL vm_op_mod_Int(vm_t vm, int b) {
	vm_data_type_t dta,dtb=DataInt;
	word_t a;
	vm_peek_data(vm,0,&dta,&a);
	fast_apply_bin_func(dta,a,dtb,b,m_mod,fmod,a,dta);
	vm_poke_data(vm,dta,a);
}

void _VM_CALL vm_op_mod_Float(vm_t vm, int b) {
	vm_data_type_t dta,dtb=DataFloat;
	word_t a;
	vm_peek_data(vm,0,&dta,&a);
	fast_apply_bin_func(dta,a,dtb,b,m_mod,fmod,a,dta);
	vm_poke_data(vm,dta,a);
}


#define _add(_a,_b) ((_a)+(_b))
#define _sub(_a,_b) ((_a)-(_b))
#define _mul(_a,_b) ((_a)*(_b))
#define _div(_a,_b) ((_a)/(_b))


void _VM_CALL vm_op_add(vm_t vm, word_t immed) {
	vm_data_type_t dta,dtb;
	word_t a,b;
	vm_peek_data(vm,0,&dta,&a);
	vm_peek_data(vm,-1,&dtb,&b);
	vm_pop_data(vm,1);
	fast_apply_bin_func(dta,a,dtb,b,_add,_add,a,dta);
	vm_poke_data(vm,dta,a);
}


void _VM_CALL vm_op_sub(vm_t vm, word_t immed) {
	vm_data_type_t dta,dtb;
	word_t a,b;
	vm_peek_data(vm,0,&dta,&a);
	vm_peek_data(vm,-1,&dtb,&b);
	vm_pop_data(vm,1);
	fast_apply_bin_func(dta,a,dtb,b,_sub,_sub,a,dta);
	vm_poke_data(vm,dta,a);
}


void _VM_CALL vm_op_mul(vm_t vm, word_t immed) {
	vm_data_type_t dta,dtb;
	word_t a,b;
	vm_peek_data(vm,0,&dta,&a);
	vm_peek_data(vm,-1,&dtb,&b);
	vm_pop_data(vm,1);
	fast_apply_bin_func(dta,a,dtb,b,_mul,_mul,a,dta);
	vm_poke_data(vm,dta,a);
}


void _VM_CALL vm_op_div(vm_t vm, word_t immed) {
	vm_data_type_t dta,dtb;
	word_t a,b;
	vm_peek_data(vm,0,&dta,&a);
	vm_peek_data(vm,-1,&dtb,&b);
	vm_pop_data(vm,1);
	fast_apply_bin_func(dta,a,dtb,b,_div,_div,a,dta);
	vm_poke_data(vm,dta,a);
}


void _VM_CALL vm_op_add_Int(vm_t vm, int b) {
	vm_data_type_t dta,dtb=DataInt;
	word_t a;
	vm_peek_data(vm,0,&dta,&a);
	fast_apply_bin_func(dta,a,dtb,b,_add,_add,a,dta);
	vm_poke_data(vm,dta,a);
}


void _VM_CALL vm_op_sub_Int(vm_t vm, int b) {
	vm_data_type_t dta,dtb=DataInt;
	word_t a;
	vm_peek_data(vm,0,&dta,&a);
	fast_apply_bin_func(dta,a,dtb,b,_sub,_sub,a,dta);
	vm_poke_data(vm,dta,a);
}


void _VM_CALL vm_op_mul_Int(vm_t vm, int b) {
	vm_data_type_t dta,dtb=DataInt;
	word_t a;
	vm_peek_data(vm,0,&dta,&a);
	fast_apply_bin_func(dta,a,dtb,b,_mul,_mul,a,dta);
	vm_poke_data(vm,dta,a);
}


void _VM_CALL vm_op_div_Int(vm_t vm, int b) {
	vm_data_type_t dta,dtb=DataInt;
	word_t a;
	vm_peek_data(vm,0,&dta,&a);
	fast_apply_bin_func(dta,a,dtb,b,_div,_div,a,dta);
	vm_poke_data(vm,dta,a);
}


void _VM_CALL vm_op_add_Float(vm_t vm, float b) {
	vm_data_type_t dta,dtb=DataFloat;
	word_t a;
	vm_peek_data(vm,0,&dta,&a);
	fast_apply_bin_func(dta,a,dtb,b,_add,_add,a,dta);
	vm_poke_data(vm,dta,a);
}


void _VM_CALL vm_op_sub_Float(vm_t vm, float b) {
	vm_data_type_t dta,dtb=DataFloat;
	word_t a;
	vm_peek_data(vm,0,&dta,&a);
	fast_apply_bin_func(dta,a,dtb,b,_sub,_sub,a,dta);
	vm_poke_data(vm,dta,a);
}


void _VM_CALL vm_op_mul_Float(vm_t vm, float b) {
	vm_data_type_t dta,dtb=DataFloat;
	word_t a;
	vm_peek_data(vm,0,&dta,&a);
	fast_apply_bin_func(dta,a,dtb,b,_mul,_mul,a,dta);
	vm_poke_data(vm,dta,a);
}


void _VM_CALL vm_op_div_Float(vm_t vm, float b) {
	vm_data_type_t dta,dtb=DataFloat;
	word_t a;
	vm_peek_data(vm,0,&dta,&a);
	fast_apply_bin_func(dta,a,dtb,b,_div,_div,a,dta);
	vm_poke_data(vm,dta,a);
}







