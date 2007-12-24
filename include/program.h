#ifndef _BML_PROGRAM_H_
#define _BML_PROGRAM_H_

program_t program_new();
void program_free(program_t);
word_t program_find_string(program_t, const char*);
void program_write_code(program_t, word_t, word_t);
void program_reserve_code(program_t, word_t);
word_t program_get_code_size(program_t);
void program_fetch(program_t, word_t, word_t*, word_t*);

#endif

