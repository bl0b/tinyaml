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
#include "dynarray.h"
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>


dynarray_t dynarray_new() {
	void* ret = malloc(sizeof(struct _dynarray_t));
	dynarray_init(ret);
	return ret;
}

void dynarray_init(dynarray_t ret) {
	memset(ret,0,sizeof(struct _dynarray_t));
}


void dynarray_deinit(dynarray_t zou,void(*callback)(word_t)) {
	if(zou->data) {
		if(callback) {
			int i;
			for(i=0;i<zou->size;i++) {
				callback(zou->data[i]);
			}
		}
		free(zou->data);
	}
}


void dynarray_del(dynarray_t d) {
	if(d->data) {
		free(d->data);
	}
	free(d);
}

void dynarray_reserve(dynarray_t d, word_t new_size) {
	/*printf("realloc'ing dynarray %p from %lu to %lu words\n",d, d->reserved, new_size);*/
	d->data = realloc(d->data, new_size*sizeof(dynarray_value_t));
	if(new_size>d->reserved) {
		memset(d->data+d->reserved,0,(new_size-d->reserved)*sizeof(dynarray_value_t));
	}
	d->reserved = new_size;
}

#define DYNARRAY_ALERT_SIZE (1<<23)

#define DYNARRAY_GRANUL 16
#define DYNARRAY_QUANTIZE(_x)\
	((_x+DYNARRAY_GRANUL)&~(DYNARRAY_GRANUL-1))

#define dynaccess(_d,_i) ((_d)->data[_i])

/*
 * enlarges array as necessary
 */
void dynarray_set(dynarray_t d, dynarray_index_t index, dynarray_value_t v) {
	assert( index < DYNARRAY_ALERT_SIZE );
/*	printf("dynarray has %u, %u, %p\n",d->reserved, d->size, d->data);
	printf("index %u required to be set\n",index);
	printf("d->reserved <= index : %i\n",d->reserved<=index);
*/
	if(d->reserved <= index) {
		word_t new_size = DYNARRAY_QUANTIZE(index);
		dynarray_reserve(d,new_size);
		if(d->size&&index>d->size) {
			memset( d->data + d->size*sizeof(dynarray_value_t), 0,
				(index - d->size)*sizeof(dynarray_value_t));
		}
	}
	if(d->size<=index) {
		d->size = index+1;
	}
	d->data[index] = v;
	//printf("dynarray %p has now %lu words reserved and %lu words used.\n",d, d->reserved, d->size);
}

dynarray_value_t dynarray_get(dynarray_t d,dynarray_index_t index) {
	assert( index < DYNARRAY_ALERT_SIZE );
	if(index<d->size) {
		return d->data[index];
	} else {
		//printf("warning : access out of dynarray bounds\n");
	}
	return 0;
}

/*
 * 1+index of the last defined element
 */
word_t dynarray_size(dynarray_t d) {
	return d->size;
}



