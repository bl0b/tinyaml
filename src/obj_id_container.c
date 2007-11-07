#include <string.h>
#include "obj.h"

/****************************************************
 * OBJ ID CONTAINER
 ***************************************************/

void obj_id_container_init(obj_id_container_t oic) {
	memset(oic,0,sizeof(struct _obj_id_container_t));
}

void obj_id_container_add(obj_id_container_t oic, id_string key, obj_t value) {
	if(oic->size==oic->capacity) {
		oic->capacity+=REALLOC_GRANUL;
		oic->table = (obj_id_t)realloc(oic->table,oic->capacity*sizeof(struct _obj_id_t));
	}
	oic->table[oic->size].sym = key?strdup(key):NULL;
	oic->table[oic->size].obj = value;
	oic->size += 1;
}

void obj_id_container_set(obj_id_container_t oic, id_string key, obj_t value) {
	int i;
	for(i=0;i<oic->size&&strcmp(oic->table[i].sym,key);i++);
	if(i==oic->size) {
		obj_id_container_add(oic,key,value);
	} else {
		oic->table[i].obj = value;
	}
}



size_t  obj_id_container_find_index(obj_id_container_t oic,id_string name) {
	int i;
	for(i=0;i<oic->size;i+=1) {
		if(!strcmp(oic->table[i].sym,name)) {
			return i;
		}
	}
	return -1;
}

obj_t obj_id_container_by_name(obj_id_container_t oic, id_string name) {
	int i;
	obj_id_t id;
	for(i=0;i<oic->size;i+=1) {
		id=oic->table+i;
		if(id->sym&&!strcmp(id->sym,name)) {
			return id->obj;
		}
	}
	return NULL;
}

obj_t obj_id_container_by_index(obj_id_container_t oic, size_t index) {
	return oic->table[index].obj;
}

void obj_id_container_remove_index(obj_id_container_t oic,size_t index) {
	/* FIXME : removed object is never freed ! */
	if(index<oic->size-1) {
		memmove(oic->table+index,
			oic->table+index+1,
			(oic->size-index-1)*sizeof(struct _obj_id_t));
	}
	oic->size -= 1;
}

const char* obj_id_container_name(obj_id_container_t oic,size_t index) {
	return oic->table[index].sym;
}


