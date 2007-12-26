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
#include "list.h"

#include <stdlib.h>

void slist_init(slist_t l) {
	l->head=l->tail=NULL;
}

void dlist_init(dlist_t l) {
	l->head=l->tail=NULL;
}

slist_t slist_new() {
	slist_t ret = malloc(sizeof(struct _slist_t));
	slist_init(ret);
	return ret;
}

void slist_del(slist_t l) {
	slist_flush(l);
	free(l);
}

dlist_t dlist_new() {
	dlist_t ret = malloc(sizeof(struct _dlist_t));
	dlist_init(ret);
	return ret;
}

void dlist_del(dlist_t l) {
	dlist_flush(l);
	free(l);
}



