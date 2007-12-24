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



