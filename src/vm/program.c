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

#include "_impl.h"
#include "vm_types.h"
#include "text_seg.h"
#include "program.h"

#include <string.h>
#include <stdio.h>

program_t program_new() {
	program_t ret = (program_t)malloc(sizeof(struct _program_t));
	/*init_hashtab(&ret->labels, (hash_func) hash_str, (compare_func) strcmp);*/
	text_seg_init(&ret->strings);
	dynarray_init(&ret->data);
	dynarray_init(&ret->code);
	//printf("PROGRAM NEW\n");
	return ret;
}

void _VM_CALL vm_op_push_Int(vm_t,word_t);
void _VM_CALL vm_op_dup_Int(vm_t,word_t);
void _VM_CALL vm_op_ret_Int(vm_t,word_t);
void _VM_CALL vm_op_setmem(vm_t,word_t);
void _VM_CALL vm_op_SZ(vm_t,word_t);
void _VM_CALL vm_op_dec(vm_t,word_t);
void _VM_CALL vm_op_jmp_Label(vm_t,word_t);
vm_t vm_run_program_fg(vm_t vm, program_t p, word_t ip, word_t prio);

word_t clean_data_seg[20] = {
	(word_t)vm_op_push_Int,	0,		/* counter, */
/* label: */
	(word_t)vm_op_push_Int,	0,		/* counter, 0 */
	(word_t)vm_op_dup_Int,	(word_t)-1,	/* counter, 0, counter */
	(word_t)vm_op_dec,	0,		/* counter, 0, counter-1 */
	(word_t)vm_op_setmem,	0,		/* counter, */
	(word_t)vm_op_dec,	0,		/* counter-1, */
	(word_t)vm_op_dup_Int,	0,		/* counter-1, counter-1, */
	(word_t)vm_op_SZ,	0,		/* counter-1, */
	(word_t)vm_op_jmp_Label,(word_t)-14,	/* counter-1, */
	(word_t)vm_op_ret_Int,	1,		/* , */
};


void program_free(vm_t vm, program_t p) {
	printf("program_free\n");
	text_seg_deinit(&p->strings);
	dynarray_deinit(&p->code,NULL);
	if(p->data.size) {
		p->code.size=18;
		p->code.reserved=0;
		p->code.data=clean_data_seg;
		clean_data_seg[1]=p->data.size>>1;
		printf("cleaning %lu data items\n",clean_data_seg[1]);
		vm_run_program_fg(vm,p,0,99);
	}
	dynarray_deinit(&p->data,NULL);
	free(p);
}

void program_fetch(program_t p, word_t ip, word_t* op, word_t* arg) {
/*	printf("fetch at @%p:%8.8lX : %8.8lX %8.8lX\n",p,ip,dynarray_get(&p->code,ip),dynarray_get(&p->code,ip+1));
*/	*arg = dynarray_get(&p->code,ip+1);
	*op = dynarray_get(&p->code,ip);
}

void program_reserve_code(program_t p, word_t sz) {
	sz+=dynarray_size(&p->code);
	dynarray_reserve(&p->code,sz);
}

void program_reserve_data(program_t p, word_t sz) {
	sz+=dynarray_size(&p->data);
	dynarray_reserve(&p->data,sz);
}

void program_write_code(program_t p, word_t op, word_t arg) {
	word_t ip = dynarray_size(&p->code);
/*	printf("writing %8.8lX:%8.8lX into code seg at %p (%lu words long)\n",op,arg,p,dynarray_size(&p->code));
*/	dynarray_set(&p->code,ip+1,arg);
	dynarray_set(&p->code,ip,op);
/*	{
		int i;
		for(i=0;i<dynarray_size(&p->code);i+=2) {
			printf("%8.8lX %8.8lX   ",dynarray_get(&p->code,i),dynarray_get(&p->code,i+1));
			if(i%8==6) printf("\n");
		}
		printf("\n");
	}
*/}

word_t program_get_code_size(program_t p) {
	return dynarray_size(&p->code);
}

word_t program_find_string(program_t p, const char*str) {
	return (word_t)text_seg_find_by_text(&p->strings,str);
}


