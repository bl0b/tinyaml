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

#ifndef _BML_PROGRAM_H_
#define _BML_PROGRAM_H_

/*! \addtogroup vm_prgs
 * @{
 */

program_t program_new();
void program_free(vm_t, program_t);

void program_add_label(program_t,word_t,const char*);
word_t program_label_to_ofs(program_t, const char*);
const char* program_ofs_to_label(program_t, word_t);

word_t program_find_string(program_t, const char*);
void program_write_code(program_t, word_t, word_t);
void program_reserve_code(program_t, word_t);
void program_reserve_data(program_t, word_t);
word_t program_get_code_size(program_t);
void program_fetch(program_t, word_t, word_t*, word_t*);

void program_add_loadlib(program_t, const char*);
void program_add_require(program_t, const char*);

void program_serialize(vm_t vm, program_t p, writer_t w);
program_t program_unserialize(vm_t vm, reader_t r);

const char* program_lookup_label(program_t p, word_t IP);
const char* program_disassemble(vm_t vm, program_t p, word_t IP);

void program_dump_stats(program_t p);

/*@}*/

#endif

