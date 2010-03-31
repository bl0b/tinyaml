/* TinyaML
 * Copyright (C) 2007 Damien Leroux
 
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

/*! \addtogroup vcop_arit
 * @{
 * Arithmetic (integer/floating point mixed) and bitwise operations.
 *
 * \defgroup _defines Internal defines
 * @{
 */

#define m_mod(_a,_b) (_a%_b)

#define _add(_a,_b) ((_a)+(_b))
#define _sub(_a,_b) ((_a)-(_b))
#define _mul(_a,_b) ((_a)*(_b))
#define _div(_a,_b) ((_a)/(_b))

#define _inf(_a,_b) ((_a)<(_b))
#define _sup(_a,_b) ((_a)>(_b))
#define _eq(_a,_b) ((_a)==(_b))
#define _neq(_a,_b) ((_a)!=(_b))
#define _infeq(_a,_b) ((_a)<=(_b))
#define _supeq(_a,_b) ((_a)>=(_b))

/*@}*/

/*
 * Bin & Arith
 */

void _VM_CALL vm_op_shr(vm_t vm, word_t count) {
	vm_peek_int(vm)->data>>=1;
}

void _VM_CALL vm_op_shr_Int(vm_t vm, word_t count) {
	vm_peek_int(vm)->data>>=count;
}

void _VM_CALL vm_op_vshr(vm_t vm, word_t unused) {
	vm_data_t count = vm_pop_int(vm);
	vm_peek_int(vm)->data>>=count->data;
}

void _VM_CALL vm_op_shl(vm_t vm, word_t count) {
	vm_peek_int(vm)->data<<=1;
}

void _VM_CALL vm_op_shl_Int(vm_t vm, word_t count) {
	vm_peek_int(vm)->data<<=count;
}

void _VM_CALL vm_op_vshl(vm_t vm, word_t unused) {
	vm_data_t count = vm_pop_int(vm);
	vm_peek_int(vm)->data<<=count->data;
}

void _VM_CALL vm_op_and(vm_t vm, word_t data) {
	vm_data_t b = vm_pop_int(vm);
	vm_peek_int(vm)->data &= b->data;
}

void _VM_CALL vm_op_or(vm_t vm, word_t data) {
	vm_data_t b = vm_pop_int(vm);
	vm_peek_int(vm)->data |= b->data;
}

void _VM_CALL vm_op_xor(vm_t vm, word_t data) {
	vm_data_t b = vm_pop_int(vm);
	vm_peek_int(vm)->data ^= b->data;
}

void _VM_CALL vm_op_not(vm_t vm, word_t data) {
	vm_data_t d = vm_peek_int(vm);
	d->data = !d->data;
}

void _VM_CALL vm_op_neg(vm_t vm, word_t data) {
	vm_data_t d = vm_peek_int(vm);
	d->data = ~d->data;
}


void _VM_CALL vm_op_and_Int(vm_t vm, word_t immed) {
	vm_peek_int(vm)->data &= immed;
}

void _VM_CALL vm_op_or_Int(vm_t vm, word_t immed) {
	vm_peek_int(vm)->data |= immed;
}

void _VM_CALL vm_op_xor_Int(vm_t vm, word_t immed) {
	vm_peek_int(vm)->data ^= immed;
}


void _VM_CALL vm_op_inc(vm_t vm, word_t immed) {
	vm_peek_int(vm)->data += 1;
}

void _VM_CALL vm_op_dec(vm_t vm, word_t immed) {
	vm_peek_int(vm)->data -= 1;
}


void _VM_CALL vm_op_rmod(vm_t vm, word_t immed) {
	vm_data_t a = vm_pop_numeric(vm);
	vm_data_t b = vm_peek_numeric(vm);
	fast_apply_bin_func(a->type, a->data,b->type, b->data,m_mod,fmod,b->data, b->type);
}

void _VM_CALL vm_op_mod(vm_t vm, word_t immed) {
	vm_data_t b = vm_pop_numeric(vm);
	vm_data_t a = vm_peek_numeric(vm);
	fast_apply_bin_func(a->type, a->data,b->type, b->data,m_mod,fmod,a->data, a->type);
}

void _VM_CALL vm_op_mod_Int(vm_t vm, long bval) {
	vm_data_t b = DATA_INT(bval);
	vm_data_t a = vm_peek_numeric(vm);
	fast_apply_bin_func(a->type, a->data,b->type, b->data,m_mod,fmod,a->data, a->type);
}

void _VM_CALL vm_op_mod_Float(vm_t vm, long bval) {
	vm_data_t b = DATA_FLOAT(bval);
	vm_data_t a = vm_peek_numeric(vm);
	fast_apply_bin_func(a->type, a->data,b->type, b->data,m_mod,fmod,a->data, a->type);
}



void _VM_CALL vm_op_add(vm_t vm, word_t immed) {
	vm_data_t b = vm_pop_numeric(vm);
	vm_data_t a = vm_peek_numeric(vm);
	fast_apply_bin_func(a->type, a->data,b->type, b->data,_add,_add,a->data, a->type);
}


void _VM_CALL vm_op_rsub(vm_t vm, word_t immed) {
	vm_data_t a = vm_pop_numeric(vm);
	vm_data_t b = vm_peek_numeric(vm);
	fast_apply_bin_func(a->type, a->data,b->type, b->data,_sub,_sub,b->data, b->type);
}

void _VM_CALL vm_op_sub(vm_t vm, word_t immed) {
	vm_data_t b = vm_pop_numeric(vm);
	vm_data_t a = vm_peek_numeric(vm);
	fast_apply_bin_func(a->type, a->data,b->type, b->data,_sub,_sub,a->data, a->type);
}


void _VM_CALL vm_op_mul(vm_t vm, word_t immed) {
	vm_data_t b = vm_pop_numeric(vm);
	vm_data_t a = vm_peek_numeric(vm);
	fast_apply_bin_func(a->type, a->data,b->type, b->data,_mul,_mul,a->data, a->type);
}


void _VM_CALL vm_op_div(vm_t vm, word_t immed) {
	vm_data_t b = vm_pop_numeric(vm);
	vm_data_t a = vm_peek_numeric(vm);
	if(b->data==0) {
		raise_exception(vm, DataString, "DivByZero");
	}
	fast_apply_bin_func(a->type, a->data,b->type, b->data,_div,_div,a->data, a->type);
}

void _VM_CALL vm_op_rdiv(vm_t vm, word_t immed) {
	vm_data_t a = vm_pop_numeric(vm);
	vm_data_t b = vm_peek_numeric(vm);
	if(b->data==0) {
		vm_fatal("Division by zero !");
	}
	fast_apply_bin_func(a->type, a->data,b->type, b->data,_div,_div,b->data, b->type);
}


void _VM_CALL vm_op_add_Int(vm_t vm, long bval) {
	vm_data_t b = DATA_INT(bval);
	vm_data_t a = vm_peek_numeric(vm);
	fast_apply_bin_func(a->type, a->data,b->type, b->data,_add,_add,a->data, a->type);
}


void _VM_CALL vm_op_sub_Int(vm_t vm, long bval) {
	vm_data_t b = DATA_INT(bval);
	vm_data_t a = vm_peek_numeric(vm);
	fast_apply_bin_func(a->type, a->data,b->type, b->data,_sub,_sub,a->data, a->type);
}


void _VM_CALL vm_op_mul_Int(vm_t vm, long bval) {
	vm_data_t b = DATA_INT(bval);
	vm_data_t a = vm_peek_numeric(vm);
	fast_apply_bin_func(a->type, a->data,b->type, b->data,_mul,_mul,a->data, a->type);
}


void _VM_CALL vm_op_div_Int(vm_t vm, long bval) {
	vm_data_t b = DATA_INT(bval);
	vm_data_t a = vm_peek_numeric(vm);
	if(b->data==0) {
		vm_fatal("Division by zero !");
	}
	fast_apply_bin_func(a->type, a->data,b->type, b->data,_div,_div,a->data, a->type);
}


void _VM_CALL vm_op_add_Float(vm_t vm, word_t bval) {
	vm_data_t b = DATA_FLOAT(bval);
	vm_data_t a = vm_peek_numeric(vm);
	fast_apply_bin_func(a->type, a->data,b->type, b->data,_add,_add,a->data, a->type);
}


void _VM_CALL vm_op_sub_Float(vm_t vm, word_t bval) {
	vm_data_t b = DATA_FLOAT(bval);
	vm_data_t a = vm_peek_numeric(vm);
	fast_apply_bin_func(a->type, a->data,b->type, b->data,_sub,_sub,a->data, a->type);
}


void _VM_CALL vm_op_mul_Float(vm_t vm, word_t bval) {
	vm_data_t b = DATA_FLOAT(bval);
	vm_data_t a = vm_peek_numeric(vm);
	fast_apply_bin_func(a->type, a->data,b->type, b->data,_mul,_mul,a->data, a->type);
}


void _VM_CALL vm_op_div_Float(vm_t vm, word_t bval) {
	vm_data_t b = DATA_FLOAT(bval);
	vm_data_t a = vm_peek_numeric(vm);
	if(b->data==0) {
		vm_fatal("Division by zero !");
	}
	fast_apply_bin_func(a->type, a->data,b->type, b->data,_div,_div,a->data, a->type);
}

void _VM_CALL vm_op_inf(vm_t vm, word_t unused) {
	vm_data_t b = vm_pop_numeric(vm);
	vm_data_t a = vm_peek_numeric(vm);
	fast_apply_bin_func(a->type, a->data,b->type, b->data,_inf,_inf,a->data, a->type);
}

void _VM_CALL vm_op_sup(vm_t vm, word_t unused) {
	vm_data_t b = vm_pop_numeric(vm);
	vm_data_t a = vm_peek_numeric(vm);
	fast_apply_bin_func(a->type, a->data,b->type, b->data,_sup,_sup,a->data, a->type);
	vm_poke_data(vm,a->type, a->data);
}

void _VM_CALL vm_op_pow(vm_t vm, word_t unused) {
	vm_data_t b = vm_pop_numeric(vm);
	vm_data_t a = vm_peek_numeric(vm);
	_IFC x, y;
	if(a->type==DataInt) {
		x.f = i2f(a->data);
	} else {
		x.i = a->data;
	}
	if(b->type==DataInt) {
		y.f = i2f(b->data);
	} else {
		y.i = b->data;
	}
	x.f = powf(x.f, y.f);
	a->data = x.i;
	a->type = DataFloat;
}

void _VM_CALL vm_op_infEq(vm_t vm, word_t unused) {
	vm_data_t b = vm_pop_numeric(vm);
	vm_data_t a = vm_peek_numeric(vm);
	fast_apply_bin_func(a->type, a->data,b->type, b->data,_infeq,_infeq,a->data, a->type);
}

void _VM_CALL vm_op_supEq(vm_t vm, word_t unused) {
	vm_data_t b = vm_pop_numeric(vm);
	vm_data_t a = vm_peek_numeric(vm);
	fast_apply_bin_func(a->type, a->data,b->type, b->data,_supeq,_supeq,a->data, a->type);
}

void _VM_CALL vm_op_eq(vm_t vm, word_t unused) {
	vm_data_t b = vm_pop_numeric(vm);
	vm_data_t a = vm_peek_numeric(vm);
	fast_apply_bin_func(a->type, a->data,b->type, b->data,_eq,_eq,a->data, a->type);
}

void _VM_CALL vm_op_nEq(vm_t vm, word_t unused) {
	vm_data_t b = vm_pop_numeric(vm);
	vm_data_t a = vm_peek_numeric(vm);
	fast_apply_bin_func(a->type, a->data,b->type, b->data,_neq,_neq,a->data, a->type);
}

void _VM_CALL vm_op_sin(vm_t vm, word_t unused) {
	_IFC conv;
	conv.i = vm_pop_float(vm)->data;
	conv.f = sinf(conv.f);
	vm_push_data(vm, DataFloat, conv.i);
}

void _VM_CALL vm_op_cos(vm_t vm, word_t unused) {
	_IFC conv;
	conv.i = vm_pop_float(vm)->data;
	conv.f = cosf(conv.f);
	vm_push_data(vm, DataFloat, conv.i);
}

void _VM_CALL vm_op_tan(vm_t vm, word_t unused) {
	_IFC conv;
	conv.i = vm_pop_float(vm)->data;
	conv.f = tanf(conv.f);
	vm_push_data(vm, DataFloat, conv.i);
}



#define ALL_BITS ((word_t)-1)
#define BIT_SIGN (ALL_BITS^(ALL_BITS>>1))


void _VM_CALL vm_op_sqrt(vm_t vm, word_t unused) {
	vm_data_t d = vm_pop_numeric(vm);
	_IFC conv;
	if((d->data&BIT_SIGN)||!d->data) {
		raise_exception(vm, DataString, "OutOfRange");
	}
	if(d->type==DataFloat) {
		conv.i = d->data;
	} else if(d->type==DataInt) {
		conv.f = i2f(d->data);
	}
	conv.f = sqrtf(conv.f);
	vm_push_data(vm, DataFloat, conv.i);
}

void _VM_CALL vm_op_log(vm_t vm, word_t unused) {
	vm_data_t d = vm_pop_numeric(vm);
	_IFC conv;
	if((d->data&BIT_SIGN)||!d->data) {
		raise_exception(vm, DataString, "OutOfRange");
	}
	if(d->type==DataFloat) {
		conv.i = d->data;
	} else if(d->type==DataInt) {
		conv.f = i2f(d->data);
	}
	conv.f = logf(conv.f);
	vm_push_data(vm, DataFloat, conv.i);
}

void _VM_CALL vm_op_log_Float(vm_t vm, word_t base) {
	vm_data_t d = vm_pop_numeric(vm);
	_IFC conv;
	_IFC convbase;
	if((d->data&BIT_SIGN)||!d->data) {
		raise_exception(vm, DataString, "OutOfRange");
	}
	if(d->type==DataFloat) {
		conv.i = d->data;
	} else if(d->type==DataInt) {
		conv.f = i2f(d->data);
	}
	convbase.i = base;
	conv.i = logf(conv.f)/logf(convbase.f);
	vm_push_data(vm, DataFloat, conv.i);
}

void _VM_CALL vm_op_exp(vm_t vm, word_t unused) {
	vm_data_t d = vm_pop_numeric(vm);
	_IFC conv;
	if(d->type==DataFloat) {
		conv.i = d->data;
	} else if(d->type==DataInt) {
		conv.f = i2f(d->data);
	}
	conv.f = expf(conv.f);
	vm_push_data(vm, DataFloat, conv.i);
}

/*@}*/

