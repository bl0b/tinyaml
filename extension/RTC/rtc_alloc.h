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

#ifndef __BMUS_ALLOC_H__
#define __BMUS_ALLOC_H__


/*
MusicMessage*alloc_msg();
void free_mm(MusicMessage*);

MusicParam*alloc_par();
void free_mp(MusicParam*);

MusicPort*alloc_port();
void free_port(MusicPort*);

MusicFilter*alloc_flt();
void free_flt(MusicFilter*);

MusicMessageConstraint*alloc_mc();
void free_mc(MusicMessageConstraint*);
*/

#define BLOC_COUNT_4W  ((2048*1024)-2)
#define BLOC_COUNT_8W  ((1024*1024)-1)
#define BLOC_COUNT_16W  ((512*1024)-1)

void init_alloc();
void term_alloc();

void*_alloc_4w();
void _free_4w(void*);

void*_alloc_8w();
void _free_8w(void*);

void*_alloc_16w();
void _free_16w(void*);

#define _alloc(__type) \
	(	sizeof(__type)<=(4*sizeof(void*))?\
	 		(__type*)_alloc_4w():\
			sizeof(__type)<=(8*sizeof(void*))?\
				(__type*)_alloc_8w():\
				(__type*)_alloc_16w()\
	)
				
#define _free(__type,__p) \
	sizeof(__type)<=(4*sizeof(void*))?\
		_free_4w(__p):\
		sizeof(__type)<=(8*sizeof(void*))?\
			_free_8w(__p):\
			_free_16w(__p)





#endif

