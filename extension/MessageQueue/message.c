#include "vm.h"
#include "fastmath.h"
#include "thread.h"
#include "_impl.h"
#include "object.h"

#include <string.h>
#include <stdlib.h>

/* Since the tutorial points here to demonstrate how to define
 * new managed user types, AND how to implement blocking
 * opcodes, this file is heavily commented.
 */

/* Our new types are :
 * - a message queue implemented as a FIFO,
 * - a message queue reader.
 */
#define FIFO_SZ 4096		/* hold 4096 messages at most */
#define FIFO_MASK (FIFO_SZ-1)

typedef struct _msg_q_t* msg_q_t;	/* our public message queue type */
typedef struct _msg_q_rd_t* msg_q_rd_t;	/* our public message queue reader type */

struct _msg_q_t {
	word_t magic;	/* it's best practice to define a magic value as first field, so we can check the type without breaking anything when we pop a DataObjUser value */
	word_t wr;	/* the write head */
	struct _data_stack_entry_t values[FIFO_SZ]; /* the FIFO */
	vm_blocker_t blocker;	/* a thread blocker to help implement blocking opcodes */
};

struct _msg_q_rd_t {
	word_t magic;	/* as I said, magic value as first field */
	struct _msg_q_t* fifo;	/* which queue this reader reads */
	word_t rd;	/* the read head */
	thread_t owner;	/* which tinyaml thread this reader belongs to (to implement blocking opcodes) */
};


msg_q_t mq_new();				/* constructor */
msg_q_rd_t mq_rd_new(vm_t vm,msg_q_t,thread_t);	/* constructor */

/* There are TWO routines one has to write to implement a new managed type :
 * - deinit(vm_t, void*) : called when an object is collected (DON'T free the object buffer),
 * - clone(vm_t, void*) : called when the VM requires to copy an object (when enclosing a value for instance).
 */

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

/* There has to be a new(..?) routine to create new instances */
msg_q_t mq_new() {
	/* vm_obj_new handles creation of a new managed object.
	 * Its arguments are :
	 * - object buffer size to allocate,
	 * - pointer to deinit routine,
	 * - pointer to clone routine,
	 * - data type (DataObjUser)
	 */
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

/* Now these are helper routines to handle reading from and writing to FIFOs */

void mq_wr(vm_t vm, msg_q_t mq, vm_data_t d) {
	vm_data_t old = mq->values+mq->wr;
	if(old->type&DataManagedObjectFlag) {
		/* before overwriting a slot where there is a managed object,
		 * update its reference counter (i.e. decrement it)
		 */
		vm_obj_deref_ptr(vm,(void*)old->data);
	}
	memcpy(old,d,sizeof(struct _data_stack_entry_t));
	if(old->type&DataManagedObjectFlag) {
		/* if the new message in the current slot is a managed object,
		 * update its reference counter (i.e. increment it)
		 */
		vm_obj_ref_ptr(vm,(void*)old->data);
	}
	/* resume any thread that was waiting for a message to arrive */
	blocker_resume(vm,mq->blocker);
	/* and move write head to next slot */
	mq->wr=(mq->wr+1)&FIFO_MASK;
}

vm_data_t mq_rd(vm_t vm, msg_q_rd_t rdr) {
	/* if there is at least ONE message pending, */
	if(rdr->rd!=rdr->fifo->wr) {
		vm_data_t ret = rdr->fifo->values+rdr->rd;
		rdr->rd = (rdr->rd+1)&FIFO_MASK;
		/* consume and return it. */
		return ret;
	} else {
		/* otherwise, just block the current thread, until there IS a message */
		blocker_suspend(vm,rdr->fifo->blocker,rdr->owner);
		return NULL;
	}
}


/*
 * And now, Ladies and Gentlemen, here come... the ooopcooooodes !
 */

/* Create a new message queue
 * Stack in : -
 * Stack out : message queue
 */
void _VM_CALL vm_op_msgQueueNew(vm_t vm, word_t unused) {
	msg_q_t mq = mq_new();
	vm_push_data(vm,DataObjUser,(word_t)mq);
}

/* Write to a message queue
 * Stack in : any
 * Stack out : -
 */
void _VM_CALL vm_op_msgQueueWrite(vm_t vm, word_t unused) {
	vm_data_t d = _vm_pop(vm);
	vm_data_t q = _vm_pop(vm);
	msg_q_t mq = (msg_q_t)q->data;
	assert(q->type==DataObjUser&&mq->magic==0x6106F1F0);
	mq_wr(vm,mq,d);
}

/* Create a new message queue reader
 * Stack in : message queue
 * Stack out : message queue reader
 */
void _VM_CALL vm_op_msgQueueReaderNew(vm_t vm, word_t unused) {
	vm_data_t q = _vm_pop(vm);
	msg_q_t mq = (msg_q_t) q->data;
	msg_q_rd_t mqr;
	assert(q->type==DataObjUser&&mq->magic==0x6106F1F0);
	mqr = mq_rd_new(vm,mq,vm->current_thread);
	vm_push_data(vm,DataObjUser,(word_t)mqr);
}

/* Read from a message queue. MAY BLOCK THREAD IF NO MESSAGE PENDING.
 * Stack in : message queue reader
 * Stack out : any
 */
void _VM_CALL vm_op_msgQueueRead(vm_t vm, word_t unused) {
	/* Handling a possible thread block :
	 * - Don't pop data. Just peek.
	 * - Pop when there is no blocking.
	 * Blocking a thread will maintain its instruction pointer at its current position.
	 * Which means that when the VM resumes the thread, the very same instruction will
	 * be executed again.
	 * So peek data, check whether the thread has to be blocked or not, and only
	 * modify state (i.e. pop inputs and push outputs) when the thread hasn't to be blocked.
	 */
	vm_data_t r = _vm_peek(vm);
	msg_q_rd_t rdr = (msg_q_rd_t)r->data;
	vm_data_t d;
	assert(r->type==DataObjUser&&rdr->magic==0x6106F1F1);
	if((d = mq_rd(vm,rdr))) {
		/* Since we didn't actually pop the input, and we have exactly one input and one output,
		 * we may as well poke data at top of stack instead of popping then pushing.
		 */
		vm_poke_data(vm,d->type,d->data);
	}
}


