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

program_t program_new();
void program_free(program_t);
word_t program_find_string(program_t, const char*);
void program_write_code(program_t, word_t, word_t);
void program_reserve_code(program_t, word_t);
void program_reserve_data(program_t, word_t);
word_t program_get_code_size(program_t);
void program_fetch(program_t, word_t, word_t*, word_t*);

#endif

