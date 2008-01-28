#include "vm.h"
#include "fastmath.h"
#include "thread.h"
#include "_impl.h"
#include "object.h"

#include <string.h>
#include <stdlib.h>

#define FIFO_SZ 4096
#define FIFO_MASK (FIFO_SZ-1)

typedef struct _msg_q_t* msg_q_t;
typedef struct _msg_q_rd_t* msg_q_rd_t;

struct _msg_q_t {
	word_t magic;
	word_t wr;
	struct _data_stack_entry_t values[FIFO_SZ];
	vm_blocker_t blocker;
};

struct _msg_q_rd_t {
	word_t magic;
	struct _msg_q_t* fifo;
	word_t rd;
	thread_t owner;
};


msg_q_t mq_new();
msg_q_rd_t mq_rd_new(vm_t vm,msg_q_t,thread_t);

void mq_deinit(vm_t vm, msg_q_t mq) {
	int i;
	/*printf("mq_deinit\n"); fflush(stdout);*/
	for(i=0;i<FIFO_SZ;i+=1) {
		if(mq->values[i].type&DataManagedObjectFlag) {
			vm_obj_deref_ptr(vm,(void*)mq->values[i].data);
		}
	}
	blocker_free(mq->blocker);
}

void* mq_clone(vm_t vm, msg_q_t mq) {
	int i;
	msg_q_t ret = mq_new();
	memcpy(ret->values,mq->values,sizeof(struct _data_stack_entry_t)*FIFO_SZ);
	for(i=0;i<FIFO_SZ;i+=1) {
		if(ret->values[i].type&DataManagedObjectFlag) {
			vm_obj_ref_ptr(vm,(void*)ret->values[i].data);
		}
	}
	ret->wr = mq->wr;
	return ret;
}

msg_q_t mq_new() {
	msg_q_t mq = (msg_q_t) vm_obj_new(sizeof(struct _msg_q_t),
			(void (*)(vm_t,void*)) mq_deinit,
			(void*(*)(vm_t,void*)) mq_clone,
			DataObjUser);
	/*printf("mq_new %p\n",mq);*/
	memset(mq,0,sizeof(struct _msg_q_t));
	mq->magic = 0x6106F1F0;
	mq->blocker = blocker_new();
	return mq;
}

void mq_rd_deinit(vm_t vm, msg_q_rd_t mqr) {
	/*printf("mq_rd_deinit\n"); fflush(stdout);*/
	vm_obj_deref_ptr(vm,mqr->owner);
	/*vm_obj_deref_ptr(vm,mqr->fifo);*/
}

void* mq_rd_clone(vm_t vm,msg_q_rd_t mqr) {
	msg_q_rd_t ret = mq_rd_new(vm,mqr->fifo,mqr->owner);
	ret->rd = mqr->rd;
	return ret;
}

msg_q_rd_t mq_rd_new(vm_t vm, msg_q_t fifo, thread_t owner) {
	msg_q_rd_t mqr = (msg_q_rd_t) vm_obj_new(sizeof(struct _msg_q_rd_t),
				(void (*)(vm_t,void*)) mq_rd_deinit,
				(void*(*)(vm_t,void*)) mq_rd_clone,
				DataObjUser);
	/*printf("mq_rd_new %p\n",mqr);*/
	mqr->fifo = fifo;
	mqr->owner= owner;
	mqr->rd=fifo->wr;
	mqr->magic = 0x6106F1F1;
	vm_obj_ref_ptr(vm,owner);
	/*vm_obj_ref_ptr(vm,fifo);*/
	return mqr;
}

void mq_wr(vm_t vm, msg_q_t mq, vm_data_t d) {
	vm_data_t old = mq->values+mq->wr;
	if(old->type&DataManagedObjectFlag) {
		vm_obj_deref_ptr(vm,(void*)old->data);
	}
	memcpy(old,d,sizeof(struct _data_stack_entry_t));
	if(old->type&DataManagedObjectFlag) {
		vm_obj_ref_ptr(vm,(void*)old->data);
	}
	blocker_resume(vm,mq->blocker);
	mq->wr=(mq->wr+1)&FIFO_MASK;
}

vm_data_t mq_rd(vm_t vm, msg_q_rd_t rdr) {
	if(rdr->rd!=rdr->fifo->wr) {
		vm_data_t ret = rdr->fifo->values+rdr->rd;
		rdr->rd = (rdr->rd+1)&FIFO_MASK;
		return ret;
	} else {
		blocker_suspend(vm,rdr->fifo->blocker,rdr->owner);
		return NULL;
	}
}


void _VM_CALL vm_op_msgQueueNew(vm_t vm, word_t unused) {
	msg_q_t mq = mq_new();
	vm_push_data(vm,DataObjUser,(word_t)mq);
}


void _VM_CALL vm_op_msgQueueWrite(vm_t vm, word_t unused) {
	vm_data_t d = _vm_pop(vm);
	vm_data_t q = _vm_pop(vm);
	msg_q_t mq = (msg_q_t)q->data;
	assert(q->type==DataObjUser&&mq->magic==0x6106F1F0);
	mq_wr(vm,mq,d);
}


void _VM_CALL vm_op_msgQueueReaderNew(vm_t vm, word_t unused) {
	vm_data_t q = _vm_pop(vm);
	msg_q_t mq = (msg_q_t) q->data;
	msg_q_rd_t mqr;
	assert(q->type==DataObjUser&&mq->magic==0x6106F1F0);
	mqr = mq_rd_new(vm,mq,vm->current_thread);
	vm_push_data(vm,DataObjUser,(word_t)mqr);
}

void _VM_CALL vm_op_msgQueueRead(vm_t vm, word_t unused) {
	vm_data_t r = _vm_peek(vm);
	msg_q_rd_t rdr = (msg_q_rd_t)r->data;
	vm_data_t d;
	assert(r->type==DataObjUser&&rdr->magic==0x6106F1F1);
	if((d = mq_rd(vm,rdr))) {
		vm_poke_data(vm,d->type,d->data);
	}
}


