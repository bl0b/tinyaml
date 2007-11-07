#ifndef _OIC_H_
#define _OIC_H_

typedef struct _obj_id_container_t {
	size_t size;
	size_t capacity;
	struct _obj_id_t* table;
}* obj_id_container_t;

void obj_id_container_init(obj_id_container_t);
void obj_id_container_add(obj_id_container_t, id_string, obj_t);
void obj_id_container_set(obj_id_container_t, id_string, obj_t);
void obj_id_container_copy(obj_id_container_t src, obj_id_container_t dest);
size_t obj_id_container_find_index(obj_id_container_t,id_string);
const char* obj_id_container_name(obj_id_container_t,size_t);
obj_t obj_id_container_by_name(obj_id_container_t,id_string);
obj_t obj_id_container_by_index(obj_id_container_t,size_t);
void obj_id_container_remove_index(obj_id_container_t,size_t);



#endif

