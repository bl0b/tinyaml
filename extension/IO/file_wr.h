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

static inline void _write_bin_word(file_t f, word_t data) {
	fwrite(&data, sizeof(word_t), 1, f->descr.f);
}

static inline void _write_byte(file_t f, char data) {
	fwrite(&data, 1, 1, f->descr.f);
}

static inline void _write_binstr(file_t f, const char* str) {
	fwrite(str, 1, strlen(str)+1, f->descr.f);
}

static inline void _write_str(file_t f, const char* str) {
	fwrite(str, 1, strlen(str), f->descr.f);
}

static inline void _write_int(file_t f, long data) {
	fprintf(f->descr.f, "%li", data);
}

static inline void _write_hexint(file_t f, word_t w) {
	fprintf(f->descr.f, "%lX", w);
}

static inline void _write_float(file_t f, float data) {
	fprintf(f->descr.f, "%f", data);
}

static inline void _pack(file_t f, char fmt, word_t data) {
	union { word_t w; float f; } conv;
	switch(fmt) {
	case 'S' : _write_str(f, (const char*)data); break;
	case 'I' : _write_int(f, data); break;
	case 'X' : _write_hexint(f, data); break;
	case 'F' : conv.w = data; _write_float(f, conv.f); break;
	case 'C' :
	case 'b' : _write_byte(f, (unsigned char)data); break;
	case 's' : _write_binstr(f, (const char*)data); break;
	case 'i' : /* see below */
	case 'f' : _write_bin_word(f, data); break;
	default: vm_fatal("Unhandled format character.");
	};
}

