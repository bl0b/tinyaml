/***************************************************************************
 *            alloc.c
 *  damien.leroux@gmail.com
 ****************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#include "list.h"
#include "rtc_alloc.h"

#include <stdio.h>
#include <stdlib.h>
#include<pthread.h>

pthread_mutex_t allocmutex4;
pthread_mutex_t allocmutex8;
pthread_mutex_t allocmutex16;

typedef unsigned char _qw[16];
typedef unsigned char _ow[32];
typedef unsigned char _para_t[64];

typedef _qw* qw[4];
typedef _ow* ow[8];
typedef _para_t* para_t[16];

static void* quawords=NULL;
static size_t qw_free=0;
static size_t qw_total=0;
static void* octwords=NULL;
static size_t ow_free=0;
static size_t ow_total=0;
static void* paragraphs=NULL;
static size_t para_free=0;
static size_t para_total=0;

GenericList qWordBufs={NULL,NULL,0},oWordBufs={NULL,NULL,0},paraBufs={NULL,NULL,0};

#define _offset(_p,_o,_sz) (((char*)(_p))+(_o)*(_sz))

static volatile int alloc_is_init=0;

void term_alloc() {
	GenericListNode* gln;
	if(!alloc_is_init) {
		return;
	}
	/*vm_printf("freeing alloc blocs.\n");*/
	while(qWordBufs.head) {
		gln=qWordBufs.head->next;
		free(qWordBufs.head);
		qWordBufs.head=gln;
	}
	while(oWordBufs.head) {
		gln=oWordBufs.head->next;
		free(oWordBufs.head);
		oWordBufs.head=gln;
	}
	while(paraBufs.head) {
		gln=paraBufs.head->next;
		free(paraBufs.head);
		paraBufs.head=gln;
	}
	alloc_is_init=0;
}



static inline void* __new_buf(size_t size,size_t countPerBuf,GenericList*l,void**first,size_t*total,size_t*free__) {
	char*ptr;
	void*p;
	int i;
//	vm_printf("__new_buf(%i,%i)\n",size,countPerBuf);
	listAddTail(*l,(GenericListNode*)*first);
	ptr=((char*)*first)+sizeof(GenericListNode);
	*first=(void*)(ptr+size);
	*total+=countPerBuf;
	--countPerBuf;
	*free__+=countPerBuf;
	p=*first;
	--countPerBuf;
	for(i=0;i<countPerBuf;i++) {
//		vm_printf("%p, ",_offset(p,i+1,size));
		*(void**)_offset(p,i,size)=_offset(p,i+1,size);
//		vm_printf("%p, ",*(void**)_offset(p,i,size));
	}
//	vm_printf("NULL.\n");
	*(void**)_offset(p,i,size)=NULL;
//	vm_printf(" [%p]\n",ptr);fflush(stdout);
	return (void*)ptr;
}

/*static inline*/ void* __allocate_(pthread_mutex_t*mtx,size_t size,size_t countPerBuf,GenericList*l,void**first,size_t*total,size_t*free__) {
	char*ptr;
//	vm_printf("__allocator_(%i)",size);fflush(stdout);
	pthread_mutex_lock(mtx);
	if(!*first) {
		if((*first=malloc(countPerBuf*size+sizeof(GenericListNode)))) {
			ptr=(char*)__new_buf(size,countPerBuf,l,first,total,free__);
			pthread_mutex_unlock(mtx);
//			vm_printf(" [%p->%p]\n",*first,**(void***)first);fflush(stdout);
//			vm_printf(" [%p]\n",ptr);fflush(stdout);
			return (void*)ptr;
		}
	}
	ptr=(char*)*first;
	if(ptr) {
//		vm_printf(" [%p->%p]\n",*first,**(void***)first);fflush(stdout);
		*first=**(void***)first;
		--*free__;
	}
//	vm_printf(" [%p]\n",ptr);fflush(stdout);
	pthread_mutex_unlock(mtx);
	return (void*)ptr;
}

/*static inline*/
void __collect_(pthread_mutex_t*mtx,void*ptr,GenericList*l,void**first) {
	pthread_mutex_lock(mtx);
//	vm_printf("collect %p (first=%p, next=%p)\n",ptr,*first,*(void**)*first);
	if(!ptr) return;
	*(void**)ptr=*first;
	*first=ptr;
//	vm_printf("after collecting %p : first=%p, next=%p\n",ptr,*first,*(void**)*first);
	pthread_mutex_unlock(mtx);
}


unsigned int allocBlocSize=1048576;

#define SIZE(n) (n*sizeof(void*))
#define COUNT(n) ((allocBlocSize-sizeof(GenericListNode))/SIZE(n))

void*_alloc_4w() { return __allocate_(&allocmutex4,SIZE(4),COUNT(4),&qWordBufs,&quawords,&qw_total,&qw_free); }
void*_alloc_8w() { return __allocate_(&allocmutex8,SIZE(8),COUNT(8),&oWordBufs,&octwords,&ow_total,&ow_free); }
void*_alloc_16w() { return __allocate_(&allocmutex16,SIZE(16),COUNT(16),&paraBufs,&paragraphs,&para_total,&para_free); }

void _free_4w(void*p) { __collect_(&allocmutex4,p,&qWordBufs,&quawords); }
void _free_8w(void*p) { __collect_(&allocmutex8,p,&oWordBufs,&octwords); }
void _free_16w(void*p) { __collect_(&allocmutex16,p,&paraBufs,&paragraphs); }


void init_alloc() {
	if(alloc_is_init) {
		return;
	}
	alloc_is_init=1;
	pthread_mutex_init(&allocmutex4,NULL);
	/*pthread_mutex_trylock(&allocmutex4);*/
	/*pthread_mutex_unlock(&allocmutex4);*/
	pthread_mutex_init(&allocmutex8,NULL);
	/*pthread_mutex_trylock(&allocmutex8);*/
	/*pthread_mutex_unlock(&allocmutex8);*/
	pthread_mutex_init(&allocmutex16,NULL);
	/*pthread_mutex_trylock(&allocmutex16);*/
	/*pthread_mutex_unlock(&allocmutex16);*/

	/*atexit(term_alloc);*/
}






/*	para_t*ptr;
	int i;
	
	if(!paragraphs) {
		paragraphs=(para_t*)malloc(BLOC_COUNT_16W*sizeof(para_t));
		ptr=paragraphs;
		vm_printf("\t(paragraph) (ptr+1)-ptr = %u\n",(char*)(ptr+1)-(char*)ptr);
		for(i=0;i<BLOC_COUNT_16W-1;++i,++ptr) {
			(*ptr)[0]=(_para_t*)(ptr+1);
		}
		**ptr=NULL;
		para_free=BLOC_COUNT_8W;
		para_total+=BLOC_COUNT_8W;
	}
	ptr=paragraphs;
	if(ptr) {
		paragraphs=*(para_t**)ptr;
		--para_free;
	}
	vm_printf("_alloc_16w [%p]\n",ptr);
	return ptr;*/





/*	if(!p) return;
	vm_printf("_free_16w [%p]\n",p);
	*(para_t**)p=paragraphs;
	paragraphs=p;*/










#if 0

typedef struct {
	int a,b,c,d;
} qstruc;

typedef struct {
	int a,b,c,d,e,f,g,h;
} ostruc;


int main() {
	int i;
	qstruc* testq[64];
	ostruc* testo[64];
	MusicPort*testp[64];
	for(i=0;i<64;i++) {
		testq[i]=(qstruc*)_alloc_4w();
		testo[i]=(ostruc*)_alloc_8w();
		testp[i]=(MusicPort*)_alloc_port();
		vm_printf("\t%p\t%p\n",testq[i],testo[i]);
		if(!(i%3)) {
			_free_4w(testq[i]);
			_free_8w(testo[i]);
			testq[i]=NULL;
			testo[i]=NULL;
		}
	}
	vm_printf("%u %u %u\n",qw_free,ow_free,ports_free);
	for(i=0;i<64;i++) {
		_free_4w(testq[i]);
		_free_8w(testo[i]);
		_free_port(testp[i]);
	}
	vm_printf("%u %u %u\n",qw_total,ow_total,ports_total);
	return 0;
}

#endif


