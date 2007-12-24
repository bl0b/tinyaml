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

void dynarray_del(dynarray_t d) {
	if(d->data) {
		free(d->data);
	}
	free(d);
}

void dynarray_reserve(dynarray_t d, word_t new_size) {
	//printf("realloc'ing dynarray %p from %lu to %lu words\n",d, d->reserved, new_size);
	d->data = realloc(d->data, new_size*sizeof(dynarray_value_t));
	memset( d->data + d->size*sizeof(dynarray_value_t), 0,
		(new_size - d->size)*sizeof(dynarray_value_t));
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



