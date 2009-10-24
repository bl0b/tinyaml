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

/* everything already included by main include file.h */

static inline word_t _read_bin_word(file_t f) {
	word_t ret;
	if(fread(&ret, sizeof(word_t), 1, f->descr.f)!=sizeof(word_t)) {
		vm_fatal("Can't read word.");
	}
	return ret;
}

static inline unsigned char _read_byte(file_t f) {
	unsigned char ret;
	if(fread(&ret, 1, 1, f->descr.f)!=1) {
		vm_fatal("Can't read byte.");
	}
	return ret;
}


#define BUFSHIFT 15
#define BUFSZ (1<<BUFSHIFT)

long _VM_CALL _fill_buf0(FILE*f, char*buf);
long _VM_CALL _fill_bufNL(FILE*f, char*buf);


static inline char* _read_s(FILE*f, long _VM_CALL(*_filler)(FILE*, char*)) {
	/* optimize for short strings (1 buffer) */
	char _buf[BUFSZ];
	char* buf, * ret;
	long len, i, N;
	dynarray_t buffers;
	_buf[0]=0;
	if(BUFSZ==_filler(f, _buf)) {
		buffers = dynarray_new();
		/*dynarray_set(buffers, 0, (word_t)_buf);*/
		N=0;
		do {
			N+=1;
			buf = (char*)malloc(BUFSZ);
			buf[0]=0;
			dynarray_set(buffers, N, (word_t)buf);
			len=_filler(f, buf);
		} while(len==BUFSZ);
		ret = vm_string_new_buf((N<<BUFSHIFT)+len+1);
		ret[(N<<BUFSHIFT)+len]=0;
		memcpy(ret, _buf, BUFSZ);
		for(i=1;i<N;i+=1) {
			memcpy(ret+(i<<BUFSHIFT), (char*)dynarray_get(buffers, i), BUFSZ);
			free((void*)dynarray_get(buffers, i));
		}
		memcpy(ret+(i<<BUFSHIFT), (char*)dynarray_get(buffers, i), len);
		dynarray_del(buffers);
		return ret;
	} else {
		return vm_string_new(_buf);
	}
}


static inline char* _read_binstr(file_t f) {
	return _read_s(f->descr.f, _fill_buf0);
}

static inline char* _read_str(file_t f) {
	return _read_s(f->descr.f, _fill_bufNL);
}

static inline long _read_int(file_t f) {
	long ret;
	if(fscanf(f->descr.f, "%li", &ret)!=1) {
		vm_fatal("Couldn't read long.");
	}
	return ret;
}

static inline word_t _read_hexint(file_t f) {
	word_t ret;
	if(fscanf(f->descr.f, "%lX", &ret)!=1) {
		vm_fatal("Couldn't read hexadecimal long.");
	}
	return ret;
}

static inline float _read_float(file_t f) {
	float ret;
	if(fscanf(f->descr.f, "%f", (float*)&ret)!=1) {
		vm_fatal("Couldn't read float.");
	}
	return ret;
}

static inline word_t _unpack(file_t f, char fmt) {
	union { long i; word_t w; float f; const char* s; } conv;
	switch(fmt) {
	case 'S' : conv.s = _read_str(f); break;
	case 'I' : conv.i = _read_int(f); break;
	case 'X' : conv.w = _read_hexint(f); break;
	case 'F' : conv.f = _read_float(f); break;
	case 'C' :
	case 'b' : conv.w = _read_byte(f); break;
	case 's' : conv.s = _read_binstr(f); break;
	case 'i' : /* see below */
	case 'f' : conv.w = _read_bin_word(f); break;
	default: vm_fatal("Unhandled format character.");
	};
	return conv.w;
}

