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

/*! \addtogroup data_struc
 * @{
 * \addtogroup data_containers
 * @{
 * \addtogroup dynarray_t Dynamic Array
 * @{
 * \brief A dynamic indexed array of \ref _data_stack_entry_t "VM data".
 * The dynamic array grows automatically when set index is out of range.
 * The get operation returns a nil value when index is out of range.
 */

/*! \brief Create a new array. */
dynarray_t dynarray_new();
/*! \brief Init a newly allocated buffer. */
void dynarray_init(dynarray_t);
/*! \brief Deinit a dynamic array, after having executed the given callback on each element (callback may be NULL). */
void dynarray_deinit(dynarray_t,void(*)(word_t));
/*! \brief Delete a dynamic array. */
void dynarray_del(dynarray_t);
/*! \brief Reserve \c new_size words for array \c d. */
void dynarray_reserve(dynarray_t d, word_t new_size);
/*! \brief Set the value at given index. \note Enlarges array as necessary. */
void dynarray_set(dynarray_t,dynarray_index_t,dynarray_value_t);
/*! \brief Get the value at given index. */
dynarray_value_t dynarray_get(dynarray_t,dynarray_index_t);
/*! \brief Get elements count. */
word_t dynarray_size(dynarray_t);

/*@}*/
/*@}*/
/*@}*/

#endif

