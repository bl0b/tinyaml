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


#ifndef _BML_TEXT_SEG_H_
#define _BML_TEXT_SEG_H_

#include "abstract_io.h"

/*! \addtogroup symtab_t
 * @{
 * \brief The text segment associates a string with an index. It can also serve as a symbol table.
 */


void text_seg_init(text_seg_t seg);
void text_seg_deinit(text_seg_t seg);
void text_seg_free(text_seg_t seg);
void text_seg_copy(text_seg_t dest, text_seg_t src);
const char* text_seg_find_by_text(text_seg_t, const char*);
const char* text_seg_find_by_index(text_seg_t, word_t);
word_t text_seg_text_to_index(text_seg_t, const char*);

void text_seg_serialize(text_seg_t, writer_t, const char* sec_name);
void text_seg_unserialize(text_seg_t, reader_t, const char* sec_name);

word_t env_sym_to_index(vm_dyn_env_t env, const char* key);
const char* env_index_to_sym(vm_dyn_env_t env, word_t index);
vm_data_t env_get(vm_dyn_env_t env, word_t index);
void env_set(vm_t vm, vm_dyn_env_t env, word_t index,vm_data_t data);

/*@}*/

#endif

