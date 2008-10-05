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

#ifndef _BML_VM_ENV_H_
#define _BML_VM_ENV_H_


/*! \addtogroup vm_env_t
 * @{
 * \brief A \ref symtab_t plus a \ref dynarray_t. Define key =&gt; data associations.
 *
 * The map supports indexed access to keys and values and random access by key lookup. Once a key is created, it is never moved. This allows for compiling indexed accesses using lookup at compile-time only.
 *
 * The VM creates its own environment, which is used to (un)serialize programs.
 */

/*! \brief Get index associated to symbol \c key. 0 means key dosen't exist in map. */
word_t env_sym_to_index(vm_dyn_env_t env, const char* key);
/*! \brief Get key associated to \c index. */
const char* env_index_to_sym(vm_dyn_env_t env, word_t index);
/*! \brief Get value at index. */
vm_data_t env_get(vm_dyn_env_t env, word_t index);
/*! \brief Set value at index. */
void env_set(vm_t vm, vm_dyn_env_t env, word_t index, vm_data_t);

/*@}*/

#endif

