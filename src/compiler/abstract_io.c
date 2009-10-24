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
#include "vm.h"
#include "abstract_io.h"
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>


struct _reader_t {
	long swap_endian;
	word_t (*read_word)(reader_t);
	const char* (*read_string)(reader_t);
	void (*close)(reader_t);
};

struct _writer_t {
	long __dummy__;	/* preserve aliasing between _reader_t and _writer_t */
	long (*write_word)(writer_t, word_t);
	long (*write_string)(writer_t, const char*);
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



void couldnt(const char* what, const char* where) {
	char error_buf[4096];
	sprintf(error_buf, "Couldn't %s %s (%s).", what?what:"", where?where:"", strerror(errno));
	vm_fatal(error_buf);
}





#define _(_s,_d,_sym,_a) struct _##_s##_##_d##_t* _sym = (struct _##_s##_##_d##_t*)_a

void file_close(reader_t r) {
	_(file,reader,fr,r);
	fclose(fr->f);
}

word_t file_read_word(reader_t r) {
	_(file,reader,fr,r);
	word_t w;
	if(fread(&w,1,sizeof(word_t),fr->f)!=sizeof(word_t)) {
		vm_printf("READ WORD :: FAILURE\n");
	}
	/*vm_printf("file_reader read word %8.8lX\n",w);*/
	return w;
}

word_t buffer_read_word(reader_t r) {
	_(buffer,reader,br,r);
	word_t w = *(word_t*)br->cursor;
	br->cursor+=sizeof(word_t);
	return w;
}

#define BUFFY_SZ 65536
static char _rdr_buffy[BUFFY_SZ];

const char* file_read_string(reader_t r) {
	char c=1;
	long i=0;
	_(file,reader,fr,r);
	memset(_rdr_buffy,0,BUFFY_SZ);
	while(i<BUFFY_SZ&&c!=0) {
		if(fread(_rdr_buffy+i,1,1,fr->f)!=1) {
			couldnt("read", "");
		}
		c=*(_rdr_buffy+i);
		i+=1;
	}
	/*vm_printf("file_reader read string (%i) \"%s\"\n",i,buffy);*/
	return _rdr_buffy;
}
#undef BUFFY_SZ

const char* buffer_read_string(reader_t r) {
	_(buffer,reader,br,r);
	const char* ret = br->cursor;
	br->cursor += strlen(ret);
	return ret;
}

long file_write_word(writer_t w, word_t data) {
	_(file,writer,fw,w);
	return fwrite(&data, sizeof(word_t), 1, fw->f);
}


long buffer_write_word(writer_t w, word_t data) {
	_(buffer,writer,bw,w);
	*(word_t*)bw->cursor = data;
	bw->cursor += sizeof(word_t);
	return 1;
}


long file_write_string(writer_t w, const char* data) {
	_(file,writer,fw,w);
	return fwrite(data, 1, strlen(data)+1, fw->f);
}

long buffer_write_string(writer_t w, const char* data) {
	_(buffer,writer,bw,w);
	long ret = 1+strlen(data);
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
	w->reader.swap_endian=0;
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
	w->reader.swap_endian=0;
	return (reader_t)w;
}

void reader_swap_endian(reader_t r) {
	r->swap_endian=1;
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

#define _swap(_x,_y) do { _x^=_y; _y^=_x; _x^=_y; } while(0)

word_t read_word(reader_t w) {
	union { word_t w; char c[4]; } ret;
	ret.w = w->read_word(w);
	if(w->swap_endian) {
		_swap(ret.c[0],ret.c[3]);
		_swap(ret.c[1],ret.c[2]);
	}
	return ret.w;
}

const char* read_string(reader_t w) {
	return w->read_string(w);
}



