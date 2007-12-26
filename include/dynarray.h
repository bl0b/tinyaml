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

