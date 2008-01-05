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
#include <string.h>

#include <math.h>
#include "fastmath.h"


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
	vm_peek_data(vm,0,&dtb,&b);
	vm_peek_data(vm,-1,&dta,&a);
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
	vm_peek_data(vm,0,&dtb,&b);
	vm_peek_data(vm,-1,&dta,&a);
	vm_pop_data(vm,1);
	fast_apply_bin_func(dta,a,dtb,b,_add,_add,a,dta);
	vm_poke_data(vm,dta,a);
}


void _VM_CALL vm_op_sub(vm_t vm, word_t immed) {
	vm_data_type_t dta,dtb;
	word_t a,b;
	vm_peek_data(vm,0,&dtb,&b);
	vm_peek_data(vm,-1,&dta,&a);
	vm_pop_data(vm,1);
	fast_apply_bin_func(dta,a,dtb,b,_sub,_sub,a,dta);
	vm_poke_data(vm,dta,a);
}


void _VM_CALL vm_op_mul(vm_t vm, word_t immed) {
	vm_data_type_t dta,dtb;
	word_t a,b;
	vm_peek_data(vm,0,&dtb,&b);
	vm_peek_data(vm,-1,&dta,&a);
	vm_pop_data(vm,1);
	fast_apply_bin_func(dta,a,dtb,b,_mul,_mul,a,dta);
	vm_poke_data(vm,dta,a);
}


void _VM_CALL vm_op_div(vm_t vm, word_t immed) {
	vm_data_type_t dta,dtb;
	word_t a,b;
	vm_peek_data(vm,0,&dtb,&b);
	vm_peek_data(vm,-1,&dta,&a);
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


void _VM_CALL vm_op_add_Float(vm_t vm, word_t b) {
	vm_data_type_t dta,dtb=DataFloat;
	word_t a;
	vm_peek_data(vm,0,&dta,&a);
	fast_apply_bin_func(dta,a,dtb,b,_add,_add,a,dta);
	vm_poke_data(vm,dta,a);
}


void _VM_CALL vm_op_sub_Float(vm_t vm, word_t b) {
	vm_data_type_t dta,dtb=DataFloat;
	word_t a;
	vm_peek_data(vm,0,&dta,&a);
	fast_apply_bin_func(dta,a,dtb,b,_sub,_sub,a,dta);
	vm_poke_data(vm,dta,a);
}


void _VM_CALL vm_op_mul_Float(vm_t vm, word_t b) {
	vm_data_type_t dta,dtb=DataFloat;
	word_t a;
	vm_peek_data(vm,0,&dta,&a);
	fast_apply_bin_func(dta,a,dtb,b,_mul,_mul,a,dta);
	vm_poke_data(vm,dta,a);
}


void _VM_CALL vm_op_div_Float(vm_t vm, word_t b) {
	vm_data_type_t dta,dtb=DataFloat;
	word_t a;
	vm_peek_data(vm,0,&dta,&a);
	fast_apply_bin_func(dta,a,dtb,b,_div,_div,a,dta);
	vm_poke_data(vm,dta,a);
}



