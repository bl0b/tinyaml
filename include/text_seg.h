
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

