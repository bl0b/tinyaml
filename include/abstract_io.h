#ifndef _BML_ABSTRACT_IO_H_
#define _BML_ABSTRACT_IO_H_

typedef struct _reader_t* reader_t;
typedef struct _writer_t* writer_t;

writer_t file_writer_new(const char*);
writer_t buffer_writer_new(const char**);
void writer_close(writer_t);

reader_t file_reader_new(const char*);
reader_t buffer_reader_new(const char**);
void reader_close(reader_t);

word_t write_word(writer_t, word_t);
word_t write_string(writer_t, const char*);

word_t read_word(reader_t);
const char* read_string(reader_t);


#endif

