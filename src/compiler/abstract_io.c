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

#include "vm_types.h"
#include "abstract_io.h"
#include <stdio.h>
#include <string.h>
#include <malloc.h>


struct _reader_t {
	word_t (*read_word)(reader_t);
	const char* (*read_string)(reader_t);
	void (*close)(reader_t);
};

struct _writer_t {
	int (*write_word)(writer_t, word_t);
	int (*write_string)(writer_t, const char*);
	void (*close)(writer_t);
};


struct _file_reader_t {
	struct _reader_t reader;
	FILE*f;
};


struct _buffer_reader_t {
	struct _reader_t reader;
	const char* data;
	const char* cursor;
	const char* end;
};


struct _file_writer_t {
	struct _writer_t writer;
	FILE*f;
};


struct _buffer_writer_t {
	struct _writer_t writer;
	char* data;
	char* cursor;
	const char* end;
};



#define _(_s,_d,_sym,_a) struct _##_s##_##_d##_t* _sym = (struct _##_s##_##_d##_t*)_a

void file_close(reader_t r) {
	_(file,reader,fr,r);
	fclose(fr->f);
}

word_t file_read_word(reader_t r) {
	_(file,reader,fr,r);
	word_t w;
	fread(&w,sizeof(word_t),1,fr->f);
	return w;
}

word_t buffer_read_word(reader_t r) {
	_(buffer,reader,br,r);
	word_t w = *(word_t*)br->cursor;
	br->cursor+=sizeof(word_t);
	return w;
}

const char* file_read_string(reader_t r) {
	static char buffy[1024];
	_(file,reader,fr,r);
	fgets(buffy,1024,fr->f);
	return buffy;
}

const char* buffer_read_string(reader_t r) {
	_(buffer,reader,br,r);
	const char* ret = br->cursor;
	br->cursor += strlen(ret);
	return ret;
}

int file_write_word(writer_t w, word_t data) {
	_(file,writer,fw,w);
	return fwrite(&data, sizeof(word_t), 1, fw->f);
}


int buffer_write_word(writer_t w, word_t data) {
	_(buffer,writer,bw,w);
	*(word_t*)bw->cursor = data;
	bw->cursor += sizeof(word_t);
	return 1;
}


int file_write_string(writer_t w, const char* data) {
	_(file,writer,fw,w);
	return fwrite(data, 1, strlen(data)+1, fw->f);
}

int buffer_write_string(writer_t w, const char* data) {
	_(buffer,writer,bw,w);
	int ret = 1+strlen(data);
	strcpy(bw->cursor,data);
	bw->cursor+=ret;
	return ret;
}


writer_t file_writer_new(const char*fname) {
	struct _file_writer_t* w = (struct _file_writer_t*)malloc(sizeof(struct _file_writer_t));
	w->f = fopen(fname,"w");
	w->writer.write_word = file_write_word;
	w->writer.write_string = file_write_string;
	w->writer.close = (void(*)(writer_t)) file_close;	/* freader and fwriter are the same */
	return (writer_t)w;
}

writer_t buffer_writer_new(char* buffer,word_t sz) {
	struct _buffer_writer_t* w = (struct _buffer_writer_t*)malloc(sizeof(struct _buffer_writer_t));
	w->data=buffer;
	w->end=buffer+sz;
	w->cursor=buffer;
	w->writer.write_word = buffer_write_word;
	w->writer.write_string = buffer_write_string;
	w->writer.close = NULL;
	return (writer_t)w;
}

void writer_close(writer_t w) {
	if(w->close) {
		w->close(w);
	}
	free(w);
}

reader_t file_reader_new(const char*fname) {
	struct _file_reader_t* w = (struct _file_reader_t*)malloc(sizeof(struct _file_reader_t));
	w->f = fopen(fname,"r");
	w->reader.read_word = file_read_word;
	w->reader.read_string = file_read_string;
	w->reader.close = file_close;
	return (reader_t)w;
}

reader_t buffer_reader_new(const char*buffer, word_t sz) {
	struct _buffer_reader_t* w = (struct _buffer_reader_t*)malloc(sizeof(struct _buffer_reader_t));
	w->data=buffer;
	w->end=buffer+sz;
	w->cursor=buffer;
	w->reader.read_word = buffer_read_word;
	w->reader.read_string = buffer_read_string;
	w->reader.close = NULL;
	return (reader_t)w;
}

void reader_close(reader_t r) {
	if(r->close) {
		r->close(r);
	}
	free(r);
}


word_t write_word(writer_t w, word_t data) {
	return w->write_word(w,data);
}

word_t write_string(writer_t w, const char* data) {
	return w->write_string(w,data);
}

word_t read_word(reader_t w) {
	return w->read_word(w);
}

const char* read_string(reader_t w) {
	return w->read_string(w);
}



