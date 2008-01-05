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

#ifndef _BML_ABSTRACT_IO_H_
#define _BML_ABSTRACT_IO_H_

typedef struct _reader_t* reader_t;
typedef struct _writer_t* writer_t;

writer_t file_writer_new(const char*);
writer_t buffer_writer_new(char*, word_t);
void writer_close(writer_t);

reader_t file_reader_new(const char*);
reader_t buffer_reader_new(const char*, word_t);
void reader_swap_endian(reader_t);
void reader_close(reader_t);

word_t write_word(writer_t, word_t);
word_t write_string(writer_t, const char*);

word_t read_word(reader_t);
const char* read_string(reader_t);


#endif

