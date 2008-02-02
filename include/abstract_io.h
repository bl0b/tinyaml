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

/*! \addtogroup abstract_io
 * Abstract readers and writers can handle 32-bit words and character strings.
 * They provide a single interface for file and memory buffer I/O.
 * Internals are not discussed here.
 * @{
 */

/*! \brief opaque type : generic reader */
typedef struct _reader_t* reader_t;
/*! \brief opaque type : generic writer */
typedef struct _writer_t* writer_t;

/*! \brief create a new writer associated to given file. */
writer_t file_writer_new(const char*);
/*! \brief create a new writer associated to given buffer of given length. */
writer_t buffer_writer_new(char*, word_t);
/*! \brief close the given writer. */
void writer_close(writer_t);

/*! \brief create a new reader associated to given file. */
reader_t file_reader_new(const char*);
/*! \brief create a new reader associated to given buffer of given length. */
reader_t buffer_reader_new(const char*, word_t);
/*! \brief toggle endian swapping for the given reader. */
void reader_swap_endian(reader_t);
/*! \brief close the given reader. */
void reader_close(reader_t);

/*! write a single word */
word_t write_word(writer_t, word_t);
/*! write a character string */
word_t write_string(writer_t, const char*);

/*! read a single word */
word_t read_word(reader_t);
/*! read a character string */
const char* read_string(reader_t);

/*@}*/

#endif

