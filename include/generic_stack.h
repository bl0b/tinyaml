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

#ifndef __BML_STACK_H__
#define __BML_STACK_H__

#include "vm_types.h"

/*! \addtogroup gstack_t
 * @{
 * \brief A dynamic stack with configurable token size.
 */

/*! \brief Create a new stack. */
generic_stack_t new_gstack(word_t token_size);
/*! \brief Init a newly allocated buffer. */
void gstack_init(generic_stack_t, word_t token_size);
/*! \brief Deinit a stack, calling \c callback on each token the stack contains. */
void gstack_deinit(generic_stack_t, void (*callback)(void*));

/*! \brief Push the token pointed to by \c w onto the stack. */
void gpush(generic_stack_t s, void* w);

/*! \brief Pop a token from the stack. */
void* _gpop(generic_stack_t s);
/*! \brief Pop a token from the stack. (typecast swallowing version) */
#define gpop(__t,__s) ((__t)_gpop(__s))
/*! \brief Peek the i-th token from the stack. (0 is top) */
void* _gpeek(generic_stack_t s,long);
/*! \brief Peek the \c __i -th token from the stack. (typecast swallowing version) */
#define gpeek(__t,__s,__i) ((__t)_gpeek(__s,__i))
/*! \brief Free a stack. */
void free_gstack(generic_stack_t s);

/*! \brief Get the current size of the stack. */
word_t gstack_size(generic_stack_t s);

/*! \brief Reserve \c count words on stack. */
void gstack_grow(generic_stack_t s, word_t count);
/*! \brief Undo a previous reservation. */
void gstack_shrink(generic_stack_t s, word_t count);

/*! \brief Predicate. */
#define gstack_is_empty(_s) (_s->sp==((word_t)-1))
/*! \brief Predicate. */
#define gstack_is_not_empty(_s) (_s->sp!=((word_t)-1))

/*@}*/

#endif

