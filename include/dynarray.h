#ifndef _BML_DYNARRAY_H_
#define _BML_DYNARRAY_H_

#include "vm_types.h"

#include <sys/types.h>

dynarray_t dynarray_new();
void dynarray_init(dynarray_t);
void dynarray_del(dynarray_t);
void dynarray_reserve(dynarray_t d, word_t new_size);
/* enlarges array as necessary */
void dynarray_set(dynarray_t,dynarray_index_t,dynarray_value_t);
dynarray_value_t dynarray_get(dynarray_t,dynarray_index_t);
word_t dynarray_size(dynarray_t);

#endif

