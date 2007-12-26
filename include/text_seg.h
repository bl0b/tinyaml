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

void text_seg_init(text_seg_t seg);
const char* text_seg_find_by_text(text_seg_t, const char*);
const char* text_seg_find_by_index(text_seg_t, word_t);
word_t text_seg_text_to_index(text_seg_t, const char*);

void text_seg_serialize(text_seg_t, writer_t);
void text_seg_unserialize(text_seg_t, reader_t);

#endif

