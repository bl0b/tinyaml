#include "_impl.h"
#include "text_seg.h"

#include <string.h>

void text_seg_init(text_seg_t seg) {
	dynarray_init(&seg->by_index);
	init_hashtab(&seg->by_text, (hash_func) hash_str, (compare_func) strcmp);
	text_seg_find_by_text(seg,"");
}

const char* text_seg_find_by_text(text_seg_t ts, const char* str) {
	const char* ret;
	word_t ofs = (word_t)hash_find(&ts->by_text, (hash_key)str);
	if(ofs==0) {
		ret = strdup(str);
		hash_addelem(&ts->by_text, (hash_key)ret, (hash_elem)dynarray_size(&ts->by_index));
		dynarray_set(&ts->by_index, dynarray_size(&ts->by_index), (value_t)ret);
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


void text_seg_serialize(text_seg_t seg, writer_t w) {
	int i,tot;
	const char*str;
	/* write header */
	write_string(w,"STRINGS");
	tot = dynarray_size(&seg->by_index);
	write_word(w,tot);
	for(i=0;i<tot;i++) {
		str = (const char*) dynarray_get(&seg->by_index,i);
		write_word(w,1+strlen(str));
		write_string(w,str);
	}
	write_word(w,0xFFFFFFFF);
}

void text_seg_unserialize(text_seg_t seg, reader_t r) {

}



