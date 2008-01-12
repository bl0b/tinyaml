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
#include "text_seg.h"
#include "object.h"

#include <string.h>
#include <stdio.h>

void text_seg_init(text_seg_t seg) {
	dynarray_init(&seg->by_index);
	init_hashtab(&seg->by_text, (hash_func) hash_str, (compare_func) strcmp);
	text_seg_find_by_text(seg,"");
}

void htab_free_dict(htab_entry_t);

void symtab_deinit(vm_t vm, text_seg_t seg) {
	if(seg->by_index.reserved) {
		dynarray_deinit(&seg->by_index,NULL);
	}
	clean_hashtab(&seg->by_text,htab_free_dict);
}

void text_seg_deinit(text_seg_t seg) {
	dynarray_deinit(&seg->by_index,(void(*)(word_t))free);
	clean_hashtab(&seg->by_text,NULL);
}

void text_seg_free(text_seg_t seg) {
	text_seg_deinit(seg);
	free(seg);
}

const char* text_seg_find_by_text(text_seg_t ts, const char* str) {
	const char* ret;
	word_t ofs = (word_t)hash_find(&ts->by_text, (hash_key)str);
	if(ofs==0) {
		ret = strdup(str);
		hash_addelem(&ts->by_text, (hash_key)ret, (hash_elem)dynarray_size(&ts->by_index));
		dynarray_set(&ts->by_index, dynarray_size(&ts->by_index), (value_t)ret);
		/*printf("added string %p:\"%s\" into seg %p at offset %lu\n",ret,ret,ts,dynarray_size(&ts->by_index)-1);*/
	} else {
		ret = (const char*)dynarray_get(&ts->by_index,ofs);
	}
	return ret;
}

const char* text_seg_find_by_index(text_seg_t ts, word_t i) {
	if(i>=dynarray_size(&ts->by_index)) {
		return NULL;
	}
	return (const char*)dynarray_get(&ts->by_index,i);
}

word_t text_seg_text_to_index(text_seg_t ts, const char*str) {
	return (word_t)hash_find(&ts->by_text, (hash_key)str);
}


void text_seg_serialize(text_seg_t seg, writer_t w, const char* sec_name) {
	int i,tot;
	const char*str;
	/* write header */
	write_string(w,sec_name);
	tot = dynarray_size(&seg->by_index);
	write_word(w,tot);
	for(i=0;i<tot;i+=1) {
		str = (const char*) dynarray_get(&seg->by_index,i);
		write_word(w,1+strlen(str));
		write_string(w,str);
	}
	write_word(w,0xFFFFFFFF);
}

void text_seg_unserialize(text_seg_t seg, reader_t r, const char* sec_name) {
	const char*str;
	word_t w,tot;
	int i;
	str = read_string(r);
	assert(!strcmp(str,sec_name));
	tot = read_word(r);
	dynarray_reserve(&seg->by_index,tot);
	w = read_word(r);
	assert(w==1);
	str = read_string(r);
	assert(!*str);
	for(i=1;i<tot;i+=1) {
		w = read_word(r);
		str = read_string(r);
		assert(strlen(str)+1==w);
		(void)text_seg_find_by_text(seg,str);
	}
	w = read_word(r);
	assert(w==0xFFFFFFFF);
}



word_t env_sym_to_index(vm_dyn_env_t env, const char* key) {
	return text_seg_text_to_index(&env->symbols,key);
}

const char* env_index_to_sym(vm_dyn_env_t env, word_t index) {
	return text_seg_find_by_index(&env->symbols,index);
}

vm_data_t env_get(vm_dyn_env_t env, word_t index) {
	return (vm_data_t) &(env->data.data[index<<1]);
}

void env_set(vm_t vm, vm_dyn_env_t env, word_t index,vm_data_t data) {
	index<<=1;
	if(env->data.size<(index+2)) {
		dynarray_reserve(&env->data,index+2);
	} else if((vm_data_type_t)env->data.data[index]==DataObject) {
		vm_obj_deref(vm,(void*)env->data.data[index+1]);
	}
	env->data.data[index] = data->type;
	env->data.data[index+1] = data->data;
	if(data->type==DataObject) {
		vm_obj_ref(vm,(void*)data->data);
	}
}


